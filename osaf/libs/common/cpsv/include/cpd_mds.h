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

 _Public_ CPD abstractions and function prototypes

*******************************************************************************/

/*
 * Module Inclusion Control...
 */

#ifndef CPD_MDS_H
#define CPD_MDS_H

EXTERN_C uns32 cpd_mds_callback(struct ncsmds_callback_info *info);
EXTERN_C uns32 cpd_mds_register(CPD_CB *cb);
EXTERN_C void cpd_mds_unregister(CPD_CB *cb);
EXTERN_C uns32 cpd_mds_callback(struct ncsmds_callback_info *info);
EXTERN_C uns32 cpd_mds_msg_sync_send(CPD_CB *cb, uns32 to_svc, MDS_DEST to_dest,
				     CPSV_EVT *i_evt, CPSV_EVT **o_evt, uns32 timeout);

EXTERN_C uns32 cpd_mds_msg_send(CPD_CB *cb, uns32 to_svc, MDS_DEST to_dest, CPSV_EVT *evt);

EXTERN_C uns32 cpd_mds_send_rsp(CPD_CB *cb, CPSV_SEND_INFO *s_info, CPSV_EVT *evt);

EXTERN_C uns32 cpd_mds_bcast_send(CPD_CB *cb, CPSV_EVT *evt, NCSMDS_SVC_ID to_svc);
EXTERN_C uns32 cpd_mds_vdest_create(CPD_CB *cb);

#endif   /* CPD_DL_API_H */
