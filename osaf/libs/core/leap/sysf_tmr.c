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

  This module contains target system specific declarations related to
  System Timer services.

..............................................................................
*/

#include <ncsgl_defs.h>
#include "ncs_osprm.h"

#include "ncs_tasks.h"
#include "ncs_tmr.h"
#include "sysf_def.h"
#include "ncssysf_def.h"
#include "ncssysf_sem.h"
#include "ncssysf_tmr.h"
#include "ncssysf_tsk.h"
#include "ncspatricia.h"
#include "ncssysf_mem.h"

#ifndef SYSF_TMR_LOG
#define SYSF_TMR_LOG  0
#endif

#if (SYSF_TMR_LOG == 1)
#define m_SYSF_TMR_LOG_ERROR(str,num)   printf("%s::0x%x\n",(str),(unsigned int)(num))
#define m_SYSF_TMR_LOG_INFO(str,num)   printf("%s::0x%x\n",(str),(unsigned int)(num))
#else
#define m_SYSF_TMR_LOG_ERROR(str,num)
#define m_SYSF_TMR_LOG_INFO(str,num)
#endif

#define TIMESPEC_DIFF_IN_NS(later, before)  \
                  (((later).tv_sec - (before).tv_sec)*1000000000LL + \
                  (((later).tv_nsec - (before).tv_nsec)))
#ifndef ENABLE_SYSLOG_TMR_STATS
#define ENABLE_SYSLOG_TMR_STATS 0
#endif

/* This is the period of the timer tick we request from the OS. */
#define NCS_MILLISECONDS_PER_TICK    100

/* SYSF_TMR  state of a timer during its lifetime */
#define  TMR_STATE_CREATE          0x01
#define  TMR_STATE_START           0x02
#define  TMR_STATE_DORMANT         0x04
#define  TMR_STATE_DESTROY         0x08

#define  TMR_SET_STATE(t,s)  (t->state=s)
#define  TMR_TEST_STATE(t,s) (t->state&s)

/*         LEGAL TRANSITIONS (for this reference implementation)
 *
 *              +------------+
 *              |            |
 *              v   +---->expired(dormant)----+
 *  create--->start-+                         +-->destroy 
 *              ^   +----> stop(dormant)  ----+
 *              |            |
 *              +------------+
 */

/* SYSF_TMR the thing given to a create/start client */

/* Timer Debug data points to see who left timers dangling               */

typedef struct sysf_tmr_leak {
	uns16 isa;		/* validation marker for SYSF_TMR instances */
	char *file;		/* File name of CREATE or START operation   */
	uns32 line;		/* File line of CREATE or START operation   */
	uns32 stamp;		/* Marks loop count to help determine age   */

} TMR_DBG_LEAK;

#define TMR_ISA  0x3434		/* existence marker 44 in ASCII */

#define TMR_DBG_TICK(s)           (s.tick++)
#define TMR_DBG_STAMP(t,v)        (t->dbg.stamp=v)
#define TMR_DBG_SET(d,f,l)        {d.isa=TMR_ISA;d.file=f;d.line=l;}
#define TMR_DBG_ASSERT_ISA(d)     {if(d.isa!=TMR_ISA)m_LEAP_DBG_SINK_VOID;}
#define TMR_DBG_ASSERT_STATE(t,s) {if(!(t->state&s))m_LEAP_DBG_SINK_VOID;}

#if ENABLE_SYSLOG_TMR_STATS
typedef struct tmr_stats {
	uns32 cnt;
	uns32 ring_hwm;
} TMR_STATS;
#else
#define TMR_STATS  uns32
#endif

#define TMR_SET_CNT(s)
#define TMR_INC_CNT(s)
#define TMR_STAT_RING_HWM(s)
#define TMR_STAT_TMR_HWM(c,s)
#define TMR_STAT_STARTS(s)
#define TMR_STAT_EXPIRY(s)
#define TMR_STAT_CANCELLED(s)
#define TMR_STAT_FREE_HWM(s)
#define TMR_STAT_ADD_FREE(s)
#define TMR_STAT_RMV_FREE(s)
#define TMR_STAT_TTL_TMRS(s)

/* SYSF_TMR holds expiry info and state                                  */
typedef struct sysf_tmr {
	struct sysf_tmr *next;	/* Must be first field !!! */
	struct sysf_tmr *keep;	/* just to know where you are !! */

	uns8 state;
	uns64 key;
	TMR_CALLBACK tmrCB;
	NCSCONTEXT tmrUarg;
	TMR_DBG_LEAK dbg;

} SYSF_TMR;

/* SYSF_TMR_PAT_NODE holds timer list info available in a pat node */
typedef struct sysf_tmr_pat_node {
	NCS_PATRICIA_NODE pat_node;
	uns64 key;
	SYSF_TMR *tmr_list_start;
	SYSF_TMR *tmr_list_end;
} SYSF_TMR_PAT_NODE;

/* TMR_SAFE the part of the timer svc within the critical region        */
typedef struct tmr_safe {
	NCS_LOCK enter_lock;	/* protect list of new timers */
	NCS_LOCK free_lock;	/* protect list of free pool timers */
	SYSF_TMR dmy_free;
	SYSF_TMR dmy_keep;

} TMR_SAFE;

/* SYSF_TMR_CB the master structure for the timer service */
typedef struct sysf_tmr_cb {
	uns32 tick;		/* Times TmrExpiry has been called     */

	NCSLPG_OBJ persist;	/* guard against fleeting destruction */
	TMR_SAFE safe;		/* critical region stuff              */
	NCS_PATRICIA_TREE tmr_pat_tree;
	TMR_STATS stats;
	void *p_tsk_hdl;	/* expiry task handle storage         */
	NCS_SEL_OBJ sel_obj;
	uns32 msg_count;

} SYSF_TMR_CB;

/* gl_tcb  ...  The global instance of the SYSF_TMR_CB                   */
static SYSF_TMR_CB gl_tcb = { 0 };

static NCS_BOOL tmr_destroying = FALSE;
static NCS_SEL_OBJ tmr_destroy_syn_obj;
static struct timespec ts_start;

static uns32 ncs_tmr_add_pat_node(SYSF_TMR *tmr)
{
	SYSF_TMR_PAT_NODE *temp_tmr_pat_node = NULL;

	temp_tmr_pat_node = (SYSF_TMR_PAT_NODE *)ncs_patricia_tree_get(&gl_tcb.tmr_pat_tree, (uns8 *)&tmr->key);

	if (temp_tmr_pat_node == (SYSF_TMR_PAT_NODE *)NULL) {
		temp_tmr_pat_node = (SYSF_TMR_PAT_NODE *)m_NCS_MEM_ALLOC(sizeof(SYSF_TMR_PAT_NODE),
									 NCS_MEM_REGION_PERSISTENT,
									 NCS_SERVICE_ID_LEAP_TMR, 0);
		memset(temp_tmr_pat_node, '\0', sizeof(SYSF_TMR_PAT_NODE));
		temp_tmr_pat_node->key = tmr->key;
		temp_tmr_pat_node->pat_node.key_info = (uns8 *)&temp_tmr_pat_node->key;
		ncs_patricia_tree_add(&gl_tcb.tmr_pat_tree, (NCS_PATRICIA_NODE *)&temp_tmr_pat_node->pat_node);
	}

	if (temp_tmr_pat_node->tmr_list_start == NULL) {
		temp_tmr_pat_node->tmr_list_end = temp_tmr_pat_node->tmr_list_start = tmr;
	} else {
		temp_tmr_pat_node->tmr_list_end->next = tmr;
		temp_tmr_pat_node->tmr_list_end = tmr;
	}

	return NCSCC_RC_SUCCESS;
}

/* This routine returns the time elapsed in units of NCS_MILLISECONDS_PER_TICK */
static uns64 get_time_elapsed_in_ticks(struct timespec *temp_ts_start)
{
	uns64 time_elapsed = 0;
	struct timespec ts_current = { 0, 0 };

	if (clock_gettime(CLOCK_MONOTONIC, &ts_current)) {
		perror("clock_gettime with MONOTONIC Failed \n");
	}
	time_elapsed = ((((ts_current.tv_sec - temp_ts_start->tv_sec) * (1000LL)) +
			 ((ts_current.tv_nsec - temp_ts_start->tv_nsec) / 1000000)) / NCS_MILLISECONDS_PER_TICK);

	return time_elapsed;
}

/****************************************************************************
 * Function Name: sysfTmrExpiry
 *
 * Purpose: Service all timers that live in this expiry bucket
 *
 ****************************************************************************/
static NCS_BOOL sysfTmrExpiry(SYSF_TMR_PAT_NODE *tmp)
{
	SYSF_TMR *now_tmr;
	SYSF_TMR dead_inst;
	SYSF_TMR *dead_tmr = &dead_inst;
	SYSF_TMR *start_dead = &dead_inst;

	/* get these guys one behind to start as well */
	dead_tmr->next = NULL;

	/* Confirm and secure tmr service is/will persist */
	if (ncslpg_take(&gl_tcb.persist) == FALSE)
		return FALSE;	/* going or gone away.. Lets leave */

	if (tmr_destroying == TRUE) {
		/* Raise An indication */
		m_NCS_SEL_OBJ_IND(tmr_destroy_syn_obj);

		/*If thread canceled here, It had no effect on timer thread destroy */
		ncslpg_give(&gl_tcb.persist, 0);

		/* returns TRUE if thread is going to be  destroyed otherwise return FALSE(normal flow) */
		return TRUE;
	}

	TMR_DBG_TICK(gl_tcb);

	dead_tmr->next = tmp->tmr_list_start;

	TMR_SET_CNT(gl_tcb.stats);

	while (dead_tmr->next != NULL) {	/* process old and new */
		now_tmr = dead_tmr->next;
		TMR_INC_CNT(gl_tcb.stats);
		/* SMM states CREATE, EXPIRED, illegal assert */

		if ((TMR_TEST_STATE(now_tmr, TMR_STATE_DORMANT)) || (TMR_TEST_STATE(now_tmr, TMR_STATE_DESTROY))) {
			TMR_STAT_CANCELLED(gl_tcb.stats);
			TMR_STAT_ADD_FREE(gl_tcb.stats);
			TMR_STAT_FREE_HWM(gl_tcb.stats);
			TMR_DBG_STAMP(now_tmr, gl_tcb.tick);

			dead_tmr = now_tmr;	/* move on to next one */
		} else {
			TMR_SET_STATE(now_tmr, TMR_STATE_DORMANT);	/* mark it dormant */
			TMR_STAT_EXPIRY(gl_tcb.stats);
			TMR_STAT_ADD_FREE(gl_tcb.stats);
			TMR_STAT_FREE_HWM(gl_tcb.stats);
			TMR_DBG_STAMP(now_tmr, gl_tcb.tick);

			/* EXPIRY HAPPENS RIGHT HERE !!.................................. */
			if (now_tmr->tmrCB != ((TMR_CALLBACK)0x0ffffff)) {
#if ENABLE_SYSLOG_TMR_STATS
				gl_tcb.stats.cnt--;
				if (gl_tcb.stats.cnt == 0) {
					syslog(LOG_INFO, "NO Timers Active in Expiry PID %u \n", getpid());
				}
#endif
				now_tmr->tmrCB(now_tmr->tmrUarg);	/* OK this is it! Expire ! */
			}

			dead_tmr = now_tmr;	/* move on to next one */
		}
	}

	TMR_STAT_RING_HWM(gl_tcb.stats);

	/* Now replenish the free pool */

	if (start_dead->next != NULL) {
		m_NCS_LOCK(&gl_tcb.safe.free_lock, NCS_LOCK_WRITE);	/* critical region START */
		dead_tmr->next = gl_tcb.safe.dmy_free.next;	/* append old free list to end of that  */
		gl_tcb.safe.dmy_free.next = start_dead->next;	/* put start of collected dead in front */
		m_NCS_UNLOCK(&gl_tcb.safe.free_lock, NCS_LOCK_WRITE);	/* critical region END */
	}

	ncs_patricia_tree_del(&gl_tcb.tmr_pat_tree, (NCS_PATRICIA_NODE *)tmp);
	m_NCS_MEM_FREE(tmp, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_LEAP_TMR, 0);

	ncslpg_give(&gl_tcb.persist, 0);
	return FALSE;
}

static uns32 ncs_tmr_select_intr_process(struct timeval *tv, struct
					 timespec *ts_current, uns64 next_delay)
{
	uns64 tmr_restart = 0;
	uns64 time_left = 0;
	struct timespec ts_curr = *ts_current;
	struct timespec ts_eint = { 0, 0 };

	tv->tv_sec = tv->tv_usec = 0;

	if (next_delay == 0) {
		tv->tv_sec = 0xffffff;
		tv->tv_usec = 0;
		return NCSCC_RC_SUCCESS;
	}

	if (clock_gettime(CLOCK_MONOTONIC, &ts_eint)) {
		perror("clock_gettime with MONOTONIC Failed \n");
		return NCSCC_RC_FAILURE;
	} else {
		tmr_restart = TIMESPEC_DIFF_IN_NS(ts_eint, ts_curr);
		time_left = ((next_delay * 1000000LL * NCS_MILLISECONDS_PER_TICK) - (tmr_restart));
		if (time_left > 0) {
			tv->tv_sec = time_left / 1000000000LL;
			tv->tv_usec = ((time_left % 1000000000LL) / 1000);
		}
	}

	return NCSCC_RC_SUCCESS;
}

static uns32 ncs_tmr_engine(struct timeval *tv, uns64 *next_delay)
{
	uns64 next_expiry = 0;
	uns64 ticks_elapsed = 0;
	SYSF_TMR_PAT_NODE *tmp = NULL;

#if ENABLE_SYSLOG_TMR_STATS
	/* To measure avg. timer expiry gap */
	struct timespec tmr_exp_prev_finish = { 0, 0 };
	struct timespec tmr_exp_curr_start = { 0, 0 };
	uns32 tot_tmr_exp = 0;
	uns64 sum_of_tmr_exp_gaps = 0;	/* Avg = sum_of_tmr_exp_gaps/tot_tmr_exp */
#endif

	while (TRUE) {
		tmp = (SYSF_TMR_PAT_NODE *)ncs_patricia_tree_getnext(&gl_tcb.tmr_pat_tree, (uns8 *)NULL);
		ticks_elapsed = get_time_elapsed_in_ticks(&ts_start);
		if (tmp != NULL) {
			next_expiry = m_NCS_OS_NTOHLL_P(&tmp->key);
		} else {
			tv->tv_sec = 0xffffff;
			tv->tv_usec = 0;
			*next_delay = 0;
			return NCSCC_RC_SUCCESS;
		}

		if (ticks_elapsed >= next_expiry) {
#if ENABLE_SYSLOG_TMR_STATS
			if (tot_tmr_exp != 0) {
				tmr_exp_curr_start.tv_sec = tmr_exp_curr_start.tv_nsec = 0;
				if (clock_gettime(CLOCK_MONOTONIC, &tmr_exp_curr_start)) {
					perror("clock_gettime with MONOTONIC Failed \n");
				}
				sum_of_tmr_exp_gaps += TIMESPEC_DIFF_IN_NS(tmr_exp_curr_start, tmr_exp_prev_finish);
				if (tot_tmr_exp >= 100) {
					printf("\nTotal active timers: %d\n", gl_tcb.stats.cnt);
					printf("Average timer-expiry gap (last %d expiries) = %lld\n",
					       tot_tmr_exp, sum_of_tmr_exp_gaps / tot_tmr_exp);
					tot_tmr_exp = 0;
					sum_of_tmr_exp_gaps = 0;
				}
			}
#endif

			if (TRUE == sysfTmrExpiry(tmp)) {	/* call expiry routine          */
				return NCSCC_RC_FAILURE;
			}
#if ENABLE_SYSLOG_TMR_STATS
			tot_tmr_exp++;
			tmr_exp_prev_finish.tv_sec = tmr_exp_prev_finish.tv_nsec = 0;
			if (clock_gettime(CLOCK_MONOTONIC, &tmr_exp_prev_finish)) {
				perror("clock_gettime with MONOTONIC Failed \n");
				return NCSCC_RC_FAILURE;
			}
#endif
		} else {
			break;
		}
	}			/* while loop end */

	(*next_delay) = next_expiry - ticks_elapsed;

	/* Convert the next_dealy intto the below structure */
	tv->tv_sec = ((*next_delay) * NCS_MILLISECONDS_PER_TICK) / 1000;
	tv->tv_usec = (((*next_delay) % (1000 / NCS_MILLISECONDS_PER_TICK)) * (1000 * NCS_MILLISECONDS_PER_TICK));

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Function Name: ncs_tmr_wait
 *
 * Purpose: Goto counting semephore and block until tmr_pulse is felt. This
 *          means the actual expiry task no longer services the sysfTmrExpiry
 *          function any more. This should eliminate timer drift.
 *
 ****************************************************************************/
static uns32 ncs_tmr_wait(void)
{

	int rc = 0;
	int inds_rmvd;
	int save_errno = 0;

	uns64 next_delay = 0;

	NCS_SEL_OBJ mbx_fd = gl_tcb.sel_obj;
	NCS_SEL_OBJ highest_sel_obj;
	NCS_SEL_OBJ_SET all_sel_obj;
	struct timeval tv = { 0xffffff, 0 };
	struct timespec ts_current = { 0, 0 };

	m_NCS_SEL_OBJ_ZERO(&all_sel_obj);
	highest_sel_obj = mbx_fd;

	if (clock_gettime(CLOCK_MONOTONIC, &ts_start)) {
		perror("clock_gettime with MONOTONIC Failed \n");
		return NCSCC_RC_FAILURE;
	}

	ts_current = ts_start;

	while (TRUE) {
 select_sleep:
		m_NCS_SEL_OBJ_SET(mbx_fd, &all_sel_obj);
		rc = select(highest_sel_obj.rmv_obj + 1, &all_sel_obj, NULL, NULL, &tv);
		save_errno = errno;
		m_NCS_LOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);

		if (rc < 0) {
			if (save_errno != EINTR)
				assert(0);

			if (ncs_tmr_select_intr_process(&tv, &ts_current, next_delay) == NCSCC_RC_SUCCESS) {
				m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);
				goto select_sleep;
			} else {
				m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);
				return NCSCC_RC_FAILURE;
			}
		} else if (rc == 1) {
			/* if select returned because of indication on sel_obj from sysfTmrDestroy */
			if (tmr_destroying == TRUE) {
				/* Raise An indication */
				m_NCS_SEL_OBJ_IND(tmr_destroy_syn_obj);
				m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);
				return NCSCC_RC_SUCCESS;
			}

			gl_tcb.msg_count--;

			if (gl_tcb.msg_count == 0) {
				inds_rmvd = m_NCS_SEL_OBJ_RMV_IND(gl_tcb.sel_obj, TRUE, TRUE);
				if (inds_rmvd <= 0) {
					if (inds_rmvd != -1) {
						/* The object has not been destroyed and it has no indication
						   raised on it inspite of msg_count being non-zero.
						 */
						m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);
						return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
					}

					m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);

					/* The mbox must have been destroyed */
					return NCSCC_RC_FAILURE;
				}
			}
		}

		rc = ncs_tmr_engine(&tv, &next_delay);
		if (rc == NCSCC_RC_FAILURE) {
			m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);
			return NCSCC_RC_FAILURE;
		}

		m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);
		ts_current.tv_sec = ts_current.tv_nsec = 0;

		if (clock_gettime(CLOCK_MONOTONIC, &ts_current)) {
			perror("clock_gettime with MONOTONIC Failed \n");
			return NCSCC_RC_FAILURE;
		}
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Function Name: sysfTmrCreate 
 *
 * Purpose: Create the timer service.Get all resources in a known state.
 *
 ****************************************************************************/

static NCS_BOOL ncs_tmr_create_done = FALSE;

NCS_BOOL sysfTmrCreate(void)
{
	NCS_PATRICIA_PARAMS pat_param;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (ncs_tmr_create_done == FALSE)
		ncs_tmr_create_done = TRUE;
	else
		return TRUE;

	/* Empty Timer Service control block. */
	memset(&gl_tcb, '\0', sizeof(SYSF_TMR_CB));

	/* put local persistent guard in start state */
	ncslpg_create(&gl_tcb.persist);

	/* Initialize the locks */
	m_NCS_LOCK_INIT(&gl_tcb.safe.enter_lock);
	m_NCS_LOCK_INIT(&gl_tcb.safe.free_lock);

	memset((void *)&pat_param, 0, sizeof(NCS_PATRICIA_PARAMS));

	pat_param.key_size = sizeof(uns64);

	rc = ncs_patricia_tree_init(&gl_tcb.tmr_pat_tree, &pat_param);
	if (rc != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	rc = m_NCS_SEL_OBJ_CREATE(&gl_tcb.sel_obj);
	if (rc != NCSCC_RC_SUCCESS) {
		ncs_patricia_tree_destroy(&gl_tcb.tmr_pat_tree);
		return NCSCC_RC_FAILURE;
	}
	tmr_destroying = FALSE;

	/* create expiry thread */

	if (m_NCS_TASK_CREATE((NCS_OS_CB)ncs_tmr_wait,
			      0,
			      NCS_TMR_TASKNAME,
			      NCS_TMR_PRIORITY, NCS_TMR_STACKSIZE, &gl_tcb.p_tsk_hdl) != NCSCC_RC_SUCCESS) {
		ncs_patricia_tree_destroy(&gl_tcb.tmr_pat_tree);
		m_NCS_SEL_OBJ_DESTROY(gl_tcb.sel_obj);
		return FALSE;
	}

	if (m_NCS_TASK_START(gl_tcb.p_tsk_hdl) != NCSCC_RC_SUCCESS) {
		m_NCS_TASK_RELEASE(gl_tcb.p_tsk_hdl);
		ncs_patricia_tree_destroy(&gl_tcb.tmr_pat_tree);
		m_NCS_SEL_OBJ_DESTROY(gl_tcb.sel_obj);
		return FALSE;
	}
	return TRUE;
}

/****************************************************************************
 * Function Name: sysfTmrDestroy
 *
 * Purpose: Disengage timer services. Memory is not given back to HEAP as
 *          it is not clear what the state of outstanding timers is.
 *
 ****************************************************************************/

NCS_BOOL sysfTmrDestroy(void)
{
	SYSF_TMR *tmr;
	SYSF_TMR *free_tmr;
	uns32 timeout = 2000;	/* 20seconds */
	SYSF_TMR_PAT_NODE *tmp = NULL;

	/* There is only ever one timer per instance */

	m_NCS_LOCK(&gl_tcb.safe.free_lock, NCS_LOCK_WRITE);

	gl_tcb.safe.dmy_free.next = NULL;

	m_NCS_UNLOCK(&gl_tcb.safe.free_lock, NCS_LOCK_WRITE);

	m_NCS_LOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);

	/* Create selection object */
	m_NCS_SEL_OBJ_CREATE(&tmr_destroy_syn_obj);

	tmr_destroying = TRUE;

	m_NCS_SEL_OBJ_IND(gl_tcb.sel_obj);

	/* Unlock the lock */
	m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);	/* critical region END */

	/* Wait on Poll object */
	m_NCS_SEL_OBJ_POLL_SINGLE_OBJ(tmr_destroy_syn_obj, &timeout);

	m_NCS_LOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);
	tmr = &gl_tcb.safe.dmy_keep;
	while (tmr->keep != NULL) {
		free_tmr = tmr->keep;
		tmr->keep = tmr->keep->keep;
		m_NCS_MEM_FREE(free_tmr, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_LEAP_TMR, 0);
	}
	while ((tmp = (SYSF_TMR_PAT_NODE *)ncs_patricia_tree_getnext(&gl_tcb.tmr_pat_tree, (uns8 *)0)) != NULL) {
		ncs_patricia_tree_del(&gl_tcb.tmr_pat_tree, (NCS_PATRICIA_NODE *)tmp);
		m_NCS_MEM_FREE(tmp, NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_LEAP_TMR, 0);
	}

	ncs_patricia_tree_destroy(&gl_tcb.tmr_pat_tree);
	m_NCS_SEL_OBJ_DESTROY(gl_tcb.sel_obj);

	/* Stop the dedicated thread that runs out of ncs_tmr_wait() */

	m_NCS_TASK_RELEASE(gl_tcb.p_tsk_hdl);

	tmr_destroying = FALSE;

	m_NCS_SEL_OBJ_DESTROY(tmr_destroy_syn_obj);

	m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);	/* critical region END */

	/* don't destroy the lock (but remember that you could!).
	 * m_NCS_LOCK_DESTROY (&l_tcb.lock); 
	 */

	m_NCS_LOCK_DESTROY(&gl_tcb.safe.enter_lock);

	m_NCS_LOCK_DESTROY(&gl_tcb.safe.free_lock);

	ncs_tmr_create_done = FALSE;

	return TRUE;
}

/*****************************************************************************
 *
 *                  Timer Add, Stop, and Expiration Handler
 *
 *****************************************************************************/

/****************************************************************************
 * Function Name: sysfTmrAlloc
 *
 * Purpose: Either fetch an existing Tmr block or get one off the HEAP
 *
 ****************************************************************************/
tmr_t ncs_tmr_alloc(char *file, uns32 line)
{
	SYSF_TMR *tmr;
	SYSF_TMR *back;

	if (tmr_destroying == TRUE)
		return NULL;

	if (ncslpg_take(&gl_tcb.persist) == FALSE)	/* guarentee persistence */
		return NULL;

	m_NCS_LOCK(&gl_tcb.safe.free_lock, NCS_LOCK_WRITE);

	back = &gl_tcb.safe.dmy_free;	/* see if we have a free one */
	tmr = back->next;

	while (tmr != NULL) {
		if (TMR_TEST_STATE(tmr, TMR_STATE_DESTROY)) {
			TMR_STAT_RMV_FREE(gl_tcb.stats);
			back->next = tmr->next;	/* and 'tmr' is our answer */
			break;
		} else {
			back = tmr;
			tmr = tmr->next;
		}
	}

	if (tmr == NULL) {
		tmr = (SYSF_TMR *)m_NCS_MEM_ALLOC(sizeof(SYSF_TMR),
						  NCS_MEM_REGION_PERSISTENT, NCS_SERVICE_ID_LEAP_TMR, 0);
		memset(tmr, '\0', sizeof(SYSF_TMR));
		if (tmr == NULL)
			m_LEAP_DBG_SINK_VOID;	/* can't allocate memory?? */
		else {
			TMR_STAT_TTL_TMRS(gl_tcb.stats);
			tmr->keep = gl_tcb.safe.dmy_keep.keep;	/* put it on keep list */
			gl_tcb.safe.dmy_keep.keep = tmr;
		}
	}

	m_NCS_UNLOCK(&gl_tcb.safe.free_lock, NCS_LOCK_WRITE);

	if (tmr != NULL) {
		tmr->next = NULL;	/* put it in start state */
		TMR_SET_STATE(tmr, TMR_STATE_CREATE);
		TMR_DBG_SET(tmr->dbg, file, line);
	}

	ncslpg_give(&gl_tcb.persist, 0);
	return (tmr_t)tmr;
}

/****************************************************************************
 * Function Name: sysfTmrStart
 *
 * Purpose: Take the passed Timer traits and engage/register with TmrSvc
 *
 ****************************************************************************/
tmr_t ncs_tmr_start(tmr_t tid, uns32 tmrDelay,	/* timer period in number of 10ms units */
		    TMR_CALLBACK tmrCB, void *tmrUarg, char *file, uns32 line)
{
	SYSF_TMR *tmr;
	SYSF_TMR *new_tmr;
	uns64 scaled;
	uns64 temp_key_value;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (((tmr = (SYSF_TMR *)tid) == NULL) || (tmr_destroying == TRUE))	/* NULL tmrs are no good! */
		return NULL;

	TMR_DBG_ASSERT_ISA(tmr->dbg);	/* confirm that its TMR memory */

	TMR_DBG_ASSERT_STATE(tmr, (TMR_STATE_DORMANT | TMR_STATE_CREATE));

	if (ncslpg_take(&gl_tcb.persist) == FALSE)	/* guarentee persistence */
		return NULL;

	if (TMR_TEST_STATE(tmr, TMR_STATE_DORMANT)) {	/* If client is re-using timer */
		m_NCS_TMR_CREATE(new_tmr, tmrDelay, tmrCB, tmrUarg);	/* get a new one */
		if (new_tmr == NULL) {
			ncslpg_give(&gl_tcb.persist, 0);
			return NULL;
		}

		TMR_SET_STATE(tmr, TMR_STATE_DESTROY);	/* TmrSvc ignores 'old' one */
		tmr = new_tmr;
	}
	scaled = (tmrDelay * 10 / NCS_MILLISECONDS_PER_TICK) + 1 + (get_time_elapsed_in_ticks(&ts_start));

	/* Lock the enter wheel in the safe area */
	m_NCS_LOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);

	/* Do some up front initialization as if all will go well */
	tmr->tmrCB = tmrCB;
	tmr->tmrUarg = tmrUarg;
	TMR_SET_STATE(tmr, TMR_STATE_START);

	tmr->next = NULL;
	m_NCS_OS_HTONLL_P(&temp_key_value, scaled);
	tmr->key = temp_key_value;

	rc = ncs_tmr_add_pat_node(tmr);
	if (rc == NCSCC_RC_FAILURE) {
		/* Free the timer created */
		m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);
		return NULL;
	}
#if ENABLE_SYSLOG_TMR_STATS
	gl_tcb.stats.cnt++;
	if (gl_tcb.stats.cnt == 1) {
		syslog(LOG_INFO, "At least one timer started\n");
	}
#endif
	if (gl_tcb.msg_count == 0) {
		/* There are no messages queued, we shall raise an indication
		   on the "sel_obj".  */
		if (m_NCS_SEL_OBJ_IND(gl_tcb.sel_obj) != NCSCC_RC_SUCCESS) {
			/* We would never reach here! */
			m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);
			m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			return NULL;
		}
	}
	gl_tcb.msg_count++;

	/*
	   1)Take Lock
	   2)Caliculate the key value based on (A+1)+T
	   T is Timer ticks elapsed  and A is ticks 
	   ncs_tmr_add_pat_node(root_node,tmr,(A+1)+T,)
	   3)See whether patricia Node is already present in patricia Tree with the 
	   caliculated Key 
	   If not present 
	   Create patricia Node and add it the patricia tree 
	   If present
	   Add the tmr data structure to the double linked list as FIFO.
	   i.e adds at the last of the double linked list.
	   4) Raise an indication on the selection object.
	   if (write(i_ind_obj.raise_obj, "A", 1) != 1)
	   return NCSCC_RC_FAILURE;
	 */

	TMR_STAT_STARTS(gl_tcb.stats);

	m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);
	TMR_DBG_SET(tmr->dbg, file, line);

	ncslpg_give(&gl_tcb.persist, 0);

	return tmr;
}

/****************************************************************************
 * Function Name: ncs_tmr_stop_v2
 *
 * Purpose:   Mark this timer as DORMANT  
 *            if timer is already in DORMANT state returns 
 *            NCSCC_RC_TMR_STOPPED
 *
 * Arguments:
 *               tmrID     :    tmr id
 *               **tmr_arg :    void double pointer  
 * Return values:
 *     NCSCC_RC_FAILURE    :    Any validations fails (or)
 *                                If this timer is in CREATE or DESTROY state
 *     NCSCC_RC_SUCCESS    :    This timer START state and is set to DORMANT state 
 *     NCSCC_RC_TMR_STOPPED:    If the timer is already in DORMANT state 
 *
 ****************************************************************************/
uns32 ncs_tmr_stop_v2(tmr_t tmrID, void **o_tmr_arg)
{
	SYSF_TMR *tmr;

	if (((tmr = (SYSF_TMR *)tmrID) == NULL) || (tmr_destroying == TRUE) || (o_tmr_arg == NULL))
		return NCSCC_RC_FAILURE;

	m_NCS_LOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);	/* critical region START */

	/* Test tmr is already expired */
	if (TMR_TEST_STATE(tmr, TMR_STATE_DORMANT)) {
		m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);	/* critical region END */
		return NCSCC_RC_TMR_STOPPED;
	} else if (TMR_TEST_STATE(tmr, TMR_STATE_START)) {
#if ENABLE_SYSLOG_TMR_STATS
		gl_tcb.stats.cnt--;
		if (gl_tcb.stats.cnt == 0) {
			syslog(LOG_INFO, "NO Timers Active STOP_V2 PID %u \n", getpid());
		}
#endif
		/* set tmr to DORMANT state */
		TMR_SET_STATE(tmr, TMR_STATE_DORMANT);
	} else {
		m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);	/* critical region END */
		return NCSCC_RC_FAILURE;
	}

	*o_tmr_arg = tmr->tmrUarg;

	m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);	/* critical region END */

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Function Name: ncs_tmr_stop
 *
 * Purpose:   Mark this timer as DORMANT.
 *
 ****************************************************************************/
void ncs_tmr_stop(tmr_t tmrID)
{
	SYSF_TMR *tmr;

	if (((tmr = (SYSF_TMR *)tmrID) == NULL) || (tmr_destroying == TRUE))
		return;

	TMR_DBG_ASSERT_ISA(tmr->dbg);	/* confirm that its TMR memory */

#if ENABLE_SYSLOG_TMR_STATS
	if (!TMR_TEST_STATE(tmr, TMR_STATE_DORMANT)) {
		gl_tcb.stats.cnt--;
		if (gl_tcb.stats.cnt == 0) {
			syslog(LOG_INFO, "NO Timers Active STOP PID %u \n", getpid());
		}
	}
#endif

	TMR_SET_STATE(tmr, TMR_STATE_DORMANT);
}

/****************************************************************************
 * Function Name: ncs_tmr_free
 *
 * Purpose: Mark this timer as DESTOYED.
 *
 ****************************************************************************/
void ncs_tmr_free(tmr_t tmrID)
{
	SYSF_TMR *tmr;

	if (((tmr = (SYSF_TMR *)tmrID) == NULL) || (tmr_destroying == TRUE))
		return;

	TMR_DBG_ASSERT_ISA(tmr->dbg);	/* confirm that its timer memory */

	m_NCS_LOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);	/* critical region START */

#if ENABLE_SYSLOG_TMR_STATS
	if ((TMR_TEST_STATE(tmr, TMR_STATE_START))) {
		gl_tcb.stats.cnt--;
		if (gl_tcb.stats.cnt == 0) {
			syslog(LOG_INFO, "NO Timers Active Destroy PID %s \n", __LINE__);
		}
	}
#endif
	TMR_SET_STATE(tmr, TMR_STATE_DESTROY);

	/* here we can only selectively 0xff out memory fields */
	tmr->tmrCB = (TMR_CALLBACK)0x0ffffff;
	tmr->tmrUarg = (void *)0x0ffffff;
	m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);	/* critical region END */
}

/****************************************************************************
 * Function Name: ncs_tmr_remaining
 *
 * Purpose:   This function calculates how much time is remaining for a
 *            particular timer.
 *
 ****************************************************************************/
uns32 ncs_tmr_remaining(tmr_t tmrID, uns32 *p_tleft)
{
	SYSF_TMR *tmr;
	uns32 total_ticks_left;
	uns32 ticks_elapsed;
	uns32 ticks_to_expiry;

	if (((tmr = (SYSF_TMR *)tmrID) == NULL) || (tmr_destroying == TRUE) || (p_tleft == NULL))
		return NCSCC_RC_FAILURE;

	*p_tleft = 0;
	TMR_DBG_ASSERT_ISA(tmr->dbg);	/* confirm that its timer memory */

	if (ncslpg_take(&gl_tcb.persist) == FALSE)	/* guarentee persistence */
		return NCSCC_RC_FAILURE;

	m_NCS_LOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);	/* critical region START */
	if (!TMR_TEST_STATE(tmr, TMR_STATE_START)) {
		m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);	/* critical region START */
		return NCSCC_RC_FAILURE;
	}
	m_NCS_UNLOCK(&gl_tcb.safe.enter_lock, NCS_LOCK_WRITE);	/* critical region START */
	ticks_elapsed = get_time_elapsed_in_ticks(&ts_start);
	ticks_to_expiry = m_NCS_OS_NTOHLL_P(&tmr->key);
	total_ticks_left = (ticks_to_expiry - ticks_elapsed);

	*p_tleft = total_ticks_left * NCS_MILLISECONDS_PER_TICK;

	return ncslpg_give(&gl_tcb.persist, NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Function Name: ncs_tmr_whatsout
 *
 * Purpose: Show all Timer Blocks that are in the free pool but are still
 *          in the dormant state (indicating that they have been STOPPED or
 *          have expired, but have not yet been DESTROYED.
 *
 * NOTE: This is quick output scheme. Needs to be fit into sysmon model.
 *
 ****************************************************************************/

static char *gl_tmr_states[] = { "illegal",
	"CREATE",
	"START",
	"illegal",
	"DORMANT",
	"illegal",
	"illegal",
	"illegal",
	"DESTROY"
};

uns32 ncs_tmr_whatsout(void)
{
	SYSF_TMR *free;
	uns32 cnt = 1;
	char pBuf[100];

	printf("|---+----+-----+-----------+-----------------------------|\n");
	printf("|            O U T S T A N D I N G   T M R S             |\n");
	printf("|---+----+-----+-----------+------------+----------------|\n");
	printf("|  #|age |Owner|Owner file |    state   |       pointer  |\n");
	printf("|  #|    | line|           |            |                |\n");
	printf("|---|----+-----+-----------+------------+----------------|\n");

	if ((ncslpg_take(&gl_tcb.persist) == FALSE) || (tmr_destroying == TRUE)) {
		printf("< . . . TMR SVC DESTROYED: .CLEANUP ALREADY DONE..>\n");
		return NCSCC_RC_FAILURE;	/* going or gone away.. Lets leave */
	}

	m_NCS_LOCK(&gl_tcb.safe.free_lock, NCS_LOCK_WRITE);	/* critical region START */

	free = gl_tcb.safe.dmy_keep.keep;	/* get start of keep list; may be NULL  */
	while (free != NULL) {
		if (!TMR_TEST_STATE(free, TMR_STATE_DESTROY)) {	/* some may be in transition, but report all */
			sprintf(pBuf, "%4d%5d%6d%12s%12s%16lx\n", cnt++,	/* Nmber  */
				gl_tcb.tick - free->dbg.stamp,	/* age    */
				free->dbg.line,	/* OwnrL  */
				ncs_fname(free->dbg.file),	/* OwnrF  */
				gl_tmr_states[free->state],	/* state  */
				(long)free);	/* pointr */
			printf("%s\n", pBuf);
		}
		free = free->keep;
	}
	m_NCS_UNLOCK(&gl_tcb.safe.free_lock, NCS_LOCK_WRITE);	/* critical region END */
	return ncslpg_give(&gl_tcb.persist, NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Function Name: ncs_tmr_getstats
 *
 * Purpose: Show the current statistics associated with the timer service.
 *
 * NOTE: This is quick output scheme. Needs to be fit into sysmon model.
 ****************************************************************************/

uns32 ncs_tmr_getstats(void)
{
	return NCSCC_RC_SUCCESS;
}
