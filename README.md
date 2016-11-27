ReOpenLDAP is...
=================
1. Also known as "TelcoLDAP" - is the telco-oriented fork of OpenLDAP.
2. A number of new features, most of which deal with highload and multi-master clustering.
3. Bug fixing and code quality improvement.

*But no Windows, Mac OS, FreeBSD, Solaris or HP-UX; just __only Linux__!*


##### ReOpenLDAP is currently running in telcos across Russia:
 * Several 2x2 multi-master clusters
 * Up to ~100 million records, up to ~100 GB of data
 * Up to ~10K updates per second, up to ~25K searches

No other LDAP server can provide this level of performance now
(due to replication failures, inadequate performance
or high risk of a crash).


ReOpenLDAP preamble
-------------------

We consider that the original OpenLDAP project was failed
due the following reasons:
 - disregard for support by community and code clarity.
 - giant technical debt and incredible low code quality.
 - unreasonable desire to support the wide range of
   an obsolete archaic systems and compilers.


Change List
-----------------

Below is a list of changes made within the ReOpenLDAP project.
For a description of the new features, please see the man page for slapd.conf.
For the changes merged from OpenLDAP project, please see the CHANGES.OpenLDAP file.

#### Features (major):
 * multi-master replication is working properly and robustly (it seems no other LDAP server can do this)
 * `reopenldap [iddqd] [idkfa] [idclip]`
 * `quorum { [vote-sids ...] [vote-rids ...] [auto-sids] [auto-rids] [require-sids ...] [require-rids ...] [all-links] }`
 * `quorum limit-concurrent-refresh`
 * `biglock { none | local | common }`
 * storage (mdbx): dreamcatcher & oom-handler (ITS#7974)
 * storage (mdbx): lifo & coalesce (ITS#7958)
 * storage (mdbx): steady & weak datasync
 * bundled with all known contributed modules/overlays/plugins

#### Features (minor):
 * `syncprov-showstatus { none | running | all }`
 * `crash-backtrace on|off`
 * `coredump-limit <mbytes>`
 * `memory-limit <mbytes>`
 * checkpoints by volume-of-changes and periodically in seconds
 * syncrepl's `requirecheckpresent` option
 * `keepalive <idle>:<probes>:<interval>` for incoming connections
 * built-in memory checker, including ls-malloc
 * ready for AddressSanitizer and Valgrind
 * ready for LTO (Link-Time Optimization) by GCC and clang
 * support for OpenSSL 1.1.x, Mozilla NSS, GnuTLS and LibreSSL 2.5.x

#### Fixes:
 * all from openldap/master and openldap/2.4 branches
 * a lot for replication, especially for multi-master
 * removed ~5K warnings from GCC/clang
 * removed ~1K warnings from ThreadSanitizer (a data race detector)
 * removed most of memory leaks (tests could be passed under Valgrind)
 * checked with PVS-Studio static analyser, [see details](https://github.com/ReOpen/ReOpenLDAP/issues/107)
 * <a href="https://scan.coverity.com/projects/reopen-reopenldap"><img alt="Coverity Scan Build Status" src="https://scan.coverity.com/projects/6972/badge.svg"/></a>
