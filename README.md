ReOpenLDAP is...
=================
1. The fork of OpenLDAP
2. With a few new features, mostly for highload and multi-master clustering
3. With additional bug fixing and code quality improvement

*But no Windows, no Mac OS, no FreeBSD, no Solaris, no HP-UX, just __only Linux__!*


##### Currently ReOpenLDAP operates in telco industry throughout Russia:
 * few 2x2 multi-master clusters
 * up to ~100 millions records, up to ~100 Gb data
 * up to ~10K updates per second, up to ~25K searches.

Now no anyone other LDAP-server that could provide this
(replication fails, not reaches required performance,
or just crashes).


The Changes
-----------------

Below is the list of changes from point-of-view of ReOpenLDAP project.
For description of the new features please see man-page for slapd.conf.
For a changes merged from OpenLDAP project please see the `CHANGES` file.

#### Features (major):
 * multi-master replication is working, properly and robustly (seems no any other LDAP-server that could do this)
 * `reopenldap [iddqd] [idkfa] [idclip]`
 * `quorum { [vote-sids ...] [vote-rids ...] [auto-sids] [auto-rids] [require-sids ...] [require-rids ...] [all-links] }`
 * `quorum limit-concurrent-refresh`
 * `biglock { none | local | common }`
 * storage (mdbx): dreamcatcher & oom-handler (ITS#7974)
 * storage (mdbx): lifo & coalesce (ITS#7958)
 * storage (mdbx): steady & weak datasync

#### Features (minor):
 * `syncprov-showstatus { none | running | all }`
 * `crash-backtrace on|off`
 * `coredump-limit <mbytes>`
 * `memory-limit <mbytes>`
 * checkpoints by volume-of-changes and periodically in seconds
 * support for LTO (Link-Time Optimization) by GCC/clang

#### Fixes:
 * all from openldap/master and openldap/2.4 branches
 * a lot for replication, especially for multi-master
 * removed ~5K warnings from GCC/clang
 * removed ~1K warnings from ThreadSanitizer (a data race detector)
 * removed most of memory leaks (tests could be passed under Valgrind)
