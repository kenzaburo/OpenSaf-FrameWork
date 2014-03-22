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

/*****************************************************************************
  DESCRIPTION:
  
  This file consists of SMFSV routines used for encoding/decoding pointer
  structures in an SMFSV_EVT.
*****************************************************************************/

#include <ncsencdec_pub.h>
#include <logtrace.h>

#include "smfsv_evt.h"


/****************************************************************************
   Name          : smfsv_evt_destroy
   
   Description   : This is the function which is called to destroy an event.
   
   Arguments     : SMFSV_EVT *
   
   Return Values : NONE
   
   Notes         : None.
   
   @param evt
******************************************************************************/
void smfsv_evt_destroy(SMFSV_EVT *evt)
{
    assert(evt != NULL);
    free(evt);
}


/****************************************************************************
  SMFD SMFD SMFD SMFD SMFD SMFD SMFD SMFD SMFD SMFD SMFD SMFD SMFD SMFD SMFD 
*****************************************************************************/
/****************************************************************************\
 PROCEDURE NAME : smfd_enc_cmd_rsp

 DESCRIPTION    : Encodes the contents of SMFD_CMD_RSP into userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                   *o_ub - User Buff.

 RETURNS        : None
\*****************************************************************************/
uns32 smfd_enc_cmd_rsp(SMFSV_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;

    if (o_ub == NULL || i_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    /** encode the exit code **/
    p8 = ncs_enc_reserve_space(o_ub, 4);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_32bit(&p8, i_evt->info.smfd.event.cmd_rsp.result);
    ncs_enc_claim_space(o_ub, 4);

    return rc;
err:
    return NCSCC_RC_FAILURE;
}


/****************************************************************************\
 PROCEDURE NAME : smfd_dec_cmd_rsp

 DESCRIPTION    : Decodes the contents of SMFND_CMD_RSP from userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                   *o_ub - User Buff.

 RETURNS        : None
\*****************************************************************************/
uns32 smfd_dec_cmd_rsp(NCS_UBAID *i_ub, SMFSV_EVT *o_evt)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;
    uns8       local_data[20];

    if (i_ub == NULL || o_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    /** decode the exit code **/
    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->info.smfd.event.cmd_rsp.result = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);

    return rc;
err:
    return NCSCC_RC_FAILURE;
}

uns32 smf_enc_cbk_rsp(SMF_RESP_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;

    p8 = ncs_enc_reserve_space(o_ub, sizeof(SMF_RESP_EVT));
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_64bit(&p8, i_evt->inv_id);
    ncs_encode_32bit(&p8, i_evt->err);

    ncs_enc_claim_space(o_ub, sizeof(SMF_RESP_EVT));

    return rc;
err:
    return NCSCC_RC_FAILURE;
}

/****************************************************************************\
 PROCEDURE NAME : smfd_enc_cbk_rsp

 DESCRIPTION    : Encodes the contents of SMFD_CBK_RSP into userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                   *o_ub - User Buff.

 RETURNS        : None
\*****************************************************************************/
uns32 smfd_enc_cbk_rsp(SMFSV_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;

    if (o_ub == NULL || i_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    p8 = ncs_enc_reserve_space(o_ub, 4);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_32bit(&p8, i_evt->info.smfd.event.cbk_rsp.evt_type);
    ncs_enc_claim_space(o_ub, 4);

    rc = smf_enc_cbk_rsp(&i_evt->info.smfd.event.cbk_rsp.evt.resp_evt, o_ub);
    return rc;
err:
    return NCSCC_RC_FAILURE;
}

uns32 smf_dec_cbk_rsp(NCS_UBAID *i_ub, SMF_RESP_EVT *o_evt)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;
    uns8       local_data[256];

    if (i_ub == NULL || o_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    p8 =  ncs_dec_flatten_space(i_ub, local_data, 8);
    o_evt->inv_id = ncs_decode_64bit(&p8);
    ncs_dec_skip_space(i_ub, 8);

    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->err = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);

    return rc;
err:
    return NCSCC_RC_FAILURE;
}
/****************************************************************************\
 PROCEDURE NAME : smfd_dec_cbk_rsp

 DESCRIPTION    : Decodes the contents of SMFD_CBK_RSP from userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                   *o_ub - User Buff.

 RETURNS        : None
\*****************************************************************************/
uns32 smfd_dec_cbk_rsp(NCS_UBAID *i_ub, SMFSV_EVT *o_evt)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;
    uns8       local_data[256];

    if (i_ub == NULL || o_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->info.smfd.event.cbk_rsp.evt_type = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);

    rc = smf_dec_cbk_rsp(i_ub, &o_evt->info.smfd.event.cbk_rsp.evt.resp_evt);
    return rc;
err:
    return NCSCC_RC_FAILURE;
}


/****************************************************************************\
 PROCEDURE NAME : smfd_evt_enc

 DESCRIPTION    : Encodes the contents of SMFD_EVT into userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                   *o_ub - User Buff.

 RETURNS        : None
\*****************************************************************************/
uns32 smfd_evt_enc(SMFSV_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;

    if (o_ub == NULL || i_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    /** encode the type of SMFD event **/
    p8 = ncs_enc_reserve_space(o_ub, 4);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_32bit(&p8, i_evt->info.smfd.type);
    ncs_enc_claim_space(o_ub, 4);

    switch (i_evt->info.smfd.type)
    {
        case SMFD_EVT_CMD_RSP:
        {
            rc = smfd_enc_cmd_rsp(i_evt, o_ub);
            break;
        }
	case SMFD_EVT_CBK_RSP:
	{
	    rc = smfd_enc_cbk_rsp(i_evt, o_ub);
	    break;
	}
        default:
        {
            LOG_ER("Unknown SMFND evt type = %d", i_evt->info.smfd.type);
            goto err;
            break;
        }
    }

    return rc;

err:
    return NCSCC_RC_FAILURE;
}

/****************************************************************************\
 PROCEDURE NAME : smfd_evt_dec

 DESCRIPTION    : Decodes the contents of SMFD_EVT from userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uns32 smfd_evt_dec(NCS_UBAID *i_ub, SMFSV_EVT *o_evt)
{
    uns32     rc = NCSCC_RC_SUCCESS;
    uns8      local_data[20];
    uns8      *p8;

    /* Decode SMFD event type */ 

    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->info.smfd.type = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);

    switch (o_evt->info.smfd.type)
    {
        case SMFD_EVT_CMD_RSP:
        {
            rc = smfd_dec_cmd_rsp (i_ub, o_evt);
            break;
        }
        case SMFD_EVT_CBK_RSP:
        {
            rc = smfd_dec_cbk_rsp (i_ub, o_evt);
            break;
        }
        default:
        {
            LOG_ER("Unknown evt type = %d", o_evt->info.smfd.type);
            goto err;
            break;
        }
    }
    return rc;

err:
    return NCSCC_RC_FAILURE;
}


/****************************************************************************
  SMFND SMFND SMFND SMFND SMFND SMFND SMFND SMFND SMFND SMFND SMFND SMFND 
*****************************************************************************/

/****************************************************************************\
 PROCEDURE NAME : smfnd_enc_cmd_req

 DESCRIPTION    : Encodes the contents of SMFND_CMD_REQ into userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                   *o_ub - User Buff.

 RETURNS        : None
\*****************************************************************************/
uns32 smfnd_enc_cmd_req(SMFSV_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;

    if (o_ub == NULL || i_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    /** encode the cmd length **/
    p8 = ncs_enc_reserve_space(o_ub, 4);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_32bit(&p8, i_evt->info.smfnd.event.cmd_req.cmd_len);
    ncs_enc_claim_space(o_ub, 4);

    /** encode the cmd **/
    ncs_encode_n_octets_in_uba(o_ub, (uns8*) i_evt->info.smfnd.event.cmd_req.cmd, i_evt->info.smfnd.event.cmd_req.cmd_len);

    return rc;
err:
    return NCSCC_RC_FAILURE;
}


/****************************************************************************\
 PROCEDURE NAME : smfnd_dec_cmd_req

 DESCRIPTION    : Decodes the contents of SMFND_CMD_REQ from userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                   *o_ub - User Buff.

 RETURNS        : None
\*****************************************************************************/
uns32 smfnd_dec_cmd_req(NCS_UBAID *i_ub, SMFSV_EVT *o_evt)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;
    uns8       local_data[20];

    if (i_ub == NULL || o_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    /** decode the cmd length **/
    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->info.smfnd.event.cmd_req.cmd_len = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);

    /** decode the cmd **/
    p8 =  ncs_dec_flatten_space(i_ub, local_data, o_evt->info.smfnd.event.cmd_req.cmd_len);
    o_evt->info.smfnd.event.cmd_req.cmd = NULL; /* In case len 0 */

    if (o_evt->info.smfnd.event.cmd_req.cmd_len != 0)
    {
        char*      cmd;

    	cmd = malloc(o_evt->info.smfnd.event.cmd_req.cmd_len + 1); /* + 1 for NULL termination */
        if (cmd == NULL)
        {
            LOG_ER("malloc == NULL");
            goto err;
        }

        ncs_decode_n_octets_from_uba(i_ub,(uns8 *)cmd, o_evt->info.smfnd.event.cmd_req.cmd_len);
        cmd[o_evt->info.smfnd.event.cmd_req.cmd_len] = 0; /* NULL terminate */
        o_evt->info.smfnd.event.cmd_req.cmd = cmd;
    }

    return rc;
err:
    return NCSCC_RC_FAILURE;
}

uns32 smf_enc_cbk_req(SMF_CBK_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;

    p8 = ncs_enc_reserve_space(o_ub, 8);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_64bit(&p8, i_evt->inv_id);
    ncs_enc_claim_space(o_ub, 8);

    p8 = ncs_enc_reserve_space(o_ub, 4);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_32bit(&p8, i_evt->scope_id);
    ncs_enc_claim_space(o_ub, 4);

    p8 = ncs_enc_reserve_space(o_ub, 4);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_32bit(&p8, i_evt->object_name.length);
    ncs_enc_claim_space(o_ub, 4);
    
    ncs_encode_n_octets_in_uba(o_ub, (uns8*) i_evt->object_name.value, 
				i_evt->object_name.length);
    //ncs_enc_claim_space(o_ub, i_evt->object_name.length);

    p8 = ncs_enc_reserve_space(o_ub, 4);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_32bit(&p8, i_evt->camp_phase);
    ncs_enc_claim_space(o_ub, 4);

    p8 = ncs_enc_reserve_space(o_ub, 8);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_64bit(&p8, i_evt->cbk_label.labelSize);
    ncs_enc_claim_space(o_ub, 8);
    
    ncs_encode_n_octets_in_uba(o_ub, (uns8*) i_evt->cbk_label.label, 
				i_evt->cbk_label.labelSize);

    p8 = ncs_enc_reserve_space(o_ub, 4);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_32bit(&p8, i_evt->params_len);
    ncs_enc_claim_space(o_ub, 4);

   if (i_evt->params_len != 0) {
	    ncs_encode_n_octets_in_uba(o_ub, (uns8*) i_evt->params, 
					i_evt->params_len);
    }
    return rc;
err:
    return NCSCC_RC_FAILURE;
}

uns32 smfnd_enc_cbk_req(SMF_CBK_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32      rc = NCSCC_RC_SUCCESS;

    rc = smf_enc_cbk_req(i_evt, o_ub);
    return rc;
}

uns32 smfnd_enc_cbk_rsp(SMF_RESP_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32      rc = NCSCC_RC_SUCCESS;

    if (o_ub == NULL || i_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    rc = smf_enc_cbk_rsp(i_evt, o_ub);
    return rc;
err:
    return NCSCC_RC_FAILURE;
}

/****************************************************************************\
 PROCEDURE NAME : smfnd_enc_cbk_req_rsp

 DESCRIPTION    : Encodes the contents of SMF_EVT into userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                   *o_ub - User Buff.

 RETURNS        : None
\*****************************************************************************/
uns32 smfnd_enc_cbk_req_rsp(SMFSV_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;

    if (o_ub == NULL || i_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    p8 = ncs_enc_reserve_space(o_ub, 4);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_32bit(&p8, i_evt->info.smfnd.event.cbk_req_rsp.evt_type);
    ncs_enc_claim_space(o_ub, 4);
    
    switch(i_evt->info.smfnd.event.cbk_req_rsp.evt_type) {
	case SMF_CLBK_EVT:
	{
		rc = smfnd_enc_cbk_req(&i_evt->info.smfnd.event.cbk_req_rsp.evt.cbk_evt, o_ub);
		break;
	}	
	case SMF_RSP_EVT:
	{
		rc = smfnd_enc_cbk_rsp(&i_evt->info.smfnd.event.cbk_req_rsp.evt.resp_evt, o_ub);
		break;
	}
	default:
            LOG_ER("Unknown SMF_EVT_TYPE evt type = %d", i_evt->info.smfnd.event.cbk_req_rsp.evt_type);
            goto err;
		break;
    }

    return rc;
err:
    return NCSCC_RC_FAILURE;
}

uns32 smf_dec_cbk_req(NCS_UBAID *i_ub, SMF_CBK_EVT *o_evt)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;
    uns8       local_data[256];

    p8 =  ncs_dec_flatten_space(i_ub, local_data, 8);
    o_evt->inv_id = ncs_decode_64bit(&p8);
    ncs_dec_skip_space(i_ub, 8);

    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->scope_id = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);

    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->object_name.length = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);

    if (o_evt->object_name.length != 0)
    {
        ncs_decode_n_octets_from_uba(i_ub,(uns8 *)o_evt->object_name.value, o_evt->object_name.length);
    }

    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->camp_phase = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);

    p8 =  ncs_dec_flatten_space(i_ub, local_data, 8);
    o_evt->cbk_label.labelSize = ncs_decode_64bit(&p8);
    ncs_dec_skip_space(i_ub, 8);
    o_evt->cbk_label.label = NULL; /* In case len 0 */

    if (o_evt->cbk_label.labelSize != 0)
    {
        char*      label;

    	label = malloc(o_evt->cbk_label.labelSize + 1); /* + 1 for NULL termination */
        if (label == NULL)
        {
            LOG_ER("malloc == NULL");
            goto err;
        }

        ncs_decode_n_octets_from_uba(i_ub,(uns8 *)label, o_evt->cbk_label.labelSize);
        label[o_evt->cbk_label.labelSize] = 0; /* NULL terminate */
        o_evt->cbk_label.label = (unsigned char *)label;
    }

    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->params_len = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);

    if (o_evt->params_len != 0)
    {
        char*      str;

    	str = malloc(o_evt->params_len + 1); /* + 1 for NULL termination */
        if (str == NULL)
        {
            LOG_ER("malloc == NULL");
            goto err;
        }

        ncs_decode_n_octets_from_uba(i_ub,(uns8 *)str, o_evt->params_len);
        str[o_evt->params_len] = 0; /* NULL terminate */
        o_evt->params = str;
    }
    return rc;
err:
    return NCSCC_RC_FAILURE;
}

uns32 smfnd_dec_cbk_req(NCS_UBAID *i_ub, SMF_CBK_EVT *o_evt)
{
    uns32      rc = NCSCC_RC_SUCCESS;

    rc = smf_dec_cbk_req(i_ub, o_evt);
    return rc;
}

uns32 smfnd_dec_cbk_rsp(NCS_UBAID *i_ub, SMF_RESP_EVT *o_evt)
{
    uns32      rc = NCSCC_RC_SUCCESS;

    if (i_ub == NULL || o_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    rc = smf_dec_cbk_rsp(i_ub, o_evt);
    return rc;
err:
    return NCSCC_RC_FAILURE;
}

/****************************************************************************\
 PROCEDURE NAME : smfnd_dec_cbk_req_rsp

 DESCRIPTION    : Decodes the contents of SMFND_CBK_REQ_RSP from userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                   *o_ub - User Buff.

 RETURNS        : None
\*****************************************************************************/
uns32 smfnd_dec_cbk_req_rsp(NCS_UBAID *i_ub, SMFSV_EVT *o_evt)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;
    uns8       local_data[20];

    if (i_ub == NULL || o_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    /** decode the cmd length **/
    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->info.smfnd.event.cbk_req_rsp.evt_type = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);

    switch (o_evt->info.smfnd.event.cbk_req_rsp.evt_type) {
	case SMF_CLBK_EVT:
	{
		rc = smfnd_dec_cbk_req(i_ub, &o_evt->info.smfnd.event.cbk_req_rsp.evt.cbk_evt);
		break;
	}
	case SMF_RSP_EVT:
	{
		rc = smfnd_dec_cbk_rsp(i_ub, &o_evt->info.smfnd.event.cbk_req_rsp.evt.resp_evt);
		break;
	}
    }

    return rc;
err:
    return NCSCC_RC_FAILURE;
}


/****************************************************************************\
 PROCEDURE NAME : smfnd_evt_enc

 DESCRIPTION    : Encodes the contents of SMFND_EVT into userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                   *o_ub - User Buff.

 RETURNS        : None
\*****************************************************************************/
uns32 smfnd_evt_enc(SMFSV_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;

    if (o_ub == NULL || i_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    /** encode the type of SMFND event **/
    p8 = ncs_enc_reserve_space(o_ub, 4);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_32bit(&p8, i_evt->info.smfnd.type);
    ncs_enc_claim_space(o_ub, 4);

    switch (i_evt->info.smfnd.type)
    {
        case SMFND_EVT_CMD_REQ:
        {
            rc = smfnd_enc_cmd_req(i_evt, o_ub);
            break;
        }
        case SMFND_EVT_CBK_RSP:
        {
            rc = smfnd_enc_cbk_req_rsp(i_evt, o_ub);
            break;
        }
        default:
        {
            LOG_ER("Unknown SMFND evt type = %d", i_evt->info.smfnd.type);
            goto err;
            break;
        }
    }

    return rc;
err:
    return NCSCC_RC_FAILURE;
}

/****************************************************************************\
 PROCEDURE NAME : smfnd_evt_dec

 DESCRIPTION    : Decodes the contents of SMFND_EVT from userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uns32 smfnd_evt_dec(NCS_UBAID *i_ub, SMFSV_EVT *o_evt)
{
    uns32     rc = NCSCC_RC_SUCCESS;
    uns8      local_data[20];
    uns8      *p8;

    /* Decode SMFND event type */ 

    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->info.smfnd.type = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);

    switch (o_evt->info.smfnd.type)
    {
        case SMFND_EVT_CMD_REQ:
        {
            rc = smfnd_dec_cmd_req (i_ub, o_evt);
            break;
        }
        case SMFND_EVT_CBK_RSP:
        {
            rc = smfnd_dec_cbk_req_rsp (i_ub, o_evt);
            break;
        }
        default:
        {
            LOG_ER("Unknown evt type = %d", o_evt->info.smfnd.type);
            goto err;
            break;
        }
    }
    return rc;

err:
    return NCSCC_RC_FAILURE;
}

/****************************************************************************
  SMFA SMFA SMFA SMFA SMFA SMFA SMFA SMFA SMFA SMFA SMFA SMFA SMFA SMFA SMFA
*****************************************************************************/

uns32 smfa_enc_cbk_req(SMFSV_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;

    p8 = ncs_enc_reserve_space(o_ub, 4);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_32bit(&p8, i_evt->info.smfa.event.cbk_req_rsp.evt_type);
    ncs_enc_claim_space(o_ub, 4);
    
    rc = smf_enc_cbk_req(&i_evt->info.smfa.event.cbk_req_rsp.evt.cbk_evt, o_ub);
    return rc;
err:
    return NCSCC_RC_FAILURE;
}

uns32 smfa_dec_cbk_req(NCS_UBAID *i_ub, SMFSV_EVT *o_evt)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8      local_data[20];
    uns8      *p8;

    /* Decode SMFA event type */ 

    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->info.smfa.event.cbk_req_rsp.evt_type = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);


    rc = smf_dec_cbk_req(i_ub, &o_evt->info.smfa.event.cbk_req_rsp.evt.cbk_evt);
    return rc;
}

/****************************************************************************\
 PROCEDURE NAME : smfa_evt_enc

 DESCRIPTION    : Encodes the contents of SMFA_EVT into userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                   *o_ub - User Buff.

 RETURNS        : None
\*****************************************************************************/
uns32 smfa_evt_enc(SMFSV_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;

    /** encode the type of SMFA event **/
    p8 = ncs_enc_reserve_space(o_ub, 4);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_32bit(&p8, i_evt->info.smfa.type);
    ncs_enc_claim_space(o_ub, 4);

    switch (i_evt->info.smfa.type)
    {
        case SMFA_EVT_CBK:
	{
		rc = smfa_enc_cbk_req(i_evt, o_ub);
		break;
	}
	default:
        {
            LOG_ER("Unknown evt type = %d", i_evt->info.smfa.type);
            goto err;
            break; /* not required */
        }
    }

    return rc; 
err:
    return NCSCC_RC_FAILURE;
}

/****************************************************************************\
 PROCEDURE NAME : smfa_evt_dec

 DESCRIPTION    : Decodes the contents of SMFA_EVT from userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uns32 smfa_evt_dec(NCS_UBAID *i_ub, SMFSV_EVT *o_evt)
{
    uns32     rc = NCSCC_RC_SUCCESS;
    uns8      local_data[20];
    uns8      *p8;

    /* Decode SMFA event type */ 

    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->info.smfa.type = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);

    switch (o_evt->info.smfa.type)
    {
        case SMFA_EVT_CBK:
	{
		rc = smfa_dec_cbk_req(i_ub, o_evt);
		break;
	}
        default:
        {
            LOG_ER("Unknown evt type = %d", o_evt->info.smfa.type);
            goto err;
            break; /* not required */
        }
    }
    return rc;

err:
    return NCSCC_RC_FAILURE;
}


/****************************************************************************
  SMFSV SMFSV SMFSV SMFSV SMFSV SMFSV SMFSV SMFSV SMFSV SMFSV SMFSV SMFSV 
*****************************************************************************/

/****************************************************************************\
 PROCEDURE NAME : smfsv_evt_enc_flat

 DESCRIPTION    : Encodes the contents of SMFSV_EVT into userbuf
                  The top level is encode flat (without byte-order correction).
                  Levels below the top are encoded with byter order correction
                  (because we dont really save much space/execution by
                  encoding the bits and pieces of the sub-level as flat).

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uns32 smfsv_evt_enc_flat(SMFSV_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32     size;
   
    size = sizeof(SMFSV_EVT);

    /* Encode the Top level evt without byte-order correction. */
    ncs_encode_n_octets_in_uba(o_ub,(uns8*)i_evt,size);

    return NCSCC_RC_SUCCESS;
}


/****************************************************************************\
 PROCEDURE NAME : smfsv_evt_dec_flat

 DESCRIPTION    : Decodes the contents of SMFSV_EVT from user buf
                  Inverse of smfsv_evt_enc_flat.

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uns32 smfsv_evt_dec_flat(NCS_UBAID *i_ub, SMFSV_EVT *o_evt)
{
    uns32     size;
   
    size = sizeof(SMFSV_EVT);
   
    /* Decode the Top level evt without byte order correction */
    ncs_decode_n_octets_from_uba(i_ub,(uns8*)o_evt, size);

    return NCSCC_RC_SUCCESS;
}
                      

/****************************************************************************\
 PROCEDURE NAME : smfsv_evt_enc

 DESCRIPTION    : Encodes the contents of SMFSV_EVT into userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                   *o_ub  - User Buff.

 RETURNS        : None
\*****************************************************************************/
uns32 smfsv_evt_enc(SMFSV_EVT *i_evt, NCS_UBAID *o_ub)
{
    uns32      rc = NCSCC_RC_SUCCESS;
    uns8       *p8;

    if (o_ub == NULL || i_evt == NULL)
    {
        LOG_ER("indata == NULL");
        goto err;
    }

    /** encode the type of message **/
    p8 = ncs_enc_reserve_space(o_ub, 4);
    if (p8 == NULL)
    {
        LOG_ER("ncs_enc_reserve_space failed");
        goto err;
    }
    ncs_encode_32bit(&p8, i_evt->type);
    ncs_enc_claim_space(o_ub, 4);

    switch (i_evt->type)
    {
        case SMFSV_EVT_TYPE_SMFD:
        {
            rc = smfd_evt_enc (i_evt, o_ub);
            break;
        }
        case SMFSV_EVT_TYPE_SMFND:
        {
            rc = smfnd_evt_enc (i_evt, o_ub);
            break;
        }
        case SMFSV_EVT_TYPE_SMFA:
        {
            rc = smfa_evt_enc (i_evt, o_ub);
            break;
        }
        default:
        {
            LOG_ER("Unknown evt type = %d", i_evt->type);
            goto err;
            break;
        }
    }

    return rc;
err:
    return NCSCC_RC_FAILURE;
}

/****************************************************************************\
 PROCEDURE NAME : smfsv_evt_dec

 DESCRIPTION    : Decodes the contents of SMFSV_EVT from userbuf

 ARGUMENTS      : *i_evt - Event Struct.
                  *o_ub  - User Buff.

 RETURNS        : None

 NOTES          : 
\*****************************************************************************/
uns32 smfsv_evt_dec(NCS_UBAID *i_ub, SMFSV_EVT *o_evt)
{
    uns32     rc = NCSCC_RC_SUCCESS;
    uns8      local_data[20];
    uns8      *p8;

    /* Decode SMFSV event type */ 
    p8 =  ncs_dec_flatten_space(i_ub, local_data, 4);
    o_evt->type = ncs_decode_32bit(&p8);
    ncs_dec_skip_space(i_ub, 4);

    switch (o_evt->type)
    {
        case SMFSV_EVT_TYPE_SMFD:
        {
            rc = smfd_evt_dec (i_ub, o_evt);
            break;
        }
        case SMFSV_EVT_TYPE_SMFND:
        {
            rc = smfnd_evt_dec (i_ub, o_evt);
            break;
        }
        case SMFSV_EVT_TYPE_SMFA:
        {
            rc = smfa_evt_dec (i_ub, o_evt);
            break;
        }
        default:
        {
            LOG_ER("Unknown evt type = %d", o_evt->type);
            goto err;
            break;
        }
    }

    return rc;
err:
    return NCSCC_RC_FAILURE;
}

   
/****************************************************************************
 * Name          : smfsv_mds_send_rsp
 *
 * Description   : Send the Response to a Sync Requests
 *
 * Arguments     : 
 *
 * Return Values : 
 *
 * Notes         :
 *****************************************************************************/
uns32 smfsv_mds_send_rsp(uns32             mds_handle, 
                         MDS_SYNC_SND_CTXT mds_ctxt,
                         uns32             to_svc, 
                         MDS_DEST          to_dest, 
                         uns32             fr_svc, 
                         MDS_DEST          fr_dest, 
                         SMFSV_EVT         *evt)
{
   NCSMDS_INFO    mds_info;
   uns32          rc;

   memset(&mds_info, 0, sizeof(NCSMDS_INFO));
   mds_info.i_mds_hdl = mds_handle;
   mds_info.i_svc_id = fr_svc;
   mds_info.i_op = MDS_SEND;

   /* fill the send structure */
   mds_info.info.svc_send.i_msg = (NCSCONTEXT)evt;
   mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
   
   mds_info.info.svc_send.i_to_svc = to_svc;
   mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_RSP;
   mds_info.info.svc_send.info.rsp.i_msg_ctxt = mds_ctxt;
   mds_info.info.svc_send.info.rsp.i_sender_dest = to_dest;

   /* send the message */
   rc = ncsmds_api(&mds_info);
   if ( rc != NCSCC_RC_SUCCESS)
   {
      LOG_NO("Failed to send mds response message");
   }

   return rc;
}

/****************************************************************************
  Name          : smfsv_mds_msg_sync_send
 
  Description   : This routine sends the Sync requests from SMFSV
 
  Arguments     : cb  - ptr to the SMFSV CB
                  i_evt - ptr to sent SMFSV event
                  o_evt - ptr to response SMFSV message returned
                  timeout - timeout value in 10 ms 
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 smfsv_mds_msg_sync_send (uns32       mds_handle, 
                               uns32       to_svc, 
                               MDS_DEST    to_dest, 
                               uns32       fr_svc, 
                               SMFSV_EVT   *i_evt, 
                               SMFSV_EVT   **o_evt,
                               uns32       timeout)
{

   NCSMDS_INFO                mds_info;
   uns32                      rc;

   if(!i_evt)
      return NCSCC_RC_FAILURE;

   memset(&mds_info, 0, sizeof(NCSMDS_INFO));
   mds_info.i_mds_hdl = mds_handle;
   mds_info.i_svc_id = fr_svc;
   mds_info.i_op = MDS_SEND;

   /* fill the send structure */
   mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_evt;
   mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
   mds_info.info.svc_send.i_to_svc = to_svc;
   mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SNDRSP;

   /* fill the send rsp strcuture */
   mds_info.info.svc_send.info.sndrsp.i_time_to_wait = timeout; /* timeto wait in 10ms */
   mds_info.info.svc_send.info.sndrsp.i_to_dest = to_dest;

   /* send the message */
   rc = ncsmds_api(&mds_info);
   if ( rc == NCSCC_RC_SUCCESS)
      *o_evt = mds_info.info.svc_send.info.sndrsp.o_rsp;
   else
   {
      LOG_NO("Failed to send sync mds message");
   }

   return rc;
}


/****************************************************************************
  Name          : smfsv_mds_msg_send
 
  Description   : This routine sends the Events from SMFSV
 
  Arguments     : cb  - ptr to the SMFSV CB
                  evt - ptr to the SMFSV event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 smfsv_mds_msg_send (uns32        mds_handle, 
                          uns32        to_svc, 
                          MDS_DEST     to_dest, 
                          uns32        from_svc, 
                          SMFSV_EVT    *evt)
{
   NCSMDS_INFO     mds_info;
   uns32           rc;

   if(!evt)
      return NCSCC_RC_FAILURE;

   memset(&mds_info, 0, sizeof(NCSMDS_INFO));
   mds_info.i_mds_hdl = mds_handle;
   mds_info.i_svc_id = from_svc;
   mds_info.i_op = MDS_SEND;

   /* fill the send structure */
   mds_info.info.svc_send.i_msg = (NCSCONTEXT)evt;
   mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
   mds_info.info.svc_send.i_to_svc = to_svc;
   mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
   mds_info.info.svc_send.info.snd.i_to_dest = to_dest;

   /* send the message */
   rc = ncsmds_api(&mds_info);

   if ( rc != NCSCC_RC_SUCCESS)
   {
      LOG_NO("Failed to send mds message, rc = %d, SMFD DEST %llu", rc, to_dest);
   }

   return rc;
}
