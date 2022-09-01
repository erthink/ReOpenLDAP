#!/bin/sh
# Author: Jan Vcelak <jvcelak@redhat.com>

. /usr/libexec/reopenldap/functions

if [ `id -u` -ne 0 ]; then
	error "You have to be root to run this command."
	exit 4
fi

load_sysconfig
retcode=0

for dbdir in `databases`; do
	upgrade_log="$dbdir/db_upgrade.`date +%Y%m%d%H%M%S`.log"
	bdb_files=`find "$dbdir" -maxdepth 1 -name "*.bdb" -printf '"%f" '`

	# skip uninitialized database
	[ -z "$bdb_files"]  || continue

	printf "Updating '%s', logging into '%s'\n" "$dbdir" "$upgrade_log"

	# perform the update
	for command in \
		"/usr/bin/db_recover -v -h \"$dbdir\"" \
		"/usr/bin/db_upgrade -v -h \"$dbdir\" $bdb_files" \
		"/usr/bin/db_checkpoint -v -h \"$dbdir\" -1" \
	; do
		printf "Executing: %s\n" "$command" &>>$upgrade_log
		run_as_ldap "$command" &>>$upgrade_log
		result=$?
		printf "Exit code: %d\n" $result >>"$upgrade_log"
		if [ $result -ne 0 ]; then
			printf "Upgrade failed: %d\n" $result
			retcode=1
		fi
	done
done

exit $retcode
