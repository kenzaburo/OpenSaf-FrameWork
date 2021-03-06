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

 
..............................................................................

  DESCRIPTION:

  This module contains procedures related to...
  ncsmem_aid - specialized, simple heap/stack memory manager for limited cases.
  ncs_stack  - a general scheme to push and pop stack elements from a hunk of
              contiguous memory.

..............................................................................

  PUBLIC FUNCTIONS INCLUDED in this module:

  ncsmem_aid_init  - put NCSMEM_AID in start state, with a hunk of memory.
  ncsmem_aid_cpy   - copy data to NCSMEM_AID memory; update internal state
  ncsmem_aid_alloc - return block of memory; update internal state

  ncsstack_init    - Put NCS_STACK in start state
  ncsstack_peek    - Fetch next stack element, but don't pop it off stack.
  ncsstack_push    - Push a new stack element onto the stack
  ncsstack_pop     - Like 'ppek', but internal state updated

  PRIVATE FUNCTIONS INCLUDED in this module:

  get_top_se      - used by pop & peek to find top stack element.

******************************************************************************/

#include <ncsgl_defs.h>
#include "ncs_osprm.h"
#include "ncssysf_def.h"
#include "ncs_stack.h"
#include "ncsencdec_pub.h"

/*****************************************************************************

  PROCEDURE NAME:   ncsmem_aid_init

  DESCRIPTION:
    put the NCSMEM_AID in start state

  RETURNS:
*****************************************************************************/

void ncsmem_aid_init(NCSMEM_AID *ma, uns8 *space, uns32 len)
{
	ma->cur_ptr = space;
	ma->bgn_ptr = space;
	ma->max_len = len;
	ma->status = NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:   ncsmem_aid_cpy

  DESCRIPTION:
    copy stuff to the space overseen by the MEM_AID. Return NULL if it
    wont fit.

  RETURNS:
     NULL        - it wont fit
    PTR         - the ptr of where the copy lives.
*****************************************************************************/

uns8 *ncsmem_aid_cpy(NCSMEM_AID *ma, const uns8 *ref, uns32 len)
{
	uns8 *answer;

	if ((answer = ncsmem_aid_alloc(ma, len)) != NULL) {
		memcpy(answer, ref, len);
		return answer;
	}

	m_LEAP_DBG_SINK(0);
	return NULL;
}

/*****************************************************************************

  PROCEDURE NAME:   ncsmem_aid_alloc

  DESCRIPTION:
    allocate memory requested from contiguous space. Return NULL if there is
    not enough. Always allocate on 32 bit boundaries. 

  RETURNS:
     NULL        - it wont fit
    PTR         - the ptr of where the copy lives.
*****************************************************************************/

uns8 *ncsmem_aid_alloc(NCSMEM_AID *ma, uns32 size)
{
	uns8 *answer;

	size = size + (size % 4);	/* only allocate on even boundaries */
	if (ma->max_len > size) {
		answer = ma->cur_ptr;
		ma->cur_ptr = ma->cur_ptr + size;
		ma->max_len = ma->max_len - size;
		return answer;
	}
	ma->status = NCSCC_RC_FAILURE;
	m_LEAP_DBG_SINK(0);
	return NULL;
}

/*****************************************************************************

  PROCEDURE NAME: get_top_se

  DESCRIPTION:
  Find the Stack Element on the top of the stack. Do some sanity checking
  to make sure the stack frames are aligned. Complain if they are not.

  RETURNS:
     NULL - Trouble
     PTR  - To a stack frame

*****************************************************************************/
NCS_SE *get_top_se(NCS_STACK *st)
{
	uns16 *p_size = (uns16 *)((long)(st) + ((long)(st->cur_depth) - (long)(sizeof(uns16)) - (long)(sizeof(uns16))));
	uns16 size = *p_size++;

	if (*p_size != SE_ALIGNMENT_MARKER) {
		m_LEAP_DBG_SINK(0);
		return NULL;
	}

	return (NCS_SE *)((uns8 *)p_size - ((uns16)sizeof(uns16) + size));
}

/*****************************************************************************

  PROCEDURE NAME:    ncsstack_init

  DESCRIPTION:
  Put the ncs_stack into start state.
  stack.

  RETURNS:
     NONE

  NOTES:
     Invoker better know where the stack starts and what the max value is.
     This function believes whatever its told.

*****************************************************************************/

void ncsstack_init(NCS_STACK *st, uns16 max_size)
{
	st->se_cnt = 0;
	st->max_depth = max_size;
	st->cur_depth = sizeof(NCS_STACK);
}

/*****************************************************************************

  PROCEDURE NAME:    ncsstack_get_utilization

  DESCRIPTION:
  Fetch the percentage stack utilization

  RETURNS:
    Number of elements on stack

*****************************************************************************/

uns32 ncsstack_get_utilization(NCS_STACK *st)
{
	if (st->max_depth == 0)
		return 0;

	return ((st->cur_depth * 100) / (st->max_depth));
}

/*****************************************************************************

  PROCEDURE NAME:    ncsstack_get_element_count

  DESCRIPTION:
  Fetch the number of elements currently on the stack

  RETURNS:
    Number of elements on stack

*****************************************************************************/

uns32 ncsstack_get_element_count(NCS_STACK *st)
{
	return (st->se_cnt);
}

/*****************************************************************************

  PROCEDURE NAME:    ncsstack_peek

  DESCRIPTION:
  Fetch the pointer of the top Stack Element in this stack. The first 
  field is always assumed to be an NCS_SE.

  RETURNS:
    SUCCESS - fetched the next SE and put it in the passed container.
    FAILURE - There is no SE to fetch.

*****************************************************************************/

NCS_SE *ncsstack_peek(NCS_STACK *st)
{
	if (st->se_cnt == 0)
		return (NCS_SE *)NULL;

	return get_top_se(st);
}

/*****************************************************************************

  PROCEDURE NAME:    ncsstack_push

  DESCRIPTION:
  Push the passed SE on to the stack.

  RETURNS:
   SUCCESS - Pushed just fine
   FAILURE - Stack overflow

*****************************************************************************/

NCS_SE *ncsstack_push(NCS_STACK *st, uns16 type, uns16 size)
{
	NCS_SE *top;
	uns16 *len;

	if (st->max_depth <= (st->cur_depth + size + sizeof(uns16) + sizeof(uns16))) {
		m_LEAP_DBG_SINK(NULL);
		return NULL;
	}

	top = (NCS_SE *)((uns8 *)st + st->cur_depth);

	top->length = size;
	top->type = type;
	len = (uns16 *)((uns8 *)top + size);
	*len++ = size;
	*len = SE_ALIGNMENT_MARKER;

	st->se_cnt++;

	/* add SE size + 16bits of lenght + 16 bits of MARKER  to the cur_depth */

	st->cur_depth = (uns16)(st->cur_depth + size + sizeof(uns16) + sizeof(uns16));

	return top;
}

/*****************************************************************************

  PROCEDURE NAME:    ncsstack_pop

  DESCRIPTION:
     Find the top SE in the stack. Update all the other fields.

  RETURNS:
     NULL - Can't get the next SE
     PTR  - Got the next SE just fine.

*****************************************************************************/

NCS_SE *ncsstack_pop(NCS_STACK *st)
{
	NCS_SE *se;
	if ((se = ncsstack_peek(st)) == NULL)
		return (NCS_SE *)NULL;

	st->se_cnt--;
	st->cur_depth = (uns16)((uns8 *)se - (uns8 *)st);
	return se;
}

/*****************************************************************************

  PROCEDURE NAME:    ncsstack_encode

  DESCRIPTION:
  Encodes the whole stack.

  RETURNS:
   NCSCC_RC_SUCCESS - Encoded the stack just fine
   NCSCC_RC_FAILURE - Could not encode the stack

*****************************************************************************/

uns32 ncsstack_encode(NCS_STACK *st, struct ncs_ubaid *uba)
{
	uns16 cur_offset;
	uns16 cur_count;
	uns8 *stream;
	NCS_SE *top;
	uns16 *len;

	if (uba == NULL)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	stream = ncs_enc_reserve_space(uba, sizeof(uns16));
	if (stream == NULL)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	ncs_encode_16bit(&stream, st->se_cnt);
	ncs_enc_claim_space(uba, sizeof(uns16));

	stream = ncs_enc_reserve_space(uba, sizeof(uns16));
	if (stream == NULL)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	ncs_encode_16bit(&stream, st->max_depth);
	ncs_enc_claim_space(uba, sizeof(uns16));

	stream = ncs_enc_reserve_space(uba, sizeof(uns16));
	if (stream == NULL)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	ncs_encode_16bit(&stream, st->cur_depth);
	ncs_enc_claim_space(uba, sizeof(uns16));

	cur_count = 0;
	cur_offset = sizeof(NCS_STACK);

	/* now encode the space */
	while ((cur_offset < st->cur_depth) && (cur_count < st->se_cnt)) {

		top = (NCS_SE *)((uns8 *)st + cur_offset);

		/* Need to encode NCS_SE separately,
		   because the decode will decode them separately.
		   Do it to prevent any problems with structure member
		   alignments
		 */
		stream = ncs_enc_reserve_space(uba, sizeof(uns16));
		if (stream == NULL)
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

		ncs_encode_16bit(&stream, top->type);
		ncs_enc_claim_space(uba, sizeof(uns16));

		stream = ncs_enc_reserve_space(uba, sizeof(uns16));
		if (stream == NULL)
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

		ncs_encode_16bit(&stream, top->length);
		ncs_enc_claim_space(uba, sizeof(uns16));

		stream = ncs_enc_reserve_space(uba, (top->length - sizeof(NCS_SE)));
		if (stream == NULL)
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

		memcpy(stream, ((uns8 *)top + sizeof(NCS_SE)), (top->length - sizeof(NCS_SE)));
		ncs_enc_claim_space(uba, (top->length - sizeof(NCS_SE)));

		len = (uns16 *)((uns8 *)top + top->length);

		stream = ncs_enc_reserve_space(uba, sizeof(uns16));
		if (stream == NULL)
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

		ncs_encode_16bit(&stream, *len);
		ncs_enc_claim_space(uba, sizeof(uns16));

		len++;

		stream = ncs_enc_reserve_space(uba, sizeof(uns16));
		if (stream == NULL)
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

		ncs_encode_16bit(&stream, *len);
		ncs_enc_claim_space(uba, sizeof(uns16));

		cur_count++;
		cur_offset = (uns16)(cur_offset + top->length + sizeof(uns16) + sizeof(uns16));

	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:    ncsstack_decode

  DESCRIPTION:
  Decodes the whole stack.

  RETURNS:
   NCSCC_RC_SUCCESS - Decoded the stack just fine
   NCSCC_RC_FAILURE - Could not decode the stack

*****************************************************************************/

uns32 ncsstack_decode(NCS_STACK *st, NCS_UBAID *uba)
{
	uns16 cur_offset;
	uns16 cur_count;
	uns8 *stream;
	uns8 space[128];
	NCS_SE *top;
	uns16 *len;

	if (uba == NULL)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	stream = ncs_dec_flatten_space(uba, space, sizeof(uns16));
	st->se_cnt = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(uba, sizeof(uns16));

	stream = ncs_dec_flatten_space(uba, space, sizeof(uns16));
	st->max_depth = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(uba, sizeof(uns16));

	stream = ncs_dec_flatten_space(uba, space, sizeof(uns16));
	st->cur_depth = ncs_decode_16bit(&stream);
	ncs_dec_skip_space(uba, sizeof(uns16));

	cur_count = 0;
	cur_offset = sizeof(NCS_STACK);

	/* now decode the space */
	while ((cur_offset < st->cur_depth) && (cur_count < st->se_cnt)) {

		top = (NCS_SE *)((uns8 *)st + cur_offset);

		stream = ncs_dec_flatten_space(uba, space, sizeof(uns16));
		top->type = ncs_decode_16bit(&stream);
		ncs_dec_skip_space(uba, sizeof(uns16));

		stream = ncs_dec_flatten_space(uba, space, sizeof(uns16));
		top->length = ncs_decode_16bit(&stream);
		ncs_dec_skip_space(uba, sizeof(uns16));

		stream = ncs_dec_flatten_space(uba, space, (top->length - sizeof(NCS_SE)));
		memcpy(((uns8 *)top + sizeof(NCS_SE)), stream, (top->length - sizeof(NCS_SE)));
		ncs_dec_skip_space(uba, (top->length - sizeof(NCS_SE)));

		len = (uns16 *)((uns8 *)top + top->length);

		stream = ncs_dec_flatten_space(uba, space, sizeof(uns16));
		*len = ncs_decode_16bit(&stream);
		ncs_dec_skip_space(uba, sizeof(uns16));

		len++;

		stream = ncs_dec_flatten_space(uba, space, sizeof(uns16));
		*len = ncs_decode_16bit(&stream);
		ncs_dec_skip_space(uba, sizeof(uns16));

		cur_count++;
		cur_offset = (uns16)(cur_offset + top->length + sizeof(uns16) + sizeof(uns16));

	}

	return NCSCC_RC_SUCCESS;

}
