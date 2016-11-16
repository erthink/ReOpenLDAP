#! /bin/sh
## $ReOpenLDAP$
## Copyright 1995-2016 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

while [ 1 ]; do
	read TAG VALUE
	if [ $? -ne 0 ]; then
		break
	fi
	case "$TAG" in
		base:)
		BASE=$VALUE
		;;
		filter:)
		FILTER=$VALUE
		;;
		# include other parameters here
	esac
done

LOGIN=`echo $FILTER | sed -e 's/.*=\(.*\))/\1/'`

PWLINE=`grep -i "^$LOGIN" /etc/passwd`

#sleep 60
# if we found an entry that matches
if [ $? = 0 ]; then
	echo $PWLINE | awk -F: '{
		printf("dn: cn=%s,%s\n", $1, base);
		printf("objectclass: top\n");
		printf("objectclass: person\n");
		printf("cn: %s\n", $1);
		printf("cn: %s\n", $5);
		printf("sn: %s\n", $1);
		printf("uid: %s\n", $1);
	}' base="$BASE"
	echo ""
fi

# result
echo "RESULT"
echo "code: 0"

exit 0
