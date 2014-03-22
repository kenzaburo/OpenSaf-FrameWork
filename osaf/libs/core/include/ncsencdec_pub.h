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

  MODULE NAME:  NCS_ENDEC.H

  REVISION HISTORY:

  Date     Version  Name          Description
  -------- -------  ------------  --------------------------------------------
  07-22-97 1.00A    H&J (NSG)     Original

 * 
 * 14    4/04/01 10:36a Fengh
 * float to ncsfloat32
 * 
 * 13    10/24/00 11:53a Pseverin
 * 
 * 12    8/02/00 5:20p Stevem
 * Added encode/decode of NCS_KEY stuff.
 * 
 * 11    12/15/99 12:16p Ptutlian
 * remove warnings for the vxworks compiler
 * 
 * 10    12/02/99 5:28p Saula
 * Merge RSVP
 * 
 * 12    12/02/99 5:13p Saula
 * Merge from main line
 * 
 * 7     9/10/98 3:26p Nidhi
 * 
 * 5     9/03/98 5:56p Stevem
 * 
 * 4     8/13/98 6:13p Kchin
 * 
 * 1     6/03/98 3:15p Daha
 * New files for LEC 2.0 and MPC 1.0.
 * 
 * 9     5/21/98 2:54p Nsg
 * changes to compile using gcc without warnings...
 * 
 * 8     5/11/98 6:26p Daha
 * Changes to make gcc compile warning free.
 * 
 * 7     12/10/97 2:43p Bfox
 * Added ncs_cpy_mac_addr()
 * 
 * 6     12/02/97 10:20p Bfox
 * Added prototype for ncs_make_addr_from_string()
 * 
 * 5     11/20/97 7:39a Billh
 * 
 * 4     10/28/97 3:55p Billh
 * 
 * 3     10/24/97 8:16a Billh
 * 
 * 2     10/24/97 7:38a Billh
 * Newly added  common include files.
 * 
 * 1     10/24/97 7:37a Billh

..............................................................................

  DESCRIPTION:
  Function prototypes for utility encode/decode operations.

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef NCSENCDEC_PUB_H
#define NCSENCDEC_PUB_H

#include "ncsgl_defs.h"
#include "ncs_osprm.h"
#include "ncsusrbuf.h"
#include "ncs_svd.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * NCS_ENC encode functions
 */
	EXTERN_C USRBUF *ncs_encode_n_octets(USRBUF *, uns8 *, unsigned int);
	EXTERN_C USRBUF *ncs_encode_uns8(USRBUF *u, uns8 val8);
	EXTERN_C USRBUF *ncs_encode_uns16(USRBUF *u, uns16 val16);
	EXTERN_C USRBUF *ncs_encode_uns32(USRBUF *u, uns32 val32);
	EXTERN_C USRBUF *ncs_encode_uns64(USRBUF *u, uns64 val64);
	EXTERN_C USRBUF *ncs_prepend_n_octets(USRBUF *pbuf, uns8 *os, unsigned int);
	EXTERN_C USRBUF *ncs_prepend_uns16(USRBUF *u, uns16);
	EXTERN_C USRBUF *ncs_prepend_uns32(USRBUF *u, uns32);
	EXTERN_C USRBUF *ncs_prepend_uns64(USRBUF *u, uns64);
	EXTERN_C USRBUF *ncs_encode_float(USRBUF *u, ncsfloat32 obj_val);

	EXTERN_C uns64 ncs_encode_64bit(uns8 **stream, uns64);
	EXTERN_C uns32 ncs_encode_32bit(uns8 **stream, uns32);
	EXTERN_C uns32 ncs_encode_24bit(uns8 **stream, uns32);
	EXTERN_C uns32 ncs_encode_16bit(uns8 **stream, uns32);
	EXTERN_C uns32 ncs_encode_8bit(uns8 **stream, uns32);
	EXTERN_C uns32 ncs_encode_key(uns8 **stream, NCS_KEY *key);

	EXTERN_C uns32 ncs_encode_octets(uns8 **stream, uns8 *val, uns32 count);

/*
 * NCS_DEC decode functions
 */
	EXTERN_C USRBUF *ncs_decode_n_octets(USRBUF *, uns8 *, uns32);

	EXTERN_C USRBUF *ncs_skip_n_octets(USRBUF *, uns32);
	EXTERN_C uns8 *ncs_flatten_n_octets(USRBUF *u, uns8 *os, uns32 count);

	EXTERN_C uns32 ncs_decode_short(uns8 **stream);
	EXTERN_C uns32 ncs_decode_24bit(uns8 **stream);
	EXTERN_C uns32 ncs_decode_32bit(uns8 **stream);
	EXTERN_C uns16 ncs_decode_16bit(uns8 **stream);
	EXTERN_C uns64 ncs_decode_64bit(uns8 **stream);
	EXTERN_C uns8 ncs_decode_8bit(uns8 **stream);
	EXTERN_C uns32 ncs_decode_key(uns8 **stream, NCS_KEY *key);

	EXTERN_C ncsfloat32 ncs_decode_float(uns8 **stream);

/*****************************************************************************
 * STRUCTURE NAME:     USRFRAME
 *
 * DESCRIPTION:        This structure is an alternative to the userbuf
 *                     for passing data across the sysf_ip interface.
 * NOTES:
 * Header goes from *bufp to *(bufp+header_len-1)
 * data goes from *(bufp+header_len) to *(bufp+len-1)
 *
 * This used to be located in ncs_ip.h.  The USRFRAME is used outside of
 * sysf_ip and hence, must be located outside of ncs_ip.h
 *
 ****************************************************************************/
	typedef struct {
		unsigned int header_len;	/* header length */
		unsigned int len;	/* length of entire frame including header */
		char *bufp;	/* ptr to a (flat) buffer containing the frame */
	} USRFRAME;

/* encode ncsfloat32 */
#if defined(__NCSINC_PSOS__)
#define m_NCS_ENCODE_FLOAT(f, enc) {\
          *((uns32 *)(enc)) = htonl(*((uns32 *)&(f))); \
         }
#else
#define m_NCS_ENCODE_FLOAT(f, enc) {\
          *((uns32 *)(enc)) = htonl(*((uns32 *)&(f))); \
         }
#endif

/* decode ncsfloat32 */
#if defined(__NCSINC_PSOS__)
#define m_NCS_DECODE_FLOAT(n, dec) {\
          *((uns32 *) (dec)) = (n); \
         }
#else
#define m_NCS_DECODE_FLOAT(n, dec) {\
          *((uns32 *) (dec)) = (n); \
         }
#endif

#ifdef  __cplusplus
}
#endif

#endif
