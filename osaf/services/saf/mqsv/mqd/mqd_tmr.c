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
  FILE NAME: mqd_tmr.c

  DESCRIPTION: MQD Timer Processing Routines

******************************************************************************/

#include "mqd.h"
/****************************************************************************
 * Name          : mqd_timer_expiry
 *
 * Description   : This function which is registered with the OS tmr function,
 *                 which will post a message to the corresponding mailbox 
 *                 depending on the component type.
 *
 *****************************************************************************/
void mqd_timer_expiry(NCSCONTEXT uarg)
{
	MQD_TMR *tmr = (MQD_TMR *)uarg;
	NCS_IPC_PRIORITY priority = NCS_IPC_PRIORITY_HIGH;
	MQD_CB *cb;
	MQSV_EVT *evt = 0;
	uns32 mqd_hdl;

	if (tmr != NULL) {
		mqd_hdl = tmr->uarg;

		if (tmr->is_active)
			tmr->is_active = FALSE;
		/* Destroy the timer if it exists.. */
		if (tmr->tmr_id != TMR_T_NULL) {
			m_NCS_TMR_DESTROY(tmr->tmr_id);
			tmr->tmr_id = TMR_T_NULL;
		}

		/* post a message to the corresponding component */
		if ((cb = (MQD_CB *)ncshm_take_hdl(NCS_SERVICE_ID_MQD, mqd_hdl))
		    != NULL) {
			evt = m_MMGR_ALLOC_MQSV_EVT(NCS_SERVICE_ID_MQD);
			if (evt == NULL) {
				m_LOG_MQSV_D(MQD_MEMORY_ALLOC_FAIL, NCSFL_LC_TIMER, NCSFL_SEV_ERROR, 0, __FILE__,
					     __LINE__);
				return;
			}
			memset(evt, 0, sizeof(MQSV_EVT));

			evt->type = MQSV_EVT_MQD_CTRL;
			evt->msg.mqd_ctrl.type = MQD_MSG_TMR_EXPIRY;
			evt->msg.mqd_ctrl.info.tmr_info.nodeid = tmr->nodeid;
			evt->msg.mqd_ctrl.info.tmr_info.type = tmr->type;

			/* Post the event to MQD Thread */
			m_NCS_IPC_SEND(&cb->mbx, evt, priority);

			ncshm_give_hdl(mqd_hdl);
		}
	}
	return;
}

/****************************************************************************
 * Name          : mqd_tmr_start
 *
 * Description   : This function which is used to start the MQD Timer
 *
 *****************************************************************************/
uns32 mqd_tmr_start(MQD_TMR *tmr, SaTimeT duration)
{
	m_LOG_MQSV_D(MQD_TMR_START, NCSFL_LC_TIMER, NCSFL_SEV_NOTICE, duration, __FILE__, __LINE__);
	if (tmr->tmr_id == TMR_T_NULL) {
		m_NCS_TMR_CREATE(tmr->tmr_id, duration, mqd_timer_expiry, (void *)tmr);
	}

	if (tmr->is_active == FALSE) {
		m_NCS_TMR_START(tmr->tmr_id, (uns32)duration, mqd_timer_expiry, (void *)tmr);
		tmr->is_active = TRUE;
	}

	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : mqd_tmr_stop
 *
 * Description   : This function which is used to stop the MQD Timer
 *
 * Arguments     : tmr      - Timer needs to be stoped.
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void mqd_tmr_stop(MQD_TMR *tmr)
{
	m_LOG_MQSV_D(MQD_TMR_STOPPED, NCSFL_LC_TIMER, NCSFL_SEV_NOTICE, 1, __FILE__, __LINE__);
	if (tmr->is_active == TRUE) {
		m_NCS_TMR_STOP(tmr->tmr_id);
		tmr->is_active = FALSE;
	}
	if (tmr->tmr_id != TMR_T_NULL) {
		m_NCS_TMR_DESTROY(tmr->tmr_id);
		tmr->tmr_id = TMR_T_NULL;
	}
	return;
}
