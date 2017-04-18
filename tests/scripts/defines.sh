#!/bin/bash
## $ReOpenLDAP$
## Copyright 1998-2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

# LY: kill all slapd running in the current session
pkill -SIGKILL -s 0 -u $EUID slapd
pkill -SIGKILL -s 0 -u $EUID lt-slapd
TESTWD=$(pwd)
umask 0002

# FIXME
function issue121_crutch() {
	echo "Sleep $1*$SLEEP1 seconds - as a workaround for https://github.com/ReOpen/ReOpenLDAP/issues/121 (FIXME)"
	local i
	for ((i=0; i<=$1; i++)); do
		sleep $SLEEP1
	done
}

#LY: man enabled overlays & backends
declare -A AC_conf
for i in $AC_SLAPD_OVERLAYS_LIST $AC_SLAPD_BACKENDS_LIST; do
	if grep -q " $i " <<< " $AC_SLAPD_DYNAMIC_OVERLAYS $AC_SLAPD_DYNAMIC_BACKENDS "; then
		AC_conf[$i]=mod
	elif grep -q " $i " <<< " $AC_SLAPD_STATIC_OVERLAYS $AC_SLAPD_STATIC_BACKENDS "; then
		AC_conf[$i]=yes
	else
		AC_conf[$i]=no
	fi
done

# misc
AC_conf[aci]=${AC_ACI_ENABLED-no}
AC_conf[tls]=${AC_WITH_TLS-no}
USE_SASL=${SLAPD_USE_SASL-no}
if [ $USE_SASL = no ]; then
	AC_conf[sasl]=no
else
	AC_conf[sasl]=${AC_WITH_SASL-no}
fi
TB="" TN=""
if test -t 1 ; then
	TB=`(tput bold; tput smul) 2>/dev/null`
	TN=`(tput rmul; tput sgr0) 2>/dev/null`
fi

# sql-backed
RDBMS=${SLAPD_USE_SQL-rdbmsno}
RDBMSWRITE=${SLAPD_USE_SQLWRITE-no}

function update_TESTDIR {
	TESTDIR=$1
	DBDIR1A=$TESTDIR/db.1.a
	DBDIR1B=$TESTDIR/db.1.b
	DBDIR1C=$TESTDIR/db.1.c
	DBDIR1=$DBDIR1A
	DBDIR2A=$TESTDIR/db.2.a
	DBDIR2B=$TESTDIR/db.2.b
	DBDIR2C=$TESTDIR/db.2.c
	DBDIR2=$DBDIR2A
	DBDIR3=$TESTDIR/db.3.a
	DBDIR4=$TESTDIR/db.4.a
	DBDIR5=$TESTDIR/db.5.a
	DBDIR6=$TESTDIR/db.6.a

	# generated files
	CONF1=$TESTDIR/slapd.1.conf
	CONF2=$TESTDIR/slapd.2.conf
	CONF3=$TESTDIR/slapd.3.conf
	CONF4=$TESTDIR/slapd.4.conf
	CONF5=$TESTDIR/slapd.5.conf
	CONF6=$TESTDIR/slapd.6.conf
	ADDCONF=$TESTDIR/slapadd.conf
	CONFLDIF=$TESTDIR/slapd-dynamic.ldif

	LOG1=$TESTDIR/slapd.1.log
	LOG2=$TESTDIR/slapd.2.log
	LOG3=$TESTDIR/slapd.3.log
	LOG4=$TESTDIR/slapd.4.log
	LOG5=$TESTDIR/slapd.5.log
	LOG6=$TESTDIR/slapd.6.log
	SLAPADDLOG1=$TESTDIR/slapadd.1.log
	SLURPLOG=$TESTDIR/slurp.log

	CONFIGPWF=$TESTDIR/configpw

	# generated outputs
	SEARCHOUT=$TESTDIR/ldapsearch.out
	SEARCHOUT2=$TESTDIR/ldapsearch2.out
	SEARCHFLT=$TESTDIR/ldapsearch.flt
	SEARCHFLT2=$TESTDIR/ldapsearch2.flt
	LDIFFLT=$TESTDIR/ldif.flt
	TESTOUT=$TESTDIR/test.out
	INITOUT=$TESTDIR/init.out
	SERVER1OUT=$TESTDIR/server1.out
	SERVER1FLT=$TESTDIR/server1.flt
	SERVER2OUT=$TESTDIR/server2.out
	SERVER2FLT=$TESTDIR/server2.flt
	SERVER3OUT=$TESTDIR/server3.out
	SERVER3FLT=$TESTDIR/server3.flt
	SERVER4OUT=$TESTDIR/server4.out
	SERVER4FLT=$TESTDIR/server4.flt
	SERVER5OUT=$TESTDIR/server5.out
	SERVER5FLT=$TESTDIR/server5.flt
	SERVER6OUT=$TESTDIR/server6.out
	SERVER6FLT=$TESTDIR/server6.flt

	MASTEROUT=$SERVER1OUT
	MASTERFLT=$SERVER1FLT
	SLAVEOUT=$SERVER2OUT
	SLAVE2OUT=$SERVER3OUT
	SLAVEFLT=$SERVER2FLT
	SLAVE2FLT=$SERVER3FLT

	MTREADOUT=$TESTDIR/mtread.out

	#detect_deadlocks=1
	export ASAN_OPTIONS="symbolize=1:allow_addr2line=1:report_globals=1:replace_str=1:replace_intrin=1:malloc_context_size=9:detect_leaks=${ASAN_DETECT_LEAKS-0}:abort_on_error=1:log_path=$TESTDIR/asan-log"
	export TSAN_OPTIONS="report_signal_unsafe=0:second_deadlock_stack=1:history_size=2:log_path=$TESTDIR/tsan-log"

	VALGRIND_OPTIONS="--fair-sched=yes --quiet --log-file=$TESTDIR/valgrind-log.%p \
		--error-markers=@ --num-callers=41 --errors-for-leak-kinds=definite \
		--gen-suppressions=no --track-origins=yes --show-leak-kinds=definite \
		--trace-children=yes --suppressions=$TESTWD/scripts/valgrind.supp \
		--error-exitcode=${VALGRIND_ERRCODE:-43}"

	VALGRIND_CMD=""
	VALGRIND_EX_CMD=""
	if [ "$AC_VALGRIND_ENABLED" = yes -a -n "$WITH_VALGRIND" ]; then
		if [ $WITH_VALGRIND -gt 0 ]; then
			VALGRIND_CMD="${AC_VALGRIND} --leak-check=full $VALGRIND_OPTIONS"
		fi
		if [ $WITH_VALGRIND -gt 2 ]; then
			VALGRIND_EX_CMD="${AC_VALGRIND} --leak-check=full $VALGRIND_OPTIONS"
		elif [ $WITH_VALGRIND -gt 1 ]; then
			VALGRIND_EX_CMD="${AC_VALGRIND} --leak-check=summary $VALGRIND_OPTIONS"
		fi
	fi
}

# dirs
PROGDIR=${TOP_BUILDDIR}/tests/progs
DATADIR=${USER_DATADIR-${TOP_SRCDIR}/tests/testdata}
BASE_TESTDIR=${USER_TESTDIR-$TESTWD/testrun}
update_TESTDIR $BASE_TESTDIR
SLAPD_SLAPD=${TOP_BUILDDIR}/servers/slapd/slapd
CLIENTDIR=${TOP_BUILDDIR}/clients/tools
DRAINDIR=${TOP_SRCDIR}/tests

SCHEMADIR=${USER_SCHEMADIR-./schema}
case "$SCHEMADIR" in
	./*)	ABS_SCHEMADIR="${TOP_BUILDDIR}/tests/$SCHEMADIR" ;;
	*)	ABS_SCHEMADIR="$SCHEMADIR" ;;
esac

SQLCONCURRENCYDIR=$DATADIR/sql-concurrency

# conf
CONF=$DATADIR/slapd.conf
CONFTWO=$DATADIR/slapd2.conf
CONF2DB=$DATADIR/slapd-2db.conf
MCONF=$DATADIR/slapd-master.conf
COMPCONF=$DATADIR/slapd-component.conf
PWCONF=$DATADIR/slapd-pw.conf
WHOAMICONF=$DATADIR/slapd-whoami.conf
ACLCONF=$DATADIR/slapd-acl.conf
RCONF=$DATADIR/slapd-referrals.conf
SRMASTERCONF=$DATADIR/slapd-syncrepl-master.conf
DSRMASTERCONF=$DATADIR/slapd-deltasync-master.conf
DSRSLAVECONF=$DATADIR/slapd-deltasync-slave.conf
PPOLICYCONF=$DATADIR/slapd-ppolicy.conf
PROXYCACHECONF=$DATADIR/slapd-pcache.conf
PROXYAUTHZCONF=$DATADIR/slapd-proxyauthz.conf
CACHEMASTERCONF=$DATADIR/slapd-cache-master.conf
PROXYAUTHZMASTERCONF=$DATADIR/slapd-cache-master-proxyauthz.conf
R1SRSLAVECONF=$DATADIR/slapd-syncrepl-slave-refresh1.conf
R2SRSLAVECONF=$DATADIR/slapd-syncrepl-slave-refresh2.conf
P1SRSLAVECONF=$DATADIR/slapd-syncrepl-slave-persist1.conf
P2SRSLAVECONF=$DATADIR/slapd-syncrepl-slave-persist2.conf
P3SRSLAVECONF=$DATADIR/slapd-syncrepl-slave-persist3.conf
REFSLAVECONF=$DATADIR/slapd-ref-slave.conf
SCHEMACONF=$DATADIR/slapd-schema.conf
GLUECONF=$DATADIR/slapd-glue.conf
REFINTCONF=$DATADIR/slapd-refint.conf
RETCODECONF=$DATADIR/slapd-retcode.conf
UNIQUECONF=$DATADIR/slapd-unique.conf
LIMITSCONF=$DATADIR/slapd-limits.conf
DNCONF=$DATADIR/slapd-dn.conf
EMPTYDNCONF=$DATADIR/slapd-emptydn.conf
IDASSERTCONF=$DATADIR/slapd-idassert.conf
LDAPGLUECONF1=$DATADIR/slapd-ldapglue.conf
LDAPGLUECONF2=$DATADIR/slapd-ldapgluepeople.conf
LDAPGLUECONF3=$DATADIR/slapd-ldapgluegroups.conf
RELAYCONF=$DATADIR/slapd-relay.conf
CHAINCONF1=$DATADIR/slapd-chain1.conf
CHAINCONF2=$DATADIR/slapd-chain2.conf
GLUESYNCCONF1=$DATADIR/slapd-glue-syncrepl1.conf
GLUESYNCCONF2=$DATADIR/slapd-glue-syncrepl2.conf
SQLCONF=$DATADIR/slapd-sql.conf
SQLSRMASTERCONF=$DATADIR/slapd-sql-syncrepl-master.conf
TRANSLUCENTLOCALCONF=$DATADIR/slapd-translucent-local.conf
TRANSLUCENTREMOTECONF=$DATADIR/slapd-translucent-remote.conf
METACONF=$DATADIR/slapd-meta.conf
METACONF1=$DATADIR/slapd-meta-target1.conf
METACONF2=$DATADIR/slapd-meta-target2.conf
GLUELDAPCONF=$DATADIR/slapd-glue-ldap.conf
ACICONF=$DATADIR/slapd-aci.conf
VALSORTCONF=$DATADIR/slapd-valsort.conf
DYNLISTCONF=$DATADIR/slapd-dynlist.conf
RSLAVECONF=$DATADIR/slapd-repl-slave-remote.conf
PLSRSLAVECONF=$DATADIR/slapd-syncrepl-slave-persist-ldap.conf
PLSRMASTERCONF=$DATADIR/slapd-syncrepl-multiproxy.conf
DDSCONF=$DATADIR/slapd-dds.conf
PASSWDCONF=$DATADIR/slapd-passwd.conf
UNDOCONF=$DATADIR/slapd-config-undo.conf
NAKEDCONF=$DATADIR/slapd-config-naked.conf
VALREGEXCONF=$DATADIR/slapd-valregex.conf
DYNAMICCONF=$DATADIR/slapd-dynamic.ldif


# args
TOOLARGS="-x $LDAP_TOOLARGS"
TOOLPROTO="-P 3"
SYNCREPL_RETRY="1 +"

if [ -n "$VALGRIND_CMD" ]; then
	# LY: extra time for Valgrind's virtualization
	TIMEOUT_S="timeout -s SIGXCPU 5m"
	TIMEOUT_L="timeout -s SIGXCPU 45m"
	TIMEOUT_H="timeout -s SIGXCPU 120m"
	SLEEP0=${SLEEP0-3}
	SLEEP1=${SLEEP1-15}
	SYNCREPL_WAIT=${SYNCREPL_WAIT-30}
	pkill -SIGKILL -s 0 -u $EUID memcheck-*
elif [ -n "$CIBUZZ_PID4" ]; then
	# LY: extra time for running on busy machine
	TIMEOUT_S="timeout -s SIGXCPU 3m"
	TIMEOUT_L="timeout -s SIGXCPU 10m"
	TIMEOUT_H="timeout -s SIGXCPU 30m"
	SLEEP0=${SLEEP0-1}
	SLEEP1=${SLEEP1-7}
	SYNCREPL_WAIT=${SYNCREPL_WAIT-30}
elif [ -n "${TEAMCITY_PROCESS_FLOW_ID}" -o -n "${TRAVIS_BUILD_ID}" ]; then
	# LY: under Teamcity, take in account ASAN/TSAN and nice
	TIMEOUT_S="timeout -s SIGXCPU 2m"
	TIMEOUT_L="timeout -s SIGXCPU 7m"
	TIMEOUT_H="timeout -s SIGXCPU 20m"
	SLEEP0=${SLEEP0-0.3}
	SLEEP1=${SLEEP1-2}
	SYNCREPL_WAIT=${SYNCREPL_WAIT-10}
else
	# LY: take in account -O0
	TIMEOUT_S="timeout -s SIGXCPU 1m"
	TIMEOUT_L="timeout -s SIGXCPU 5m"
	TIMEOUT_H="timeout -s SIGXCPU 15m"
	SLEEP0=${SLEEP0-0.1}
	SLEEP1=${SLEEP1-1}
	SYNCREPL_WAIT=${SYNCREPL_WAIT-5}
fi

SLAP_VERBOSE=${SLAP_VERBOSE-none}
SLAPADD="$TIMEOUT_S $VALGRIND_CMD $SLAPD_SLAPD -Ta -d $SLAP_VERBOSE"
SLAPCAT="$TIMEOUT_S $VALGRIND_CMD $SLAPD_SLAPD -Tc -d $SLAP_VERBOSE"
SLAPINDEX="$TIMEOUT_S $VALGRIND_CMD $SLAPD_SLAPD -Ti -d $SLAP_VERBOSE"
SLAPMODIFY="$TIMEOUT_S $VALGRIND_CMD $SLAPD_SLAPD -Tm -d $SLAP_VERBOSE"
SLAPPASSWD="$TIMEOUT_S $VALGRIND_CMD $SLAPD_SLAPD -Tpasswd"

unset DIFF_OPTIONS
SLAPDMTREAD=$PROGDIR/slapd_mtread
LVL=${SLAPD_DEBUG-sync,stats,args,trace,conns}
LOCALHOST=localhost
BASEPORT=${SLAPD_BASEPORT-9010}
# NOTE: -u/-c is not that portable...
DIFF="diff -i"
CMP="diff -i -Z"
BCMP="diff -iB"
CMPOUT=/dev/null
SLAPD="$TIMEOUT_L $VALGRIND_CMD $SLAPD_SLAPD -D -s0 -d $LVL"
SLAPD_HUGE="$TIMEOUT_H $VALGRIND_CMD $SLAPD_SLAPD -D -s0 -d $LVL"
LDAPPASSWD="$TIMEOUT_S $VALGRIND_EX_CMD $CLIENTDIR/ldappasswd $TOOLARGS"
LDAPSASLSEARCH="$TIMEOUT_S $VALGRIND_EX_CMD $CLIENTDIR/ldapsearch $TOOLPROTO $LDAP_TOOLARGS -LLL"
LDAPSEARCH="$TIMEOUT_S $VALGRIND_EX_CMD $CLIENTDIR/ldapsearch $TOOLPROTO $TOOLARGS -LLL"
LDAPRSEARCH="$TIMEOUT_S $VALGRIND_EX_CMD $CLIENTDIR/ldapsearch $TOOLPROTO $TOOLARGS"
LDAPDELETE="$TIMEOUT_S $VALGRIND_EX_CMD $CLIENTDIR/ldapdelete $TOOLPROTO $TOOLARGS"
LDAPMODIFY="$TIMEOUT_S $VALGRIND_EX_CMD $CLIENTDIR/ldapmodify $TOOLPROTO $TOOLARGS"
LDAPADD="$TIMEOUT_S $VALGRIND_EX_CMD $CLIENTDIR/ldapmodify -a $TOOLPROTO $TOOLARGS"
LDAPMODRDN="$TIMEOUT_S $VALGRIND_EX_CMD $CLIENTDIR/ldapmodrdn $TOOLPROTO $TOOLARGS"
LDAPWHOAMI="$TIMEOUT_S $VALGRIND_EX_CMD $CLIENTDIR/ldapwhoami $TOOLARGS"
LDAPCOMPARE="$TIMEOUT_S $VALGRIND_EX_CMD $CLIENTDIR/ldapcompare $TOOLARGS"
LDAPEXOP="$TIMEOUT_S $VALGRIND_EX_CMD $CLIENTDIR/ldapexop $TOOLARGS"
SLAPDTESTER="$TIMEOUT_H $PROGDIR/slapd_tester"

function ldif-filter-unwrap {
	grep -v ^== | $PROGDIR/ldif_filter "$@" | sed -n -e 'H; ${ x; s/\n//; s/\n //g; p}'
}

LDIFFILTER=ldif-filter-unwrap
PORT1=`expr $BASEPORT + 1`
PORT2=`expr $BASEPORT + 2`
PORT3=`expr $BASEPORT + 3`
PORT4=`expr $BASEPORT + 4`
PORT5=`expr $BASEPORT + 5`
PORT6=`expr $BASEPORT + 6`
URI1="ldap://${LOCALHOST}:$PORT1/"
URI2="ldap://${LOCALHOST}:$PORT2/"
URI3="ldap://${LOCALHOST}:$PORT3/"
URI4="ldap://${LOCALHOST}:$PORT4/"
URI5="ldap://${LOCALHOST}:$PORT5/"
URI6="ldap://${LOCALHOST}:$PORT6/"

# LDIF
LDIF=$DATADIR/test.ldif
LDIFADD1=$DATADIR/do_add.1
LDIFGLUED=$DATADIR/test-glued.ldif
LDIFORDERED=$DATADIR/test-ordered.ldif
LDIFORDEREDCP=$DATADIR/test-ordered-cp.ldif
LDIFORDEREDNOCP=$DATADIR/test-ordered-nocp.ldif
LDIFBASE=$DATADIR/test-base.ldif
LDIFPASSWD=$DATADIR/passwd.ldif
LDIFWHOAMI=$DATADIR/test-whoami.ldif
LDIFPASSWDOUT=$DATADIR/passwd-out.ldif
LDIFPPOLICY=$DATADIR/ppolicy.ldif
LDIFLANG=$DATADIR/test-lang.ldif
LDIFLANGOUT=$DATADIR/lang-out.ldif
LDIFREF=$DATADIR/referrals.ldif
LDIFREFINT=$DATADIR/test-refint.ldif
LDIFUNIQUE=$DATADIR/test-unique.ldif
LDIFLIMITS=$DATADIR/test-limits.ldif
LDIFDN=$DATADIR/test-dn.ldif
LDIFEMPTYDN1=$DATADIR/test-emptydn1.ldif
LDIFEMPTYDN2=$DATADIR/test-emptydn2.ldif
LDIFIDASSERT1=$DATADIR/test-idassert1.ldif
LDIFIDASSERT2=$DATADIR/test-idassert2.ldif
LDIFLDAPGLUE1=$DATADIR/test-ldapglue.ldif
LDIFLDAPGLUE2=$DATADIR/test-ldapgluepeople.ldif
LDIFLDAPGLUE3=$DATADIR/test-ldapgluegroups.ldif
LDIFCOMPMATCH=$DATADIR/test-compmatch.ldif
LDIFCHAIN1=$DATADIR/test-chain1.ldif
LDIFCHAIN2=$DATADIR/test-chain2.ldif
LDIFTRANSLUCENTDATA=$DATADIR/test-translucent-data.ldif
LDIFTRANSLUCENTCONFIG=$DATADIR/test-translucent-config.ldif
LDIFTRANSLUCENTADD=$DATADIR/test-translucent-add.ldif
LDIFTRANSLUCENTMERGED=$DATADIR/test-translucent-merged.ldif
LDIFMETA=$DATADIR/test-meta.ldif
LDIFVALSORT=$DATADIR/test-valsort.ldif
SQLADD=$DATADIR/sql-add.ldif
LDIFUNORDERED=$DATADIR/test-unordered.ldif
LDIFREORDERED=$DATADIR/test-reordered.ldif
LDIFMODIFY=$DATADIR/test-modify.ldif

# strings
MONITOR=""
REFDN="c=US"
BASEDN="dc=example,dc=com"
MANAGERDN="cn=Manager,$BASEDN"
UPDATEDN="cn=Replica,$BASEDN"
PASSWD=secret
BABSDN="cn=Barbara Jensen,ou=Information Technology DivisioN,ou=People,$BASEDN"
BJORNSDN="cn=Bjorn Jensen,ou=Information Technology DivisioN,ou=People,$BASEDN"
JAJDN="cn=James A Jones 1,ou=Alumni Association,ou=People,$BASEDN"
JOHNDDN="cn=John Doe,ou=Information Technology Division,ou=People,$BASEDN"
MELLIOTDN="cn=Mark Elliot,ou=Alumni Association,ou=People,$BASEDN"
REFINTDN="cn=Manager,o=refint"
RETCODEDN="ou=RetCodes,$BASEDN"
UNIQUEDN="cn=Manager,o=unique"
EMPTYDNDN="cn=Manager,c=US"
TRANSLUCENTROOT="o=translucent"
TRANSLUCENTUSER="ou=users,o=translucent"
TRANSLUCENTDN="uid=binder,o=translucent"
TRANSLUCENTPASSWD="bindtest"
METABASEDN="ou=Meta,$BASEDN"
METAMANAGERDN="cn=Manager,$METABASEDN"
VALSORTDN="cn=Manager,o=valsort"
VALSORTBASEDN="o=valsort"
MONITORDN="cn=Monitor"
OPERATIONSMONITORDN="cn=Operations,$MONITORDN"
CONNECTIONSMONITORDN="cn=Connections,$MONITORDN"
DATABASESMONITORDN="cn=Databases,$MONITORDN"
STATISTICSMONITORDN="cn=Statistics,$MONITORDN"

VALSORTOUT1=$DATADIR/valsort1.out
VALSORTOUT2=$DATADIR/valsort2.out
VALSORTOUT3=$DATADIR/valsort3.out
MONITOROUT1=$DATADIR/monitor1.out
MONITOROUT2=$DATADIR/monitor2.out
MONITOROUT3=$DATADIR/monitor3.out
MONITOROUT4=$DATADIR/monitor4.out


# original outputs for cmp
PROXYCACHEOUT=$DATADIR/pcache.out
REFERRALOUT=$DATADIR/referrals.out
SEARCHOUTMASTER=$DATADIR/search.out.master
SEARCHOUTX=$DATADIR/search.out.xsearch
COMPSEARCHOUT=$DATADIR/compsearch.out
MODIFYOUTMASTER=$DATADIR/modify.out.master
ADDDELOUTMASTER=$DATADIR/adddel.out.master
MODRDNOUTMASTER0=$DATADIR/modrdn.out.master.0
MODRDNOUTMASTER1=$DATADIR/modrdn.out.master.1
MODRDNOUTMASTER2=$DATADIR/modrdn.out.master.2
MODRDNOUTMASTER3=$DATADIR/modrdn.out.master.3
ACLOUTMASTER=$DATADIR/acl.out.master
REPLOUTMASTER=$DATADIR/repl.out.master
MODSRCHFILTERS=$DATADIR/modify.search.filters
CERTIFICATETLS=$DATADIR/certificate.tls
CERTIFICATEOUT=$DATADIR/certificate.out
DNOUT=$DATADIR/dn.out
EMPTYDNOUT1=$DATADIR/emptydn.out.slapadd
EMPTYDNOUT2=$DATADIR/emptydn.out
IDASSERTOUT=$DATADIR/idassert.out
LDAPGLUEOUT=$DATADIR/ldapglue.out
LDAPGLUEANONYMOUSOUT=$DATADIR/ldapglueanonymous.out
RELAYOUT=$DATADIR/relay.out
CHAINOUT=$DATADIR/chain.out
CHAINREFOUT=$DATADIR/chainref.out
CHAINMODOUT=$DATADIR/chainmod.out
GLUESYNCOUT=$DATADIR/gluesync.out
SQLREAD=$DATADIR/sql-read.out
SQLWRITE=$DATADIR/sql-write.out
TRANSLUCENTOUT=$DATADIR/translucent.search.out
METAOUT=$DATADIR/meta.out
METACONCURRENCYOUT=$DATADIR/metaconcurrency.out
MANAGEOUT=$DATADIR/manage.out
SUBTREERENAMEOUT=$DATADIR/subtree-rename.out
ACIOUT=$DATADIR/aci.out
DYNLISTOUT=$DATADIR/dynlist.out
DDSOUT=$DATADIR/dds.out
MEMBEROFOUT=$DATADIR/memberof.out
MEMBEROFREFINTOUT=$DATADIR/memberof-refint.out

function teamcity_msg {
	if [ -n "${TEAMCITY_PROCESS_FLOW_ID}" ]; then
		echo "##teamcity[$@]"
	fi
}

function teamcity_sleep {
	if [ -n "${TEAMCITY_PROCESS_FLOW_ID}" ]; then
		sleep $1
	fi
}

function safewait {
	wait "$@"
	local RC=$?
	if [ $RC -gt 132 ]; then
		echo " coredump/signal-$(($RC - 128))"
		sleep 5
		exit $RC
	fi
}

function killpids {
	if [ $# != 0 ]; then
		echo -n ">>>>> waiting for things ($@) to exit..."
		kill -HUP "$@" && safewait "$@"
		echo " done"
	fi
}

function killservers {
	if [ "$KILLSERVERS" != no -o -z "$KILLPIDS" ]; then
		killpids $KILLPIDS
		KILLPIDS=
	fi
}

function dumppids {
	if [ $# != 0 ]; then
		echo -n ">>>>> waiting for things ($@) to dump..."
		kill -SIGABRT "$@" && safewait "$@"
		echo " done"
	fi
}

function dumpservers {
	echo -n ">>>>> waiting for things ($(pgrep -s 0 slapd)) to dump..."
	(pkill -SIGABRT -s 0 slapd && sleep 5 && echo "done") || echo "fail"
}

function get_df_mb {
	local path=$1
	while [ -n "$path" ]; do
		local npath=$(readlink -q -e $path)
		if [ -n "$npath" ]; then
			local df_avail_k=$(df -k -P $npath | tail -1 | tr -s '\t ' ' ' | cut -d ' ' -f 4)
			if [ -n "$df_avail_k" ]; then
				echo $((df_avail_k / 1024))
				return
			fi
			break
		fi
		path=$(dirname $path)
	done
	echo 0
}

function cibuzz_report {
	if [ -n "$CIBUZZ_STATUS" ]; then
		echo "$(date --rfc-3339=seconds) $@" > $CIBUZZ_STATUS
	fi
}

function failure {
	cibuzz_report "failure: $@"
	echo "$@" >&2
	killservers
	exit 125
}

function safepath {
	local arg
	# LY: readlink/realpath from RHEL6 support only one filename.
	local buildroot=$(readlink -f $([ -n "${TESTING_ROOT}" ] && echo "${TESTING_ROOT}" || echo "${TOP_BUILDDIR}"))
	for arg in "$@"; do
		local r=$(realpath --relative-to ${buildroot} $arg 2>/dev/null | sed -e 's|/\./|/|g' -e 's|^\./||g')
		# LY: realpath from RHEL6 don't support '--relative-to' option,
		#     this is workaround/crutch.
		if [ -n "$r" ]; then
			echo $arg
		else
			readlink -f $arg | sed -e "s|^${buildroot}/||g"
		fi
	done
}

function collect_coredumps {
	wait
	local id=${1:-xxx-$(date '+%F.%H%M%S.%N')}
	find -L ${DRAINDIR} -type f -size 0 '(' -name 'valgrind-log*' -o -name 'tsan-log*' -o -name 'asan-log*' ')' -delete
	local cores="$(find -L ${DRAINDIR} -type f -size +0 '(' -name core -o -name 'valgrind-log*.core.*' ')' )"
	local sans="$(find -L ${DRAINDIR} -type f -size +0 '(' -name 'tsan-log*' -o -name 'asan-log*' ')' )"
	local vags="$(find -L ${DRAINDIR} -type f -size +0 '(' -regextype posix-egrep -regex '.*/valgrind-log.[0-9]+$' ')' )"
	local rc=0
	if [ -n "${cores}" -o -n "${sans}" -o -n "${vags}" ]; then
		if [ -n "${cores}" ]; then
			echo "Found some CORE(s): '$(safepath $cores)', collect it..." >&2
			local exe=${SLAPD_SLAPD}
			[ -x $(dirname $exe)/.libs/lt-slapd ] && exe=$(dirname $exe)/.libs/lt-slapd
		fi
		if [ -n "${sans}" ]; then
			echo "Found some TSAN/ASAN(s): '$(safepath $sans)', collect it..." >&2
		fi
		if [ -n "${vags}" ]; then
			echo "Found some VALGRIND(s): '$(safepath $vags)', collect it..." >&2
		fi

		local dir n c target
		if [ -n "${TEST_NOOK}" ]; then
			dir="${TOP_BUILDDIR}/${TEST_NOOK}"
		else
			dir="${TOP_BUILDDIR}/@cores-n-sans"
		fi

		mkdir -p "$dir" || failure "failed: mkdir -p '$dir'"
		n=
		for c in ${cores}; do
			while true; do
				target="${dir}/${id}${n}.core"
				if [ ! -e "${target}" ]; then break; fi
				n=$((n-1))
			done
			mv "$c" "${target}" || failure "failed: mv '$c' '${target}'"
			gdb -q $exe -c "${target}" << EOF |& tee -a "${target}-gdb" | sed -n '/[Cc]ore was generated by/,+42p' >&2
bt
info thread
thread apply all bt full
quit
EOF
			git show -s &>> "${target}-gdb"
		done

		n=
		for c in ${sans}; do
			git show -s &>> "$c"
			ext=$(sed -n 's/^.\+\.\([0-9]\+\)$/\1/p' <<< "$c")
			if [ -n "$ext" ]; then
				target="${dir}/${id}-${ext}.san"
			else
				while true; do
					target="${dir}/${id}${n}.san"
					if [ ! -e "${target}" ]; then break; fi
					n=$((n-1))
				done
			fi
			mv "$c" "${target}" || failure "failed: mv '$c' '${target}'"
		done

		local filtered_vags=0
		n=
		for c in ${vags}; do
			grep -i -e "invalid read" -e "invalid write" -e "_definitely lost" \
				-e "uninitialised value" -e "uninitialised bytes" -e "invalid free" \
				-e "mismatched free" -e "destination overlap" -e "has a fishy" "$c" \
				| sed -e 's/^/VALGRIND: /'
			if [ ${PIPESTATUS[0]} -ne 0 ]; then
				continue
			fi

			git show -s &>> "$c"
			ext=$(sed -n 's/^.\+\.\([0-9]\+\)$/\1/p' <<< "$c")
			if [ -n "$ext" ]; then
				target="${dir}/${id}-${ext}.vag"
			else
				while true; do
					target="${dir}/${id}${n}.vag"
					if [ ! -e "${target}" ]; then break; fi
					n=$((n-1))
				done
			fi
			mv "$c" "${target}" || failure "failed: mv '$c' '${target}'"
			filtered_vags=$((filtered_vags+1))
		done

		if [  ${filtered_vags} -gt 0 ]; then
			teamcity_msg "publishArtifacts '$(safepath ${dir})/${id}*.vag => ${id}-vags.tar.gz'"
			RC_EXT="valgrind"
			rc=1
		fi
		if [ -n "${sans}" ]; then
			teamcity_msg "publishArtifacts '$(safepath ${dir})/${id}*.san => ${id}-sans.tar.gz'"
			RC_EXT="tsan/asan"
			rc=1
		fi
		if [ -n "${cores}" ]; then
			teamcity_msg "publishArtifacts '$(safepath ${dir})/${id}*.core* => ${id}-cores.tar.gz'"
			RC_EXT="coredump"
			rc=1
		fi
		teamcity_sleep 1
	fi
	return $rc
}

function collect_test {
	wait
	local id=${1:-xxx-$(date '+%F.%H%M%S.%N')}
	local failed=${2:-yes}
	local from="${TOP_BUILDDIR}/tests/testrun"
	local status="${TOP_BUILDDIR}/@successful.log"

	if [ -n "$1" -a "$failed" = "no" -a -s $status ] && grep -q -- "$id" $status; then
		echo "Skipping a result(s) collecting of successful $id (already have)" >&2
		return
	fi

	if [ -n "$(ls $from)" ]; then
		echo "Collect result(s) from $id..." >&2

		local dir n target
		if [ -n "${TEST_NOOK}" ]; then
			dir="${TOP_BUILDDIR}/${TEST_NOOK}/$id.dump"
		else
			dir="${TOP_BUILDDIR}/@dumps/$id.dump"
		fi

		wanna_free=1000
		if [ "$failed" = "no" -a $(get_df_mb $(dirname $dir)) -lt $wanna_free ]; then
			echo "Skipping a result(s) collecting of successful $id (less than ${wanna_free}Mb space available)" >&2
			return
		fi

		n=
		while true; do
			target="${dir}/${n}"
			if [ ! -e "${target}" ]; then break; fi
			n=$((n+1))
		done

		mkdir -p "$target" || failure "failed: mkdir -p '$target'"
		mv -t "${target}" $from/* || failure "failed: mv -t '${target}' '$from/*'"

		if [ "${failed}" == "yes" ]; then
			teamcity_msg "publishArtifacts '$(safepath ${target}) => ${id}-dump.tar.gz'"
			teamcity_sleep 1
		else
			echo "$id	$(date '+%F.%H%M%S.%N')" >> $status
		fi
	fi
}

function wait_syncrepl {
	local scope=${3:-base}
	local base=${4}
	local extra=${5}
	local caption=""
	if [ -z "$base" ]; then base="$BASEDN"; else caption="$base "; fi
	echo -n "Waiting while syncrepl replicates a changes (${caption}between $1 and $2)..."
	sleep $SLEEP0
	local t=$SLEEP0

	while true; do

		#echo "  Using ldapsearch to read contextCSN from the provider:$1..."
		provider_csn=$($LDAPSEARCH -s $scope -b "$base" $extra -h $LOCALHOST -p $1 contextCSN -v \
			2>${TESTDIR}/ldapsearch.error | tee ${TESTDIR}/ldapsearch.output \
				|& grep -i ^contextCSN; exit ${PIPESTATUS[0]})
		RC=${PIPESTATUS[0]}
		if [ "$RC" -ne 0 ]; then
			echo "ldapsearch failed at provider ($RC, csn=$provider_csn)!"
			sleep 1
			local cores="$(find -L ${DRAINDIR} -type f -name core)"
			local sans="$(find -L ${DRAINDIR} -type f -name 'tsan-log*' -o -name 'asan-log*')"
			if [ -z "${cores}" -a -z "${sans}" ]; then
				dumpservers
			else
				killservers
			fi
			exit $RC
		fi

		#echo "  Using ldapsearch to read contextCSN from the consumer:$2..."
		consumer_csn=$($LDAPSEARCH -s $scope -b "$base" $extra -h $LOCALHOST -p $2 contextCSN -v \
			2>${TESTDIR}/ldapsearch.error | tee ${TESTDIR}/ldapsearch.output \
				|& grep -i ^contextCSN; exit ${PIPESTATUS[0]})
		RC=${PIPESTATUS[0]}
		if [ "$RC" -ne 0 -a "$RC" -ne 32 -a "$RC" -ne 34 ]; then
			echo "ldapsearch failed at consumer ($RC, csn=$consumer_csn)!"
			sleep 1
			local cores="$(find -L ${DRAINDIR} -type f -name core)"
			local sans="$(find -L ${DRAINDIR} -type f -name 'tsan-log*' -o -name 'asan-log*')"
			if [ -z "${cores}" -a -z "${sans}" ]; then
				dumpservers
			else
				killservers
			fi
			exit $RC
		fi

		if [ "$provider_csn" == "$consumer_csn" ]; then
			echo " Done in $t seconds"
			return
		fi

		if [ $(echo "$t > $SYNCREPL_WAIT" | bc -q) == "1" ]; then
			echo " Timeout $t seconds"
			echo -n "Provider: "
			$LDAPSEARCH -s $scope -b "$base" $extra -h $LOCALHOST -p $1 contextCSN
			echo -n "Consumer: "
			$LDAPSEARCH -s $scope -b "$base" $extra -h $LOCALHOST -p $2 contextCSN
			killservers
			exit 42
		fi

		#echo "  Waiting +SLEEP0 seconds for syncrepl to receive changes (from $1 to $2)..."
		sleep $SLEEP0
		t=$(echo "$SLEEP0 + $t" | bc -q)
	done
}

function check_running {
	local port=$(($BASEPORT + $1))
	local caption=$2
	local extra=$3
	local i
	if [ -n "$caption" ]; then caption+=" "; fi
	echo "Using ldapsearch to check that ${caption}slapd is running (port $port)..."
	for i in $SLEEP0 0.5 1 2 3 4 5 5; do
		echo "Waiting $i seconds for ${caption}slapd to start..."
		sleep $i
		$LDAPSEARCH $extra -s base -b "$MONITOR" -h $LOCALHOST -p $port \
			'(objectClass=*)' > /dev/null 2>&1
		RC=$?
		if test $RC = 0 ; then
			break
		fi
	done

	if test $RC != 0 ; then
		echo "ldapsearch failed! ($RC, $(date --rfc-3339=ns))"
		killservers
		exit $RC
	fi
}

function config_filter {
	if [ -z "$BACKEND" -o -z "$BACKENDTYPE" -o -z "$INDEXDB" -o -z "$MAINDB" ]; then
		echo "undefined BACKEND / BACKENDTYPE / INDEXDB / MAINDB" >&2
		exit 1
	fi

	local sasl_mech
	if [ $USE_SASL = no ] ; then
		sasl_mech=
	else
		if [ $USE_SASL = yes ] ; then
			USE_SASL=DIGEST-MD5
		fi
		sasl_mech="\"saslmech=$USE_SASL\""
	fi

	local monitor
	if [ ${AC_conf[monitor]} != no ]; then
		monitor=enabled
	else
		monitor=disabled
	fi

	sed -e "s/@BACKEND@/${BACKEND}/g"			\
		-e "s/^#be=${BACKEND}#//g"			\
		-e "s/^#be=${BACKEND},dbnosync=${DBNOSYNC:-no}#//g"\
		-e "/^#~/s/^#[^#]*~${BACKEND}~[^#]*#/#omit: /g"	\
			-e "s/^#~[^#]*~#//g"			\
		-e "s/@RELAY@/${RELAY}/g"			\
		-e "s/^#relay=${RELAY}#//g"			\
		-e "s/^#be-type=${BACKENDTYPE}#//g"		\
		-e "s/^#ldap=${AC_conf[ldap]}#//g"		\
		-e "s/^#meta=${AC_conf[meta]}#//g"		\
		-e "s/^#relay=${AC_conf[relay]}#//g"		\
		-e "s/^#sql=${AC_conf[sql]}#//g"		\
			-e "s/^#${RDBMS}#//g"			\
		-e "s/^#accesslog=${AC_conf[accesslog]}#//g"	\
		-e "s/^#autoca=${AC_conf[autoca]}#//g"		\
		-e "s/^#dds=${AC_conf[dds]}#//g"		\
		-e "s/^#dynlist=${AC_conf[dynlist]}#//g"	\
		-e "s/^#memberof=${AC_conf[memberof]}#//g"	\
		-e "s/^#pcache=${AC_conf[pcache]}#//g"	\
		-e "s/^#ppolicy=${AC_conf[ppolicy]}#//g"	\
		-e "s/^#refint=${AC_conf[refint]}#//g"		\
		-e "s/^#retcode=${AC_conf[retcode]}#//g"	\
		-e "s/^#rwm=${AC_conf[rwm]}#//g"		\
		-e "s/^#syncprov=${AC_conf[syncprov]}#//g"	\
		-e "s/^#translucent=${AC_conf[translucent]}#//g"\
		-e "s/^#unique=${AC_conf[unique]}#//g"		\
		-e "s/^#valsort=${AC_conf[valsort]}#//g"	\
		-e "s/^#${INDEXDB}#//g"				\
		-e "s/^#${MAINDB}#//g"				\
		-e "s/^#monitor=${AC_conf[monitor]}#//g"	\
		-e "s/^#monitor=${monitor}#//g"			\
		-e "s/^#sasl=${AC_conf[sasl]}#//g"		\
		-e "s/^#aci=${AC_conf[aci]}#//g"		\
		-e "s;@URI1@;${URI1};g"				\
		-e "s;@URI2@;${URI2};g"				\
		-e "s;@URI3@;${URI3};g"				\
		-e "s;@URI4@;${URI4};g"				\
		-e "s;@URI5@;${URI5};g"				\
		-e "s;@URI6@;${URI6};g"				\
		-e "s;@PORT1@;${PORT1};g"			\
		-e "s;@PORT2@;${PORT2};g"			\
		-e "s;@PORT3@;${PORT3};g"			\
		-e "s;@PORT4@;${PORT4};g"			\
		-e "s;@PORT5@;${PORT5};g"			\
		-e "s;@PORT6@;${PORT6};g"			\
		-e "s/@SASL_MECH@/${sasl_mech}/g"		\
		-e "s;@TESTDIR@;${TESTDIR};g"			\
		-e "s;@TESTWD@;${TESTWD};"			\
		-e "s;@DATADIR@;${DATADIR};g"			\
		-e "s;@SCHEMADIR@;${SCHEMADIR};g"
}

function monitor_data {
	[ $# = 2 ] || failure "monitor_data srcdir dstdir"

	local SRCDIR=$(readlink -e $1)
	local DSTDIR=$(readlink -e $2)

	echo "MONITORDB ${AC_conf[monitor]}"
	echo "SRCDIR $SRCDIR"
	echo "DSTDIR $DSTDIR"
	echo "pwd `pwd`"

	[ -d "$SRCDIR" -a -d "$DSTDIR" ] || failure "Invalid srcdir or dstdir"

	# copy test data
	cp "$SRCDIR"/do_* "$DSTDIR"
	if test ${AC_conf[monitor]} != no ; then

		# add back-monitor testing data
		cat >> "$DSTDIR/do_search.0" << EOF
cn=Monitor
(objectClass=*)
cn=Monitor
(objectClass=*)
cn=Monitor
(objectClass=*)
cn=Monitor
(objectClass=*)
EOF

		cat >> "$DSTDIR/do_read.0" << EOF
cn=Backend 1,cn=Backends,cn=Monitor
cn=Entries,cn=Statistics,cn=Monitor
cn=Database 1,cn=Databases,cn=Monitor
EOF

	fi
}

function run_testset {
	FAILCOUNT=0
	SKIPCOUNT=0
	SLEEPTIME=10
	RESULTS=$TESTWD/results

	BACKEND_MODE=$BACKEND
	if [ -n "$REOPENLDAP_MODE" ]; then
		BACKEND_MODE="$BACKEND_MODE/$REOPENLDAP_MODE"
	fi

	if [ -n "$VALGRIND_CMD" -a "$BACKEND" != mdb ]; then
		echo ">>>>> Skip ${TESTSET} for $BACKEND_MODE under Valgrind"
		exit 0
	fi

	teamcity_msg "testSuiteStarted name='${TESTSET} for $BACKEND_MODE'"
	echo ">>>>> Executing ${TESTSET} for $BACKEND_MODE"

	if [ -n "$NOEXIT" ]; then
		echo "Result	Test" > $RESULTS
	fi

	for CMD in $TEST_LIST_CMD; do

		TESTNO=$(sed -n 's/^.*\/test\([0-9]\+\)-.*/\1/p;s/^.*\/its\([0-9]\+\)/\1/p' <<< "$CMD")
		if [ 0"$TESTNO" -lt ${TESTNO_FROM:-0} ]; then continue; fi

		case "$CMD" in
			*~)	continue;;
			*.bak)	continue;;
			*.orig)	continue;;
			*.sav)	continue;;
			*)	test -f "$CMD" || continue;;
		esac

		# remove cruft from prior test
		if test $PRESERVE = yes ; then
			/bin/rm -rf $TESTDIR/db.*
		else
			find -H ${TESTDIR} -maxdepth 1 -mindepth 1 | xargs rm -rf || exit $?
		fi
		if test $BACKEND = ndb ; then
mysql --user root <<EOF
	drop database if exists db_1;
	drop database if exists db_2;
	drop database if exists db_3;
	drop database if exists db_4;
	drop database if exists db_5;
	drop database if exists db_6;
EOF
		fi

		BCMD=`basename $CMD`
		TEST_ID="$BCMD-$BACKEND"
		if [ -n "$REOPENLDAP_MODE" ]; then
			TEST_ID="$BCMD-$BACKEND-$REOPENLDAP_MODE"
		fi

		if [ -n "$SKIPLONG" ]; then
			if echo $TEST_ID | grep -q -e 008 -e 036 -e 039 -e 058 -e 060 -e 8444; then
				((SKIPCOUNT++))
				echo "***** Skip long ${TB}$BCMD${TN} for $BACKEND_MODE"
				echo
				teamcity_msg "testIgnored name='$TEST_ID'"
				continue
			fi
		fi

		if [ -n "$REPLONLY" ]; then
			if echo $BCMD | grep -q -v syncrepl; then
				((SKIPCOUNT++))
				echo "***** Skip non-syncrepl ${TB}$BCMD${TN} for $BACKEND_MODE"
				echo
				teamcity_msg "testIgnored name='$TEST_ID'"
				continue
			fi
		fi

		if [ -x "$CMD" ]; then
			wanna_free=1000
			if [ $(get_df_mb $TESTDIR) -lt $wanna_free ]; then
				echo "***** Less than ${wanna_free}Mb space available in the $TESTDIR"
				if [ -n "$NOEXIT" ]; then
					RC=2
					echo "(exit $RC)"
					exit $RC
				fi
				cibuzz_report "=== waiting for space in $TESTDIR"
				while [ $(get_df_mb $TESTDIR) -lt $wanna_free ]; do
					sleep 5
				done
			fi

			teamcity_msg "testStarted name='$TEST_ID' captureStandardOutput='true'"
			echo ">>>>> Starting ${TB}$BCMD${TN} for $BACKEND_MODE..."
			cibuzz_report ">>> ${TEST_ITER}--${TEST_ID}"

			$CMD 2>&1 | tee $TESTDIR/main.log
			RC=${PIPESTATUS[0]}
			if [ ! -f $TESTDIR/main.log ]; then
				echo "!!!!! Script deleted files, please fix it" >&2
				exit 125
			fi

			RC_EXT="test-failure"
			if ! collect_coredumps $TEST_ID; then
				RC=134
			fi

			if test $RC -eq 0 ; then
				cibuzz_report "<<< ${TEST_ITER}--${TEST_ID}"
				[ -z "$NO_COLLECT_SUCCESS" ] && collect_test $TEST_ID no
				echo "<<<<< $BCMD completed ${TB}OK${TN} for $BACKEND_MODE."
			else
				cibuzz_report "=== ${TEST_ITER}--${TEST_ID} = $RC"
				collect_test $TEST_ID yes
				echo "<<<<< $BCMD ${TB}failed${TN} for $BACKEND_MODE (code $RC, $RC_EXT)"
				teamcity_msg "testFailed name='$TEST_ID' message='code $RC, $RC_EXT'"
				((FAILCOUNT++))

				if [ -n "$NOEXIT" ]; then
					echo "Continuing."
				else
					exit $RC
				fi
			fi
			teamcity_msg "testFinished name='$TEST_ID'"
		else
			echo "***** Skipping ${TB}$BCMD${TN} for $BACKEND_MODE."
			((SKIPCOUNT++))
			RC="-"
			teamcity_msg "testIgnored name='$TEST_ID'"
		fi

		if [ -n "$NOEXIT" ]; then
			echo "$RC	$TEST_ID" >> $RESULTS
		fi
		rm -rf $TESTDIR/* || failure "failed: cleanup $$TESTDIR/*"

	#	echo ">>>>> waiting $SLEEPTIME seconds for things to exit"
	#	sleep $SLEEPTIME
		echo ""
	done

	RC=0
	if [ -n "$NOEXIT" ]; then
		if [ "$FAILCOUNT" -gt 0 ]; then
			cat $RESULTS
			echo "$FAILCOUNT tests for $BACKEND_MODE ${TB}failed${TN}. Please review the test log."
			RC=2
		else
			echo "${TESTSET} for $BACKEND_MODE ${TB}succeeded${TN}."
		fi
	fi

	echo "$SKIPCOUNT tests for $BACKEND_MODE were ${TB}skipped${TN}."
	teamcity_msg "testSuiteFinished name='${TESTSET} for $BACKEND_MODE'"
	exit $RC
}
