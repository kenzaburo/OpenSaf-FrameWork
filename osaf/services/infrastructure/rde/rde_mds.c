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

#include <logtrace.h>
#include <mds_papi.h>
#include <ncsencdec_pub.h>

#include <rde_cb.h>

#define RDE_MDS_PVT_SUBPART_VERSION 1

static MDS_DEST peer_dest;
static MDS_HDL mds_hdl;

static uns32 msg_encode(MDS_CALLBACK_ENC_INFO *enc_info)
{
	struct rde_msg *msg;
	NCS_UBAID *uba;
	uns8 *data;

	enc_info->o_msg_fmt_ver = 1;
	uba = enc_info->io_uba;
	msg = (struct rde_msg *)enc_info->i_msg;

	data = ncs_enc_reserve_space(uba, sizeof(uns32));
	assert(data);
	ncs_encode_32bit(&data, msg->type);
	ncs_enc_claim_space(uba, sizeof(uns32));

	switch (msg->type) {
	case RDE_MSG_PEER_INFO_REQ:
	case RDE_MSG_PEER_INFO_RESP:
		data = ncs_enc_reserve_space(uba, sizeof(uns32));
		assert(data);
		ncs_encode_32bit(&data, msg->info.peer_info.ha_role);
		ncs_enc_claim_space(uba, sizeof(uns32));
		break;

	default:
		assert(0);
		break;
	}

	return NCSCC_RC_SUCCESS;
}

static uns32 msg_decode(MDS_CALLBACK_DEC_INFO *dec_info)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	struct rde_msg *msg;
	NCS_UBAID *uba;
	uns8 *data;
	uns8 data_buff[256];

	if (dec_info->i_fr_svc_id != NCSMDS_SVC_ID_RDE) {
			rc = NCSCC_RC_FAILURE;
			goto done;
	}

	if (dec_info->i_msg_fmt_ver != 1) {
			rc = NCSCC_RC_FAILURE;
			goto done;
	}

	msg = malloc(sizeof(*msg));
	assert(msg);

	dec_info->o_msg = msg;
	uba = dec_info->io_uba;

	data = ncs_dec_flatten_space(uba, data_buff, sizeof(uns32));
	assert(data);
	msg->type = ncs_decode_32bit(&data);
	ncs_dec_skip_space(uba, sizeof(uns32));

	switch (msg->type) {
	case RDE_MSG_PEER_INFO_REQ:
	case RDE_MSG_PEER_INFO_RESP:
		data = ncs_dec_flatten_space(uba, data_buff, sizeof(uns32));
		assert(data);
		msg->info.peer_info.ha_role = ncs_decode_32bit(&data);
		ncs_dec_skip_space(uba, sizeof(uns32));
		break;

	default:
		assert(0);
		break;
	}

done:
	return rc;
}

static int mbx_send(RDE_MSG_TYPE type, MDS_DEST fr_dest, NODE_ID fr_node_id)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	struct rde_msg *msg = calloc(1, sizeof(struct rde_msg));
	RDE_CONTROL_BLOCK *cb = rde_get_control_block();

	msg->type = type;
	msg->fr_dest = fr_dest;
	msg->fr_node_id = fr_node_id;

	if (ncs_ipc_send(&cb->mbx, (NCS_IPC_MSG *)msg, NCS_IPC_PRIORITY_HIGH) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_ipc_send FAILED");
		free(msg);
		rc = NCSCC_RC_FAILURE;
	}

	return rc;
}

static uns32 mds_callback(struct ncsmds_callback_info *info)
{
	struct rde_msg *msg;
	uns32 rc = NCSCC_RC_SUCCESS;
	RDE_CONTROL_BLOCK *cb = rde_get_control_block();

	switch (info->i_op) {
	case MDS_CALLBACK_COPY:
		rc = NCSCC_RC_FAILURE;
		break;
	case MDS_CALLBACK_ENC:
		rc = msg_encode(&info->info.enc);
		break;
	case MDS_CALLBACK_DEC:
		rc = msg_decode(&info->info.dec);
		break;
	case MDS_CALLBACK_ENC_FLAT:
		break;
	case MDS_CALLBACK_DEC_FLAT:
		break;
	case MDS_CALLBACK_RECEIVE:
		if (!peer_dest) {
			/* Sometimes a message from peer is received before MDS_UP, simulate MDS_UP */
			TRACE("generating up msg from rec event!");
			rc = mbx_send(RDE_MSG_PEER_UP, info->info.receive.i_fr_dest, info->info.receive.i_node_id);
		}

		msg = (struct rde_msg*)info->info.receive.i_msg;
		msg->fr_dest = info->info.receive.i_fr_dest;
		msg->fr_node_id = info->info.receive.i_node_id;
		if (ncs_ipc_send(&cb->mbx, (NCS_IPC_MSG *)info->info.receive.i_msg, NCS_IPC_PRIORITY_NORMAL) != NCSCC_RC_SUCCESS) {
			LOG_ER("ncs_ipc_send FAILED");
			free(msg);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		break;
	case MDS_CALLBACK_SVC_EVENT:
		if (rde_my_node_id == info->info.svc_evt.i_node_id)
			goto done;

		if (info->info.svc_evt.i_change == NCSMDS_DOWN) {
			TRACE("MDS DOWN dest: %llx, node ID: %x, svc_id: %d",
				info->info.svc_evt.i_dest, info->info.svc_evt.i_node_id, info->info.svc_evt.i_svc_id);
			peer_dest = 0;
			rc = mbx_send(RDE_MSG_PEER_DOWN, info->info.svc_evt.i_dest, info->info.svc_evt.i_node_id);
		} else if (info->info.svc_evt.i_change == NCSMDS_UP) {
			TRACE("MDS UP dest: %llx, node ID: %x, svc_id: %d",
				info->info.svc_evt.i_dest, info->info.svc_evt.i_node_id, info->info.svc_evt.i_svc_id);
			peer_dest = info->info.svc_evt.i_dest;
			rc = mbx_send(RDE_MSG_PEER_UP, info->info.svc_evt.i_dest, info->info.svc_evt.i_node_id);
		} else {
			TRACE("MDS %u dest: %llx, node ID: %x, svc_id: %d", info->info.svc_evt.i_change,
				info->info.svc_evt.i_dest, info->info.svc_evt.i_node_id, info->info.svc_evt.i_svc_id);
		}

		break;
	case MDS_CALLBACK_QUIESCED_ACK:
		break;
	case MDS_CALLBACK_DIRECT_RECEIVE:
		break;
	default:
		break;
	}
done:
	return rc;
}

uns32 rde_mds_register(RDE_CONTROL_BLOCK *cb)
{
	NCSADA_INFO ada_info;
	NCSMDS_INFO svc_info;
	MDS_SVC_ID svc_id[1] = { NCSMDS_SVC_ID_RDE };
	MDS_DEST mds_adest;

	TRACE_ENTER();

	ada_info.req = NCSADA_GET_HDLS;
	if (ncsada_api(&ada_info) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: NCSADA_GET_HDLS Failed", __FUNCTION__);
		return NCSCC_RC_FAILURE;
	}

	mds_hdl = ada_info.info.adest_get_hdls.o_mds_pwe1_hdl;
	mds_adest = ada_info.info.adest_get_hdls.o_adest;

	svc_info.i_mds_hdl = mds_hdl;
	svc_info.i_svc_id = NCSMDS_SVC_ID_RDE;
	svc_info.i_op = MDS_INSTALL;

	svc_info.info.svc_install.i_yr_svc_hdl = 0;
	svc_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;	/*node specific */
	svc_info.info.svc_install.i_svc_cb = mds_callback;	/* callback */
	svc_info.info.svc_install.i_mds_q_ownership = FALSE;
	svc_info.info.svc_install.i_mds_svc_pvt_ver = RDE_MDS_PVT_SUBPART_VERSION;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("%s: MDS Install Failed", __FUNCTION__);
		return NCSCC_RC_FAILURE;
	}

	memset(&svc_info, 0, sizeof(NCSMDS_INFO));
	svc_info.i_mds_hdl = mds_hdl;
	svc_info.i_svc_id = NCSMDS_SVC_ID_RDE;
	svc_info.i_op = MDS_RED_SUBSCRIBE;
	svc_info.info.svc_subscribe.i_num_svcs = 1;
	svc_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	svc_info.info.svc_subscribe.i_svc_ids = svc_id;

	if (ncsmds_api(&svc_info) == NCSCC_RC_FAILURE) {
		LOG_ER("MDS Subscribe for redundancy Failed");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE2("NodeId:%x, mds_adest:%llx", ncs_get_node_id(), mds_adest);

	return NCSCC_RC_SUCCESS;
}

uns32 rde_mds_send(struct rde_msg *msg, MDS_DEST to_dest)
{
	NCSMDS_INFO info;
	uns32 rc;

	TRACE("Sending %s to %llx", rde_msg_name[msg->type], to_dest);
	memset(&info, 0, sizeof(info));

	info.i_mds_hdl = mds_hdl;
	info.i_op = MDS_SEND;
	info.i_svc_id = NCSMDS_SVC_ID_RDE;
	
	info.info.svc_send.i_msg = msg;
	info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
	info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_RDE;
	info.info.svc_send.info.snd.i_to_dest = to_dest;
	
	rc = ncsmds_api(&info);
	if (NCSCC_RC_FAILURE == rc)
		LOG_ER("rde async MDS send FAILED");

	return rc;
}

