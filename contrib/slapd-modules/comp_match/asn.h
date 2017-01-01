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

/* ACKNOWLEDGEMENTS
 * This work originally developed by Sang Seok Lim
 * 2004/06/18	03:20:00	slim@OpenLDAP.org
 */

#ifndef _H_ASN_MODULE
#define _H_ASN_MODULE

#include <snacc/c/asn-incl.h>

typedef enum { BER, GSER } EncRulesType;

typedef enum AsnTypeId {
	BASICTYPE_BOOLEAN = 0,
	BASICTYPE_INTEGER,
	BASICTYPE_BITSTRING,
	BASICTYPE_OCTETSTRING,
	BASICTYPE_NULL,
	BASICTYPE_OID,
	BASICTYPE_REAL,
	BASICTYPE_ENUMERATED,
	BASICTYPE_NUMERIC_STR,
	BASICTYPE_PRINTABLE_STR,
	BASICTYPE_UNIVERSAL_STR,
	BASICTYPE_IA5_STR,
	BASICTYPE_BMP_STR,
	BASICTYPE_UTF8_STR,
	BASICTYPE_UTCTIME,
	BASICTYPE_GENERALIZEDTIME,
	BASICTYPE_GRAPHIC_STR,
	BASICTYPE_VISIBLE_STR,
	BASICTYPE_GENERAL_STR,
	BASICTYPE_OBJECTDESCRIPTOR,
	BASICTYPE_VIDEOTEX_STR,
	BASICTYPE_T61_STR,
	BASICTYPE_OCTETCONTAINING,
	BASICTYPE_BITCONTAINING,
	BASICTYPE_RELATIVE_OID,	/* 25 */
	BASICTYPE_ANY,
	/* Embedded Composite Types*/
	COMPOSITE_ASN1_TYPE,
	/* A New ASN.1 types including type reference */
	RDNSequence,
	RelativeDistinguishedName,
	TelephoneNumber,
	FacsimileTelephoneNumber__telephoneNumber,
	DirectoryString,
	/* Newly Defined ASN.1 Type, Manually registered */
	ASN_COMP_CERTIFICATE,
	/* ASN.1 Type End */
	ASNTYPE_END
} AsnTypeId;

typedef GeneralString BMPString;
typedef GeneralString UniversalString;
typedef GeneralString UTF8String;
typedef AsnOid AsnRelativeOid;

#endif
