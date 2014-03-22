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
 * licensing terms.z
 *
 * Author(s): Ericsson AB
 *
 */

/*****************************************************************************
..............................................................................

  $Header: 

..............................................................................

  DESCRIPTION:

 _Public_ IMMA abstractions and function prototypes

*******************************************************************************/

#ifndef IMMA_MDS_H
#define IMMA_MDS_H

EXTERN_C uns32 imma_mds_callback(struct ncsmds_callback_info *info);
EXTERN_C uns32 imma_mds_register(IMMA_CB *cb);
EXTERN_C void imma_mds_unregister(IMMA_CB *cb);
EXTERN_C uns32 imma_mds_callback(struct ncsmds_callback_info *info);
/* EXTERN_C uns32 imma_mds_msg_sync_send (NCSCONTEXT imma_mds_hdl, */
EXTERN_C uns32 imma_mds_msg_sync_send(uns32 imma_mds_hdl,
				      MDS_DEST *destination, IMMSV_EVT *i_evt, IMMSV_EVT **o_evt, uns32 timeout);
/* EXTERN_C uns32 imma_mds_msg_send (NCSCONTEXT imma_mds_hdl,  */
EXTERN_C uns32 imma_mds_msg_send(uns32 imma_mds_hdl, MDS_DEST *destination, IMMSV_EVT *i_evt, uns32 to_svc);

#endif   /* IMMA_DL_API_H */
