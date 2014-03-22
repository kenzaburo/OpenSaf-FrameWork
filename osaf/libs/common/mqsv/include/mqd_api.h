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

  FILE NAME: mqd_api.h

..............................................................................

  DESCRIPTION:
  
  API decleration for MQD library..
    
******************************************************************************/
#ifndef MQD_API_H
#define MQD_API_H

#define MQD_COMP_NAME      "MQD"
#define MQD_TASK_PRIORITY  5
#define MQD_TASK_STACKSIZE NCS_STACKSIZE_HUGE
#define MQSV_MQD_REV       1	/* MQD Release 1.00 */

/*****************************EVT HANDLER ***********************************/
typedef uns32 (*MQD_EVT_HANDLER) (struct mqsv_evt *, struct mqd_cb *);
/****************************************************************************/

EXTERN_C uns32 mqd_track_add(NCS_QUEUE *, MDS_DEST *, MDS_SVC_ID);
EXTERN_C uns32 mqd_track_del(NCS_QUEUE *, MDS_DEST *);
EXTERN_C uns32 mqd_mds_init(MQD_CB *);
EXTERN_C uns32 mqd_mds_shut(MQD_CB *);
EXTERN_C uns32 mqd_evt_process(MQSV_EVT *);
EXTERN_C uns32 mqd_asapi_evt_hdlr(MQSV_EVT *, MQD_CB *);
EXTERN_C uns32 mqd_ctrl_evt_hdlr(MQSV_EVT *, MQD_CB *);

EXTERN_C uns32 mqd_asapi_dereg_hdlr(MQD_CB *, ASAPi_DEREG_INFO *, MQSV_SEND_INFO *);
EXTERN_C uns32 mqd_mds_send_rsp(MQD_CB *cb, MQSV_SEND_INFO *s_info, MQSV_EVT *evt);
EXTERN_C void mqd_nd_restart_update_dest_info(MQD_CB *pMqd, MDS_DEST dest);
EXTERN_C void mqd_nd_down_update_info(MQD_CB *pMqd, MDS_DEST dest);
EXTERN_C NCS_BOOL mqd_obj_cmp(void *key, void *elem);

#endif   /* MQD_API_H */
