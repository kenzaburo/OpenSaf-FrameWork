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

#ifndef MQD_TMR_H
#define MQD_TMR_H

typedef enum mqdq_tmr_type {
	MQD_TMR_TYPE_RETENTION = 1,
	MQD_ND_TMR_TYPE_EXPIRY,
	MQD_CTRL_EVT_TMR_EXPIRY,
	MQD_TMR_TYPE_MAX = MQD_CTRL_EVT_TMR_EXPIRY
} MQD_TMR_TYPE;

typedef struct mqd_tmr {
	MQD_TMR_TYPE type;
	tmr_t tmr_id;
	NODE_ID nodeid;
	uns32 uarg;
	NCS_BOOL is_active;
	NCS_BOOL is_expired;
} MQD_TMR;

EXTERN_C uns32 mqd_tmr_start(MQD_TMR *tmr, SaTimeT duration);
EXTERN_C void mqd_tmr_stop(MQD_TMR *tmr);
EXTERN_C void mqd_timer_expiry(NCSCONTEXT uarg);

#endif
