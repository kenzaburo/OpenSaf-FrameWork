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

/*
* Module Inclusion Control...
*/
#ifndef SMFSV_EVT_H
#define SMFSV_EVT_H

#include <ncsgl_defs.h>
#include <mds_papi.h>
#include <saSmf.h>

/* DO NOT CHANGE ANY OF THE NUMBERS BELOW SINCE IT WILL CAUSE
   NONBACKWARD COMPATIBILITY AND MAKE ROLLING UPGRADES IMPOSSIBLE.
   When adding messages, add them last with new numbers */

/* Message type enums */
typedef enum {
	SMFSV_EVT_TYPE_SMFD = 1,
	SMFSV_EVT_TYPE_SMFND = 2,
	SMFSV_EVT_TYPE_SMFA = 3,
	SMFSV_MSG_MAX
} SMFSV_EVT_TYPE;

/* SMFD event enums */
typedef enum {
	SMFD_EVT_MDS_INFO = 1,
	SMFD_EVT_CMD_RSP = 2,
	SMFD_EVT_CBK_RSP = 3,
	SMFD_EVT_MAX
} SMFD_EVT_TYPE;

/* SMFND event enums */
typedef enum {
	SMFND_EVT_MDS_INFO = 1,
	SMFND_EVT_CMD_REQ = 2,
	SMFND_EVT_CBK_RSP = 3,
	SMFND_EVT_MAX
} SMFND_EVT_TYPE;

/* SMFA event enums */
typedef enum {
	SMFA_EVT_MDS_INFO = 1,
	SMFA_EVT_DUMMY = 2,
	SMFA_EVT_CBK = 3,
	SMFA_EVT_MAX
} SMFA_EVT_TYPE;

/****************************************************************************
 Events SMFSV 
 ****************************************************************************/

typedef enum {
        SMF_CLBK_EVT,
        SMF_RSP_EVT
}SMF_EVT_TYPE;

typedef struct smf_cbk_evt{
        SaInvocationT         inv_id;
	SaSmfCallbackScopeIdT	scope_id;
        SaNameT               object_name;
        SaSmfPhaseT           camp_phase;
        SaSmfCallbackLabelT   cbk_label;
	uns32			params_len;
        SaStringT             params;
}SMF_CBK_EVT;

typedef struct smf_resp_evt{
        SaInvocationT         inv_id;
        SaAisErrorT           err;
}SMF_RESP_EVT;

typedef struct smf_evt{
        SMF_EVT_TYPE          evt_type;
        union {
                SMF_CBK_EVT   cbk_evt;
                SMF_RESP_EVT  resp_evt;
        }evt;
        struct smf_evt        *next;
}SMF_EVT;



/* Structure for passing MDS info to components */
typedef struct {
	NCSMDS_CHG change;	/* GONE, UP, DOWN, CHG ROLE  */
	MDS_DEST dest;
	MDS_SVC_ID svc_id;
	NODE_ID node_id;
	V_DEST_RL role;
} smfsv_mds_info;

/****************************************************************************
 Events to SMFD 
 ****************************************************************************/

/*** SMFD event definitions ***/
typedef struct {
	uns32 result;
} smfd_evt_cmd_rsp;

typedef struct {
	SMFD_EVT_TYPE type;	/* evt type */
	union {
		smfsv_mds_info 		mds_info;
		smfd_evt_cmd_rsp 	cmd_rsp;
		struct smf_evt		cbk_rsp;
	} event;
} SMFD_EVT;

/****************************************************************************
 Events to SMFND 
 ****************************************************************************/

/*** SMFND event definitions ***/
typedef struct {
	uns32 cmd_len;
	char *cmd;
} smfnd_evt_cmd_req;

typedef struct {
	SMFND_EVT_TYPE type;	/* evt type */
	union {
		smfsv_mds_info 		mds_info;
		smfnd_evt_cmd_req 	cmd_req;
		struct smf_evt		cbk_req_rsp;
	} event;
} SMFND_EVT;

/****************************************************************************
 Events to SMFA 
 ****************************************************************************/

/*** SMFND event definitions ***/

typedef struct {
	SMFA_EVT_TYPE type;	/* evt type */
	union {
		smfsv_mds_info 	mds_info;
		struct smf_evt	cbk_req_rsp;	
	} event;
} SMFA_EVT;

/******************************************************************************
 SMFSV Event Data Structure
 ******************************************************************************/
typedef struct smfsv_evt {
	struct smfsv_evt *next;
	SMFSV_EVT_TYPE type;	/* api type */
	uns32 cb_hdl;
	MDS_SYNC_SND_CTXT mds_ctxt;	/* Relevant when this event has to be responded to
					 * in a synchronous fashion.
					 */
	MDS_DEST fr_dest;
	MDS_SVC_ID fr_svc;
	NODE_ID fr_node_id;
	MDS_SEND_PRIORITY_TYPE rcvd_prio;	/* Priority of the recvd evt */
	union {
		SMFD_EVT smfd;
		SMFND_EVT smfnd;
		SMFA_EVT smfa;
	} info;
} SMFSV_EVT;

/******************************************************************************
 SMFSV Public functions
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

	uns32 smfsv_evt_enc(SMFSV_EVT * i_evt, NCS_UBAID * o_ub);
	uns32 smfsv_evt_dec(NCS_UBAID * i_ub, SMFSV_EVT * o_evt);
	void smfsv_evt_destroy(SMFSV_EVT * evt);

	uns32 smfsv_mds_send_rsp(uns32 mds_handle,
				 MDS_SYNC_SND_CTXT mds_ctxt,
				 uns32 to_svc,
				 MDS_DEST to_dest,
				 uns32 fr_svc,
				 MDS_DEST fr_dest, SMFSV_EVT * evt);

	uns32 smfsv_mds_msg_sync_send(uns32 mds_handle,
				      uns32 to_svc,
				      MDS_DEST to_dest,
				      uns32 fr_svc,
				      SMFSV_EVT * i_evt,
				      SMFSV_EVT ** o_evt, uns32 timeout);

	uns32 smfsv_mds_msg_send(uns32 mds_handle,
				 uns32 to_svc,
				 MDS_DEST to_dest,
				 uns32 from_svc, SMFSV_EVT * evt);

#ifdef __cplusplus
}
#endif
#endif				/* SMFSV_EVT_H */
