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

  This file contains evt handling routines for events received by the EDS.
..............................................................................
  

*******************************************************************************/
#ifndef EDS_EVT_H
#define EDS_EVT_H

typedef enum edsv_eds_evt_type {
	EDSV_EDS_EVT_BASE,
	EDSV_EDS_EDSV_MSG = EDSV_EDS_EVT_BASE,
	EDSV_EDS_EVT_EDA_UP,
	EDSV_EDS_EVT_EDA_DOWN,
	EDSV_EDS_RET_TIMER_EXP,
	EDSV_EVT_QUIESCED_ACK,
	EDSV_EDS_EVT_MAX
} EDSV_EDS_EVT_TYPE;

typedef struct edsv_eds_mds_info_tag {
	uns32 node_id;
	MDS_DEST mds_dest_id;
} EDSV_EDS_MDS_INFO;

typedef struct edsv_eds_evt_tag {
	/* NCS_IPC_MSG       next; */
	struct edsv_eds_evt_tag *next;
	uns32 cb_hdl;
	MDS_SYNC_SND_CTXT mds_ctxt;	/* Relevant when this event has to be responded to
					 * in a synchronous fashion.
					 */
	MDS_DEST fr_dest;
	NODE_ID fr_node_id;
	MDS_SEND_PRIORITY_TYPE rcvd_prio;	/* Priority of the recvd evt */
	EDSV_EDS_EVT_TYPE evt_type;
	union {
		EDSV_MSG msg;
		EDSV_EDS_MDS_INFO mds_info;
		EDS_TMR tmr_info;
	} info;
} EDSV_EDS_EVT;

#define EDSV_EDS_EVT_NULL ((EDSV_EDS_EVT *)0)

/* These are the function prototypes for event handling */
typedef uns32 (*EDSV_EDS_EDA_API_MSG_HANDLER) (EDS_CB *, struct edsv_eds_evt_tag * evt);
typedef uns32 (*EDSV_EDS_EVT_HANDLER) (struct edsv_eds_evt_tag * evt);
EXTERN_C uns32 eds_process_evt(EDSV_EDS_EVT *evt);
EXTERN_C void eds_evt_destroy(struct edsv_eds_evt_tag *);

#endif   /*!EDS_EVT_H */
