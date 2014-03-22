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

  H&J InterProcess Communication Facility
  -------------------------------------
  Interprocess and Intraprocess Event Posting.  We use this for posting events
  (received pdu's, API requests) at sub-system  boundaries.  It is also used
  for timer expirations, and posting of events within subsystems.

******************************************************************************
*/

/** Module Inclusion Control...
 **/
#ifndef SYSF_IPC_H
#define SYSF_IPC_H

#include "ncssysf_ipc.h"
#include "ncssysfpool.h"

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

            H&J Internal Messaging Facility Definitions

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/**
 **
 ** Posting Events or IPC Messages.
 **
 ** The set of macros provided here can be modified at integration time to 
 ** make use of a Target System's IPC or Queueing facilities already in 
 ** existance.
 **
 **/

#define m_NCS_SM_IPC_ELEM_ADD(a)
#define m_NCS_SM_IPC_ELEM_DEL(a)
#define m_NCS_SM_IPC_ELEM_CUR_DEPTH_INC(a)
#define m_NCS_SM_IPC_ELEM_CUR_DEPTH_DEC(a)

typedef struct ncs_ipc_queue {
	NCS_IPC_MSG *head;
	NCS_IPC_MSG *tail;
} NCS_IPC_QUEUE;

typedef struct tag_ncs_ipc {
	NCS_LOCK queue_lock;	/* to protect access to queue */

	NCS_IPC_QUEUE queue[NCS_IPC_PRIO_LEVELS];	/* element 0 for queueing HIGH priority IPC messages */
	/* element 1 for queueing NORMAL priority IPC messages */
	/* element 2 for queueing LOW priority IPC messages */

	uns32 no_of_msgs[NCS_IPC_PRIO_LEVELS];	/* (priority level message count, used to compare 
						   with the corresponding threshold value) */

	uns32 max_no_of_msgs[NCS_IPC_PRIO_LEVELS];	/* (threshold value configured through 
							   m_NCS_IPC_CONFIG_MAX_MSGS otherwise initialized to zero) */
	uns32 *usr_counters[NCS_IPC_PRIO_LEVELS];	/* user given 32bit counters are accessed through these pointers */

	unsigned int active_queue;	/* the next queue to check for an IPC message */

	/* Changes to migrate to SAF model:PM:7/Jan/03. */
	NCS_SEL_OBJ sel_obj;
	/* msg_count: Keeps a count of messages queued. When this count is 
	   non-zero, "sel_obj" will have an "indication" raised against
	   it. The "indication" on "sel_obj" will be removed when
	   the count goes down to zero.

	   This way there need not be an indication raised for every
	   message. An indication is raised only per "burst of 
	   messages"
	 */
	uns32 msg_count;

	/* If "sel_obj" is put to use, the "sem_handle" member will be removed. 
	   For now it stays */
	void *sem_handle;	/* for blocking/waking IPC msg receiver */
	NCS_BOOL releasing;	/* flag from ncs_ipc_release to ncs_ipc_recv */
	uns32 ref_count;	/* reference count - number of instances attached
				 * to this IPC */
	char *name;		/* mbx task name */
} NCS_IPC;

#define m_NCS_SYSM_LM_OP_IPC_LBGN(info)   NCSCC_RC_FAILURE
#define m_NCS_SYSM_LM_OP_IPC_LTCY(info)   NCSCC_RC_FAILURE

#define m_NCS_SET_ST_QLAT()
#define m_NCS_SET_FN_QLAT()

#endif   /* SYSF_IPC_H */
