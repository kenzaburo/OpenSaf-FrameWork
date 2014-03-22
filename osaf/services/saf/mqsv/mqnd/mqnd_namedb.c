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
  FILE NAME: mqnd_namedb.c

  DESCRIPTION: MQND Queue Name Data base Updation routines.

******************************************************************************/

#include "mqnd.h"

/****************************************************************************
 * Name          : mqnd_qname_node_get
 *
 * Description   : Function to get the queue node from Tree.
 *
 * Arguments     : MQND_CB *cb, - MQND Control Block
 *                 SaNameT qname - queue handle
 *                 
 * Return Values : MQND_QNAME_NODE** o_qnode - Queu Node at MQND
 *
 * Notes         : None.
 *****************************************************************************/
void mqnd_qname_node_get(MQND_CB *cb, SaNameT qname, MQND_QNAME_NODE **o_qnode)
{
	*o_qnode = NULL;
	if (cb->is_qname_db_up) {
		if (qname.length)
			qname.length = m_HTON_SANAMET_LEN(qname.length);
		*o_qnode = (MQND_QNAME_NODE *)ncs_patricia_tree_get(&cb->qname_db, (uns8 *)&qname);
	}
	return;
}

/****************************************************************************
 * Name          : mqnd_qname_node_getnext
 *
 * Description   : Function to get the queue node from Tree.
 *
 * Arguments     : MQND_CB *cb, - MQND Control Block
 *                 SaNameT qname- queue name
 *                 
 * Return Values : MQND_QNAME_NODE** o_qnode
 *
 * Notes         : None.
 *****************************************************************************/
void mqnd_qname_node_getnext(MQND_CB *cb, SaNameT qname, MQND_QNAME_NODE **o_qnode)
{
	if (cb->is_qname_db_up) {
		if (qname.length) {
			qname.length = m_HTON_SANAMET_LEN(qname.length);
			*o_qnode = (MQND_QNAME_NODE *)ncs_patricia_tree_getnext(&cb->qname_db, (uns8 *)&qname);
		} else
			*o_qnode = (MQND_QNAME_NODE *)ncs_patricia_tree_getnext(&cb->qname_db, (uns8 *)NULL);
	}
	return;
}

/****************************************************************************
 * Name          : mqnd_qname_node_add
 *
 * Description   : Function to Add the qname node from Tree.
 *
 * Arguments     : MQND_CB *cb, - MQND Control Block
 *                 SaNameT qname -queue name
 *                 
 * Return Values : MQND_QNAME_NODE** o_qnode - QueueName Node at MQND
 *
 * Notes         : None.
 *****************************************************************************/
uns32 mqnd_qname_node_add(MQND_CB *cb, MQND_QNAME_NODE *qnode)
{
	uns32 rc = NCSCC_RC_FAILURE;
	qnode->qname.length = m_HTON_SANAMET_LEN(qnode->qname.length);
	qnode->pnode.key_info = (uns8 *)&qnode->qname;

	if (cb->is_qname_db_up)
		rc = ncs_patricia_tree_add(&cb->qname_db, (NCS_PATRICIA_NODE *)qnode);
	return rc;
}

/****************************************************************************
 * Name          : mqnd_qname_node_del
 *
 * Description   : Function to delete the queuename node from Tree.
 *
 * Arguments     : MQND_CB *cb, - MQND Control Block
 *                 SaNameT qname- queue name
 *                 
 * Return Values : MQND_QNAME_NODE** o_qnode - Queu Name Node at MQND
 *
 * Notes         : None.
 *****************************************************************************/
uns32 mqnd_qname_node_del(MQND_CB *cb, MQND_QNAME_NODE *qnode)
{
	uns32 rc = NCSCC_RC_FAILURE;

	if (cb->is_qname_db_up && qnode) {
		rc = ncs_patricia_tree_del(&cb->qname_db, (NCS_PATRICIA_NODE *)qnode);
	}
	return rc;
}
