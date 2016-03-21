/*
    Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
    Copyright (c) 2015,2016 Peter-Service R&D LLC.

    This file is part of ReOpenLDAP.

    ReOpenLDAP is free software; you can redistribute it and/or modify it under
    the terms of the GNU Affero General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ReOpenLDAP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* backtrace.c - stack backtrace routine */

#ifndef __linux__
void slap_backtrace_set_enable( int value ) {}
int slap_backtrace_get_enable() {return 0;}
void slap_backtrace_set_dir(const char* path ) {}
int slap_limit_coredump_set(int mbytes) {return mbytes > 0;}
int slap_limit_memory_set(int mbytes) {return mbytes > 0;}
int slap_limit_coredump_get() {return 0;}
int slap_limit_memory_get() {return 0;}
#else /* __linux__ */

/* TODO: add libelf detection to configure. */
#define HAVE_LIBELF 1

#ifndef _GNU_SOURCE
#	define _GNU_SOURCE
#endif

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
#include <sys/time.h>
#include <sys/resource.h>

#include "portable.h"
#include "slap.h"
#include "config.h"
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

static char* homedir;
static void backtrace_cleanup(void)
{
	if (homedir) {
		ch_free(homedir);
		homedir = NULL;
	}
}

#ifdef HAVE_LIBBFD
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
#if defined(__i386__)
	caller_address = (void *) uc->uc_mcontext.gregs[REG_EIP]; // EIP: x86 specific
#elif defined(__x86_64__)
	caller_address = (void *) uc->uc_mcontext.gregs[REG_RIP]; // RIP: x86_64 specific
#else
#	error Unsupported architecture.
#endif

	int should_die = 0;
	switch(info->si_signo) {
	case SIGABRT:
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
	time_t t = ldap_time_steady();
	strftime(time_buf, sizeof(time_buf), "%F-%H%M%S", localtime(&t));

	char name_buf[PATH_MAX];
	int fd = -1;
#ifdef snprintf
#	undef snprintf
#endif
	if (snprintf(name_buf, sizeof(name_buf), "%s/slapd-backtrace.%s-%i.log%c",
				 homedir ? homedir : ".", time_buf, getpid(), 0) > 0)
		fd = open(name_buf, O_CREAT | O_EXCL | O_WRONLY, 0644);

	if (fd < 0) {
		if (homedir)
			fd = open(strrchr(name_buf, '/') + 1, O_CREAT | O_EXCL | O_WRONLY, 0644);
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
		} else if (child_pid < 0 || waitpid(child_pid, &status, 0) < 0 || status != W_EXITCODE(EXIT_SUCCESS, 0)) {
			dprintf(fd, "\n*** Unable complete backtrace by addr2line, sorry (%d, %d, 0x%x).\n", child_pid, errno, status);
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
	if (should_die)
		exit(EXIT_FAILURE);

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
		homedir = ch_strdup(path);
	}
}

int slap_limit_coredump_set(int mbytes) {
	struct rlimit limit;

	limit.rlim_cur = (mbytes > 0)
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
			Log0( LDAP_DEBUG_ANY, LDAP_LEVEL_NOTICE, "Backtrace could be UNUSEFUL, because symbols not available.\n");
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

			sa.sa_sigaction = backtrace_sigaction;
			sa.sa_flags = SA_SIGINFO;
			sigfillset(&sa.sa_mask);
			sigdelset(&sa.sa_mask, SIGCONT);
			sigdelset(&sa.sa_mask, SIGTRAP);
			sigdelset(&sa.sa_mask, SIGTERM);

		} else {
			sa.sa_handler = SIG_DFL;
			sa.sa_flags = 0;
			sigemptyset(&sa.sa_mask);
		}

		sigaction(SIGSEGV, &sa, NULL);
		sigaction(SIGBUS, &sa, NULL);
		sigaction(SIGILL, &sa, NULL);
		sigaction(SIGFPE, &sa, NULL);
		sigaction(SIGABRT, &sa, NULL);

		sa.sa_flags |= SA_RESTART;
		sigaction(SIGTRAP, &sa, NULL);

		enabled = value ? 1 : -1;
	}
}

void slap_backtrace_debug() {
	void *array[42];
	size_t size, i;
	char name_buf[PATH_MAX];
	char **bt_glibc;

	/* get all entries on the stack */
	size = backtrace(array, 42);

	bt_glibc = backtrace_symbols(array, size);
	if (bt_glibc) {
		lutil_debug_print("*** Backtrace by glibc:\n");
		for(i = 0; i < size; i++)
			lutil_debug_print("(%zd) %s\n", i, bt_glibc[i]);
		free(bt_glibc);
	}

	int n = readlink("/proc/self/exe", name_buf, sizeof(name_buf) - 1);
	if (n < 0) {
		lutil_debug_print("*** Unable read executable name: %s\n", STRERROR(errno));
		return;
	}
	name_buf[n] = 0;

	int to_addr2line[2], from_addr2line[2];
	if (pipe(to_addr2line)|| pipe(from_addr2line)) {
		lutil_debug_print("*** Unable complete backtrace by addr2line, sorry (%s, %d).\n", "pipe", errno);
		return;
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
		lutil_debug_print("*** Unable complete backtrace by addr2line, sorry (%s, %d).\n", "fork", errno);
		close(to_addr2line[1]);
		close(from_addr2line[0]);
		return;
	}

	FILE* file = fdopen(from_addr2line[0], "r");
	lutil_debug_print("*** Backtrace by addr2line:\n");
	for(i = 0; i < size; ++i) {
		char addr_buf[1024];

		dprintf(to_addr2line[1], "%p\n", array[i]);
		if (! fgets(addr_buf, sizeof(addr_buf), file))
			break;
		lutil_debug_print("(%zd) %s", i, addr_buf);
	}

	close(to_addr2line[1]);
	close(from_addr2line[0]);

	int status = 0;
	if (waitpid(child_pid, &status, 0) < 0 || status != W_EXITCODE(EXIT_SUCCESS, 0))
		lutil_debug_print("*** Unable complete backtrace by addr2line, sorry (%s, %d).\n", "wait", status ? status : errno);
}

#endif /* __linux__ */
