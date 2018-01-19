/* $ReOpenLDAP$ */
/* Copyright 1990-2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

/* backtrace.c - stack backtrace routine */

#include "reldap.h"

#ifdef HAVE_ENOUGH4BACKTRACE

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <execinfo.h>
#include <ucontext.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <sys/syscall.h>
#include <errno.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "slap.h"
#include "slapconfig.h"
#include "ac/errno.h"

/* LY: avoid collision with slapd memory checking. */
#ifdef free
#	undef free
#endif

#ifdef HAVE_LIBBFD
/* Include for BFD processing */
#	include "bfd.h"
#endif /* HAVE_LIBBFD */

#ifdef HAVE_LIBELF
/* Include for ELF processing */
#	include <libelf.h>
#	include <gelf.h>
#endif /* HAVE_LIBELF */

#ifndef PR_SET_PTRACER
#	define PR_SET_PTRACER 0x59616d61
#	define PR_SET_PTRACER_ANY ((unsigned long)-1)
#endif

/*
   It is convenient to get backtrace of a 'guilty' thread and its stack frame,
   which had generated SIGSEGV/SIGBUS or SIGABORT. But the problem is than
   no robust way to do so, when gdb executing from a signal handler.

GDB_SWITCH2GUILTY_THREAD == 0
   In this case a full and the same backtrace will be made for all threads,
   without a switching to the guilty stack frame or a thread.
   This is ROBUST WAY, but the problematic stack frame and thread should
   be found manually.

GDB_SWITCH2GUILTY_THREAD != 0
   In this case a 'ping-pong' of control will be used for switch gdb to
   the 'guilty' thread. By some unclear conditions this could fail,
   so this is UNRELIABLE.


DETAILS:
   Unfortunately gdb unable to select the thread by LWP/TID. Therefore,
   in general, to do so we should:
	- Issue the 'continue' command to gdb, after it was started
	  and attached to the our process;
	- Send SIGTRAP to a `guilty` thread, gdb should catch this signal
	  and switch to the subject thread;
	- Give the 'bt' command to gdb, after it has captured SIGTRAP
	  and got the control back.

   By incomprehensible reasons this may fail in some gdb versions,
   especially a modern. */
#define GDB_SWITCH2GUILTY_THREAD 0

static char* backtrace_homedir;
static void backtrace_cleanup(void)
{
	if (backtrace_homedir) {
		ch_free(backtrace_homedir);
		backtrace_homedir = NULL;
	}
}

#if defined(HAVE_LIBBFD) && !defined(PS_COMPAT_RHEL6)
static int is_bfd_symbols_available(void) {
	char name_buf[PATH_MAX];
	int n = readlink("/proc/self/exe", name_buf, sizeof(name_buf) - 1);
	if (n < 1)
		return 0;

	bfd *abfd;
	char** matching;

	bfd_init();
	name_buf[n] = 0;
	abfd = bfd_openr(name_buf, NULL);
	if (! abfd)
		return 0;

	n = 0;
	if ((bfd_get_file_flags(abfd) & HAS_SYMS) != 0
			&& bfd_check_format_matches(abfd, bfd_object, &matching))
		n = bfd_get_symtab_upper_bound(abfd) > 0;
	bfd_close(abfd);
	return n;
}
#else
#	define is_bfd_symbols_available() (0)
#endif /* HAVE_LIBBFD */

#ifdef HAVE_LIBELF
static int is_elf_symbols_available(void) {
	if (elf_version(EV_CURRENT) == EV_NONE)
		return 0;

	char name_buf[PATH_MAX];
	int n = readlink("/proc/self/exe", name_buf, sizeof(name_buf) - 1);
	if (n < 1)
		return 0;

	name_buf[n] = 0;

	/* Open ELF file to obtain file descriptor */
	int fd;
	if((fd = open(name_buf, O_RDONLY)) < 0)
		return 0;

	Elf *elf;       /* ELF pointer for libelf */
	Elf_Scn *scn;   /* section descriptor pointer */
	GElf_Shdr shdr; /* section header */
	n = 0;

	/* Initialize elf pointer for examining contents of file */
	elf = elf_begin(fd, ELF_C_READ, NULL);
	if (elf) {

		/* Initialize section descriptor pointer so that elf_nextscn()
		 * returns a pointer to the section descriptor at index 1. */
		scn = NULL;

		/* Iterate through ELF sections */
		while((scn = elf_nextscn(elf, scn)) != NULL) {
			/* Retrieve section header */
			gelf_getshdr(scn, &shdr);

			/* If a section header holding a symbol table (.symtab)
			 * is found, this ELF file has not been stripped. */
			if(shdr.sh_type == SHT_SYMTAB) {
				n = 1;
				break;
			}
		}

		elf_end(elf);
	}

	close(fd);
	return n;
}
#else
#	define is_elf_symbols_available() (0)
#endif /* HAVE_LIBELF */

static int is_debugger_present(void) {
	int fd = open("/proc/self/status", O_RDONLY);
	if (fd == -1)
		return 0;

	char buf[1024];
	int debugger_pid = 0;
	ssize_t num_read = read(fd, buf, sizeof(buf));
	if (num_read > 0) {
		static const char TracerPid[] = "TracerPid:";
		char *tracer = strstr(buf, TracerPid);
		if (tracer)
			debugger_pid = atoi(tracer + sizeof(TracerPid) - 1);
	}

	close(fd);
	return debugger_pid != 0;
}

static int is_valgrind_present() {
#ifdef RUNNING_ON_VALGRIND
	if (RUNNING_ON_VALGRIND)
		return 1;
#endif
	const char *str = getenv("RUNNING_ON_VALGRIND");
	if (str)
		return strcmp(str, "0") != 0;
	return 0;
}

volatile int gdb_is_ready_for_backtrace __attribute__((visibility("default")));

static ATTRIBUTE_NO_SANITIZE_THREAD
void backtrace_sigaction(int signum, siginfo_t *info, void* ptr) {
	void *array[42];
	size_t size;
	void * caller_address;
	ucontext_t *uc = (ucontext_t *) ptr;
	int gdb_pid = -1;

	/* get all entries on the stack */
	size = backtrace(array, 42);

	/* Get the address at the time the signal was raised */
#if defined(REG_RIP)
	caller_address = (void *) uc->uc_mcontext.gregs[REG_RIP];
#elif defined(REG_EIP)
	caller_address = (void *) uc->uc_mcontext.gregs[REG_EIP];
#elif defined(__arm__)
	caller_address = (void *) uc->uc_mcontext.arm_pc;
#elif defined(__mips__)
	caller_address = (void *) uc->uc_mcontext.sc_pc;
#else /* TODO support more arch(s) */
#	warning Unsupported architecture.
	caller_address = NULL;
#endif

	int should_die = 0;
	switch(info->si_signo) {
	case SIGXCPU:
	case SIGXFSZ:
		should_die = 1;
		break;
	case SIGABRT:
	case SIGALRM:
		if (info->si_pid == getpid())
			should_die = 1;
	case SIGBUS:
	case SIGFPE:
	case SIGILL:
	case SIGSEGV:
#ifndef SI_FROMKERNEL
#	define SI_FROMKERNEL(info) (info->si_code > 0)
#endif
		if (SI_FROMKERNEL(info))
			should_die = 1;
	}

	char time_buf[64];
	time_t t = ldap_time_unsteady();
	strftime(time_buf, sizeof(time_buf), "%F-%H%M%S", localtime(&t));

	char name_buf[PATH_MAX];
	int fd = -1;
#ifdef snprintf
#	undef snprintf
#endif
	if (snprintf(name_buf, sizeof(name_buf), "%s/slapd-backtrace.%s-%i.log%c",
				 backtrace_homedir ? backtrace_homedir : ".", time_buf, getpid(), 0) > 0)
		fd = open(name_buf, O_CREAT | O_EXCL | O_WRONLY | O_APPEND, 0644);

	if (fd < 0) {
		if (backtrace_homedir)
			fd = open(strrchr(name_buf, '/') + 1, O_CREAT | O_EXCL | O_WRONLY | O_APPEND, 0644);
		if (fd < 0)
			fd = STDERR_FILENO;
		dprintf(fd, "\n\n*** Unable create \"%s\": %s!", name_buf, STRERROR(errno));
	}

	dprintf(fd, "\n\n*** Signal %d (%s), address is %p from %p\n", signum, strsignal(signum),
			info->si_addr, (void *)caller_address);

	int n = readlink("/proc/self/exe", name_buf, sizeof(name_buf) - 1);
	if (n > 0) {
		name_buf[n] = 0;
		dprintf(fd, "    Executable file %s\n", name_buf);
	} else {
		dprintf(fd, "    Unable read executable name: %s\n", STRERROR(errno));
		strcpy(name_buf, "unknown");
	}

	void** actual = array;
	int frame = 0;
	for (n = 0; n < size; ++n)
		if (array[n] == caller_address) {
			frame = n;
			actual = array + frame;
			size -= frame;
			break;
		}

	dprintf(fd, "\n*** Backtrace by glibc:\n");
	backtrace_symbols_fd(actual, size, fd);

	int mem_map_fd = open("/proc/self/smaps", O_RDONLY, 0);
	if (mem_map_fd >= 0) {
		char buf[1024];
		dprintf(fd, "\n*** Memory usage map (by /proc/self/smaps):\n");
		while(1) {
			int n = read(mem_map_fd, &buf, sizeof(buf));
			if (n < 1)
				break;
			if (write(fd, &buf, n) != n)
				break;
		}
		close(mem_map_fd);
	}

	/* avoid ECHILD from waitpid() */
	signal(SIGCHLD, SIG_DFL);

	dprintf(fd, "\n*** Backtrace by addr2line:\n");
	for(n = 0; n < size; ++n) {
		int status = EXIT_FAILURE, child_pid = fork();
		if (! child_pid) {
			char addr_buf[64];
			close(STDIN_FILENO);
			dup2(fd, STDOUT_FILENO);
			close(fd);
			dprintf(STDOUT_FILENO, "(%d) %p: ", n, actual[n]);
			sprintf(addr_buf, "%p", actual[n]);
			execlp("addr2line", "addr2line", addr_buf, "-C", "-f", "-i",
#if __GLIBC_PREREQ(2,14)
				"-p", /* LY: not available on RHEL6, guest by glibc version */
#endif
				"-e", name_buf, NULL);
			exit(EXIT_FAILURE);
		} else if (child_pid < 0) {
			dprintf(fd, "\n*** Unable complete backtrace by addr2line, sorry (%s, %d).\n", "fork", errno);
			break;
		} else if (waitpid(child_pid, &status, 0) < 0 || status != W_EXITCODE(EXIT_SUCCESS, 0)) {
			dprintf(fd, "\n*** Unable complete backtrace by addr2line, sorry (%s, pid %d, errno %d, status 0x%x).\n",
				"waitpid", child_pid, errno, status);
			break;
		}
	}

	if (is_debugger_present()) {
		dprintf(fd, "*** debugger already present\n");
		goto ballout;
	}

	int retry_by_return = 0;
	if (should_die && SI_FROMKERNEL(info)) {
		/* LY: Expect kernel kill us again,
		 * therefore for switch to 'guilty' thread and we may just return,
		 * instead of sending SIGTRAP and later switch stack frame by GDB. */
		retry_by_return = 1;
	}

	if (is_valgrind_present()) {
		dprintf(fd, "*** valgrind present, skip backtrace by gdb\n");
		goto ballout;
	}

	int pipe_fd[2];
	if (pipe(pipe_fd)) {
		pipe_fd[0] = pipe_fd[1] = -1;
		goto ballout;
	}

	gdb_is_ready_for_backtrace = 0;
	pid_t tid = syscall(SYS_gettid);
	gdb_pid = fork();
	if (!gdb_pid) {
		char pid_buf[16];
		sprintf(pid_buf, "%d", getppid());

		dup2(pipe_fd[0], STDIN_FILENO);
		close(pipe_fd[1]);
		pipe_fd[0] = pipe_fd[1] =-1;
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		for(fd = getdtablesize(); fd > STDERR_FILENO; --fd)
			close(fd);
		setsid();
		setpgid(0, 0);

		dprintf(STDOUT_FILENO, "\n*** Backtrace by GDB "
#if GDB_SWITCH2GUILTY_THREAD
			"(pid %s, LWP %i, frame #%d):\n", pid_buf, tid, frame);
#else
			"(pid %s, LWP %i, please find frame manually):\n", pid_buf, tid);
#endif
		execlp("gdb", "gdb", "-q", "-se", name_buf, "-n", NULL);
		kill(getppid(), SIGKILL);
		dprintf(STDOUT_FILENO, "\n*** Sorry, GDB launch failed: %s\n", STRERROR(errno));
		fsync(STDOUT_FILENO);
		exit(EXIT_FAILURE);
	}

	if (gdb_pid > 0) {
		/* enable debugging */
		prctl(PR_SET_PTRACER, gdb_pid /* PR_SET_PTRACER_ANY */, 0, 0, 0);

		close(pipe_fd[0]);
		pipe_fd[0] = -1;

		if (0 >= dprintf(pipe_fd[1],
			"set confirm off\n"
			"set width 132\n"
			"set prompt =============================================================================\\n\n"
			"attach %d\n"
			"handle SIGPIPE pass nostop\n"
			"interrupt\n"
			"set scheduler-locking on\n"
			"thread find %d\n"

			"info sharedlibrary\n"
			"info threads\n"
			"thread apply all backtrace\n"
			"thread apply all disassemble\n"
			"thread apply all info all-registers\n"
			"thread apply all backtrace full\n"
			"shell uname -a\n"
			"shell df -l -h\n"
			"show environment\n"
			"set variable gdb_is_ready_for_backtrace=%d\n"
#if GDB_SWITCH2GUILTY_THREAD
			"continue\n", getpid(), tid, gdb_pid
#else
			"%s\n"
			"quit\n", getpid(), tid, gdb_pid,
			should_die ? "kill" : "detach"
#endif
			))
				goto ballout;

		time_t timeout;
		/* wait until GDB climbs up */
		for (timeout = ldap_time_steady() + 42; ldap_time_steady() < timeout; usleep(10 * 1000)) {
			if (waitpid(gdb_pid, NULL, WNOHANG) != 0) {
#if ! GDB_SWITCH2GUILTY_THREAD
				if (gdb_is_ready_for_backtrace == gdb_pid)
					goto done;
#endif
				break;
			}

			if (gdb_is_ready_for_backtrace != gdb_pid)
				continue;

			if (! is_debugger_present())
				continue;

#if GDB_SWITCH2GUILTY_THREAD
			dprintf(fd, "\n*** GDB 7.8 may hang here - this is just a bug, sorry.\n");

			if (0 >= dprintf(pipe_fd[1],
				"frame %d\n"
				"thread\n"
				"backtrace\n"
				"info all-registers\n"
				"disassemble\n"
				"backtrace full\n"
				"%s\n"
				"quit\n",
				/* LY: first frame if we will return in order to be catched by debugger. */
				retry_by_return ? 1 : frame + 1,
				should_die ? "kill" : "detach"))
					goto ballout;
#endif

			if (retry_by_return) {
				dprintf(fd, "\n*** Expect kernel and GDB catch us again...\n");
				/* LY: The trouble is that GDB should be READY, but no way to check it.
				 * If we return from signal handler while GDB not ready the kernel just terminate us.
				 * Assume that checking gdb_is_ready_for_backtrace == gdb_pid is enough. */
				return;
			}

			pthread_kill(pthread_self(), SIGTRAP);
			goto done;
		}
	}

ballout:
	dprintf(fd, "\n*** Unable complete backtrace by GDB, sorry.\n");

done:
	if (should_die) {
		exit(EXIT_FAILURE);
	}

	if (gdb_pid > 0) {
		dprintf(fd, "\n*** Waitfor GDB done.\n");
		waitpid(gdb_pid, NULL, 0);
	}

	if (pipe_fd[0] >= 0)
		close(pipe_fd[0]);
	if (pipe_fd[1] >= 0)
		close(pipe_fd[1]);
	dprintf(fd, "\n*** No reason for die, continue running.\n");
	close(fd);
}

static int enabled;

int slap_backtrace_get_enable() {
	return enabled > 0;
}

void slap_backtrace_set_dir( const char* path ) {
#ifdef SLAPD_ENABLE_CI
	if (backtrace_homedir) {
		if (!path || strcmp(backtrace_homedir, path))
			Log( LDAP_DEBUG_ANY, LDAP_LEVEL_NOTICE,
				"Ignore changing of backtrace directory due SLAPD_ENABLE_CI.\n");
		return;
	}
#endif /* SLAPD_ENABLE_CI */

	backtrace_cleanup();
	if (path) {
		static char not_a_first_time;
		if (! not_a_first_time) {
			not_a_first_time = 1;
			if (atexit(backtrace_cleanup)) {
				perror("atexit()");
				abort();
			}
		}
		backtrace_homedir = ch_strdup(path);
	}
}

int slap_limit_coredump_set(int mbytes) {
	struct rlimit limit;

	limit.rlim_cur = (mbytes >= 0)
			? ((unsigned long) mbytes) << 20 : RLIM_INFINITY;
	limit.rlim_max = RLIM_INFINITY;
	return setrlimit(RLIMIT_CORE, &limit);
}

int slap_limit_memory_set(int mbytes) {
	struct rlimit limit;

	limit.rlim_cur = (mbytes > 0)
			? ((unsigned long) mbytes) << 20 : RLIM_INFINITY;
	limit.rlim_max = RLIM_INFINITY;
	(void) setrlimit(RLIMIT_DATA, &limit);
	return setrlimit(RLIMIT_AS, &limit);
}

int slap_limit_coredump_get() {
	struct rlimit limit;

	if (getrlimit(RLIMIT_CORE, &limit))
		return -1;

	if (limit.rlim_cur == RLIM_INFINITY)
		return 0;

	return limit.rlim_cur >> 20;
}

int slap_limit_memory_get() {
	struct rlimit limit;

	if (getrlimit(RLIMIT_AS, &limit))
		return -1;

	if (limit.rlim_cur == RLIM_INFINITY)
		return 0;

	return limit.rlim_cur >> 20;
}

void slap_backtrace_set_enable( int value )
{
#if defined(HAVE_LIBBFD) || defined(HAVE_LIBELF)
	if (value && ! is_bfd_symbols_available() && ! is_elf_symbols_available()) {
		if (slap_backtrace_get_enable() != (value != 0))
			Log( LDAP_DEBUG_ANY, LDAP_LEVEL_NOTICE, "Backtrace could be UNUSEFUL, because symbols not available.\n");
	}
#endif

	if (slap_backtrace_get_enable() != (value != 0)) {
		struct sigaction sa;

		if (value) {
			/* LY: first time call libc for initialization,
			 * this avoid stall/deadlock later where in pthread_once()
			 * was called from signal handler. */
			if (enabled == 0) {
				void *array[42];
				size_t size = backtrace(array, 42);
				if (size) {
					char **messages = backtrace_symbols(array, size);
					free(messages);
				}
			}

			/* enable debugger attaching */
			prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);

			sa.sa_sigaction = backtrace_sigaction;
			sa.sa_flags = SA_SIGINFO;
			sigfillset(&sa.sa_mask);
			sigdelset(&sa.sa_mask, SIGCONT);
			sigdelset(&sa.sa_mask, SIGTRAP);
			sigdelset(&sa.sa_mask, SIGTERM);
		} else {
#ifdef SLAPD_ENABLE_CI
			Log( LDAP_DEBUG_ANY, LDAP_LEVEL_NOTICE, "Ignore disabling of backtrace due SLAPD_ENABLE_CI.\n");
#endif /* SLAPD_ENABLE_CI */
			sa.sa_handler = SIG_DFL;
			sa.sa_flags = 0;
			sigemptyset(&sa.sa_mask);
		}

		sigaction(SIGSEGV, &sa, NULL);
		sigaction(SIGBUS, &sa, NULL);
		sigaction(SIGILL, &sa, NULL);
		sigaction(SIGFPE, &sa, NULL);
		sigaction(SIGABRT, &sa, NULL);

		sigaction(SIGXCPU, &sa, NULL);
		sigaction(SIGXFSZ, &sa, NULL);

		sa.sa_flags |= SA_RESTART;
		sigaction(SIGTRAP, &sa, NULL);
		/* allow use SIGALRM to make a backtrace */
		sigaction(SIGALRM, &sa, NULL);

		enabled = value ? 1 : -1;
	}
}

void __noinline
slap_backtrace_log(void *array[], int nentries, const char* caption)
{
	int i;
	char **bt_glibc;
	char name_buf[PATH_MAX];

	if (nentries < 1)
		return;

	ldap_debug_lock();

	int n = readlink("/proc/self/exe", name_buf, sizeof(name_buf) - 1);
	if (n < 0) {
		ldap_debug_print("*** Unable read executable name: %s\n", STRERROR(errno));
		goto fallback;
	}
	name_buf[n] = 0;

	int to_addr2line[2], from_addr2line[2];
	if (pipe(to_addr2line)|| pipe(from_addr2line)) {
		ldap_debug_print("*** Unable complete backtrace by addr2line, sorry (%s, %d).\n", "pipe", errno);
		goto fallback;
	}

	int child_pid = fork();
	if (child_pid == 0) {
		dup2(to_addr2line[0], STDIN_FILENO);
		close(to_addr2line[0]);
		close(to_addr2line[1]);

		dup2(from_addr2line[1], STDOUT_FILENO);
		close(from_addr2line[0]);
		close(from_addr2line[1]);

		execlp("addr2line", "addr2line", "-C", "-f", "-i",
#if __GLIBC_PREREQ(2,14)
			"-p", /* LY: not available on RHEL6, guest by glibc version */
#endif
			"-e", name_buf, NULL);
		exit(errno);
	}

	close(to_addr2line[0]);
	close(from_addr2line[1]);

	if (child_pid < 0) {
		ldap_debug_print("*** Unable complete backtrace by addr2line, sorry (%s, %d).\n", "fork", errno);
		close(to_addr2line[1]);
		close(from_addr2line[0]);
		goto fallback;
	}

	FILE* file = fdopen(from_addr2line[0], "r");
	ldap_debug_print("*** %s by addr2line:\n", caption);
	for(i = 0; i < nentries; ++i) {
		char addr_buf[1024];

		dprintf(to_addr2line[1], "%p\n", array[i]);
		if (! fgets(addr_buf, sizeof(addr_buf), file))
			break;
		ldap_debug_print("(%d) %s", i, addr_buf);
	}

	close(to_addr2line[1]);
	close(from_addr2line[0]);

	int status = 0;
	if (waitpid(child_pid, &status, 0) < 0 || status != W_EXITCODE(EXIT_SUCCESS, 0)) {
		ldap_debug_print("*** Unable complete backtrace by addr2line, sorry (%s, pid %d, errno %d, status 0x%x).\n",
			"waitpid", child_pid, errno, status);
		goto fallback;
	}

	ldap_debug_unlock();
	return;

fallback:
	bt_glibc = backtrace_symbols(array, nentries);
	if (bt_glibc) {
		ldap_debug_print("*** %s by glibc:\n", caption);
		for(i = 0; i < nentries; i++)
			ldap_debug_print("(%d) %s\n", i, bt_glibc[i]);
		free(bt_glibc);
	}
	ldap_debug_unlock();
}

#else /* HAVE_ENOUGH4BACKTRACE */

#ifndef ENOSYS
#define ENOSYS 38
#endif

void slap_backtrace_set_enable(int value) {(void)value;}
int slap_backtrace_get_enable() {return 0;}
void slap_backtrace_set_dir(const char* path ) {(void)path;}
int slap_limit_coredump_set(int mbytes) { if (mbytes == 0) return 0; errno = ENOSYS; return -1; }
int slap_limit_memory_set(int mbytes) { if (mbytes == 0) return 0; errno = ENOSYS; return -1; }
int slap_limit_coredump_get() {return 0;}
int slap_limit_memory_get() {return 0;}
void slap_backtrace_log(void *array[], int nentries, const char* caption) {
	(void)array;
	(void)nentries;
	(void)caption;
}

#endif /* HAVE_ENOUGH4BACKTRACE */

void
slap_backtrace_debug(void) {
	slap_backtrace_debug_ex(2, 42, "Backtrace");
}

void
slap_backtrace_debug_ex(int skip, int deep, const char *caption) {
	void **array = alloca(sizeof(void*) * (deep + skip));
	slap_backtrace_log(array + skip, backtrace(array, deep + skip) - skip, caption);
}

#ifdef SLAPD_ENABLE_CI
/* simplify testing for Continuous Integration */
void slap_setup_ci(void) {
	slap_limit_coredump_set( -1 /* unlimit */ );
	slap_backtrace_set_enable(1);

#ifdef __linux__
	/* enable debugger attaching */
	prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);
#endif /* __linux__ */

	/* set directory for backtraces */
	const char* dir = getenv("SLAPD_TESTING_DIR");
#ifdef SLAPD_TESTING_DIR
	if (!dir)
		dir = SLAPD_TESTING_DIR;
#endif
	if (dir) {
		slap_backtrace_set_dir(dir);
		Log( LDAP_DEBUG_ANY, LDAP_LEVEL_NOTICE, "Set backtrace directory to %s (due SLAPD_ENABLE_CI).\n", dir);
	}

	/* set alarm to dump backtraces */
	long seconds = 0;
#ifdef SLAPD_TESTING_TIMEOUT
	seconds = SLAPD_TESTING_TIMEOUT;
#endif
	const char* timeout = getenv("SLAPD_TESTING_TIMEOUT");
	if (timeout)
		seconds = atol(timeout);
	if (seconds > 0) {
		Log( LDAP_DEBUG_ANY, LDAP_LEVEL_NOTICE, "Set testing timeout to %ld seconds (due SLAPD_ENABLE_CI).\n", seconds);
		struct itimerval itimer;
		itimer.it_interval.tv_sec = 42;
		itimer.it_interval.tv_usec = 0;
		itimer.it_value.tv_sec = seconds;
		itimer.it_value.tv_usec = 0;
		if (setitimer(ITIMER_REAL, &itimer, NULL)) {
			perror("setitimer()");
			abort();
		}
	}
}
#endif /* SLAPD_ENABLE_CI */
