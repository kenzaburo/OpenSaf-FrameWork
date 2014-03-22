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

#ifndef CPA_PROC_H
#define CPA_PROC_H

/* Call back Types */
typedef enum cpa_callback_type {
	CPA_CALLBACK_TYPE_OPEN = 1,
	CPA_CALLBACK_TYPE_SYNC,
	CPA_CALLBACK_TYPE_ARRIVAL_NTFY,
	CPA_CALLBACK_TYPE_MAX = CPA_CALLBACK_TYPE_SYNC
} CPA_CALLBACK_TYPE;

/* Info required for Call back */
typedef struct cpa_callback_info {
	struct cpa_callback_info *next;	/* This is required, as this struct 
					   is posted to mailbox */
	CPA_CALLBACK_TYPE type;
	SaCkptCheckpointHandleT lcl_ckpt_hdl;
	SaInvocationT invocation;
	SaCkptIOVectorElementT *ioVector;
	SaUint32T num_of_elmts;
	SaAisErrorT sa_err;
} CPA_CALLBACK_INFO;

EXTERN_C uns32 cpa_process_evt(CPA_CB *cb, CPSV_EVT *evt);
EXTERN_C uns32 cpa_version_validate(SaVersionT *version);
EXTERN_C uns32 cpa_open_attr_validate(const SaCkptCheckpointCreationAttributesT
				      *checkpointCreationAttributes,
				      SaCkptCheckpointOpenFlagsT checkpointOpenFlags, const SaNameT *checkpointName);

EXTERN_C uns32 cpa_callback_ipc_init(CPA_CLIENT_NODE *client_info);
EXTERN_C void cpa_callback_ipc_destroy(CPA_CLIENT_NODE *client_info);
EXTERN_C uns32 cpa_ckpt_finalize_proc(CPA_CB *cb, CPA_CLIENT_NODE *cl_node);
EXTERN_C uns32 cpa_proc_shm_open(CPA_CB *cb, CPA_GLOBAL_CKPT_NODE *gc_node,
				 const SaNameT *ckpt_name, SaSizeT ckpt_size);
EXTERN_C uns32 cpa_proc_shm_close(CPA_GLOBAL_CKPT_NODE *gc_node);

EXTERN_C uns32 cpa_proc_build_data_access_evt(const SaCkptIOVectorElementT *ioVector,
					      uns32 numberOfElements, uns32 data_access_type,
					      SaSizeT maxSectionSize, SaUint32T *errflag, CPSV_CKPT_DATA **ckpt_data);

EXTERN_C void cpa_proc_free_cpsv_ckpt_data(CPSV_CKPT_DATA *ckpt_data);

EXTERN_C void cpa_proc_free_arrival_ntfy_cpsv_ckpt_data(CPSV_CKPT_DATA *ckpt_data);

EXTERN_C uns32 cpa_proc_check_iovector(CPA_CB *cb, CPA_LOCAL_CKPT_NODE *lc_node,
				       const SaCkptIOVectorElementT *iovector, uns32 num_of_elmts, uns32 *errflag);

EXTERN_C uns32 cpa_proc_replica_read(CPA_CB *cb, SaUint32T numberOfElements,
				     SaCkptCheckpointHandleT gbl_ckpt_hdl,
				     SaCkptIOVectorElementT **ioVector, CPSV_ND2A_READ_MAP *read_map,
				     SaUint32T **erroneousVectorIndex);

EXTERN_C uns32 cpa_proc_rmt_replica_read(SaUint32T numberOfElements,
					 SaCkptIOVectorElementT *ioVector, CPSV_ND2A_READ_DATA *read_data,
					 SaUint32T **erroneousVectorIndex, SaVersionT *version);

EXTERN_C void cpa_proc_free_read_data(CPSV_ND2A_DATA_ACCESS_RSP *rmt_read_rsp);

EXTERN_C void cpa_cb_dump(void);

EXTERN_C uns32 cpa_open_name_validate(const SaNameT *checkpointName);

/* callback prototypes */
EXTERN_C CPA_CALLBACK_INFO *cpa_callback_ipc_rcv(CPA_CLIENT_NODE *cl_node);
EXTERN_C uns32 cpa_hdl_callbk_dispatch_one(CPA_CB *cb, SaCkptHandleT ckptHandle);
EXTERN_C uns32 cpa_hdl_callbk_dispatch_all(CPA_CB *cb, SaCkptHandleT ckptHandle);
EXTERN_C uns32 cpa_hdl_callbk_dispatch_block(CPA_CB *cb, SaCkptHandleT ckptHandle);
EXTERN_C void cpa_sync_with_cpd_for_active_replica_set(CPA_GLOBAL_CKPT_NODE *gc_node);

#endif
