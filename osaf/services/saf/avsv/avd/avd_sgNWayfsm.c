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

  DESCRIPTION: This file is part of the SG processing module. It contains
  the SG state machine, for processing the events related to SG for N-Way
  redundancy model.

..............................................................................

  FUNCTIONS INCLUDED in this file:

  avd_sg_nway_si_func - Function to process the new complete SI in the SG.
  avd_sg_nway_siswitch_func - function called when a operator does a SI switch.
  avd_sg_nway_su_fault_func - function is called when a SU failed and switchover
                            needs to be done.
  avd_sg_nway_su_insvc_func - function is called when a SU readiness state changes
                            to inservice from out of service
  avd_sg_nway_susi_sucss_func - processes successful SUSI assignment. 
  avd_sg_nway_susi_fail_func - processes failure of SUSI assignment.
  avd_sg_nway_realign_func - function called when SG operation is done or cluster
                           timer expires.
  avd_sg_nway_node_fail_func - function is called when the node has already failed and
                             the SIs have to be failed over.
  avd_sg_nway_su_admin_fail -  function is called when SU is LOCKED/SHUTDOWN.  
  avd_sg_nway_si_admin_down - function is called when SIs is LOCKED/SHUTDOWN.
  avd_sg_nway_sg_admin_down - function is called when SGs is LOCKED/SHUTDOWN.
******************************************************************************
*/

#include <logtrace.h>

#include <avd.h>

/* static function declarations */

static uns32 avd_sg_nway_su_fault_stable(AVD_CL_CB *, AVD_SU *);
static uns32 avd_sg_nway_su_fault_sg_realign(AVD_CL_CB *, AVD_SU *);
static uns32 avd_sg_nway_su_fault_su_oper(AVD_CL_CB *, AVD_SU *);
static uns32 avd_sg_nway_su_fault_si_oper(AVD_CL_CB *, AVD_SU *);
static uns32 avd_sg_nway_su_fault_sg_admin(AVD_CL_CB *, AVD_SU *);

static uns32 avd_sg_nway_susi_succ_sg_realign(AVD_CL_CB *, AVD_SU *, AVD_SU_SI_REL *, AVSV_SUSI_ACT, SaAmfHAStateT);
static uns32 avd_sg_nway_susi_succ_su_oper(AVD_CL_CB *, AVD_SU *, AVD_SU_SI_REL *, AVSV_SUSI_ACT, SaAmfHAStateT);
static uns32 avd_sg_nway_susi_succ_si_oper(AVD_CL_CB *, AVD_SU *, AVD_SU_SI_REL *, AVSV_SUSI_ACT, SaAmfHAStateT);
static uns32 avd_sg_nway_susi_succ_sg_admin(AVD_CL_CB *, AVD_SU *, AVD_SU_SI_REL *, AVSV_SUSI_ACT, SaAmfHAStateT);

static void avd_sg_nway_node_fail_stable(AVD_CL_CB *, AVD_SU *, AVD_SU_SI_REL *);
static void avd_sg_nway_node_fail_su_oper(AVD_CL_CB *, AVD_SU *);
static void avd_sg_nway_node_fail_si_oper(AVD_CL_CB *, AVD_SU *);
static void avd_sg_nway_node_fail_sg_admin(AVD_CL_CB *, AVD_SU *);
static void avd_sg_nway_node_fail_sg_realign(AVD_CL_CB *, AVD_SU *);

/* macro to determine if all the active sis assigned to a 
   particular su have been successfully engaged by the standby */
#define m_AVD_SG_NWAY_ARE_STDBY_SUS_ENGAGED(su, susi, is) \
{ \
   AVD_SU_SI_REL *csusi = 0, *csisu = 0; \
   NCS_BOOL is_si_eng; \
   (is) = TRUE; \
   for (csusi = (su)->list_of_susi; csusi; csusi = csusi->su_next) { \
      if ( (csusi == (susi)) || (SA_AMF_HA_STANDBY == csusi->state) ) \
         continue; \
      if ( (SA_AMF_HA_ACTIVE == csusi->state) || \
           (SA_AMF_HA_QUIESCING == csusi->state) || \
           ((SA_AMF_HA_QUIESCED == csusi->state) && \
            (AVD_SU_SI_STATE_MODIFY == csusi->fsm)) ) { \
         (is) = FALSE; \
         break; \
      } \
      is_si_eng = FALSE; \
      for (csisu = csusi->si->list_of_sisu; csisu; csisu = csisu->si_next) { \
         if (SA_AMF_HA_ACTIVE == csisu->state) break; \
      } \
      if (!csisu || (csisu && (AVD_SU_SI_STATE_ASGND == csisu->fsm))) \
         is_si_eng = TRUE; \
      if (FALSE == is_si_eng) { \
         (is) = FALSE; \
         break; \
      } \
   } \
};

/*****************************************************************************
 * Function: avd_sg_nway_si_func
 *
 * Purpose:  This function is called when a new SI is added to a SG. The SG is
 * of type N-Way redundancy model. This function will perform the functionality
 * described in the SG FSM design. 
 *
 * Input: cb - the AVD control block
 *        si - The pointer to the service instance.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function. If there are
 * any SIs being transitioned due to operator, this call will return
 * failure.
 *
 **************************************************************************/
uns32 avd_sg_nway_si_func(AVD_CL_CB *cb, AVD_SI *si)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* If the SG FSM state is not stable just return success. */
	if (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE)
		return rc;

	if ((cb->init_state != AVD_APP_STATE) && (si->sg_of_si->sg_ncs_spec == SA_FALSE)) {
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, si->sg_of_si->sg_ncs_spec);
		return rc;
	}

	/* assign the si to the appropriate su */
	rc = avd_sg_nway_si_assign(cb, si->sg_of_si);

	return rc;
}

/*****************************************************************************
 * Function: avd_sg_nway_siswitch_func
 *
 * Purpose:  This function is called when a operator does a SI switch on
 * a SI that belongs to N-Way redundancy model SG. 
 * This will trigger a role change action as described in the SG FSM design.
 *
 * Input: cb - the AVD control block
 *        si - The pointer to the SI that needs to be switched.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function.
 *
 **************************************************************************/
uns32 avd_sg_nway_siswitch_func(AVD_CL_CB *cb, AVD_SI *si)
{
	AVD_SU_SI_REL *curr_susi = 0;
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%u", si->sg_of_si->sg_fsm_state);

	/* Switch operation is not allowed on NCS SGs, when the AvD is not in
	 * application state, if si has no SU assignments and If the SG FSM state is
	 * not stable.
	 */
	if ((cb->init_state != AVD_APP_STATE) ||
	    (si->sg_of_si->sg_ncs_spec == SA_TRUE) ||
	    (si->sg_of_si->sg_fsm_state != AVD_SG_FSM_STABLE) || (si->list_of_sisu == AVD_SU_SI_REL_NULL))
		return NCSCC_RC_FAILURE;

	/* switch operation is not valid on SIs that have only one assignment.
	 */
	if (si->list_of_sisu->si_next == AVD_SU_SI_REL_NULL)
		return NCSCC_RC_FAILURE;

	/* identify the active susi rel */
	for (curr_susi = si->list_of_sisu;
	     curr_susi && (curr_susi->state != SA_AMF_HA_ACTIVE); curr_susi = curr_susi->si_next) ;

	if (!curr_susi) {
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
		return NCSCC_RC_FAILURE;
	}

	/* send quiesced assignment */
	old_state = curr_susi->state;
	old_fsm_state = curr_susi->fsm;
	curr_susi->state = SA_AMF_HA_QUIESCED;
	curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
	avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
	rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
		curr_susi->state = old_state;
		curr_susi->fsm = old_fsm_state;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
		avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
		return rc;
	}

	/* set the toggle switch flag */
	m_AVD_SET_SI_SWITCH(cb, si, AVSV_SI_TOGGLE_SWITCH);

	/* Add the SI to the SG admin pointer and change the SG state to SI_operation. */
	m_AVD_SET_SG_ADMIN_SI(cb, si);
	m_AVD_SET_SG_FSM(cb, si->sg_of_si, AVD_SG_FSM_SI_OPER);

	return rc;
}

 /*****************************************************************************
 * Function: avd_sg_nway_su_fault_func
 *
 * Purpose:  This function is called when a SU readiness state changes to
 * OOS due to a fault. It will do the functionality specified in
 * SG FSM.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the service unit.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: None.
 *
 **************************************************************************/
uns32 avd_sg_nway_su_fault_func(AVD_CL_CB *cb, AVD_SU *su)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if (!su->list_of_susi)
		return rc;

	/* Do the functionality based on the current state. */
	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		rc = avd_sg_nway_su_fault_stable(cb, su);
		break;

	case AVD_SG_FSM_SG_REALIGN:
		rc = avd_sg_nway_su_fault_sg_realign(cb, su);
		break;

	case AVD_SG_FSM_SU_OPER:
		rc = avd_sg_nway_su_fault_su_oper(cb, su);
		break;

	case AVD_SG_FSM_SI_OPER:
		rc = avd_sg_nway_su_fault_si_oper(cb, su);
		break;

	case AVD_SG_FSM_SG_ADMIN:
		rc = avd_sg_nway_su_fault_sg_admin(cb, su);
		break;

	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->sg_fsm_state);
		return NCSCC_RC_FAILURE;
	}			/* switch */

	return rc;
}

 /*****************************************************************************
 * Function: avd_sg_nway_su_insvc_func
 *
 * Purpose:  This function is called when a SU readiness state changes
 * to inservice from out of service. The SG is of type N-Way redundancy
 * model. It will do the functionality specified in the SG FSM.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the service unit.
 *        
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: none.
 *
 **************************************************************************/
uns32 avd_sg_nway_su_insvc_func(AVD_CL_CB *cb, AVD_SU *su)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("'%s', %u", su->name.value, su->sg_of_su->sg_fsm_state);

	/* An SU will not become in service when the SG is being locked or shutdown.
	 */
	if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_SG_ADMIN) {
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->sg_fsm_state);
		return NCSCC_RC_FAILURE;
	}

	/* If the SG FSM state is not stable just return success. */
	if (su->sg_of_su->sg_fsm_state != AVD_SG_FSM_STABLE)
		return rc;

	if ((cb->init_state != AVD_APP_STATE) && (su->sg_of_su->sg_ncs_spec == SA_FALSE))
		return rc;

	/* a new su is available for assignments.. start assigning */
	rc = avd_sg_nway_si_assign(cb, su->sg_of_su);

	if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) {
		avd_sg_app_su_inst_func(cb, su->sg_of_su);
		/* if equal ranked su then screen for redistribution
		 * of the SIs if the SG fsm state is still STABLE */
		if ((su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) &&
				(su->sg_of_su->equal_ranked_su == TRUE) &&
				(su->sg_of_su->saAmfSGAutoAdjust == SA_TRUE))
			avd_sg_nway_screen_si_distr_equal(su->sg_of_su);
	}

	return rc;
}

/*****************************************************************************
 * Function: avd_sg_nway_susi_sucss_func
 *
 * Purpose:  This function is called when a SU SI ack function is
 * received from the AVND with success value. The SG FSM for N-Way redundancy
 * model will be run. The SUSI fsm state will
 * be changed to assigned or it will freed for the SU SI. 
 * 
 *
 * Input: cb - the AVD control block
 *        su - In case of entire SU related operation the SU for
 *               which the ack is received.
 *        susi - The pointer to the service unit service instance relationship.
 *        act  - The action received in the ack message.
 *        state - The HA state in the message.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function.
 * 
 **************************************************************************/
uns32 avd_sg_nway_susi_sucss_func(AVD_CL_CB *cb,
				  AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	/* Do the functionality based on the current state. */
	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_SG_REALIGN:
		rc = avd_sg_nway_susi_succ_sg_realign(cb, su, susi, act, state);
		break;

	case AVD_SG_FSM_SU_OPER:
		rc = avd_sg_nway_susi_succ_su_oper(cb, su, susi, act, state);
		break;

	case AVD_SG_FSM_SI_OPER:
		rc = avd_sg_nway_susi_succ_si_oper(cb, su, susi, act, state);
		break;

	case AVD_SG_FSM_SG_ADMIN:
		rc = avd_sg_nway_susi_succ_sg_admin(cb, su, susi, act, state);
		break;

	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, su->sg_of_su->sg_fsm_state);
		return NCSCC_RC_FAILURE;
	}			/* switch */

	if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) {
		avd_sg_app_su_inst_func(cb, su->sg_of_su);
		/* if equal ranked su then screen for redistribution
		 * of the SIs if the SG fsm state is still STABLE */
		if ((su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) &&
				(su->sg_of_su->equal_ranked_su == TRUE) &&
				(su->sg_of_su->saAmfSGAutoAdjust == SA_TRUE))
			avd_sg_nway_screen_si_distr_equal(su->sg_of_su);
		avd_sg_screen_si_si_dependencies(cb, su->sg_of_su);
	}
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_nway_susi_fail_func
 *
 * Purpose:  This function is called when a SU SI ack function is
 * received from the AVND with some error value. The message may be an
 * ack for a particular SU SI or for the entire SU. It will log an event
 * about the failure. Since if a CSI set callback returns error it is
 * considered as failure of the component, AvND would have updated that
 * info for each of the components that failed and also for the SU an
 * operation state message would be sent the processing will be done in that
 * event context. For faulted SU this event would be considered as
 * completion of action, for healthy SU no SUSI state change will be done. 
 * 
 *
 * Input: cb - the AVD control block
 *        su - In case of entire SU related operation the SU for
 *               which the ack is received.
 *        susi - The pointer to the service unit service instance relationship.
 *        act  - The action received in the ack message.
 *        state - The HA state in the message.
 *        
 * Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function.
 * 
 **************************************************************************/
uns32 avd_sg_nway_susi_fail_func(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *curr_susi = 0;
	AVD_SG *sg = su->sg_of_su;
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	NCS_BOOL is_eng = FALSE;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_SU_OPER:
		if (susi && (SA_AMF_HA_QUIESCED == susi->state)) {
			/* send remove all msg for all sis for this su */
			rc = avd_sg_su_si_del_snd(cb, susi->su);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
				return rc;
			}
		}
		break;

	case AVD_SG_FSM_SI_OPER:
		if (susi && (SA_AMF_HA_QUIESCED == susi->state) &&
		    (sg->admin_si == susi->si) &&
		    (AVSV_SI_TOGGLE_SWITCH == susi->si->si_switch) &&
		    (SA_AMF_READINESS_IN_SERVICE == susi->su->saAmfSuReadinessState)) {
			/* send active assignment */
			old_state = susi->state;
			old_fsm_state = susi->fsm;
			susi->state = SA_AMF_HA_ACTIVE;
			susi->fsm = AVD_SU_SI_STATE_MODIFY;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
			avd_gen_su_ha_state_changed_ntf(cb, susi);
			rc = avd_snd_susi_msg(cb, susi->su, susi, AVSV_SUSI_ACT_MOD, false, NULL);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
				susi->state = old_state;
				susi->fsm = old_fsm_state;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, susi);
				return rc;
			}

			/* reset the switch operation */
			m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
			m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);

			/* add the su to su-oper list */
			avd_sg_su_oper_list_add(cb, susi->su, FALSE);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		} else if (susi && sg->si_tobe_redistributed == susi->si) {
			/* if si redistribution is going on */
			if ((susi->state == SA_AMF_HA_QUIESCED) && (susi->su == sg->max_assigned_su)) {
				/* if the si is being quiesced on max su in SI transfer */
				/* send active assignment */
				old_state =susi->state;
				old_fsm_state = susi->fsm;
				susi->state = SA_AMF_HA_ACTIVE;
				susi->fsm = AVD_SU_SI_STATE_MODIFY;
				rc = avd_snd_susi_msg(cb,susi->su,susi,AVSV_SUSI_ACT_MOD, false, NULL);
				if (rc == NCSCC_RC_SUCCESS) {
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, susi);
					avd_sg_su_oper_list_add(cb, susi->su, FALSE);
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
				} else {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
					susi->state = old_state;
					susi->fsm = old_fsm_state;
				}
			}
		       	else if ((susi->state == SA_AMF_HA_ACTIVE) && (susi->su == sg->min_assigned_su)) {
				AVD_SU_SI_REL *t_susi = NULL;
				/* if the si is assigned active on min su in SI transfer */
				/* delete the quiesced assignment on the max su  and let su failure
				 * handling take care of assigning the standby to active */
				/* find the susi with max assigned su which should be by now quiesced */
				t_susi = avd_su_susi_find(avd_cb, susi->si->sg_of_si->max_assigned_su, &susi->si->name);
				assert(t_susi->state == SA_AMF_HA_QUIESCED);

				old_fsm_state = t_susi->fsm;
				t_susi->fsm = AVD_SU_SI_STATE_UNASGN;
				if (avd_snd_susi_msg(avd_cb, t_susi->su, t_susi, AVSV_SUSI_ACT_DEL, false, NULL) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, t_susi->su->name.value, t_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, t_susi->si->name.value, t_susi->si->name.length);
					t_susi->fsm = old_fsm_state;
				} else {
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, t_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(avd_cb, t_susi);
					/* add the su to su-oper list and move the SG to Realign */
					avd_sg_su_oper_list_add(avd_cb, t_susi->su, FALSE);
					m_AVD_SET_SG_FSM(avd_cb, t_susi->su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
				}
			}
			/* reset all the pointers marked for si transfer */
			sg->si_tobe_redistributed = NULL;
			sg->max_assigned_su = NULL;
			sg->min_assigned_su = NULL;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, sg, AVSV_CKPT_AVD_SI_TRANS);
			TRACE ("SI transfer FAILED '%s'", susi->si->name.value);
		}
		break;

	case AVD_SG_FSM_SG_ADMIN:
		if (susi && (SA_AMF_HA_QUIESCED == state) && (AVSV_SUSI_ACT_DEL != act)) {
			/* => single quiesced failure */
			if (SA_AMF_ADMIN_LOCKED == sg->saAmfSGAdminState) {
				/* determine if the in-svc su has any quiesced assigning assignment */
				if (SA_AMF_READINESS_IN_SERVICE == susi->su->saAmfSuReadinessState) {
					for (curr_susi = susi->su->list_of_susi; curr_susi;
					     curr_susi = curr_susi->su_next) {
						if (curr_susi == susi)
							continue;

						if ((SA_AMF_HA_QUIESCED == curr_susi->state) &&
						    (curr_susi->fsm == AVD_SU_SI_STATE_MODIFY))
							break;
					}
				}

				if (!curr_susi) {
					/* => now remove all can be sent */
					rc = avd_sg_su_si_del_snd(cb, su);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
						return rc;
					}
				}
			}
		}
		break;

	case AVD_SG_FSM_SG_REALIGN:
		if (susi && (SA_AMF_HA_QUIESCING == susi->state)) {
			/* send quiesced assignment */
			old_state = susi->state;
			old_fsm_state = susi->fsm;
			susi->state = SA_AMF_HA_QUIESCED;
			susi->fsm = AVD_SU_SI_STATE_MODIFY;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
			avd_gen_su_ha_state_changed_ntf(cb, susi);
			rc = avd_snd_susi_msg(cb, susi->su, susi, AVSV_SUSI_ACT_MOD, false, NULL);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
				susi->state = old_state;
				susi->fsm = old_fsm_state;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, susi);
				return rc;
			}
		} else if (susi && (SA_AMF_HA_QUIESCED == susi->state)) {
			if (sg->admin_si != susi->si) {
				/* determine if all the standby sus are engaged */
				m_AVD_SG_NWAY_ARE_STDBY_SUS_ENGAGED(susi->su, susi, is_eng);
				if (TRUE == is_eng) {
					/* send remove all msg for all sis for this su */
					rc = avd_sg_su_si_del_snd(cb, susi->su);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value,
										 susi->su->name.length);
						return rc;
					}
				}
			}

			if (FALSE == is_eng) {
				/* send remove one msg */
				old_fsm_state = susi->fsm;
				susi->fsm = AVD_SU_SI_STATE_UNASGN;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				rc = avd_snd_susi_msg(cb, susi->su, susi, AVSV_SUSI_ACT_DEL, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
					susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
					return rc;
				}
			}
		}
		break;

	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uns32)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		return NCSCC_RC_FAILURE;
	}			/* switch */

	return rc;
}

 /*****************************************************************************
 * Function: avd_sg_nway_realign_func
 *
 * Purpose:  This function will call the chose assign function to check and
 * assign SIs. If any assigning is being done it adds the SU to the operation
 * list and sets the SG FSM state to SG realign. It resets the ncsSGAdjustState.
 * If everything is 
 * fine, it calls the routine to bring the preffered number of SUs to 
 * inservice state and change the SG state to stable. The functionality is
 * described in the SG FSM. The same function is used for both cluster_timer and
 * and sg_operator events as described in the SG FSM.
 *
 * Input: cb - the AVD control block
 *        sg - The pointer to the service group.
 *        
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: none.
 *
 **************************************************************************/
uns32 avd_sg_nway_realign_func(AVD_CL_CB *cb, AVD_SG *sg)
{
	TRACE_ENTER2("'%s'", sg->name.value);

	/* If the SG FSM state is not stable just return success. */
	if ((cb->init_state != AVD_APP_STATE) && (sg->sg_ncs_spec == SA_FALSE)) {
		goto done;
	}

	if (sg->sg_fsm_state != AVD_SG_FSM_STABLE) {
		m_AVD_SET_SG_ADJUST(cb, sg, AVSV_SG_STABLE);
		avd_sg_app_su_inst_func(cb, sg);
		goto done;
	}

	/* initiate si assignments */
	avd_sg_nway_si_assign(cb, sg);

	if (sg->sg_fsm_state == AVD_SG_FSM_STABLE) {
		avd_sg_app_su_inst_func(cb, sg);
		/* if equal ranked su then screen for redistribution
		 * of the SIs if the SG fsm state is still STABLE */
		if ((sg->sg_fsm_state == AVD_SG_FSM_STABLE) &&
				(sg->equal_ranked_su == TRUE) &&
				(sg->saAmfSGAutoAdjust == SA_TRUE))
			avd_sg_nway_screen_si_distr_equal(sg);
	}
 done:
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_sg_nway_node_fail_func
 *
 * Purpose:  This function is called when the node has already failed and
 *           the SIs have to be failed over. It does the functionality 
 *           specified in the design.
 *
 * Input: cb - the AVD control block
 *        su - The SU that has faulted because of the node failure.
 *        
 * Returns: None.
 *
 * Notes: This is a N-Way redundancy model specific function.
 * 
 **************************************************************************/
void avd_sg_nway_node_fail_func(AVD_CL_CB *cb, AVD_SU *su)
{
	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if (!su->list_of_susi)
		return;

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		avd_sg_nway_node_fail_stable(cb, su, 0);
		break;

	case AVD_SG_FSM_SU_OPER:
		avd_sg_nway_node_fail_su_oper(cb, su);
		break;

	case AVD_SG_FSM_SI_OPER:
		avd_sg_nway_node_fail_si_oper(cb, su);
		break;

	case AVD_SG_FSM_SG_ADMIN:
		avd_sg_nway_node_fail_sg_admin(cb, su);
		break;

	case AVD_SG_FSM_SG_REALIGN:
		avd_sg_nway_node_fail_sg_realign(cb, su);
		break;

	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uns32)su->sg_of_su->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		return;
	}			/* switch */

	if (su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) {
		avd_sg_app_su_inst_func(cb, su->sg_of_su);
		/* if equal ranked su then screen for redistribution
		 * of the SIs if the SG fsm state is still STABLE */
		if ((su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) &&
				(su->sg_of_su->equal_ranked_su == TRUE) &&
				(su->sg_of_su->saAmfSGAutoAdjust == SA_TRUE))
			avd_sg_nway_screen_si_distr_equal(su->sg_of_su);
		avd_sg_screen_si_si_dependencies(cb, su->sg_of_su);
	}
	return;
}

/*****************************************************************************
 * Function: avd_sg_nway_su_admin_fail
 *
 * Purpose:  This function is called when SU become OOS because of the
 * LOCK or shutdown of the SU or node.The functionality will be as described in
 * the SG design FSM. 
 *
 * Input: cb - the AVD control block
 *        su - The SU that has failed because of the admin operation.
 *        avnd - The AvND structure of the node that is being operated upon.
 *        
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function. The avnd pointer
 * value is valid only if this is a SU operation being done because of the node
 * admin change.
 *
 **************************************************************************/
uns32 avd_sg_nway_su_admin_fail(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd)
{
	AVD_SU_SI_REL *curr_susi = 0;
	AVD_SU_SI_STATE old_fsm_state;
	SaAmfHAStateT old_state, state;
	NCS_BOOL is_all_stdby = TRUE;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%u", su->sg_of_su->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (su->sg_of_su->sg_ncs_spec == SA_FALSE))
		return NCSCC_RC_FAILURE;

	switch (su->sg_of_su->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if (((SA_AMF_ADMIN_LOCKED == su->saAmfSUAdminState)
		     || (SA_AMF_ADMIN_SHUTTING_DOWN == su->saAmfSUAdminState)) || (avnd
										   &&
										   ((SA_AMF_ADMIN_LOCKED ==
										     avnd->saAmfNodeAdminState)
										    || (SA_AMF_ADMIN_SHUTTING_DOWN ==
											avnd->saAmfNodeAdminState)))) {
			/* determine the modify-state for active sis */
			state = ((SA_AMF_ADMIN_LOCKED == su->saAmfSUAdminState) ||
				 (avnd && (SA_AMF_ADMIN_LOCKED == avnd->saAmfNodeAdminState))) ?
			    SA_AMF_HA_QUIESCED : SA_AMF_HA_QUIESCING;

			/* identify all the active assignments & send quiesced / quiescing assignment */
			for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
				if (SA_AMF_HA_ACTIVE != curr_susi->state)
					continue;

				/* send quiesced / quiescing assignment */
				old_state = curr_susi->state;
				old_fsm_state = curr_susi->fsm;
				curr_susi->state = state;
				curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
									 curr_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
									 curr_susi->si->name.length);
					curr_susi->state = old_state;
					curr_susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					return rc;
				}

				/* transition to su-oper state */
				m_AVD_SET_SG_FSM(cb, su->sg_of_su, AVD_SG_FSM_SU_OPER);

				is_all_stdby = FALSE;
			}	/* for */

			/* if all standby assignments.. remove them */
			if (TRUE == is_all_stdby) {
				rc = avd_sg_su_si_del_snd(cb, su);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return rc;
				}

				if (SA_AMF_ADMIN_SHUTTING_DOWN == su->saAmfSUAdminState) {
					avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
				}

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
			}

			/* add su to the su-oper list */
			avd_sg_su_oper_list_add(cb, su, FALSE);
		}
		break;

	case AVD_SG_FSM_SU_OPER:
		if ((su->sg_of_su->su_oper_list.su == su) &&
		    ((su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED) ||
		     (avnd && (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED)))) {
			/* identify all the quiescing assignments & send quiesced assignment */
			for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
				if (SA_AMF_HA_QUIESCING != curr_susi->state)
					continue;

				/* send quiesced assignment */
				old_state = curr_susi->state;
				old_fsm_state = curr_susi->fsm;
				curr_susi->state = SA_AMF_HA_QUIESCED;
				curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);

				rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
									 curr_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
									 curr_susi->si->name.length);
					curr_susi->state = old_state;
					curr_susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					return rc;
				}
			}	/* for */
		}
		break;

	default:
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ((uns32)su->sg_of_su->sg_fsm_state));
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
		return NCSCC_RC_FAILURE;
	}			/* switch */

	return rc;
}

/*****************************************************************************
 * Function: avd_sg_nway_si_admin_down
 *
 * Purpose:  This function is called when SIs admin state is changed to
 * LOCK or shutdown. The functionality will be as described in
 * the SG design FSM. 
 *
 * Input: cb - the AVD control block
 *        si - The SI pointer.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function.
 *
 **************************************************************************/
uns32 avd_sg_nway_si_admin_down(AVD_CL_CB *cb, AVD_SI *si)
{
	AVD_SU_SI_REL *curr_susi = 0;
	AVD_SU_SI_STATE old_fsm_state;
	SaAmfHAStateT old_state;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%u", si->sg_of_si->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (si->sg_of_si->sg_ncs_spec == SA_FALSE))
		return NCSCC_RC_FAILURE;

	/* if nothing's assigned, do nothing */
	if (si->list_of_sisu == AVD_SU_SI_REL_NULL)
		return NCSCC_RC_SUCCESS;

	switch (si->sg_of_si->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if ((SA_AMF_ADMIN_LOCKED == si->saAmfSIAdminState) ||
		    (SA_AMF_ADMIN_SHUTTING_DOWN == si->saAmfSIAdminState)) {
			/* identify the active susi rel */
			for (curr_susi = si->list_of_sisu;
			     curr_susi && (curr_susi->state != SA_AMF_HA_ACTIVE); curr_susi = curr_susi->si_next) ;

			if (!curr_susi) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
				return NCSCC_RC_FAILURE;
			}

			/* send quiesced / quiescing assignment */
			old_state = curr_susi->state;
			old_fsm_state = curr_susi->fsm;
			curr_susi->state = (SA_AMF_ADMIN_LOCKED == si->saAmfSIAdminState) ?
			    SA_AMF_HA_QUIESCED : SA_AMF_HA_QUIESCING;
			curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
			avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
			rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
				curr_susi->state = old_state;
				curr_susi->fsm = old_fsm_state;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				return rc;
			}

			/* Add the SI to the SG admin pointer and change the SG state to SI_operation. */
			m_AVD_SET_SG_ADMIN_SI(cb, si);
			m_AVD_SET_SG_FSM(cb, si->sg_of_si, AVD_SG_FSM_SI_OPER);
		}
		break;

	case AVD_SG_FSM_SI_OPER:
		if ((si->sg_of_si->admin_si == si) && (SA_AMF_ADMIN_LOCKED == si->saAmfSIAdminState)) {
			/* the si that was being shutdown is now being locked.. 
			   identify the quiescing susi rel */
			for (curr_susi = si->list_of_sisu;
			     curr_susi && (curr_susi->state != SA_AMF_HA_QUIESCING); curr_susi = curr_susi->si_next) ;

			if (!curr_susi) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
				return NCSCC_RC_FAILURE;
			}

			/* send quiesced assignment */
			old_state = curr_susi->state;
			old_fsm_state = curr_susi->fsm;
			curr_susi->state = SA_AMF_HA_QUIESCED;
			curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
			avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
			rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
				curr_susi->state = old_state;
				curr_susi->fsm = old_fsm_state;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				return rc;
			}
		}
		break;

	default:
		LOG_ER("%s:%u: %u", __FILE__, __LINE__, ((uns32)si->sg_of_si->sg_fsm_state));
		LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, si->name.value, si->name.length);
		return NCSCC_RC_FAILURE;
	}			/* switch */

	return rc;
}

/*****************************************************************************
 * Function: avd_sg_nway_sg_admin_down
 *
 * Purpose:  This function is called when SGs admin state is changed to
 * LOCK or shutdown. The functionality will be as described in
 * the SG design FSM. 
 *
 * Input: cb - the AVD control block
 *        sg - The SG pointer.
 *        
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes: This is a N-Way redundancy model specific function.
 * 
 **************************************************************************/
uns32 avd_sg_nway_sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg)
{
	AVD_SU_SI_REL *curr_susi = 0;
	AVD_SU *curr_su = 0;
	AVD_SI *curr_si = 0;
	AVD_SU_SI_STATE old_fsm_state;
	SaAmfHAStateT old_state;
	NCS_BOOL is_act_asgn;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%u", sg->sg_fsm_state);

	if ((cb->init_state != AVD_APP_STATE) && (sg->sg_ncs_spec == SA_FALSE))
		return NCSCC_RC_FAILURE;

	switch (sg->sg_fsm_state) {
	case AVD_SG_FSM_STABLE:
		if ((SA_AMF_ADMIN_LOCKED == sg->saAmfSGAdminState) ||
		    (SA_AMF_ADMIN_SHUTTING_DOWN == sg->saAmfSGAdminState)) {
			/* identify & send quiesced / quiescing assignment to all the 
			   active susi assignments in this sg */
			for (curr_su = sg->list_of_su; curr_su; curr_su = curr_su->sg_list_su_next) {
				/* skip the su if there are no assignments */
				if (!curr_su->list_of_susi)
					continue;

				is_act_asgn = FALSE;
				for (curr_susi = curr_su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
					if (SA_AMF_HA_ACTIVE != curr_susi->state)
						continue;

					is_act_asgn = TRUE;
					old_state = curr_susi->state;
					old_fsm_state = curr_susi->fsm;
					curr_susi->state = (SA_AMF_ADMIN_LOCKED == sg->saAmfSGAdminState) ?
					    SA_AMF_HA_QUIESCED : SA_AMF_HA_QUIESCING;
					curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
										 curr_susi->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
										 curr_susi->si->name.length);
						curr_susi->state = old_state;
						curr_susi->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
						return rc;
					}
				}	/* for */

				/* if there are no active assignments to this su, send remove all message */
				if (FALSE == is_act_asgn) {
					/* send remove all msg for all sis for this su */
					rc = avd_sg_su_si_del_snd(cb, curr_su);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_su->name.value,
										 curr_su->name.length);
						return rc;
					}
				}

				/* add the su to su-oper list & transition to sg-admin state */
				avd_sg_su_oper_list_add(cb, curr_su, FALSE);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_ADMIN);
			}	/* for */

			/* if sg is still in stable state, lock it */
			if (AVD_SG_FSM_STABLE == sg->sg_fsm_state) {
				avd_sg_admin_state_set(sg, SA_AMF_ADMIN_LOCKED);
			}
		}
		break;

	case AVD_SG_FSM_SG_ADMIN:
		if (SA_AMF_ADMIN_LOCKED == sg->saAmfSGAdminState) {
			/* pick up all the quiescing susi assignments & 
			   send quiesced assignments to them */
			for (curr_si = sg->list_of_si; curr_si; curr_si = curr_si->sg_list_of_si_next) {
				for (curr_susi = curr_si->list_of_sisu;
				     curr_susi && (SA_AMF_HA_QUIESCING != curr_susi->state);
				     curr_susi = curr_susi->si_next) ;

				if (curr_susi) {
					old_state = curr_susi->state;
					old_fsm_state = curr_susi->fsm;
					curr_susi->state = SA_AMF_HA_QUIESCED;
					curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
										 curr_susi->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
										 curr_susi->si->name.length);
						curr_susi->state = old_state;
						curr_susi->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
						return rc;
					}
				}
			}	/* for */
		}
		break;

	default:
		LOG_EM("%s:%u: %u", __FILE__, __LINE__, ((uns32)sg->sg_fsm_state));
		LOG_EM("%s:%u: %s (%u)", __FILE__, __LINE__, sg->name.value, sg->name.length);
		return NCSCC_RC_FAILURE;
	}			/* switch */

	return rc;
}

/**
 *
 * @brief    This function transfers the si_tobe_redistributed in this sg 
 * 	     from a Maximum assigned SU to a Minimum assinged SU, that are 
 * 	     populated in the AVD_SG fields max_assigned_su, min_assigned_su
 * 	     prior to calling this function. 
 * 	     
 * @param[in] sg
 *
 * @return    none 
 *
 */
static void avd_sg_nway_swap_si_redistr(AVD_SG *sg)
{
	AVD_SU_SI_REL *susi = NULL;
	AVD_SU_SI_STATE old_fsm_state;
	SaAmfHAStateT old_ha_state;

	assert(sg->si_tobe_redistributed != NULL);
	assert(sg->max_assigned_su != NULL);
	assert(sg->min_assigned_su != NULL);

	TRACE("SI transfer '%s' from '%s' to '%s'",
			sg->si_tobe_redistributed->name.value,
			sg->max_assigned_su->name.value,
			sg->min_assigned_su->name.value);

	/* check point the SI transfer parameters with STANDBY*/
	m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(avd_cb, sg, AVSV_CKPT_AVD_SI_TRANS);

	/* get the susi that is to be transferred */
	susi = avd_su_susi_find(avd_cb, sg->max_assigned_su, &sg->si_tobe_redistributed->name); 
	assert(susi != NULL);

	old_fsm_state = susi->fsm;
	old_ha_state = susi->state;

	if (susi->state == SA_AMF_HA_ACTIVE) {
		/* if active assignment of an SI is to be transferred then first make the SU 
		 * which is having the current active assignment to quiesced and then
		 * once the susi response comes make the assignment on the min su
		 */
		susi->state = SA_AMF_HA_QUIESCED;
		susi->fsm = AVD_SU_SI_STATE_MODIFY;
		if (avd_snd_susi_msg(avd_cb, susi->su, susi, AVSV_SUSI_ACT_MOD, false, NULL) == NCSCC_RC_FAILURE) {
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
			susi->state = old_ha_state;
			susi->fsm = old_fsm_state;
			sg->max_assigned_su = NULL;
			sg->min_assigned_su = NULL;
			sg->si_tobe_redistributed = NULL;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, sg, AVSV_CKPT_AVD_SI_TRANS);
		} else {
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, susi, AVSV_CKPT_AVD_SI_ASS);
			avd_gen_su_ha_state_changed_ntf(avd_cb, susi);
			m_AVD_SET_SG_FSM(avd_cb, susi->su->sg_of_su, AVD_SG_FSM_SI_OPER);
		}

	} else if (susi->state == SA_AMF_HA_STANDBY) {
		/* if standby assignment is to be transferred simply remove the standby assignment
		 * and let the SI assignment flow with in the SG take care of assigning the removed SI 
		 * to lowest assigned SU
		 */
		susi->fsm = AVD_SU_SI_STATE_UNASGN;
		if (avd_snd_susi_msg(avd_cb, susi->su, susi, AVSV_SUSI_ACT_DEL, false, NULL) == NCSCC_RC_FAILURE) {
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
			susi->fsm = old_fsm_state;
		} else {
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, susi, AVSV_CKPT_AVD_SI_ASS);
			avd_gen_su_ha_state_changed_ntf(avd_cb, susi);
			avd_sg_su_oper_list_add(avd_cb, susi->su, FALSE);
			m_AVD_SET_SG_FSM(avd_cb, susi->su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
		}
		/* reset the min max Sus and si to be redistributed 
		 * as they are not needed further, SG si assignment 
		 * flow will take care of assigning the removed SI 
		 * to the possible minimum assigned SU
		 */
		sg->max_assigned_su = NULL;
		sg->min_assigned_su = NULL;
		sg->si_tobe_redistributed = NULL;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, sg, AVSV_CKPT_AVD_SI_TRANS);
	} else {
		/* susi to be redistributed cannot be in any other HA state */
		assert(0);
	}
}

/**
 *
 * @brief   This routine scans the SU list for the specified SG & picks up 
 *          an SU with minimum number of assignments and SU maximum number 
 *          of assignments and initiates a transfer of SI from maximum 
 *          assigned SU to minimum assigned SU 
 *
 * @param[in] sg 
 *
 * @return none
 */
void avd_sg_nway_screen_si_distr_equal(AVD_SG *sg)
{
	AVD_SU *curr_su = NULL;
	AVD_SI *curr_si = NULL;
	AVD_SU_SI_REL *susi = NULL;

	TRACE_ENTER();

	sg->max_assigned_su = NULL; 
	sg->min_assigned_su = NULL;
	sg->si_tobe_redistributed = NULL;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, sg, AVSV_CKPT_AVD_SI_TRANS);

	/* first screen for equal number of active assignments 
	 * among the SUs if active assignments are already 
	 * equally distributed among the in-service SUs then
	 * check for equal distribution of standby assignments 
	 */

	/* do screening for active assignements */
	curr_su = sg->list_of_su;

	while (curr_su != NULL) {
		if (curr_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
			/* set the max and min su ptr to the first inservice su */
			if (sg->max_assigned_su == NULL)
				sg->max_assigned_su = curr_su;

			if (sg->min_assigned_su == NULL)
				sg->min_assigned_su = curr_su;

			if (curr_su->saAmfSUNumCurrActiveSIs > sg->max_assigned_su->saAmfSUNumCurrActiveSIs)
				sg->max_assigned_su = curr_su;
			else if (curr_su->saAmfSUNumCurrActiveSIs < sg->min_assigned_su->saAmfSUNumCurrActiveSIs)
				sg->min_assigned_su = curr_su;
			TRACE("'%s'-'%d'\n'%s'-'%d'\n'%s'-'%d'", curr_su->name.value, curr_su->saAmfSUNumCurrActiveSIs,
					sg->max_assigned_su->name.value, sg->max_assigned_su->saAmfSUNumCurrActiveSIs,
					sg->min_assigned_su->name.value, sg->min_assigned_su->saAmfSUNumCurrActiveSIs);
		}
		curr_su = curr_su->sg_list_su_next;
	}

	/* this means there are no inservice sus nothing to be done */
	if (sg->max_assigned_su == NULL || sg->min_assigned_su == NULL) {
		TRACE("No redistribution of SI assignments needed");
		goto done;
	}
	else if ((sg->max_assigned_su->saAmfSUNumCurrActiveSIs - sg->min_assigned_su->saAmfSUNumCurrActiveSIs) > 1) {
		TRACE("Redistribution of active assignments initiated");
		TRACE("max_assigned_su '%s' no of act assignments '%u', min_assigned_su '%s' no of act assignments '%u'", 
				sg->max_assigned_su->name.value, sg->max_assigned_su->saAmfSUNumCurrActiveSIs, 
				sg->min_assigned_su->name.value, sg->min_assigned_su->saAmfSUNumCurrActiveSIs);

		/* Now find the SI whose active assignment needs to be transferred */
		susi = sg->max_assigned_su->list_of_susi;

		while (susi != NULL) {
			if (susi->state == SA_AMF_HA_ACTIVE) {
				if (curr_si == NULL)
					curr_si = susi->si;
				else if (curr_si->saAmfSIRank < susi->si->saAmfSIRank)
					curr_si = susi->si;
			}
			susi = susi->su_next;
		}

		if (curr_si != NULL) {
			sg->si_tobe_redistributed = curr_si;
			avd_sg_nway_swap_si_redistr(sg);
			goto done;
		}

		TRACE("Active assignments redistribution not possible at this time");
	}

	TRACE("No redistribution of active assignments needed");
	TRACE("max_assigned_su '%s' no of act assignments '%u', min_assigned_su '%s' no of act assignments '%u'", 
			sg->max_assigned_su->name.value, sg->max_assigned_su->saAmfSUNumCurrActiveSIs, 
			sg->min_assigned_su->name.value, sg->min_assigned_su->saAmfSUNumCurrActiveSIs);

	/* do screening for standby assignments */
	sg->max_assigned_su = sg->min_assigned_su = NULL;
	curr_su = sg->list_of_su;

	while (curr_su != NULL) {
		if (curr_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
			/* set the max and min su ptr to the first inservice su */
			if (sg->max_assigned_su == NULL)
				sg->max_assigned_su = curr_su;

			if (sg->min_assigned_su == NULL)
				sg->min_assigned_su = curr_su;

			if (curr_su->saAmfSUNumCurrStandbySIs > sg->max_assigned_su->saAmfSUNumCurrStandbySIs)
				sg->max_assigned_su = curr_su;
			else if (curr_su->saAmfSUNumCurrStandbySIs < sg->min_assigned_su->saAmfSUNumCurrStandbySIs)
				sg->min_assigned_su = curr_su;
		}
		curr_su = curr_su->sg_list_su_next;
	}

	if ((sg->max_assigned_su->saAmfSUNumCurrStandbySIs - sg->min_assigned_su->saAmfSUNumCurrStandbySIs) > 1) {
		TRACE("Redistribution of standby assignments initiated");
		TRACE("max_assigned_su '%s' no of std assignments '%u', min_assigned_su '%s' no of std assignments '%u'", 
				sg->max_assigned_su->name.value, sg->max_assigned_su->saAmfSUNumCurrStandbySIs, 
				sg->min_assigned_su->name.value, sg->min_assigned_su->saAmfSUNumCurrStandbySIs);

		/* Now find the SI whose standby assignment can be transferred */
		susi = sg->max_assigned_su->list_of_susi;

		while (susi != NULL) {

			/* find if this si is already assigned on the min_assigned_su */
			if (avd_su_susi_find(avd_cb, sg->min_assigned_su, &susi->si->name) != AVD_SU_SI_REL_NULL) {
				/* si already assigned on min SU so continue to pick another si */
				susi = susi->su_next;
				continue;
			}

			if (susi->state == SA_AMF_HA_STANDBY) {
				if (curr_si == NULL)
					curr_si = susi->si;
				else if (curr_si->saAmfSIRank < susi->si->saAmfSIRank)
					curr_si = susi->si;
			}
			susi = susi->su_next;
		}

		if (curr_si != NULL ) {
			sg->si_tobe_redistributed = curr_si;
			avd_sg_nway_swap_si_redistr(sg);
			goto done;
		}

		/* if curr_si == NULL means that there is no SI that is 
		 * assigned on max SU and not assigned on min SU
		 * here we cannot redistribute the SIs as of now
		 */
		TRACE("Standby assignments redistribution not possible at this time");
	}

	/* Here means that the SG is load balanced for both 
	 * active and standby SI assignments */
	TRACE("No redistribution of standby assignments needed");
	TRACE("max_assigned_su '%s' no of std assignments '%u', min_assigned_su '%s' no of std assignments '%u'", 
			sg->max_assigned_su->name.value, sg->max_assigned_su->saAmfSUNumCurrStandbySIs, 
			sg->min_assigned_su->name.value, sg->min_assigned_su->saAmfSUNumCurrStandbySIs);
done:
	TRACE_LEAVE();
}

/**
 *
 * @brief   This routine scans the SU list for the specified SG & picks up 
 *          an SU for preferred number of standby assignment for given SI. 
 *
 * @param[in] curr_si 
 *        
 * @param[in] sg 
 *
 * @return AVD_SU - pointer to an qualified service unit
 */
AVD_SU *avd_sg_nway_get_su_std_equal(AVD_SG *sg, AVD_SI *curr_si)
{
	AVD_SU *curr_su = NULL;
	AVD_SU *pref_su = NULL;
	NCS_BOOL l_flag = FALSE;

	TRACE_ENTER2("SI to be assigned : %s", curr_si->name.value);

	for (curr_su = sg->list_of_su; curr_su; curr_su = curr_su->sg_list_su_next) {
		/* verify if this su can take the standby assignment */
		if ((curr_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) ||
				(curr_su->saAmfSUNumCurrStandbySIs >= curr_su->si_max_standby) ||
				((curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU != 0) &&
				 (curr_su->saAmfSUNumCurrStandbySIs >= curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU)))
			continue;

		l_flag = TRUE;

		/* first try to select an SU which has no assignments */
		if ((curr_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
				(curr_su->saAmfSUNumCurrActiveSIs == 0) &&
				(curr_su->saAmfSUNumCurrStandbySIs == 0)) {
			/* got an SU without any assignments select 
			 * it for this SI's standby assignment
			 */
			pref_su = curr_su;
			break;
		}
		/* else try to select an SU with Standby assignments */
		else if ((curr_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
				((curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU == 0) ||
				 (curr_su->saAmfSUNumCurrStandbySIs < curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU)) &&
				(!pref_su || pref_su->saAmfSUNumCurrStandbySIs > curr_su->saAmfSUNumCurrStandbySIs) &&
				(avd_su_susi_find(avd_cb, curr_su, &curr_si->name) == AVD_SU_SI_REL_NULL)) {
			/* mark this as the preferred SU */
			pref_su = curr_su;
		}
	}
	/* no more SIs can be assigned */
	if (l_flag == FALSE)
		curr_si = NULL;

	return pref_su;
}

/*****************************************************************************
 * Function : avd_sg_nway_si_assign
 *
 * Purpose  : This routine scans the SI list for the specified SG & picks up 
 *            an unassigned SI for active assignment. If no unassigned SI is 
 *            found, it picks up the SI that has only active assignment & 
 *            assigns standbys to it (as many as possible).
 *            This routine is invoked when a new-si assignment is received or 
 *            the SG FSM upheaval is over.
 *
 * Input    : cb - the AVD control block
 *            sg - ptr to SG
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : This routine update the su-oper list & moves to sg-realign 
 *            state if any su-si assignment message is sent to AvND.
 * 
 **************************************************************************/
uns32 avd_sg_nway_si_assign(AVD_CL_CB *cb, AVD_SG *sg)
{
	AVD_SI *curr_si = 0;
	AVD_SU *curr_su = NULL;
	AVD_SU *pref_su = NULL;
	AVD_SUS_PER_SI_RANK_INDX i_idx;
	AVD_SUS_PER_SI_RANK *su_rank_rec = 0;
	NCS_BOOL is_act_ass_sent = FALSE, is_all_su_oos = TRUE, is_all_si_ok = FALSE, l_flag = TRUE;
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_SU_SI_REL *tmp_susi;

	TRACE_ENTER();

	sg->sg_fsm_state = AVD_SG_FSM_STABLE;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, sg, AVSV_CKPT_SG_FSM_STATE);

	/* assign active assignments to unassigned sis */
	for (curr_si = sg->list_of_si; curr_si; curr_si = curr_si->sg_list_of_si_next) {
		/* Screen SI sponsors state and adjust the SI-SI dep state accordingly */
		avd_screen_sponsor_si_state(cb, curr_si, FALSE);

		/* verify if si is ready and needs an assignment */
		if ((curr_si->saAmfSIAdminState != SA_AMF_ADMIN_UNLOCKED) ||
		    (curr_si->si_dep_state == AVD_SI_SPONSOR_UNASSIGNED) ||
		    (curr_si->si_dep_state == AVD_SI_UNASSIGNING_DUE_TO_DEP) ||
		    (curr_si->saAmfSIAssignmentState == SA_AMF_ASSIGNMENT_FULLY_ASSIGNED) ||
		    (curr_si->list_of_csi == NULL) ||
		    (curr_si->num_csi != curr_si->max_num_csi))
			continue;

		is_all_si_ok = TRUE;

		/* verify if si is unassigned */
		if (curr_si->list_of_sisu)
			continue;

		/* we've an unassigned si.. find su for active assignment */

		/* first, scan based on su rank for this si */
		memset((uns8 *)&i_idx, '\0', sizeof(i_idx));
		i_idx.si_name = curr_si->name;
		i_idx.su_rank = 0;
		curr_su = NULL;
		for (su_rank_rec = avd_sirankedsu_getnext_valid(cb, i_idx, &curr_su);
		     su_rank_rec; su_rank_rec = avd_sirankedsu_getnext_valid(cb, su_rank_rec->indx, &curr_su)) {
			if (m_CMP_HORDER_SANAMET(su_rank_rec->indx.si_name, curr_si->name) != 0) {
				curr_su = 0;
				break;
			}

			if (!curr_su)
				continue;

			if ((curr_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) &&
			    (curr_su->saAmfSUNumCurrActiveSIs < curr_su->si_max_active) &&
			    ((curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU == 0) ||
			     (curr_su->saAmfSUNumCurrActiveSIs < curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU)))
				break;

			/* reset the curr-su ptr */
			curr_su = 0;
		}

		/* if not found, scan based on su rank for the sg */
		if (!curr_su) {
			/* Reset pref_su for every SI */	
			pref_su = NULL;

			for (curr_su = sg->list_of_su; curr_su; curr_su = curr_su->sg_list_su_next) {
				if (SA_AMF_READINESS_IN_SERVICE == curr_su->saAmfSuReadinessState) {
					is_all_su_oos = FALSE;
					/* if equal distribution is needed when all SUs are of
					 * equal rank or no rank configured for any of the SU
					 */
					if (sg->equal_ranked_su == TRUE) {
						/* first try to select an SU which has no assignments */
						if ((curr_su->saAmfSUNumCurrActiveSIs == 0) &&
							(curr_su->saAmfSUNumCurrStandbySIs == 0)) { 
							/* got an SU without any assignments select 
							 * it for this SI's active assignment
							 */
							pref_su = curr_su;
							break;
						}
						/* else try to select an SU with least active assignments */
						else {
							if ((curr_su->list_of_susi != AVD_SU_SI_REL_NULL) &&
								((curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU == 0) ||
								 (curr_su->saAmfSUNumCurrActiveSIs < curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU)) &&
								(!pref_su || pref_su->saAmfSUNumCurrActiveSIs > curr_su->saAmfSUNumCurrActiveSIs)) 
								/* mark this as the preferred SU */
								pref_su = curr_su;
						}
					}
					else if ((curr_su->saAmfSUNumCurrActiveSIs < curr_su->si_max_active) &&
					    ((curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU == 0) ||
					     (curr_su->saAmfSUNumCurrActiveSIs <
					      curr_su->sg_of_su->saAmfSGMaxActiveSIsperSU)))
						break;
				}
			}

			if (TRUE == is_all_su_oos)
				return NCSCC_RC_SUCCESS;

			/* pref_su is used only for equal_ranked_su, so adding the check */
			if (TRUE == sg->equal_ranked_su)
				curr_su = pref_su;
		}

		/* if found, send active assignment */
		if (curr_su) {
			/* set the flag */
			is_act_ass_sent = TRUE;

			rc = avd_new_assgn_susi(cb, curr_su, curr_si, SA_AMF_HA_ACTIVE, FALSE, &tmp_susi);
			if (NCSCC_RC_SUCCESS == rc) {
				/* add su to the su-oper list & change the fsm state to sg-realign */
				avd_sg_su_oper_list_add(cb, curr_su, FALSE);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_si->name.value, curr_si->name.length);
			}
		} else
			break;
	}			/* for */

	if ((TRUE == is_act_ass_sent) || (FALSE == is_all_si_ok))
		return rc;

	/* assign standby assignments to the sis */
	for (curr_si = sg->list_of_si; curr_si && (TRUE == l_flag); curr_si = curr_si->sg_list_of_si_next) {
		/* verify if si is ready */
		if ((curr_si->saAmfSIAdminState != SA_AMF_ADMIN_UNLOCKED) || (curr_si->num_csi != curr_si->max_num_csi))
			continue;

		/* this si has to have one active assignment */
		if (!curr_si->list_of_sisu)
			continue;

		/* verify if si needs more assigments */
		if (m_AVD_SI_STDBY_CURR_SU(curr_si) == m_AVD_SI_STDBY_MAX_SU(curr_si))
			continue;

		/* we've a not-so-fully-assigned si.. find sus for standby assignment */

		/* first, scan based on su rank for this si */
		memset((uns8 *)&i_idx, '\0', sizeof(i_idx));
		i_idx.si_name = curr_si->name;
		i_idx.su_rank = 0;
		for (su_rank_rec = avd_sirankedsu_getnext_valid(cb, i_idx, &curr_su);
		     su_rank_rec && (m_CMP_HORDER_SANAMET(su_rank_rec->indx.si_name, curr_si->name) == 0);
		     su_rank_rec = avd_sirankedsu_getnext_valid(cb, su_rank_rec->indx, &curr_su)) {
			/* verify if this su can take the standby assignment */
			if (!curr_su || (curr_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) ||
			    (curr_su->saAmfSUNumCurrStandbySIs >= curr_su->si_max_standby) ||
			    ((curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU != 0) &&
			     (curr_su->saAmfSUNumCurrStandbySIs >= curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU)))
				continue;

			/* verify if this su does not have this assignment */
			if (avd_su_susi_find(cb, curr_su, &curr_si->name) != AVD_SU_SI_REL_NULL)
				continue;

			/* send the standby assignment */
			rc = avd_new_assgn_susi(cb, curr_su, curr_si, SA_AMF_HA_STANDBY, FALSE, &tmp_susi);
			if (NCSCC_RC_SUCCESS == rc) {
				/* add su to the su-oper list & change the fsm state to sg-realign */
				avd_sg_su_oper_list_add(cb, curr_su, FALSE);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

				/* verify if si needs more assigments */
				if (m_AVD_SI_STDBY_CURR_SU(curr_si) == m_AVD_SI_STDBY_MAX_SU(curr_si))
					break;
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_si->name.value, curr_si->name.length);
			}
		}

		/* verify if si needs more assigments */
		if (m_AVD_SI_STDBY_CURR_SU(curr_si) == m_AVD_SI_STDBY_MAX_SU(curr_si))
			continue;

		l_flag = FALSE;

		if (sg->equal_ranked_su == TRUE) {

			while (TRUE) {
				/* do the preferred number of standby assignments in equal way */
				curr_su = avd_sg_nway_get_su_std_equal(sg, curr_si);

				if (curr_su == NULL)
					break;

				/* send the standby assignment */
				rc = avd_new_assgn_susi(cb, curr_su, curr_si, SA_AMF_HA_STANDBY, FALSE, &tmp_susi);
				if (NCSCC_RC_SUCCESS == rc) {
					/* add su to the su-oper list & change the fsm state to sg-realign */
					avd_sg_su_oper_list_add(cb, curr_su, FALSE);
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

					/* verify if si needs more assigments */
					if (m_AVD_SI_STDBY_CURR_SU(curr_si) == m_AVD_SI_STDBY_MAX_SU(curr_si))
						break;
				} else {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_si->name.value, curr_si->name.length);
				}
			}
			/* skip to the next SI to be assigned */
			if (curr_si == NULL) /* no more si to be assigned */
				break;
			else /* continue with the next SI assignment */
				continue;
		}

		/* next, scan based on su rank for the sg */
		for (curr_su = sg->list_of_su; curr_su; curr_su = curr_su->sg_list_su_next) {
			/* verify if this su can take the standby assignment */
			if (!curr_su || (curr_su->saAmfSuReadinessState != SA_AMF_READINESS_IN_SERVICE) ||
					(curr_su->saAmfSUNumCurrStandbySIs >= curr_su->si_max_standby) ||
					((curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU != 0) &&
					 (curr_su->saAmfSUNumCurrStandbySIs >= curr_su->sg_of_su->saAmfSGMaxStandbySIsperSU)))
				continue;

			l_flag = TRUE;

			/* verify if this su does not have this assignment */
			if (avd_su_susi_find(cb, curr_su, &curr_si->name) != AVD_SU_SI_REL_NULL) 
				continue;

			/* send the standby assignment */
			rc = avd_new_assgn_susi(cb, curr_su, curr_si, SA_AMF_HA_STANDBY, FALSE, &tmp_susi);
			if (NCSCC_RC_SUCCESS == rc) {
				/* add su to the su-oper list & change the fsm state to sg-realign */
				avd_sg_su_oper_list_add(cb, curr_su, FALSE);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

				/* verify if si needs more assigments */
				if (m_AVD_SI_STDBY_CURR_SU(curr_si) == m_AVD_SI_STDBY_MAX_SU(curr_si))
					break;
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_si->name.value, curr_si->name.length);
			}
		}		/* for */
	}			/* for */

	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_su_fault_stable
 *
 * Purpose  : This routine handles the su-fault event in the stable state.
 *
 * Input    : cb - the AVD control block
 *            su - ptr to SU
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uns32 avd_sg_nway_su_fault_stable(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_susi = 0;
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	NCS_BOOL is_all_stdby = TRUE;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (!su->list_of_susi)
		return rc;

	/* identify all the active assignments & send quiesced assignment */
	for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
		if (SA_AMF_HA_ACTIVE != curr_susi->state)
			continue;

		/* send quiesced assignment */
		old_state = curr_susi->state;
		old_fsm_state = curr_susi->fsm;
		curr_susi->state = SA_AMF_HA_QUIESCED;
		curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
		avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
		rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
			curr_susi->state = old_state;
			curr_susi->fsm = old_fsm_state;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
			avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
			return rc;
		}

		/* transition to su-oper state */
		m_AVD_SET_SG_FSM(cb, su->sg_of_su, AVD_SG_FSM_SU_OPER);

		is_all_stdby = FALSE;
	}			/* for */

	/* if all standby assignments.. remove them */
	if (TRUE == is_all_stdby) {
		rc = avd_sg_su_si_del_snd(cb, su);
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
			return rc;
		}

		/* transition to sg-realign state */
		m_AVD_SET_SG_FSM(cb, su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
	}

	/* add su to the su-oper list */
	avd_sg_su_oper_list_add(cb, su, FALSE);

	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_su_fault_sg_realign
 *
 * Purpose  : This routine handles the su-fault event in the sg-realign state.
 *
 * Input    : cb - the AVD control block
 *            su - ptr to SU
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uns32 avd_sg_nway_su_fault_sg_realign(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_susi = 0;
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	AVD_SG *sg = su->sg_of_su;
	AVD_SI *si = sg->admin_si;
	NCS_BOOL is_su_present, flag;
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_AVND *su_node_ptr = NULL;

	/* check if su is present in the su-oper list */
	m_AVD_CHK_OPLIST(su, is_su_present);

	if (!si) {
		/* => no si operation in progress */

		if (TRUE == is_su_present) {
			/* => su is present in the su-oper list */
			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

			if ((SA_AMF_ADMIN_SHUTTING_DOWN == su->saAmfSUAdminState) ||
			    (SA_AMF_ADMIN_SHUTTING_DOWN == su_node_ptr->saAmfNodeAdminState)) {
				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
					avd_su_readiness_state_set(su, SA_AMF_READINESS_OUT_OF_SERVICE);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
					if (flag == TRUE) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}

				/* identify all the quiescing assignments & send quiesced assignment */
				for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
					if (SA_AMF_HA_QUIESCING != curr_susi->state)
						continue;

					/* send quiesced assignment */
					old_state = curr_susi->state;
					old_fsm_state = curr_susi->fsm;
					curr_susi->state = SA_AMF_HA_QUIESCED;
					curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
										 curr_susi->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
										 curr_susi->si->name.length);
						curr_susi->state = old_state;
						curr_susi->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
						return rc;
					}
				}	/* for */
			} else {
				/* process as su-oos evt is processed in stable state */
				rc = avd_sg_nway_su_fault_stable(cb, su);

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			}
		} else {
			/* => su is not present in the su-oper list */

			/* process as su-oos evt is processed in stable state */
			rc = avd_sg_nway_su_fault_stable(cb, su);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		}
	} else {
		/* => si operation in progress */

		/* determine if this si has a rel with the su */
		for (curr_susi = si->list_of_sisu; curr_susi && (curr_susi->su != su); curr_susi = curr_susi->si_next) ;

		if (curr_susi) {
			/* => relationship exists */

			if (si->si_switch == AVSV_SI_TOGGLE_SWITCH) {
				if (SA_AMF_HA_QUIESCED == curr_susi->state) {
					/* si switch operation aborted */
					m_AVD_SET_SI_SWITCH(cb, si, AVSV_SI_TOGGLE_STABLE);
					m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

					/* identify all the active assigned assignments & send quiesced assignment */
					for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
						if (!((SA_AMF_HA_ACTIVE == curr_susi->state) &&
						      (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)))
							continue;

						/* send quiesced assignment */
						old_state = curr_susi->state;
						old_fsm_state = curr_susi->fsm;
						curr_susi->state = SA_AMF_HA_QUIESCED;
						curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
						rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
						if (NCSCC_RC_SUCCESS != rc) {
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
											 curr_susi->su->name.length);
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
											 curr_susi->si->name.length);
							curr_susi->state = old_state;
							curr_susi->fsm = old_fsm_state;
							m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi,
											 AVSV_CKPT_AVD_SI_ASS);
							avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
							return rc;
						}
					}	/* for */

					/* add su to the su-oper list */
					avd_sg_su_oper_list_add(cb, su, FALSE);
				} else if ((SA_AMF_HA_ACTIVE == curr_susi->state) &&
					   (AVD_SU_SI_STATE_ASGN == curr_susi->fsm)) {
					/* identify all the active assignments & send quiesced assignment */
					for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
						if (SA_AMF_HA_ACTIVE != curr_susi->state)
							continue;

						/* send quiesced assignment */
						old_state = curr_susi->state;
						old_fsm_state = curr_susi->fsm;
						curr_susi->state = SA_AMF_HA_QUIESCED;
						curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
						rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
						if (NCSCC_RC_SUCCESS != rc) {
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
											 curr_susi->su->name.length);
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
											 curr_susi->si->name.length);
							curr_susi->state = old_state;
							curr_susi->fsm = old_fsm_state;
							m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi,
											 AVSV_CKPT_AVD_SI_ASS);
							avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
							return rc;
						}
					}	/* for */

					/* add su to the su-oper list */
					avd_sg_su_oper_list_add(cb, su, FALSE);
				} else if ((SA_AMF_HA_STANDBY == curr_susi->state) &&
					   (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)) {
					/* TBD */
				}
			} else {
				if ((SA_AMF_HA_QUIESCING == curr_susi->state) ||
				    ((SA_AMF_HA_QUIESCED == curr_susi->state) &&
				     (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm))) {
					/* identify all the active assigned assignments & send quiesced assignment */
					for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
						if (!((SA_AMF_HA_ACTIVE == curr_susi->state) &&
						      (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)))
							continue;

						/* send quiesced assignment */
						old_state = curr_susi->state;
						old_fsm_state = curr_susi->fsm;
						curr_susi->state = SA_AMF_HA_QUIESCED;
						curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
						rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
						if (NCSCC_RC_SUCCESS != rc) {
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
											 curr_susi->su->name.length);
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
											 curr_susi->si->name.length);
							curr_susi->state = old_state;
							curr_susi->fsm = old_fsm_state;
							m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi,
											 AVSV_CKPT_AVD_SI_ASS);
							avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
							return rc;
						}
					}	/* for */

					/* add su to the su-oper list */
					avd_sg_su_oper_list_add(cb, su, FALSE);
				} else if ((SA_AMF_HA_STANDBY == curr_susi->state) &&
					   (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)) {
					/* process as su-oos evt is processed in stable state */
					rc = avd_sg_nway_su_fault_stable(cb, su);

					/* transition to sg-realign state */
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
				}
			}
		} else {
			/* => relationship doesnt exist */

			/* process as su-oos evt is processed in stable state */
			rc = avd_sg_nway_su_fault_stable(cb, su);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		}
	}

	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_su_fault_su_oper
 *
 * Purpose  : This routine handles the su-fault event in the su-oper state.
 *
 * Input    : cb - the AVD control block
 *            su - ptr to SU
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uns32 avd_sg_nway_su_fault_su_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_susi = 0;
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	NCS_BOOL is_all_stdby = TRUE, flag;
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_AVND *su_node_ptr = NULL;

	if (su->sg_of_su->su_oper_list.su == su) {

		m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

		/* => su-lock/shutdown is followed by su-disable event */
		if ((SA_AMF_ADMIN_SHUTTING_DOWN == su->saAmfSUAdminState) ||
		    (SA_AMF_ADMIN_SHUTTING_DOWN == su_node_ptr->saAmfNodeAdminState)) {
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
				if (flag == TRUE) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}

			/* identify all the quiescing assignments & send quiesced assignment */
			for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
				if (SA_AMF_HA_QUIESCING != curr_susi->state)
					continue;

				/* send quiesced assignment */
				old_state = curr_susi->state;
				old_fsm_state = curr_susi->fsm;
				curr_susi->state = SA_AMF_HA_QUIESCED;
				curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
									 curr_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
									 curr_susi->si->name.length);
					curr_susi->state = old_state;
					curr_susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					return rc;
				}
			}	/* for */
		}
	} else {
		/* => multiple sus undergoing sg semantics */
		/* identify all the active assignments & send quiesced assignment */
		for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
			if (SA_AMF_HA_ACTIVE != curr_susi->state)
				continue;

			/* send quiesced assignment */
			old_state = curr_susi->state;
			old_fsm_state = curr_susi->fsm;
			curr_susi->state = SA_AMF_HA_QUIESCED;
			curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
			avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
			rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
				curr_susi->state = old_state;
				curr_susi->fsm = old_fsm_state;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				return rc;
			}
			is_all_stdby = FALSE;
		}		/* for */

		/* if all standby assignments.. remove them */
		if (TRUE == is_all_stdby) {
			rc = avd_sg_su_si_del_snd(cb, su);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
				return rc;
			}
		}

		/* transition to sg-realign state */
		m_AVD_SET_SG_FSM(cb, su->sg_of_su, AVD_SG_FSM_SG_REALIGN);

		/* add su to the su-oper list */
		avd_sg_su_oper_list_add(cb, su, FALSE);
	}

	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_su_fault_si_oper
 *
 * Purpose  : This routine handles the su-fault event in the si-oper state.
 *
 * Input    : cb - the AVD control block
 *            su - ptr to SU
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uns32 avd_sg_nway_su_fault_si_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_susi = 0;
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	AVD_SG *sg = su->sg_of_su;
	AVD_SI *si = sg->admin_si;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* process the failure of su while there is an si transfer 
	 * going in this SG */

	if (su->sg_of_su->si_tobe_redistributed) {
		if (su->sg_of_su->min_assigned_su == su) {
			/* determine if this si has a rel with the su */
			for (curr_susi = si->list_of_sisu; curr_susi && (curr_susi->su != su);
					curr_susi = curr_susi->si_next);
			if (curr_susi) {
				if ((curr_susi->state == SA_AMF_HA_ACTIVE) &&
						(curr_susi->fsm == AVD_SU_SI_STATE_MODIFY)) {
					/* send quiesced assignment */
					old_state = curr_susi->state;
					old_fsm_state = curr_susi->fsm;
					curr_susi->state = SA_AMF_HA_QUIESCED;
					curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
								curr_susi->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
								curr_susi->si->name.length);
						curr_susi->state = old_state;
						curr_susi->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
						return rc;
					}

					/* add su to the su-oper list */
					avd_sg_su_oper_list_add(cb, su, FALSE);
				}

				/* delete the quiesced assignment to max assigned su */
				curr_susi = avd_su_susi_find(cb, su->sg_of_su->max_assigned_su,
						&su->sg_of_su->si_tobe_redistributed->name);
				assert(curr_susi->state == SA_AMF_HA_QUIESCED);

				old_fsm_state = curr_susi->fsm;
				curr_susi->fsm = AVD_SU_SI_STATE_UNASGN;
				if (avd_snd_susi_msg(avd_cb, curr_susi->su, curr_susi,
							AVSV_SUSI_ACT_DEL, false, NULL) == NCSCC_RC_FAILURE) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__,
							curr_susi->su->name.value, curr_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__,
							curr_susi->si->name.value, curr_susi->si->name.length);
					curr_susi->fsm = old_fsm_state;
				} else {
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					/* add the su to su-oper list and move the SG to Realign */
					avd_sg_su_oper_list_add(cb, curr_susi->su, FALSE);
				}
				m_AVD_SET_SG_FSM(cb, curr_susi->su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
			}

		} else if (su->sg_of_su->max_assigned_su == su) {
			/* determine if this si has a rel with the max su */
			for (curr_susi = si->list_of_sisu; curr_susi && (curr_susi->su != su);
					curr_susi = curr_susi->si_next);
			if (curr_susi) {
				if ((curr_susi->state == SA_AMF_HA_QUIESCED) &&
						(curr_susi->fsm == AVD_SU_SI_STATE_MODIFY)) {

					/* add su to the su-oper list & transition to su-oper state */
					avd_sg_su_oper_list_add(cb, su, FALSE);
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SU_OPER);
				}
			}

		} else {
			/* any other su has faulted then add to su oper and move to oper state */
			avd_sg_su_oper_list_add(cb, su, FALSE);
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SU_OPER);
		}
		/* reset all pointers for SI transfer */
		su->sg_of_su->si_tobe_redistributed = NULL;
		su->sg_of_su->max_assigned_su = NULL;
		su->sg_of_su->min_assigned_su = NULL;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
		TRACE("SI transfer aborted '%s'", su->name.value);
		goto process_remaining;
	}

	if (!si)
		return NCSCC_RC_FAILURE;

	if (si->si_switch == AVSV_SI_TOGGLE_SWITCH) {
		/* => si-switch operation semantics in progress */

		/* determine if this si has a rel with the su */
		for (curr_susi = si->list_of_sisu; curr_susi && (curr_susi->su != su); curr_susi = curr_susi->si_next) ;

		if (curr_susi) {
			/* => relationship exists */

			if ((SA_AMF_HA_QUIESCED == curr_susi->state) && (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm)) {
				/* si switch operation aborted */
				m_AVD_SET_SI_SWITCH(cb, si, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

				/* transition to su-oper state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SU_OPER);

				/* add su to the su-oper list */
				avd_sg_su_oper_list_add(cb, su, FALSE);
			} else if ((SA_AMF_HA_ACTIVE == curr_susi->state) && (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm)) {
				/* send quiesced assignment */
				old_state = curr_susi->state;
				old_fsm_state = curr_susi->fsm;
				curr_susi->state = SA_AMF_HA_QUIESCED;
				curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
									 curr_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
									 curr_susi->si->name.length);
					curr_susi->state = old_state;
					curr_susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					return rc;
				}

				/* add su to the su-oper list */
				avd_sg_su_oper_list_add(cb, su, FALSE);
			} else if ((SA_AMF_HA_QUIESCED == curr_susi->state) &&
				   (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)) {
				/* si switch operation aborted */
				m_AVD_SET_SI_SWITCH(cb, si, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

				/* add su to the su-oper list */
				avd_sg_su_oper_list_add(cb, su, FALSE);
			} else if ((SA_AMF_HA_STANDBY == curr_susi->state) && (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)) {
				AVD_SU_SI_REL *susi = 0;

				/* si switch operation aborted */
				m_AVD_SET_SI_SWITCH(cb, si, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

				/* identify the quiesced assigning susi */
				for (susi = si->list_of_sisu;
				     susi && !((SA_AMF_READINESS_IN_SERVICE == susi->su->saAmfSuReadinessState) &&
					       (SA_AMF_HA_QUIESCED == susi->state) &&
					       (AVD_SU_SI_STATE_MODIFY == susi->fsm)); susi = susi->si_next) ;

				/* send active assignment */
				if (susi) {
					old_state = susi->state;
					old_fsm_state = susi->fsm;
					susi->state = SA_AMF_HA_ACTIVE;
					susi->fsm = AVD_SU_SI_STATE_MODIFY;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, susi);
					rc = avd_snd_susi_msg(cb, susi->su, susi, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value,
										 susi->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value,
										 susi->si->name.length);
						susi->state = old_state;
						susi->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, susi);
						return rc;
					}

					/* transition to sg-realign state */
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

					/* add su to the su-oper list */
					avd_sg_su_oper_list_add(cb, susi->su, FALSE);
				}
			}
		} else {
			/* => relationship doesnt exist */

			/* si switch operation aborted */
			m_AVD_SET_SI_SWITCH(cb, si, AVSV_SI_TOGGLE_STABLE);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

			/* identify the quiesced assigning susi */
			for (curr_susi = si->list_of_sisu;
			     curr_susi && !((SA_AMF_READINESS_IN_SERVICE == curr_susi->su->saAmfSuReadinessState) &&
					    (SA_AMF_HA_QUIESCED == curr_susi->state) &&
					    (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm));
			     curr_susi = curr_susi->si_next) ;

			/* send active assignment */
			if (curr_susi) {
				old_state = curr_susi->state;
				old_fsm_state = curr_susi->fsm;
				curr_susi->state = SA_AMF_HA_ACTIVE;
				curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
									 curr_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
									 curr_susi->si->name.length);
					curr_susi->state = old_state;
					curr_susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					return rc;
				}

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

				/* add su to the su-oper list */
				avd_sg_su_oper_list_add(cb, curr_susi->su, FALSE);
			}
		}
	} else {
		/* => non si-switch operation semantics in progress */

		/* determine if this si has a rel with the su */
		for (curr_susi = si->list_of_sisu; curr_susi && (curr_susi->su != su); curr_susi = curr_susi->si_next) ;

		if (curr_susi) {
			/* => relationship exists */

			if ((SA_AMF_HA_QUIESCING == curr_susi->state) ||
			    ((SA_AMF_HA_QUIESCED == curr_susi->state) && (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm))) {
				/* si operation aborted */
				avd_si_admin_state_set(si, SA_AMF_ADMIN_UNLOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

				/* if quiescing, send quiesced assignment */
				if (SA_AMF_HA_QUIESCING == curr_susi->state) {
					old_state = curr_susi->state;
					old_fsm_state = curr_susi->fsm;
					curr_susi->state = SA_AMF_HA_QUIESCED;
					curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
										 curr_susi->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
										 curr_susi->si->name.length);
						curr_susi->state = old_state;
						curr_susi->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
						return rc;
					}
				}

				/* transition to su-oper state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SU_OPER);

				/* add su to the su-oper list */
				avd_sg_su_oper_list_add(cb, su, FALSE);
			} else if ((SA_AMF_HA_STANDBY == curr_susi->state) && (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)) {
				AVD_SU_SI_REL *susi = 0;

				/* si operation aborted */
				avd_si_admin_state_set(si, SA_AMF_ADMIN_UNLOCKED);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

				/* identify the quiesced assigning susi */
				for (susi = si->list_of_sisu;
				     susi && !((SA_AMF_READINESS_IN_SERVICE == curr_susi->su->saAmfSuReadinessState) &&
					       (SA_AMF_HA_QUIESCED == susi->state) &&
					       (AVD_SU_SI_STATE_MODIFY == susi->fsm)); susi = susi->si_next) ;

				/* send active assignment */
				if (susi) {
					old_state = susi->state;
					old_fsm_state = susi->fsm;
					susi->state = SA_AMF_HA_ACTIVE;
					susi->fsm = AVD_SU_SI_STATE_MODIFY;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, susi);
					rc = avd_snd_susi_msg(cb, susi->su, susi, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value,
										 susi->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value,
										 susi->si->name.length);
						susi->state = old_state;
						susi->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, susi);
						return rc;
					}

					/* transition to sg-realign state */
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

					/* add su to the su-oper list */
					avd_sg_su_oper_list_add(cb, susi->su, FALSE);
				}
			}
		} else {
			/* => relationship doesnt exist */
			AVD_SU_SI_REL *susi = 0;

			/* si operation aborted */
			avd_si_admin_state_set(si, SA_AMF_ADMIN_UNLOCKED);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

			/* identify the quiesced assigning susi */
			for (susi = si->list_of_sisu;
			     susi && !((SA_AMF_READINESS_IN_SERVICE == curr_susi->su->saAmfSuReadinessState) &&
				       (SA_AMF_HA_QUIESCED == susi->state) &&
				       (AVD_SU_SI_STATE_MODIFY == susi->fsm)); susi = susi->si_next) ;

			/* send active assignment */
			if (susi) {
				old_state = susi->state;
				old_fsm_state = susi->fsm;
				susi->state = SA_AMF_HA_ACTIVE;
				susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, susi);
				rc = avd_snd_susi_msg(cb, susi->su, susi, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
					susi->state = old_state;
					susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, susi);
					return rc;
				}

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

				/* add su to the su-oper list */
				avd_sg_su_oper_list_add(cb, susi->su, FALSE);
			}
		}
	}

process_remaining:

	/* process the remaining sis as is done in stable state */
	for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
		if ((curr_susi->si == si) || (SA_AMF_HA_ACTIVE != curr_susi->state))
			continue;

		/* send quiesced assignment */
		old_state = curr_susi->state;
		old_fsm_state = curr_susi->fsm;
		curr_susi->state = SA_AMF_HA_QUIESCED;
		curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
		avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
		rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
		if (NCSCC_RC_SUCCESS != rc) {
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
			curr_susi->state = old_state;
			curr_susi->fsm = old_fsm_state;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
			avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
			return rc;
		}
	}			/* for */

	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_su_fault_sg_admin
 *
 * Purpose  : This routine handles the su-fault event in the sg-admin state.
 *
 * Input    : cb - the AVD control block
 *            su - ptr to SU
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uns32 avd_sg_nway_su_fault_sg_admin(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_susi = 0;
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* if already locked, do nothing */
	if (SA_AMF_ADMIN_LOCKED == su->sg_of_su->saAmfSGAdminState)
		return rc;

	if (SA_AMF_ADMIN_SHUTTING_DOWN == su->sg_of_su->saAmfSGAdminState) {
		/* identify all the quiescing assignments & send quiesced assignment */
		for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
			if (SA_AMF_HA_QUIESCING != curr_susi->state)
				continue;

			/* send quiesced assignment */
			old_state = curr_susi->state;
			old_fsm_state = curr_susi->fsm;
			curr_susi->state = SA_AMF_HA_QUIESCED;
			curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
			avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
			rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
				curr_susi->state = old_state;
				curr_susi->fsm = old_fsm_state;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				return rc;
			}
		}		/* for */
	}

	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_susi_succ_sg_realign
 *
 * Purpose  : This routine handles the susi-success event in the sg-realign 
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *            susi  - ptr to susi rel
 *            act   - susi operation
 *            state - ha state
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uns32 avd_sg_nway_susi_succ_sg_realign(AVD_CL_CB *cb,
				       AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *curr_susi = 0, *curr_sisu = 0, tmp_susi;
	AVD_SG *sg = su->sg_of_su;
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	NCS_BOOL is_su_present, is_eng, flag;
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_AVND *su_node_ptr = NULL;

	if (susi && (SA_AMF_HA_ACTIVE == state) && (AVSV_SUSI_ACT_DEL != act)) {
		/* => single active assignment success */

		if (susi->si == sg->admin_si) {
			/* si switch semantics in progress */

			/* identify the quiesced assigned assignment */
			for (curr_susi = susi->si->list_of_sisu;
			     curr_susi && !((SA_AMF_HA_QUIESCED == curr_susi->state) &&
					    (AVD_SU_SI_STATE_ASGND == curr_susi->fsm));
			     curr_susi = curr_susi->si_next) ;

			if (curr_susi) {
				/* send standby assignment */
				old_state = curr_susi->state;
				old_fsm_state = curr_susi->fsm;
				curr_susi->state = SA_AMF_HA_STANDBY;
				curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
									 curr_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
									 curr_susi->si->name.length);
					curr_susi->state = old_state;
					curr_susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					return rc;
				}

				/* reset the switch operation */
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
				m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);

				/* add the su to su-oper list */
				avd_sg_su_oper_list_add(cb, curr_susi->su, FALSE);
			}
		} else {
			/* check if su is present in the su-oper list */
			m_AVD_CHK_OPLIST(su, is_su_present);
			if (is_su_present) {
				/* if other susis of this SU are assigned, remove it from the su-oper list */
				for (curr_susi = su->list_of_susi;
				     curr_susi && (AVD_SU_SI_STATE_ASGND == curr_susi->fsm);
				     curr_susi = curr_susi->su_next) ;

				if (!curr_susi)
					avd_sg_su_oper_list_del(cb, su, FALSE);
			} else {
				/* identify the quiesced susi assignment */
				for (curr_susi = susi->si->list_of_sisu;
				     curr_susi && (SA_AMF_HA_QUIESCED != curr_susi->state);
				     curr_susi = curr_susi->si_next) ;

				if (curr_susi) {
					/* the corresponding su should be in the su-oper list */
					m_AVD_CHK_OPLIST(curr_susi->su, is_su_present);
					if (is_su_present) {
						/* determine if all the standby sus are engaged */
						m_AVD_SG_NWAY_ARE_STDBY_SUS_ENGAGED(curr_susi->su, 0, is_eng);
						if (TRUE == is_eng) {
							/* send remove all msg for all sis for this su */
							rc = avd_sg_su_si_del_snd(cb, curr_susi->su);
							if (NCSCC_RC_SUCCESS != rc) {
								LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.
												 value,
												 curr_susi->su->name.
												 length);
								return rc;
							}

							m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

							/* if su-shutdown semantics in progress, mark it locked */
							if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
								avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
							} else if (su_node_ptr->saAmfNodeAdminState ==
								   SA_AMF_ADMIN_SHUTTING_DOWN) {
								m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
								if (flag == TRUE) {
									node_admin_state_set(su_node_ptr,
											     SA_AMF_ADMIN_LOCKED);
								}
							}
						}
					}
				}
			}
		}

		/* if we are done with the upheaval, try assigning new sis to the sg */
		if (!sg->admin_si && !sg->su_oper_list.su)
			rc = avd_sg_nway_si_assign(cb, sg);

	} else if (susi && (SA_AMF_HA_STANDBY == state) && (AVSV_SUSI_ACT_DEL != act)) {
		/* => single standby assignment success */

		/* if all susis are assigned, remove the su from the su-oper list */
		for (curr_susi = su->list_of_susi;
		     curr_susi && (AVD_SU_SI_STATE_ASGND == curr_susi->fsm); curr_susi = curr_susi->su_next) ;

		if (!curr_susi)
			avd_sg_su_oper_list_del(cb, su, FALSE);

		/* if we are done with the upheaval, try assigning new sis to the sg */
		if (!sg->admin_si && !sg->su_oper_list.su)
			rc = avd_sg_nway_si_assign(cb, sg);

	} else if (susi && (SA_AMF_HA_QUIESCED == state) && (AVSV_SUSI_ACT_DEL != act)) {
		/* => single quiesced assignment success */

		if (susi->si == sg->admin_si) {
			/* si switch semantics in progress */

			/* identify the other quiesced assignment */
			for (curr_susi = susi->si->list_of_sisu;
			     curr_susi && !((SA_AMF_HA_QUIESCED == curr_susi->state) &&
					    (AVD_SU_SI_STATE_ASGND == curr_susi->fsm) &&
					    (curr_susi != susi)); curr_susi = curr_susi->si_next) ;

			if (curr_susi) {
				/* send active assignment to curr_susi */
				old_state = curr_susi->state;
				old_fsm_state = curr_susi->fsm;
				curr_susi->state = SA_AMF_HA_ACTIVE;
				curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
									 curr_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
									 curr_susi->si->name.length);
					curr_susi->state = old_state;
					curr_susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					return rc;
				}

				/* also send remove susi msg to the susi for which the response is rcvd */
				old_fsm_state = susi->fsm;
				susi->fsm = AVD_SU_SI_STATE_UNASGN;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				rc = avd_snd_susi_msg(cb, susi->su, susi, AVSV_SUSI_ACT_DEL, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
					susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
					return rc;
				}

				/* reset the switch operation */
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
				m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);

				/* add both sus to su-oper list */
				avd_sg_su_oper_list_add(cb, curr_susi->su, FALSE);
				avd_sg_su_oper_list_add(cb, susi->su, FALSE);
			}
		} else {
			/* check if su is present in the su-oper list */
			m_AVD_CHK_OPLIST(su, is_su_present);
			if (is_su_present) {
				/* identify the most preferred standby su for this si */
				for (curr_susi = susi->si->list_of_sisu;
				     curr_susi && !((SA_AMF_HA_STANDBY == curr_susi->state) &&
						    (SA_AMF_READINESS_IN_SERVICE ==
						     curr_susi->su->saAmfSuReadinessState));
				     curr_susi = curr_susi->si_next) ;

				if (curr_susi) {
					/* send active assignment */
					old_state = curr_susi->state;
					old_fsm_state = curr_susi->fsm;
					curr_susi->state = SA_AMF_HA_ACTIVE;
					curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
										 curr_susi->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
										 curr_susi->si->name.length);
						curr_susi->state = old_state;
						curr_susi->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
						return rc;
					}
				} else {
					/* determine if all the standby sus are engaged */
					m_AVD_SG_NWAY_ARE_STDBY_SUS_ENGAGED(su, 0, is_eng);
					if (TRUE == is_eng) {
						/* send remove all msg for all sis for this su */
						rc = avd_sg_su_si_del_snd(cb, su);
						if (NCSCC_RC_SUCCESS != rc) {
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value,
											 su->name.length);
							return rc;
						}

						m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

						/* if su-shutdown semantics in progress, mark it locked */
						if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
							avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
						} else if (su_node_ptr->saAmfNodeAdminState ==
							   SA_AMF_ADMIN_SHUTTING_DOWN) {
							m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
							if (flag == TRUE) {
								node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
							}
						}

					}
				}
			}
		}

	} else if (susi && (AVSV_SUSI_ACT_DEL == act)) {
		/* => single remove success */

		/* store the susi info before freeing */
		tmp_susi = *susi;

		/* free all the CSI assignments  */
		avd_compcsi_delete(cb, susi, FALSE);
		/* free susi assignment */
		m_AVD_SU_SI_TRG_DEL(cb, susi);

		if (sg->admin_si == tmp_susi.si) {
			if (SA_AMF_HA_QUIESCED == tmp_susi.state) {
				/* identify the other quiesced assigned in service susi */
				for (curr_susi = tmp_susi.si->list_of_sisu;
				     curr_susi && !((SA_AMF_HA_QUIESCED == curr_susi->state) &&
						    (AVD_SU_SI_STATE_ASGND == curr_susi->fsm) &&
						    (SA_AMF_READINESS_IN_SERVICE ==
						     curr_susi->su->saAmfSuReadinessState));
				     curr_susi = curr_susi->si_next) ;

				if (curr_susi) {
					/* send active assignment to curr_susi */
					old_state = curr_susi->state;
					old_fsm_state = curr_susi->fsm;
					curr_susi->state = SA_AMF_HA_ACTIVE;
					curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
										 curr_susi->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
										 curr_susi->si->name.length);
						curr_susi->state = old_state;
						curr_susi->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
						return rc;
					}

					/* reset the switch operation */
					m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
					m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);

					/* add the su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_susi->su, FALSE);
				}
			}
		}

		/* identify if there's any active for this si */
		for (curr_susi = tmp_susi.si->list_of_sisu;
		     curr_susi && (SA_AMF_HA_ACTIVE != curr_susi->state); curr_susi = curr_susi->si_next) ;

		if (!curr_susi && (SA_AMF_ADMIN_UNLOCKED == tmp_susi.si->saAmfSIAdminState)
                               && ((tmp_susi.si->si_dep_state == AVD_SI_ASSIGNED) ||
                                (tmp_susi.si->si_dep_state == AVD_SI_NO_DEPENDENCY))) {
			/* no active present.. identify the highest ranked in-svc standby & 
			   send active assignment to it */

			for (curr_susi = tmp_susi.si->list_of_sisu;
			     curr_susi && !((SA_AMF_HA_STANDBY == curr_susi->state) &&
					    (SA_AMF_READINESS_IN_SERVICE == curr_susi->su->saAmfSuReadinessState));
			     curr_susi = curr_susi->si_next) ;

			if (curr_susi) {
				/* send active assignment to curr_susi */
				old_state = curr_susi->state;
				old_fsm_state = curr_susi->fsm;
				curr_susi->state = SA_AMF_HA_ACTIVE;
				curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
									 curr_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
									 curr_susi->si->name.length);
					curr_susi->state = old_state;
					curr_susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					return rc;
				}
			}
		}

		/* if all the other assignments are assigned for this SU and their 
		   corresponding actives are assigned, remove the su from the su-oper list */
		for (curr_susi = tmp_susi.su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
			if (AVD_SU_SI_STATE_ASGND != curr_susi->fsm)
				break;

			if (SA_AMF_HA_STANDBY == curr_susi->state) {
				for (curr_sisu = curr_susi->si->list_of_sisu;
				     curr_sisu && !((SA_AMF_HA_ACTIVE == curr_sisu->state) &&
						    (AVD_SU_SI_STATE_ASGND == curr_sisu->fsm));
				     curr_sisu = curr_sisu->si_next) ;
				if (!curr_sisu)
					break;
			}
		}

		if (!curr_susi)
			avd_sg_su_oper_list_del(cb, su, FALSE);

		/* if we are done with the upheaval, try assigning new sis to the sg */
		if (!sg->admin_si && !sg->su_oper_list.su)
			rc = avd_sg_nway_si_assign(cb, sg);

	} else if (!susi && (AVSV_SUSI_ACT_DEL == act)) {
		/* => remove all success */
		/* if su-shutdown semantics in progress, mark it locked */
		m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

		if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
		} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
			if (flag == TRUE) {
				node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
			}
		}

		/* delete the si assignments to this su */
		avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);

		/* remove the su from the su-oper list */
		avd_sg_su_oper_list_del(cb, su, FALSE);

		/* if we are done with the upheaval, try assigning new sis to the sg */
		if (!sg->admin_si && !sg->su_oper_list.su)
			rc = avd_sg_nway_si_assign(cb, sg);
	}

	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_susi_succ_su_oper
 *
 * Purpose  : This routine handles the susi-success event in the su-oper
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *            susi  - ptr to susi rel
 *            act   - susi operation
 *            state - ha state
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uns32 avd_sg_nway_susi_succ_su_oper(AVD_CL_CB *cb,
				    AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *curr_susi = 0, *curr_sisu = 0;
	AVD_SG *sg = su->sg_of_su;
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	NCS_BOOL is_eng = FALSE, flag;
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_AVND *su_node_ptr = NULL;

	if (susi && (SA_AMF_HA_QUIESCED == state) && (AVSV_SUSI_ACT_DEL != act)) {
		/* => single quiesced assignment success */

		/* identify the most preferred standby su for this si */
		for (curr_susi = susi->si->list_of_sisu;
		     curr_susi && !((SA_AMF_HA_STANDBY == curr_susi->state) &&
				    (SA_AMF_READINESS_IN_SERVICE == curr_susi->su->saAmfSuReadinessState));
		     curr_susi = curr_susi->si_next) ;

		if (curr_susi) {
			/* send active assignment */
			old_state = curr_susi->state;
			old_fsm_state = curr_susi->fsm;
			curr_susi->state = SA_AMF_HA_ACTIVE;
			curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
			avd_susi_update(curr_susi, state);
			avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
			rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
				curr_susi->state = old_state;
				curr_susi->fsm = old_fsm_state;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				return rc;
			}
		} else {
			/* determine if all the standby sus are engaged */
			m_AVD_SG_NWAY_ARE_STDBY_SUS_ENGAGED(su, 0, is_eng);
			if (TRUE == is_eng) {
				/* send remove all msg for all sis for this su */
				rc = avd_sg_su_si_del_snd(cb, su);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return rc;
				}

				m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

				/* if su-shutdown semantics in progress, mark it locked */
				if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
				} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
					m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
					if (flag == TRUE) {
						node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
					}
				}

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			}
		}
	} else if (susi && (SA_AMF_HA_ACTIVE == state) && (AVSV_SUSI_ACT_DEL != act)) {
		/* => single active assignment success */

		avd_susi_update(susi, state);
		/* determine if all the standby sus are engaged */
		m_AVD_SG_NWAY_ARE_STDBY_SUS_ENGAGED(sg->su_oper_list.su, 0, is_eng);
		if (TRUE == is_eng) {
			/* send remove all msg for all sis for this su */
			rc = avd_sg_su_si_del_snd(cb, sg->su_oper_list.su);
			if (NCSCC_RC_SUCCESS != rc) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, sg->su_oper_list.su->name.value,
								 sg->su_oper_list.su->name.length);
				return rc;
			}

			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

			/* if su-shutdown semantics in progress, mark it locked */
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
				if (flag == TRUE) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		}
	} else if (!susi && (AVSV_SUSI_ACT_DEL == act)) {
		/* => remove all success */

		/* delete the su from the oper list */
		avd_sg_su_oper_list_del(cb, su, FALSE);

		/* scan the su-si list & send active assignments to the standbys (if any) */
		for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
			/* skip standby assignments */
			if (SA_AMF_HA_STANDBY == curr_susi->state)
				continue;

			/* identify the most preferred standby su for this si */
			for (curr_sisu = curr_susi->si->list_of_sisu;
			     curr_sisu && !((SA_AMF_HA_STANDBY == curr_sisu->state) &&
					    (SA_AMF_READINESS_IN_SERVICE == curr_sisu->su->saAmfSuReadinessState));
			     curr_sisu = curr_sisu->si_next) ;

			if (curr_sisu) {
				/* send active assignment */
				old_state = curr_sisu->state;
				old_fsm_state = curr_sisu->fsm;
				curr_sisu->state = SA_AMF_HA_ACTIVE;
				curr_sisu->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				rc = avd_snd_susi_msg(cb, curr_sisu->su, curr_sisu, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->su->name.value,
									 curr_sisu->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->si->name.value,
									 curr_sisu->si->name.length);
					curr_sisu->state = old_state;
					curr_sisu->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					return rc;
				}

				/* add the prv standby to the su-oper list */
				avd_sg_su_oper_list_add(cb, curr_sisu->su, FALSE);
			}
		}		/* for */

		/* free all the SI assignments to this SU */
		avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);

		/* transition to sg-realign state or initiate si assignments */
		if (sg->su_oper_list.su) {
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		} else
			avd_sg_nway_si_assign(cb, sg);
	}

	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_susi_succ_si_oper
 *
 * Purpose  : This routine handles the susi-success event in the si-oper
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *            susi  - ptr to susi rel
 *            act   - susi operation
 *            state - ha state
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uns32 avd_sg_nway_susi_succ_si_oper(AVD_CL_CB *cb,
				    AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *curr_susi = 0;
	AVD_SG *sg = su->sg_of_su;
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (susi && (SA_AMF_HA_QUIESCED == state) && (AVSV_SUSI_ACT_DEL != act)) {
		if (sg->admin_si != susi->si && susi->si->sg_of_si->si_tobe_redistributed != susi->si ) {
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
			return NCSCC_RC_FAILURE;
		}

		if ((SA_AMF_ADMIN_LOCKED == susi->si->saAmfSIAdminState) ||
		    (SA_AMF_ADMIN_SHUTTING_DOWN == susi->si->saAmfSIAdminState)) {
			/* lock / shutdown semantics in progress.. 
			   remove the assignment of this si from all the sus */
			for (curr_susi = susi->si->list_of_sisu; curr_susi; curr_susi = curr_susi->si_next) {
				old_fsm_state = curr_susi->fsm;
				curr_susi->fsm = AVD_SU_SI_STATE_UNASGN;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);

				rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_DEL, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
									 curr_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
									 curr_susi->si->name.length);
					curr_susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					return rc;
				}

				/* add the su to su-oper list */
				avd_sg_su_oper_list_add(cb, curr_susi->su, FALSE);
			}	/* for */

			/* reset si-admin ptr & transition to sg-realign state */
			m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);

			/* transition to locked admin state (if shutdown) */
			if (SA_AMF_ADMIN_SHUTTING_DOWN == susi->si->saAmfSIAdminState) {
				avd_si_admin_state_set(susi->si, SA_AMF_ADMIN_LOCKED);
			}
		} else if (AVSV_SI_TOGGLE_SWITCH == susi->si->si_switch) {
			/* si switch semantics in progress.. 
			   identify the most preferred standby su & assign it active */
			for (curr_susi = susi->si->list_of_sisu;
			     curr_susi && (SA_AMF_HA_STANDBY != curr_susi->state); curr_susi = curr_susi->si_next) ;

			if (curr_susi) {
				/* send active assignment */
				old_state = curr_susi->state;
				old_fsm_state = curr_susi->fsm;
				curr_susi->state = SA_AMF_HA_ACTIVE;
				curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value,
									 curr_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value,
									 curr_susi->si->name.length);
					curr_susi->state = old_state;
					curr_susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					return rc;
				}
			} else {
				/* reset the switch operation */
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
				m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);

				/* no standby.. send the prv active (now quiesced) active */
				old_state = susi->state;
				old_fsm_state = susi->fsm;
				susi->state = SA_AMF_HA_ACTIVE;
				susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				rc = avd_snd_susi_msg(cb, susi->su, susi, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
					susi->state = old_state;
					susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					return rc;
				}

				/* add the su to su-oper list & transition to sg-realign state */
				avd_sg_su_oper_list_add(cb, susi->su, FALSE);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			}
		} else if (susi->si->sg_of_si->si_tobe_redistributed == susi->si) {
			TRACE("SI transfer '%s'", susi->si->name.value);

			assert(susi->su == susi->si->sg_of_si->max_assigned_su);

			/* check if the min SU is still in-service */
			if (susi->si->sg_of_si->min_assigned_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
				AVD_SU_SI_REL *t_susi = NULL;
				t_susi = avd_su_susi_find(avd_cb, susi->si->sg_of_si->min_assigned_su, &susi->si->name);
				if (t_susi == NULL) {
					/* initiate new active assignment for this SI */
					rc = avd_new_assgn_susi(avd_cb, susi->si->sg_of_si->min_assigned_su,
							susi->si->sg_of_si->si_tobe_redistributed, 
							SA_AMF_HA_ACTIVE, FALSE, &t_susi);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, t_susi->su->name.value,
								t_susi->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, t_susi->si->name.value,
								t_susi->si->name.length);
						susi->si->sg_of_si->max_assigned_su = NULL;
						susi->si->sg_of_si->min_assigned_su = NULL;
						susi->si->sg_of_si->si_tobe_redistributed = NULL;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, susi->si->sg_of_si, AVSV_CKPT_AVD_SI_TRANS);
					}
				} else {
					/* check whether an standby assignment is existing then 
					 * send modify for this SI from standby to active 
					 */
					/* send active assignment */
					old_state = t_susi->state;
					old_fsm_state = t_susi->fsm;
					t_susi->state = SA_AMF_HA_ACTIVE;
					t_susi->fsm = AVD_SU_SI_STATE_MODIFY;
					rc = avd_snd_susi_msg(cb, t_susi->su, t_susi, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, t_susi->su->name.value,
								t_susi->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, t_susi->si->name.value,
								t_susi->si->name.length);
						t_susi->state = old_state;
						t_susi->fsm = old_fsm_state;
						susi->si->sg_of_si->max_assigned_su = NULL;
						susi->si->sg_of_si->min_assigned_su = NULL;
						susi->si->sg_of_si->si_tobe_redistributed = NULL;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, susi->si->sg_of_si, AVSV_CKPT_AVD_SI_TRANS);
					} else {
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, t_susi, AVSV_CKPT_AVD_SI_ASS);
						avd_susi_update(t_susi, t_susi->state);
						avd_gen_su_ha_state_changed_ntf(avd_cb, t_susi);
					}
				}
			} else {
				TRACE("Min assigned SU out of service '%s'",
						susi->si->sg_of_si->min_assigned_su->name.value);

				/* reset the min, max SUs and si_tobe_redistributed of the sg to NULL*/
				susi->si->sg_of_si->max_assigned_su = NULL;
				susi->si->sg_of_si->min_assigned_su = NULL;
				susi->si->sg_of_si->si_tobe_redistributed = NULL;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, susi->si->sg_of_si, AVSV_CKPT_AVD_SI_TRANS);
				old_state = susi->state;
				old_fsm_state = susi->fsm;
				susi->state = SA_AMF_HA_ACTIVE;
				susi->fsm = AVD_SU_SI_STATE_MODIFY;
				rc = avd_snd_susi_msg(cb, susi->su, susi, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
					susi->state = old_state;
					susi->fsm = old_fsm_state;
				} else {
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(avd_cb, susi);
					/* add the su to su-oper list & transition to sg-realign state */
					avd_sg_su_oper_list_add(avd_cb, susi->su, FALSE);
					m_AVD_SET_SG_FSM(avd_cb, susi->si->sg_of_si, AVD_SG_FSM_SG_REALIGN);
				}
			}
		}
	} else if (susi && (SA_AMF_HA_ACTIVE == state) && (AVSV_SUSI_ACT_DEL != act)) {
		if ((sg->admin_si == susi->si) && (AVSV_SI_TOGGLE_SWITCH == susi->si->si_switch)) {
			/* reset the switch operation */
			m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
			m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);

			/* identify the prv active su (now quiesced) */
			for (curr_susi = susi->si->list_of_sisu;
					curr_susi && (SA_AMF_HA_QUIESCED != curr_susi->state); curr_susi = curr_susi->si_next) ;

			if (curr_susi) {
				/* send standby assignment */
				old_state = curr_susi->state;
				old_fsm_state = curr_susi->fsm;
				curr_susi->state = SA_AMF_HA_STANDBY;
				curr_susi->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
				rc = avd_snd_susi_msg(cb, curr_susi->su, curr_susi, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->su->name.value, curr_susi->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_susi->si->name.value, curr_susi->si->name.length);
					curr_susi->state = old_state;
					curr_susi->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_susi);
					return rc;
				}

				/* add the su to su-oper list & transition to sg-realign state */
				avd_sg_su_oper_list_add(cb, curr_susi->su, FALSE);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			} else {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
			}
		} else if (susi->si->sg_of_si->si_tobe_redistributed == susi->si) { 
			AVD_SU_SI_REL *t_susi = NULL;
			/* si transfer is in progress for equal distribution 
			 * now get the susi between si tobe redistributed and max SU 
			 * should be quiesced state, now send a remove for that susi
			 */
			assert(susi->su == susi->si->sg_of_si->min_assigned_su);

			/* find the susi with max assigned su which should be by now quiesced */
			t_susi = avd_su_susi_find(avd_cb, susi->si->sg_of_si->max_assigned_su, &susi->si->name);
			assert(t_susi->state == SA_AMF_HA_QUIESCED);

			old_fsm_state = t_susi->fsm;
			t_susi->fsm = AVD_SU_SI_STATE_UNASGN;
			if (avd_snd_susi_msg(avd_cb, t_susi->su, t_susi, AVSV_SUSI_ACT_DEL, false, NULL) == NCSCC_RC_FAILURE) {
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, t_susi->su->name.value, t_susi->su->name.length);
				LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, t_susi->si->name.value, t_susi->si->name.length);
				t_susi->fsm = old_fsm_state;
			} else {
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, t_susi, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(avd_cb, t_susi);
				avd_sg_su_oper_list_add(avd_cb, t_susi->su, FALSE);
				m_AVD_SET_SG_FSM(avd_cb, t_susi->su->sg_of_su, AVD_SG_FSM_SG_REALIGN);
			}
			t_susi->si->sg_of_si->max_assigned_su = NULL;
			t_susi->si->sg_of_si->min_assigned_su = NULL;
			t_susi->si->sg_of_si->si_tobe_redistributed = NULL;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, t_susi->si->sg_of_si, AVSV_CKPT_AVD_SI_TRANS);
			TRACE ("SI transfer completed '%s'", t_susi->si->name.value);
		}
	       	else {
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->su->name.value, susi->su->name.length);
			LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, susi->si->name.value, susi->si->name.length);
			return NCSCC_RC_FAILURE;
		}
	}

	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_susi_succ_sg_admin
 *
 * Purpose  : This routine handles the susi-success event in the sg-admin
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *            susi  - ptr to susi rel
 *            act   - susi operation
 *            state - ha state
 *        
 * Returns  : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes    : None.
 * 
 **************************************************************************/
uns32 avd_sg_nway_susi_succ_sg_admin(AVD_CL_CB *cb,
				     AVD_SU *su, AVD_SU_SI_REL *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state)
{
	AVD_SU_SI_REL *curr_susi = 0;
	AVD_SG *sg = su->sg_of_su;
	uns32 rc = NCSCC_RC_SUCCESS;

	if (susi && (SA_AMF_HA_QUIESCED == state) && (AVSV_SUSI_ACT_DEL != act)) {
		/* => single quiesced success */
		if ((SA_AMF_ADMIN_LOCKED == sg->saAmfSGAdminState) ||
		    (SA_AMF_ADMIN_SHUTTING_DOWN == sg->saAmfSGAdminState)) {
			/* determine if the su has any quiesced assigning assignment */
			for (curr_susi = su->list_of_susi;
			     curr_susi
			     &&
			     !(((SA_AMF_HA_QUIESCED == curr_susi->state) || (SA_AMF_HA_QUIESCING == curr_susi->state))
			       && (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm)); curr_susi = curr_susi->su_next) ;

			if (!curr_susi) {
				/* => now remove all can be sent */
				rc = avd_sg_su_si_del_snd(cb, su);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, su->name.value, su->name.length);
					return rc;
				}
			}
		}
	} else if (!susi && (AVSV_SUSI_ACT_DEL == act)) {
		/* => delete all success */
		if ((SA_AMF_ADMIN_LOCKED == sg->saAmfSGAdminState) ||
		    (SA_AMF_ADMIN_SHUTTING_DOWN == sg->saAmfSGAdminState)) {
			/* free all the SI assignments to this SU */
			avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);

			/* delete the su from the oper list */
			avd_sg_su_oper_list_del(cb, su, FALSE);

			/* if oper list is empty, transition the sg back to stable state */
			if (!sg->su_oper_list.su) {
				avd_sg_admin_state_set(sg, SA_AMF_ADMIN_LOCKED);
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_STABLE);
			}
		}
	}

	return rc;
}

/*****************************************************************************
 * Function : avd_sg_nway_node_fail_stable
 *
 * Purpose  : This routine handles the node failure event in the stable
 *            state.
 *
 * Input    : cb   - the AvD control block
 *            su   - ptr to su
 *            susi - ptr to susi rel that is to be skipped
 *        
 * Returns  : None.
 *
 * Notes    : None.
 * 
 **************************************************************************/
void avd_sg_nway_node_fail_stable(AVD_CL_CB *cb, AVD_SU *su, AVD_SU_SI_REL *susi)
{
	AVD_SU_SI_REL *curr_susi = 0, *curr_sisu = 0;
	AVD_SG *sg = su->sg_of_su;
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s', %u", su->name.value, su->sg_of_su->sg_fsm_state);

	if (!su->list_of_susi)
		return;

	/* engage the active susis with their standbys */
	for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
		if (curr_susi == susi)
			continue;

		if ((SA_AMF_HA_ACTIVE == curr_susi->state) ||
		    (SA_AMF_HA_QUIESCED == curr_susi->state) || (SA_AMF_HA_QUIESCING == curr_susi->state)) {
			for (curr_sisu = curr_susi->si->list_of_sisu;
			     curr_sisu && !((SA_AMF_HA_STANDBY == curr_sisu->state) &&
					    (SA_AMF_READINESS_IN_SERVICE == curr_sisu->su->saAmfSuReadinessState));
			     curr_sisu = curr_sisu->si_next) ;

			/* send active assignment */
			if (curr_sisu) {
				old_state = curr_sisu->state;
				old_fsm_state = curr_sisu->fsm;
				curr_sisu->state = SA_AMF_HA_ACTIVE;
				curr_sisu->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
				avd_susi_update(curr_sisu, SA_AMF_HA_ACTIVE);
				avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
				rc = avd_snd_susi_msg(cb, curr_sisu->su, curr_sisu, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->su->name.value,
									 curr_sisu->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->si->name.value,
									 curr_sisu->si->name.length);
					curr_sisu->state = old_state;
					curr_sisu->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
					avd_susi_update(curr_sisu, old_state);
					avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
					return;
				}

				/* add su to su-oper list */
				avd_sg_su_oper_list_add(cb, curr_sisu->su, FALSE);
			}
		}

		/* transition to sg-realign state */
		if (sg->su_oper_list.su) {
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		}
	}			/* for */

	/* delete the si assignments to this su */
	avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);

	/* if sg is still in stable state, initiate si assignments */
	if (AVD_SG_FSM_STABLE == sg->sg_fsm_state) {
		avd_sg_nway_si_assign(cb, sg);
		/* if equal ranked su then screen for redistribution
		 * of the SIs if the SG fsm state is still STABLE */
		if ((su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) &&
				(su->sg_of_su->equal_ranked_su == TRUE) &&
				(su->sg_of_su->saAmfSGAutoAdjust == SA_TRUE))
			avd_sg_nway_screen_si_distr_equal(su->sg_of_su);
		avd_sg_screen_si_si_dependencies(cb, sg);
	}

	TRACE_LEAVE();
	return;
}

/*****************************************************************************
 * Function : avd_sg_nway_node_fail_su_oper
 *
 * Purpose  : This routine handles the node failure event in the su-oper
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *        
 * Returns  : None.
 *
 * Notes    : None.
 * 
 **************************************************************************/
void avd_sg_nway_node_fail_su_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_susi = 0, *curr_sisu = 0;
	AVD_SG *sg = su->sg_of_su;
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	NCS_BOOL is_su_present, flag;
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_AVND *su_node_ptr = NULL;

	/* check if su is present in the su-oper list */
	m_AVD_CHK_OPLIST(su, is_su_present);

	if (is_su_present) {
		m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

		if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
		} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
			m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
			if (flag == TRUE) {
				node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
			}
		}

		for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
			if (((SA_AMF_HA_QUIESCING == curr_susi->state) ||
			     ((SA_AMF_HA_QUIESCED == curr_susi->state) &&
			      (AVD_SU_SI_STATE_MODIFY == curr_susi->fsm))) ||
			    (AVD_SU_SI_STATE_UNASGN == curr_susi->fsm)) {
				/* identify the standbys & send them active assignment */
				for (curr_sisu = curr_susi->si->list_of_sisu;
				     curr_sisu && !((SA_AMF_HA_STANDBY == curr_sisu->state) &&
						    (SA_AMF_READINESS_IN_SERVICE ==
						     curr_sisu->su->saAmfSuReadinessState));
				     curr_sisu = curr_sisu->si_next) ;

				/* send active assignment */
				if (curr_sisu) {
					old_state = curr_sisu->state;
					old_fsm_state = curr_sisu->fsm;
					curr_sisu->state = SA_AMF_HA_ACTIVE;
					curr_sisu->fsm = AVD_SU_SI_STATE_MODIFY;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
					rc = avd_snd_susi_msg(cb, curr_sisu->su, curr_sisu, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->su->name.value,
										 curr_sisu->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->si->name.value,
										 curr_sisu->si->name.length);
						curr_sisu->state = old_state;
						curr_sisu->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
						return;
					}

					/* add su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_sisu->su, FALSE);

					/* transition to sg-realign state */
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
				}
			} else if ((SA_AMF_HA_QUIESCED == curr_susi->state) &&
				   (AVD_SU_SI_STATE_ASGND == curr_susi->fsm)) {
				/* already quiesced assigned.. idenify the active assigning & 
				   add it to the su-oper list */
				for (curr_sisu = curr_susi->si->list_of_sisu;
				     curr_sisu && !((SA_AMF_HA_ACTIVE == curr_sisu->state) &&
						    (AVD_SU_SI_STATE_ASGN == curr_sisu->fsm) &&
						    (SA_AMF_READINESS_IN_SERVICE ==
						     curr_sisu->su->saAmfSuReadinessState));
				     curr_sisu = curr_sisu->si_next) ;

				/* add su to su-oper list */
				if (curr_sisu)
					avd_sg_su_oper_list_add(cb, curr_sisu->su, FALSE);

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			}
		}		/* for */

		/* remove this su from su-oper list */
		avd_sg_su_oper_list_del(cb, su, FALSE);

		/* if sg hasnt transitioned to sg-realign, initiate new si assignments */
		if (AVD_SG_FSM_SG_REALIGN != sg->sg_fsm_state)
			avd_sg_nway_si_assign(cb, sg);
	} else {
		/* engage the active susis with their standbys */
		for (curr_susi = su->list_of_susi; curr_susi; curr_susi = curr_susi->su_next) {
			if (SA_AMF_HA_ACTIVE == curr_susi->state) {
				for (curr_sisu = curr_susi->si->list_of_sisu;
				     curr_sisu && !((SA_AMF_HA_STANDBY == curr_sisu->state) &&
						    (SA_AMF_READINESS_IN_SERVICE ==
						     curr_sisu->su->saAmfSuReadinessState));
				     curr_sisu = curr_sisu->si_next) ;

				/* send active assignment */
				if (curr_sisu) {
					old_state = curr_sisu->state;
					old_fsm_state = curr_sisu->fsm;
					curr_sisu->state = SA_AMF_HA_ACTIVE;
					curr_sisu->fsm = AVD_SU_SI_STATE_MODIFY;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
					rc = avd_snd_susi_msg(cb, curr_sisu->su, curr_sisu, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->su->name.value,
										 curr_sisu->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->si->name.value,
										 curr_sisu->si->name.length);
						curr_sisu->state = old_state;
						curr_sisu->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
						return;
					}

					/* add su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_sisu->su, FALSE);

					/* transition to sg-realign state */
					m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
				}	/* if (curr_sisu) */
			}
		}		/* for */
	}

	/* delete the si assignments to this su */
	avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);

	return;
}

/*****************************************************************************
 * Function : avd_sg_nway_node_fail_si_oper
 *
 * Purpose  : This routine handles the node failure event in the si-oper
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *        
 * Returns  : None.
 *
 * Notes    : None.
 * 
 **************************************************************************/
void avd_sg_nway_node_fail_si_oper(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SU_SI_REL *curr_sisu = 0, *susi = 0;
	AVD_SG *sg = su->sg_of_su;
	SaAmfHAStateT old_state;
	AVD_SU_SI_STATE old_fsm_state;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* process the failure of su while there is an si transfer 
	 * going in this SG */

	if (su->sg_of_su->si_tobe_redistributed) {
		if (su->sg_of_su->min_assigned_su == su) {
			/* find whether the min_assigned su is already
			 * in the process of taking active assignment */	
			susi = avd_su_susi_find(cb, sg->min_assigned_su,
					&sg->si_tobe_redistributed->name);

			if (susi != NULL && susi->state == SA_AMF_HA_ACTIVE) {
				/* identify the quiesced assigning assignment
				 * for the si_tobe_redistributed with max_su */

				curr_sisu = avd_su_susi_find(cb, sg->max_assigned_su,
						&sg->si_tobe_redistributed->name);
				/* if already ACTIVE assignment under process then
				 * remove the quiesced assignment */
				if (curr_sisu) {
					old_state = curr_sisu->state;
					old_fsm_state = curr_sisu->fsm;
					curr_sisu->fsm = AVD_SU_SI_STATE_UNASGN;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
					rc = avd_snd_susi_msg(cb, curr_sisu->su, curr_sisu, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->su->name.value,
								curr_sisu->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->si->name.value,
								curr_sisu->si->name.length);
						curr_sisu->state = old_state;
						curr_sisu->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
						return;
					}
					/* add su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_sisu->su, FALSE);
				}
			} else {
				/* identify the quiesced assignment of si_tobe_redistributed
				 * to max su and assign back the active assignment */
				 
				curr_sisu = avd_su_susi_find(cb, sg->max_assigned_su,
						&sg->si_tobe_redistributed->name);
				if (curr_sisu) {
					old_state = curr_sisu->state;
					old_fsm_state = curr_sisu->fsm;
					curr_sisu->state = SA_AMF_HA_ACTIVE;
					curr_sisu->fsm = AVD_SU_SI_STATE_UNASGN;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
					rc = avd_snd_susi_msg(cb, curr_sisu->su, curr_sisu,
							AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__,
								curr_sisu->su->name.value,
								curr_sisu->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__,
								curr_sisu->si->name.value,
								curr_sisu->si->name.length);
						curr_sisu->state = old_state;
						curr_sisu->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
						return;
					}
					/* add su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_sisu->su, FALSE);
				}
			}

			/* process the susis assigned to this su as in stable state */
			avd_sg_nway_node_fail_stable(cb, su, 0);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		} else {
			/* process the susis assigned to this su as in stable state */
			avd_sg_nway_node_fail_stable(cb, su, 0);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		}
		/* reset the pointer for SI transfer */
		su->sg_of_su->si_tobe_redistributed = NULL;
		su->sg_of_su->max_assigned_su = NULL;
		su->sg_of_su->min_assigned_su = NULL;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(avd_cb, su->sg_of_su, AVSV_CKPT_AVD_SI_TRANS);
		TRACE("SI transfer aborted '%s'", su->name.value);
	}

	if (!sg->admin_si)
		return;

	/* check if admin-si has a relationship with this su */
	for (susi = sg->admin_si->list_of_sisu; susi && (susi->su != su); susi = susi->si_next) ;

	if ((SA_AMF_ADMIN_LOCKED == sg->admin_si->saAmfSIAdminState) ||
	    (SA_AMF_ADMIN_SHUTTING_DOWN == sg->admin_si->saAmfSIAdminState)) {
		if (!susi) {
			/* process as in stable state */
			avd_sg_nway_node_fail_stable(cb, su, 0);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		} else {
			/* abort the si lock / shutdown operation */
			avd_si_admin_state_set(susi->si, SA_AMF_ADMIN_UNLOCKED);
			m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

			if (((SA_AMF_HA_QUIESCED == susi->state) ||
			     (SA_AMF_HA_QUIESCING == susi->state)) && (AVD_SU_SI_STATE_MODIFY == susi->fsm)) {
				/* process as in stable state */
				avd_sg_nway_node_fail_stable(cb, su, 0);

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			} else if ((SA_AMF_HA_STANDBY == susi->state) && (AVD_SU_SI_STATE_ASGND == susi->fsm)) {
				/* identify in-svc quiesced/quiescing assignment & 
				   send an active assignment */
				for (curr_sisu = susi->si->list_of_sisu;
				     curr_sisu
				     && !((SA_AMF_READINESS_IN_SERVICE == curr_sisu->su->saAmfSuReadinessState)
					  && ((SA_AMF_HA_QUIESCED == curr_sisu->state)
					      || (SA_AMF_HA_QUIESCING == curr_sisu->state)));
				     curr_sisu = curr_sisu->si_next) ;

				/* send active assignment */
				if (curr_sisu) {
					old_state = curr_sisu->state;
					old_fsm_state = curr_sisu->fsm;
					curr_sisu->state = SA_AMF_HA_ACTIVE;
					curr_sisu->fsm = AVD_SU_SI_STATE_MODIFY;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
					rc = avd_snd_susi_msg(cb, curr_sisu->su, curr_sisu, AVSV_SUSI_ACT_MOD, false, NULL);
					if (NCSCC_RC_SUCCESS != rc) {
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->su->name.value,
										 curr_sisu->su->name.length);
						LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->si->name.value,
										 curr_sisu->si->name.length);
						curr_sisu->state = old_state;
						curr_sisu->fsm = old_fsm_state;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
						return;
					}

					/* add su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_sisu->su, FALSE);
				}

				/* process the rest susis as in stable state */
				avd_sg_nway_node_fail_stable(cb, su, susi);

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			}
		}
	} else if (AVSV_SI_TOGGLE_SWITCH == sg->admin_si->si_switch) {
		if (!susi) {
			/* identify the quiesced assigning assignment for the admin si ptr */
			for (curr_sisu = sg->admin_si->list_of_sisu;
			     curr_sisu && !((SA_AMF_HA_QUIESCED == curr_sisu->state) &&
					    (AVD_SU_SI_STATE_MODIFY == curr_sisu->fsm));
			     curr_sisu = curr_sisu->si_next) ;

			/* send active assignment */
			if (curr_sisu) {
				old_state = curr_sisu->state;
				old_fsm_state = curr_sisu->fsm;
				curr_sisu->state = SA_AMF_HA_ACTIVE;
				curr_sisu->fsm = AVD_SU_SI_STATE_MODIFY;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
				avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
				rc = avd_snd_susi_msg(cb, curr_sisu->su, curr_sisu, AVSV_SUSI_ACT_MOD, false, NULL);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->su->name.value,
									 curr_sisu->su->name.length);
					LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->si->name.value,
									 curr_sisu->si->name.length);
					curr_sisu->state = old_state;
					curr_sisu->fsm = old_fsm_state;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
					return;
				}

				/* add su to su-oper list */
				avd_sg_su_oper_list_add(cb, curr_sisu->su, FALSE);

				/* si switch operation aborted */
				m_AVD_SET_SI_SWITCH(cb, sg->admin_si, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);
			}

			/* process the susis assigned to this su as in stable state */
			avd_sg_nway_node_fail_stable(cb, su, 0);

			/* transition to sg-realign state */
			m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
		} else {
			if (((SA_AMF_HA_QUIESCED == susi->state) && (AVD_SU_SI_STATE_MODIFY == susi->fsm)) ||
			    ((SA_AMF_HA_ACTIVE == susi->state) && (AVD_SU_SI_STATE_ASGN == susi->fsm)) ||
			    ((SA_AMF_HA_QUIESCED == susi->state) && (AVD_SU_SI_STATE_ASGND == susi->fsm)) ||
			    ((SA_AMF_HA_STANDBY == susi->state) && (AVD_SU_SI_STATE_ASGND == susi->fsm))
			    ) {
				/* si switch operation aborted */
				m_AVD_SET_SI_SWITCH(cb, susi->si, AVSV_SI_TOGGLE_STABLE);
				m_AVD_CLEAR_SG_ADMIN_SI(cb, sg);

				/* identify the susi that has to be assigned active */
				for (curr_sisu = susi->si->list_of_sisu; curr_sisu; curr_sisu = curr_sisu->si_next) {
					if ((SA_AMF_HA_QUIESCED == susi->state)
					    && (AVD_SU_SI_STATE_MODIFY == susi->fsm)) {
						if ((curr_sisu != susi)
						    && ((SA_AMF_HA_QUIESCED == curr_sisu->state)
							|| (SA_AMF_HA_STANDBY == curr_sisu->state)))
							break;
					}

					if ((SA_AMF_HA_ACTIVE == susi->state) && (AVD_SU_SI_STATE_ASGN == susi->fsm))
						if (SA_AMF_HA_QUIESCED == curr_sisu->state)
							break;

					if ((SA_AMF_HA_QUIESCED == susi->state) && (AVD_SU_SI_STATE_ASGND == susi->fsm))
						if (SA_AMF_HA_ACTIVE == curr_sisu->state)
							break;

					if ((SA_AMF_HA_STANDBY == susi->state) && (AVD_SU_SI_STATE_ASGND == susi->fsm))
						if (SA_AMF_HA_QUIESCED == curr_sisu->state)
							break;
				}	/* for */

				/* send active assignment */
				if (curr_sisu) {
					if (!((SA_AMF_HA_QUIESCED == susi->state)
					      && (AVD_SU_SI_STATE_ASGND == susi->fsm))) {
						old_state = curr_sisu->state;
						old_fsm_state = curr_sisu->fsm;
						curr_sisu->state = SA_AMF_HA_ACTIVE;
						curr_sisu->fsm = AVD_SU_SI_STATE_MODIFY;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
						rc = avd_snd_susi_msg(cb, curr_sisu->su, curr_sisu, AVSV_SUSI_ACT_MOD, false, NULL);
						if (NCSCC_RC_SUCCESS != rc) {
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->su->name.value,
											 curr_sisu->su->name.length);
							LOG_ER("%s:%u: %s (%u)", __FILE__, __LINE__, curr_sisu->si->name.value,
											 curr_sisu->si->name.length);
							curr_sisu->state = old_state;
							curr_sisu->fsm = old_fsm_state;
							m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_sisu,
											 AVSV_CKPT_AVD_SI_ASS);
							avd_gen_su_ha_state_changed_ntf(cb, curr_sisu);
							return;
						}
					}

					/* add su to su-oper list */
					avd_sg_su_oper_list_add(cb, curr_sisu->su, FALSE);
				}

				/* process the rest susis as in stable state */
				avd_sg_nway_node_fail_stable(cb, su, susi);

				/* transition to sg-realign state */
				m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_SG_REALIGN);
			}
		}
	}

	return;
}

/*****************************************************************************
 * Function : avd_sg_nway_node_fail_sg_admin
 *
 * Purpose  : This routine handles the node failure event in the sg-admin
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *        
 * Returns  : None.
 *
 * Notes    : None.
 * 
 **************************************************************************/
void avd_sg_nway_node_fail_sg_admin(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SG *sg = su->sg_of_su;

	/* delete the si assignments to this su */
	avd_sg_su_asgn_del_util(cb, su, TRUE, FALSE);

	/* delete the su from the su-oper list */
	avd_sg_su_oper_list_del(cb, su, FALSE);

	if (!sg->su_oper_list.su) {
		avd_sg_admin_state_set(sg, SA_AMF_ADMIN_LOCKED);
		m_AVD_SET_SG_FSM(cb, sg, AVD_SG_FSM_STABLE);
	}

	return;
}

/*****************************************************************************
 * Function : avd_sg_nway_node_fail_sg_realign
 *
 * Purpose  : This routine handles the node failure event in the sg-realign
 *            state.
 *
 * Input    : cb    - the AvD control block
 *            su    - ptr to SU
 *        
 * Returns  : None.
 *
 * Notes    : None.
 * 
 **************************************************************************/
void avd_sg_nway_node_fail_sg_realign(AVD_CL_CB *cb, AVD_SU *su)
{
	AVD_SG *sg = su->sg_of_su;
	NCS_BOOL is_su_present, flag;
	AVD_AVND *su_node_ptr = NULL;

	if (sg->admin_si) {
		/* process as in si-oper state */
		avd_sg_nway_node_fail_si_oper(cb, su);
	} else {
		/* => si operation isnt in progress */

		/* check if su is present in the su-oper list */
		m_AVD_CHK_OPLIST(su, is_su_present);

		if (TRUE == is_su_present) {
			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);
			if (su->saAmfSUAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				avd_su_admin_state_set(su, SA_AMF_ADMIN_LOCKED);
			} else if (su_node_ptr->saAmfNodeAdminState == SA_AMF_ADMIN_SHUTTING_DOWN) {
				m_AVD_IS_NODE_LOCK((su_node_ptr), flag);
				if (flag == TRUE) {
					node_admin_state_set(su_node_ptr, SA_AMF_ADMIN_LOCKED);
				}
			}
		}

		/* process as in stable state */
		avd_sg_nway_node_fail_stable(cb, su, 0);

		/* delete the su from the su-oper list */
		avd_sg_su_oper_list_del(cb, su, FALSE);

		/* if we are done with the upheaval, try assigning new sis to the sg */
		if (!sg->su_oper_list.su)
			avd_sg_nway_si_assign(cb, sg);
	}

	return;
}
