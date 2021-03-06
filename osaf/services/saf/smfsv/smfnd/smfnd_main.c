/*       OpenSAF
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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
 *            Wind River Systems
 *
 */

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <configmake.h>

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <poll.h>

#include <daemon.h>
#include <nid_api.h>
#include <ncs_main_papi.h>

#include "smfnd.h"
#include "smfsv_defs.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */
static smfnd_cb_t _smfnd_cb;
smfnd_cb_t *smfnd_cb = &_smfnd_cb;

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

/**
 * USR1 signal handler to activate AMF
 * @param sig
 */
static void usr1_sig_handler(int sig)
{
	/*signal(SIGUSR1, SIG_IGN); */
	if (smfnd_cb->amf_hdl == 0)
		ncs_sel_obj_ind(smfnd_cb->usr1_sel_obj);
}

/****************************************************************************
 * Name          : smfnd_cb_init
 *
 * Description   : This function initializes the SMFND_CB. 
 *                 
 *
 * Arguments     : smfnd_cb * - Pointer to the SMFND_CB.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 smfnd_cb_init(smfnd_cb_t * smfnd_cb)
{
	TRACE_ENTER();

	/* Assign Initial HA state */
	smfnd_cb->ha_state = SMFND_HA_INIT_STATE;

	/* Assign Version. Currently, hardcoded, This will change later */
	smfnd_cb->smf_version.releaseCode = SMF_RELEASE_CODE;
	smfnd_cb->smf_version.majorVersion = SMF_MAJOR_VERSION;
	smfnd_cb->smf_version.minorVersion = SMF_MINOR_VERSION;

	smfnd_cb->amf_hdl = 0;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Initialize smfnd
 * 
 * @return uns32
 */
static uns32 initialize_smfnd(void)
{
	uns32 rc;

	TRACE_ENTER();

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_agents_startup FAILED");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Initialize smfnd control block */
	if (smfnd_cb_init(smfnd_cb) != NCSCC_RC_SUCCESS) {
		TRACE("smfnd_cb_init FAILED");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Create the mailbox used for communication with SMFD/SMFA */
	if ((rc = m_NCS_IPC_CREATE(&smfnd_cb->mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_IPC_CREATE FAILED %d", rc);
		goto done;
	}

	/* Attach mailbox to this thread */
	if ((rc = m_NCS_IPC_ATTACH(&smfnd_cb->mbx) != NCSCC_RC_SUCCESS)) {
		LOG_ER("m_NCS_IPC_ATTACH FAILED %d", rc);
		goto done;
	}

	/* Create a selection object for USR1 signal handling */
	if ((rc = ncs_sel_obj_create(&smfnd_cb->usr1_sel_obj)) != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_sel_obj_create failed");
		goto done;
	}

	/* Initialize mds communication */
	if ((rc = smfnd_mds_init(smfnd_cb)) != NCSCC_RC_SUCCESS) {
		TRACE("smfnd_mds_init FAILED %d", rc);
		return rc;
	}

	/* Check if AMF started */
	if (smfnd_cb->nid_started == 0) {
		/* Started by AMF, so let's init AMF */
		if ((rc = smfnd_amf_init(smfnd_cb)) != NCSCC_RC_SUCCESS) {
			LOG_ER("init amf failed");
			goto done;
		}
	}

 done:
	TRACE_LEAVE();
	return (rc);
}

typedef enum {
	SMFND_AMF_FD,
	SMFND_MBX_FD,
	SMFND_MAX_FD
} smfnd_pollfd_t;

struct pollfd fds[SMFND_MAX_FD];

/**
 * Restore USR1 signal handling.
 */
#if 0
uns32 smfnd_amf_disconnected(smfnd_cb_t * cb)
{
	fds[SMFND_AMF_FD].fd = cb->usr1_sel_obj.rmv_obj;
	cb->amf_hdl = 0;
	return NCSCC_RC_SUCCESS;
}
#endif

/**
 * Forever wait on events and process them.
 */
static void main_process(void)
{
	NCS_SEL_OBJ mbx_fd;
	SaAisErrorT error = SA_AIS_OK;

	TRACE_ENTER();

	mbx_fd = ncs_ipc_get_sel_obj(&smfnd_cb->mbx);

	/* Set up all file descriptors to listen to */
	if (smfnd_cb->nid_started)
		fds[SMFND_AMF_FD].fd = smfnd_cb->usr1_sel_obj.rmv_obj;
	else
		fds[SMFND_AMF_FD].fd = smfnd_cb->amfSelectionObject;

	fds[SMFND_AMF_FD].events = POLLIN;
	fds[SMFND_MBX_FD].fd = mbx_fd.rmv_obj;
	fds[SMFND_MBX_FD].events = POLLIN;

	while (1) {
		int ret = poll(fds, SMFND_MAX_FD, -1);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		/* Process all the AMF messages */
		if (fds[SMFND_AMF_FD].revents & POLLIN) {
			if (smfnd_cb->amf_hdl != 0) {
				/* dispatch all the AMF pending function */
				if ((error = saAmfDispatch(smfnd_cb->amf_hdl, SA_DISPATCH_ALL)) != SA_AIS_OK) {
					LOG_ER("saAmfDispatch failed: %u", error);
					break;
				}
			} else {
				TRACE("SIGUSR1 event rec");

				if (smfnd_amf_init(smfnd_cb) != NCSCC_RC_SUCCESS) {
					LOG_ER("amf init failed");
					break;
				}

				TRACE("AMF Initialization SUCCESS......");
				fds[SMFND_AMF_FD].fd = smfnd_cb->amfSelectionObject;
			}
		}

		/* Process all the Mail box events */
		if (fds[SMFND_MBX_FD].revents & POLLIN) {
			/* dispatch all the MBX events */
			smfnd_process_mbx(&smfnd_cb->mbx);
		}
	}
}

/**
 * The main routine for the smfnd daemon.
 * @param argc
 * @param argv
 * 
 * @return int
 */
int main(int argc, char *argv[])
{
	/* Determine how this process was started, by NID or AMF */
	if (getenv("SA_AMF_COMPONENT_NAME") != NULL) {
		/* AMF start */
		smfnd_cb->nid_started = 0;
	} else {
		/* NID start */
		smfnd_cb->nid_started = 1;
	}

	daemonize(argc, argv);

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_agents_startup failed");
		goto done;
	}

	if (initialize_smfnd() != NCSCC_RC_SUCCESS) {
		LOG_ER("initialize_smfnd failed");
		goto done;
	}

	if (signal(SIGUSR1, usr1_sig_handler) == SIG_ERR) {
		LOG_ER("signal SIGUSR1 failed");
		goto done;
	}

	if (smfnd_cb->nid_started == 1) {
		if (nid_notify("SMFND", NCSCC_RC_SUCCESS, NULL) != NCSCC_RC_SUCCESS)
			LOG_ER("nid_notify failed");
	}

	main_process();

	exit(EXIT_SUCCESS);

 done:
	if (smfnd_cb->nid_started == 1) {
		if (nid_notify("SMFND", NCSCC_RC_FAILURE, NULL) != NCSCC_RC_SUCCESS)
			LOG_ER("nid_notify failed");
	}

	LOG_ER("SMFND initialization failed, exiting...");
	exit(EXIT_FAILURE);
}
