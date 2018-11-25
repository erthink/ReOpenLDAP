v1.1.10 2018-09-26, Golden Bull
===============================

## Briefly:
 1. Prevention mdb-backend database corruption.

    > Corresponding [rebalance bug](https://github.com/leo-yuriev/libmdbx/issues/38) inheritred from LMDB/OpenLDAP.
    > Now it completely fixed in the `devel` branch and future ReOpenLDAP 1.2.x versions,
    > where the new _libmdbx_ version is used.
    >
    > Unfortunately these fixes cannot be backported into the legacy _libmdbx_ version,
    > which used in the `master` branch and 1.1.x versions.
    > On the other hand, the conditions prerequisite for DB corruption are extremely rare and
    > maybe cannot be reproduced by LDAP use cases.
    >
    > Therefore 1.1.10 and later 1.1.x releases (the `stable/1.1` branch) will not contain the complete fix,
    > but only the minimal changes for prevention such corruption,
    > i.e. will **return error and abort transaction instead of DB corruption**.

 2. Improvements for configure, building. Cleanup and reformatting the source code.
 3. Other minor bugs were fixed.

### New features and Compatibility breaking: _none_

### Documentation:
 * mdbx: `mdb_cursor_del` don't invalidate the cursor (ITS#8857).
 * mdbx: `GET_MULTIPLE` don't return the key (ITS#8908).
 * doc: update `README.md`

### Major and Security bugs:
 * mdbx: prevent DB corruption due rebalance bugs.

### Minor bugs:
 * libreldap: fix "retry `gnutls_handshake` after `GNUTLS_E_AGAIN`" (ITS#8650).
 * slapd: omit hidden DBs from rootDse (ITS#8912).
 * slapd: fix `authz-policy all` condition (ITS#8909).
 * backend-mdb, backend-bdb: fix index delete.

### Performance: _none_

### Build:
 * reopenldap: fix GCC-8.x warnings.
 * libreldap: add missing includes (ITS#8809).
 * configure: rework search `NdbClient` headers and libraries.
 * configure: add `OSSP-UUID` search for modern _Fedora/RHEL_.
 * configure: fix `EXTRA_CFLAGS` typo.
 * configure: explicit separation of experimental backends.
 * configure: refine error-msg for mysql_cluster's `mysql_config`.

### Cosmetics:
 * reopenldap: fix typo with ITS#8843 description.
 * reopenldap: reformat source code by `clang-format-6.0`.
 * reopenldap: remove `LDAP_P` macro.
 * reopenldap: drop `LDAP_CONST` macro.

### Other:
 * mdbx: drop inherited broken audit.
 * tests: export `LC_ALL=C` as workaround as Fedora's `diff` utility bugs.
 * ci: add `LIBTOOL_SUPPRESS_DEFAULT=no` into scripts.
 * ci: add `ci/fedora.sh` script.
 * ci: update `ci/debian.sh` script.

--------------------------------------------------------------------------------

v1.1.9 2018-08-02, Airborne Positive
====================================

## Briefly:
 1. Fixed TLS/SSL major bugs (deadlocks and segfaults).
 2. Other minor bugs were fixed.
 3. Added TLS/SSL test into test suite.

### New features and Compatibility breaking: _none_

### Documentation:
 * fix quoting example in man-pages.
 * add `DN` qualifier and `regexp` for `sock` backend (ITS#8051).
 * update libmdbx Project Status.

### Major and Security bugs:
 * libreldap: fix init/shutdown races/segfaults with modern OpenSSL.
 * libreldap: fix deadlock/recursion inside `tls_init()` internals.

### Minor bugs:
 * libreldap: add printability checks on the dc `RDN` (ITS#8842).
 * overlay-memberof: Improve `memberof cn=config` handling (ITS#8663).
 * backend-glue: don't finish initialisation in tool mode unless requested (ITS#8667).
 * backend-mdb, backend-bdb: don't convert `IDL` to range needlessly (ITS#8868).
 * backend-sock: use a `regexp` (ITS#8051).
 * backend-sock: add `DN` qualifier (ITS#8051).
 * libreldap: fix unlock in error-case inside thread-pool `handle_pause()`.
 * libreldap: fix `ber_realloc` after a partial `ber_flush` (ITS#8864).
 * slapd: fix wrong/duplicate listening if bind failed.
 * slapd: fix ldif-wrap errmsg typo.

### Performance:
 * slapd: minor refine suspend/refine listeners.

### Build:
 * configure: update for modern libtool.
 * configure: fix quoting for empty variable.
 * configure: add TLS-tests.
 * configure: add checking for `libnsspem`.

### Cosmetics:
 * libreldap: message-hint to check `libnsspem.so` for TLS by MozNSS.
 * slapd: cleanup-drop `SLAP_FD2SOCK/SLAP_SOCK2FD/SLAP_SOCKNEW`.

### Other:
 * tests: don't ignore `its8667` regression while CI/buzz-testing.
 * tests: fix `$MONITORDB` usage.
 * tests: add regression test for ITS#8616.
 * tests: add TLS-tests.
 * ci: migrate to Circle-CI 2.0

--------------------------------------------------------------------------------

v1.1.8 2018-06-04
=================

## Briefly:
 1. Fixed MDBX major bug (DB corruption).
 2. Fixed slapd memory corruption and other segfaults.
 3. Fixed build for Elbrus architecture.

### New features and Compatibility breaking: _none_

### Documentation:
 * `SASL_MECH/SASL_REALM` are not user-only (ITS#8818).
 * fix `SIGHUB` typo.

### Major and Security bugs:
 * fix wrong `freeDB` search.
 * fix memory corruption in connection-handling code.
 * fix `op_response()` segfault.

### Minor bugs:
 * syncprov: don't check for existing value when deleting values (ITS#8616).
 * slapd: fix `domainScope` control to ensure the control value is absent (ITS#8840).
 * mdbx: can't use fakepage `mp_ptrs` directly (ITS#8819).
 * mdbx: fix regression in 0.9.19 (ITS#8760).
 * mdbx: fix `FIRST_DUP/LAST_DUP` cursor bounds check (ITS#8722).

### Performance:
 * mdbx: `XCURSOR_REFRESH()` fixups/cleanup.
 * syncprov, syncrepl, accesslog: reduce unnecessary writes of `contextCSN` entry (ITS#8789).

### Build:
 * mdbx: fix check make target (minor).
 * reopenldap: refine `bootstrap.sh`.
 * automake: fix `-fno-lto` for `.symver memcpy_compat, memcpy@@@GLIBC_2.2.5`
 * liblutil: fix warning variable `hex` set but not used (minor).
 * libreldap, slapd: avoid trigraphs in comments (minor).
 * ldaptools: fix 'uninit' warnigs form lcc (minor).
 * libreldap: fix callbacks for `NSPR`.
 * libreldap: `#ifdef` for `SSL_LIBRARY_VERSION_TLS_1_3`.

### Cosmetics:
 * mdbx: minor fixup comments and warnings.

### Other:
 * tests: fix copypasta in `its8444` regression script.
 * ci: drop support for old/legacy versions.
 * slapd: add backtrace support for _Elbrus_.
 * reopenldap: update `reldap.h` and `ldap_cdefs.h` for _Elbrus_.
 * libmdbx: update `defs.h` for _Elbrus_.
 * tests: Ensure there are no differences due to different checkpoints (ITS#8800).

--------------------------------------------------------------------------------

v1.1.7 2018-02-23, Red Army Soldier
===================================

## Briefly:
 1. Added more Russian man-pages (thanks to Egor Levintsa, http://pro-ldap.ru).
 2. The _`ldap dirs`_ bug fixed.
    A _@variables@_ macros were not replaced with actual configured paths (thanks to Dmitrii Zolotov).
 3. Fixed enough other bugs and warnings.

### New features and Compatibility breaking:
 * Public key pinning support (ITS#8753).
 * Allow to recognize title-case characters even if they do not have lower-case equivalents (ITS#8508).
 * Legacy `ldap_pvt_thread_rmutex` removed.
 * POSIX recursive mutex for libevent (ITS#8638).
 * New `ldap_connect()` function (ITS#7532).

### Documentation:
 * man: Docs for `reqEntryUUID` (ITS#6656).
 * man: Added Russian man-pages.
 * contrib-moduled: remove obsolete notes about `LDAP_SRC`.
 * man: Note `ldap_sasl_bind()` can be used to make simple binds via the LDAP_SASL_SIMPLE mechanism (ITS#8121).
 * man: Index on `entryCSN` is mandatory note (ITS#5048).
 * configure: Note about `EXTRA_CFLAGS` variable.
 * man: Fix typo with `olcTLSCipherSuite` (ITS#8715).

### Major and Security bugs:
 * back-ldap: Fix search double-free and/or memory corruption.
 * overlay-sss: Fix server-side-sort overlay segfault.
 * build: Fix `ldap_dirs.h` issue (@variables@ are not replaced with paths on).

### Minor bugs:
 * libmdbx: Fix cursor ops (squashed ITS#8722).
 * overlay-dds: Fix callbacks (invalid results or bug-check).
 * slapd-schema: Don't strip pretty-trailing zeros while normalize time.
 * syncprov: Try other CSNs as pivot if consumer's and provider's sets are the same.
 * syncrepl: Add `SYNC_NEED_RESTART` code and handling.
 * syncrepl: Fix resched-interval.
 * syncrepl: return `LDAP_UNWILLING_TO_PERFORM` in case no any CSN's.
 * libmdbx: Fix regression in 0.9.19 (ITS#8760).
 * accesslog: Fix CSN queue processing (ITS#8801).
 * syncprov: Don't replicate checkpoints (ITS#8607).
 * syncprov: Fixes for delta-syncrepl with empty accesslog (ITS#8100).
 * slapd: Fix SASL SSF reset (ITS#8796).
 * libreldap: Fix MozNSS initialization (ITS#8484).
 * libreldap: Plug memleaks in cancel (ITS#8782).
 * slapd: Fix `telephoneNumberNormalize` (ITS#8778).
 * syncprov: Use cookie and `thread_pool_retract()` when overlay deleting.
 * accesslog: Fix recursive locking.
 * slapd: `olcTimeLimit` should be Single Value (ITS#8153).
 * overlay-lastbind: Allow `authTimestamp` updates to be forwarded via updateref (ITS#7721).
 * libreldap: Non-blocking TLS is not compatible with MOZNSS (ITS#7428).
 * slapd: Fix additional compile for `/dev/poll` support.
 * slapd: Fix calls to `SLAP_DEVPOLL_SOCK_LX` for multi-listener support.
 * slapd: Always remove listener descriptors from daemon on shutdown.
 * slapd: Avoid listener thread startup race (ITS#8725).
 * libreldap: Plug ber leaks (ITS#8727).
 * backend-sock: Send out EXTENDED operation message from back-sock (ITS#8714).
 * backendl-ldap: TLS connection timeout with high latency connections (ITS#8270).
 * overlay-constraint: Fix comparison between pointer and zero character constant.
 * backend-mdb: Fix "warning: '%s' directive output may be truncated..."
 * syncrepl: Fix "syncrepl_process: rid=NNN (-45) Unknown API error".
 * slapd: Fix 'ptrace: Operation not permitted' from backtrace feature.

### Performance:
 * slapd: isolate tsan-mutexes under #ifdef __SANITIZE_THREAD__.
 * syncprov: Don't keep `sl_mutex` locked when playing the `sessionlog` (ITS#8486).
 * accesslog: Fix CSNs for purge logs.
 * backend-mdb: Optimize restart search txn (ITS#8226).

### Build:
 * libreldap, slapd, contrib-modules: Fix `RELDAP_TLS`/`RELDAP_TLS_FALLBACK`.
 * libreldap: Fix uninit warning for GNUTLS.
 * libreldap: Fix compilation with older versions of OpenSSL (ITS#8753, ITS#8774).
 * libreldap: Fix `HAVE_OPENSSL_CRL` and `HAVE_GNUTLS` usage.
 * libreldap: Move base64 decoding to separate file (ITS#8753).
 * reopenldap: Refine reopenldap's macros definitions.
 * slapd: Add `LDAP_GCCATTR` for compat.
 * configure: Fix `EXTRA_CFLAGS` substitution magic.
 * contrib-modules: Refine totp for OpenSSL 1.1.0 compatibility.
 * slapd: Add `crypt_r()` support (ITS#8719).
 * build: Patch `libltdl` to avoid clang > 3.x warnings.
 * travis-ci: Allow build in forks
 * configure: Note about `EXTRA_CFLAGS`.

### Cosmetics:
 * tests: Update its8800 to be less different from openldap.
 * tests: Enumerate its-ignore messages.
 * reopenldap: Sync CHANGES.OpenLDAP.
 * syncrepl: Refine loging, rename `SYNCLOG_LOGBASED` (cosmetics).
 * libmdbx: Sync CHANGES with LMDB.
 * reopenldap: Remove kqueue from project list (ITS#6300).
 * reopenldap: Rename `MAY_UNUSED`/`__maybe_unused`.
 * reopenldap: Spelling fixes (ITS#8605).
 * syncprov: Minor refine loging when the consumer has a newer cookie than the provider (ITS#8527).
 * contrib: move 'docker' dir into contrib.
 * reopenldap: HNY!

### Other:
 * tests: Skip its4336 and its4326 regressions for ldap-backend.
 * slapd-schema: Use `slap_csn_verify_full()` for CSN validation.
 * syncprov: Rework contextCSN/si_cookie/maxcsn inside `syncprov_op_response()`.
 * libutil: Add `const` to `lutil_parsetime()`.
 * tests: Ignore its8800 failures due known issue.
 * tests: Move ignore-note to its number (cosmetics).
 * tests: Regression test for ITS8800.
 * tests: Refine its8752.
 * tests: Enable its4448 for CI.
 * tests: More `DBNOSYNC=YES` to speedup.
 * tests: Add timeout and inter-pause parametrs for wait_syncrepl.
 * libreldap: Refine `ldap_now_steady_ns()`.
 * slapd: Simplify csn's queue internals.
 * tests: Ignore its8444 failures while buzz-testing.
 * tests: Fix use-after-free in slapd_bind.
 * tests: Fix swapped arguments (ITS#8798).
 * tests: Do not insert delays on a successful bind (ITS#8798).
 * tests: Add SASL support to tools (ITS#8798).
 * tests: Enable retry/delay in slapd-bind (ITS#8798).
 * tests: Use `lrand48()` instead of legacy `rand()`.
 * tests: Unify tools setup (ITS#8798).
 * tests: Fix description to match the actual issue that was fixed (ITS#8444).
 * slapd: Add `slap_sl_mark()` and `slap_sl_release()`.
 * syncrepl: Add support for relax control to delta-syncrepl (ITS#8037).
 * libreldap: Allow a raw integer to be decoded from a berval (ITS#8733).
 * libreldap: Allow extraction of the complete ber element (ITS#8733).
 * reopenldap: Extend `CIRCLEQ` macros (ITS#8732).
 * libreldap: Koging mismatching of SASL-version.
 * backend-mdb: Temporary hack/workaround for hash collisions (ITS#8678).
 * libreldap: Call `ldap_int_sasl_init()` from `ldap_int_initialize()`.
 * tests: Fix 'warning: comparison between pointer and zero character constant'.
 * slapd: Check `prctl(PR_SET_PTRACER)` result while backtracing.
 * reopenldap: Add Dmitrii Zolotov into AUTHORS.

--------------------------------------------------------------------------------

v1.1.6 2017-08-12
=================

## Briefly:
 1. A lot of bug fixing.
 2. Support for [musl-libc](https://www.musl-libc.org), fixes related to build and dependencies.
 3. Continuous Integration by [Travis-CI](https://travis-ci.org/leo-yuriev/ReOpenLDAP)
    and [Circle-CI](https://circleci.com/gh/leo-yuriev/ReOpenLDAP).

### New features and Compatibility breaking:
 * libreldap, mdbx: musl support.
 * contrib: argon2 password hashing module (ITS#8575).
 * libreldap: more for LibreSSL and OpenSSL 1.1.0c (ITS#8533, ITS#8353).
 * overlays: add AutoCA overlay.
 * mdbx: support glibc < 2.18 for TLS cleanup on thread termination.
 * libreldap: adds ldif_open_mem() (ITS#8603).
 * slapd: Add config support for binary values.
 * libreldap: Add options to use DER format cert+keys directly.
 * proxy-cache, all: use LDAP_DEBUG_CACHE/Cache.
 * mdbx: don't ignore `data` arg in mdb_del() for libfpta.
 * mdbx: rework mdbx_replace() for libfpta.
 * mdbx: add mdbx_dbi_open_ex() for libfpta.
 * mdbx: add mdbx_is_dirty() for libfpta.
 * mdbx: add MDBX_RESULT_FALSE and MDBX_RESULT_TRUE for libfpta.
 * mdbx: zero-length key is not an error for MDBX.
 * mdbx: MDBX_EMULTIVAL errcode for libfpta.
 * mdbx: allows cursors to be free/reuse explicitly, regardless of transaction wr/ro type.
 * mdbx: adds mdbx_get_ex() for libfpta.
 * mdbx: adds mdbx_replace() for libfpta.
 * mdbx: allows zero-length keys for libfpta.
 * mdbx: rework MDB_CURRENT handling for libfpta.
 * mdbx: adds mdbx_cursor_eof() for libfpta.
 * mdbx: explicit overwrite support for mdbx_put().
 * mdbx: add 'canary' support for libfpta.
 * mdbx: 'attributes' support for Nexenta.

### Documentation:
 * man: Fix wording to match examples (ITS#8123).
 * man-contib: add man-pages for contrib overlays (ITS#8205).
 * man: Note that non-zero serverID's are required for MMR, and that serverID 0 is specific to single master replication only (ITS#8635).
 * man: Note that slapo-memberOf should not be used in a replicated environment (ITS#8613).
 * doc: cleanup tabs in CHANGES.OpenLDAP
 * doc: Catalog of assigned OID arcs.
 * man: Fix VV option information (ITS#7177, ITS#6339).
 * man: Further clarification around replication information (ITS#8253).
 * Update CONTRIBUTING.md
 * mdbx: notes about free/reuse cursors.
 * slapd: refine note for Cyrus-SASL memleak.
 * contrib: minor Update TOTP README (ITS#8513).
 * man: Add a manpage for slapo-autogroup (ITS#8569).
 * man: Grammar and escaping fixes (ITS#8544).
 * man: Clearly document rootdn requirement for the ppolicy overlay (ITS#8565).
 * mdbx: rework README.

### Major and Security bugs:
 * mdbx: don't madvise(MADV_REMOVE).
 * backend-mdb: fix double free on paged search with pagesize 0 (ITS#8655).
 * reldap: retry gnutls_handshake after GNUTLS_E_AGAIN (ITS#8650).
 * slapo-sssvlv: Cleanup double-free fix in sssvlv overlay (ITS#8592).
 * libreldap: fix races around tls_init().
 * libreldap: use pthread_once() for SASL init (fix Debian Bug #860947).
 * mdbx: fix snap-state bug (backport).
 * slapd: fix segfault (ITS#8631)
 * libreldap: Fixup cacert/cert/key options.
 * libreldap: fix hipagut for ARM/ARM64 (and other where alignment is required).
 * overlay-sssvlv: try to fix double-free in server side sort (ITS#8592, ITS#8368).
 * libreldap: Avoid hiding the error if user specified CA does not load (ITS#8529).
 * syncrepl: fix refer to freed mem.
 * slapd: fix sasl SEGV rebind in same session (ITS#8568).
 * mdbx: CHANGES for glibc bugs #21031 and #21032.

### Minor bugs:
 * mdbx: ITS#8699 more for cursor_del ITS#8622.
 * slapd: avoid hang/crash the backtrace_sigaction().
 * reopenldap: avoid deadlock/recursion in debug-output.
 * syncrepl: LDAP_PROTOCOL_ERROR if entryCSN missing in 'IDCLIP' mode.
 * mdbx: fix mdbx_set_attr().
 * mdbx: fix mdbx_txn_straggler() for write-txn (backport from devel).
 * mdbx: fix crash on twice txn-end (backport from devel).
 * reldap: check result of ldap_int_initialize in ldap_{get,set}_option (ITS#8648).
 * slapd: fix LDAP_TAILQ macro, nice bug since 2002 (ITS#8576).
 * slapd, autoca-overlay: Move privateKey schema into slapd.
 * slapd: Update accesslog format and syncrepl consumer (ITS#6545).
 * libreldap: Ensure that the deprecated API is not used when using OpenSSL 1.1 or later (ITS#8353, ITS#8533).
 * unique-overlay: Allow empty mods (ITS#8266).
 * libutil, slapd: Separate Avlnode and TAvlnode types (ITS#8625).
 * libreldap, slapd: Fixes for multiple threadpool queues.
 * mdbx: ITS#8622 fix xcursor after cursor_del.
 * slapd: Deal with rDN correctly (ITS#8574).
 * syncprov: fix possibility of use freed `pivot_csn`.
 * mdbx: fix cursor-untrack bug.
 * slapd: fix memleaks from mask_to_verbstring().
 * slapd: fix minor config-value_string memleak.
 * libreldap: fix minor PL_strdup(noforkenvvar) memleak.
 * slapd: workaround for Cyrus memleak.
 * backend-mdb: fix cursor leaks (follow libmdbx API changes).
 * libreldap: Fail ldap_result if handle is already bad (ITS#8585).
 * mdbx: fix losing a zero-length value of sorted-dups (for libfpta).
 * slapd: fix slap_tls_get_config().
 * slapd: fix mr_index_cmp() for match-rules.
 * ci: fix static/dymanic for backends.
 * mdbx: fix MDB_CURRENT for MDB_DUPSORT in mdbx_cursor_put() for libfpta.
 * mdbx: fix LEAF2-pages handling in mdb_cursor_count().
 * slapd: fix LDAP_OPT_X_TLS_CRLFILE.
 * slapd: temporary fix for issue#120 (its8444).
 * syncprov: bypass refresh for refrech-and-persist requests when no local cookies.
 * syncprov: minor fix rid/sid debug output.
 * slapd: don't treat an empty cookie string as the protocol violation.
 * syncrepl: pull cookies before fallback to refresh from delta-mmr.
 * mdbx: fix xflags inside mdb_cursor_put().
 * mdbx: fix cursor EOF tricks.
 * syncrepl: immediately schedule retry for LDAP_SYNC_REFRESH_REQUIRED.
 * syncprov: LDAP_BUG() in op-responce if op-tag missing.
 * accesslog: fix missing op-tag.
 * syncrepl: allow empty sync-cookie for delta-mmr (accesslog).
 * mdbx: fix mdb_cursor_last (ITS#8557).
 * mdbx: ITS#8558 fix mdb_load with escaped plaintext.
 * mdbx: fix cursor_count() for libfpta.
 * mdbx: mdb_chk - don't close dbi-handles, set_maxdbs() instead.
 * mdbx: fix MDB_GET_CURRENT for dupsort's subcursor.

### Performance:
 * mdbx: `unlikely` for DB_STALE.
 * mdbx: check `__OPTIMIZE__` for `__hot`/`__cold`/`__flatten`.

### Build:
 * configure: fix subst for VALGRIND_SUPPRESSIONS_FILES.
 * configure: add '--enable-ci' option for Continuous Integration.
 * bootstrap: add patch for old ltmain.sh versions.
 * configure: check for pkg_config.
 * configure: use CPPFLAGS while check headers.
 * configure: use OPENSSL_CFLAGS and GNUTLS_CFLAG while check headers.
 * build: add workaround for libtool `-no-suppress`.
 * build: add `common.mk` (placeholder for now).
 * build: add support for EXTRA_CFLAGS.
 * slapd: fix gcc `-Ofast` warnings.
 * build: check libsodium >= 1.0.9 for argon2.
 * dist: use `expr` instead of `bc`.
 * reopenldap: update automake's stuff for libmdbx changes.
 * contrib: `-Wno-address` for nssov.
 * slapd: checks and HAVE_ENOUGH4BACKTRACE for backtrace feature (compatibility).
 * configure: add missing ldap_dir.h.in (oops).
 * configure: libuuid by pkg-config.
 * reopenldap: initial for cross-compilation.
 * mdbx: adds -ffunction-sections for CFLAGS.
 * mdbx: enable C99.

### Cosmetics:
 * configure: fix message alignment (cosmetics).
 * reopenldap: update links after move the repo.
 * mdbx: update links after move the repo.
 * ci: add Travis-CI status to README.md
 * reopenldap: add TODO.md
 * libreldap: Fix minor typo (ITS#8643).
 * back-monitor: fix monitoredInfo.
 * reopenldap: fix 'emtpy' typos (ITS#8587).
 * syncprov: refine 'syncprov-sessionlog' config.
 * syncprov: minor renames (cosmetics).
 * syncprov: refine add_slog (cosmetics).
 * slapd: refine SlapdVersionStr.
 * mdbx: remote extra LNs (cosmetics).
 * mdbx: mdb_chk - cosmetics (no extra \n).

### Other:
 * libreldap: rename ber_error_print() to ber_debug_print().
 * reopenldap: rename ldap-time functions.
 * libreldap: drop -ber_pvt_log_output().
 * reopenldap: rework ldap-time functions.
 * slapd: refine daemon event loop (still historically madness).
 * reldap: add ldap_debug_flush(), refine debug-locking.
 * syncrepl: clarity debug error-string.
 * ci: add SLAPD_TESTING_DIR and SLAPD_TESTING_TIMEOUT.
 * slapd: add slap_setup_ci() with engaged by '--enable-ci'.
 * test: add regression test for ITS#8667.
 * libreldap: move ldap_init_fd() definition to ldap.h
 * slapo-valsort: fix 'unused result' warnings around strtol().
 * slapd: log 'active_threads' on TRACE-level from daemon.
 * autoca-overlay: tweaks length of keys.
 * autoca-overlay: Tweaks for OpenSSL 1.1 API deprecations.
 * libreldap: add MAY_UNUSED to avoid warnings from Clang.
 * libreldap: remove needless conds.
 * test: add temporary workaround for issue#121.
 * test: add `dbnosync` flag for its4448.
 * slapd: Tweak privateKeyValidate for PKCS#8.
 * libreldap: Add GnuTLS support for direct DER config of cacert/cert/key.
 * autoca-overlay: squashed fixups.
 * libreldap: Add ldap_pvt_thread_pool_queues decl.
 * slapd: Fixup for binary config attrs.
 * slapd: minor fixup pause handling in config-backend.
 * slapd: Support setting cacert/cert/key directly in cn=config entry.
 * libreldap: fix debug-log warning.
 * mdbx: don't close/lost DBI-handles on ro-txn renew/reset.
 * mdbx: don't close DBI-handles from R/O txn_abort().
 * slapd: use ARG_BAD_CONF for config().
 * backend-mdb: use ARG_BAD_CONF for config().
 * mdbx: more for robustness free/reuse of cursors.
 * mdbx: minor simplify mdb_del0().
 * mdbx: use MDB_SET_KEY inside mdbx_replace() for libfpta.
 * mdbx: fix MDB_CURRENT for mdb_cursor_put() with MDB_DUPSORT.
 * mdbx: refine mdbx_cursor_eof().
 * mdbx: Tweak cursor_next C_EOF check.
 * mdbx: rework TLS cleanup on thread termination.
 * mdbx: assert_fail() when `INDXSIZE(key) > nodemax`.

--------------------------------------------------------------------------------

v1.1.5 2016-12-30
=================

## Briefly:
 1. Set of fixes for MDBX and mdb-backend.
 2. Several fixes related to testing.
 3. Few fixes related to build and dependencies.

### New features and Compatibility breaking:
 * ci: scripts from `ps/build` branch.
 * configure: adds `check-news` option.
 * build: add its-regressions to `make test` target.

### Documentation:
 * mdbx: set of LMDB-0.9.19 updates (doxygen and comments).
 * man: `interval` keyword info (ITS#8538).

### Major and Security bugs: _none_

### Minor bugs:
 * mdbx: more for cursor tracking after deletion (ITS#8406).
 * mdbx: mdb_env_copyfd2(): Don't abort on SIGPIPE (ITS#8504).
 * mdbx: fix ov-pages copying in cursor_put().
 * mdbx: catch mdb_cursor_sibling() error (ITS#7377).
 * mdbx: mdb_dbi_open(): Protect mainDB cursors (ITS#8542).
 * backend-mdb: fix mdb_indexer() segfault after cursor closing.
 * backend-mdb: refine mdb_tool_xxx() cursor closing.
 * backend-mdb: fix mdb_idl_fetch_key() segfault after cursor closing.
 * backend-mdb: fix mdb_online_index() cursor leak.
 * backend-mdb: simplify mdb_attr_index_config() AttrInfo init.
 * backend-mdb: fix mdb_add() cursor leak.
 * backend-mdb: fix cursor leak.

### Performance: _none_

### Build:
 * configure: checking for libperl.
 * reopenldap: fix `unused` warnings for `--disable-debug`.
 * build: add its-regressions to `make test` target.
 * reopenldap: fix GCC 6.x warnings (misleading indentation).
 * tests: refine ITS's regression tests.
 * build: fix automake xxx_DEPENDENCIES.
 * ci: scripts from `ps/build` branch.
 * configure: adds `check-news` option.

### Cosmetics:
 * tests: uses `tput` for change output color/contrast.
 * reopenldap: changelog note for ITS#8525.
 * reopenldap: fix typos in NEWS/ChangeLog.
 * mdbx: fix typo.

### Other:
 * backend-mdb: Fix its6794 test.
 * tests: refine running its-cases.
 * tests: Specifically test for error 32 on the consumer.
 * mdbx: Pass cursor to mdb_page_get(), mdb_node_read().
 * mdbx: Cleanup: Add flag DB_DUPDATA, drop DB_DIRTY hack.
 * tests: fix its8521 config data.
 * slapd: fix build legacy backends after str2entry() changes.
 * tests: fix regression test for its8521.
 * slapd: return error from str2entry().
 * backend-mdb: mdb_tool_terminate_txn().
 * tests: split-put run_testset().
 * tests: testcase for ITS#8521 regression.
 * mdbx: minor simplify mc_signature.
 * mdbx: factor out refreshing sub-page pointers.
 * tests: Fix regression test for ITS#4337 (ITS#8535).
 * tests: Fix regression test to correctly load back-ldap if it is built as a module (ITS#8534).

--------------------------------------------------------------------------------

v1.1.4 2016-11-30
=================

## Briefly:
 1. Return to the original OpenLDAP Foundation license.
 2. More fixed for OpenSSL 1.1, LibreSSL 2.5 and Mozilla NSS.
 3. Minor fixes for configure/build and so forth.

### New features and Compatibility breaking:
 * reopenldap: support for OpenSSL-1.1.x and LibreSSL-2.5.x (#115, #116).
 * contrib: added mr_passthru module.
 * configure --with-buildid=SUFFIX.
 * return to the original OpenLDAP Foundation license.
 * moznss: support for <nspr4/nspr.h> and <nss3/nss.h>

### Documentation:
 * man: fix typo (ITS#8185).

### Major and Security bugs: _none_

### Minor bugs:
 * mdbx: avoid large '.data' section in mdbx_chk.
 * mdbx: fix cursor tracking after mdb_cursor_del (ITS#8406).
 * reopenldap: fix LDAPI_SOCK, adds LDAP_VARDIR.
 * mdbx: use O_CLOEXEC/FD_CLOEXEC for me_fd,env_copy as well (ITS#8505).
 * mdbx: reset cursor EOF flag in cursor_set (ITS#8489).
 * slapd: return error on invalid syntax filter-present (#108).

### Performance: _none_

### Build:
 * ppolicy: fix libltdl's includes for ppolicy overlay.
 * libltdl: move `build/libltdl` to the start of SUBDURS.
 * mdbx: don't enable tracing for MDBX by --enable-debug.
 * reopenldap: fix missing space in bootstrap.sh

### Cosmetics:
 * slapd: adds RELEASE_DATE/STAMP to `slapd -V` output.
 * mdbx: clarify fork's caveat (ITS#8505).

### Other:
 * cleanup/refine AUTHORS file.

--------------------------------------------------------------------------------

v1.1.3, 2016-08-30
==================

## Briefly:
 1. Imported all relevant patches from RedHat, ALT Linux and Debian/Ubuntu.
 2. More fixes especially for TLS and Mozilla NSS.
 3. Checked with PVS-Studio static analyser (first 10 defects were shown and fixed).
    Checking with Coverity static analyser also was started, but unfortunately it is
    a lot of false-positives (pending fixing).

### New features and Compatibility breaking:
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

### Documentation:
 * man: added page for contrib/smbk5pwd.
 * man: note for ldap.conf that on Debian is linked against GnuTLS.
 * doc: added preamble to devel/README.
 * man: remove refer to <ldap_log.h>
 * man: note olcAuthzRegex needs restart (ITS6035).
 * doc: fixed readme's module-names for contrib (.so -> .la)
 * mdbx: comment MDB_page, rename mp_ksize.
 * mdbx: VALID_FLAGS, mm_last_pg, mt_loose_count.
 * man: fixed SASL_NOCANON option missing in ldap.conf manual page.

### Major and Security bugs:
 * slapd: fixed #104, check for writers while close the connection.
 * slapd: fixed #103, stop glue-search on errors.
 * libreldap: MozNSS fixed CVE-2015-3276 (RHEL#1238322).
 * libreldap: TLS do not reuse tls_session if hostname check fails (RHEL#852476).
 * slapd: Switch to lt_dlopenadvise() to get RTLD_GLOBAL set (RHEL#960048, Debian#327585).
 * libreldap: reentrant gethostby() (RHEL#179730).
 * libreldap: MozNSS ignore certdb database type prefix when checking existence of the directory (RHEL#857373).

### Minor bugs:
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

### Performance:
 * libreldap: remove resolv-mutex around getnameinfo() and getnameinfo() (Debian#340601).
 * slapd: fixed major typo in rurw_r_unlock() which could cause performance degradation.

### Build:
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

### Cosmetics:
 * slapindex: print a warning if it's run as root.
 * fixed printf format in mdb-backend and liblunicode.
 * fixed minor typo in print_vlv() for ldif-output.
 * mdbx: minor fix mdb_page_list() message
 * fixed 'experimantal' typo ;)
 * slap-tools: fixed set debug-level.

### Other:
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

--------------------------------------------------------------------------------

v1.1.2, 2016-07-30
==================

## Briefly:
 1. Fixed few build bugs which were introduced by previous changes.
 2. Fixed the one replication related bug which was introduced in ReOpenLDAP-1.0
    So there is no even a rare related to replication test failures.
 3. Added a set of configure options.

### New:
 * `configure --enable-contrib` for build contributes modules and plugins.
 * `configure --enable-experimental` for experimental and developing features.
 * 'configure --enable-valgrind' for testing with Valgrind Memory Debugger.
 * `configure --enable-check --enable-hipagut` for builtin runtime checking.
 * Now '--enable-debug' and '--enable-syslog' are completely independent of each other.

### Documentation:
 * man: minor cleanup 'deprecated' libreldap functions.

### Major bugs:
 * syncprov: fixed find-csn error handling.

### Minor bugs:
 * slapd: accept module/plugin name with hyphen.
 * syncprov: fixed RS_ASSERT failure inside mdb-search.
 * slapd: result-asserts (RS_ASSERT) now controlled by mode 'check/idkfa'.
 * pcache: fixed RS_ASSERT failure.
 * mdbx: ITS#8209 fixed MDB_CP_COMPACT.

### Performance: _none_

### Build:
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

### Cosmetics: _none_

### Other:
 * libreldap, slapd: add and use ldap_debug_perror().
 * slapd: support ARM and MIPS for backtrace.
 * mdbx: Refactor mdb_page_get().
 * mdbx: Fix MDB_INTEGERKEY doc of integer types.
 * all: rework debug & logging.
 * slapd: LDAP_EXPERIMENTAL instead of LDAP_DEVEL.
 * slapd, libreldap: drop LDAP_TEST, introduce LDAP_CHECK.
 * slapd, libreldap: always checking if LDAP_CHECK > 2.
 * reopenldap: little bit cleanup of EBCDIC.

--------------------------------------------------------------------------------

v1.1.1, 2016-07-12
==================

## Briefly:
 1. Few replication (syncprov) bugs are fixed.
 2. Additions to russian man-pages were translated to english.
 3. A lot of segfault and minor bugs were fixed.
 4. Done a lot of work on the transition to actual versions of autoconf and automake.

### New:
 * reopenldap: use automake-1.15 and autoconf-2.69.
 * slapd: upgradable recursive read/write lock.
 * slapd: rurw-locking for config-backend.

### Documentation:
 * doc: english man-page for 'syncprov-showstatus none/running/all'.
 * doc: syncrepl's 'requirecheckpresent' option.
 * man: note about 'ServerID 0' in multi-master mode.
 * man: man-pages for global 'keepalive idle:probes:interval' option.

### Major bugs:
 * slapd: rurw-locking for config-backend.
 * syncprov: fixed syncprov_findbase() race with backover's hacks.
 * syncprov: bypass 'dead' items in syncprov_playback_locked().
 * syncprov: fixed syncprov_playback_locked() segfault.
 * syncprov: fixed syncprov_matchops() race with backover's hacks.
 * syncprov: fixed rare syncprov_unlink_syncop() deadlock with abandon.
 * slapd: fixed deadlock in connections_shutdown().
 * overlays: fixed a lot of segfaults (callback initialization).

### Minor bugs:
 * install: hotfix slaptools install, sbin instead of libexec.
 * contrib-modules: hotfix - remove obsolete ad-hoc of copy register_at().
 * syncrepl: ITS#8432 fix infinite looping mods in delta-mmr.
 * reopenldap: hotfix 'derived from' copy-paste error.
 * mdbx: mdb_env_setup_locks() Plug mutexattr leak on error.
 * mdbx: ITS#8339 Solaris 10/11 robust mutex fixes.
 * libreldap: fixed PR_GetUniqueIdentity() for ReOpenLDAP.
 * liblber: don't trap ber_memcpy_safe() when dst == src.
 * syncprov: kicks the connection from syncprov_unlink_syncop().
 * slapd: reschedule from connection_closing().
 * slapd: connections_socket_troube() and EPOLLERR|EPOLLHUP.
 * slapd: 2-stage for connection_abandon().
 * syncprov: rework cancellation path in syncprov_matchops().
 * syncprov: fixed invalid status ContextCSN.
 * slapd: fixed handling idle/write timeouts.
 * accesslog: ITS#8423 check for pause in accesslog_purge.
 * mdbx: ITS#8424 init cursor in mdb_env_cwalk.

### Performance: _none_

### Build:
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

### Cosmetics:
 * syncrepl: cleanup rebus-like error codes.
 * slapd: rename reopenldap's modes.
 * slapd: debug-locking for backtrace.
 * slapd, libreldap: closing conn/fd debug.

### Other:
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

--------------------------------------------------------------------------------

ReOpenLDAP-1.0, 2016-05-09
==========================

## Briefly:
 1. The first stable release ReOpenLDAP on Great Victory Day!
 2. Fixed huge number of bugs. Made large number of improvements.
 3. On-line replication works robustly in the mode multi-master.

### Currently ReOpenLDAP operates in telco industry throughout Russia:
 * few 2x2 multi-master clusters with online replication.
 * up to ~100 millions records, up to ~100 Gb data.
 * up to ~10K updates per second, up to ~25K searches.

Seems no anyone other LDAP-server that could provide this (replication fails,
not reaches required performance, or just crashes).

### New:
 * slapd: 'keepalive' config option.
 * slapd: adds biglock's latency tracer (-DSLAPD_BIGLOCK_TRACELATENCY=deep).
 * mdbx: lifo-reclaimig for weak-to-steady conversion.
 * contrib: ITS#6826 conversion scripts.
 * mdbx: simple ioarena-based benchmark.
 * syncrepl: 'require-present' config option.
 * syncprov: 'syncprov-showstatus' config option.

### Documentation:
 * man-ru: 'syncprov-showstatus none/running/all' feature.
 * man: libreldap ITS#7506 Properly support DHParamFile (backport).

### Major bugs:
 * syncrepl: fix RETARD_ALTER when no-cookie but incomming entryCSN is newer.
 * mdbx: ITS#8412 fix NEXT_DUP after cursor_del.
 * mdbx: ITS#8406 fix xcursors after cursor_del.
 * backend-mdb: fix 'forgotten txn' bug.
 * syncprov: fix error handling when syncprov_findcsn() fails.
 * syncprov: fix rare segfault in search_cleanup().
 * backend-bdb/hdb: fix cache segfault.
 * syncprov: fix possibility of loss changes.
 * syncprov: fix error handling in find-max/csn/present.
 * syncprov: fix 'missing present-list' bug.
 * syncprov: avoid lock-order-reversal/deadlock (search under si-ops mutex).
 * slapd: fix segfault in connection_write().
 * mdbx: ITS#8363 Fix off-by-one in mdb_midl_shrink().
 * mdbx: ITS#8355 fix subcursors.
 * syncprov: avoid deadlock with biglock and/or threadpool pausing.

### Minor bugs:
 * syncrepl: refine status-nofify for dead/dirty cases.
 * syncrepl: more o_dont_replicate for syncprov's mock status.
 * syncrepl: don't notify QS_DIRTY before obtain connection.
 * syncprov: refine matchops() for search-cleanup case.
 * slapd: fix valgrind-checks for sl-malloc.
 * liblber: fix hipagut support for realloc.
 * backend-ldap: fix/remove gentle-kick.
 * mdbx: workaround for pthread_setspecific's memleak.
 * mdbx: clarify mdbx_oomkick() for LMDB-mode.
 * syncrepl: ITS#8413 don't use str2filter on precomputable filters.
 * mdbx: always copy the rest of page (MDB_RESERVE case).
 * mdbx: fix nasty/stupid mistake in cmp-functions.
 * mdbx: ITS#8393 fix MDB_GET_BOTH on non-dup record.
 * slapd: request thread-pool pause only for SLAP_SERVER_MODE.
 * slapd: fix backover bug (since 532929a0776d47753377461dcf89ff38aba61779).
 * syncrepl: enforce csn/cookie while recovering lost-delete(s).
 * syncrepl: fix 'quorum' for mad configurations.
 * backend-mdb: fix mdb_opinfo_get() error handling.
 * syncrepl: fix 'limit-concurrent-refresh' feature.
 * slapd: ignore EBADF in epoll_ctl(DEL).
 * syncprov: fix rare assert-failure on race with abandon.
 * mdbx: fix mdb_kill_page() for MDB_PAGEPERTURB.
 * libldap: ITS#8385 Fix use-after-free with GnuTLS.
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
 * syncprov: ITS#8365 partially revert ITS#8281.
 * backend-mdb: more to avoid races on mi_numads.
 * backend-mdb: ITS#8360 fix ad info after failed txn.
 * backend-mdb: ITS#8226 limit size of read txns in searches.
 * slapd: cleanup bullshit around op->o_csn.
 * syncprov: wake waiting mod-ops when handle loop-pause.
 * syncprov: don't block mod-ops by waiting fetch-ops when pool-pause pending.
 * accesslog: ITS#8351 fix callback init.
 * syncprov: mutual fetch/modify - wakes opposite if waiting was broken.
 * syncrepl: fix race on cookieState->cs_ref.

### Performance:
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
 * mdbx: mdb_drop optimization.
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

### Build:
 * backend-mdb: ability to use old/origin liblmdb.
 * libldap: ITS#8353 more for OpenSSL 1.1.x compat.
 * libldap: ITS#8353 partial fixes (openssl 1.1.x).

### Cosmetics:
 * mdbx: reporting 'Unallocated' instead of 'Free pages'.
 * mdbx: reporting 'detaited'  instead of 'reading'.

### Other:
 * mdbx: be a bit more precise that mdb_get retrieves data (ITS#8386).
 * mdbx: adds MDBX_ALLOC_KICK for freelist backlog.
 * check MDB_RESERVE against MDB_DUPSORT.
 * syncrepl: dead/dirty/refresh/process/ready for quorum-status.
 * syncrepl: idclip-resync when present-list leftover after del_nonpresent().
 * syncrepl: fix missing strict-refresh in config-unparse.
 * syncprov: tracking for refresh-stage of sync-psearches.
 * syncprov: jammed & robust sync.
 * mdbx: adds MDB_PAGEPERTURB.

--------------------------------------------------------------------------------
## For news and changes early than 2016 please refer to [ChangeLog](ChangeLog)
