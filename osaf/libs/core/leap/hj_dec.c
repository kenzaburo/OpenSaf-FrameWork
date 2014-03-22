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

  MODULE NAME: NCS_DEC.C  

  REVISION HISTORY:

  Date     Version  Name          Description
  -------- -------  ------------  --------------------------------------------
  10-15-97 1.00A    H&J (DS)      Original

  $Id: 

  $Log:

..............................................................................

  DESCRIPTION:

  This module contains useful decoding function used during parsing of
  LUNI 1.0 control frames.

..............................................................................

  FUNCTIONS INCLUDED in this module:

The following set does buffer chaining management
  ncs_decode_n_octets(USRBUF *u, uns8 *os, uns32 count)
  ncs_skip_n_octets( USRBUF *u, uns32 count)

The following set does NOT do buffer chaining management
  ncs_decode_short( uns8 **stream)
  ncs_decode_24bit( uns8 **stream)
  ncs_decode_32bit( uns8 **stream)

*******************************************************************************
*/

#include <ncsgl_defs.h>
#include "ncssysf_mem.h"
#include "ncssysf_def.h"
#include "ncs_svd.h"
#include "ncsencdec_pub.h"

USRBUF *ncs_decode_n_octets(USRBUF *u, uns8 *os, uns32 count)
{

	char *s;

  /** Xtract "count" # of octets from the packet...
   **/
	if ((s = m_MMGR_DATA_AT_START(u, count, (char *)os)) != (char *)os) {
		if (s == 0) {
			m_LEAP_DBG_SINK(0);
			return (USRBUF *)0;
		}
		memcpy(os, s, (size_t)count);
	}

  /** Strip off "count" number of octets from the packet...
   **/
	m_MMGR_REMOVE_FROM_START(&u, count);
	return u;		/* Return the new head */

}

uns8 *ncs_flatten_n_octets(USRBUF *u, uns8 *os, uns32 count)
{
	if (u == BNULL) {
		m_LEAP_DBG_SINK(0);
		return NULL;
	}

	return (uns8 *)m_MMGR_DATA_AT_START(u, count, (char *)os);
}

USRBUF *ncs_skip_n_octets(USRBUF *u, uns32 count)
{

  /** Strip off "count" number of octets from the packet...
   **/
	m_MMGR_REMOVE_FROM_START(&u, count);
	return u;		/* Return the new head */

}

/*****************************************************************************

  PROCEDURE NAME:    ncs_decode_short

  DESCRIPTION:

  Decode a 2-octet *NON-0/1-EXT-ENCODED* field.

  ARGUMENTS:

 stream:  Address of a pointer to streamed data.

  RETURNS:

  NOTES:

  "stream" points to the start of 2-octet unsigned short in the network order.
  Network-order is the same as "big-endian" order (MSB first).
  This function has a built-in network-to-host order effect.

*****************************************************************************/
uns32 ncs_decode_short(uns8 **stream)
{

	uns32 val = 0;		/* Accumulator */

	val = (uns32)*(*stream)++ << 8;
	val |= (uns32)*(*stream)++;

	return (val & 0x0000FFFF);

}

/*****************************************************************************

  PROCEDURE NAME:    ncs_decode_24bit

  DESCRIPTION:

  Decode a 3-octet *NON-0/1-EXT-ENCODED* field.

  ARGUMENTS:

 stream:  Address of a pointer to streamed data.

  RETURNS:

  NOTES:

  "stream" points to the start of 3-octet unsigned value in the network order.
  Network-order is the same as "big-endian" order (MSB first).
  This function has a built-in network-to-host order effect.

*****************************************************************************/
uns32 ncs_decode_24bit(uns8 **stream)
{

	uns32 val = 0;		/* Accumulator */

	val = (uns32)*(*stream)++ << 16;
	val |= (uns32)*(*stream)++ << 8;
	val |= (uns32)*(*stream)++;

	return (val & 0x00FFFFFF);

}

/*****************************************************************************

  PROCEDURE NAME:    ncs_decode_32bit

  DESCRIPTION:

  Decode a 4-octet *NON-0/1-EXT-ENCODED* field.

  ARGUMENTS:

 stream:  Address of a pointer to streamed data.

  RETURNS:

  NOTES:

  "stream" points to the start of 4-octet unsigned value in the network order.
  Network-order is the same as "big-endian" order (MSB first).
  This function has a built-in network-to-host order effect.

*****************************************************************************/
uns32 ncs_decode_32bit(uns8 **stream)
{

	uns32 val = 0;		/* Accumulator */

	val = (uns32)*(*stream)++ << 24;
	val |= (uns32)*(*stream)++ << 16;
	val |= (uns32)*(*stream)++ << 8;
	val |= (uns32)*(*stream)++;

	return val;

}

/*****************************************************************************

  PROCEDURE NAME:    ncs_decode_64bit

  DESCRIPTION:

  Decode a 8-octet *NON-0/1-EXT-ENCODED* field.

  ARGUMENTS:

 stream:  Address of a pointer to streamed data.

  RETURNS:

  NOTES:

  "stream" points to the start of 8-octet unsigned value in the network order.
  Network-order is the same as "big-endian" order (MSB first).
  This function has a built-in network-to-host order effect.

*****************************************************************************/
uns64 ncs_decode_64bit(uns8 **stream)
{
	uns64 val;

	val = m_NCS_OS_NTOHLL_P((*stream));
	(*stream) += 8;

	return val;
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_decode_16bit

  DESCRIPTION:

  Decode a 2-octet *NON-0/1-EXT-ENCODED* field.

  ARGUMENTS:

 stream:  Address of a pointer to streamed data.

  RETURNS:

  NOTES:

  "stream" points to the start of 2-octet unsigned short in the network order.
  Network-order is the same as "big-endian" order (MSB first).
  This function has a built-in network-to-host order effect.

*****************************************************************************/
uns16 ncs_decode_16bit(uns8 **stream)
{

	uns32 val = 0;		/* Accumulator */

	val = (uns32)*(*stream)++ << 8;
	val |= (uns32)*(*stream)++;

	return (uns16)(val & 0x0000FFFF);

}

/*****************************************************************************

  PROCEDURE NAME:    ncs_decode_8bit

  DESCRIPTION:

  Decode a 2-octet *NON-0/1-EXT-ENCODED* field.

  ARGUMENTS:

 stream:  Address of a pointer to streamed data.

  RETURNS:

  NOTES:

  "stream" points to the start of 2-octet unsigned short in the network order.
  Network-order is the same as "big-endian" order (MSB first).
  This function has a built-in network-to-host order effect.

*****************************************************************************/
uns8 ncs_decode_8bit(uns8 **stream)
{

	uns32 val = 0;		/* Accumulator */

	val = (uns32)*(*stream)++;

	return (uns8)(val & 0x000000FF);

}

/*****************************************************************************

  PROCEDURE NAME:    ncs_decode_key

  DESCRIPTION:

  Decode an NCS_KEY. Put it in the passed NCS_KEY structure.

  ARGUMENTS:

 stream:  Address of a pointer to streamed data.
  key   :   An NCS_KEY that is to get the values.

  RETURNS:

  count of number of octets consumed in 'stream'. The 'stream' pointer is
  updated past the consumed data used to populate the NCS_KEY.

  NOTES:

  "stream" points to the start of an encoded NCS_KEY structure that has
  been encoded as pwe ncs_encode_key().

*****************************************************************************/

uns32 ncs_decode_key(uns8 **stream, NCS_KEY *key)
{
	uns8 len;

	key->svc = *(*stream)++;
	key->fmat = *(*stream)++;
	key->type = *(*stream)++;

	switch (key->fmat) {
	case NCS_FMT_NUM:
		key->val.num = ncs_decode_32bit(stream);
		return 7;
		break;
	case NCS_FMT_STR:
		len = *(*stream)++;
		m_KEY_CHK_LEN(key->val.oct.len);
		strncpy((char *)key->val.str, (char *)(*stream), len);
		*stream = *stream + len;	/* move pointer beyond what was consumed */
		return (3 + len + 1);
		break;

	case NCS_FMT_OCT:
		len = *(*stream)++;
		m_KEY_CHK_LEN(key->val.oct.len);
		key->val.oct.len = len;
		memcpy(key->val.oct.data, *stream, len);
		*stream = *stream + len;	/* move pointer beyond what was consumed */
		return (3 + len + 1);
		break;
	}
	return m_LEAP_DBG_SINK(0);
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_decode_float

  DESCRIPTION:

  Decode a Float value.

  ARGUMENTS:

 stream:  Address of a pointer to streamed data.

  RETURNS:

  NOTES:

  "stream" points to the start of float value in the network order.
  Network-order is the same as "big-endian" order (MSB first).
  Convert a 4 byte IEEE Float object to host format.

*****************************************************************************/
ncsfloat32 ncs_decode_float(uns8 **stream)
{
	uns32 val;
	ncsfloat32 ret_val = 0;

	val = (uns32)*(*stream)++ << 24;
	val |= (uns32)*(*stream)++ << 16;
	val |= (uns32)*(*stream)++ << 8;
	val |= (uns32)*(*stream)++;
	m_NCS_DECODE_FLOAT(val, &ret_val);
	return ret_val;
}
