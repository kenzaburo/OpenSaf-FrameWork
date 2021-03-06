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
  
  GLND Timer related definitions.
    
******************************************************************************
*/

#ifndef GLND_TMR_H
#define GLND_TMR_H

#define GLND_AGENT_INFO_GET_TIMEOUT  1000000000
struct glnd_cb_tag;

/* timer type enums */
typedef enum glnd_tmr_type {
	GLND_TMR_RES_LOCK_REQ_TIMEOUT = 1,
	GLND_TMR_RES_NM_LOCK_REQ_TIMEOUT,
	GLND_TMR_RES_NM_UNLOCK_REQ_TIMEOUT,
	GLND_TMR_RES_REQ_TIMEOUT,
	GLND_TMR_AGENT_INFO_GET_WAIT,
	GLND_TMR_MAX
} GLND_TMR_TYPE;

/* GLND Timer definition */
typedef struct glnd_tmr {
	tmr_t tmr_id;
	GLND_TMR_TYPE type;	/* timer type */
	uns32 cb_hdl;		/* cb hdl to retrieve the GLND cb ptr */
	uns32 opq_hdl;		/* hdl to retrive the timer context */
	NCS_BOOL is_active;
} GLND_TMR;

/*** Extern function declarations ***/

EXTERN_C void glnd_tmr_exp(void *);

EXTERN_C uns32 glnd_start_tmr(struct glnd_cb_tag *, GLND_TMR *, GLND_TMR_TYPE, SaTimeT, uns32);

EXTERN_C void glnd_stop_tmr(GLND_TMR *);

#endif   /* !GLND_TMR_H */
