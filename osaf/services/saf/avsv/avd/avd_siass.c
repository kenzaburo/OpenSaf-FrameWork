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

  DESCRIPTION:This module deals with the creation, accessing and deletion of
  the SU SI relationship list on the AVD.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include <immutil.h>
#include <logtrace.h>
#include <saflog.h>

#include <avsv_util.h>
#include <avd_util.h>
#include <avd_susi.h>
#include <avd_imm.h>
#include <avd_csi.h>
#include <avd_proc.h>

/**
 * Create an SaAmfSIAssignment runtime object in IMM.
 * @param ha_state
 * @param si_dn
 * @param su_dn
 */
static void avd_create_susi_in_imm(SaAmfHAStateT ha_state,
       const SaNameT *si_dn, const SaNameT *su_dn)
{
       SaAisErrorT rc;
       SaNameT dn;
       SaAmfHAReadinessStateT saAmfSISUHAReadinessState = SA_AMF_HARS_READY_FOR_ASSIGNMENT;
       void *arr1[] = { &dn };
       void *arr2[] = { &ha_state };
       void *arr3[] = { &saAmfSISUHAReadinessState };
       const SaImmAttrValuesT_2 attr_safSISU = {
	       .attrName = "safSISU",
	       .attrValueType = SA_IMM_ATTR_SANAMET,
	       .attrValuesNumber = 1,
	       .attrValues = arr1
       };
       const SaImmAttrValuesT_2 attr_saAmfSISUHAState = {
	       .attrName = "saAmfSISUHAState",
	       .attrValueType = SA_IMM_ATTR_SAUINT32T,
	       .attrValuesNumber = 1,
	       .attrValues = arr2
       };
       const SaImmAttrValuesT_2 attr_saAmfSISUHAReadinessState = {
	       .attrName = "saAmfSISUHAReadinessState",
	       .attrValueType = SA_IMM_ATTR_SAUINT32T,
	       .attrValuesNumber = 1,
	       .attrValues = arr3
       };
       const SaImmAttrValuesT_2 *attrValues[] = {
	       &attr_safSISU,
	       &attr_saAmfSISUHAState,
	       &attr_saAmfSISUHAReadinessState,
	       NULL
       };

       if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE)
	       return;

       avsv_create_association_class_dn(su_dn, NULL, "safSISU", &dn);

       if ((rc = avd_saImmOiRtObjectCreate("SaAmfSIAssignment", si_dn, attrValues)) != SA_AIS_OK)
	   LOG_ER("rc=%u, '%s'", rc, dn.value);
}

/** Delete an SaAmfSIAssignment from IMM
 * 
 * @param si_dn
 * @param su_dn
 */
static void avd_delete_siassignment_from_imm(const SaNameT *si_dn, const SaNameT *su_dn)
{
       SaAisErrorT rc;
       SaNameT dn;

       if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE)
	       return;

       avsv_create_association_class_dn(su_dn, si_dn, "safSISU", &dn);

       if ((rc = avd_saImmOiRtObjectDelete(&dn)) != SA_AIS_OK)
	       LOG_ER("rc=%u, '%s'", rc, dn.value);
}

/**
 * Update an SaAmfSIAssignment runtime object in IMM.
 * @param ha_state
 * @param si_dn
 * @param su_dn
 */
void avd_susi_update(AVD_SU_SI_REL *susi, SaAmfHAStateT ha_state)
{
       SaAisErrorT rc;
       SaNameT dn;
       AVD_COMP_CSI_REL *compcsi;

       if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE)
	       return;

       avsv_create_association_class_dn(&susi->su->name, &susi->si->name, "safSISU", &dn);

       TRACE("HA State %s of %s for %s", avd_ha_state[ha_state],
	       susi->su->name.value, susi->si->name.value);
       saflog(LOG_NOTICE, amfSvcUsrName, "HA State %s of %s for %s",
	       avd_ha_state[ha_state], susi->su->name.value, susi->si->name.value);

       if ((rc = avd_saImmOiRtObjectUpdate(&dn,"saAmfSISUHAState", SA_IMM_ATTR_SAUINT32T, &ha_state)) != SA_AIS_OK)
	       LOG_ER("rc=%u, '%s'", rc, dn.value);

       /* Update all CSI assignments */
       for (compcsi = susi->list_of_csicomp; compcsi != NULL; compcsi = compcsi->susi_csicomp_next) {
	       avsv_create_association_class_dn(&compcsi->comp->comp_info.name,
		       &compcsi->csi->name, "safCSIComp", &dn);

	       if ((rc = avd_saImmOiRtObjectUpdate(&dn,"saAmfCSICompHAState", SA_IMM_ATTR_SAUINT32T, &ha_state)) != SA_AIS_OK)
		       LOG_ER("rc=%u, '%s'", rc, dn.value);
       }
}

/*****************************************************************************
 * Function: avd_susi_create
 *
 * Purpose:  This function will create and add a AVD_SU_SI_REL structure to 
 * the list of susi in both si and su.
 *
 * Input: cb - the AVD control block
 *        su - The SU structure that needs to have the SU SI relation.
 *        si - The SI structure that needs to have the SU SI relation.
 *
 * Returns: The AVD_SU_SI_REL structure that was created and added
 *
 * NOTES:
 *
 * 
 **************************************************************************/

AVD_SU_SI_REL *avd_susi_create(AVD_CL_CB *cb, AVD_SI *si, AVD_SU *su, SaAmfHAStateT state, NCS_BOOL ckpt)
{
	AVD_SU_SI_REL *su_si, *p_su_si, *i_su_si;
	AVD_SU *curr_su = 0;
	AVD_SUS_PER_SI_RANK_INDX i_idx;
	AVD_SUS_PER_SI_RANK *su_rank_rec = 0, *i_su_rank_rec = 0;
	uns32 rank1, rank2;

	TRACE_ENTER2("%s %s state=%u", su->name.value, si->name.value, state);

	/* Allocate a new block structure now
	 */
	if ((su_si = calloc(1, sizeof(AVD_SU_SI_REL))) == NULL) {
		/* log an error */
		LOG_ER("Memory Alloc failed.");
		return NULL;
	}

	su_si->state = state;
	su_si->fsm = AVD_SU_SI_STATE_ABSENT;
	su_si->list_of_csicomp = NULL;
	su_si->si = si;
	su_si->su = su;

	/* 
	 * Add the susi rel rec to the ordered si-list
	 */

	/* determine if the su is ranked per si */
	memset((uns8 *)&i_idx, '\0', sizeof(i_idx));
	i_idx.si_name = si->name;
	i_idx.su_rank = 0;
	for (su_rank_rec = avd_sirankedsu_getnext(cb, i_idx);
	     su_rank_rec && (m_CMP_HORDER_SANAMET(su_rank_rec->indx.si_name, si->name) == 0);
	     su_rank_rec = avd_sirankedsu_getnext(cb, su_rank_rec->indx)) {
		curr_su = avd_su_get(&su_rank_rec->su_name);
		if (curr_su == su)
			break;
	}

	/* set the ranking flag */
	su_si->is_per_si = (curr_su == su) ? TRUE : FALSE;

	/* determine the insert position */
	for (p_su_si = NULL, i_su_si = si->list_of_sisu;
	     i_su_si; p_su_si = i_su_si, i_su_si = i_su_si->si_next) {
		if (i_su_si->is_per_si == TRUE) {
			if (FALSE == su_si->is_per_si)
				continue;

			/* determine the su_rank rec for this rec */
			memset((uns8 *)&i_idx, '\0', sizeof(i_idx));
			i_idx.si_name = si->name;
			i_idx.su_rank = 0;
			for (i_su_rank_rec = avd_sirankedsu_getnext(cb, i_idx);
			     i_su_rank_rec
			     && (m_CMP_HORDER_SANAMET(i_su_rank_rec->indx.si_name, si->name) == 0);
			     i_su_rank_rec = avd_sirankedsu_getnext(cb, i_su_rank_rec->indx)) {
				curr_su = avd_su_get(&i_su_rank_rec->su_name);
				if (curr_su == i_su_si->su)
					break;
			}

			assert(i_su_rank_rec);

			rank1 = su_rank_rec->indx.su_rank;
			rank2 = i_su_rank_rec->indx.su_rank;
			if (rank1 <= rank2)
				break;
		} else {
			if (TRUE == su_si->is_per_si)
				break;

			if (su->saAmfSURank <= i_su_si->su->saAmfSURank)
				break;
		}
	}			/* for */

	/* now insert the susi rel at the correct position */
	if (p_su_si) {
		su_si->si_next = p_su_si->si_next;
		p_su_si->si_next = su_si;
	} else {
		su_si->si_next = si->list_of_sisu;
		si->list_of_sisu = su_si;
	}

	/* keep the list in su inascending order */
	if (su->list_of_susi == NULL) {
		su->list_of_susi = su_si;
		su_si->su_next = NULL;
		goto done;
	}

	p_su_si = NULL;
	i_su_si = su->list_of_susi;
	while ((i_su_si != NULL) &&
	       (m_CMP_HORDER_SANAMET(i_su_si->si->name, su_si->si->name) < 0)) {
		p_su_si = i_su_si;
		i_su_si = i_su_si->su_next;
	}

	if (p_su_si == NULL) {
		su_si->su_next = su->list_of_susi;
		su->list_of_susi = su_si;
	} else {
		su_si->su_next = p_su_si->su_next;
		p_su_si->su_next = su_si;
	}

done:
	if ((ckpt == FALSE) && (su_si != NULL)) {
		avd_create_susi_in_imm(state, &si->name, &su->name);
		saflog(LOG_NOTICE, amfSvcUsrName, "HA State %s of %s for %s",
			   avd_ha_state[state], su->name.value, si->name.value);

		if (state == SA_AMF_HA_ACTIVE) {
			avd_su_inc_curr_act_si(su);
			avd_si_inc_curr_act_ass(si);
		}
		else {
			avd_su_inc_curr_stdby_si(su);
			avd_si_inc_curr_stdby_ass(si);
		}
	}

	TRACE_LEAVE();
	return su_si;
}

/*****************************************************************************
 * Function: avd_su_susi_find
 *
 * Purpose:  This function will find a AVD_SU_SI_REL structure from the
 * list of susis in a su.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the SU . 
 *        si_name - The SI name of the SU SI relation.
 *        
 * Returns: The AVD_SU_SI_REL structure that was found.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

AVD_SU_SI_REL *avd_su_susi_find(AVD_CL_CB *cb, AVD_SU *su, const SaNameT *si_name)
{
	AVD_SU_SI_REL *su_si;
	SaNameT lsi_name;

	su_si = su->list_of_susi;

	memset((char *)&lsi_name, '\0', sizeof(SaNameT));
	memcpy(lsi_name.value, si_name->value, si_name->length);
	lsi_name.length = si_name->length;

	while ((su_si != NULL) && (m_CMP_HORDER_SANAMET(su_si->si->name, lsi_name) < 0)) {
		su_si = su_si->su_next;
	}

	if ((su_si != NULL) && (m_CMP_HORDER_SANAMET(su_si->si->name, lsi_name) == 0)) {
		return su_si;
	}

	return NULL;
}

/*****************************************************************************
 * Function: avd_susi_find
 *
 * Purpose:  This function will find a AVD_SU_SI_REL structure from the
 * list of susis in a su.
 *
 * Input: cb - the AVD control block
 *        su_name - The SU name of the SU SI relation. 
 *        si_name - The SI name of the SU SI relation.
 *        
 * Returns: The AVD_SU_SI_REL structure that was found.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

AVD_SU_SI_REL *avd_susi_find(AVD_CL_CB *cb, const SaNameT *su_name, const SaNameT *si_name)
{
	AVD_SU *su;

	if ((su = avd_su_get(su_name)) == NULL)
		return NULL;

	return avd_su_susi_find(cb, su, si_name);
}

/*****************************************************************************
 * Function: avd_susi_find_next
 *
 * Purpose:  This function will find the next AVD_SU_SI_REL structure from the
 * list of susis in a su. If NULL, it will find the first SUSI for the next
 * SU in the tree.
 *
 * Input: cb - the AVD control block
 *        su_name - The SU name of the SU SI relation. 
 *        si_name - The SI name of the SU SI relation.
 *        
 * Returns: The AVD_SU_SI_REL structure that was found.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

AVD_SU_SI_REL *avd_susi_find_next(AVD_CL_CB *cb, SaNameT su_name, SaNameT si_name)
{
	AVD_SU *su;
	AVD_SU_SI_REL *su_si = NULL;
	SaNameT lsu_name, lsi_name;

	/* check if exact match of SU is found so that the next SU SI
	 * in the list of SU can be found. If not select the next SUs
	 * first SU SI relation
	 */
	if (su_name.length != 0) {
		su = avd_su_get(&su_name);
		if (su == NULL)
			su_si = NULL;
		else
			su_si = su->list_of_susi;
	}

	memset((char *)&lsi_name, '\0', sizeof(SaNameT));
	memcpy(lsi_name.value, si_name.value, si_name.length);
	lsi_name.length = si_name.length;
	
	while ((su_si != NULL) && (m_CMP_HORDER_SANAMET(su_si->si->name, lsi_name) <= 0)) {
		su_si = su_si->su_next;
	}

	if (su_si != NULL) {
		return su_si;
	}

	/* Find the the first SU SI relation in the next SU with
	 * a SU SI relation.
	 */
	lsu_name = su_name;

	while ((su = avd_su_getnext(&lsu_name)) != NULL) {
		if (su->list_of_susi != NULL)
			break;

		lsu_name = su->name;
	}

	/* The given element didn't have a exact match but an element with
	 * a greater SI name was found in the list
	 */

	if (su == NULL)
		return NULL;
	else
		return su->list_of_susi;

	return su_si;
}

/*****************************************************************************
 * Function: avd_susi_delete
 *
 * Purpose:  This function will delete and free AVD_SU_SI_REL structure both
 * the su and si list of susi structures.
 *
 * Input: cb - the AVD control block
 *        susi - The SU SI relation structure that needs to be deleted. 
 *
 * Returns:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE 
 *
 *
 * NOTES:
 *
 * 
 **************************************************************************/

uns32 avd_susi_delete(AVD_CL_CB *cb, AVD_SU_SI_REL *susi, NCS_BOOL ckpt)
{
	AVD_SU_SI_REL *p_su_si, *p_si_su, *i_su_si;
	AVD_AVND *avnd = NULL;
	SaBoolT is_ncs = susi->su->sg_of_su->sg_ncs_spec;

	TRACE_ENTER2("%s %s", susi->su->name.value, susi->si->name.value);
	m_AVD_GET_SU_NODE_PTR(cb, susi->su, avnd);

	/* check the SU list to get the prev pointer */
	i_su_si = susi->su->list_of_susi;
	p_su_si = NULL;
	while ((i_su_si != NULL) && (i_su_si != susi)) {
		p_su_si = i_su_si;
		i_su_si = i_su_si->su_next;
	}

	if (i_su_si == NULL) {
		/* problem it is mssing to delete */
		/* log error */
		return NCSCC_RC_FAILURE;
	}

	/* check the SI list to get the prev pointer */
	i_su_si = susi->si->list_of_sisu;
	p_si_su = NULL;

	while ((i_su_si != NULL) && (i_su_si != susi)) {
		p_si_su = i_su_si;
		i_su_si = i_su_si->si_next;
	}

	if (i_su_si == NULL) {
		/* problem it is mssing to delete */
		/* log error */
		return NCSCC_RC_FAILURE;
	}

	/* now delete it from the SU list */
	if (p_su_si == NULL) {
		susi->su->list_of_susi = susi->su_next;
		susi->su_next = NULL;
	} else {
		p_su_si->su_next = susi->su_next;
		susi->su_next = NULL;
	}

	/* now delete it from the SI list */
	if (p_si_su == NULL) {
		susi->si->list_of_sisu = susi->si_next;
		susi->si_next = NULL;
	} else {
		p_si_su->si_next = susi->si_next;
		susi->si_next = NULL;
	}

	if (ckpt == FALSE) {
		if (susi->fsm == AVD_SU_SI_STATE_MODIFY) {
			/* Dont delete here, if i am in modify state. 
			   only happens when active -> qsd and standby rebooted */
		} else if (SA_AMF_HA_STANDBY == susi->state) {
			avd_su_dec_curr_stdby_si(susi->su);
			avd_si_dec_curr_stdby_ass(susi->si);
		} else {
			avd_su_dec_curr_act_si(susi->su);
			avd_si_dec_curr_act_ass(susi->si);
		}
	}

	avd_delete_siassignment_from_imm(&susi->si->name, &susi->su->name);

	susi->si = NULL;
	susi->su = NULL;

	/* call the func to check on the context for deletion */
	if (!ckpt) {
		avd_chk_failover_shutdown_cxt(cb, avnd, is_ncs);
	}

	free(susi);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

void avd_susi_ha_state_set(AVD_SU_SI_REL *susi, SaAmfHAStateT ha_state)
{
	SaAmfHAStateT old_state = susi->state;

	assert(ha_state <= SA_AMF_HA_QUIESCING);
	TRACE_ENTER2("'%s' %s => %s", susi->si->name.value, avd_ha_state[susi->state],
			avd_ha_state[ha_state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s HA State %s => %s", susi->si->name.value,
			avd_ha_state[susi->state], avd_ha_state[ha_state]);

	susi->state = ha_state;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(avd_cb, susi, AVSV_CKPT_AVD_SI_ASS);

	/* alarm & notifications */
	avd_send_su_ha_state_chg_ntf(&susi->su->name, &susi->si->name, old_state, susi->state);
}

/* This function serves as a wrapper. avd_susi_ha_state_set should be used for state 
 * changes and ntf but introducing avd_susi_ha_state_set and removing 
 * avd_gen_su_ha_state_changed_ntf (155 occurrences!) have big impact on the code.
 * */
uns32 avd_gen_su_ha_state_changed_ntf(AVD_CL_CB *avd_cb, AVD_SU_SI_REL *susi)
{
	uns32 status = NCSCC_RC_FAILURE;

	TRACE_ENTER2("'%s' assigned to '%s' HA state UNKNOWN => %s", susi->si->name.value, 
			susi->su->name.value, avd_ha_state[susi->state]);
	saflog(LOG_NOTICE, amfSvcUsrName, "%s assigned to %s HA State UNKNOWN => %s", 
			susi->si->name.value, susi->su->name.value, avd_ha_state[susi->state]);

	/* alarm & notifications */
	avd_send_su_ha_state_chg_ntf(&susi->su->name, &susi->si->name, SA_FALSE, susi->state); /*old state not known*/
	
	return status;
}
