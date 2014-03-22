/*      OpenSAF 
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

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <configmake.h>

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <logtrace.h>

#include "smfd.h"
#include "smfd_smfnd.h"
#include "smfsv_defs.h"
#include "smfsv_evt.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

typedef struct SmfndNodeT {
	struct SmfndNodeT *next;
	SaClmClusterNodeT clmInfo;
	MDS_DEST dest;
} SmfndNodeT;

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

static SmfndNodeT *firstSmfnd = NULL;
static SaVersionT clmVersion = { 'B', 1, 1 };

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

/**
 * smfnd_dest_for_name
 * @param  i_nodeName Clm node name
 */
MDS_DEST smfnd_dest_for_name(const char *i_nodeName)
{
	SmfndNodeT *smfnd = firstSmfnd;

	while (smfnd != NULL) {
		if (!strcmp((char *)smfnd->clmInfo.nodeName.value, i_nodeName)) {
			return smfnd->dest;
		}
		smfnd = smfnd->next;
	}

	return 0;
}

/**
 * get_smfnd
 * @param i_node_id Clm node id
 */
SmfndNodeT *get_smfnd(SaClmNodeIdT i_node_id)
{
	SmfndNodeT *smfnd = firstSmfnd;

	while (smfnd != NULL) {
		if (smfnd->clmInfo.nodeId == i_node_id) {
			return smfnd;
		}
		smfnd = smfnd->next;
	}

	return NULL;
}

/**
 * smfnd_up
 * @param i_node_id Clm node id
 * @param i_smfnd_dest Destination to Clm node
 */
uns32 smfnd_up(SaClmNodeIdT i_node_id, MDS_DEST i_smfnd_dest)
{
	SaAisErrorT rc;
	SaClmHandleT clmHandle;
	SmfndNodeT *smfnd = NULL;

	TRACE("SMFND UP for node id %x", i_node_id);

	/* Find Clm info about the node */
	rc = saClmInitialize(&clmHandle, NULL, &clmVersion);
	if (rc != SA_AIS_OK) {
		LOG_ER("saClmInitialize failed %d", rc);
		return NCSCC_RC_FAILURE;
	}

	/* Make shure the node id does not already exists */
	smfnd = get_smfnd(i_node_id);

	if (smfnd != NULL) {
		LOG_ER("smfnd already exists %x", i_node_id);
		return NCSCC_RC_FAILURE;
	}

	/* Does not exists, create a new cleared structure */
	smfnd = calloc(1, sizeof(SmfndNodeT));
	if (smfnd == NULL) {
		LOG_ER("alloc failed");
		return NCSCC_RC_FAILURE;
	}

	/* Get Clm info about the node */
	rc = saClmClusterNodeGet(clmHandle, i_node_id,
				 10000000000LL, &smfnd->clmInfo);
	if (rc != SA_AIS_OK) {
		LOG_ER("saClmClusterNodeGet failed %d", rc);
		free(smfnd);
		return NCSCC_RC_FAILURE;
	}

	rc = saClmFinalize(clmHandle);
	if (rc != SA_AIS_OK) {
		LOG_ER("saClmFinalize failed %d", rc);
		free(smfnd);
		return NCSCC_RC_FAILURE;
	}

	/* Make shure the name string is null terminated */
	smfnd->clmInfo.nodeName.value[smfnd->clmInfo.nodeName.length] = 0;

	/* Store the destination to the smfnd */
	smfnd->dest = i_smfnd_dest;

	/* Store the node info in the smfnd list */
	smfnd->next = firstSmfnd;
	firstSmfnd = smfnd;
	TRACE("SMFND added for node name %s, id %x",
	      smfnd->clmInfo.nodeName.value, smfnd->clmInfo.nodeId);

	return NCSCC_RC_SUCCESS;
}

/**
 * smfnd_down
 * @param i_node_id Clm node id
 */
uns32 smfnd_down(SaClmNodeIdT i_node_id)
{
	SmfndNodeT *smfnd = firstSmfnd;
	SmfndNodeT *previous = firstSmfnd;

	TRACE("SMFND DOWN for node id %x", i_node_id);

	/* Clear the node info */
	while (smfnd != NULL) {
		if (smfnd->clmInfo.nodeId == i_node_id) {
			if (smfnd == firstSmfnd) {
				firstSmfnd = smfnd->next;
			} else {
				previous->next = smfnd->next;
			}

			TRACE("SMFND removed for node name %s, id %x",
			      smfnd->clmInfo.nodeName.value,
			      smfnd->clmInfo.nodeId);
			free(smfnd);
			return NCSCC_RC_SUCCESS;
		}

		previous = smfnd;
		smfnd = smfnd->next;
	}

	LOG_ER("node id %x not found for removal", i_node_id);
	return NCSCC_RC_FAILURE;
}

/**
 * smfnd_remote_cmd
 * @param i_cmd Remote command to be executed
 * @param i_smfnd_dest Destination to the node where to execute
 *                     the command
 * @param i_timeout Max time the command may take
 */
int smfnd_remote_cmd(const char *i_cmd, MDS_DEST i_smfnd_dest, uns32 i_timeout)
{
	SMFSV_EVT cmd_req;
	SMFSV_EVT *cmd_rsp;
	uns32 rc;

	cmd_req.type = SMFSV_EVT_TYPE_SMFND;
	cmd_req.info.smfnd.type = SMFND_EVT_CMD_REQ;
	cmd_req.info.smfnd.event.cmd_req.cmd = (char *)i_cmd;
	cmd_req.info.smfnd.event.cmd_req.cmd_len = strlen(i_cmd);

	rc = smfsv_mds_msg_sync_send(smfd_cb->mds_handle,
				     NCSMDS_SVC_ID_SMFND,
				     i_smfnd_dest,
				     NCSMDS_SVC_ID_SMFD,
				     &cmd_req, &cmd_rsp, i_timeout);

	if (rc != NCSCC_RC_SUCCESS) {
		return -1;
	}

	return cmd_rsp->info.smfd.event.cmd_rsp.result;
}
