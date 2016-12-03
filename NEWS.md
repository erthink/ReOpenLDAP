1.2.0-Devel (not yet released)
==============================

Briefly:
 1. Packages for Fedora/RedHat/CentOS and Debian/Ubuntu.
 2. TBD.
 3. TBD.


v1.1.4 2016-11-30
=================

Briefly:
 1. Return to the original OpenLDAP Foundation license.
 2. More fixed for OpenSSL 1.1, LibreSSL 2.5 and Mozilla NSS.
 3. Minor fixes for configure/build and so forth.

New features and Compatibility breaking:
 * reopenldap: support for OpenSSL-1.1.x and LibreSSL-2.5.x (#115, #116).
 * contrib: added mr_passthru module.
 * configure --with-buildid=SUFFIX.
 * return to the original OpenLDAP Foundation license.
 * moznss: support for <nspr4/nspr.h> and <nss3/nss.h>

Documentation:
 * man: fix typo (ITS#8185).

Major and Security bugs: none

Minor bugs:
 * mdbx: avoid large '.data' section in mdbx_chk.
 * mdbx: fix cursor tracking after mdb_cursor_del (ITS#8406).
 * reopenldap: fix LDAPI_SOCK, adds LDAP_VARDIR.
 * mdbx: use O_CLOEXEC/FD_CLOEXEC for me_fd,env_copy as well (ITS#8505).
 * mdbx: reset cursor EOF flag in cursor_set (ITS#8489).
 * slapd: return error on invalid syntax filter-present (#108).

Performance: none

Build:
 * ppolicy: fix libltdl's includes for ppolicy overlay.
 * libltdl: move `build/libltdl` to the start of SUBDURS.
 * mdbx: don't enable tracing for MDBX by --enable-debug.
 * reopenldap: fix missing space in bootstrap.sh

Cosmetics:
 * slapd: adds RELEASE_DATE/STAMP to `slapd -V` output.
 * mdbx: clarify fork's caveat (ITS#8505).

Other:
 * cleanup/refine AUTHORS file.


v1.1.3, 2016-08-30
==================

Briefly:
 1. Imported all relevant patches from RedHat, ALT Linux and Debian/Ubuntu.
 2. More fixes especially for TLS and Mozilla NSS.
 3. Checked with PVS-Studio static analyser (first 10 defects were shown and fixed).
    Checking with Coverity static analyser also was started, but unfortunately it is
    a lot of false-positives (pending fixing).

New features and Compatibility breaking:
 * configure --with-gssapi=auto/yes/no.
 * slapi: use `/var/log/slapi-errors` instead of `/var/errors`.
 * slapd: move the ldapi socket to `/var/run/slapd` from `/var/run`.
 * reopenldap LICENSE note.
 * configure --enable-debug=extra.
 * libreldap: NTLM bind support.
 * contrib: added check_password module.
 * contrib: allow build smbk5pwd without heimdal-kerberos.
 * libreldap: Disables opening of `ldaprc` file in current directory (RHEL#38402).
 * libreldap: Support TLSv1.3 and later.

Documentation:
 * man: added page for contrib/smbk5pwd.
 * man: note for ldap.conf that on Debian is linked against GnuTLS.
 * doc: added preamble to devel/README.
 * man: remove refer to <ldap_log.h>
 * man: note olcAuthzRegex needs restart (ITS6035).
 * doc: fixed readme's module-names for contrib (.so -> .la)
 * mdbx: comment MDB_page, rename mp_ksize.
 * mdbx: VALID_FLAGS, mm_last_pg, mt_loose_count.
 * man: fixed SASL_NOCANON option missing in ldap.conf manual page.

Major and Security bugs:
 * slapd: fixed #104, check for writers while close the connection.
 * slapd: fixed #103, stop glue-search on errors.
 * libreldap: MozNSS fixed CVE-2015-3276 (RHEL#1238322).
 * libreldap: TLS do not reuse tls_session if hostname check fails (RHEL#852476).
 * slapd: Switch to lt_dlopenadvise() to get RTLD_GLOBAL set (RHEL#960048, Debian#327585).
 * libreldap: reentrant gethostby() (RHEL#179730).
 * libreldap: MozNSS ignore certdb database type prefix when checking existence of the directory (RHEL#857373).

Minor bugs:
 * slapd: fixed compare pointer with '\0' in syn_add().
 * slapd: fixed indereferenced pointer in fe_acl_group().
 * libreldap: fixed overwriting a parameter in tlso_session_errmsg().
 * slapd: fixed recurring check in register_matching_rule().
 * syncprov/syncrepl: more for #105, two workarounds.
 * mdbx: fixed mdb_dump tool and other issues detected by PVS-Studio.
 * mdbx: fixed assertions when debug enabled for various open/sync modes.
 * slapd: fixed use-after-free in debug/syslog message on module unloaded.
 * monitor-backend: fixed cache-release on errors.
 * slapd: don't create pid-file for config-check mode.
 * libreldap: "tls_reqcert never" by default for ldap.conf
 * libreldap: Disables opening of ldaprc file in current directory (RHEL#38402).
 * libreldap: MozNSS update list of supported cipher suites.
 * libreldap: MozNSS better file name matching for hashed CA certificate directory (RHEL#852786).
 * libreldap: MozNSS free PK11 slot (RHEL#929357).
 * libreldap: MozNSS load certificates from certdb, fallback to PEM (RHEL#857455).
 * slapd: fixed loglevel2bvarray() for config-backend.
 * libreldap: LDAPI SASL fix (RHEL#960222).
 * libreldap: use AI_ADDRCONFIG if defined in the environment (RHEL#835013).
 * libreldap: fixed false-positive ASAN-trap when Valgrind also enabled.

Performance:
 * libreldap: remove resolv-mutex around getnameinfo() and getnameinfo() (Debian#340601).
 * slapd: fixed major typo in rurw_r_unlock() which could cause performance degradation.

Build:
 * configure: added `--with-gssapi=auto/yes/no`.
 * mdbx: fixed CC and XCFLAGS in 'ci' make-target rules.
 * mdbx: fixed 'clean' make-target typo.
 * mdbx: fixed Makefile deps from mdbx.c
 * tests: fixed lt-exe-name for coredump collection.
 * backend-mdb: enable debug for libmdbx if --enable-debug.
 * mdbx: make ci-target without NDEBUG and with MDB_DEBUG=2.
 * mdbx: allow CC=xyz for ci-target rules.
 * configure: fixed cases when corresponding to --with-tls=xyz package not available.
 * configure: take in account --enable-lmpasswd for TLS choice.
 * configure: workaround for --enable-lmpasswd with GnuTLS (ITS#6232).
 * liblutils: fixed build with --enable-lmpasswd.
 * libreldap: fixed warnings when Mozilla NSS used.
 * configure: rework TLS detection (Mozilla NSS, GnuTLS, OpenSSL).
 * libreldap: fixed build --with-tls=gnutls.
 * contrib: don't build passwd/totp, passwd/pbkdf2 and smbk5pwd with --with-tls=moznss.
 * automake: install lber_types.h and ldap_features.h
 * automake: fixed $(DESTDIR) for install/uninstall hooks.
 * automake: fixed ldapadd tool uninstall.
 * configure: Check whether ucred is defined without _GNU_SOURCE.
 * slapd: don't link with BerkeleyDB, but bdb/hdb backends only.
 * configure: checking for krb5-gssapi for contrib-gssacl.
 * configure: Use pkg-config for Mozilla NSS library detection.
 * libreldap: fixed build in case --with-tls=moznss.

Cosmetics:
 * slapindex: print a warning if it's run as root.
 * fixed printf format in mdb-backend and liblunicode.
 * fixed minor typo in print_vlv() for ldif-output.
 * mdbx: minor fix mdb_page_list() message
 * fixed 'experimantal' typo ;)
 * slap-tools: fixed set debug-level.

Other:
 * reopenldap AUTHORS and CONTRIBUTION.
 * reopenldap: fix copyright timestamps.
 * libreldap: fixed deprecated ldap_search_s() in case --with-gssapi=yes.
 * libreldap, slapd: don't second-guess SASL ABI (Debian#546885).
 * slapd: added LDAP_SYSCONFDIR/sasl2 to the SASL configuration search path.
 * backend-bdb: don't second-guess BDB ABI (Debian#651333).
 * libreldap: added /etc/ssl/certs/ca-certificates.crt for ldap.conf
 * reopenldap: added Coverity scan build status.
 * mdbx: fix usage of __attribute__((format(gnu_printf, ...)) for clang.
 * backend-mdb: turn MDBX's debugging depending on --enable-debug=xyz.
 * reopenldap: use LDAP_DEBUG instead of !NDEBUG.
 * reopenldap: remove obsolete OLD_DEBUG.
 * tests: more for #92 (mtread).
 * tests: added biglock to test048-syncrepl-multiproxy.
 * slapd: refine biglock for passwd_extop().
 * tests: fixed #105, adds biglock to test054-syncrepl-parallel-load.
 * libreldap: more worarounds for #104.
 * slapd: show 'glue' like a static overlay.
 * mdbx: fixed copyright timestamps.
 * mdbx: check assertions depending on NDEBUG.
 * contrib/check_password: fixed default values usage.
 * tests: support RANDOM_ORDER for load balancing.
 * libreldap: TLS fixed unused warnings.
 * slapd: backtrace for CLM-166490.
 * tests: use Valgrind from configure.


v1.1.2, 2016-07-30
==================

Briefly:
 1. Fixed few build bugs which were introduced by previous changes.
 2. Fixed the one replication related bug which was introduced in ReOpenLDAP-1.0
    So there is no even a rare related to replication test failures.
 3. Added a set of configure options.

New:
 * `configure --enable-contrib` for build contributes modules and plugins.
 * `configure --enable-experimental` for experimental and developing features.
 * 'configure --enable-valgrind' for testing with Valgrind Memory Debugger.
 * `configure --enable-check --enable-hipagut` for builtin runtime checking.
 * Now '--enable-debug' and '--enable-syslog' are completely independent of each other.

Documentation:
 * man: minor cleanup 'deprecated' libreldap functions.

Major bugs:
 * syncprov: fixed find-csn error handling.

Minor bugs:
 * slapd: accept module/plugin name with hyphen.
 * syncprov: fixed RS_ASSERT failure inside mdb-search.
 * slapd: result-asserts (RS_ASSERT) now controlled by mode 'check/idkfa'.
 * pcache: fixed RS_ASSERT failure.
 * mdbx: backport - ITS#8209 fixed MDB_CP_COMPACT.

Performance: none

Build:
 * slapd: fixed old gcc's double typedef error.
 * slapd: fixed bdb/hdb backends build distinction.
 * contrib: fixed out-of-source build.
 * configure: build contrib-modiles conditionaly if 'heimdal' package not available.
 * slapd: fixed warning with --enable-experimental.
 * pcache: fixed build with --enable-experimental.
 * slapd: fixed dynamic module support.
 * configure: refine libtool patch for LTO.
 * build: fixup banner-versioning for tools and libs.
 * slapd: fixed build with --enable-wrappers.
 * all: fixup 'unused' vars, in case assert-checking disabled.
 * build: silencing make by default.
 * build: mbdx-tools within mdb-backend.

Cosmetics: none

Other:
 * libreldap, slapd: add and use ldap_debug_perror().
 * slapd: support ARM and MIPS for backtrace.
 * mdbx: backport - Refactor mdb_page_get().
 * mdbx: backport - Fix MDB_INTEGERKEY doc of integer types.
 * all: rework debug & logging.
 * slapd: LDAP_EXPERIMENTAL instead of LDAP_DEVEL.
 * slapd, libreldap: drop LDAP_TEST, introduce LDAP_CHECK.
 * slapd, libreldap: always checking if LDAP_CHECK > 2.
 * reopenldap: little bit cleanup of EBCDIC.


v1.1.1, 2016-07-12
==================

Briefly:
 1. Few replication (syncprov) bugs are fixed.
 2. Additions to russian man-pages were translated to english.
 3. A lot of segfault and minor bugs were fixed.
 4. Done a lot of work on the transition to actual versions of autoconf and automake.

New:
 * reopenldap: use automake-1.15 and autoconf-2.69.
 * slapd: upgradable recursive read/write lock.
 * slapd: rurw-locking for config-backend.

Documentation:
 * doc: english man-page for 'syncprov-showstatus none/running/all'.
 * doc: syncrepl's 'requirecheckpresent' option.
 * man: note about 'ServerID 0' in multi-master mode.
 * man: man-pages for global 'keepalive idle:probes:interval' option.

Major bugs:
 * slapd: rurw-locking for config-backend.
 * syncprov: fixed syncprov_findbase() race with backover's hacks.
 * syncprov: bypass 'dead' items in syncprov_playback_locked().
 * syncprov: fixed syncprov_playback_locked() segfault.
 * syncprov: fixed syncprov_matchops() race with backover's hacks.
 * syncprov: fixed rare syncprov_unlink_syncop() deadlock with abandon.
 * slapd: fixed deadlock in connections_shutdown().
 * overlays: fixed a lot of segfaults (callback initialization).

Minor bugs:
 * install: hotfix slaptools install, sbin instead of libexec.
 * contrib-modules: hotfix - remove obsolete ad-hoc of copy register_at().
 * syncrepl: backport - ITS#8432 fix infinite looping mods in delta-mmr.
 * reopenldap: hotfix 'derived from' copy-paste error.
 * mdbx: backport - mdb_env_setup_locks() Plug mutexattr leak on error.
 * mdbx: backport - ITS#8339 Solaris 10/11 robust mutex fixes.
 * libreldap: fixed PR_GetUniqueIdentity() for ReOpenLDAP.
 * liblber: don't trap ber_memcpy_safe() when dst == src.
 * syncprov: kicks the connection from syncprov_unlink_syncop().
 * slapd: reschedule from connection_closing().
 * slapd: connections_socket_troube() and EPOLLERR|EPOLLHUP.
 * slapd: 2-stage for connection_abandon().
 * syncprov: rework cancellation path in syncprov_matchops().
 * syncprov: fixed invalid status ContextCSN.
 * slapd: fixed handling idle/write timeouts.
 * accesslog: backport - ITS#8423 check for pause in accesslog_purge.
 * mdbx: backport - ITS#8424 init cursor in mdb_env_cwalk.

Performance: none

Build:
 * contrib-modules: fixed build, contrib-mod.mk
 * configure: fixed 'pointers aliasing' for libltdl.
 * configure: check for libbfd and libelf for backtrace.
 * configure: check for 'soelim' and 'soelim -r'.
 * configure: build librewrite only if rwm-overlay or meta-backed is enabled.
 * configure: PERL_LDFLAGS and PERL_RDIR (rpath) for perl-backend.
 * configure: NDB_LDFLAGS and NDB_RDIR (rpath) for ndb-backend.
 * reopenldap: fixed build parts by C++ (back-ndb).
 * mdbx: fixed build by clang (missing-field-initializers).
 * slapd: fixed build ASAN + dynamic + visibility=hidden.
 * libreldap: fixed 'msgid' may be used uninitialized in ldap_modify_*().
 * configure: error if libuuid is missing.
 * libreldap: fixed build by clang.
 * shell-backends: fixed passwd-shell tool building.
 * contrib/acl: checking for --enable-dynacl.
 * slapd: fixed keepalive-related typo in slap_listener().
 * libldap: fixed typo ';' in ldap_pvt_tcpkeepalive().
 * libldap: fixed build with GnuTLS (error at @wanna_steady_or_not).

Cosmetics:
 * syncrepl: cleanup rebus-like error codes.
 * slapd: rename reopenldap's modes.
 * slapd: debug-locking for backtrace.
 * slapd, libreldap: closing conn/fd debug.

Other:
 * slapd: rework dynamic modules.
 * libreldap: rework 'deprecated' interfaces.
 * libreldap: rename to lber_strerror().
 * libreldap: refine memory.c, drop littery LDAP_MEMORY_ASSERT.
 * reopenldap: remove obsolete EBCDIC support.
 * reopenldap: autotools bootstrap.
 * reopenldap: ban the compilers older than GCC 4.2 or incompatible with it.
 * reopenldap: clarify LDAP_API_FEATURE_X_OPENLDAP_THREAD_SAFE.
 * slapd: cleanup Windows support.
 * reopenldap: rename libslapi -> libreslapi.
 * reopenldap: rename liblmdb -> libmdbx.
 * reopenldap: remove obsolete & unsupported parts.
 * reopenldap: liblber+libldap -> libreldap (big-bang).
 * mdbx: cleanup tools from Windows.
 * syncrepl: more LDAP_PROTOCOL_ERROR.
 * slapd: remove unusable zn-malloc.
 * slapd: refine connection_client_stop() for robustness.
 * slapd: adds slap_backtrace_debug_ex(), etc.
 * mdbx: clarify ov-pages copying in cursor_put().


ReOpenLDAP-1.0, 2016-05-09
==========================

Briefly:
 1. The first stable release ‎ReOpenLDAP‬ on Great Victory Day!
 2. Fixed huge number of bugs. Made large number of improvements.
 3. On-line replication works robustly in the mode multi-master.

Currently ReOpenLDAP operates in telco industry throughout Russia:
 * few 2x2 multi-master clusters with online replication.
 * up to ~100 millions records, up to ~100 Gb data.
 * up to ~10K updates per second, up to ~25K searches.

Seems no anyone other LDAP-server that could provide this (replication fails,
not reaches required performance, or just crashes).

New:
 * slapd: 'keepalive' config option.
 * slapd: adds biglock's latency tracer (-DSLAPD_BIGLOCK_TRACELATENCY=deep).
 * mdbx: lifo-reclaimig for weak-to-steady conversion.
 * contrib: backport - ITS#6826 conversion scripts.
 * mdbx: simple ioarena-based benchmark.
 * syncrepl: 'require-present' config option.
 * syncprov: 'syncprov-showstatus' config option.

Documentation:
 * man-ru: 'syncprov-showstatus none/running/all' feature.
 * man: libreldap ITS#7506 Properly support DHParamFile (backport).

Major bugs:
 * syncrepl: fix RETARD_ALTER when no-cookie but incomming entryCSN is newer.
 * mdbx: backport - ITS#8412 fix NEXT_DUP after cursor_del.
 * mdbx: backport - ITS#8406 fix xcursors after cursor_del.
 * backend-mdb: fix 'forgotten txn' bug.
 * syncprov: fix error handling when syncprov_findcsn() fails.
 * syncprov: fix rare segfault in search_cleanup().
 * backend-bdb/hdb: fix cache segfault.
 * syncprov: fix possibility of loss changes.
 * syncprov: fix error handling in find-max/csn/present.
 * syncprov: fix 'missing present-list' bug.
 * syncprov: avoid lock-order-reversal/deadlock (search under si-ops mutex).
 * slapd: fix segfault in connection_write().
 * mdbx: backport - ITS#8363 Fix off-by-one in mdb_midl_shrink().
 * mdbx: backport - ITS#8355 fix subcursors.
 * syncprov: avoid deadlock with biglock and/or threadpool pausing.

Minor bugs:
 * syncrepl: refine status-nofify for dead/dirty cases.
 * syncrepl: more o_dont_replicate for syncprov's mock status.
 * syncrepl: don't notify QS_DIRTY before obtain connection.
 * syncprov: refine matchops() for search-cleanup case.
 * slapd: fix valgrind-checks for sl-malloc.
 * liblber: fix hipagut support for realloc.
 * backend-ldap: fix/remove gentle-kick.
 * mdbx: workaround for pthread_setspecific's memleak.
 * mdbx: clarify mdbx_oomkick() for LMDB-mode.
 * syncrepl: backport - ITS#8413 don't use str2filter on precomputable filters.
 * mdbx: always copy the rest of page (MDB_RESERVE case).
 * mdbx: fix nasty/stupid mistake in cmp-functions.
 * mdbx: backport - ITS#8393 fix MDB_GET_BOTH on non-dup record.
 * slapd: request thread-pool pause only for SLAP_SERVER_MODE.
 * slapd: fix backover bug (since 532929a0776d47753377461dcf89ff38aba61779).
 * syncrepl: enforce csn/cookie while recovering lost-delete(s).
 * syncrepl: fix 'quorum' for mad configurations.
 * backend-mdb: fix mdb_opinfo_get() error handling.
 * syncrepl: fix 'limit-concurrent-refresh' feature.
 * slapd: ignore EBADF in epoll_ctl(DEL).
 * syncprov: fix rare assert-failure on race with abandon.
 * mdbx: fix mdb_kill_page() for MDB_PAGEPERTURB.
 * libldap: backport - ITS#8385 Fix use-after-free with GnuTLS.
 * syncprov: fix minor op-memleak.
 * syncrepl: don't skipping retarded DELETE-notification with UUID.
 * syncrepl: don't replicate glue-ancestors, but not an entry.
 * syncrepl: del_nonpresent() - filtering glue entries as usual.
 * syncrepl: consider notifications with the same CSN as an 'echo'.
 * syncrepl: checking for present-list before delete-nonpresent.
 * syncrepl: accepts extra refresh-present in multi-master.
 * syncprov: send cookie even if entryCSN available.
 * syncprov: oversight refresh-present for multi-master.
 * syncprov: fix persistent-search - rework cleanup and release.
 * syncprov: don't skips sending DELETE-notify to the originator.
 * syncprov: don't filter refresh-resp just by originator.
 * slapd: stop scan on self-committed in slap_get_commit_csn().
 * syncrepl: enumerates operations to distinguish from each other.
 * backend-mdb: fix mistake backport ITS#8226.
 * syncrepl: fix & rework compare_cookies().
 * syncrepl: fix pre-condition for delete-nonpresent.
 * mdbx: mdbx_chk - empty freedb record isn't an error.
 * backend-mdb: fix infinite loop in callback removal.
 * mdbx: fix percent in mdbx_txn_straggler().
 * slapd: fix and make optional ordering of pending-csn queue.
 * mdbx: fix madvise() flags, it is not a bitmask.
 * mdbx: cleanup inherited errno's bug.
 * syncrepl: accept empty incoming cookies if iddqd/idclip is off.
 * syncprov: don't skip 1900-sub csn in SS_CHANGED case.
 * syncprov: backport - ITS#8365 partially revert ITS#8281.
 * backend-mdb: more to avoid races on mi_numads.
 * backend-mdb: backport - ITS#8360 fix ad info after failed txn.
 * backend-mdb: backport - ITS#8226 limit size of read txns in searches.
 * slapd: cleanup bullshit around op->o_csn.
 * syncprov: wake waiting mod-ops when handle loop-pause.
 * syncprov: don't block mod-ops by waiting fetch-ops when pool-pause pending.
 * accesslog: backport - ITS#8351 fix callback init.
 * syncprov: mutual fetch/modify - wakes opposite if waiting was broken.
 * syncrepl: fix race on cookieState->cs_ref.

Performance:
 * mdbx: more likely/unlikely for mdb_node_add.
 * slapd: remove crutch-locks from config-backend.
 * mdbx: don't memcpy when src eq dest.
 * backend-mdb: logs begin/end of OOM, but not an iteration.
 * mdbx: refine find_oldest() and oom_kick().
 * mdbx: refine/speedup mdb_cmp_memn().
 * mdbx: MADV_REMOVE for unallocated space.
 * mdbx: extra backlog's page for MDB_LIFORECLAIM.
 * mdbx: rework backlog for freeDB deletion.
 * mdbx: refine mdb_env_sync0().
 * mdbx: refine mdbx_cmp2int().
 * mdbx: backport - mdb_drop optimization.
 * slapd: remove unused scoped-locks.
 * syncrepl: non-modal del_nonpresent().
 * syncprov: less locking for mock ContextCSN.
 * syncrepl: less debug/logging.
 * slapd: less debug/logging for biglock.
 * mdbx: more __inline/__hot.
 * backend-mdb: less debug/logging for dreamcatcher.
 * mdbx: refine mdb_meta_head_r().
 * mdbx: refine mdb_env_sync().
 * syncrepl: simplify biglock usage in msg-loop.
 * slapd: kill bconfig's crutch mutex.
 * syncrepl: minor speedup check_for_retard().
 * mdbx: msync only used part instead of entire db.

Build:
 * backend-mdb: ability to use old/origin liblmdb.
 * libldap: ITS#8353 more for OpenSSL 1.1.x compat.
 * libldap: ITS#8353 partial fixes (openssl 1.1.x).

Cosmetics:
 * mdbx: reporting 'Unallocated' instead of 'Free pages'.
 * mdbx: reporting 'detaited'  instead of 'reading'.

Other:
 * mdbx: be a bit more precise that mdb_get retrieves data (ITS#8386).
 * mdbx: adds MDBX_ALLOC_KICK for freelist backlog.
 * check MDB_RESERVE against MDB_DUPSORT.
 * syncrepl: dead/dirty/refresh/process/ready for quorum-status.
 * syncrepl: idclip-resync when present-list leftover after del_nonpresent().
 * syncrepl: fix missing strict-refresh in config-unparse.
 * syncprov: tracking for refresh-stage of sync-psearches.
 * syncprov: jammed & robust sync.
 * mdbx: adds MDB_PAGEPERTURB.

For news and changes early than 2016 please refer to [ChangeLog](ChangeLog)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
