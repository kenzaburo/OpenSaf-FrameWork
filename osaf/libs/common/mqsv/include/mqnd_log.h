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

#ifndef MQND_LOG_H
#define MQND_LOG_H

extern void _mqnd_genlog(uns8 severity, const char *function, const char *format, ...);

#define mqnd_genlog(severity, format, args...) _mqnd_genlog((severity), __FUNCTION__, (format), ##args)
/******************************************************************************
 Logging offset indexes for MQA Headline logging
 ******************************************************************************/
typedef enum mqnd_hdln_log_flex {
	MQND_CB_INIT_FAILED,
	MQND_CB_DB_INIT_FAILED,
	MQND_CB_DB_INIT_SUCCESS,
	MQND_CB_HDL_CREATE_SUCCESS,
	MQND_IPC_CREATE_FAIL,
	MQND_IPC_CREATE_SUCCESS,
	MQND_IPC_ATTACH_FAIL,
	MQND_IPC_ATTACH_SUCCESS,
	MQND_TASK_CREATE_FAIL,
	MQND_TASK_CREATE_SUCCESS,
	MQND_TASK_START_FAIL,
	MQND_TASK_START_SUCCESS,
	MQND_MQD_SERVICE_WENT_DOWN,
	MQND_MQD_SERVICE_IS_DOWN,

	MQND_CB_ALLOC_FAILED,
	MQND_EVT_ALLOC_FAILED,
	MQND_MQA_LNODE_ALLOC_FAILED,
	MQND_ALLOC_QUEUE_NODE_FAILED,
	MQND_ALLOC_QNAME_NODE_FAILED,
	MQND_ALLOC_QTRANS_EVT_NODE_FAILED,

	MQND_MDS_REGISTER_FAILED,
	MQND_MDS_REGISTER_SUCCESS,
	MQND_MDS_GET_HDL_FAILED,
	MQND_MDS_INSTALL_FAILED,
	MQND_MDS_SUBSCRIBE_FAILED,
	MQND_MDS_UNREG_FAILED,
	MQND_MDS_SEND_FAIL,
	MQND_MDS_ENC_FAILED,
	MQND_MDS_DEC_FAILED,
	MQND_MDS_SND_TO_MAILBOX_FAILED,
	MQND_MDS_SNDDIRECT_RCV,
	MQND_MQA_SERVICE_WENT_DOWN,
	MQND_MQD_SERVICE_CAME_UP,
	MQND_MQA_CAME_UP,
	MQND_MDS_SND_RSP_DIRECT_FAILED,
	MQND_MDS_BCAST_SEND_FAIL,

	MQND_CB_HDL_CREATE_FAILED,
	MQND_CB_HDL_TAKE_FAILED,
	MQND_CB_HDL_GIVE_FAILED,
	MQND_CB_HDL_DESTROY_FAILED,

	MQND_CLM_DISPATCH_FAILURE,

	MQND_ASAPI_BAD_RESP,

	MQND_INIT_SUCCESS,
	MQND_INIT_FAIL,

	MQND_DESTROY_SUCCESS,
	MQND_DESTROY_FAIL,

	MQND_AMF_REGISTER_FAILED,
	MQND_AMF_REGISTER_SUCCESS,
	MQND_AMF_INIT_FAILED,
	MQND_AMF_INIT_SUCCESS,
	MQND_AMF_DESTROY_FAILED,
	MQND_AMF_RESPONSE_FAILED,
	MQND_AMF_GET_SEL_OBJ_FAILURE,
	MQND_AMF_GET_SEL_OBJ_SUCCESS,
	MQND_AMF_DISPATCH_FAILURE,
	MQND_AMF_DISPATCH_SUCCESS,
	MQND_AMF_COMP_NAME_GET_FAILED,
	MQND_AMF_COMP_NAME_GET_SUCCESS,
	MQND_AMF_COMP_UNREGISTER_FAILED,
	MQND_AMF_COMP_UNREGISTER_SUCCESS,
	MQND_AMF_TERM_CLBK_CALLED,
	MQND_AMF_CSI_SET_CLBK_CALLED,
	MQND_AMF_CSI_RMV_CLBK_CALLED,
	MQND_MQA_TMR_STARTED,

	MQND_CPSV_CKPT_INIT_FAILURE,
	MQND_RESTART_INIT_OPEN_SUCCESS,
	MQND_RESTART_BUILD_DB_FROM_CKPTSVC_FAILED,
	MQND_RESTART_CKPT_READ_FAILED,
	MQND_RESTART_CKPT_SECTION_DELETE_FAILED,
	MQND_RESTART_CPSV_INIT_FAILED,
	MQND_AMF_REGISTRATION_SUCCESS,
	MQND_AMF_INITIALISATION_SUCCESS,
	MQND_RESTART_INIT_FIRST_TIME,
	MQND_RESTART_NO_MQA_FOUND,

	MQND_HEALTH_CHECK_START_FAILED,
	MQND_HEALTH_CHECK_START_SUCCESS,
	MQND_MDS_GET_HDL_SUCCESS,
	MQND_MDS_INSTALL_SUCCESS,
	MQND_MDS_SUBSCRIBE_SUCCESS,
	MQND_MDS_UNREG_SUCCESS,
	MQND_MDS_CLBK_COMPLETE,
	MQND_MSGQ_CREATE_FAILED,
	MQND_LISTENERQ_CREATE_FAILED,
	MQND_QNODE_ADD_DB_FAILED,
	MQND_QNAMENODE_ADD_DB_FAILED,
	MQND_QTRANS_EVT_NODE_ADD_DB_FAILED,
	MQND_CKPT_SECTION_CREATE_FAILED,
	MQND_Q_REG_WITH_MQD_FAILED,
	MQND_QUEUE_CREATE_SUCCESS,
	MQND_CKPT_SECTION_DELETE_FAILED,
	MQND_Q_CREATE_ATTR_GET,
	MQND_Q_REG_WITH_MQD_SUCCESS,
	MQND_ASAPI_REG_HDLR_FAILED,
	MQND_ASAPI_DEREG_HDLR_FAILED,
	MQND_MEM_ALLOC_FAILED,
	MQND_MSGQ_TBL_EXACT_FLTR_REGISTER_FAILED,
	MQND_MSGQ_TBL_EXACT_FLTR_REGISTER_SUCCESS,
	MQND_MSGQPR_TBL_EXACT_FLTR_REGISTER_FAILED,
	MQND_MSGQPR_TBL_EXACT_FLTR_REGISTER_SUCCESS,
	MQND_MQA_TMR_EXPIRED,
	MQND_TMR_STARTED,
	MQND_EVT_RECEIVED,
	MQND_CTRL_EVT_RECEIVED,
	MQND_MQA_DOWN,
	MQND_INITIALIZATION_INCOMPLETE,
	MQND_FINALIZE_CLOSE_STATUS,
	MQND_FINALIZE_RESP_SENT_FAILED,
	MQND_FINALIZE_RESP_SENT_SUCCESS,
	MQND_INIT_RESP_SENT_FAILED,
	MQND_INIT_RESP_SENT_SUCCESS,
	MQND_ASAPI_NRESOLVE_HDLR_FAILED,
	MQND_PROC_QUEUE_OPEN_FAILED,
	MQND_GET_QNODE_FAILED,
	MQND_PROC_QUEUE_CLOSE_FAILED,
	MQND_PROC_QUEUE_CLOSE_SUCCESS,
	MQND_QUEUE_CLOSE_RESP_FAILED,
	MQND_QUEUE_CLOSE_RESP_SUCCESS,
	MQND_QUEUE_UNLINK_RESP_FAILED,
	MQND_QUEUE_UNLINK_RESP_SUCCESS,
	MQND_QUEUE_STAT_REQ_RESP_FAILED,
	MQND_QUEUE_STAT_REQ_RESP_SUCCESS,
	MQND_UNDER_TRANSFER_STATE,
	MQND_QUEUE_FULL,
	MQND_QUEUE_GET_ATTR_FAILED,
	MQND_QUEUE_RESIZE_FAILED,
	MQND_SEND_FAILED,
	MQND_LISTENER_SEND_FAILED,
	MQND_MDS_SND_FAILED,
	MQND_MDS_SND_RSP_FAILED,
	MQND_MDS_SND_RSP_SUCCESS,
	MQND_MQA_ADD_NODE_FAILED,
	MQND_RETENTION_TMR_EXPIRED,
	MQND_QTRANSFER_NODE1_TMR_EXPIRED,
	MQND_QTRANSFER_NODE2_TMR_EXPIRED,
	MQND_MQ_DESTROY_FAILED,
	MQND_QUEUE_ERR_BUSY,
	MQND_QUEUE_TRANS_IN_PROGRESS,
	MQND_QUEUE_CREATE_FAILED,
	MQND_TRANSER_REQ_FAILED,
	MQND_MQUEUE_EMPTY_FAILED,
	MQND_Q_ATTR_GET_FAILED,
	MQND_Q_ATTR_COMPARE_FAILED,
	MQND_TRANSFER_OWNER_REQ_FAILED,
	MQND_EXISTING_LOCAL_QUEUE_OPEN_FAILED,
	MQND_QUEUE_CREAT_FLAG_NOT_SET,
	MQND_RETENTION_TMR_STARTED,
	MQND_QTRANSFER_NODE1_TMR_STARTED,
	MQND_QTRANSFER_NODE2_TMR_STARTED,
	MQND_MQA_LNODE_ADD_FAILED,
	MQND_CPSV_CKPT_INIT_SUCCESS,
	MQND_RESTART_OPEN_FIRST_TIME_FAILED,
	MQND_CKPT_OPEN_FAILED,
	MQND_CKPT_ITERATION_INIT_FAILED,
	MQND_CKPT_BUILD_DATABASE_FAILED,
	MQND_CKPT_SECTION_OVERWRITE_FAILED,
	MQND_QNODE_ADD_TO_DATABASE_FAILED,
	MQND_CKPT_READ_SUCCESS,
	MQND_SHM_CREATE_FAILED,
	MQND_MSG_FRMT_VER_INVALID,
	MQND_CLM_INIT_FAILED,
	MQND_CLM_NODE_GET_FAILED
} MQND_HDLN_LOG_FLEX;

/******************************************************************************
 Logging offset indexes for canned constant strings for the ASCII SPEC
 ******************************************************************************/

typedef enum mqnd_flex_sets {
	MQND_FC_HDLN,
	MQND_FC_GENLOG,
	MQND_FC_MEMFAIL,
	MQND_FC_EVT
} MQND_FLEX_SETS;

typedef enum mqnd_log_ids {
	MQND_LID_HDLN,
	MQND_LID_GENLOG,
	MQND_LID_MEMFAIL,
	MQND_LID_EVT
} MQND_LOG_IDS;

EXTERN_C uns32 mqnd_log_ascii_reg(void);
EXTERN_C void mqnd_log_ascii_dereg(void);

#if((NCS_DTA == 1) && (NCS_MQSV_LOG == 1))
EXTERN_C void mqnd_log(uns8, uns32, uns8, uns32, char *, uns32);
#define m_LOG_MQSV_ND(id,category,sev,rc,fname,fno)  mqnd_log(id,category,sev,rc,fname,fno)
#else
#define m_LOG_MQSV_ND(id,category,sev,rc,fname,fno)
#endif   /* (NCS_FLA == 1) && (NCS_MQSV_LOG == 1) */

EXTERN_C void mqnd_flx_log_reg(void);
EXTERN_C void mqnd_flx_log_dereg(void);

#endif