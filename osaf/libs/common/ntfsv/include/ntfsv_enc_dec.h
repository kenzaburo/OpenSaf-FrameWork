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
 * Author(s): Ericsson AB
 *
 */

#ifndef NTFSV_ENC_DEC_H
#define NTFSV_ENC_DEC_H

#include <saNtf.h>
#include <ncsgl_defs.h>
#include <ncs_log.h>
#include <ncs_lib.h>
#include <mds_papi.h>
#include <ncs_mda_pvt.h>
#include <mbcsv_papi.h>
#include <ncs_edu_pub.h>
#include <ncs_util.h>
#include <logtrace.h>
#include <ntfsv_msg.h>

#ifdef  __cplusplus
extern "C" {
#endif
	uns32 ntfsv_enc_not_msg(NCS_UBAID *uba, ntfsv_send_not_req_t *param);
	uns32 ntfsv_enc_discard_msg(NCS_UBAID *uba, ntfsv_discarded_info_t *param);
	uns32 ntfsv_dec_not_msg(NCS_UBAID *uba, ntfsv_send_not_req_t *param);
	uns32 ntfsv_dec_discard_msg(NCS_UBAID *uba, ntfsv_discarded_info_t *param);
	uns32 ntfsv_enc_subscribe_msg(NCS_UBAID *uba, ntfsv_subscribe_req_t *param);
	uns32 ntfsv_dec_subscribe_msg(NCS_UBAID *uba, ntfsv_subscribe_req_t *param);
	uns32 ntfsv_enc_unsubscribe_msg(NCS_UBAID *uba, ntfsv_unsubscribe_req_t *param);
	uns32 ntfsv_dec_unsubscribe_msg(NCS_UBAID *uba, ntfsv_unsubscribe_req_t *param);
	uns32 ntfsv_enc_64bit_msg(NCS_UBAID *uba, uns64 param);
	uns32 ntfsv_dec_64bit_msg(NCS_UBAID *uba, uns64 *param);
	uns32 ntfsv_enc_32bit_msg(NCS_UBAID *uba, uns32 param);
	uns32 ntfsv_dec_32bit_msg(NCS_UBAID *uba, uns32 *param);

	void ntfsv_print_object_attributes(SaNtfAttributeT *objectAttributes, SaUint16T numAttributes);
#ifdef  __cplusplus
}
#endif

#endif   /* NTFSV_ENC_DEC_H */
