#! /bin/sh
# $ReOpenLDAP$
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
## ---
##
## Copyright 1998-2014 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.

DIR=`dirname $0`
. $DIR/version.var

case "$ol_suffix" in
"")
	ol_type=Release
	;;
RC*|rc*)
	ol_type=ReleaseCandidate
	;;
X|x)
	ol_type=Engineering
	;;
*)
	ol_type=Devel
	;;
esac

ol_version=${ol_major}.${ol_minor}.${ol_patch}
ol_string="${ol_package} ${ol_version}-${ol_type}"
ol_api_lib_release=${ol_major}.${ol_minor}
ol_api_lib_version="${ol_api_current}:${ol_api_revision}:${ol_api_age}"

echo OL_PACKAGE=\"${ol_package}\"
echo OL_MAJOR=$ol_major
echo OL_MINOR=$ol_minor
echo OL_PATCH=$ol_patch
echo OL_API_INC=$ol_api_inc
echo OL_API_LIB_RELEASE=$ol_api_lib_release
echo OL_API_LIB_VERSION=$ol_api_lib_version
echo OL_VERSION=$ol_version
echo OL_TYPE=$ol_type
echo OL_STRING=\"${ol_string}\"
echo OL_RELEASE_DATE=\"${ol_release_date}\"
