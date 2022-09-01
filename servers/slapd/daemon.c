/* $ReOpenLDAP$ */
/* Copyright 1990-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
 * All rights reserved.
 *
 * This file is part of ReOpenLDAP.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "reldap.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include "slap.h"
#include "ldap_pvt_thread.h"
#include "lutil.h"

#include "ldap_rq.h"

#ifdef HAVE_POLL
#include <poll.h>
#endif

/* LY: needed by config_keepalive() */
#include "slapconfig.h"

/* LY: for ldap_pvt_tcpkeepalive() from libreldap */
#include "../../libraries/libreldap/ldap-int.h"

#if defined(HAVE_SYS_EPOLL_H) && defined(HAVE_EPOLL)
#include <sys/epoll.h>
#elif defined(SLAP_X_DEVPOLL) && defined(HAVE_SYS_DEVPOLL_H) &&                \
    defined(HAVE_DEVPOLL)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/devpoll.h>
#endif /* ! epoll && ! /dev/poll */

#ifdef HAVE_TCPD
int allow_severity = LOG_INFO;
int deny_severity = LOG_NOTICE;
#endif /* TCP Wrappers */

#ifdef LDAP_PF_LOCAL
#include <sys/stat.h>
/* this should go in <ldap.h> as soon as it is accepted */
#define LDAPI_MOD_URLEXT "x-mod"
#endif /* LDAP_PF_LOCAL */

#ifdef LDAP_PF_INET6
int slap_inet4or6 = AF_UNSPEC;
#else  /* ! INETv6 */
int slap_inet4or6 = AF_INET;
#endif /* ! INETv6 */

/* globals */
slap_time_t starttime;
ber_socket_t dtblsize;
slap_ssf_t local_ssf = LDAP_PVT_SASL_LOCAL_SSF;
struct runqueue_s slapd_rq;

struct {
  int idle, probes, interval;
} slapd_tcpkeepalive = {-1, -1, -1};

#ifndef SLAPD_MAX_DAEMON_THREADS
#define SLAPD_MAX_DAEMON_THREADS 16
#endif
int slapd_daemon_threads = 1;
int slapd_daemon_mask;

#ifdef LDAP_TCP_BUFFER
int slapd_tcp_rmem;
int slapd_tcp_wmem;
#endif /* LDAP_TCP_BUFFER */

Listener **slap_listeners = NULL;
static volatile sig_atomic_t listening = 1; /* 0 when slap_listeners closed */
static ldap_pvt_thread_t *listener_tid;

#ifndef SLAPD_LISTEN_BACKLOG
#define SLAPD_LISTEN_BACKLOG 1024
#endif /* ! SLAPD_LISTEN_BACKLOG */

#define DAEMON_ID(fd) (fd & slapd_daemon_mask)

static ber_socket_t wake_sds[SLAPD_MAX_DAEMON_THREADS][2];
static int emfile;

#ifdef SLAPD_WRITE_TIMEOUT
static slap_time_t chk_writetime;
#endif /* SLAPD_WRITE_TIMEOUT */

static volatile int waking;
#ifdef NO_THREADS
#define WAKE_LISTENER(l, w)                                                    \
  do {                                                                         \
    if ((w) && ++waking < 5) {                                                 \
      int ignore __maybe_unused = tcp_write(wake_sds[l][1], "0", 1);           \
    }                                                                          \
  } while (0)
#else /* ! NO_THREADS */
#define WAKE_LISTENER(l, w)                                                    \
  do {                                                                         \
    if (w) {                                                                   \
      int ignore __maybe_unused = tcp_write(wake_sds[l][1], "0", 1);           \
    }                                                                          \
  } while (0)
#endif /* ! NO_THREADS */

#ifdef __SANITIZE_THREAD__

static volatile sig_atomic_t _slapd_shutdown = 0;
static volatile sig_atomic_t _slapd_gentle_shutdown = 0;
static volatile sig_atomic_t _slapd_abrupt_shutdown = 0;

ATTRIBUTE_NO_SANITIZE_THREAD
int get_shutdown() { return _slapd_shutdown; }

ATTRIBUTE_NO_SANITIZE_THREAD
int get_gentle_shutdown() { return _slapd_gentle_shutdown; }

ATTRIBUTE_NO_SANITIZE_THREAD
int get_abrupt_shutdown() { return _slapd_abrupt_shutdown; }

ATTRIBUTE_NO_SANITIZE_THREAD
int set_shutdown(int v) { return _slapd_shutdown = v; }

ATTRIBUTE_NO_SANITIZE_THREAD
int set_gentle_shutdown(int v) { return _slapd_gentle_shutdown = v; }

ATTRIBUTE_NO_SANITIZE_THREAD
int set_abrupt_shutdown(int v) { return _slapd_abrupt_shutdown = v; }

#else

volatile sig_atomic_t _slapd_shutdown = 0;
volatile sig_atomic_t _slapd_gentle_shutdown = 0;
volatile sig_atomic_t _slapd_abrupt_shutdown = 0;

#endif /* __SANITIZE_THREAD__ */

#ifdef HAVE_TCPD
static ldap_pvt_thread_mutex_t sd_tcpd_mutex;
#endif /* TCP Wrappers */

#ifdef __SANITIZE_THREAD__
static pthread_mutex_t tsan_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

typedef struct slap_daemon_st {
  ldap_pvt_thread_mutex_t sd_mutex;

  int sd_nactives;
  int sd_nwriters;
  int sd_nfds;

#if defined(HAVE_EPOLL)
  struct epoll_event *sd_epolls;
  int *sd_index;
  int sd_epfd;
#elif defined(SLAP_X_DEVPOLL) && defined(HAVE_DEVPOLL)
  /* eXperimental */
  struct pollfd *sd_pollfd;
  int *sd_index;
  Listener **sd_l;
  int sd_dpfd;
#else  /* ! epoll && ! /dev/poll */
  fd_set sd_actives;
  fd_set sd_readers;
  fd_set sd_writers;
#endif /* ! epoll && ! /dev/poll */
} slap_daemon_st;

static slap_daemon_st slap_daemon[SLAPD_MAX_DAEMON_THREADS];

/*
 * NOTE: naming convention for macros:
 *
 * - SLAP_SOCK_* and SLAP_EVENT_* for public interface that deals
 *   with file descriptors and events respectively
 *
 * - SLAP_<type>_* for private interface; type by now is one of
 *   EPOLL, DEVPOLL, SELECT
 *
 * private interface should not be used in the code.
 */
#if defined(HAVE_EPOLL)
/***************************************
 * Use epoll infrastructure - epoll(4) *
 ***************************************/
#define SLAP_EVENT_FNAME "epoll"
#define SLAP_EVENTS_ARE_INDEXED 0
#define SLAP_EPOLL_SOCK_IX(t, s) (slap_daemon[t].sd_index[(s)])
#define SLAP_EPOLL_SOCK_EP(t, s)                                               \
  (slap_daemon[t].sd_epolls[SLAP_EPOLL_SOCK_IX(t, s)])
#define SLAP_EPOLL_SOCK_EV(t, s) (SLAP_EPOLL_SOCK_EP(t, s).events)
#define SLAP_SOCK_IS_ACTIVE(t, s) (SLAP_EPOLL_SOCK_IX(t, s) != -1)
#define SLAP_SOCK_NOT_ACTIVE(t, s) (SLAP_EPOLL_SOCK_IX(t, s) == -1)
#define SLAP_EPOLL_SOCK_IS_SET(t, s, mode) (SLAP_EPOLL_SOCK_EV(t, s) & (mode))

#define SLAP_SOCK_IS_READ(t, s) SLAP_EPOLL_SOCK_IS_SET(t, (s), EPOLLIN)
#define SLAP_SOCK_IS_WRITE(t, s) SLAP_EPOLL_SOCK_IS_SET(t, (s), EPOLLOUT)

#define SLAP_EPOLL_SOCK_SET(t, s, mode)                                        \
  do {                                                                         \
    if ((SLAP_EPOLL_SOCK_EV(t, s) & (mode)) != (mode)) {                       \
      SLAP_EPOLL_SOCK_EV(t, s) |= (mode);                                      \
      epoll_ctl(slap_daemon[t].sd_epfd, EPOLL_CTL_MOD, (s),                    \
                &SLAP_EPOLL_SOCK_EP(t, s));                                    \
    }                                                                          \
  } while (0)

#define SLAP_EPOLL_SOCK_CLR(t, s, mode)                                        \
  do {                                                                         \
    if ((SLAP_EPOLL_SOCK_EV(t, s) & (mode))) {                                 \
      SLAP_EPOLL_SOCK_EV(t, s) &= ~(mode);                                     \
      epoll_ctl(slap_daemon[t].sd_epfd, EPOLL_CTL_MOD, s,                      \
                &SLAP_EPOLL_SOCK_EP(t, s));                                    \
    }                                                                          \
  } while (0)

#define SLAP_SOCK_SET_READ(t, s) SLAP_EPOLL_SOCK_SET(t, s, EPOLLIN)
#define SLAP_SOCK_SET_WRITE(t, s) SLAP_EPOLL_SOCK_SET(t, s, EPOLLOUT)

#define SLAP_SOCK_CLR_READ(t, s) SLAP_EPOLL_SOCK_CLR(t, (s), EPOLLIN)
#define SLAP_SOCK_CLR_WRITE(t, s) SLAP_EPOLL_SOCK_CLR(t, (s), EPOLLOUT)

#define SLAP_SOCK_SET_SUSPEND(t, s)                                            \
  (slap_daemon[t].sd_suspend[SLAP_EPOLL_SOCK_IX(t, s)] = 1)
#define SLAP_SOCK_CLR_SUSPEND(t, s)                                            \
  (slap_daemon[t].sd_suspend[SLAP_EPOLL_SOCK_IX(t, s)] = 0)
#define SLAP_SOCK_IS_SUSPEND(t, s)                                             \
  (slap_daemon[t].sd_suspend[SLAP_EPOLL_SOCK_IX(t, s)] == 1)

#define SLAP_EPOLL_EVENT_CLR(i, mode) (revents[(i)].events &= ~(mode))

#define SLAP_EVENT_MAX(t) slap_daemon[t].sd_nfds

/* If a Listener address is provided, store that as the epoll data.
 * Otherwise, store the address of this socket's slot in the
 * index array. If we can't do this add, the system is out of
 * resources and we need to shutdown.
 */
#define SLAP_SOCK_ADD(t, s, l)                                                 \
  do {                                                                         \
    int rc;                                                                    \
    SLAP_EPOLL_SOCK_IX(t, (s)) = slap_daemon[t].sd_nfds;                       \
    SLAP_EPOLL_SOCK_EP(t, (s)).data.ptr =                                      \
        (l) ? (l) : (void *)(&SLAP_EPOLL_SOCK_IX(t, s));                       \
    SLAP_EPOLL_SOCK_EV(t, (s)) = EPOLLIN;                                      \
    rc = epoll_ctl(slap_daemon[t].sd_epfd, EPOLL_CTL_ADD, (s),                 \
                   &SLAP_EPOLL_SOCK_EP(t, (s)));                               \
    if (rc == 0) {                                                             \
      slap_daemon[t].sd_nfds++;                                                \
    } else {                                                                   \
      Debug(LDAP_DEBUG_ANY,                                                    \
            "daemon: epoll_ctl(ADD,fd=%d) failed, errno=%d, shutting down\n",  \
            s, errno);                                                         \
      set_shutdown(SHUT_RDWR);                                                 \
    }                                                                          \
  } while (0)

#define SLAP_EPOLL_EV_LISTENER(t, ptr)                                         \
  (((int *)(ptr) >= slap_daemon[t].sd_index &&                                 \
    (int *)(ptr) <= &slap_daemon[t].sd_index[dtblsize])                        \
       ? 0                                                                     \
       : 1)

#define SLAP_EPOLL_EV_PTRFD(t, ptr)                                            \
  (SLAP_EPOLL_EV_LISTENER(t, ptr)                                              \
       ? ((Listener *)ptr)->sl_sd                                              \
       : (ber_socket_t)((int *)(ptr)-slap_daemon[t].sd_index))

#define SLAP_SOCK_DEL(t, s)                                                    \
  do {                                                                         \
    int fd, rc, index = SLAP_EPOLL_SOCK_IX(t, (s));                            \
    if (index < 0)                                                             \
      break;                                                                   \
    rc = epoll_ctl(slap_daemon[t].sd_epfd, EPOLL_CTL_DEL, (s),                 \
                   &SLAP_EPOLL_SOCK_EP(t, (s)));                               \
    if (rc) {                                                                  \
      Debug(LDAP_DEBUG_ANY,                                                    \
            "daemon: epoll_ctl(epfd=%d,DEL,fd=%d) failed, errno=%d, shutting " \
            "down\n",                                                          \
            slap_daemon[t].sd_epfd, s, errno);                                 \
      if (errno != EBADF)                                                      \
        set_shutdown(SHUT_RDWR);                                               \
    }                                                                          \
    slap_daemon[t].sd_epolls[index] =                                          \
        slap_daemon[t].sd_epolls[slap_daemon[t].sd_nfds - 1];                  \
    fd = SLAP_EPOLL_EV_PTRFD(t, slap_daemon[t].sd_epolls[index].data.ptr);     \
    slap_daemon[t].sd_index[fd] = index;                                       \
    slap_daemon[t].sd_index[(s)] = -1;                                         \
    slap_daemon[t].sd_nfds--;                                                  \
  } while (0)

#define SLAP_EVENT_CLR_READ(i) SLAP_EPOLL_EVENT_CLR((i), EPOLLIN)
#define SLAP_EVENT_CLR_WRITE(i) SLAP_EPOLL_EVENT_CLR((i), EPOLLOUT)

#define SLAP_EPOLL_EVENT_CHK(i, mode) (revents[(i)].events & (mode))

#define SLAP_EVENT_IS_READ(i) SLAP_EPOLL_EVENT_CHK((i), EPOLLIN)
#define SLAP_EVENT_IS_WRITE(i) SLAP_EPOLL_EVENT_CHK((i), EPOLLOUT)
#define SLAP_EVENT_IS_ERROR(i) SLAP_EPOLL_EVENT_CHK((i), (EPOLLERR | EPOLLHUP))
#define SLAP_EVENT_IS_LISTENER(t, i)                                           \
  SLAP_EPOLL_EV_LISTENER(t, revents[(i)].data.ptr)
#define SLAP_EVENT_LISTENER(t, i) ((Listener *)(revents[(i)].data.ptr))

#define SLAP_EVENT_FD(t, i) SLAP_EPOLL_EV_PTRFD(t, revents[(i)].data.ptr)

#define SLAP_SOCK_INIT(t)                                                      \
  do {                                                                         \
    int j;                                                                     \
    slap_daemon[t].sd_epolls = ch_calloc(                                      \
        1, (sizeof(struct epoll_event) * 2 + sizeof(int)) * dtblsize * 2);     \
    slap_daemon[t].sd_index = (int *)&slap_daemon[t].sd_epolls[2 * dtblsize];  \
    slap_daemon[t].sd_epfd = epoll_create(dtblsize / slapd_daemon_threads);    \
    for (j = 0; j < dtblsize; j++)                                             \
      slap_daemon[t].sd_index[j] = -1;                                         \
  } while (0)

#define SLAP_SOCK_DESTROY(t)                                                   \
  do {                                                                         \
    if (slap_daemon[t].sd_epolls != NULL) {                                    \
      ch_free(slap_daemon[t].sd_epolls);                                       \
      slap_daemon[t].sd_epolls = NULL;                                         \
      slap_daemon[t].sd_index = NULL;                                          \
      close(slap_daemon[t].sd_epfd);                                           \
    }                                                                          \
  } while (0)

#define SLAP_EVENT_DECL struct epoll_event *revents

#define SLAP_EVENT_INIT(t)                                                     \
  do {                                                                         \
    revents = slap_daemon[t].sd_epolls + dtblsize;                             \
  } while (0)

#define SLAP_EVENT_WAIT(t, tvp, nsp)                                           \
  do {                                                                         \
    *(nsp) = epoll_wait(slap_daemon[t].sd_epfd, revents, dtblsize,             \
                        (tvp) ? ldap_to_milliseconds(*tvp) : -1);              \
  } while (0)

#elif defined(SLAP_X_DEVPOLL) && defined(HAVE_DEVPOLL)

/*************************************************************
 * Use Solaris' (>= 2.7) /dev/poll infrastructure - poll(7d) *
 *************************************************************/
#define SLAP_EVENT_FNAME "/dev/poll"
#define SLAP_EVENTS_ARE_INDEXED 0
/*
 * - sd_index	is used much like with epoll()
 * - sd_l	is maintained as an array containing the address
 *		of the listener; the index is the fd itself
 * - sd_pollfd	is used to keep track of what data has been
 *		registered in /dev/poll
 */
#define SLAP_DEVPOLL_SOCK_IX(t, s) (slap_daemon[t].sd_index[(s)])
#define SLAP_DEVPOLL_SOCK_LX(t, s) (slap_daemon[t].sd_l[(s)])
#define SLAP_DEVPOLL_SOCK_EP(t, s)                                             \
  (slap_daemon[t].sd_pollfd[SLAP_DEVPOLL_SOCK_IX(t, (s))])
#define SLAP_DEVPOLL_SOCK_FD(t, s) (SLAP_DEVPOLL_SOCK_EP(t, (s)).fd)
#define SLAP_DEVPOLL_SOCK_EV(t, s) (SLAP_DEVPOLL_SOCK_EP(t, (s)).events)
#define SLAP_SOCK_IS_ACTIVE(t, s) (SLAP_DEVPOLL_SOCK_IX(t, (s)) != -1)
#define SLAP_SOCK_NOT_ACTIVE(t, s) (SLAP_DEVPOLL_SOCK_IX(t, (s)) == -1)
#define SLAP_SOCK_IS_SET(t, s, mode) (SLAP_DEVPOLL_SOCK_EV(t, (s)) & (mode))

#define SLAP_SOCK_IS_READ(t, s) SLAP_SOCK_IS_SET(t, (s), POLLIN)
#define SLAP_SOCK_IS_WRITE(t, s) SLAP_SOCK_IS_SET(t, (s), POLLOUT)
#define SLAP_SOCK_IS_ERROR(t, s) SLAP_SOCK_IS_SET(t, (s), (POLLERR | POLLHUP))

/* as far as I understand, any time we need to communicate with the kernel
 * about the number and/or properties of a file descriptor we need it to
 * wait for, we have to rewrite the whole set */
#define SLAP_DEVPOLL_WRITE_POLLFD(t, s, pfd, n, what, shdn)                    \
  do {                                                                         \
    int rc;                                                                    \
    size_t size = (n) * sizeof(struct pollfd);                                 \
    /* FIXME: use pwrite? */                                                   \
    rc = write(slap_daemon[t].sd_dpfd, (pfd), size);                           \
    if (rc != size) {                                                          \
      Debug(LDAP_DEBUG_ANY,                                                    \
            "daemon: " SLAP_EVENT_FNAME ": "                                   \
            "%s fd=%d failed errno=%d\n",                                      \
            (what), (s), errno);                                               \
      if ((shdn)) {                                                            \
        slapd_shutdown = 2;                                                    \
      }                                                                        \
    }                                                                          \
  } while (0)

#define SLAP_DEVPOLL_SOCK_SET(t, s, mode)                                      \
  do {                                                                         \
    Debug(LDAP_DEBUG_CONNS, "SLAP_SOCK_SET_%s(%d)=%d\n",                       \
          (mode) == POLLIN ? "READ" : "WRITE", (s),                            \
          ((SLAP_DEVPOLL_SOCK_EV(t, (s)) & (mode)) != (mode)));                \
    if ((SLAP_DEVPOLL_SOCK_EV(t, (s)) & (mode)) != (mode)) {                   \
      struct pollfd pfd;                                                       \
      SLAP_DEVPOLL_SOCK_EV(t, (s)) |= (mode);                                  \
      pfd.fd = SLAP_DEVPOLL_SOCK_FD(t, (s));                                   \
      pfd.events = /* (mode) */ SLAP_DEVPOLL_SOCK_EV(t, (s));                  \
      SLAP_DEVPOLL_WRITE_POLLFD(t, (s), &pfd, 1, "SET", 0);                    \
    }                                                                          \
  } while (0)

#define SLAP_DEVPOLL_SOCK_CLR(t, s, mode)                                      \
  do {                                                                         \
    Debug(LDAP_DEBUG_CONNS, "SLAP_SOCK_CLR_%s(%d)=%d\n",                       \
          (mode) == POLLIN ? "READ" : "WRITE", (s),                            \
          ((SLAP_DEVPOLL_SOCK_EV(t, (s)) & (mode)) == (mode)));                \
    if ((SLAP_DEVPOLL_SOCK_EV(t, (s)) & (mode)) == (mode)) {                   \
      struct pollfd pfd[2];                                                    \
      SLAP_DEVPOLL_SOCK_EV(t, (s)) &= ~(mode);                                 \
      pfd[0].fd = SLAP_DEVPOLL_SOCK_FD(t, (s));                                \
      pfd[0].events = POLLREMOVE;                                              \
      pfd[1] = SLAP_DEVPOLL_SOCK_EP(t, (s));                                   \
      SLAP_DEVPOLL_WRITE_POLLFD(t, (s), &pfd[0], 2, "CLR", 0);                 \
    }                                                                          \
  } while (0)

#define SLAP_SOCK_SET_READ(t, s) SLAP_DEVPOLL_SOCK_SET(t, s, POLLIN)
#define SLAP_SOCK_SET_WRITE(t, s) SLAP_DEVPOLL_SOCK_SET(t, s, POLLOUT)

#define SLAP_SOCK_CLR_READ(t, s) SLAP_DEVPOLL_SOCK_CLR(t, (s), POLLIN)
#define SLAP_SOCK_CLR_WRITE(t, s) SLAP_DEVPOLL_SOCK_CLR(t, (s), POLLOUT)

#define SLAP_SOCK_SET_SUSPEND(t, s)                                            \
  (slap_daemon[t].sd_suspend[SLAP_DEVPOLL_SOCK_IX(t, (s))] = 1)
#define SLAP_SOCK_CLR_SUSPEND(t, s)                                            \
  (slap_daemon[t].sd_suspend[SLAP_DEVPOLL_SOCK_IX(t, (s))] = 0)
#define SLAP_SOCK_IS_SUSPEND(t, s)                                             \
  (slap_daemon[t].sd_suspend[SLAP_DEVPOLL_SOCK_IX(t, (s))] == 1)

#define SLAP_DEVPOLL_EVENT_CLR(i, mode) (revents[(i)].events &= ~(mode))

#define SLAP_EVENT_MAX(t) slap_daemon[t].sd_nfds

/* If a Listener address is provided, store that in the sd_l array.
 * If we can't do this add, the system is out of resources and we
 * need to shutdown.
 */
#define SLAP_SOCK_ADD(t, s, l)                                                 \
  do {                                                                         \
    Debug(LDAP_DEBUG_CONNS, "SLAP_SOCK_ADD(%d, %p)\n", (s), (l));              \
    SLAP_DEVPOLL_SOCK_IX(t, (s)) = slap_daemon[t].sd_nfds;                     \
    SLAP_DEVPOLL_SOCK_LX(t, (s)) = (l);                                        \
    SLAP_DEVPOLL_SOCK_FD(t, (s)) = (s);                                        \
    SLAP_DEVPOLL_SOCK_EV(t, (s)) = POLLIN;                                     \
    SLAP_DEVPOLL_WRITE_POLLFD(t, (s), &SLAP_DEVPOLL_SOCK_EP(t, (s)), 1, "ADD", \
                              1);                                              \
    slap_daemon[t].sd_nfds++;                                                  \
  } while (0)

#define SLAP_DEVPOLL_EV_LISTENER(ptr) ((ptr) != NULL)

#define SLAP_SOCK_DEL(t, s)                                                    \
  do {                                                                         \
    int fd, index = SLAP_DEVPOLL_SOCK_IX(t, (s));                              \
    Debug(LDAP_DEBUG_CONNS, "SLAP_SOCK_DEL(%d)\n", (s));                       \
    if (index < 0)                                                             \
      break;                                                                   \
    if (index < slap_daemon[t].sd_nfds - 1) {                                  \
      struct pollfd pfd = slap_daemon[t].sd_pollfd[index];                     \
      fd = slap_daemon[t].sd_pollfd[slap_daemon[t].sd_nfds - 1].fd;            \
      slap_daemon[t].sd_pollfd[index] =                                        \
          slap_daemon[t].sd_pollfd[slap_daemon[t].sd_nfds - 1];                \
      slap_daemon[t].sd_pollfd[slap_daemon[t].sd_nfds - 1] = pfd;              \
      slap_daemon[t].sd_index[fd] = index;                                     \
    }                                                                          \
    slap_daemon[t].sd_index[(s)] = -1;                                         \
    slap_daemon[t].sd_pollfd[slap_daemon[t].sd_nfds - 1].events = POLLREMOVE;  \
    SLAP_DEVPOLL_WRITE_POLLFD(                                                 \
        t, (s), &slap_daemon[t].sd_pollfd[slap_daemon[t].sd_nfds - 1], 1,      \
        "DEL", 0);                                                             \
    slap_daemon[t].sd_pollfd[slap_daemon[t].sd_nfds - 1].events = 0;           \
    slap_daemon[t].sd_nfds--;                                                  \
  } while (0)

#define SLAP_EVENT_CLR_READ(i) SLAP_DEVPOLL_EVENT_CLR((i), POLLIN)
#define SLAP_EVENT_CLR_WRITE(i) SLAP_DEVPOLL_EVENT_CLR((i), POLLOUT)

#define SLAP_DEVPOLL_EVENT_CHK(i, mode) (revents[(i)].events & (mode))

#define SLAP_EVENT_FD(t, i) (revents[(i)].fd)

#define SLAP_EVENT_IS_READ(i) SLAP_DEVPOLL_EVENT_CHK((i), POLLIN)
#define SLAP_EVENT_IS_WRITE(i) SLAP_DEVPOLL_EVENT_CHK((i), POLLOUT)
#define SLAP_EVENT_IS_ERROR(i) SLAP_DEVPOLL_EVENT_CHK((i), (POLLERR | POLLHUP))

#define SLAP_EVENT_IS_LISTENER(t, i)                                           \
  SLAP_DEVPOLL_EV_LISTENER(SLAP_DEVPOLL_SOCK_LX(t, SLAP_EVENT_FD(t, (i))))
#define SLAP_EVENT_LISTENER(t, i) SLAP_DEVPOLL_SOCK_LX(t, SLAP_EVENT_FD(t, (i)))

#define SLAP_SOCK_DESTROY(t)                                                   \
  do {                                                                         \
    if (slap_daemon[t].sd_pollfd != NULL) {                                    \
      ch_free(slap_daemon[t].sd_pollfd);                                       \
      slap_daemon[t].sd_pollfd = NULL;                                         \
      slap_daemon[t].sd_index = NULL;                                          \
      slap_daemon[t].sd_l = NULL;                                              \
      close(slap_daemon[t].sd_dpfd);                                           \
    }                                                                          \
  } while (0)

#define SLAP_SOCK_INIT(t)                                                      \
  do {                                                                         \
    slap_daemon[t].sd_pollfd = ch_calloc(                                      \
        1, (sizeof(struct pollfd) * 2 + sizeof(int) + sizeof(Listener *)) *    \
               dtblsize);                                                      \
    slap_daemon[t].sd_index = (int *)&slap_daemon[t].sd_pollfd[2 * dtblsize];  \
    slap_daemon[t].sd_l = (Listener **)&slap_daemon[t].sd_index[dtblsize];     \
    slap_daemon[t].sd_dpfd = open(SLAP_EVENT_FNAME, O_RDWR);                   \
    if (slap_daemon[t].sd_dpfd == -1) {                                        \
      Debug(LDAP_DEBUG_ANY,                                                    \
            "daemon: " SLAP_EVENT_FNAME ": "                                   \
            "open(\"" SLAP_EVENT_FNAME "\") failed errno=%d\n",                \
            errno);                                                            \
      SLAP_SOCK_DESTROY(t);                                                    \
      return -1;                                                               \
    }                                                                          \
    for (i = 0; i < dtblsize; i++) {                                           \
      slap_daemon[t].sd_pollfd[i].fd = -1;                                     \
      slap_daemon[t].sd_index[i] = -1;                                         \
    }                                                                          \
  } while (0)

#define SLAP_EVENT_DECL struct pollfd *revents

#define SLAP_EVENT_INIT(t)                                                     \
  do {                                                                         \
    revents = &slap_daemon[t].sd_pollfd[dtblsize];                             \
  } while (0)

#define SLAP_EVENT_WAIT(t, tvp, nsp)                                           \
  do {                                                                         \
    struct dvpoll sd_dvpoll;                                                   \
    sd_dvpoll.dp_timeout = (tvp) ? (tvp)->tv_sec * 1000 : -1;                  \
    sd_dvpoll.dp_nfds = dtblsize;                                              \
    sd_dvpoll.dp_fds = revents;                                                \
    *(nsp) = ioctl(slap_daemon[t].sd_dpfd, DP_POLL, &sd_dvpoll);               \
  } while (0)

#else /* ! epoll && ! /dev/poll */

/**************************************
 * Use select system call - select(2) *
 **************************************/
#define SLAP_EVENT_FNAME "select"
/* select */
#define SLAP_EVENTS_ARE_INDEXED 1
#define SLAP_EVENT_DECL fd_set readfds, writefds

#define SLAP_EVENT_INIT(t)                                                     \
  do {                                                                         \
    memcpy(&readfds, &slap_daemon[t].sd_readers, sizeof(fd_set));              \
    if (nwriters) {                                                            \
      memcpy(&writefds, &slap_daemon[t].sd_writers, sizeof(fd_set));           \
    } else {                                                                   \
      FD_ZERO(&writefds);                                                      \
    }                                                                          \
  } while (0)

#ifdef FD_SETSIZE
#define SLAP_SELECT_CHK_SETSIZE                                                \
  do {                                                                         \
    if (dtblsize > FD_SETSIZE)                                                 \
      dtblsize = FD_SETSIZE;                                                   \
  } while (0)
#else /* ! FD_SETSIZE */
#define SLAP_SELECT_CHK_SETSIZE                                                \
  do {                                                                         \
    ;                                                                          \
  } while (0)
#endif /* ! FD_SETSIZE */

#define SLAP_SOCK_INIT(t)                                                      \
  do {                                                                         \
    SLAP_SELECT_CHK_SETSIZE;                                                   \
    FD_ZERO(&slap_daemon[t].sd_actives);                                       \
    FD_ZERO(&slap_daemon[t].sd_readers);                                       \
    FD_ZERO(&slap_daemon[t].sd_writers);                                       \
  } while (0)

#define SLAP_SOCK_DESTROY(t)

#define SLAP_SOCK_IS_ACTIVE(t, fd) FD_ISSET((fd), &slap_daemon[t].sd_actives)
#define SLAP_SOCK_IS_READ(t, fd) FD_ISSET((fd), &slap_daemon[t].sd_readers)
#define SLAP_SOCK_IS_WRITE(t, fd) FD_ISSET((fd), &slap_daemon[t].sd_writers)

#define SLAP_SOCK_NOT_ACTIVE(t, fd)                                            \
  (!SLAP_SOCK_IS_ACTIVE(t, fd) && !SLAP_SOCK_IS_READ(t, fd) &&                 \
   !SLAP_SOCK_IS_WRITE(t, fd))

#define SLAP_SOCK_SET_READ(t, fd) FD_SET((fd), &slap_daemon[t].sd_readers)
#define SLAP_SOCK_SET_WRITE(t, fd) FD_SET((fd), &slap_daemon[t].sd_writers)

#define SLAP_EVENT_MAX(t) slap_daemon[t].sd_nfds
#define SLAP_SELECT_ADDTEST(t, s)                                              \
  do {                                                                         \
    if ((s) >= slap_daemon[t].sd_nfds)                                         \
      slap_daemon[t].sd_nfds = (s) + 1;                                        \
  } while (0)

#define SLAP_SOCK_CLR_READ(t, fd) FD_CLR((fd), &slap_daemon[t].sd_readers)
#define SLAP_SOCK_CLR_WRITE(t, fd) FD_CLR((fd), &slap_daemon[t].sd_writers)

#define SLAP_SOCK_ADD(t, s, l)                                                 \
  do {                                                                         \
    SLAP_SELECT_ADDTEST(t, (s));                                               \
    FD_SET((s), &slap_daemon[t].sd_actives);                                   \
    FD_SET((s), &slap_daemon[t].sd_readers);                                   \
  } while (0)

#define SLAP_SOCK_DEL(t, s)                                                    \
  do {                                                                         \
    FD_CLR((s), &slap_daemon[t].sd_actives);                                   \
    FD_CLR((s), &slap_daemon[t].sd_readers);                                   \
    FD_CLR((s), &slap_daemon[t].sd_writers);                                   \
  } while (0)

#define SLAP_EVENT_IS_READ(fd) FD_ISSET((fd), &readfds)
#define SLAP_EVENT_IS_WRITE(fd) FD_ISSET((fd), &writefds)

#define SLAP_EVENT_CLR_READ(fd) FD_CLR((fd), &readfds)
#define SLAP_EVENT_CLR_WRITE(fd) FD_CLR((fd), &writefds)

#define SLAP_EVENT_WAIT(t, tvp, nsp)                                           \
  do {                                                                         \
    *(nsp) = select(SLAP_EVENT_MAX(t), &readfds,                               \
                    nwriters > 0 ? &writefds : NULL, NULL, (tvp));             \
  } while (0)
#endif /* ! epoll && ! /dev/poll */

#ifdef HAVE_SLP
/*
 * SLP related functions
 */
#include <slp.h>

#define LDAP_SRVTYPE_PREFIX "service:ldap://"
#define LDAPS_SRVTYPE_PREFIX "service:ldaps://"
static char **slapd_srvurls = NULL;
static SLPHandle slapd_hslp = 0;
int slapd_register_slp = 0;
const char *slapd_slp_attrs = NULL;

static SLPError slapd_slp_cookie;

static void slapd_slp_init(const char *urls) {
  int i;
  SLPError err;

  slapd_srvurls = ldap_str2charray(urls, " ");

  if (slapd_srvurls == NULL)
    return;

  /* find and expand INADDR_ANY URLs */
  for (i = 0; slapd_srvurls[i] != NULL; i++) {
    if (strcmp(slapd_srvurls[i], "ldap:///") == 0) {
      slapd_srvurls[i] =
          (char *)ch_realloc(slapd_srvurls[i], global_host_bv.bv_len +
                                                   sizeof(LDAP_SRVTYPE_PREFIX));
      strcpy(lutil_strcopy(slapd_srvurls[i], LDAP_SRVTYPE_PREFIX),
             global_host_bv.bv_val);
    } else if (strcmp(slapd_srvurls[i], "ldaps:///") == 0) {
      slapd_srvurls[i] = (char *)ch_realloc(slapd_srvurls[i],
                                            global_host_bv.bv_len +
                                                sizeof(LDAPS_SRVTYPE_PREFIX));
      strcpy(lutil_strcopy(slapd_srvurls[i], LDAPS_SRVTYPE_PREFIX),
             global_host_bv.bv_val);
    }
  }

  /* open the SLP handle */
  err = SLPOpen("en", 0, &slapd_hslp);

  if (err != SLP_OK) {
    Debug(LDAP_DEBUG_CONNS, "daemon: SLPOpen() failed with %ld\n", (long)err);
  }
}

static void slapd_slp_deinit(void) {
  if (slapd_srvurls == NULL)
    return;

  ldap_charray_free(slapd_srvurls);
  slapd_srvurls = NULL;

  /* close the SLP handle */
  SLPClose(slapd_hslp);
}

static void slapd_slp_regreport(SLPHandle hslp, SLPError errcode,
                                void *cookie) {
  /* return the error code in the cookie */
  *(SLPError *)cookie = errcode;
}

static void slapd_slp_reg() {
  int i;
  SLPError err;

  if (slapd_srvurls == NULL)
    return;

  for (i = 0; slapd_srvurls[i] != NULL; i++) {
    if (strncmp(slapd_srvurls[i], LDAP_SRVTYPE_PREFIX,
                sizeof(LDAP_SRVTYPE_PREFIX) - 1) == 0 ||
        strncmp(slapd_srvurls[i], LDAPS_SRVTYPE_PREFIX,
                sizeof(LDAPS_SRVTYPE_PREFIX) - 1) == 0) {
      err = SLPReg(slapd_hslp, slapd_srvurls[i], SLP_LIFETIME_MAXIMUM, "ldap",
                   (slapd_slp_attrs) ? slapd_slp_attrs : "", SLP_TRUE,
                   slapd_slp_regreport, &slapd_slp_cookie);

      if (err != SLP_OK || slapd_slp_cookie != SLP_OK) {
        Debug(LDAP_DEBUG_CONNS,
              "daemon: SLPReg(%s) failed with %ld, cookie = %ld\n",
              slapd_srvurls[i], (long)err, (long)slapd_slp_cookie);
      }
    }
  }
}

static void slapd_slp_dereg(void) {
  int i;
  SLPError err;

  if (slapd_srvurls == NULL)
    return;

  for (i = 0; slapd_srvurls[i] != NULL; i++) {
    err = SLPDereg(slapd_hslp, slapd_srvurls[i], slapd_slp_regreport,
                   &slapd_slp_cookie);

    if (err != SLP_OK || slapd_slp_cookie != SLP_OK) {
      Debug(LDAP_DEBUG_CONNS,
            "daemon: SLPDereg(%s) failed with %ld, cookie = %ld\n",
            slapd_srvurls[i], (long)err, (long)slapd_slp_cookie);
    }
  }
}
#endif /* HAVE_SLP */

/*
 * Add a descriptor to daemon control
 *
 * If isactive, the descriptor is a live server session and is subject
 * to idletimeout control. Otherwise, the descriptor is a passive
 * listener or an outbound client session, and not subject to
 * idletimeout. The underlying event handler may record the Listener
 * argument to differentiate Listener's from real sessions.
 */
static void slapd_add(ber_socket_t s, int isactive, Listener *sl, int id) {
  if (id < 0)
    id = DAEMON_ID(s);
  ldap_pvt_thread_mutex_lock(&slap_daemon[id].sd_mutex);

  assert(SLAP_SOCK_NOT_ACTIVE(id, s));

  if (isactive)
    slap_daemon[id].sd_nactives++;

#ifdef __SANITIZE_THREAD__
  ldap_pvt_thread_mutex_lock(&tsan_mutex);
#endif
  SLAP_SOCK_ADD(id, s, sl);
#ifdef __SANITIZE_THREAD__
  ldap_pvt_thread_mutex_unlock(&tsan_mutex);
#endif

  Debug(LDAP_DEBUG_CONNS, "daemon: added %ldr%s listener=%p\n", (long)s,
        isactive ? " (active)" : "", (void *)sl);

  ldap_pvt_thread_mutex_unlock(&slap_daemon[id].sd_mutex);

  WAKE_LISTENER(id, 1);
}

/*
 * Remove the descriptor from daemon control
 */
void slapd_remove(ber_socket_t s, Sockbuf *sb, int wasactive, int wake,
                  int locked) {
  int waswriter;
  int wasreader;
  int id = DAEMON_ID(s);

  if (!locked)
    ldap_pvt_thread_mutex_lock(&slap_daemon[id].sd_mutex);

#ifdef __SANITIZE_THREAD__
  ldap_pvt_thread_mutex_lock(&tsan_mutex);
#endif
  assert(SLAP_SOCK_IS_ACTIVE(id, s));

  if (wasactive)
    slap_daemon[id].sd_nactives--;

  waswriter = SLAP_SOCK_IS_WRITE(id, s);
  wasreader = SLAP_SOCK_IS_READ(id, s);

  Debug(LDAP_DEBUG_CONNS, "daemon: removing %ld%s%s\n", (long)s,
        wasreader ? "r" : "", waswriter ? "w" : "");

  if (waswriter)
    slap_daemon[id].sd_nwriters--;

  SLAP_SOCK_DEL(id, s);

  if (sb)
    ber_sockbuf_free(sb);

  /* If we ran out of file descriptors, we dropped a listener from
   * the select() loop. Now that we're removing a session from our
   * control, we can try to resume a dropped listener to use.
   */
  if (emfile && listening) {
    int i;
    for (i = 0; slap_listeners[i] != NULL; i++) {
      Listener *lr = slap_listeners[i];

      if (lr->sl_sd == AC_SOCKET_INVALID)
        continue;
      if (lr->sl_sd == s)
        continue;
      if (lr->sl_mute) {
        lr->sl_mute = 0;
        emfile--;
        if (DAEMON_ID(lr->sl_sd) != id)
          WAKE_LISTENER(DAEMON_ID(lr->sl_sd), wake);
        break;
      }
    }
    /* Walked the entire list without enabling anything; emfile
     * counter is stale. Reset it.
     */
    if (slap_listeners[i] == NULL)
      emfile = 0;
  }
#ifdef __SANITIZE_THREAD__
  ldap_pvt_thread_mutex_unlock(&tsan_mutex);
#endif
  ldap_pvt_thread_mutex_unlock(&slap_daemon[id].sd_mutex);
  WAKE_LISTENER(id, wake || slapd_gentle_shutdown);
}

void slapd_clr_write(ber_socket_t s, int wake) {
  int id = DAEMON_ID(s);
  ldap_pvt_thread_mutex_lock(&slap_daemon[id].sd_mutex);

  if (SLAP_SOCK_IS_WRITE(id, s)) {
    assert(SLAP_SOCK_IS_ACTIVE(id, s));

    SLAP_SOCK_CLR_WRITE(id, s);
    slap_daemon[id].sd_nwriters--;
  }

  ldap_pvt_thread_mutex_unlock(&slap_daemon[id].sd_mutex);
  WAKE_LISTENER(id, wake);
}

void slapd_set_write(ber_socket_t s, int wake) {
  int id = DAEMON_ID(s);
  ldap_pvt_thread_mutex_lock(&slap_daemon[id].sd_mutex);

  assert(SLAP_SOCK_IS_ACTIVE(id, s));

  if (!SLAP_SOCK_IS_WRITE(id, s)) {
    SLAP_SOCK_SET_WRITE(id, s);
    slap_daemon[id].sd_nwriters++;
  }

#ifdef SLAPD_WRITE_TIMEOUT
  if ((wake & 2) && global_writetimeout > 0 && !chk_writetime.ns) {
    if (id)
      ldap_pvt_thread_mutex_lock(&slap_daemon[0].sd_mutex);
    if (!chk_writetime.ns)
      chk_writetime = ldap_now_steady();
    if (id)
      ldap_pvt_thread_mutex_unlock(&slap_daemon[0].sd_mutex);
  }
#endif /* SLAPD_WRITE_TIMEOUT */

  ldap_pvt_thread_mutex_unlock(&slap_daemon[id].sd_mutex);
  WAKE_LISTENER(id, wake);
}

int slapd_clr_read(ber_socket_t s, int wake) {
  int rc = 1;
  int id = DAEMON_ID(s);
  ldap_pvt_thread_mutex_lock(&slap_daemon[id].sd_mutex);

  if (SLAP_SOCK_IS_ACTIVE(id, s)) {
    SLAP_SOCK_CLR_READ(id, s);
    rc = 0;
  }
  ldap_pvt_thread_mutex_unlock(&slap_daemon[id].sd_mutex);
  if (!rc)
    WAKE_LISTENER(id, wake);
  return rc;
}

void slapd_set_read(ber_socket_t s, int wake) {
  int do_wake = 1;
  int id = DAEMON_ID(s);
  ldap_pvt_thread_mutex_lock(&slap_daemon[id].sd_mutex);

  if (SLAP_SOCK_IS_ACTIVE(id, s) && !SLAP_SOCK_IS_READ(id, s)) {
    SLAP_SOCK_SET_READ(id, s);
  } else {
    do_wake = 0;
  }
  ldap_pvt_thread_mutex_unlock(&slap_daemon[id].sd_mutex);
  if (do_wake)
    WAKE_LISTENER(id, wake);
}

#ifdef SLAPD_WRITE_TIMEOUT
slap_time_t slapd_get_writetime() {
  slap_time_t cur;
  ldap_pvt_thread_mutex_lock(&slap_daemon[0].sd_mutex);
  cur = chk_writetime;
  ldap_pvt_thread_mutex_unlock(&slap_daemon[0].sd_mutex);
  return cur;
}

void slapd_clr_writetime(slap_time_t old) {
  ldap_pvt_thread_mutex_lock(&slap_daemon[0].sd_mutex);
  if (chk_writetime.ns == old.ns)
    chk_writetime.ns = 0;
  ldap_pvt_thread_mutex_unlock(&slap_daemon[0].sd_mutex);
}
#endif /* SLAPD_WRITE_TIMEOUT */

static void slapd_close(ber_socket_t s) {
  Debug(LDAP_DEBUG_CONNS, "daemon: closing %ld\n", (long)s);
  tcp_close(s);
}

void slapd_shutsock(ber_socket_t s) {
  Debug(LDAP_DEBUG_CONNS, "daemon: shutdown socket %ld\n", (long)s);
  shutdown(s, SHUT_RDWR);
}

static void slap_free_listener_addresses(struct sockaddr **sal) {
  struct sockaddr **sap;
  if (sal == NULL)
    return;
  for (sap = sal; *sap != NULL; sap++)
    ch_free(*sap);
  ch_free(sal);
}

#if defined(LDAP_PF_LOCAL) || defined(SLAP_X_LISTENER_MOD)
static int get_url_perms(char **exts, mode_t *perms, int *crit) {
  int i;

  assert(exts != NULL);
  assert(perms != NULL);
  assert(crit != NULL);

  *crit = 0;
  for (i = 0; exts[i]; i++) {
    char *type = exts[i];
    int c = 0;

    if (type[0] == '!') {
      c = 1;
      type++;
    }

    if (strncasecmp(type, LDAPI_MOD_URLEXT "=",
                    sizeof(LDAPI_MOD_URLEXT "=") - 1) == 0) {
      char *value = type + (sizeof(LDAPI_MOD_URLEXT "=") - 1);
      mode_t p = 0;
      int j;

      switch (strlen(value)) {
      case 4:
        /* skip leading '0' */
        if (value[0] != '0')
          return LDAP_OTHER;
        value++;

      case 3:
        for (j = 0; j < 3; j++) {
          int v;

          v = value[j] - '0';

          if (v < 0 || v > 7)
            return LDAP_OTHER;

          p |= v << 3 * (2 - j);
        }
        break;

      case 10:
        for (j = 1; j < 10; j++) {
          static mode_t m[] = {0,       S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP,
                               S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};
          static const char c[] = "-rwxrwxrwx";

          if (value[j] == c[j]) {
            p |= m[j];

          } else if (value[j] != '-') {
            return LDAP_OTHER;
          }
        }
        break;

      default:
        return LDAP_OTHER;
      }

      *crit = c;
      *perms = p;

      return LDAP_SUCCESS;
    }
  }

  return LDAP_OTHER;
}
#endif /* LDAP_PF_LOCAL || SLAP_X_LISTENER_MOD */

/* port = 0 indicates AF_LOCAL */
static int slap_get_listener_addresses(const char *host, unsigned short port,
                                       struct sockaddr ***sal) {
  struct sockaddr **sap;

#ifdef LDAP_PF_LOCAL
  if (port == 0) {
    *sal = ch_malloc(2 * sizeof(void *));
    if (*sal == NULL)
      return -1;

    sap = *sal;
    *sap = ch_malloc(sizeof(struct sockaddr_un));
    if (*sap == NULL)
      goto errexit;
    sap[1] = NULL;

    if (strlen(host) > (sizeof(((struct sockaddr_un *)*sap)->sun_path) - 1)) {
      Debug(LDAP_DEBUG_ANY, "daemon: domain socket path (%s) too long in URL",
            host);
      goto errexit;
    }

    (void)memset((void *)*sap, '\0', sizeof(struct sockaddr_un));
    (*sap)->sa_family = AF_LOCAL;
    strcpy(((struct sockaddr_un *)*sap)->sun_path, host);
  } else
#endif /* LDAP_PF_LOCAL */
  {
#ifdef HAVE_GETADDRINFO
    struct addrinfo hints, *res, *sai;
    int n, err;
    char serv[7];

    memset(&hints, '\0', sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = slap_inet4or6;
    snprintf(serv, sizeof serv, "%d", port);

    if ((err = getaddrinfo(host, serv, &hints, &res))) {
      Debug(LDAP_DEBUG_ANY, "daemon: getaddrinfo() failed: %s\n",
            AC_GAI_STRERROR(err));
      return -1;
    }

    sai = res;
    for (n = 2; (sai = sai->ai_next) != NULL; n++) {
      /* EMPTY */;
    }
    *sal = ch_calloc(n, sizeof(void *));
    if (*sal == NULL)
      return -1;

    sap = *sal;
    *sap = NULL;

    for (sai = res; sai; sai = sai->ai_next) {
      if (sai->ai_addr == NULL) {
        Debug(LDAP_DEBUG_ANY, "slap_get_listener_addresses: "
                              "getaddrinfo ai_addr is NULL?\n");
        freeaddrinfo(res);
        goto errexit;
      }

      switch (sai->ai_family) {
#ifdef LDAP_PF_INET6
      case AF_INET6:
        *sap = ch_malloc(sizeof(struct sockaddr_in6));
        if (*sap == NULL) {
          freeaddrinfo(res);
          goto errexit;
        }
        *(struct sockaddr_in6 *)*sap = *((struct sockaddr_in6 *)sai->ai_addr);
        break;
#endif /* LDAP_PF_INET6 */
      case AF_INET:
        *sap = ch_malloc(sizeof(struct sockaddr_in));
        if (*sap == NULL) {
          freeaddrinfo(res);
          goto errexit;
        }
        *(struct sockaddr_in *)*sap = *((struct sockaddr_in *)sai->ai_addr);
        break;
      default:
        *sap = NULL;
        break;
      }

      if (*sap != NULL) {
        (*sap)->sa_family = sai->ai_family;
        sap++;
        *sap = NULL;
      }
    }

    freeaddrinfo(res);

#else  /* ! HAVE_GETADDRINFO */
    int i, n = 1;
    struct in_addr in;
    struct hostent *he = NULL;

    if (host == NULL) {
      in.s_addr = htonl(INADDR_ANY);

    } else if (!inet_aton(host, &in)) {
      he = gethostbyname(host);
      if (he == NULL) {
        Debug(LDAP_DEBUG_ANY, "daemon: invalid host %s", host);
        return -1;
      }
      for (n = 0; he->h_addr_list[n]; n++) /* empty */
        ;
    }

    *sal = ch_malloc((n + 1) * sizeof(void *));
    if (*sal == NULL)
      return -1;

    sap = *sal;
    for (i = 0; i < n; i++) {
      sap[i] = ch_malloc(sizeof(struct sockaddr_in));
      if (*sap == NULL)
        goto errexit;

      (void)memset((void *)sap[i], '\0', sizeof(struct sockaddr_in));
      sap[i]->sa_family = AF_INET;
      ((struct sockaddr_in *)sap[i])->sin_port = htons(port);
      memcpy(&((struct sockaddr_in *)sap[i])->sin_addr,
             he ? (struct in_addr *)he->h_addr_list[i] : &in,
             sizeof(struct in_addr));
    }
    sap[i] = NULL;
#endif /* ! HAVE_GETADDRINFO */
  }

  return 0;

errexit:
  slap_free_listener_addresses(*sal);
  return -1;
}

static int slap_open_listener(const char *url, int *listeners, int *cur) {
  int tmp, rc;
  Listener l;
  Listener *li = NULL;
  LDAPURLDesc *lud;
  unsigned short port;
  int err, addrlen = 0;
  struct sockaddr **sal = NULL, **psal;
  int socktype = SOCK_STREAM; /* default to COTS */
  ber_socket_t s;

#if defined(LDAP_PF_LOCAL) || defined(SLAP_X_LISTENER_MOD)
  /*
   * use safe defaults
   */
  int crit = 1;
#endif /* LDAP_PF_LOCAL || SLAP_X_LISTENER_MOD */

  Debug(LDAP_DEBUG_TRACE, "daemon: open listener %s\n", url);

  rc = ldap_url_parse(url, &lud);

  if (rc != LDAP_URL_SUCCESS) {
    Debug(LDAP_DEBUG_ANY, "daemon: listen URL \"%s\" parse error=%d\n", url,
          rc);
    return rc;
  }

  l.sl_url.bv_val = NULL;
  l.sl_mute = 1;
  l.sl_busy = 0;

#ifndef WITH_TLS
  if (ldap_pvt_url_scheme2tls(lud->lud_scheme)) {
    Debug(LDAP_DEBUG_ANY, "daemon: TLS not supported (%s)\n", url);
    ldap_free_urldesc(lud);
    return -1;
  }

  if (!lud->lud_port)
    lud->lud_port = LDAP_PORT;

#else  /* WITH_TLS */
  l.sl_is_tls = ldap_pvt_url_scheme2tls(lud->lud_scheme);

  if (!lud->lud_port) {
    lud->lud_port = l.sl_is_tls ? LDAPS_PORT : LDAP_PORT;
  }
#endif /* WITH_TLS */

#ifdef LDAP_TCP_BUFFER
  l.sl_tcp_rmem = 0;
  l.sl_tcp_wmem = 0;
#endif /* LDAP_TCP_BUFFER */

  port = (unsigned short)lud->lud_port;

  tmp = ldap_pvt_url_scheme2proto(lud->lud_scheme);
  if (tmp == LDAP_PROTO_IPC) {
#ifdef LDAP_PF_LOCAL
    if (lud->lud_host == NULL || lud->lud_host[0] == '\0') {
      err = slap_get_listener_addresses(LDAPI_SOCK, 0, &sal);
    } else {
      err = slap_get_listener_addresses(lud->lud_host, 0, &sal);
    }
#else  /* ! LDAP_PF_LOCAL */

    Debug(LDAP_DEBUG_ANY, "daemon: URL scheme not supported: %s", url);
    ldap_free_urldesc(lud);
    return -1;
#endif /* ! LDAP_PF_LOCAL */
  } else {
    if (lud->lud_host == NULL || lud->lud_host[0] == '\0' ||
        strcmp(lud->lud_host, "*") == 0) {
      err = slap_get_listener_addresses(NULL, port, &sal);
    } else {
      err = slap_get_listener_addresses(lud->lud_host, port, &sal);
    }
  }

#ifdef LDAP_CONNECTIONLESS
  l.sl_is_udp = (tmp == LDAP_PROTO_UDP);
#endif /* LDAP_CONNECTIONLESS */

#if defined(LDAP_PF_LOCAL) || defined(SLAP_X_LISTENER_MOD)
  if (lud->lud_exts) {
    err = get_url_perms(lud->lud_exts, &l.sl_perms, &crit);
  } else {
    l.sl_perms = S_IRWXU | S_IRWXO;
  }
#endif /* LDAP_PF_LOCAL || SLAP_X_LISTENER_MOD */

  ldap_free_urldesc(lud);
  if (err) {
    slap_free_listener_addresses(sal);
    return -1;
  }

  psal = sal;
  while (*sal != NULL) {
    char *af;
    switch ((*sal)->sa_family) {
    case AF_INET:
      af = "IPv4";
      break;
#ifdef LDAP_PF_INET6
    case AF_INET6:
      af = "IPv6";
      break;
#endif /* LDAP_PF_INET6 */
#ifdef LDAP_PF_LOCAL
    case AF_LOCAL:
      af = "Local";
      break;
#endif /* LDAP_PF_LOCAL */
    default:
      sal++;
      continue;
    }

#ifdef LDAP_CONNECTIONLESS
    if (l.sl_is_udp)
      socktype = SOCK_DGRAM;
#endif /* LDAP_CONNECTIONLESS */

    s = socket((*sal)->sa_family, socktype, 0);
    if (s == AC_SOCKET_INVALID) {
      int err = sock_errno();
      Debug(LDAP_DEBUG_ANY, "daemon: %s socket() failed errno=%d (%s)\n", af,
            err, sock_errstr(err));
      sal++;
      continue;
    }
    l.sl_sd = s;

    if (l.sl_sd >= dtblsize) {
      Debug(LDAP_DEBUG_ANY,
            "daemon: listener descriptor %ld is too great %ld\n", (long)l.sl_sd,
            (long)dtblsize);
      tcp_close(s);
      sal++;
      continue;
    }

#ifdef LDAP_PF_LOCAL
    if ((*sal)->sa_family == AF_LOCAL) {
      unlink(((struct sockaddr_un *)*sal)->sun_path);
    } else
#endif /* LDAP_PF_LOCAL */
    {
#ifdef SO_REUSEADDR
      /* enable address reuse */
      tmp = 1;
      rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&tmp, sizeof(tmp));
      if (rc == AC_SOCKET_ERROR) {
        int err = sock_errno();
        Debug(LDAP_DEBUG_ANY,
              "slapd(%ld): "
              "setsockopt(SO_REUSEADDR) failed errno=%d (%s)\n",
              (long)l.sl_sd, err, sock_errstr(err));
      }
#endif /* SO_REUSEADDR */
    }

    switch ((*sal)->sa_family) {
    case AF_INET:
      addrlen = sizeof(struct sockaddr_in);
      break;
#ifdef LDAP_PF_INET6
    case AF_INET6:
#ifdef IPV6_V6ONLY
      /* Try to use IPv6 sockets for IPv6 only */
      tmp = 1;
      rc = setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&tmp, sizeof(tmp));
      if (rc == AC_SOCKET_ERROR) {
        int err = sock_errno();
        Debug(LDAP_DEBUG_ANY,
              "slapd(%ld): "
              "setsockopt(IPV6_V6ONLY) failed errno=%d (%s)\n",
              (long)l.sl_sd, err, sock_errstr(err));
      }
#endif /* IPV6_V6ONLY */
      addrlen = sizeof(struct sockaddr_in6);
      break;
#endif /* LDAP_PF_INET6 */

#ifdef LDAP_PF_LOCAL
    case AF_LOCAL:
#ifdef LOCAL_CREDS
    {
      int one = 1;
      setsockopt(s, 0, LOCAL_CREDS, &one, sizeof(one));
    }
#endif /* LOCAL_CREDS */

      addrlen = sizeof(struct sockaddr_un);
      break;
#endif /* LDAP_PF_LOCAL */
    }

#ifdef LDAP_PF_LOCAL
    /* create socket with all permissions set for those systems
     * that honor permissions on sockets (e.g. Linux); typically,
     * only write is required.  To exploit filesystem permissions,
     * place the socket in a directory and use directory's
     * permissions.  Need write perms to the directory to
     * create/unlink the socket; likely need exec perms to access
     * the socket (ITS#4709) */
    {
      mode_t old_umask = 0;

      if ((*sal)->sa_family == AF_LOCAL) {
        old_umask = umask(0);
      }
#endif /* LDAP_PF_LOCAL */
      rc = bind(s, *sal, addrlen);
#ifdef LDAP_PF_LOCAL
      if (old_umask != 0) {
        umask(old_umask);
      }
    }
#endif /* LDAP_PF_LOCAL */
    if (rc) {
      err = sock_errno();
      Debug(LDAP_DEBUG_ANY, "daemon: bind(%ld) failed errno=%d (%s)\n",
            (long)l.sl_sd, err, sock_errstr(err));
      tcp_close(s);
      sal++;
      continue;
    }

    switch ((*sal)->sa_family) {
#ifdef LDAP_PF_LOCAL
    case AF_LOCAL: {
      char *path = ((struct sockaddr_un *)*sal)->sun_path;
      l.sl_name.bv_len = strlen(path) + STRLENOF("PATH=");
      l.sl_name.bv_val = ber_memalloc(l.sl_name.bv_len + 1);
      snprintf(l.sl_name.bv_val, l.sl_name.bv_len + 1, "PATH=%s", path);
    } break;
#endif /* LDAP_PF_LOCAL */

    case AF_INET: {
      char addr[INET_ADDRSTRLEN];
      const char *s;
#if defined(HAVE_GETADDRINFO) && defined(HAVE_INET_NTOP)
      s = inet_ntop(AF_INET, &((struct sockaddr_in *)*sal)->sin_addr, addr,
                    sizeof(addr));
#else  /* ! HAVE_GETADDRINFO || ! HAVE_INET_NTOP */
      s = inet_ntoa(((struct sockaddr_in *)*sal)->sin_addr);
#endif /* ! HAVE_GETADDRINFO || ! HAVE_INET_NTOP */
      if (!s)
        s = SLAP_STRING_UNKNOWN;
      port = ntohs(((struct sockaddr_in *)*sal)->sin_port);
      l.sl_name.bv_val = ber_memalloc(sizeof("IP=255.255.255.255:65535"));
      snprintf(l.sl_name.bv_val, sizeof("IP=255.255.255.255:65535"), "IP=%s:%d",
               s, port);
      l.sl_name.bv_len = strlen(l.sl_name.bv_val);
    } break;

#ifdef LDAP_PF_INET6
    case AF_INET6: {
      char addr[INET6_ADDRSTRLEN];
      const char *s;
      s = inet_ntop(AF_INET6, &((struct sockaddr_in6 *)*sal)->sin6_addr, addr,
                    sizeof addr);
      if (!s)
        s = SLAP_STRING_UNKNOWN;
      port = ntohs(((struct sockaddr_in6 *)*sal)->sin6_port);
      l.sl_name.bv_len = strlen(s) + sizeof("IP=[]:65535");
      l.sl_name.bv_val = ber_memalloc(l.sl_name.bv_len);
      snprintf(l.sl_name.bv_val, l.sl_name.bv_len, "IP=[%s]:%d", s, port);
      l.sl_name.bv_len = strlen(l.sl_name.bv_val);
    } break;
#endif /* LDAP_PF_INET6 */

    default:
      Debug(LDAP_DEBUG_ANY, "daemon: unsupported address family (%d)\n",
            (int)(*sal)->sa_family);
      break;
    }

    /* If we got more, we need to make space the slap_listeners array. */
    if (*cur == *listeners) {
      *listeners += 1;
      slap_listeners =
          ch_realloc(slap_listeners, (*listeners + 1) * sizeof(Listener *));
      if (!slap_listeners)
        return ENOMEM;
    }

    memcpy(&l.sl_sa, *sal, addrlen);
    ber_str2bv(url, 0, 1, &l.sl_url);
    li = ch_malloc(sizeof(Listener));
    *li = l;
    slap_listeners[*cur] = li;
    (*cur)++;
    sal++;
  }

  slap_free_listener_addresses(psal);

  if (li == NULL) {
    Debug(LDAP_DEBUG_TRACE, "slap_open_listener: failed on %s\n", url);
  } else {
    Debug(LDAP_DEBUG_TRACE, "daemon: listener initialized %s\n",
          l.sl_url.bv_val);
  }
  return 0;
}

static int daemon_inited = 0;

int slapd_daemon_init(const char *urls) {
  int i, j, n, rc;
  char **u;

  Debug(LDAP_DEBUG_ARGS, "daemon_init: %s\n", urls ? urls : "<null>");

  for (i = 0; i < SLAPD_MAX_DAEMON_THREADS; i++) {
    wake_sds[i][0] = AC_SOCKET_INVALID;
    wake_sds[i][1] = AC_SOCKET_INVALID;
  }

  ldap_pvt_thread_mutex_init(&slap_daemon[0].sd_mutex);
#ifdef HAVE_TCPD
  ldap_pvt_thread_mutex_init(&sd_tcpd_mutex);
#endif /* TCP Wrappers */

  daemon_inited = 1;

#ifdef HAVE_SYSCONF
  dtblsize = sysconf(_SC_OPEN_MAX);
#elif defined(HAVE_GETDTABLESIZE)
  dtblsize = getdtablesize();
#else  /* ! HAVE_SYSCONF && ! HAVE_GETDTABLESIZE */
  dtblsize = FD_SETSIZE;
#endif /* ! HAVE_SYSCONF && ! HAVE_GETDTABLESIZE */

  /* open a pipe (or something equivalent connected to itself).
   * we write a byte on this fd whenever we catch a signal. The main
   * loop will be select'ing on this socket, and will wake up when
   * this byte arrives.
   */
  if ((rc = lutil_pair(wake_sds[0])) < 0) {
    Debug(LDAP_DEBUG_ANY, "daemon: lutil_pair() failed rc=%d\n", rc);
    return rc;
  }
  ber_pvt_socket_set_nonblock(wake_sds[0][1], 1);

  SLAP_SOCK_INIT(0);

  if (urls == NULL)
    urls = "ldap:///";

  u = ldap_str2charray(urls, " ");

  if (u == NULL || u[0] == NULL) {
    Debug(LDAP_DEBUG_ANY, "daemon_init: no urls (%s) provided.a\n", urls);
    if (u)
      ldap_charray_free(u);
    return -1;
  }

  for (i = 0; u[i] != NULL; i++) {
    Debug(LDAP_DEBUG_TRACE, "daemon_init: listen on %s\n", u[i]);
  }

  if (i == 0) {
    Debug(LDAP_DEBUG_ANY, "daemon_init: no listeners to open (%s)\n", urls);
    ldap_charray_free(u);
    return -1;
  }

  Debug(LDAP_DEBUG_TRACE, "daemon_init: %d listeners to open...\n", i);
  slap_listeners = ch_calloc(i + 1, sizeof(Listener *));

  for (n = 0, j = 0; u[n]; n++) {
    if (slap_open_listener(u[n], &i, &j)) {
      ldap_charray_free(u);
      return -1;
    }
  }
  slap_listeners[j] = NULL;

  if (j == 0) {
    Debug(LDAP_DEBUG_TRACE,
          "daemon_init: no listeners opened (see previous errors)\n");
    ldap_charray_free(u);
    return -1;
  }
  Debug(LDAP_DEBUG_TRACE, "daemon_init: %d listeners opened\n", j);

#ifdef HAVE_SLP
  if (slapd_register_slp) {
    slapd_slp_init(urls);
    slapd_slp_reg();
  }
#endif /* HAVE_SLP */

  ldap_charray_free(u);

  return !i;
}

static void destroy_listeners(void) {
  Listener *lr, **ll = slap_listeners;

  if (ll == NULL)
    return;

  while ((lr = *ll++) != NULL) {
    if (lr->sl_url.bv_val) {
      ber_memfree(lr->sl_url.bv_val);
    }

    if (lr->sl_name.bv_val) {
      ber_memfree(lr->sl_name.bv_val);
    }

    free(lr);
  }

  free(slap_listeners);
  slap_listeners = NULL;
}

int slapd_daemon_destroy(void) {
  destroy_listeners();
  connections_destroy();
  if (daemon_inited) {
    int i;

    for (i = 0; i < slapd_daemon_threads; i++) {
      Debug(LDAP_DEBUG_CONNS, "daemon: closing [1] %d/fd %d, [0] %d/fd %d\n",
            wake_sds[i][1], wake_sds[i][1], wake_sds[i][0], wake_sds[i][0]);
      tcp_close(wake_sds[i][1]);
      tcp_close(wake_sds[i][0]);
      ldap_pvt_thread_mutex_destroy(&slap_daemon[i].sd_mutex);
      SLAP_SOCK_DESTROY(i);
    }
    daemon_inited = 0;
#ifdef HAVE_TCPD
    ldap_pvt_thread_mutex_destroy(&sd_tcpd_mutex);
#endif /* TCP Wrappers */
  }

#ifdef HAVE_SLP
  if (slapd_register_slp) {
    slapd_slp_dereg();
    slapd_slp_deinit();
  }
#endif /* HAVE_SLP */

  return 0;
}

static void close_listeners(int remove) {
  int l;

  if (!listening)
    return;
  listening = 0;

  for (l = 0; slap_listeners[l] != NULL; l++) {
    Listener *lr = slap_listeners[l];

    if (lr->sl_sd != AC_SOCKET_INVALID) {
      int s = lr->sl_sd;
      lr->sl_sd = AC_SOCKET_INVALID;
      if (remove)
        slapd_remove(s, NULL, 0, 0, 0);

#ifdef LDAP_PF_LOCAL
      if (lr->sl_sa.sa_addr.sa_family == AF_LOCAL) {
        unlink(lr->sl_sa.sa_un_addr.sun_path);
      }
#endif /* LDAP_PF_LOCAL */

      slapd_close(s);
    }
  }
}

static int slap_listener(Listener *sl) {
  Sockaddr from;

  ber_socket_t s;
  ber_socklen_t len = sizeof(from);
  Connection *c;
  slap_ssf_t ssf = 0;
  struct berval authid = BER_BVNULL;
#ifdef SLAPD_RLOOKUPS
  char hbuf[NI_MAXHOST];
#endif /* SLAPD_RLOOKUPS */

  char *dnsname = NULL;
  const char *peeraddr = NULL;
  /* we assume INET6_ADDRSTRLEN > INET_ADDRSTRLEN */
  char addr[INET6_ADDRSTRLEN];
#ifdef LDAP_PF_LOCAL
  char peername[MAXPATHLEN + sizeof("PATH=")];
#ifdef LDAP_PF_LOCAL_SENDMSG
  char peerbuf[8];
  struct berval peerbv = BER_BVNULL;
#endif
#elif defined(LDAP_PF_INET6)
  char peername[sizeof("IP=[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]:65535")];
#else  /* ! LDAP_PF_LOCAL && ! LDAP_PF_INET6 */
  char peername[sizeof("IP=255.255.255.255:65336")];
#endif /* LDAP_PF_LOCAL */
  int cflag;
  int tid;

  Debug(LDAP_DEBUG_TRACE, ">>> slap_listener(%s)\n", sl->sl_url.bv_val);

  peername[0] = '\0';

#ifdef LDAP_CONNECTIONLESS
  if (sl->sl_is_udp)
    return 1;
#endif /* LDAP_CONNECTIONLESS */

#ifdef LDAP_PF_LOCAL
  /* FIXME: apparently accept doesn't fill
   * the sun_path sun_path member */
  from.sa_un_addr.sun_path[0] = '\0';
#endif /* LDAP_PF_LOCAL */

  s = accept(sl->sl_sd, (struct sockaddr *)&from, &len);

  /* Resume the listener FD to allow concurrent-processing of
   * additional incoming connections.
   */
#ifdef __SANITIZE_THREAD__
  ldap_pvt_thread_mutex_lock(&tsan_mutex);
#endif
  sl->sl_busy = 0;
  WAKE_LISTENER(DAEMON_ID(sl->sl_sd), 1);
#ifdef __SANITIZE_THREAD__
  ldap_pvt_thread_mutex_unlock(&tsan_mutex);
#endif

  if (s == AC_SOCKET_INVALID) {
    int err = sock_errno();

    if (
#ifdef EMFILE
        err == EMFILE ||
#endif /* EMFILE */
#ifdef ENFILE
        err == ENFILE ||
#endif /* ENFILE */
        0) {
      ldap_pvt_thread_mutex_lock(&slap_daemon[0].sd_mutex);
      emfile++;
      /* Stop listening until an existing session closes */
      sl->sl_mute = 1;
      ldap_pvt_thread_mutex_unlock(&slap_daemon[0].sd_mutex);
    }

    Debug(LDAP_DEBUG_ANY, "daemon: accept(%ld) failed errno=%d (%s)\n",
          (long)sl->sl_sd, err, sock_errstr(err));
    ldap_pvt_thread_yield();
    return 0;
  }

  /* make sure descriptor number isn't too great */
  if (s >= dtblsize) {
    Debug(LDAP_DEBUG_ANY, "daemon: %ld beyond descriptor table size %ld\n",
          (long)s, (long)dtblsize);

    tcp_close(s);
    ldap_pvt_thread_yield();
    return 0;
  }
  tid = DAEMON_ID(s);

#ifdef LDAP_DEBUG
  ldap_pvt_thread_mutex_lock(&slap_daemon[tid].sd_mutex);
  /* newly accepted stream should not be in any of the FD SETS */
  assert(SLAP_SOCK_NOT_ACTIVE(tid, s));
  ldap_pvt_thread_mutex_unlock(&slap_daemon[tid].sd_mutex);
#else
  (void)tid;
#endif /* LDAP_DEBUG */

#ifdef LDAP_PF_LOCAL
  /* for IPv4 and IPv6 sockets only */
  if (from.sa_addr.sa_family != AF_LOCAL)
#endif /* LDAP_PF_LOCAL */
  {
    int rc = ldap_pvt_tcpkeepalive(s, slapd_tcpkeepalive.idle,
                                   slapd_tcpkeepalive.probes,
                                   slapd_tcpkeepalive.interval);
    if (rc) {
      int err = sock_errno();
      Debug(LDAP_DEBUG_ANY,
            "slapd(%ld): tcp-keepalive setup failed "
            "errno=%d (%s)\n",
            (long)s, err, sock_errstr(err));
    }
#ifdef TCP_NODELAY
    /* enable no delay */
    int tmp = 1;
    rc = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *)&tmp, sizeof(tmp));
    if (rc == AC_SOCKET_ERROR) {
      int err = sock_errno();
      Debug(LDAP_DEBUG_ANY,
            "slapd(%ld): setsockopt(TCP_NODELAY) failed "
            "errno=%d (%s)\n",
            (long)s, err, sock_errstr(err));
    }
#endif /* TCP_NODELAY */
  }

  Debug(LDAP_DEBUG_CONNS, "daemon: listen=%ld, new connection on %ld\n",
        (long)sl->sl_sd, (long)s);

  cflag = 0;
  switch (from.sa_addr.sa_family) {
#ifdef LDAP_PF_LOCAL
  case AF_LOCAL:
    cflag |= CONN_IS_IPC;

    /* FIXME: apparently accept doesn't fill
     * the sun_path sun_path member */
    if (from.sa_un_addr.sun_path[0] == '\0') {
      memcpy(from.sa_un_addr.sun_path, sl->sl_sa.sa_un_addr.sun_path,
             sizeof(from.sa_un_addr.sun_path));
    }

    sprintf(peername, "PATH=%s", from.sa_un_addr.sun_path);
    ssf = local_ssf;
    {
      uid_t uid;
      gid_t gid;

#ifdef LDAP_PF_LOCAL_SENDMSG
      peerbv.bv_val = peerbuf;
      peerbv.bv_len = sizeof(peerbuf);
#endif
      if (LUTIL_GETPEEREID(s, &uid, &gid, &peerbv) == 0) {
        authid.bv_val =
            ch_malloc(STRLENOF("gidNumber=4294967295+uidNumber=4294967295,"
                               "cn=peercred,cn=external,cn=auth") +
                      1);
        authid.bv_len = sprintf(authid.bv_val,
                                "gidNumber=%d+uidNumber=%d,"
                                "cn=peercred,cn=external,cn=auth",
                                (int)gid, (int)uid);
        assert(authid.bv_len <=
               STRLENOF("gidNumber=4294967295+uidNumber=4294967295,"
                        "cn=peercred,cn=external,cn=auth"));
      }
    }
    dnsname = "local";
    break;
#endif /* LDAP_PF_LOCAL */

#ifdef LDAP_PF_INET6
  case AF_INET6:
    if (IN6_IS_ADDR_V4MAPPED(&from.sa_in6_addr.sin6_addr)) {
#if defined(HAVE_GETADDRINFO) && defined(HAVE_INET_NTOP)
      peeraddr = inet_ntop(
          AF_INET, ((struct in_addr *)&from.sa_in6_addr.sin6_addr.s6_addr[12]),
          addr, sizeof(addr));
#else  /* ! HAVE_GETADDRINFO || ! HAVE_INET_NTOP */
      peeraddr = inet_ntoa(
          *((struct in_addr *)&from.sa_in6_addr.sin6_addr.s6_addr[12]));
#endif /* ! HAVE_GETADDRINFO || ! HAVE_INET_NTOP */
      if (!peeraddr)
        peeraddr = SLAP_STRING_UNKNOWN;
      sprintf(peername, "IP=%s:%d", peeraddr,
              (unsigned)ntohs(from.sa_in6_addr.sin6_port));
    } else {
      peeraddr =
          inet_ntop(AF_INET6, &from.sa_in6_addr.sin6_addr, addr, sizeof addr);
      if (!peeraddr)
        peeraddr = SLAP_STRING_UNKNOWN;
      sprintf(peername, "IP=[%s]:%d", peeraddr,
              (unsigned)ntohs(from.sa_in6_addr.sin6_port));
    }
    break;
#endif /* LDAP_PF_INET6 */

  case AF_INET: {
#if defined(HAVE_GETADDRINFO) && defined(HAVE_INET_NTOP)
    peeraddr =
        inet_ntop(AF_INET, &from.sa_in_addr.sin_addr, addr, sizeof(addr));
#else  /* ! HAVE_GETADDRINFO || ! HAVE_INET_NTOP */
    peeraddr = inet_ntoa(from.sa_in_addr.sin_addr);
#endif /* ! HAVE_GETADDRINFO || ! HAVE_INET_NTOP */
    if (!peeraddr)
      peeraddr = SLAP_STRING_UNKNOWN;
    sprintf(peername, "IP=%s:%d", peeraddr,
            (unsigned)ntohs(from.sa_in_addr.sin_port));
  } break;

  default:
    slapd_close(s);
    return 0;
  }

  if ((from.sa_addr.sa_family == AF_INET)
#ifdef LDAP_PF_INET6
      || (from.sa_addr.sa_family == AF_INET6)
#endif /* LDAP_PF_INET6 */
  ) {
    dnsname = NULL;
#ifdef SLAPD_RLOOKUPS
    if (use_reverse_lookup) {
      char *herr;
      if (ldap_pvt_get_hname((const struct sockaddr *)&from, len, hbuf,
                             sizeof(hbuf), &herr) == 0) {
        ldap_pvt_str2lower(hbuf);
        dnsname = hbuf;
      }
    }
#endif /* SLAPD_RLOOKUPS */

#ifdef HAVE_TCPD
    {
      int rc;
      ldap_pvt_thread_mutex_lock(&sd_tcpd_mutex);
      rc = hosts_ctl("slapd", dnsname != NULL ? dnsname : SLAP_STRING_UNKNOWN,
                     (char *)peeraddr, SLAP_STRING_UNKNOWN);
      ldap_pvt_thread_mutex_unlock(&sd_tcpd_mutex);
      if (!rc) {
        /* DENY ACCESS */
        Statslog(LDAP_DEBUG_STATS, "fd=%ld DENIED from %s (%s)\n", (long)s,
                 dnsname != NULL ? dnsname : SLAP_STRING_UNKNOWN, peeraddr);
        slapd_close(s);
        return 0;
      }
    }
#endif /* HAVE_TCPD */
  }

#ifdef WITH_TLS
  if (sl->sl_is_tls)
    cflag |= CONN_IS_TLS;
#endif
  c = connection_init(
      s, sl, dnsname != NULL ? dnsname : SLAP_STRING_UNKNOWN, peername, cflag,
      ssf, authid.bv_val ? &authid : NULL LDAP_PF_LOCAL_SENDMSG_ARG(&peerbv));

  if (authid.bv_val)
    ch_free(authid.bv_val);

  if (!c) {
    Debug(LDAP_DEBUG_ANY, "daemon: connection_init(%ld, %s, %s) failed.\n",
          (long)s, peername, sl->sl_name.bv_val);
    slapd_close(s);
  }

  return 0;
}

static void *slap_listener_thread(void *ctx, void *ptr) {
  int rc;
  Listener *sl = (Listener *)ptr;

  rc = slap_listener(sl);

  if (rc != LDAP_SUCCESS) {
    Debug(LDAP_DEBUG_ANY, "slap_listener_thread(%s): failed err=%d",
          sl->sl_url.bv_val, rc);
  }

  return (void *)NULL;
}

static int slap_listener_activate(Listener *sl) {
  int rc;

#ifdef __SANITIZE_THREAD__
  ldap_pvt_thread_mutex_lock(&tsan_mutex);
#endif
  Debug(LDAP_DEBUG_TRACE, "slap_listener_activate(%d): %s%s\n", sl->sl_sd,
        sl->sl_mute ? " muted" : "", sl->sl_busy ? " busy" : "");

  sl->sl_busy = 1;

  rc = ldap_pvt_thread_pool_submit(&connection_pool, slap_listener_thread,
                                   (void *)sl);

  if (rc != 0) {
    Debug(LDAP_DEBUG_ANY, "slap_listener_activate(%d): submit failed (%d)\n",
          sl->sl_sd, rc);

    sl->sl_busy = 0; /* LY: ?! */
  }

#ifdef __SANITIZE_THREAD__
  ldap_pvt_thread_mutex_unlock(&tsan_mutex);
#endif
  return rc;
}

static void *slapd_daemon_task(void *ptr) {
  slap_time_t last_idle_check = {0};
  int ebadf = 0;
  int tid = (ldap_pvt_thread_t *)ptr - listener_tid;

#define SLAPD_IDLE_CHECK_LIMIT 4

  slapd_add(wake_sds[tid][0], 0, NULL, tid);
  if (tid)
    goto loop;

  /* Init stuff done only by thread 0 */

  last_idle_check = ldap_now_steady();

  for (int l = 0; slap_listeners[l] != NULL; l++) {
    if (slap_listeners[l]->sl_sd == AC_SOCKET_INVALID)
      continue;

#ifdef LDAP_CONNECTIONLESS
    /* Since this is connectionless, the data port is the
     * listening port. The listen() and accept() calls
     * are unnecessary.
     */
    if (slap_listeners[l]->sl_is_udp)
      continue;
#endif /* LDAP_CONNECTIONLESS */

      /* FIXME: TCP-only! */
#ifdef LDAP_TCP_BUFFER
    if (1) {
      int origsize, size, realsize, rc;
      socklen_t optlen;
      char buf[SLAP_TEXT_BUFLEN];

      size = 0;
      if (slap_listeners[l]->sl_tcp_rmem > 0) {
        size = slap_listeners[l]->sl_tcp_rmem;
      } else if (slapd_tcp_rmem > 0) {
        size = slapd_tcp_rmem;
      }

      if (size > 0) {
        optlen = sizeof(origsize);
        rc = getsockopt(slap_listeners[l]->sl_sd, SOL_SOCKET, SO_RCVBUF,
                        (void *)&origsize, &optlen);

        if (rc) {
          int err = sock_errno();
          Debug(
              LDAP_DEBUG_ANY,
              "slapd_daemon_task: getsockopt(SO_RCVBUF) failed errno=%d (%s)\n",
              err, sock_errstr(err));
        }

        optlen = sizeof(size);
        rc = setsockopt(slap_listeners[l]->sl_sd, SOL_SOCKET, SO_RCVBUF,
                        (const void *)&size, optlen);

        if (rc) {
          int err = sock_errno();
          Debug(
              LDAP_DEBUG_ANY,
              "slapd_daemon_task: setsockopt(SO_RCVBUF) failed errno=%d (%s)\n",
              err, sock_errstr(err));
        }

        optlen = sizeof(realsize);
        rc = getsockopt(slap_listeners[l]->sl_sd, SOL_SOCKET, SO_RCVBUF,
                        (void *)&realsize, &optlen);

        if (rc) {
          int err = sock_errno();
          Debug(
              LDAP_DEBUG_ANY,
              "slapd_daemon_task: getsockopt(SO_RCVBUF) failed errno=%d (%s)\n",
              err, sock_errstr(err));
        }

        snprintf(buf, sizeof(buf),
                 "url=%s (#%d) RCVBUF original size=%d requested size=%d real "
                 "size=%d",
                 slap_listeners[l]->sl_url.bv_val, l, origsize, size, realsize);
        Debug(LDAP_DEBUG_ANY, "slapd_daemon_task: %s\n", buf);
      }

      size = 0;
      if (slap_listeners[l]->sl_tcp_wmem > 0) {
        size = slap_listeners[l]->sl_tcp_wmem;
      } else if (slapd_tcp_wmem > 0) {
        size = slapd_tcp_wmem;
      }

      if (size > 0) {
        optlen = sizeof(origsize);
        rc = getsockopt(slap_listeners[l]->sl_sd, SOL_SOCKET, SO_SNDBUF,
                        (void *)&origsize, &optlen);

        if (rc) {
          int err = sock_errno();
          Debug(
              LDAP_DEBUG_ANY,
              "slapd_daemon_task: getsockopt(SO_SNDBUF) failed errno=%d (%s)\n",
              err, sock_errstr(err));
        }

        optlen = sizeof(size);
        rc = setsockopt(slap_listeners[l]->sl_sd, SOL_SOCKET, SO_SNDBUF,
                        (const void *)&size, optlen);

        if (rc) {
          int err = sock_errno();
          Debug(LDAP_DEBUG_ANY,
                "slapd_daemon_task: setsockopt(SO_SNDBUF) failed errno=%d (%s)",
                err, sock_errstr(err));
        }

        optlen = sizeof(realsize);
        rc = getsockopt(slap_listeners[l]->sl_sd, SOL_SOCKET, SO_SNDBUF,
                        (void *)&realsize, &optlen);

        if (rc) {
          int err = sock_errno();
          Debug(
              LDAP_DEBUG_ANY,
              "slapd_daemon_task: getsockopt(SO_SNDBUF) failed errno=%d (%s)\n",
              err, sock_errstr(err));
        }

        snprintf(buf, sizeof(buf),
                 "url=%s (#%d) SNDBUF original size=%d requested size=%d real "
                 "size=%d",
                 slap_listeners[l]->sl_url.bv_val, l, origsize, size, realsize);
        Debug(LDAP_DEBUG_ANY, "slapd_daemon_task: %s\n", buf);
      }
    }
#endif /* LDAP_TCP_BUFFER */

    Debug(LDAP_DEBUG_TRACE, "daemon: listener %d, %s, socked %d\n", l,
          slap_listeners[l]->sl_url.bv_val, slap_listeners[l]->sl_sd);

    if (listen(slap_listeners[l]->sl_sd, SLAPD_LISTEN_BACKLOG) == -1) {
      int err = sock_errno();

#ifdef LDAP_PF_INET6
      /* If error is EADDRINUSE, we are trying to listen to INADDR_ANY and
       * we are already listening to in6addr_any, then we want to ignore
       * this and continue.
       */
      if (err == EADDRINUSE) {
        struct sockaddr_in sa = slap_listeners[l]->sl_sa.sa_in_addr;
        struct sockaddr_in6 sa6;

        if (sa.sin_family == AF_INET &&
            sa.sin_addr.s_addr == htonl(INADDR_ANY)) {
          int i;
          for (i = 0; i < l; i++) {
            sa6 = slap_listeners[i]->sl_sa.sa_in6_addr;
            if (sa6.sin6_family == AF_INET6 &&
                !memcmp(&sa6.sin6_addr, &in6addr_any,
                        sizeof(struct in6_addr))) {
              break;
            }
          }

          if (i < l) {
            /* We are already listening to in6addr_any */
            Debug(LDAP_DEBUG_CONNS,
                  "daemon: Attempt to listen to 0.0.0.0 failed, "
                  "already listening on ::, assuming IPv4 included\n");
            slapd_close(slap_listeners[l]->sl_sd);
            slap_listeners[l]->sl_sd = AC_SOCKET_INVALID;
            slap_listeners[l]->sl_mute = 1;
            continue;
          }
        }
      }
#endif /* LDAP_PF_INET6 */
      Debug(LDAP_DEBUG_ANY, "daemon: listen(%s, %i) failed errno=%d (%s)\n",
            slap_listeners[l]->sl_url.bv_val, SLAPD_LISTEN_BACKLOG, err,
            sock_errstr(err));
      return (void *)-1;
    }
    slap_listeners[l]->sl_mute = 0;

    /* make the listening socket non-blocking */
    if (ber_pvt_socket_set_nonblock(slap_listeners[l]->sl_sd, 1) < 0) {
      Debug(LDAP_DEBUG_ANY, "slapd_daemon_task: "
                            "set nonblocking on a listening socket failed\n");
      set_shutdown(SHUT_RDWR);
      return (void *)-1;
    }

    slapd_add(slap_listeners[l]->sl_sd, 0, slap_listeners[l], -1);
  }

loop:

  /* initialization complete. Here comes the loop. */

  while (!slapd_shutdown) {
#define SLAPD_EBADF_LIMIT 16
    SLAP_EVENT_DECL;
    slap_time_t tv, now = ldap_now_steady();

    tv.ns = 0;
    if (!tid) {
      int check = 0;
      /* Set the select timeout. */
      if (global_idletimeout > 0 &&
          now.ns >
              last_idle_check.ns +
                  ldap_from_seconds(global_idletimeout / SLAPD_IDLE_CHECK_LIMIT)
                      .ns) {
        check = 1;
        tv = ldap_from_seconds(global_idletimeout / SLAPD_IDLE_CHECK_LIMIT);
      }
#ifdef SLAPD_WRITE_TIMEOUT
      if (chk_writetime.ns && now.ns > chk_writetime.ns) {
        check = 2;
        if (!tv.ns || tv.ns > ldap_from_seconds(global_writetimeout).ns)
          tv = ldap_from_seconds(global_writetimeout);
      }
#endif /* SLAPD_WRITE_TIMEOUT */
      if (check) {
        connections_timeout_idle(now);
        last_idle_check = now;
      }
    }

    if (slapd_gentle_shutdown) {
      ber_socket_t active;

      if (!tid && slapd_gentle_shutdown == 1) {
        BackendDB *be;
        Debug(LDAP_DEBUG_ANY, "slapd gentle shutdown\n");
        close_listeners(1);
        frontendDB->be_restrictops |= SLAP_RESTRICT_OP_WRITES;
        LDAP_STAILQ_FOREACH(be, &backendDB, be_next) {
          be->be_restrictops |= SLAP_RESTRICT_OP_WRITES;
        }
        connections_shutdown(SHUT_WR);
        if (reopenldap_mode_righteous() && !global_gentlehup) {
          if (!global_idletimeout)
            global_idletimeout = 5;
          if (!global_writetimeout)
            global_writetimeout = 5;
        }
        set_gentle_shutdown(SHUT_RDWR);
      }

      ldap_pvt_thread_mutex_lock(&slap_daemon[tid].sd_mutex);
      active = slap_daemon[tid].sd_nactives;
      ldap_pvt_thread_mutex_unlock(&slap_daemon[tid].sd_mutex);

      if (active == 0) {
        if (!tid) {
          for (int l = 1; l < slapd_daemon_threads; l++) {
            ldap_pvt_thread_mutex_lock(&slap_daemon[l].sd_mutex);
            active += slap_daemon[l].sd_nactives;
            ldap_pvt_thread_mutex_unlock(&slap_daemon[l].sd_mutex);
          }
          if (!active)
            set_shutdown(SHUT_WR);
        }
        if (!active)
          break;
      }
    }

    ldap_pvt_thread_mutex_lock(&slap_daemon[tid].sd_mutex);

#ifdef __SANITIZE_THREAD__
    ldap_pvt_thread_mutex_lock(&tsan_mutex);
#endif
#if SLAP_EVENTS_ARE_INDEXED
    const int nwriters = slap_daemon[tid].sd_nwriters;
#endif
    if (listening)
      for (int l = 0; slap_listeners[l] != NULL; l++) {
        Listener *lr = slap_listeners[l];

        if (lr->sl_sd == AC_SOCKET_INVALID)
          continue;
        if (DAEMON_ID(lr->sl_sd) != tid)
          continue;
        if (!SLAP_SOCK_IS_ACTIVE(tid, lr->sl_sd))
          continue;

        if (lr->sl_mute || lr->sl_busy) {
          SLAP_SOCK_CLR_READ(tid, lr->sl_sd);
        } else {
          SLAP_SOCK_SET_READ(tid, lr->sl_sd);
        }
      }
#ifdef __SANITIZE_THREAD__
    ldap_pvt_thread_mutex_unlock(&tsan_mutex);
#endif

    SLAP_EVENT_INIT(tid);
    slap_time_t *tvp = (tv.ns && slap_daemon[tid].sd_nactives) ? &tv : NULL;

    ldap_pvt_thread_mutex_unlock(&slap_daemon[tid].sd_mutex);

    /* Only thread 0 handles runqueue */
    if (!tid) {
      ldap_pvt_thread_mutex_lock(&slapd_rq.rq_mutex);
      slap_time_t cat;
      struct re_s *rtask = ldap_pvt_runqueue_next_sched(&slapd_rq, &cat);
      while (rtask && cat.ns && cat.ns <= now.ns) {
        if (ldap_pvt_runqueue_isrunning(&slapd_rq, rtask)) {
          ldap_pvt_runqueue_resched(&slapd_rq, rtask, 0);
        } else {
          ldap_pvt_runqueue_runtask(&slapd_rq, rtask);
          ldap_pvt_runqueue_resched(&slapd_rq, rtask, 0);
          ldap_pvt_thread_mutex_unlock(&slapd_rq.rq_mutex);
          ldap_pvt_thread_pool_submit2(&connection_pool, rtask->routine,
                                       (void *)rtask, &rtask->pool_cookie);
          ldap_pvt_thread_mutex_lock(&slapd_rq.rq_mutex);
        }
        rtask = ldap_pvt_runqueue_next_sched(&slapd_rq, &cat);
      }
      ldap_pvt_thread_mutex_unlock(&slapd_rq.rq_mutex);

      if (rtask && cat.ns) {
        /* NOTE: diff __should__ always be >= 0,
         * AFAI understand; however (ITS#4872),
         * time_t might be unsigned in some systems,
         * while difftime() returns a double */
        int64_t diff_ns = cat.ns - now.ns;
        if (diff_ns < 0)
          diff_ns = 0;
        if (tvp == NULL || diff_ns < (int64_t)tv.ns) {
          tv.ns = diff_ns;
          tvp = &tv;
        }
      }
    }

#ifdef __SANITIZE_THREAD__
    ldap_pvt_thread_mutex_lock(&tsan_mutex);
#endif
    for (int l = 0; slap_listeners[l] != NULL; l++) {
      Listener *lr = slap_listeners[l];

      if (lr->sl_sd == AC_SOCKET_INVALID) {
        continue;
      }

      if (lr->sl_mute || lr->sl_busy) {
        Debug(LDAP_DEBUG_CONNS, "daemon: " SLAP_EVENT_FNAME ": listen=%d%s%s\n",
              lr->sl_sd, lr->sl_mute ? " muted" : "",
              lr->sl_busy ? " busy" : "");
        continue;
      }

      Debug(LDAP_DEBUG_TRACE,
            "daemon: " SLAP_EVENT_FNAME ": "
            "listen=%d tvp=%s%" PRIu64 "\n",
            lr->sl_sd, tvp == NULL ? "NULL/" : "", tvp ? tvp->ns : 0);
    }
#ifdef __SANITIZE_THREAD__
    ldap_pvt_thread_mutex_unlock(&tsan_mutex);
#endif

    int ns;
    SLAP_EVENT_WAIT(tid, tvp, &ns);
    switch (ns) {
    case -1: { /* failure - try again */
      int err = sock_errno();

      if (err != EINTR) {
        ebadf++;

        /* Don't log unless we got it twice in a row */
        if (!(ebadf & 1)) {
          Debug(LDAP_DEBUG_ANY,
                "daemon: " SLAP_EVENT_FNAME " failed count %d "
                "err (%d): %s\n",
                ebadf, err, sock_errstr(err));
        }
        if (ebadf >= SLAPD_EBADF_LIMIT) {
          set_shutdown(SHUT_RDWR);
        }
      }
    }
      continue;

    case 0: /* timeout - let threads run */
      ebadf = 0;
#ifndef HAVE_YIELDING_SELECT
      Debug(LDAP_DEBUG_CONNS,
            "daemon: " SLAP_EVENT_FNAME "timeout - yielding\n");

      ldap_pvt_thread_yield();
#endif /* ! HAVE_YIELDING_SELECT */
      continue;

    default: /* something happened - deal with it */
      if (slapd_shutdown)
        continue;

      ebadf = 0;
      Debug(LDAP_DEBUG_CONNS, "daemon: activity on %d descriptor%s\n", ns,
            ns != 1 ? "s" : "");
      /* FALL THRU */
    }

#if SLAP_EVENTS_ARE_INDEXED
    if (SLAP_EVENT_IS_READ(wake_sds[tid][0])) {
      char c[BUFSIZ];
      SLAP_EVENT_CLR_READ(wake_sds[tid][0]);
      waking = 0;
      tcp_read(wake_sds[tid][0], c, sizeof(c));
      Debug(LDAP_DEBUG_CONNS, "daemon: waked\n");
      continue;
    }

    /* The event slot equals the descriptor number - this is
     * true for Unix select and poll. We treat Windows select
     * like this too, even though it's a kludge.
     */
    if (listening)
      for (int l = 0; slap_listeners[l] != NULL; l++) {
        int rc;

        if (ns <= 0)
          break;
        if (slap_listeners[l]->sl_sd == AC_SOCKET_INVALID)
          continue;
#ifdef LDAP_CONNECTIONLESS
        if (slap_listeners[l]->sl_is_udp)
          continue;
#endif /* LDAP_CONNECTIONLESS */
        if (!SLAP_EVENT_IS_READ(slap_listeners[l]->sl_sd))
          continue;

        /* clear events */
        SLAP_EVENT_CLR_READ(slap_listeners[l]->sl_sd);
        SLAP_EVENT_CLR_WRITE(slap_listeners[l]->sl_sd);
        ns--;

        rc = slap_listener_activate(slap_listeners[l]);
      }

    /* bypass the following tests if no descriptors left */
    if (ns <= 0) {
#ifndef HAVE_YIELDING_SELECT
      ldap_pvt_thread_yield();
#endif /* HAVE_YIELDING_SELECT */
      continue;
    }

    Debug(LDAP_DEBUG_CONNS, "daemon: activity on:");
    const int nfds = SLAP_EVENT_MAX(tid);
    int nrfds = 0;
    int nwfds = 0;
    for (int i = 0; i < nfds; i++) {
      int r, w;

      r = SLAP_EVENT_IS_READ(i);
      /* writefds was not initialized if nwriters was zero */
      w = nwriters ? SLAP_EVENT_IS_WRITE(i) : 0;
      if (r || w) {
        Debug(LDAP_DEBUG_CONNS, " %d%s%s", i, r ? "r" : "", w ? "w" : "");
        if (r) {
          nrfds++;
          ns--;
        }
        if (w) {
          nwfds++;
          ns--;
        }
      }
      if (ns <= 0)
        break;
    }
    Debug(LDAP_DEBUG_CONNS, ".\n");

    /* loop through the writers */
    for (int i = 0; nwfds > 0 && i < nfds; i++) {
      ber_socket_t wd;
      if (!SLAP_EVENT_IS_WRITE(i))
        continue;
      wd = i;

      SLAP_EVENT_CLR_WRITE(wd);
      nwfds--;

      Debug(LDAP_DEBUG_CONNS, "daemon: write active on %d\n", wd);

      /*
       * NOTE: it is possible that the connection was closed
       * and that the stream is now inactive.
       * connection_write() must validate the stream is still
       * active.
       *
       * ITS#4338: if the stream is invalid, there is no need to
       * close it here. It has already been closed in connection.c.
       */
      if (connection_write(wd) < 0) {
        if (SLAP_EVENT_IS_READ(wd)) {
          SLAP_EVENT_CLR_READ((unsigned)wd);
          nrfds--;
        }
      }
    }

    for (int i = 0; nrfds > 0 && i < nfds; i++) {
      ber_socket_t rd;
      if (!SLAP_EVENT_IS_READ(i))
        continue;
      rd = i;
      SLAP_EVENT_CLR_READ(rd);
      nrfds--;

      Debug(LDAP_DEBUG_CONNS, "daemon: read activity on %d\n", rd);
      /*
       * NOTE: it is possible that the connection was closed
       * and that the stream is now inactive.
       * connection_read() must valid the stream is still
       * active.
       */

      connection_read_activate(rd);
    }
#else /* !SLAP_EVENTS_ARE_INDEXED */
    /* FIXME */
    /* The events are returned in an arbitrary list. This is true
     * for /dev/poll, epoll and kqueue. In order to prioritize things
     * so that we can handle wake_sds first, listeners second, and then
     * all other connections last (as we do for select), we would need
     * to use multiple event handles and cascade them.
     *
     * That seems like a bit of hassle. So the wake_sds check has been
     * skipped. For epoll and kqueue we can associate arbitrary data with
     * an event, so we could use pointers to the listener structure
     * instead of just the file descriptor. For /dev/poll we have to
     * search the listeners array for a matching descriptor.
     *
     * We now handle wake events when we see them; they are not given
     * higher priority.
     */
#ifdef LDAP_DEBUG
    Debug(LDAP_DEBUG_CONNS, "daemon: activity on:");

    for (int i = 0; i < ns; i++) {
      int r, w, fd;

      /* Don't log listener events */
      if (SLAP_EVENT_IS_LISTENER(tid, i)
#ifdef LDAP_CONNECTIONLESS
          && !((SLAP_EVENT_LISTENER(tid, i))->sl_is_udp)
#endif /* LDAP_CONNECTIONLESS */
      ) {
        continue;
      }

      fd = SLAP_EVENT_FD(tid, i);
      /* Don't log internal wake events */
      if (fd == wake_sds[tid][0])
        continue;

      r = SLAP_EVENT_IS_READ(i);
      w = SLAP_EVENT_IS_WRITE(i);
      if (r || w) {
        Debug(LDAP_DEBUG_CONNS, " %d%s%s", fd, r ? "r" : "", w ? "w" : "");
      }
    }
    Debug(LDAP_DEBUG_CONNS, ".\n");
#endif /* LDAP_DEBUG */

    for (int i = 0; i < ns; i++) {
      int rc = 1, fd, w = 0, r = 0;

      if (SLAP_EVENT_IS_LISTENER(tid, i)) {
        rc = slap_listener_activate(SLAP_EVENT_LISTENER(tid, i));
      }

      /* If we found a regular listener, rc is now zero, and we
       * can skip the data portion. But if it was a UDP listener
       * then rc is still 1, and we want to handle the data.
       */
      if (rc) {
        fd = SLAP_EVENT_FD(tid, i);

        /* Handle wake events */
        if (fd == wake_sds[tid][0]) {
          char c[BUFSIZ];
          waking = 0;
          int ignore __maybe_unused = tcp_read(wake_sds[tid][0], c, sizeof(c));
          continue;
        }

        if (SLAP_EVENT_IS_WRITE(i)) {
          Debug(LDAP_DEBUG_CONNS, "daemon: write active on %d\n", fd);

          SLAP_EVENT_CLR_WRITE(i);
          w = 1;

          /*
           * NOTE: it is possible that the connection was closed
           * and that the stream is now inactive.
           * connection_write() must valid the stream is still
           * active.
           */
          if (connection_write(fd) < 0)
            w = -2;
        }
        /* If event is a read */
        if (SLAP_EVENT_IS_READ(i)) {
          r = 1;
          Debug(LDAP_DEBUG_CONNS, "daemon: read active on %d\n", fd);

          SLAP_EVENT_CLR_READ(i);
          connection_read_activate(fd);
        }
        if (r + w < 0
#ifdef SLAP_EVENT_IS_ERROR
            || SLAP_EVENT_IS_ERROR(fd)
#endif
        ) {
          Debug(LDAP_DEBUG_CONNS, "daemon: socket troube on %d\n", fd);
          if (connections_socket_trouble(fd) < 0) {
#ifdef HAVE_EPOLL
            /* Don't keep reporting the hangup */
            ldap_pvt_thread_mutex_lock(&slap_daemon[tid].sd_mutex);
            if (SLAP_SOCK_IS_ACTIVE(tid, fd))
              SLAP_EPOLL_SOCK_SET(tid, fd, EPOLLET);
            ldap_pvt_thread_mutex_unlock(&slap_daemon[tid].sd_mutex);
#endif /* HAVE_EPOLL */
          }
          continue;
        }
      }
    }
#endif /* SLAP_EVENTS_ARE_INDEXED */

#ifndef HAVE_YIELDING_SELECT
    ldap_pvt_thread_yield();
#endif /* ! HAVE_YIELDING_SELECT */
  }

  /* Only thread 0 handles shutdown */
  if (tid)
    return NULL;

  if (slapd_shutdown == 1) {
    Debug(LDAP_DEBUG_ANY, "daemon: shutdown requested and initiated.\n");

  } else if (slapd_shutdown == 2) {
    Debug(LDAP_DEBUG_ANY, "daemon: abnormal condition, shutdown initiated.\n");
  } else {
    Debug(LDAP_DEBUG_ANY, "daemon: no active streams, shutdown initiated.\n");
  }

  close_listeners(1);

  if (!slapd_gentle_shutdown) {
    set_abrupt_shutdown(SHUT_WR);
    connections_shutdown(SHUT_RD);
  }

  if (DebugTest(LDAP_DEBUG_ANY)) {
    int t = ldap_pvt_thread_pool_backload(&connection_pool);
    Debug(LDAP_DEBUG_ANY,
          "slapd shutdown: waiting for %d operations/tasks to finish\n", t);
  }
  ldap_pvt_thread_pool_close(&connection_pool, 1);

  return NULL;
}

#ifdef LDAP_CONNECTIONLESS
static int connectionless_init(void) {
  int l;

  for (l = 0; slap_listeners[l] != NULL; l++) {
    Listener *lr = slap_listeners[l];
    Connection *c;

    if (!lr->sl_is_udp) {
      continue;
    }

    c = connection_init(lr->sl_sd, lr, "", "", CONN_IS_UDP, (slap_ssf_t)0,
                        NULL LDAP_PF_LOCAL_SENDMSG_ARG(NULL));

    if (!c) {
      Debug(LDAP_DEBUG_TRACE, "connectionless_init: failed on %s (%d)\n",
            lr->sl_url.bv_val, lr->sl_sd);
      return -1;
    }
    lr->sl_is_udp++;
  }

  return 0;
}
#endif /* LDAP_CONNECTIONLESS */

int slapd_daemon(void) {
  int i, rc;

  Debug(LDAP_DEBUG_ANY, "daemon: using " SLAP_EVENT_FNAME "\n");

#ifdef LDAP_CONNECTIONLESS
  connectionless_init();
#endif /* LDAP_CONNECTIONLESS */

  if (slapd_daemon_threads > SLAPD_MAX_DAEMON_THREADS)
    slapd_daemon_threads = SLAPD_MAX_DAEMON_THREADS;

  listener_tid = ch_malloc(slapd_daemon_threads * sizeof(ldap_pvt_thread_t));

  /* daemon_init only inits element 0 */
  for (i = 1; i < slapd_daemon_threads; i++) {
    ldap_pvt_thread_mutex_init(&slap_daemon[i].sd_mutex);

    if ((rc = lutil_pair(wake_sds[i])) < 0) {
      Debug(LDAP_DEBUG_ANY, "daemon: lutil_pair() failed rc=%d\n", rc);
      return rc;
    }
    ber_pvt_socket_set_nonblock(wake_sds[i][1], 1);

    SLAP_SOCK_INIT(i);
  }

  for (i = 0; i < slapd_daemon_threads; i++) {
    /* listener as a separate THREAD */
    rc = ldap_pvt_thread_create(&listener_tid[i], 0, slapd_daemon_task,
                                &listener_tid[i]);

    if (rc != 0) {
      Debug(LDAP_DEBUG_ANY, "listener ldap_pvt_thread_create failed (%d)\n",
            rc);
      return rc;
    }
  }

  /* wait for the listener threads to complete */
  for (i = 0; i < slapd_daemon_threads; i++)
    ldap_pvt_thread_join(listener_tid[i], (void *)NULL);

  ch_free(listener_tid);
  listener_tid = NULL;

  return 0;
}

void slap_sig_shutdown(int sig) {
  int save_errno = errno;
  int i;

#if 0
	Debug(LDAP_DEBUG_TRACE, "slap_sig_shutdown: signal %d\n", sig);
#endif

  /*
   * If the NT Service Manager is controlling the server, we don't
   * want SIGBREAK to kill the server. For some strange reason,
   * SIGBREAK is generated when a user logs out.
   */

  if ((reopenldap_mode_righteous() ||
#ifdef SIGHUP
       sig == SIGHUP
#endif /* SIGHUP */
       ) &&
      global_gentlehup && slapd_gentle_shutdown == 0) {
    set_gentle_shutdown(SHUT_WR);
  } else {
    set_shutdown(SHUT_WR);
  }

  for (i = 0; i < slapd_daemon_threads; i++) {
    WAKE_LISTENER(i, 1);
  }

  /* reinstall self */
  (void)SIGNAL_REINSTALL(sig, slap_sig_shutdown);

  errno = save_errno;
}

void slap_sig_wake(int sig) {
  int save_errno = errno;

  WAKE_LISTENER(0, 1);

  /* reinstall self */
  (void)SIGNAL_REINSTALL(sig, slap_sig_wake);

  errno = save_errno;
}

int slap_pause_server(void) {
  BackendInfo *bi;
  int rc = LDAP_SUCCESS;

  rc = ldap_pvt_thread_pool_pause(&connection_pool);

  LDAP_STAILQ_FOREACH(bi, &backendInfo, bi_next) {
    if (bi->bi_pause) {
      rc = bi->bi_pause(bi);
      if (rc != LDAP_SUCCESS) {
        Debug(LDAP_DEBUG_ANY,
              "slap_pause_server: "
              "bi_pause failed for backend %s\n",
              bi->bi_type);
        return rc;
      }
    }
  }

  return rc;
}

int slap_unpause_server(void) {
  BackendInfo *bi;
  int rc = LDAP_SUCCESS;

  LDAP_STAILQ_FOREACH(bi, &backendInfo, bi_next) {
    if (bi->bi_unpause) {
      rc = bi->bi_unpause(bi);
      if (rc != LDAP_SUCCESS) {
        Debug(LDAP_DEBUG_ANY,
              "slap_unpause_server: "
              "bi_unpause failed for backend %s\n",
              bi->bi_type);
        return rc;
      }
    }
  }

  rc = ldap_pvt_thread_pool_resume(&connection_pool);
  return rc;
}

void slapd_add_internal(ber_socket_t s, int isactive) {
  slapd_add(s, isactive, NULL, -1);
}

Listener **slapd_get_listeners(void) {
  /* Could return array with no listeners if !listening, but current
   * callers mostly look at the URLs.  E.g. syncrepl uses this to
   * identify the server, which means it wants the startup arguments.
   */
  return slap_listeners;
}

/* Reject all incoming requests */
void slap_suspend_listeners(void) {
  int i;
#ifdef __SANITIZE_THREAD__
  ldap_pvt_thread_mutex_lock(&tsan_mutex);
#endif
  for (i = 0; slap_listeners[i]; i++) {
    if (slap_listeners[i]->sl_mute ||
        slap_listeners[i]->sl_sd == AC_SOCKET_INVALID)
      continue;
    Debug(LDAP_DEBUG_TRACE, "daemon: suspend listener %d, %s, socked %d\n", i,
          slap_listeners[i]->sl_url.bv_val, slap_listeners[i]->sl_sd);
    slap_listeners[i]->sl_mute = 1;
    listen(slap_listeners[i]->sl_sd, 0);
  }
#ifdef __SANITIZE_THREAD__
  ldap_pvt_thread_mutex_unlock(&tsan_mutex);
#endif
}

/* Resume after a suspend */
void slap_resume_listeners(void) {
  int i;
#ifdef __SANITIZE_THREAD__
  ldap_pvt_thread_mutex_lock(&tsan_mutex);
#endif
  for (i = 0; slap_listeners[i]; i++) {
    if (slap_listeners[i]->sl_mute == 0 ||
        slap_listeners[i]->sl_sd == AC_SOCKET_INVALID)
      continue;
    Debug(LDAP_DEBUG_TRACE, "daemon: resume listener %d, %s, socked %d\n", i,
          slap_listeners[i]->sl_url.bv_val, slap_listeners[i]->sl_sd);
    slap_listeners[i]->sl_mute = 0;
    listen(slap_listeners[i]->sl_sd, SLAPD_LISTEN_BACKLOG);
  }
#ifdef __SANITIZE_THREAD__
  ldap_pvt_thread_mutex_unlock(&tsan_mutex);
#endif
}

void slap_wake_listener() { WAKE_LISTENER(0, 1); }

/* return 0 on timeout, 1 on writer ready
 * -1 on general error
 */
int slapd_wait_writer(ber_socket_t sd) {
  struct pollfd fds;
  int timeout = global_writetimeout ? global_writetimeout * 1000 : -1;

  fds.fd = sd;
  fds.events = POLLOUT;

  return poll(&fds, 1, timeout);
}

int config_keepalive(ConfigArgs *c) {
  if (c->op == SLAP_CONFIG_EMIT) {
    if (slapd_tcpkeepalive.idle < 0 && slapd_tcpkeepalive.probes < 0 &&
        slapd_tcpkeepalive.interval < 0) {
      c->value_string = ch_strdup("default");
    } else if (slapd_tcpkeepalive.idle == 0 && slapd_tcpkeepalive.probes == 0 &&
               slapd_tcpkeepalive.interval == 0) {
      c->value_string = ch_strdup("disable");
    } else {
      char buf[64], *s = buf;
      if (slapd_tcpkeepalive.idle >= 0)
        s += snprintf(s, buf + sizeof(buf) - s, "%d", slapd_tcpkeepalive.idle);
      *s++ = ':';
      if (slapd_tcpkeepalive.probes >= 0)
        s +=
            snprintf(s, buf + sizeof(buf) - s, "%d", slapd_tcpkeepalive.probes);
      *s++ = ':';
      if (slapd_tcpkeepalive.interval >= 0)
        s += snprintf(s, buf + sizeof(buf) - s, "%d",
                      slapd_tcpkeepalive.interval);
      *s = '\0';
    }
    return 0;
  } else if (c->op == LDAP_MOD_DELETE ||
             strcasecmp(c->value_string, "default") == 0) {
    slapd_tcpkeepalive.idle = -1;
    slapd_tcpkeepalive.probes = -1;
    slapd_tcpkeepalive.interval = -1;
    return 0;
  } else if (strcasecmp(c->value_string, "disable") == 0) {
    slapd_tcpkeepalive.idle = 0;
    slapd_tcpkeepalive.probes = 0;
    slapd_tcpkeepalive.interval = 0;
    return 0;
  } else {
    int idle, probes, interval;
    char *s = c->value_string;

    idle = -1;
    if (*s) {
      if (*s != ':') {
        idle = strtoul(s, &s, 10);
        if (idle < 0)
          goto bailout;
      }
      if (*s == ':')
        s++;
    }

    probes = -1;
    if (*s) {
      if (*s != ':') {
        probes = strtoul(s, &s, 10);
        if (probes < 0)
          goto bailout;
      }
      if (*s == ':')
        s++;
    }

    interval = -1;
    if (*s) {
      interval = strtoul(s, &s, 10);
      if (interval < 0)
        goto bailout;
    }

    if (*s == '\0') {
      slapd_tcpkeepalive.idle = idle;
      slapd_tcpkeepalive.probes = probes;
      slapd_tcpkeepalive.interval = interval;
      return 0;
    }

  bailout:
    snprintf(c->cr_msg, sizeof(c->cr_msg), "<%s> invalid specification",
             c->argv[0]);
    Debug(LDAP_DEBUG_ANY, "%s: %s '%s'\n", c->log, c->cr_msg, c->value_string);
    return 1;
  }
}
