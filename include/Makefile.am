## $ReOpenLDAP$
## Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
## Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
##
## This file is part of ReOpenLDAP.
##
## ReOpenLDAP is free software; you can redistribute it and/or modify it under
## the terms of the GNU Affero General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## ReOpenLDAP is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Affero General Public License for more details.
##
## You should have received a copy of the GNU Affero General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

pkginclude_HEADERS = lber.h ldap_cdefs.h ldap.h ldap_schema.h \
	ldap_utf8.h ldif.h slapi-plugin.h

noinst_HEADERS = ac/alloca.h ac/bytes.h ac/crypt.h ac/ctype.h \
	ac/dirent.h ac/errno.h ac/fdset.h ac/localize.h ac/param.h \
	ac/regex.h ac/setproctitle.h ac/signal.h ac/socket.h \
	ac/stdarg.h ac/stdlib.h ac/string.h ac/sysexits.h ac/syslog.h \
	ac/termios.h ac/time.h ac/unistd.h ac/wait.h avl.h \
	getopt-compat.h lber_hipagut.h lber_pvt.h ldap_defaults.h \
	ldap_int_thread.h ldap_log.h ldap_pvt.h ldap_pvt_thread.h \
	ldap_pvt_uc.h ldap_queue.h ldap_rq.h lutil.h lutil_hash.h \
	lutil_ldap.h lutil_lockf.h lutil_md5.h lutil_meter.h \
	lutil_sha1.h reldap.h rewrite.h sysexits-compat.h

BUILT_SOURCES = ldap_dirs.h
CLEANFILES = ldap_dirs.h
EXTRA_DIST = ldap_dirs.h.in

ldap_dirs.h: $(srcdir)/ldap_dirs.h.in Makefile
	@echo "Making $@" && $(RM) $@ && \
	echo "/* Generated from $< on `date --rfc-3339=seconds --utc` */" > $@ && \
	$(FIXUP_DIST) $< | $(FIXUP_DIRS) >> $@
