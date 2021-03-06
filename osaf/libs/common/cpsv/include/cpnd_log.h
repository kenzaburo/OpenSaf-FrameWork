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

#ifndef CPND_LOG_H
#define CPND_LOG_H

/******************************************************************************
 Logging offset indexes for CPA Headline logging
 ******************************************************************************/
typedef enum cpnd_hdln_log_flex {
	CPND_CB_INIT_FAILED,

	CPND_IPC_CREATE_FAIL,
	CPND_IPC_ATTACH_FAIL,
	CPND_TASK_CREATE_FAIL,
	CPND_TASK_START_FAIL,

	CPND_CPD_SERVICE_WENT_DOWN,
	CPND_CPD_SERVICE_IS_DOWN,
	CPND_CPD_SERVICE_CAME_UP,
	CPND_CPD_SERVICE_NOACTIVE,
	CPND_CPD_SERVICE_NEWACTIVE,
	CPND_MDS_REGISTER_FAILED,
	CPND_MDS_GET_HDL_FAILED,
	CPND_MDS_INSTALL_FAILED,
	CPND_MDS_SUBSCRIBE_FAILED,
	CPA_MDS_UNREG_FAILED,
	CPND_MDS_SEND_FAIL,
	CPND_MDS_SEND_TRYAGAIN,
	CPND_CB_HDL_CREATE_FAILED,
	CPND_CB_HDL_TAKE_FAILED,
	CPND_CB_HDL_GIVE_FAILED,
	CPND_CB_HDL_DESTROY_FAILED,
	CPND_INIT_SUCCESS,
	CPND_INIT_FAIL,
	CPND_DESTROY_SUCCESS,
	CPND_DESTROY_FAIL,
	CPND_AMF_REGISTER_FAILED,
	CPND_AMF_DEREGISTER_FAILED,
	CPND_AMF_INIT_FAILED,
	CPND_AMF_DESTROY_FAILED,
	CPND_AMF_RESPONSE_FAILED,
	CPND_AMF_GET_SEL_OBJ_FAILURE,
	CPND_AMF_DISPATCH_FAILURE,
	CPND_CB_RETRIEVAL_FAILED,

	CPND_CKPT_SECT_CREATE_FAILED,
	CPND_CKPT_SECT_WRITE_FAILED,
	CPND_CKPT_SECT_READ_FAILED,
	CPND_CKPT_HDR_UPDATE_FAILED,
	CPND_CLIENT_HDL_GET_FAILED,
	CPND_MDS_ACTIVE_TO_REMOTE_SEND_FAILED,

	CPND_AMF_COMP_NAME_GET_FAILED,
	CPND_AMF_COMP_REG_FAILED,
	CPND_CLM_INIT_FAILED,
	CPND_CLM_NODE_GET_FAILED,
	CPND_AMF_HLTH_CHK_START_FAILED,
	CPND_SECT_HDR_UPDATE_FAILED,
	CPND_ACTIVE_REP_SET_SUCCESS,

} CPND_HDLN_LOG_FLEX;

/******************************************************************************
 Logging offset indexes for Memory Fail logging 
 ******************************************************************************/
typedef enum cpnd_memfail_log_flex {
	CPND_CB_ALLOC_FAILED,
	CPND_EVT_ALLOC_FAILED,
	CPND_CKPT_ALLOC_FAILED,
	CPND_SECT_ALLOC_FAILED,
	CPND_CKPT_LIST_ALLOC_FAILED,
	CPND_CKPT_CLIST_ALLOC_FAILED,
	CPND_DEFAULT_ALLOC_FAILED,
	CPND_DEST_INFO_ALLOC_FAILED,
	CPND_CKPT_DATA_ALLOC_FAILED,
	CPND_CLIENT_ALLOC_FAILED,
	CPND_CKPT_SECTION_INFO_FAILED,
	CPND_SYNC_SEND_NODE_ALLOC_FAILED,
	CPND_CPD_DEFERRED_REQ_NODE_ALLOC_FAILED,
	CPND_CPSV_ND2A_READ_MAP_ALLOC_FAIL,
	CPND_CPSV_ND2A_READ_DATA_ALLOC_FAIL,

} CPND_MEMFAIL_LOG_FLEX;

/******************************************************************************
 Logging offset indexes for Event logging 
 ******************************************************************************/
typedef enum cpnd_evt_log_flex {
	CPND_EVT_UNKNOWN,
	CPND_EVT_PROCESS_FAILURE,
	CPND_EVT_RECIEVED,
	CPND_CPD_NEW_ACTIVE_DESTROY_FAILED,
	CPND_CPD_NEW_ACTIVE_DESTROY_BYNAME_FAILED,
	CPND_CPD_NEW_ACTIVE_UNLINK_FAILED,
	CPND_CPD_NEW_ACTIVE_RDSET_FAILED,
	CPND_CPD_NEW_ACTIVE_AREP_SET_FAILED,
} CPND_EVT_LOG_FLEX;

typedef enum cpnd_api_log_flex {

	CPND_CLIENT_TREE_ADD_FAILED,
	CPND_CLIENT_TREE_DEL_FAILED,
	CPND_CLIENT_TREE_GET_FAILED,

	CPND_CKPT_NODE_ADD_FAILED,
	CPND_CKPT_NODE_FIND_FAILED,
	CPND_CKPT_NODE_DEL_FAILED,
	CPND_CKPT_NODE_GET_FAILED,
	CPND_CKPT_NODE_FIND_BY_NAME_FAILED,
	CPND_CLIENT_NODE_GET_FAILED,
	CPND_CKPT_SECT_ADD_FAILED,
	CPND_CKPT_SECT_DEL_FAILED,
	CPND_CKPT_SECT_GET_FAILED,
	CPND_CKPT_SECT_FIND_FAILED,
	CPND_RMT_CPND_FIND_FAILED,
	CPND_REP_ADD_SUCCESS,
	CPND_REP_DEL_SUCCESS,
	CPND_PROC_SEC_EXPIRY_FAILED,
	CPND_PROC_RT_EXPIRY_FAILED,
	CPND_CB_DB_INIT_FAILED,
	CPND_CKPT_NODE_TREE_INIT_FAILED,
	CPND_CLIENT_NODE_TREE_INIT_FAILED,
	CPND_RMT_CPND_ENTRY_DOES_NOT_EXIST,
	CPND_CLIENT_FINALIZE_SUCCESS,
	CPND_CKPT_OPEN_SUCCESS,
	CPND_CKPT_OPEN_FAILURE,
	CPND_CLIENT_CKPT_CLOSE_SUCCESS,
	CPND_PROC_CKPT_UNLINK_SUCCESS,
	CPND_OPEN_ACTIVE_SYNC_EXPIRY_FAILED
} CPND_API_LOG_FLEX;

typedef enum cpnd_sys_call_log_flex {
	CPND_CKPT_OPEN_FAILED,
	CPND_CKPT_UNLINK_FAILED,
	CPND_CKPT_CLOSE_FAILED,

} CPND_SYS_CALL_LOG_FLEX;

typedef enum cpnd_restart_log_flex {
	CPND_COMING_UP_FIRST_TIME,
	CPND_NEW_SHM_CREATE_SUCCESS,
	CPND_RESTARTED,
	CPND_CLIENT_INFO_READ_SUCCESS,
	CPND_OPEN_REQ_FAILED,
	CPND_NUM_CLIENTS_READ,
	CPND_CLIENT_HDL_EXTRACTED,
	CPND_MAX_CLIENTS_REACHED,
	CPND_CLIENT_FREE_BLOCK_SUCCESS,
	CPND_CLIENT_FREE_BLOCK_FAILED,
	CPND_MAX_CKPTS_REACHED,
	CPND_CKPT_FREE_BLOCK_SUCCESS,
	CPND_CKPT_WRITE_HEADER_SUCCESS,
	CPND_CLI_INFO_WRITE_HEADER_SUCCESS,
	CPND_CLIENT_INF0_UPDATE_FAILED,
	CPND_CLIENT_INF0_UPDATE_SUCCESS,
	CPND_CKPT_INF0_READ_FAILED,
	CPND_CKPT_INF0_WRITE_FAILED,
	CPND_CKPT_INF0_WRITE_SUCCESS,
	CPND_CLIENT_BITMAP_RESET_SUCCESS,
	CPND_CKPT_REPLICA_CREATE_FAILED,
	CPND_SECT_HDR_READ_FAILED,
	CPND_CLIHDR_INFO_READ_FAILED,
	CPND_CLIENT_INF0_READ_FAILED,
	CPND_RESTART_SHM_CKPT_UPDATE_FAILED,
} CPND_RESTART_LOG_FLEX;

typedef enum cpnd_mdsfail_log_flex {
	CPND_ACTIVE_TO_REMOTE_MDS_SEND_FAIL,
	CPND_REMOTE_TO_ACTIVE_MDS_SEND_FAIL,
	CPND_SYNC_SEND_TO_CPD_FAILED
} CPND_MDSFAIL_LOG_FLEX;

typedef enum cpnd_generic_log_flex {
	CPND_CKPT_REPLICA_DESTROY_SUCCESS,
	CPND_CKPT_REPLICA_DESTROY_FAILED,
	CPND_CKPT_REPLICA_CLOSE_FAILED,
	CPND_CKPT_REP_CREATE_FAILED,
	CPND_REPLICA_HAS_NO_SECTIONS,
	CPND_SECTION_BOUNDARY_VIOLATION,
	CPND_MDS_DECODE_FAILED,
	CPND_MDS_ENC_FLAT_FAILED,
	CPND_NCS_ENQUEUE_EVT_FAILED,
	CPND_NCS_IPC_SEND_FAILED,
	CPND_CREATING_MORE_THAN_MAX_SECTIONS,
	CPND_MDS_DEC_FLAT_FAILED,
	CPND_AMF_HLTH_CB_SUCCESS,
	CPND_AMF_REGISTER_SUCCESS,
	CPND_AMF_COMP_UNREG_FAILED,
	CPND_AMF_TERM_CB_INVOKED,
	CPND_CSI_RMV_CB_INVOKED,
	CPND_CSI_CB_INVOKED,
	CPND_MDS_SUBSCRIBE_CPD_FAILED,
	CPND_MDS_SUBSCRIBE_CPA_FAILED,
} CPND_GENERIC_LOG_FLEX;

typedef enum cpnd_ckptinfo_log_flex {
	CPND_CKPT_NODE_ADDITION_FAILED,
	CPND_CHECKPOINT_NODE_GET_FAILED,
	CPND_CKPT_RET_TMR_SUCCESS,
	CPND_PROC_CKPT_UNLINK_SET,
	CPND_CKPT_REPLICA_CREATE_SUCCESS,
	CPND_NON_COLLOC_CKPT_REPLICA_CREATE_SUCCESS,
	CPND_CKPT_CLIENT_DEL_SUCCESS,
	CPND_OPEN_ACTIVE_SYNC_START_TMR_SUCCESS,
	CPND_OPEN_ACTIVE_SYNC_STOP_TMR_SUCCESS
} CPND_CKPTINFO_LOG_FLEX;

/******************************************************************************
 Logging offset indexes for canned constant strings for the ASCII SPEC
 ******************************************************************************/

typedef enum cpnd_flex_sets {
	CPND_FC_HDLN,
	CPND_FC_MEMFAIL,
	CPND_FC_EVT,
	CPND_FC_NCS_LOCK,	/* Sashi: Use this only if Required */
	CPND_FC_SYS_CALL,
	CPND_FC_RESTART,
	CPND_FC_API,
	CPND_FC_GENERIC,
	CPND_FC_MDSFAIL,
	CPND_FC_CKPTINFO,

} CPND_FLEX_SETS;

typedef enum cpnd_log_ids {
	CPND_LID_TILCL,
	CPND_LID_TICL,
	CPND_LID_TIFCL,
	CPND_LID_TICCL,
	CPND_LID_TIFFCL,
	CPND_LID_TIFFLCL,
	CPND_LID_TIFFFLCL,
	CPND_LID_TICFCL,
	CPND_LID_TIFLCL,
	CPND_LID_TICFFFCL,
} CPND_LOG_IDS;

EXTERN_C void cpnd_flx_log_reg();
EXTERN_C void cpnd_flx_log_dereg();
EXTERN_C uns32 cpnd_log_ascii_reg(void);
EXTERN_C void cpnd_log_ascii_dereg(void);

#define NCSFL_TYPE_TIFFFCL "TIFFFCL"
#define NCSFL_TYPE_TIFCLCL "TIFCLCL"
#define NCSFL_TYPE_TILCL  "TILCL"
#define NCSFL_TYPE_TICL   "TICL"
#define NCSFL_TYPE_TIFCL  "TIFCL"
#define NCSFL_TYPE_TICCL  "TICCL"
#define NCSFL_TYPE_TIFFFLCL "TIFFFLCL"
#define NCSFL_TYPE_TICFCL "TICFCL"
#define NCSFL_TYPE_TIFLCL "TIFLCL"
#define NCSFL_TYPE_TIFFCL "TIFFCL"
#define NCSFL_TYPE_TIFFLCL "TIFFLCL"
#define NCSFL_TYPE_TICFFFCL "TICFFFCL"

#define m_LOG_CPND_LCL(id,strid,sev,rc,filename,lineno) ncs_logmsg(NCS_SERVICE_ID_CPND,CPND_LID_TILCL,strid, \
                                                       NCSFL_LC_HEADLINE,sev,NCSFL_TYPE_TILCL,id,rc,filename,lineno)

#define m_LOG_CPND_CL(id,strid,sev,filename,lineno)  ncs_logmsg(NCS_SERVICE_ID_CPND,CPND_LID_TICL,strid,NCSFL_LC_HEADLINE, \
                                                     sev,NCSFL_TYPE_TICL,id,filename,lineno)

#define m_LOG_CPND_FCL(id,strid,sev,hdl,filename,lineno)  ncs_logmsg(NCS_SERVICE_ID_CPND,CPND_LID_TIFCL, \
                                   strid,NCSFL_LC_HEADLINE,sev,NCSFL_TYPE_TIFCL,id,(DOUBLE)hdl,filename,lineno)

#define m_LOG_CPND_CFFFCL(id,strid,sev,str1,dest1,dest2,ckptid,filename,lineno)  ncs_logmsg(NCS_SERVICE_ID_CPND, \
              CPND_LID_TICFFFCL,strid,NCSFL_LC_HEADLINE,sev,NCSFL_TYPE_TICFFFCL,id,str1,(DOUBLE)dest1,(DOUBLE)dest2, \
              (DOUBLE)ckptid,filename,lineno)

#define m_LOG_CPND_FFCL(id,strid,sev,cli_hdl,ckptid,filename,lineno)  ncs_logmsg(NCS_SERVICE_ID_CPND, \
              CPND_LID_TIFFCL,strid,NCSFL_LC_HEADLINE,sev,NCSFL_TYPE_TIFFCL,id, (DOUBLE)cli_hdl ,\
              (DOUBLE)ckptid,filename,lineno)

#define m_LOG_CPND_FFLCL(id,strid,sev,cli_hdl,ckptid,cnt, filename,lineno)  ncs_logmsg(NCS_SERVICE_ID_CPND, \
              CPND_LID_TIFFLCL,strid,NCSFL_LC_HEADLINE,sev,NCSFL_TYPE_TIFFLCL,id, (DOUBLE)cli_hdl ,\
              (DOUBLE)ckptid,cnt, filename,lineno)

#define m_LOG_CPND_FFFLCL(id,strid,sev,dest1,dest2,ckptid,rc,filename,lineno)  ncs_logmsg(NCS_SERVICE_ID_CPND, \
              CPND_LID_TIFFFLCL,strid,NCSFL_LC_HEADLINE,sev,NCSFL_TYPE_TIFFFLCL,id,(DOUBLE)dest1,(DOUBLE)dest2, \
              (DOUBLE)ckptid,rc,filename,lineno)

#define m_LOG_CPND_CFCL(id,strid,sev,str1,ckptid,filename,lineno)  ncs_logmsg(NCS_SERVICE_ID_CPND, \
              CPND_LID_TICFCL,strid,NCSFL_LC_HEADLINE,sev,NCSFL_TYPE_TICFCL,id,str1,(DOUBLE)ckptid, \
              filename,lineno)

#define m_LOG_CPND_FLCL(id,strid,sev,ckptid,rc,filename,lineno)  ncs_logmsg(NCS_SERVICE_ID_CPND, \
              CPND_LID_TIFLCL,strid,NCSFL_LC_HEADLINE,sev,NCSFL_TYPE_TIFLCL,id,(DOUBLE)ckptid,rc, \
              filename,lineno)

#define m_LOG_CPND_CCL(id,strid,sev,ckptname,filename,lineno) ncs_logmsg(NCS_SERVICE_ID_CPND,CPND_LID_TICCL, \
              strid,NCSFL_LC_HEADLINE,sev,NCSFL_TYPE_TICCL,id,ckptname,filename,lineno)

#endif
