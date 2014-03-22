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

  This module contains routines related H&J Tasking mechanisms.
..............................................................................

  FUNCTIONS INCLUDED in this module:

  ncs_task_create....create a task
  ncs_task_release...release a task
  ncs_task_detach...changes the task created in the joinable state to
                     detachable state
  ncs_task_start.....add a task to the "ready" list
  ncs_task_stop......remove a task from the "ready" list
  ncs_task_sleep.....suspend a task for "n" milliseconds

 ******************************************************************************
 */

#include <ncsgl_defs.h>
#include "ncs_osprm.h"

#include "ncssysf_tsk.h"

uns32
ncs_task_create(NCS_OS_CB entry_pt,
		void *arg, char *name, unsigned int priority, unsigned int stack_size_in_bytes, void **task_handle)
{
	NCS_OS_TASK task;
	uns32 rc;

	task.info.create.i_entry_point = entry_pt;
	task.info.create.i_name = name;
	task.info.create.i_priority = priority;
	task.info.create.i_stack_nbytes = stack_size_in_bytes;
	task.info.create.i_ep_arg = arg;

	rc = m_NCS_OS_TASK(&task, NCS_OS_TASK_CREATE);

	if (NCSCC_RC_SUCCESS == rc)
		*task_handle = task.info.create.o_handle;

	return rc;
}

uns32 ncs_task_release(void *task_handle)
{
	NCS_OS_TASK task;

	if (task_handle == NULL)
		return NCSCC_RC_SUCCESS;

	task.info.release.i_handle = task_handle;

	return m_NCS_OS_TASK(&task, NCS_OS_TASK_RELEASE);
}

uns32 ncs_task_join(void *task_handle)
{
	NCS_OS_TASK task;

	if (task_handle == NULL)
		return NCSCC_RC_SUCCESS;

	task.info.release.i_handle = task_handle;

	return m_NCS_OS_TASK(&task, NCS_OS_TASK_JOIN);
}

uns32 ncs_task_detach(void *task_handle)
{
	NCS_OS_TASK task;

	if (task_handle == NULL)
		return NCSCC_RC_SUCCESS;

	task.info.release.i_handle = task_handle;

	return m_NCS_OS_TASK(&task, NCS_OS_TASK_DETACH);
}

uns32 ncs_task_start(void *task_handle)
{
	NCS_OS_TASK task;

	if (task_handle == NULL)
		return NCSCC_RC_SUCCESS;

	task.info.start.i_handle = task_handle;

	return m_NCS_OS_TASK(&task, NCS_OS_TASK_START);
}

uns32 ncs_task_stop(void *task_handle)
{
	NCS_OS_TASK task;

	if (task_handle == NULL)
		return NCSCC_RC_SUCCESS;

	task.info.stop.i_handle = task_handle;

	return m_NCS_OS_TASK(&task, NCS_OS_TASK_STOP);
}

uns32 ncs_task_sleep(unsigned int delay_in_ms)
{
	NCS_OS_TASK task;

	task.info.sleep.i_delay_in_ms = delay_in_ms;

	return m_NCS_OS_TASK(&task, NCS_OS_TASK_SLEEP);
}

uns32 ncs_task_current(void **task_handle)
{
	NCS_OS_TASK task;
	uns32 rc;

	rc = m_NCS_OS_TASK(&task, NCS_OS_TASK_CURRENT_HANDLE);

	if (NCSCC_RC_SUCCESS == rc)
		*task_handle = task.info.current_handle.o_handle;

	return rc;
}

int ncs_task_entry(NCS_OS_TASK *task)
{
	m_NCS_OS_TASK_PRELUDE;

	task->info.create.i_entry_point(task->info.create.i_ep_arg);

	return 0;
}
