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

#ifndef MQND_MDS_H
#define MQND_MDS_H

EXTERN_C uns32 mqnd_mds_register(MQND_CB *cb);

EXTERN_C void mqnd_mds_unregister(MQND_CB *cb);

EXTERN_C uns32 mqnd_mds_send_rsp(MQND_CB *cb, MQSV_SEND_INFO *s_info, MQSV_EVT *evt);

EXTERN_C uns32 mqnd_mds_send_rsp_direct(MQND_CB *cb, MQSV_DSEND_INFO *s_info, MQSV_DSEND_EVT *evt);

EXTERN_C uns32 mqnd_mds_msg_sync_send(MQND_CB *cb, uns32 to_svc,
				      MDS_DEST to_dest, MQSV_EVT *i_evt, MQSV_EVT **o_evt, uns32 timeout);

EXTERN_C uns32 mqnd_mds_send(MQND_CB *cb, uns32 to_svc, MDS_DEST to_dest, MQSV_EVT *evt);

EXTERN_C uns32 mqnd_mds_bcast_send(MQND_CB *cb, MQSV_EVT *evt, NCSMDS_SVC_ID to_svc);

#endif
