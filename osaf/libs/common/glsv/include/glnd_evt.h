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

#ifndef GLND_EVT_H
#define GLND_EVT_H

/*****************************************************************************
 * Event Type of GLND 
 *****************************************************************************/
typedef enum glsv_glnd_evt_type {

	/* events from GLA to GLND */
	GLSV_GLND_EVT_BASE,
	GLSV_GLND_EVT_REG_AGENT,
	GLSV_GLND_EVT_UNREG_AGENT,
	GLSV_GLND_EVT_INITIALIZE,
	GLSV_GLND_EVT_FINALIZE,
	GLSV_GLND_EVT_RSC_OPEN,
	GLSV_GLND_EVT_RSC_CLOSE,
	GLSV_GLND_EVT_RSC_LOCK,
	GLSV_GLND_EVT_RSC_UNLOCK,
	GLSV_GLND_EVT_RSC_PURGE,
	GLSV_GLND_EVT_CLIENT_INFO,

	/* events from GLND to GLND */
	GLSV_GLND_EVT_LCK_REQ,
	GLSV_GLND_EVT_UNLCK_REQ,
	GLSV_GLND_EVT_LCK_RSP,
	GLSV_GLND_EVT_UNLCK_RSP,
	GLSV_GLND_EVT_LCK_REQ_CANCEL,
	GLSV_GLND_EVT_LCK_REQ_ORPHAN,
	GLSV_GLND_EVT_LCK_WAITER_CALLBACK,
	GLSV_GLND_EVT_LCK_PURGE,
	GLSV_GLND_EVT_SND_RSC_INFO,
	GLSV_GLND_EVT_FWD_DD_PROBE,
	GLSV_GLND_EVT_DD_PROBE,

	/* Events from GLD to GLND */
	GLSV_GLND_EVT_RSC_GLD_DETAILS,
	GLSV_GLND_EVT_RSC_NEW_MASTER,
	GLSV_GLND_EVT_RSC_MASTER_INFO,

	/* Timer Timeout Events */
	GLSV_GLND_EVT_RSC_OPEN_TIMEOUT,
	GLSV_GLND_EVT_RSC_LOCK_TIMEOUT,
	GLSV_GLND_EVT_NM_RSC_LOCK_TIMEOUT,
	GLSV_GLND_EVT_NM_RSC_UNLOCK_TIMEOUT,
	GLSV_GLND_EVT_AGENT_INFO_GET_TIMEOUT,

	/* Local Event */
	GLSV_GLND_EVT_NON_MASTER_INFO,

	/* Debug Events */
	GLSV_GLND_EVT_CB_DUMP,

	GLSV_GLND_EVT_MAX,

} GLSV_GLND_EVT_TYPE;

/*****************************************************************************
 * This Event structures are used between GLA and GLND 
 *****************************************************************************/
typedef struct glsv_evt_agent_info_tag {
	uns32 process_id;
	MDS_DEST agent_mds_dest;
} GLSV_EVT_AGENT_INFO;

typedef struct glsv_evt_client_info_tag {
	uns32 client_proc_id;
	MDS_DEST agent_mds_dest;
	SaVersionT version;
	uns16 cbk_reg_info;	/* bit-wise data */
} GLSV_EVT_CLIENT_INFO;

typedef struct glsv_evt_finalize_info_tag {
	MDS_DEST agent_mds_dest;
	SaLckHandleT handle_id;
} GLSV_EVT_FINALIZE_INFO;

typedef struct glsv_evt_rsc_info_tag {
	SaLckHandleT client_handle_id;
	MDS_DEST agent_mds_dest;
	SaNameT resource_name;
	SaLckResourceIdT resource_id;
	SaLckResourceIdT lcl_resource_id;
	SaLckLockIdT lcl_lockid;
	uns32 lcl_resource_id_count;
	SaInvocationT invocation;
	GLSV_CALL_TYPE call_type;
	SaTimeT timeout;
	SaLckResourceOpenFlagsT flag;
} GLSV_EVT_RSC_INFO;

typedef struct glsv_evt_restart_res_info {
	SaLckResourceIdT resource_id;
	struct glsv_evt_restart_res_info *next;
} GLSV_EVT_RESTART_RES_INFO;

typedef struct glsv_evt_restart_client_info {
	SaLckHandleT client_handle_id;
	uns32 app_proc_id;
	MDS_DEST agent_mds_dest;
	SaVersionT version;
	uns16 cbk_reg_info;	/* bit-wise data */
	uns32 no_of_res;
	SaLckResourceIdT resource_id;
} GLSV_EVT_RESTART_CLIENT_INFO;

typedef struct glsv_evt_rsc_lock_info_tag {
	SaLckHandleT client_handle_id;
	MDS_DEST agent_mds_dest;
	SaLckResourceIdT resource_id;
	SaLckResourceIdT lcl_resource_id;
	SaLckLockIdT lcl_lockid;
	SaInvocationT invocation;
	SaLckLockModeT lock_type;
	SaTimeT timeout;
	SaLckLockFlagsT lockFlags;
	GLSV_CALL_TYPE call_type;
	GLSV_LOCK_STATUS status;
	SaLckWaiterSignalT waiter_signal;
} GLSV_EVT_RSC_LOCK_INFO;

typedef struct glsv_evt_rsc_unlock_info_tag {
	SaLckHandleT client_handle_id;
	MDS_DEST agent_mds_dest;
	SaLckResourceIdT resource_id;
	SaLckResourceIdT lcl_resource_id;
	SaLckLockIdT lockid;
	SaLckLockIdT lcl_lockid;
	SaInvocationT invocation;
	GLSV_CALL_TYPE call_type;
	SaTimeT timeout;
} GLSV_EVT_RSC_UNLOCK_INFO;

typedef struct glsv_evt_glnd_lck_info_tag {
	SaLckResourceIdT resource_id;
	SaLckResourceIdT lcl_resource_id;
	SaLckHandleT client_handle_id;
	MDS_DEST glnd_mds_dest;	/* can replace it with node id */
	SaLckLockIdT lockid;
	SaLckLockIdT lcl_lockid;
	SaLckLockModeT lock_type;
	SaLckLockFlagsT lockFlags;
	SaLckLockStatusT lockStatus;
	SaLckWaiterSignalT waiter_signal;
	SaLckLockModeT mode_held;
	SaAisErrorT error;
	SaInvocationT invocation;
} GLSV_EVT_GLND_LCK_INFO;

typedef struct glsv_evt_glnd_rsc_info_tag {
	/* list of all rsc-lck info */
	SaLckResourceIdT resource_id;
	MDS_DEST glnd_mds_dest;
	uns32 num_requests;
	GLND_LOCK_LIST_INFO *list_of_req;
} GLSV_EVT_GLND_RSC_INFO;

typedef struct glsv_glnd_dd_info_list_tag {
	SaLckResourceIdT rsc_id;
	MDS_DEST blck_dest_id;
	SaLckHandleT blck_hdl_id;
	SaLckLockIdT lck_id;
	struct glsv_glnd_dd_info_list_tag *next;
} GLSV_GLND_DD_INFO_LIST;

typedef struct glsv_evt_glnd_dd_probe_info_tag {
	SaLckResourceIdT rsc_id;
	SaLckResourceIdT lcl_rsc_id;
	SaLckHandleT hdl_id;
	MDS_DEST dest_id;
	SaLckLockIdT lck_id;
	struct glsv_glnd_dd_info_list_tag *dd_info_list;
} GLSV_EVT_GLND_DD_PROBE_INFO;

typedef struct glsv_evt_glnd_rsc_gld_info_tag {
	SaNameT rsc_name;
	SaLckResourceIdT rsc_id;
	MDS_DEST master_dest_id;
	NCS_BOOL can_orphan;
	SaLckLockModeT orphan_mode;
	SaAisErrorT error;
} GLSV_EVT_GLND_RSC_GLD_INFO;

typedef struct glsv_evt_glnd_rsc_master_info_list_tag {
	SaLckResourceIdT rsc_id;
	MDS_DEST master_dest_id;
	uns32 master_status;
	struct glsv_evt_glnd_rsc_master_info_list_tag *next;
} GLSV_GLND_RSC_MASTER_INFO_LIST;

typedef struct glsv_evt_glnd_rsc_master_info_tag {
	uns32 no_of_res;
	struct glsv_evt_glnd_rsc_master_info_list_tag *rsc_master_list;
} GLSV_EVT_GLND_RSC_MASTER_INFO;

typedef struct glsv_evt_glnd_new_master_info_tag {
	SaLckResourceIdT rsc_id;
	MDS_DEST master_dest_id;
	NCS_BOOL orphan;	/*should the new master orphan the lck? */
	SaLckLockModeT orphan_lck_mode;	/*lck_mode for orphaning            */
	uns32 status;		/* newly added */
} GLSV_EVT_GLND_NEW_MAST_INFO;

/* timer event definition */
typedef struct glnd_evt_tmr_tag {
	uns32 opq_hdl;
} GLND_EVT_TMR;

typedef struct glnd_evt_glnd_down_tag {
	MDS_DEST dest_id;
	uns32 status;
} GLND_EVT_GLND_NON_MASTER_STATUS;

/*****************************************************************************
 * GLND event data structure.
 *****************************************************************************/
typedef struct glsv_glnd_evt {
	struct glsv_glnd_evt *next;
	GLSV_GLND_EVT_TYPE type;
	uns32 priority;
	uns32 glnd_hdl;
	union {
		GLSV_EVT_AGENT_INFO agent_info;	/* GLSV_GLND_EVT_REG_AGENT , GLSV_GLND_EVT_UNREG_AGENT  */
		GLSV_EVT_CLIENT_INFO client_info;	/* GLSV_GLND_EVT_INITIALIZE  */
		GLSV_EVT_RESTART_CLIENT_INFO restart_client_info;	/*GLSV_GLND_EVT_CLIENT_INFO             */
		GLSV_EVT_RESTART_RES_INFO restart_res_info;	/*GLSV_GLND_EVT_RES_INFO             */
		GLSV_EVT_FINALIZE_INFO finalize_info;	/* GLSV_GLND_EVT_FINALIZE */
		GLSV_EVT_RSC_INFO rsc_info;	/* GLSV_GLND_EVT_RSC_OPEN , GLSV_GLND_EVT_RSC_CLOSE, 
						   GLSV_GLND_EVT_RSC_PURGE, GLSV_GLND_EVT_LCK_PURGE */
		GLSV_EVT_RSC_LOCK_INFO rsc_lock_info;	/* GLSV_GLND_EVT_RSC_LOCK */
		GLSV_EVT_RSC_UNLOCK_INFO rsc_unlock_info;	/* GLSV_GLND_EVT_RSC_UNLOCK */

		GLSV_EVT_GLND_LCK_INFO node_lck_info;	/* GLSV_GLND_EVT_LCK_REQ, GLSV_GLND_EVT_UNLCK_REQ */
		/* GLSV_GLND_EVT_LCK_RSP, GLSV_GLND_EVT_LCK_WAITER_CALLBACK,
		   GLSV_GLND_EVT_UNLCK_RSP , GLSV_GLND_EVT_LCK_REQ_CANCEL 
		   GLSV_GLND_EVT_LCK_REQ_ORPHAN */
		GLSV_EVT_GLND_RSC_INFO node_rsc_info;	/* GLSV_GLND_EVT_SND_RSC_INFO */
		GLSV_EVT_GLND_DD_PROBE_INFO dd_probe_info;	/* GLSV_GLND_EVT_FWD_DD_PROBE, GLSV_GLND_EVT_DD_PROBE */
		GLSV_EVT_GLND_RSC_GLD_INFO rsc_gld_info;	/* GLSV_GLND_EVT_RSC_GLD_DETAILS */
		GLSV_EVT_GLND_RSC_MASTER_INFO rsc_master_info;	/* GLSV_GLND_EVT_RSC_MASTER_INFO */
		GLSV_EVT_GLND_NEW_MAST_INFO new_master_info;	/* GLSV_GLND_EVT_RSC_NEW_MASTER */
		GLND_EVT_TMR tmr;	/* All the timer events */
		GLND_EVT_GLND_NON_MASTER_STATUS non_master_info;	/* GLSV_GLND_EVT_NON_MASTER_INFO */
	} info;
	MDS_SYNC_SND_CTXT mds_context;
	uns32 shm_index;
} GLSV_GLND_EVT;

/* prototypes */
EXTERN_C void glnd_evt_destroy(GLSV_GLND_EVT *evt);
EXTERN_C uns32 glnd_process_evt(NCSCONTEXT cb, GLSV_GLND_EVT *evt);

#endif
