/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
..............................................................................

  MODULE NAME: NCS_ENC.C  

  REVISION HISTORY:

  Date     Version  Name          Description
  -------- -------  ------------  --------------------------------------------
  10-15-97 1.00A    H&J (DS)      Original

  $Id: 

  $Log:

..............................................................................

  DESCRIPTION:

  This module contains useful encoding function used during construction of
  LUNI 1.0 control frames.

..............................................................................

  FUNCTIONS INCLUDED in this module:

The following set does buffer chaining management
  ncs_encode_n_octets....Encode "n" octets in the control frame
  ncs_encode_uns16   ....Encode short
  ncs_encode_uns32   ....Encode long
  ncs_encode_uns64   ....Encode long long
  ncs_prepend_n_octets...Encode "n" octets encapsulating given frame
  ncs_prepend_uns16   ...Encode 16 bit unsigned encapsulating given frame
  ncs_prepend_uns32   ...Encode 32 bit unsigned encapsulating given frame
  ncs_prepend_uns64   ...Encode 64 bit unsigned encapsulating given frame

The following set does NOT do buffer chaining management
  ncs_encode_64bit   ....Encode 64 bits of a 64 bit value
  ncs_encode_32bit   ....Encode 32 bits of a 32 bit value
  ncs_encode_24bit   ....Encode 24 least significant octets of a 32 bit value
  ncs_encode_16bit   ....Encode 16 least significant octets of a 32 bit value
  ncs_encode_8bit    ....Encode  8 least significant octets of a 32 bit value
  ncs_encode_octets  ....Encode "n" octets encapsulating given frame

*******************************************************************************
*/

#include <ncsgl_defs.h>
#include "ncs_osprm.h"
#include "ncsencdec_pub.h"
#include "ncssysf_def.h"
#include "ncssysf_mem.h"
#include "ncs_svd.h"

/** A NULL os implies "count" number of zeros...
 **/

USRBUF *ncs_encode_n_octets(USRBUF *u, uns8 *os, unsigned int count)
{
	uns8 *p;
	unsigned int remaining = count;
	unsigned int offset = 0;

	if (remaining > PAYLOAD_BUF_SIZE)
		count = PAYLOAD_BUF_SIZE;
	else
		count = remaining;

	do {
		if ((p = m_MMGR_RESERVE_AT_END(&u, count, uns8 *)) != (uns8 *)0) {
			/*
			 * Build the octet string...Remember a NULL pointer
			 * indicates an all-zero octet-string...
			 */
			if (os == (uns8 *)0)
				memset((char *)p, '\0', (size_t)count);
			else
				memcpy((char *)p, (char *)(os + offset), (size_t)count);
		} else {
			break;
		}
		remaining = remaining - count;
		offset = offset + count;

		if (remaining > PAYLOAD_BUF_SIZE)
			count = PAYLOAD_BUF_SIZE;
		else
			count = remaining;

	} while (remaining > 0);
	return u;
}

USRBUF *ncs_encode_uns8(USRBUF *u, uns8 val8)
{
	uns8 *p8;

	if ((p8 = m_MMGR_RESERVE_AT_END(&u, 1, uns8 *)) != (uns8 *)0)
		*p8 = val8;

	return u;
}

USRBUF *ncs_encode_uns16(USRBUF *u, uns16 val16)
{
	uns8 *p16;

	if ((p16 = m_MMGR_RESERVE_AT_END(&u, 2, uns8 *)) != (uns8 *)0) {
		*p16++ = (uns8)(val16 >> 8);
		*p16 = (uns8)(val16);
	}

	return u;
}

USRBUF *ncs_encode_uns32(USRBUF *u, uns32 val32)
{
	uns8 *p32;

	if ((p32 = m_MMGR_RESERVE_AT_END(&u, 4, uns8 *)) != (uns8 *)0) {
		*p32++ = (uns8)(val32 >> 24);
		*p32++ = (uns8)(val32 >> 16);
		*p32++ = (uns8)(val32 >> 8);
		*p32 = (uns8)(val32);
	}

	return u;
}

USRBUF *ncs_encode_uns64(USRBUF *u, uns64 val64)
{
	uns8 *p64;

	if ((p64 = m_MMGR_RESERVE_AT_END(&u, 8, uns8 *)) != (uns8 *)0) {
		m_NCS_OS_HTONLL_P(p64, val64);
	}

	return u;
}

USRBUF *ncs_encode_float(USRBUF *u, ncsfloat32 obj_val)
{
	ncsfloat32 *pfloat;

	if ((pfloat = m_MMGR_RESERVE_AT_END(&u, (uns32)sizeof(obj_val), ncsfloat32 *)) != (ncsfloat32 *)0)
		 m_NCS_ENCODE_FLOAT(obj_val, pfloat);

	return u;
}

USRBUF *ncs_prepend_n_octets(USRBUF *pbuf, uns8 *os, unsigned int length)
{
	uns8 *pch;

	pch = m_MMGR_RESERVE_AT_START(&pbuf, length, uns8 *);
	if (pch == NULL) {
		m_LEAP_DBG_SINK((long)BNULL);
		return BNULL;
	}

	memcpy((char *)pch, (char *)os, (size_t)length);
	return (pbuf);
}

USRBUF *ncs_prepend_uns16(USRBUF *pbuf, uns16 val16)
{
	uns8 *p16;

	p16 = m_MMGR_RESERVE_AT_START(&pbuf, (uns32)sizeof(uns16), uns8 *);
	if (p16 == NULL) {
		m_LEAP_DBG_SINK((long)BNULL);
		return BNULL;
	}
	*p16++ = (uns8)(val16 >> 8);
	*p16 = (uns8)(val16);

	return (pbuf);
}

USRBUF *ncs_prepend_uns32(USRBUF *pbuf, uns32 val32)
{
	uns8 *p32;

	p32 = m_MMGR_RESERVE_AT_START(&pbuf, (uns32)sizeof(uns32), uns8 *);
	if (p32 == NULL) {
		m_LEAP_DBG_SINK((long)BNULL);
		return BNULL;
	}
	*p32++ = (uns8)(val32 >> 24);
	*p32++ = (uns8)(val32 >> 16);
	*p32++ = (uns8)(val32 >> 8);
	*p32 = (uns8)(val32);

	return (pbuf);
}

USRBUF *ncs_prepend_uns64(USRBUF *pbuf, uns64 val64)
{
	uns8 *p64;

	p64 = m_MMGR_RESERVE_AT_START(&pbuf, 8, uns8 *);
	if (p64 == NULL) {
		m_LEAP_DBG_SINK((long)BNULL);
		return BNULL;
	}

	m_NCS_OS_HTONLL_P(p64, val64);

	return (pbuf);
}

/*****************************************************************************
 *
 * Routines to encode 16, 24, 32, or 64 ints into network order in pdu ...
 *
 * "stream" points to the start of a n-octet  in the host order.
 * Network-order is the same as "big-endian" order (MSB first).
 * These functions has a built-in network-to-host order effect.
 *
 *****************************************************************************/

uns64 ncs_encode_64bit(uns8 **stream, uns64 val)
{

	m_NCS_OS_HTONLL_P((*stream), val);
	(*stream) += 8;
	return 8;
}

uns32 ncs_encode_32bit(uns8 **stream, uns32 val)
{
	*(*stream)++ = (uns8)(val >> 24);
	*(*stream)++ = (uns8)(val >> 16);
	*(*stream)++ = (uns8)(val >> 8);
	*(*stream)++ = (uns8)(val);
	return 4;
}

uns32 ncs_encode_24bit(uns8 **stream, uns32 val)
{
	*(*stream)++ = (uns8)(val >> 16);
	*(*stream)++ = (uns8)(val >> 8);
	*(*stream)++ = (uns8)(val);
	return 3;
}

uns32 ncs_encode_16bit(uns8 **stream, uns32 val)
{
	*(*stream)++ = (uns8)(val >> 8);
	*(*stream)++ = (uns8)(val);
	return 2;
}

uns32 ncs_encode_8bit(uns8 **stream, uns32 val)
{
	*(*stream)++ = (uns8)(val);
	return 1;
}

uns32 ncs_encode_key(uns8 **stream, NCS_KEY *key)
{
	uns8 len;

	*(*stream)++ = (uns8)(key->svc);
	*(*stream)++ = (uns8)(key->fmat);
	*(*stream)++ = (uns8)(key->type);

	switch (key->fmat) {
	case NCS_FMT_NUM:
		break;

	case NCS_FMT_STR:
		len = (uns8)strlen((char *)key->val.str);
		ncs_encode_8bit(stream, len);
		ncs_encode_octets(stream, key->val.str, len);
		return (3 + len + 1);
		break;

	case NCS_FMT_OCT:
		len = key->val.oct.len;
		ncs_encode_8bit(stream, len);
		ncs_encode_octets(stream, key->val.oct.data, len);
		return (3 + len + 1);
		break;
	}
	return m_LEAP_DBG_SINK(0);
}

uns32 ncs_encode_octets(uns8 **stream, uns8 *val, uns32 count)
{
	uns32 i;
	for (i = 0; i < count; i++)
		*(*stream)++ = *val++;
	return count;
}
