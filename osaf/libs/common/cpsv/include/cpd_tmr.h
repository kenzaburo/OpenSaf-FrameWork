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

#ifndef CPD_TMR_H
#define CPD_TMR_H

#include "ncssysf_tmr.h"

typedef enum cpd_tmr_type {
	CPD_TMR_TYPE_CPND_RETENTION = 1,
	CPD_TMR_TYPE_MAX = CPD_TMR_TYPE_CPND_RETENTION,
} CPD_TMR_TYPE;

typedef struct cpd_tmr {
	CPD_TMR_TYPE type;
	tmr_t tmr_id;
	uns32 uarg;
	NCS_BOOL is_active;
	union {
		MDS_DEST cpnd_dest;
	} info;
} CPD_TMR;

EXTERN_C uns32 cpd_tmr_start(CPD_TMR *tmr, uns32 duration);
EXTERN_C void cpd_timer_expiry(NCSCONTEXT uarg);
EXTERN_C void cpd_tmr_stop(CPD_TMR *tmr);

#endif
