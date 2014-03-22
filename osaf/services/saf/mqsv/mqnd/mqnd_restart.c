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
  FILE NAME: mqnd_restart.c

  DESCRIPTION: Functions to do the processing after MQND Restart.

******************************************************************************/

#include "mqnd.h"

static uns32 mqnd_restart_queue_node_add(MQND_CB *cb, MQND_QUEUE_NODE *qnode);
static SaAisErrorT mqnd_build_database_from_shm(MQND_CB *cb);
static SaAisErrorT mqnd_restart_ckpt_read(MQND_CB *cb, MQND_QUEUE_CKPT_INFO
					  *ckpt_queue_info, uns32 offset);
static void mqnd_remove_mqalist(MQND_CB *cb);
static void mqnd_fill_queue_node(MQND_QUEUE_CKPT_INFO *ckpt_queue_info, MQND_QUEUE_INFO *queue_info);
/****************************************************************************
 * Name          : mqnd_restart_init 
 *
 * Description   : Main function which will initialize the MQND after restart.
 *
 * Arguments     : MQND_CB *cb - MQND Control Block Pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS - Successfull execution.
 *                 NCSCC_RC_FAILURE - Failure
 *
 * Notes         : None.
 *****************************************************************************/

uns32 mqnd_restart_init(MQND_CB *cb)
{
	SaAisErrorT rc = NCSCC_RC_SUCCESS;

	if (cb->is_create_ckpt == FALSE) {
		/*Build the database */
		m_LOG_MQSV_ND(MQND_RESTART_INIT_OPEN_SUCCESS, NCSFL_LC_MQSV_INIT, NCSFL_SEV_NOTICE, SA_AIS_OK, __FILE__,
			      __LINE__);
		rc = mqnd_build_database_from_shm(cb);
		TRACE("After Building database");
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_MQSV_ND(MQND_RESTART_BUILD_DB_FROM_CKPTSVC_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR,
				      rc, __FILE__, __LINE__);
			/*Should the shared memory be deleted at this stage */
			return rc;
		}
		cb->is_restart_done = TRUE;
	} else
		cb->is_restart_done = TRUE;

	mqnd_remove_mqalist(cb);
	return rc;

}

/****************************************************************************
 * Name          : mqnd_add_node_to_mqalist
 *
 * Description   : Function to add the MQA node to the list of MQAs which are up
 * 
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *                 MDS_DEST dest - MDS_DEST of up MQA
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 mqnd_add_node_to_mqalist(MQND_CB *cb, MDS_DEST dest)
{
	MQND_MQA_LIST_NODE *mqa_node = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	mqa_node = m_MMGR_ALLOC_MQND_MQA_LIST_NODE;
	if (mqa_node == NULL) {
		/*Memory Failure Error */
		m_LOG_MQSV_ND(MQND_MQA_LNODE_ALLOC_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, SA_AIS_ERR_NO_MEMORY,
			      __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}
	memset(mqa_node, 0, sizeof(MQND_MQA_LIST_NODE));
	mqa_node->mqa_dest = dest;
	mqa_node->lnode.key = (uns8 *)&mqa_node->mqa_dest;

	rc = ncs_db_link_list_add(&cb->mqa_list_info, &mqa_node->lnode);
	if (rc != NCSCC_RC_SUCCESS)
		m_LOG_MQSV_ND(MQND_MQA_LNODE_ADD_FAILED, NCSFL_LC_MQSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__);

	return rc;
}

/****************************************************************************
 * Name          : mqnd_remove_mqalist
 *
 * Description   : Function to remove the mqa_listinfo from the MQND CB
 *
 * Arguments     : MQND_CB *cb - MQND CB pointer
 *
 * Return Values : None
 *
 *****************************************************************************/

static void mqnd_remove_mqalist(MQND_CB *cb)
{
	MQND_MQA_LIST_NODE *mqa_node = NULL;

	while ((mqa_node = (MQND_MQA_LIST_NODE *)ncs_db_link_list_dequeue(&cb->mqa_list_info))) {
		m_MMGR_FREE_MQND_MQA_LIST_NODE(mqa_node);
	}
}

/****************************************************************************
 * Name          : mqnd_build_database_from_shm
 *
 * Description   : Function to read the data from the shared memory that
                   was checkpointed
 *
 * Arguments     : MQND_CB *cb - MQND Control block pointer.
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/

static SaAisErrorT mqnd_build_database_from_shm(MQND_CB *cb)
{
	MQND_QUEUE_CKPT_INFO ckpt_queue_info;
	MQND_QUEUE_NODE *qnode = 0;
	SaAisErrorT rc = SA_AIS_OK;
	MQND_QUEUE_CKPT_INFO *shm_base_addr;
	uns32 i;

	shm_base_addr = cb->mqnd_shm.shm_base_addr;

	TRACE("Building Database");

	for (i = 0; i < cb->mqnd_shm.max_open_queues; i++) {
		if (shm_base_addr[i].valid == SHM_QUEUE_INFO_VALID) {
			/* Read the queue_info from shared memory and build the client tree */
			memset(&ckpt_queue_info, 0, sizeof(MQND_QUEUE_CKPT_INFO));
			rc = mqnd_restart_ckpt_read(cb, &ckpt_queue_info, i);

			qnode = m_MMGR_ALLOC_MQND_QUEUE_NODE;
			if (!qnode) {
				rc = SA_AIS_ERR_NO_MEMORY;
				return rc;
			}
			memset(qnode, 0, sizeof(MQND_QUEUE_NODE));

			mqnd_fill_queue_node(&ckpt_queue_info, &(qnode->qinfo));
			/* Check for the insertion and add the queue info to the patricia tree */
			mqnd_restart_queue_node_add(cb, qnode);
		} else if (shm_base_addr[i].valid)
			assert(0);
	}
	cb->is_restart_done = TRUE;
	return rc;
}

/****************************************************************************
 * Name          : mqnd_restart_ckpt_read
 *
 * Description   : Function to read the data from shared memory segment 
 *                 to  MQND Checkpoint Info Datastructure.
 *
 * Arguments     : MQND_CB *cb - MQND Control block pointer.
                   offset - Shared memory index pointing the queue info.
 *                 MQND_QUEUE_CKPT_INFO *ckpt_queue_info - Checkpoint Info Data
 *                 Structure Pointer.
 *
 * Return Values : Success/Failure
 *
 * Notes         : None.
 *****************************************************************************/

static SaAisErrorT mqnd_restart_ckpt_read(MQND_CB *cb, MQND_QUEUE_CKPT_INFO *ckpt_queue_info, uns32 offset)
{
	SaAisErrorT rc = SA_AIS_OK;
	NCS_OS_POSIX_SHM_REQ_INFO read_req;

	/*Use read option of shared memory to fill ckpt_queue_info */
	memset(&read_req, '\0', sizeof(NCS_OS_POSIX_SHM_REQ_INFO));

	read_req.type = NCS_OS_POSIX_SHM_REQ_READ;
	read_req.info.read.i_addr = cb->mqnd_shm.shm_base_addr;
	read_req.info.read.i_read_size = sizeof(MQND_QUEUE_CKPT_INFO);
	read_req.info.read.i_offset = offset * sizeof(MQND_QUEUE_CKPT_INFO);	/*check with srikant */
	read_req.info.read.i_to_buff = (MQND_QUEUE_CKPT_INFO *)ckpt_queue_info;

	rc = ncs_os_posix_shm(&read_req);

	return rc;
}

/****************************************************************************
 * Name          : mqnd_fill_queue_node
 *
 * Description   : Function to read the data from MQND Checkpoint Info Datastruc 
 *                 -ture to the MQND_QUEUE_INFO to before adding into the patricia tree.
 *
 * Arguments     : 
 *                 MQND_QUEUE_CKPT_INFO *ckpt_queue_info - MQND Checkpoint Info data Struture Pointer.
 *                 MQND_QUEUE_INFO *queue_info- MQND Queue Database Patricia node information Structure Pointer.                  
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/

static void mqnd_fill_queue_node(MQND_QUEUE_CKPT_INFO *ckpt_queue_info, MQND_QUEUE_INFO *queue_info)
{
	uns32 i = 0;

	queue_info->queueHandle = ckpt_queue_info->queueHandle;
	queue_info->listenerHandle = ckpt_queue_info->listenerHandle;
	queue_info->queueName = ckpt_queue_info->queueName;
	for (i = SA_MSG_MESSAGE_HIGHEST_PRIORITY; i <= SA_MSG_MESSAGE_LOWEST_PRIORITY; i++) {
		queue_info->numberOfFullErrors[i] = ckpt_queue_info->numberOfFullErrors[i];
		queue_info->size[i] = ckpt_queue_info->size[i];
		queue_info->queueStatus.saMsgQueueUsage[i].queueSize =
		    ckpt_queue_info->QueueStatsShm.saMsgQueueUsage[i].queueSize;
	}

	if (ckpt_queue_info->qtransfer_complete_tmr.type == MQND_TMR_TYPE_NODE2_QTRANSFER
	    && ckpt_queue_info->qtransfer_complete_tmr.is_active
	    && ckpt_queue_info->qtransfer_complete_tmr.tmr_id != NULL) {
		queue_info->qtransfer_complete_tmr.type = ckpt_queue_info->qtransfer_complete_tmr.type;
		queue_info->qtransfer_complete_tmr.uarg = ckpt_queue_info->qtransfer_complete_tmr.uarg;
		queue_info->qtransfer_complete_tmr.tmr_id = ckpt_queue_info->qtransfer_complete_tmr.tmr_id;
		queue_info->qtransfer_complete_tmr.is_active = ckpt_queue_info->qtransfer_complete_tmr.is_active;
		queue_info->qtransfer_complete_tmr.qhdl = ckpt_queue_info->qtransfer_complete_tmr.qhdl;
	}

	queue_info->queueStatus.creationFlags = ckpt_queue_info->creationFlags;
	queue_info->creationTime = ckpt_queue_info->creationTime;
	queue_info->queueStatus.retentionTime = ckpt_queue_info->retentionTime;
	queue_info->queueStatus.closeTime = ckpt_queue_info->closeTime;
	queue_info->sendingState = ckpt_queue_info->sendingState;
	queue_info->msgHandle = ckpt_queue_info->msgHandle;
	queue_info->owner_flag = ckpt_queue_info->owner_flag;
	queue_info->rcvr_mqa = ckpt_queue_info->rcvr_mqa;
	queue_info->q_key = ckpt_queue_info->q_key;
	queue_info->shm_queue_index = ckpt_queue_info->shm_queue_index;
}

/****************************************************************************
 * Name          : mqnd_restart_queue_node_add
 *
 * Description   : Function to add the MQND_QUEUE_NODE to the MQND Database after
 *                 checking  the retention timer value in case of non persistent 
 *                 Queue case.If the timer got expired then deregister it.
 *                 In case the MQA whcih owns the queue is down then also deregister the queue.  
 * Arguments     : MQND_CB *cb - MQND Control block pointer.
 *                 MQND_QUEUE_NODE *queue_info- MQND Queue Database Patricia node information Structure Pointer.
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 mqnd_restart_queue_node_add(MQND_CB *cb, MQND_QUEUE_NODE *qnode)
{
	/*Check for the Close Timer */
	SaTimeT presentTime, timeout = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* Add the queue to the MQND queue node, queue name DB */
	if (1) {
		MQND_QNAME_NODE *pnode = 0;

		/* Add the queue into qnode DB */
		mqnd_queue_node_add(cb, qnode);

		/* For unlinked queues, don't add it to name DB */
		if (!(qnode->qinfo.sendingState == MSG_QUEUE_UNAVAILABLE)) {

			/* IF not unlinked, add it to name DB */
			pnode = m_MMGR_ALLOC_MQND_QNAME_NODE;

			if (!pnode)
				return NCSCC_RC_FAILURE;

			memset(pnode, 0, sizeof(MQND_QNAME_NODE));
			pnode->qname = qnode->qinfo.queueName;
			pnode->qhdl = qnode->qinfo.queueHandle;
			mqnd_qname_node_add(cb, pnode);
		}
	}

	if (qnode->qinfo.qtransfer_complete_tmr.type == MQND_TMR_TYPE_NODE2_QTRANSFER
	    && qnode->qinfo.qtransfer_complete_tmr.is_active
	    && qnode->qinfo.qtransfer_complete_tmr.tmr_id != TMR_T_NULL) {
		rc = mqnd_tmr_start(&qnode->qinfo.qtransfer_complete_tmr, MQND_QTRANSFER_REQ_TIMER);
		if (rc == NCSCC_RC_SUCCESS)
			m_LOG_MQSV_ND(MQND_QTRANSFER_NODE2_TMR_STARTED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO,
				      qnode->qinfo.qtransfer_complete_tmr.qhdl, __FILE__, __LINE__);
		else
			m_LOG_MQSV_ND(MQND_QTRANSFER_NODE2_TMR_STARTED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_ERROR,
				      qnode->qinfo.qtransfer_complete_tmr.qhdl, __FILE__, __LINE__);
	}

	if ((!(qnode->qinfo.queueStatus.creationFlags & SA_MSG_QUEUE_PERSISTENT)) &&
	    (qnode->qinfo.queueStatus.closeTime)) {
		m_GET_TIME_STAMP(presentTime);
		/*Present Time - closeTime > Retention time then queue timer expired else                                                       start the a timer with the remaining time of expiry */
		if ((qnode->qinfo.queueStatus.retentionTime) >
		    (SA_TIME_ONE_SECOND * (presentTime - qnode->qinfo.queueStatus.closeTime))) {
			timeout =
			    (qnode->qinfo.queueStatus.retentionTime) -
			    (SA_TIME_ONE_SECOND * (presentTime - qnode->qinfo.queueStatus.closeTime));
			timeout = m_NCS_CONVERT_SATIME_TO_TEN_MILLI_SEC(timeout);
		}
		if (timeout) {
			/* Retention timer started before restart, not yet expired by now */
			qnode->qinfo.tmr.type = MQND_TMR_TYPE_RETENTION;
			qnode->qinfo.tmr.qhdl = qnode->qinfo.queueHandle;
			qnode->qinfo.tmr.uarg = cb->cb_hdl;
			qnode->qinfo.tmr.tmr_id = 0;
			qnode->qinfo.tmr.is_active = FALSE;

			rc = mqnd_tmr_start(&qnode->qinfo.tmr, timeout);
			m_LOG_MQSV_ND(MQND_RETENTION_TMR_STARTED, NCSFL_LC_MQSV_Q_MGMT, NCSFL_SEV_INFO, rc, __FILE__,
				      __LINE__);

		} else {
			MQSV_EVT evt;
			/* Retention timer started before restart, expired by now */
			memset(&evt, 0, sizeof(MQSV_EVT));

			evt.type = MQSV_EVT_MQND_CTRL;
			evt.msg.mqnd_ctrl.type = MQND_CTRL_EVT_TMR_EXPIRY;
			evt.msg.mqnd_ctrl.info.tmr_info.qhdl = qnode->qinfo.queueHandle;
			evt.msg.mqnd_ctrl.info.tmr_info.type = MQND_TMR_TYPE_RETENTION;

			/* Call the retention timer expiry processing */
			mqnd_evt_proc_tmr_expiry(cb, &evt);
			goto done;
		}
	}

	/* Agent that opened the queue is no more */
	if (1) {
		MQND_MQA_LIST_NODE *mqa_list_node = 0;
		SaAisErrorT err = 1;

		mqa_list_node = (MQND_MQA_LIST_NODE *)ncs_db_link_list_find(&cb->mqa_list_info,
									    (uns8 *)&qnode->qinfo.rcvr_mqa);
		if (mqa_list_node == NULL) {
			/* That MQA is down so close all the queues of that agent */
			mqnd_proc_queue_close(cb, qnode, &err);
		}
	}
 done:
	return rc;

}

/****************************************************************************
 * Name          : mqnd_cpy_qnodeinfo_to_ckptinfo
 *
 * Description   : To copy the MQND QUEUENODE Information to the Checkpoint Information to write to the Checkpoint.
 *
 * Arguments     : MQND_CB *cb
                   MQND_QUEUE_NODE *queue_info- MQND Data base node pointer.
 *                 MQND_QUEUE_CKPT_INFO *ckpt_queue_info- MQND checkpoint Node pointer.
 *
 * Return Values : 
 *
 * Notes         : None.
 *****************************************************************************/

void mqnd_cpy_qnodeinfo_to_ckptinfo(MQND_CB *cb, MQND_QUEUE_NODE *queue_info, MQND_QUEUE_CKPT_INFO *ckpt_queue_info)
{
	uns32 i = 0, offset;
	MQND_QUEUE_CKPT_INFO *shm_base_addr;

	shm_base_addr = cb->mqnd_shm.shm_base_addr;
	offset = queue_info->qinfo.shm_queue_index;

	ckpt_queue_info->queueHandle = queue_info->qinfo.queueHandle;
	ckpt_queue_info->listenerHandle = queue_info->qinfo.listenerHandle;
	ckpt_queue_info->queueName = queue_info->qinfo.queueName;
	for (i = SA_MSG_MESSAGE_HIGHEST_PRIORITY; i <= SA_MSG_MESSAGE_LOWEST_PRIORITY; i++) {
		ckpt_queue_info->numberOfFullErrors[i] = queue_info->qinfo.numberOfFullErrors[i];
		ckpt_queue_info->size[i] = queue_info->qinfo.size[i];
		ckpt_queue_info->QueueStatsShm.saMsgQueueUsage[i].queueSize =
		    queue_info->qinfo.queueStatus.saMsgQueueUsage[i].queueSize;
		/*For persistent queue the queue used stats are to be retained */
		ckpt_queue_info->QueueStatsShm.saMsgQueueUsage[i].queueUsed =
		    shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[i].queueUsed;
		ckpt_queue_info->QueueStatsShm.saMsgQueueUsage[i].numberOfMessages =
		    shm_base_addr[offset].QueueStatsShm.saMsgQueueUsage[i].numberOfMessages;
	}
	/*For persistent queue the queue used stats are to be retained */
	ckpt_queue_info->QueueStatsShm.totalNumberOfMessages =
	    shm_base_addr[offset].QueueStatsShm.totalNumberOfMessages;
	ckpt_queue_info->QueueStatsShm.totalQueueUsed = shm_base_addr[offset].QueueStatsShm.totalQueueUsed;

	if (queue_info->qinfo.qtransfer_complete_tmr.type == MQND_TMR_TYPE_NODE2_QTRANSFER
	    && queue_info->qinfo.qtransfer_complete_tmr.is_active
	    && queue_info->qinfo.qtransfer_complete_tmr.tmr_id != TMR_T_NULL) {
		ckpt_queue_info->qtransfer_complete_tmr.type = queue_info->qinfo.qtransfer_complete_tmr.type;
		ckpt_queue_info->qtransfer_complete_tmr.qhdl = queue_info->qinfo.qtransfer_complete_tmr.qhdl;
		ckpt_queue_info->qtransfer_complete_tmr.uarg = queue_info->qinfo.qtransfer_complete_tmr.uarg;
		ckpt_queue_info->qtransfer_complete_tmr.tmr_id = queue_info->qinfo.qtransfer_complete_tmr.tmr_id;
		ckpt_queue_info->qtransfer_complete_tmr.is_active = queue_info->qinfo.qtransfer_complete_tmr.is_active;
	}

	ckpt_queue_info->creationFlags = queue_info->qinfo.queueStatus.creationFlags;
	ckpt_queue_info->creationTime = queue_info->qinfo.creationTime;
	ckpt_queue_info->retentionTime = queue_info->qinfo.queueStatus.retentionTime;
	ckpt_queue_info->closeTime = queue_info->qinfo.queueStatus.closeTime;
	ckpt_queue_info->sendingState = queue_info->qinfo.sendingState;
	ckpt_queue_info->msgHandle = queue_info->qinfo.msgHandle;
	ckpt_queue_info->owner_flag = queue_info->qinfo.owner_flag;
	ckpt_queue_info->rcvr_mqa = queue_info->qinfo.rcvr_mqa;
	ckpt_queue_info->q_key = queue_info->qinfo.q_key;
	ckpt_queue_info->shm_queue_index = queue_info->qinfo.shm_queue_index;
	ckpt_queue_info->valid = SHM_QUEUE_INFO_VALID;
}

/****************************************************************************
 * Name          : mqnd_ckpt_queue_info_write 
 *
 * Description   : To checkpoint queue info to local shared memory of MQND
 *
 * Arguments     : MQND_CB - MQND control block
 *                 MQND_QUEUE_CKPT_INFO - MQND checkpoint Node pointer.
                   index - offset to the queue info to be written
 *
 * Return Values :
 *
 * Notes         : None.
 *****************************************************************************/

uns32 mqnd_ckpt_queue_info_write(MQND_CB *cb, MQND_QUEUE_CKPT_INFO *queue_ckpt_node, uns32 index)
{
	NCS_OS_POSIX_SHM_REQ_INFO queue_info_write;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&queue_info_write, '\0', sizeof(NCS_OS_POSIX_SHM_REQ_INFO));

	queue_info_write.type = NCS_OS_POSIX_SHM_REQ_WRITE;
	queue_info_write.info.write.i_addr = cb->mqnd_shm.shm_base_addr;
	queue_info_write.info.write.i_from_buff = queue_ckpt_node;
	queue_info_write.info.write.i_offset = index * sizeof(MQND_QUEUE_CKPT_INFO);
	queue_info_write.info.write.i_write_size = sizeof(MQND_QUEUE_CKPT_INFO);

	rc = ncs_os_posix_shm(&queue_info_write);
	if (rc != NCSCC_RC_SUCCESS) {
		/*log the error */
		return rc;
	}
	return rc;
}
