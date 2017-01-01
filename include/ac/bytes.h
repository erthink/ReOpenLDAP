/* $ReOpenLDAP$ */
/* Copyright 1992-2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
 * All rights reserved.
 *
 * This file is part of ReOpenLDAP.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#ifndef _AC_BYTES_H
#define _AC_BYTES_H

#ifndef BYTE_ORDER
/* cross compilers should define BYTE_ORDER in CPPFLAGS */

/*
 * Definitions for byte order, according to byte significance from low
 * address to high.
 */
#define LITTLE_ENDIAN   1234    /* LSB first: i386, vax */
#define BIG_ENDIAN  4321        /* MSB first: 68000, ibm, net */
#define PDP_ENDIAN  3412        /* LSB first in word, MSW first in long */

/* assume autoconf's AC_C_BIGENDIAN has been ran */
/* if it hasn't, we assume (maybe falsely) the order is LITTLE ENDIAN */
#	ifdef WORDS_BIGENDIAN
#		define BYTE_ORDER  BIG_ENDIAN
#	else
#		define BYTE_ORDER  LITTLE_ENDIAN
#	endif

#endif /* BYTE_ORDER */

#endif /* _AC_BYTES_H */
