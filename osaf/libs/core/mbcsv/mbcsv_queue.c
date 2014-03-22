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
..............................................................................

..............................................................................

  DESCRIPTION:

  This file contains the routines to handle the callback queue mgmt

..............................................................................

  FUNCTIONS INCLUDED in this module:
   mbcsv_client_cleanup_mbx      - Clean up mailbox.
   mbcsv_client_queue_init       - Init queue.
   mbcsv_client_queue_destroy    - Destroy queue.

******************************************************************************
*/

#include "mbcsv.h"

/****************************************************************************
  PROCEDURE     : mbcsv_client_queue_init
  
  Description   : This routine is used to initialize the queue for the client.
 
  Arguments     : mbc_reg - pointer to the MBCSV registration instance.
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 mbcsv_client_queue_init(MBCSV_REG *mbc_reg)
{
	if (m_NCS_IPC_CREATE(&mbc_reg->mbx) == NCSCC_RC_SUCCESS) {
		if (m_NCS_IPC_ATTACH(&mbc_reg->mbx) == NCSCC_RC_SUCCESS) {
			return NCSCC_RC_SUCCESS;
		}
		m_NCS_IPC_RELEASE(&mbc_reg->mbx, NULL);
	}

	return m_MBCSV_DBG_SINK_SVC(NCSCC_RC_FAILURE, "Failed to create MBCSv mailbox", mbc_reg->svc_id);
}

/****************************************************************************
  PROCEDURE     : mbcsv_client_cleanup_mbx
  
  Description   : This routine is used to destroy the queue.
 
  Arguments     : arg - Call back function argument.
                  msg - Message from the mailbox..

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
NCS_BOOL mbcsv_client_cleanup_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	MBCSV_EVT *node = (MBCSV_EVT *)msg;

	/* deallocate the nodes */
	if (NULL != node) {
		m_MMGR_FREE_MBCSV_EVT(node);
	}
	return TRUE;
}

/****************************************************************************
  PROCEDURE     : mbcsv_client_queue_destroy
  
  Description   : This routine is used to destroy the queue for the client.
 
  Arguments     : mbc_reg - pointer to the MBCSV registration instance.
                   
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
void mbcsv_client_queue_destroy(MBCSV_REG *mbc_reg)
{
	/* detach the mail box */
	m_NCS_IPC_DETACH(&mbc_reg->mbx, mbcsv_client_cleanup_mbx, (NCSCONTEXT)mbc_reg);

	/* delete the mailbox */
	m_NCS_IPC_RELEASE(&mbc_reg->mbx, NULL);

	mbc_reg->mbx = 0;

}
