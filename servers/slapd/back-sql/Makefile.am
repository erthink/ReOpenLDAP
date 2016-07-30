# Makefile.in for back-sql
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

pkglib_LTLIBRARIES =
noinst_LTLIBRARIES =
back_sql_la_LDFLAGS = -module
AM_CPPFLAGS = -DSLAPD_IMPORT -I$(srcdir)/.. -I$(top_srcdir)/include

if SQL_DYNAMIC
pkglib_LTLIBRARIES += back_sql.la
back_sql_la_LDFLAGS += -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
else
noinst_LTLIBRARIES += back_sql.la
endif

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name backend_sql --global 'backend-sql' > $@ || $(RM) $@
endif

back_sql_la_SOURCES = add.c api.c back-sql.h banner.c bind.c \
	compare.c config.c delete.c entry-id.c init.c modify.c modrdn.c \
	operational.c proto-sql.h schema-map.c search.c sql-wrap.c \
	util.c

AM_CPPFLAGS += $(SQL_INCLUDES)
back_sql_la_LIBADD = $(SQL_LIBS)

EXTRA_DIST = docs/install docs/platforms docs/bugs docs/todo \
	docs/concept rdbms_depend/README \
	rdbms_depend/pgsql/slapd.conf \
	rdbms_depend/pgsql/testdb_drop.sql \
	rdbms_depend/pgsql/testdb_metadata.sql \
	rdbms_depend/pgsql/testdb_create.sql \
	rdbms_depend/pgsql/testdb_data.sql \
	rdbms_depend/pgsql/backsql_create.sql \
	rdbms_depend/pgsql/backsql_drop.sql \
	rdbms_depend/mssql/slapd.conf \
	rdbms_depend/mssql/testdb_drop.sql \
	rdbms_depend/mssql/testdb_metadata.sql \
	rdbms_depend/mssql/testdb_create.sql \
	rdbms_depend/mssql/testdb_data.sql \
	rdbms_depend/mssql/backsql_create.sql \
	rdbms_depend/mssql/backsql_drop.sql \
	rdbms_depend/mysql/slapd.conf \
	rdbms_depend/mysql/testdb_drop.sql \
	rdbms_depend/mysql/testdb_metadata.sql \
	rdbms_depend/mysql/testdb_create.sql \
	rdbms_depend/mysql/testdb_data.sql \
	rdbms_depend/mysql/backsql_create.sql \
	rdbms_depend/mysql/backsql_drop.sql \
	rdbms_depend/timesten/slapd.conf \
	rdbms_depend/timesten/testdb_drop.sql \
	rdbms_depend/timesten/tttestdb_metadata.sql \
	rdbms_depend/timesten/ttcreate_schema.sh \
	rdbms_depend/timesten/tttestdb_drop.sql \
	rdbms_depend/timesten/tttestdb_data.sql \
	rdbms_depend/timesten/testdb_metadata.sql \
	rdbms_depend/timesten/testdb_create.sql \
	rdbms_depend/timesten/create_schema.sh \
	rdbms_depend/timesten/testdb_data.sql \
	rdbms_depend/timesten/backsql_create.sql \
	rdbms_depend/timesten/dnreverse/dnreverse.cpp \
	rdbms_depend/timesten/dnreverse/Makefile \
	rdbms_depend/timesten/backsql_drop.sql \
	rdbms_depend/timesten/tttestdb_create.sql \
	rdbms_depend/ibmdb2/slapd.conf \
	rdbms_depend/ibmdb2/testdb_drop.sql \
	rdbms_depend/ibmdb2/testdb_metadata.sql \
	rdbms_depend/ibmdb2/testdb_create.sql \
	rdbms_depend/ibmdb2/testdb_data.sql\
	rdbms_depend/ibmdb2/backsql_create.sql \
	rdbms_depend/ibmdb2/backsql_drop.sql \
	rdbms_depend/oracle/slapd.conf \
	rdbms_depend/oracle/testdb_drop.sql \
	rdbms_depend/oracle/testdb_metadata.sql \
	rdbms_depend/oracle/testdb_create.sql \
	rdbms_depend/oracle/testdb_data.sql \
	rdbms_depend/oracle/backsql_create.sql \
	rdbms_depend/oracle/backsql_drop.sql
