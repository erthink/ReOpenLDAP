## $ReOpenLDAP$
## Copyright 2016 ReOpenLDAP AUTHORS: please see AUTHORS file.
## All rights reserved.
##
## This file is part of ReOpenLDAP.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.
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
