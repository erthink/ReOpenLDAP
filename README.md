ReOpenLDAP is...
=================
1. The fork of OpenLDAP
2. With a few new features, mostly for highload and multi-master clustering
3. With additional bug fixing and code quality improvement

*But no Windows, no Mac OS, no FreeBSD, just __only Linux__!*


The Changes
-----------------

Below is the list of changes from point-of-view of ReOpenLDAP project.
For description of the new features please see man-page for slapd.conf.
For a changes merged from OpenLDAP project please see the `CHANGES` file.

##### Features (major):
 * `reopenldap [iddqd] [idkfa] [idclip]`
 * `quorum { [vote-sids ...] [vote-rids ...] [auto-sids] [auto-rids] [require-sids ...] [require-rids ...] [all-links] }`
 * `quorum limit-concurrent-refresh`
 * `biglock { none | local | common }`
 * storage (mdbx): dreamcatcher & oom-handler (ITS#7974)
 * storage (mdbx): lifo & coalesce (ITS#7958)
 * storage (mdbx): steady & weak datasync

##### Features (minor):
 * `crash-backtrace on|off`
 * `coredump-limit <mbytes>`
 * `memory-limit <mbytes>`
 * checkpoints by volume-of-changes and periodically in seconds
 * support for LTO (Link-Time Optimization) by GCC/clang

##### Fixes:
 * all from openldap/master and openldap/2.4 branches
 * a lot for replication, especially for multi-master
 * removed ~5K warnings from GCC/clang
 * removed ~1K warnings from ThreadSanitizer (a data race detector)
 * removed most of memory leaks (tests could be passed under Valgrind)
