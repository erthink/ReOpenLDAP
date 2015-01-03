/* backtrace.c - stack backtrace routine */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#ifndef __linux__
void slap_backtrace_set_enable( int value ) {}
int slap_backtrace_get_enable() {return 0;}
void slap_backtrace_set_dir(const char* path ) {}
int slap_limit_coredump_set(int mbytes) {return mbytes > 0;}
int slap_limit_memory_set(int mbytes) {return mbytes > 0;}
int slap_limit_coredump_get() {return 0;}
int slap_limit_memory_get() {return 0;}
#else /* __linux__ */

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

/* Include for BFD processing */
#include "bfd.h"

/* Include for ELF processing */
#include <libelf.h>
#include <gelf.h>

#ifndef PR_SET_PTRACER
#	define PR_SET_PTRACER 0x59616d61
#	define PR_SET_PTRACER_ANY ((unsigned long)-1)
#endif

static char* homedir;

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

static volatile sig_atomic_t is_debugger_active;
static void trap_sigaction(int signum, siginfo_t *info, void* ptr) {
	is_debugger_active = 0;
}

#if HAVE_VALGRIND
#	include <valgrind/valgrind.h>
#endif

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

static void backtrace_sigaction(int signum, siginfo_t *info, void* ptr) {
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
	time_t t = time(NULL);
	strftime(time_buf, sizeof(time_buf), "%F-%H%M%S", localtime(&t));

	char name_buf[PATH_MAX];
	int fd = -1;
#ifdef snprintf
#	undef snprintf
#endif
	if (snprintf(name_buf, sizeof(name_buf), "%s/lmdb-backtrace.%s-%i.log%c",
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
				   /* "-p", LY: not available on RHEL6 */
				   "-e", name_buf, NULL);
			exit(EXIT_FAILURE);
		} else if (child_pid < 0 || waitpid(child_pid, &status, 0) < 0 || status != W_EXITCODE(EXIT_SUCCESS, 0)) {
			dprintf(fd, "\n*** Unable complete backtrace by addr2line, sorry (%d, %d, 0x%x).\n", child_pid, errno, status);
			break;
		}
	}

	sigset_t mask, prev_mask;
	if (pthread_sigmask(SIG_SETMASK, NULL, &prev_mask))
		goto ballout;

	struct sigaction sa, prev_sa;
	sa.sa_sigaction = trap_sigaction;
	sa.sa_flags = SA_RESTART | SA_SIGINFO;
	sa.sa_mask = prev_mask;
	sigdelset(&sa.sa_mask, SIGTRAP);
	if(sigaction(SIGTRAP, &sa, &prev_sa))
		goto ballout;

	sigemptyset(&mask);
	sigaddset(&mask, SIGTRAP);
	if(pthread_sigmask(SIG_UNBLOCK, &mask, NULL))
		goto ballout;

	if (is_debugger_present()) {
		dprintf(fd, "*** debugger already present\n");
		goto ballout;
	}

	is_debugger_active = 1;
	if (pthread_kill(pthread_self(), SIGTRAP) < 0)
		goto ballout;
	if (is_debugger_active) {
		dprintf(fd, "*** debugger already running\n");
		goto ballout;
	}

	if (is_valgrind_present()) {
		dprintf(fd, "*** valgrind present, skip backtrace by gdb\n");
		goto ballout;
	}

	int	pipe_fd[2];
	if (pipe(pipe_fd)) {
		pipe_fd[0] = pipe_fd[1] = -1;
		goto ballout;
	}

	int retry_by_return = 0;
	if (should_die && SI_FROMKERNEL(info)) {
		/* LY: Expect kernel kill us again,
		 * therefore for switch to 'guilty' thread and we we may just return,
		 * instead of sending SIGTRAP and later switch stack frame by GDB. */
		retry_by_return = 1;
		/* LY: The trouble is that GDB should be ready, but no way to check it.
		 * If we just return from handle while GDB not ready the kernel just terminate us.
		 * So, we can just some delay before return or don't try this way. */
		retry_by_return =  0;
	}

	pid_t tid = syscall(SYS_gettid);
	gdb_pid = fork();
	if (!gdb_pid) {
		char pid_buf[16];
		char tid_buf[32];
		char frame_buf[16];

		dup2(pipe_fd[0], STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		close(pipe_fd[0]);
		close(pipe_fd[1]);
		close(fd);
		setpgid(0, 0);
		setsid();

		sprintf(pid_buf, "%d", getppid());
		sprintf(frame_buf, "frame %d", frame);
		sprintf(tid_buf, "thread find %d", tid);

		dprintf(STDOUT_FILENO, "\n*** Backtrace by GDB (pid %s, tid %i frame #%d):\n", pid_buf, tid, frame);
		execlp("gdb", "gdb", "-q", "-p", pid_buf, "-se", name_buf, "-n",
			"-ex", "set confirm off",
			"-ex", "handle SIG33 pass nostop noprint",
			"-ex", "handle SIGPIPE pass nostop noprint",
			"-ex", "handle SIGTRAP nopass stop print",
			"-ex", "continue",
			NULL);
		dprintf(STDOUT_FILENO, "\n*** Sorry, GDB launch failed: %s\n", STRERROR(errno));
		fsync(STDOUT_FILENO);
		kill(getppid(), SIGKILL);
	} else if (gdb_pid > 0) {
		/* enable debugging */
		prctl(PR_SET_PTRACER, gdb_pid /* PR_SET_PTRACER_ANY */, 0, 0, 0);

		close(pipe_fd[0]);
		pipe_fd[0] = -1;
		if (0 >= dprintf(pipe_fd[1],
			"info threads\n"
			"thread find %d\n"
			"frame %d\n"
			"thread\n"
			"backtrace\n"
			"info all-registers\n"
			"disassemble\n"
			"backtrace full\n"

			"info sharedlibrary\n"
			"info threads\n"
			"thread apply all bt\n"
			"thread apply all backtrace full\n"
			"thread apply all disassemble\n"
			"thread apply all info all-registers\n"
			"shell uname -a\n"
			"show environment\n"
			"%s\n"
			"quit\n",
			tid,
			/* LY: first frame if we will just return in order to be catched by debugger. */
			retry_by_return ? 1 : frame + 1,
			should_die ? "kill" : "detach"))
				goto ballout;

		time_t timeout;
		is_debugger_active = -1;
		for (timeout = time(NULL) + 11; waitpid(gdb_pid, NULL, WNOHANG) == 0
			&& time(NULL) < timeout; usleep(10 * 1000)) {
			/* wait until GDB climbs up */
			if (! is_debugger_present())
				continue;

			if (is_debugger_active < 0)
				dprintf(fd, "\n*** GDB 7.8 may hang here - this is just a bug, sorry.\n");

			if (retry_by_return) {
				dprintf(fd, "\n*** Expect kernel catch us again...\n");
				usleep(10 * 1000);
				return;
			}

			is_debugger_active = 1;
			/* LY: gdb needs  a kick to switch on this thread. */
			if (pthread_kill(pthread_self(), SIGTRAP) < 0)
				break;
			sched_yield();
			if (is_debugger_active)
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

	sigaction(SIGTRAP, &prev_sa, NULL);
	pthread_sigmask(SIG_SETMASK, &prev_mask, NULL);
}

static int enabled;

int slap_backtrace_get_enable() {
	return enabled > 0;
}

void slap_backtrace_set_dir( const char* path ) {
	free(homedir);
	homedir = path ? strdup(path) : NULL;
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
	if (value && ! is_bfd_symbols_available() && ! is_elf_symbols_available()) {
		if (enabled != (value != 0))
			Log0( LDAP_DEBUG_ANY, LDAP_LEVEL_NOTICE, "backtrace could not be enabled, because symbols not available.\n");
		value = 0;
	}

	if (enabled != (value != 0)) {
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

#endif /* __linux__ */
