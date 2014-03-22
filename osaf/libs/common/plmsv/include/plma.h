/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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

  DESCRIPTION:
  This include file contains PLMA Control block and data structures used by PLM agent

******************************************************************************/
#ifndef PLMA_H
#define PLMA_H

#include "ncsgl_defs.h"
#include "ncs_main_papi.h"
#include "ncs_saf.h"
#include "ncs_lib.h"
#include "mds_papi.h"
#include "ncs_edu_pub.h"
#include "ncssysf_lck.h"

#include <saAis.h>
#include <saPlm.h>

#include "logtrace.h"

#include "plms_evt.h"

typedef struct plma_cb
{
	SaUint32T	cb_hdl;            /* CB hdl returned by hdl mngr */
	SaUint32T	plms_svc_up;   
	  
	/* mds parameters */
	MDS_HDL             mds_hdl;           /* mds handle */
	MDS_DEST            mdest_id;
	MDS_DEST            plms_mdest_id;     /*plms mdest_id */
	
	EDU_HDL edu_hdl;        /* edu handle used for encode/decode */
	
	NCS_SEL_OBJ         sel_obj;           /* sel obj for mds sync indication */
	NCS_PATRICIA_TREE   client_info;       /* PLMA_CLIENT_INFO */
	NCS_PATRICIA_TREE   entity_group_info;  /* PLMA_ENTITY_GROUP_INFO */
        NCS_LOCK            cb_lock;
	SaUint32T           plms_sync_awaited;
} PLMA_CB;

typedef  struct plma_client_info  PLMA_CLIENT_INFO;

typedef  struct plma_rdns_trk_mem_list
{
	SaPlmReadinessTrackedEntityT *entities;  /* Pointer to be freed */
	struct plma_rdns_trk_mem_list *next;
} PLMA_RDNS_TRK_MEM_LIST;

typedef  struct plma_entity_group_info
{
	NCS_PATRICIA_NODE         pat_node; 
	SaPlmEntityGroupHandleT   entity_group_handle;   /* Key field */
	SaUint32T		  trk_strt_stop;		
	PLMA_CLIENT_INFO          *client_info;   
	PLMA_RDNS_TRK_MEM_LIST	  *rdns_trk_mem_list;
	SaUint32T		  is_trk_enabled;
} PLMA_ENTITY_GROUP_INFO;

/* Data structure to group entity list per client */
typedef  struct plma_entity_group_info_list
{
	PLMA_ENTITY_GROUP_INFO              *entity_group_info;
	struct plma_entity_group_info_list   *next;
} PLMA_ENTITY_GROUP_INFO_LIST;

struct plma_client_info
{
	NCS_PATRICIA_NODE              pat_node; 
	SaPlmHandleT                   plm_handle;         /* Key field */
	/* Mailbox Queue to store the callback messages for the clients */
	SYSF_MBX                       callbk_mbx;
	SaPlmCallbacksT               track_callback;
	PLMA_ENTITY_GROUP_INFO_LIST    *grp_info_list;
} plma_client_info;

#define PLMA_MDS_PVT_SUBPART_VERSION 1
/*PLMA - PLMS communication */
#define PLMA_WRT_PLMS_SUBPART_VER_MIN 1
#define PLMA_WRT_PLMS_SUBPART_VER_MAX 1

#define PLMA_WRT_PLMS_SUBPART_VER_RANGE \
        (PLMA_WRT_PLMS_SUBPART_VER_MAX - \
	PLMA_WRT_PLMS_SUBPART_VER_MIN + 1 )
	

EXTERN_C PLMA_CB *plma_ctrlblk;


EXTERN_C uns32 ncs_plma_startup();
EXTERN_C uns32 ncs_plma_shutdown();

EXTERN_C void plma_callback_ipc_destroy(PLMA_CLIENT_INFO *client_info);
EXTERN_C uns32 plma_callback_ipc_init(PLMA_CLIENT_INFO *client_info);

/* MDS Public APIS */
EXTERN_C uns32 plma_mds_register();
EXTERN_C void plma_mds_unregister();
EXTERN_C uns32 plm_mds_msg_sync_send(MDS_HDL mds_hdl, uns32 from_svc, 
			uns32 to_svc, MDS_DEST to_dest, PLMS_EVT *i_evt,
			PLMS_EVT **o_evt,uns32 timeout);

EXTERN_C uns32 plms_mds_normal_send (MDS_HDL mds_hdl, NCSMDS_SVC_ID from_svc,
		NCSCONTEXT evt, MDS_DEST dest, NCSMDS_SVC_ID to_svc);

/* Function Declarations */
extern uns32 plms_edp_plms_evt(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
                        NCSCONTEXT ptr, uns32 *ptr_data_len,
                        EDU_BUF_ENV *buf_env, EDP_OP_TYPE op,
                        EDU_ERR *o_err);

EXTERN_C void clean_group_info_node(PLMA_ENTITY_GROUP_INFO *);
EXTERN_C void clean_client_info_node(PLMA_CLIENT_INFO *);
EXTERN_C uns32 plms_mds_enc(MDS_CALLBACK_ENC_INFO *info, EDU_HDL *edu_hdl);
EXTERN_C uns32 plms_mds_dec(MDS_CALLBACK_DEC_INFO *info, EDU_HDL *edu_hdl);
EXTERN_C uns32 plms_mds_enc_flat(struct ncsmds_callback_info *info, EDU_HDL *edu_hdl);
EXTERN_C uns32 plms_mds_dec_flat(struct ncsmds_callback_info *info, EDU_HDL *edu_hdl);
EXTERN_C SaUint32T  plms_free_evt(PLMS_EVT *evt);
#endif /* PLMA_H */
