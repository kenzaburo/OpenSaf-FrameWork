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
 
  This module contains routines to create, modify, delete & fetch the SU-SI
  and component CSI relationship records.

..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "avnd.h"
#include <immutil.h>

/* static function declarations */

static uns32 avnd_su_si_csi_rec_modify(AVND_CB *, AVND_SU *, AVND_SU_SI_REC *, AVND_COMP_CSI_PARAM *);

static uns32 avnd_su_si_csi_all_modify(AVND_CB *, AVND_SU *, AVND_COMP_CSI_PARAM *);

static uns32 avnd_su_si_csi_rec_del(AVND_CB *, AVND_SU *, AVND_SU_SI_REC *, AVND_COMP_CSI_REC *);

static uns32 avnd_su_si_csi_del(AVND_CB *, AVND_SU *, AVND_SU_SI_REC *);

/* macro to add a csi-record to the si-csi list */
#define m_AVND_SU_SI_CSI_REC_ADD(si, csi, rc) \
{ \
   (csi).si_dll_node.key = (uns8 *)&(csi).rank; \
   rc = ncs_db_link_list_add(&(si).csi_list, &(csi).si_dll_node); \
};

/* macro to remove a csi-record from the si-csi list */
#define m_AVND_SU_SI_CSI_REC_REM(si, csi) \
           ncs_db_link_list_delink(&(si).csi_list, &(csi).si_dll_node)

/* macro to add a susi record to the beginning of the susi queue */
#define m_AVND_SUDB_REC_SIQ_ADD(su, susi, rc) \
           (rc) = ncs_db_link_list_add(&(su).siq, &(susi).su_dll_node);

/****************************************************************************
  Name          : avnd_su_si_rec_add
 
  Description   : This routine creates an su-si relationship record. It 
                  updates the su-si record & creates the records in the 
                  comp-csi list. If an su-si relationship record already
                  exists, nothing is done.
 
  Arguments     : cb    - ptr to AvND control block
                  su    - ptr to the AvND SU
                  param - ptr to the SI parameters
                  rc    - ptr to the operation result
 
  Return Values : ptr to the su-si relationship record
 
  Notes         : None
******************************************************************************/
AVND_SU_SI_REC *avnd_su_si_rec_add(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_PARAM *param, uns32 *rc)
{
	AVND_SU_SI_REC *si_rec = 0;
	AVND_COMP_CSI_PARAM *csi_param = 0;

	TRACE_ENTER();

	*rc = NCSCC_RC_SUCCESS;

	/* verify if su-si relationship already exists */
	if (0 != avnd_su_si_rec_get(cb, &param->su_name, &param->si_name)) {
		*rc = AVND_ERR_DUP_SI;
		goto err;
	}

	/* a fresh si... */
	si_rec = calloc(1, sizeof(AVND_SU_SI_REC));
	if (!si_rec) {
		*rc = AVND_ERR_NO_MEMORY;
		goto err;
	}

	/*
	 * Update the supplied parameters.
	 */
	/* update the si-name (key) */
	memcpy(&si_rec->name, &param->si_name, sizeof(SaNameT));
	si_rec->curr_state = param->ha_state;

	/*
	 * Update the rest of the parameters with default values.
	 */
	m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(si_rec, AVND_SU_SI_ASSIGN_STATE_UNASSIGNED);

	/*
	 * Add the csi records.
	 */
	/* initialize the csi-list (maintained by si) */
	si_rec->csi_list.order = NCS_DBLIST_ASSCEND_ORDER;
	si_rec->csi_list.cmp_cookie = avsv_dblist_uns32_cmp;
	si_rec->csi_list.free_cookie = 0;

	/*
	 * Add to the si-list (maintained by su)
	 */
	m_AVND_SUDB_REC_SI_ADD(*su, *si_rec, *rc);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_DLL;
		goto err;
	}

	/*
	 * Update links to other entities.
	 */
	si_rec->su = su;
	si_rec->su_name = su->name;

	/* now add the csi records */
	csi_param = param->list;
	while (0 != csi_param) {
		avnd_su_si_csi_rec_add(cb, su, si_rec, csi_param, rc);
		if (NCSCC_RC_SUCCESS != *rc)
			goto err;
		csi_param = csi_param->next;
	}

	TRACE_1("SU-SI record added, SU= %s : SI=%s",param->su_name.value,param->si_name.value);
	return si_rec;

 err:
	if (si_rec) {
		avnd_su_si_csi_del(cb, su, si_rec);
		free(si_rec);
	}

	LOG_CR("SU-SI record addition failed, SU= %s : SI=%s",param->su_name.value,param->si_name.value);
	TRACE_LEAVE();
	return 0;
}

/****************************************************************************
  Name          : avnd_su_si_csi_rec_add
 
  Description   : This routine creates a comp-csi relationship record & adds 
                  it to the 2 csi lists (maintained by si & comp).
 
  Arguments     : cb     - ptr to AvND control block
                  su     - ptr to the AvND SU
                  si_rec - ptr to the SI record
                  param  - ptr to the CSI parameters
                  rc     - ptr to the operation result
 
  Return Values : ptr to the comp-csi relationship record
 
  Notes         : None
******************************************************************************/
AVND_COMP_CSI_REC *avnd_su_si_csi_rec_add(AVND_CB *cb,
					  AVND_SU *su, AVND_SU_SI_REC *si_rec, AVND_COMP_CSI_PARAM *param, uns32 *rc)
{
	AVND_COMP_CSI_REC *csi_rec = 0;
	AVND_COMP *comp = 0;

	TRACE_ENTER2("Comp'%s', Csi'%s' and Rank'%u'",param->csi_name.value, param->comp_name.value, param->csi_rank);

	*rc = NCSCC_RC_SUCCESS;

	/* verify if csi record already exists */
	if (0 != avnd_compdb_csi_rec_get(cb, &param->comp_name, &param->csi_name)) {
		*rc = AVND_ERR_DUP_CSI;
		goto err;
	}

	/* get the comp */
	comp = m_AVND_COMPDB_REC_GET(cb->compdb, param->comp_name);
	if (!comp) {
		*rc = AVND_ERR_NO_COMP;
		goto err;
	}

	/* a fresh csi... */
	csi_rec = calloc(1, sizeof(AVND_COMP_CSI_REC));
	if (!csi_rec) {
		*rc = AVND_ERR_NO_MEMORY;
		goto err;
	}

	/*
	 * Update the supplied parameters.
	 */
	/* update the csi-name & csi-rank (keys to comp-csi & si-csi lists resp) */
	memcpy(&csi_rec->name, &param->csi_name, sizeof(SaNameT));
	csi_rec->rank = param->csi_rank;

	{
		/* Fill the cs type param from imm db, it will be used in finding comp capability */
		SaAisErrorT error;
		SaImmAccessorHandleT accessorHandle;
		const SaImmAttrValuesT_2 **attributes;
		SaImmAttrNameT attributeNames[2] = {"saAmfCSType", NULL};


		immutil_saImmOmAccessorInitialize(avnd_cb->immOmHandle, &accessorHandle);

		if ((error = immutil_saImmOmAccessorGet_2(accessorHandle, &param->csi_name, attributeNames, 
						(SaImmAttrValuesT_2 ***)&attributes)) != SA_AIS_OK) {
			LOG_ER("saImmOmAccessorGet FAILED %u for %s", error, param->csi_name.value);
			assert(0);
		}

		if (immutil_getAttr("saAmfCSType", attributes, 0, &csi_rec->saAmfCSType) != SA_AIS_OK)
			assert(0);
		immutil_saImmOmAccessorFinalize(accessorHandle);
	}

	/* update the assignment related parameters */
	memcpy(&csi_rec->act_comp_name, &param->active_comp_name, sizeof(SaNameT));
	csi_rec->trans_desc = param->active_comp_dsc;
	csi_rec->standby_rank = param->stdby_rank;

	/* update the csi-attrs.. steal it from param */
	csi_rec->attrs.number = param->attrs.number;
	csi_rec->attrs.list = param->attrs.list;
	param->attrs.number = 0;
	param->attrs.list = 0;

	/*
	 * Update the rest of the parameters with default values.
	 */
	m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(csi_rec, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);
	m_AVND_COMP_CSI_PRV_ASSIGN_STATE_SET(csi_rec, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);

	/*
	 * Add to the csi-list (maintained by si).
	 */
	m_AVND_SU_SI_CSI_REC_ADD(*si_rec, *csi_rec, *rc);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_DLL;
		goto err;
	}

	/*
	 * Add to the csi-list (maintained by comp).
	 */
	m_AVND_COMPDB_REC_CSI_ADD(*comp, *csi_rec, *rc);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_DLL;
		goto err;
	}

	/*
	 * Update links to other entities.
	 */
	csi_rec->si = si_rec;
	csi_rec->comp = comp;
	csi_rec->comp_name = comp->name;
	csi_rec->si_name = si_rec->name;
	csi_rec->su_name = su->name;

	return csi_rec;

 err:
	if (csi_rec) {
		/* remove from comp-csi & si-csi lists */
		ncs_db_link_list_delink(&si_rec->csi_list, &csi_rec->si_dll_node);
		m_AVND_COMPDB_REC_CSI_REM(*comp, *csi_rec);
		free(csi_rec);
	}

	LOG_CR("Comp-CSI record addition failed, Comp=%s : CSI=%s",param->comp_name.value,param->csi_name.value);
	TRACE_LEAVE();
	return 0;
}

/****************************************************************************
  Name          : avnd_su_si_rec_modify
 
  Description   : This routine modifies an su-si relationship record. It 
                  updates the su-si record & modifies the records in the 
                  comp-csi list.
 
  Arguments     : cb    - ptr to AvND control block
                  su    - ptr to the AvND SU
                  param - ptr to the SI parameters
                  rc    - ptr to the operation result
 
  Return Values : ptr to the modified su-si relationship record
 
  Notes         : None
******************************************************************************/
AVND_SU_SI_REC *avnd_su_si_rec_modify(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_PARAM *param, uns32 *rc)
{
	AVND_SU_SI_REC *si_rec = 0;

	TRACE_ENTER2();

	*rc = NCSCC_RC_SUCCESS;

	/* get the su-si relationship record */
	si_rec = avnd_su_si_rec_get(cb, &param->su_name, &param->si_name);
	if (!si_rec) {
		*rc = AVND_ERR_NO_SI;
		goto err;
	}

	/* store the prv state & update the new state */
	si_rec->prv_state = si_rec->curr_state;
	si_rec->curr_state = param->ha_state;

	/* store the prv assign-state & update the new assign-state */
	si_rec->prv_assign_state = si_rec->curr_assign_state;
	m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(si_rec, AVND_SU_SI_ASSIGN_STATE_UNASSIGNED);

	m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, si_rec, AVND_CKPT_SU_SI_REC);

	/* now modify the csi records */
	*rc = avnd_su_si_csi_rec_modify(cb, su, si_rec,
					((SA_AMF_HA_QUIESCED == param->ha_state) ||
					 (SA_AMF_HA_QUIESCING == param->ha_state)) ? 0 : param->list);
	if (*rc != NCSCC_RC_SUCCESS)
		goto err;
	TRACE_LEAVE();
	return si_rec;

 err:
	TRACE_LEAVE();
	return 0;
}

/****************************************************************************
  Name          : avnd_su_si_csi_rec_modify
 
  Description   : This routine modifies a comp-csi relationship record.
 
  Arguments     : cb     - ptr to AvND control block
                  su     - ptr to the AvND SU
                  si_rec - ptr to the SI record
                  param  - ptr to the CSI parameters
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_su_si_csi_rec_modify(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si_rec, AVND_COMP_CSI_PARAM *param)
{
	AVND_COMP_CSI_PARAM *curr_param = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%p", param);
	/* pick up all the csis belonging to the si & modify them */
	if (!param) {
		for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si_rec->csi_list);
		     curr_csi; curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {
			/* store the prv assign-state & update the new assign-state */
			curr_csi->prv_assign_state = curr_csi->curr_assign_state;
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_PRV_ASSIGN_STATE);
			m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);
			m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
		}		/* for */
	}

	/* pick up the csis belonging to the comps specified in the param-list */
	for (curr_param = param; curr_param; curr_param = curr_param->next) {
		/* get the comp & csi */
		curr_csi = avnd_compdb_csi_rec_get(cb, &curr_param->comp_name, &curr_param->csi_name);
		if (!curr_csi || (curr_csi->comp->su != su)) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		/* update the assignment related parameters */
		curr_csi->act_comp_name = curr_param->active_comp_name;
		curr_csi->trans_desc = curr_param->active_comp_dsc;
		curr_csi->standby_rank = curr_param->stdby_rank;

		/* store the prv assign-state & update the new assign-state */
		curr_csi->prv_assign_state = curr_csi->curr_assign_state;
		m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_CSI_REC);
	}			/* for */

 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_all_modify
 
  Description   : This routine modifies all the SU-SI & comp-csi records in 
                  the database.
 
  Arguments     : cb    - ptr to AvND control block
                  su    - ptr to the AvND SU
                  param - ptr to the SI parameters
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_su_si_all_modify(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_PARAM *param)
{
	AVND_SU_SI_REC *curr_si = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2();
	/* modify all the si records */
	for (curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
	     curr_si; curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_si->su_dll_node)) {
		/* store the prv state & update the new state */
		curr_si->prv_state = curr_si->curr_state;
		curr_si->curr_state = param->ha_state;

		/* store the prv assign-state & update the new assign-state */
		curr_si->prv_assign_state = curr_si->curr_assign_state;
		m_AVND_SU_SI_CURR_ASSIGN_STATE_SET(curr_si, AVND_SU_SI_ASSIGN_STATE_UNASSIGNED);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_si, AVND_CKPT_SU_SI_REC);
	}			/* for */

	/* now modify the comp-csi records */
	rc = avnd_su_si_csi_all_modify(cb, su, ((SA_AMF_HA_QUIESCED == param->ha_state) ||
						(SA_AMF_HA_QUIESCING == param->ha_state)) ? 0 : param->list);
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_csi_all_modify
 
  Description   : This routine modifies the csi records.
 
  Arguments     : cb     - ptr to AvND control block
                  su     - ptr to the AvND SU
                  param  - ptr to the CSI parameters (if 0, => all the CSIs 
                           belonging to all the SIs in the SU are modified. 
                           Else all the CSIs belonging to all the components 
                           in the SU are modified)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_su_si_csi_all_modify(AVND_CB *cb, AVND_SU *su, AVND_COMP_CSI_PARAM *param)
{
	AVND_COMP_CSI_PARAM *curr_param = 0;
	AVND_COMP_CSI_REC *curr_csi = 0;
	AVND_SU_SI_REC *curr_si = 0;
	AVND_COMP *curr_comp = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER2("%p", param);
	/* pick up all the csis belonging to all the sis & modify them */
	if (!param) {
		for (curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list);
		     curr_si; curr_si = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_si->su_dll_node)) {
			for (curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&curr_si->csi_list);
			     curr_csi; curr_csi = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_NEXT(&curr_csi->si_dll_node)) {
				/* store the prv assign-state & update the new assign-state */
				curr_csi->prv_assign_state = curr_csi->curr_assign_state;
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_PRV_ASSIGN_STATE);
				m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_COMP_CSI_CURR_ASSIGN_STATE);
			}	/* for */
		}		/* for */
	}

	/* pick up all the csis belonging to the comps specified in the param-list */
	for (curr_param = param; curr_param; curr_param = curr_param->next) {
		/* get the comp */
		curr_comp = m_AVND_COMPDB_REC_GET(cb->compdb, curr_param->comp_name);
		if (!curr_comp || (curr_comp->su != su)) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		curr_comp->assigned_flag = FALSE;
	}

	/* pick up all the csis belonging to the comps specified in the param-list */
	for (curr_param = param; curr_param; curr_param = curr_param->next) {
		/* get the comp */
		curr_comp = m_AVND_COMPDB_REC_GET(cb->compdb, curr_param->comp_name);
		if (!curr_comp || (curr_comp->su != su)) {
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		if (FALSE == curr_comp->assigned_flag) {
			/* modify all the csi-records */
			for (curr_csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_FIRST(&curr_comp->csi_list));
					curr_csi;
					curr_csi = m_AVND_CSI_REC_FROM_COMP_DLL_NODE_GET(m_NCS_DBLIST_FIND_NEXT(&curr_csi->comp_dll_node)))
			{
				/* update the assignment related parameters */
				curr_csi->act_comp_name = curr_param->active_comp_name;
				curr_csi->trans_desc = curr_param->active_comp_dsc;
				curr_csi->standby_rank = curr_param->stdby_rank;

				/* store the prv assign-state & update the new assign-state */
				curr_csi->prv_assign_state = curr_csi->curr_assign_state;
				m_AVND_COMP_CSI_CURR_ASSIGN_STATE_SET(curr_csi, AVND_COMP_CSI_ASSIGN_STATE_UNASSIGNED);
				m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, curr_csi, AVND_CKPT_CSI_REC);
			}		/* for */
			curr_comp->assigned_flag = TRUE;
		}
	}			/* for */

 done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_rec_del
 
  Description   : This routine deletes a su-si relationship record. It 
                  traverses the entire csi-list and deletes each comp-csi 
                  relationship record.
 
  Arguments     : cb          - ptr to AvND control block
                  su_name - ptr to the su-name
                  si_name - ptr to the si-name
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_su_si_rec_del(AVND_CB *cb, SaNameT *su_name, SaNameT *si_name)
{
	AVND_SU *su = 0;
	AVND_SU_SI_REC *si_rec = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s' : '%s'", su_name->value, si_name->value);

	/* get the su record */
	su = m_AVND_SUDB_REC_GET(cb->sudb, *su_name);
	if (!su) {
		rc = AVND_ERR_NO_SU;
		goto err;
	}

	/* get the si record */
	si_rec = avnd_su_si_rec_get(cb, su_name, si_name);
	if (!si_rec) {
		rc = AVND_ERR_NO_SI;
		goto err;
	}

	/*
	 * Delete the csi-list.
	 */
	rc = avnd_su_si_csi_del(cb, su, si_rec);
	if (NCSCC_RC_SUCCESS != rc)
		goto err;

	/*
	 * Detach from the si-list (maintained by su).
	 */
	rc = m_AVND_SUDB_REC_SI_REM(*su, *si_rec);
	if (NCSCC_RC_SUCCESS != rc)
		goto err;

	TRACE_1("SU-SI record deleted, SU= %s : SI=%s",su_name->value,si_name->value);

	/* free the memory */
	free(si_rec);

	return rc;

 err:
	LOG_CR("SU-SI record deletion failed, SU= %s : SI=%s",su_name->value,si_name->value);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_del
 
  Description   : This routine traverses the entire si-list and deletes each
                  record.
 
  Arguments     : cb          - ptr to AvND control block
                  su_name - ptr to the su-name (n/w order)
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_su_si_del(AVND_CB *cb, SaNameT *su_name)
{
	AVND_SU *su = 0;
	AVND_SU_SI_REC *si_rec = 0;
	SaNameT lsu_name;
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s'", su_name->value);

	/* get the su record */
	su = m_AVND_SUDB_REC_GET(cb->sudb, *su_name);
	if (!su) {
		rc = AVND_ERR_NO_SU;
		goto err;
	}

	/* scan & delete each si record */
	while (0 != (si_rec = (AVND_SU_SI_REC *)m_NCS_DBLIST_FIND_FIRST(&su->si_list))) {
		m_AVND_SEND_CKPT_UPDT_ASYNC_RMV(cb, si_rec, AVND_CKPT_SU_SI_REC);
		rc = avnd_su_si_rec_del(cb, su_name, &si_rec->name);
		if (NCSCC_RC_SUCCESS != rc)
			goto err;
	}

	if (cb->term_state == AVND_TERM_STATE_SHUTTING_APP_DONE) {
		/* check whether all NCS SI are removed */
		memset(&lsu_name, 0, sizeof(SaNameT));
		su = m_AVND_SUDB_REC_GET_NEXT(cb->sudb, su_name);

		while (su && (su->si_list.n_nodes == 0)) {
			lsu_name.length = su->name.length;
			memcpy(&lsu_name.value, &su->name.value, su->name.length);
			su = m_AVND_SUDB_REC_GET_NEXT(cb->sudb, lsu_name);
		}

		/* All SI's are removed, Now we can safely terminate the SU's */
		if (su == AVND_SU_NULL)
			cb->term_state = AVND_TERM_STATE_SHUTTING_NCS_SI;

	}
	TRACE_LEAVE2("%u", rc);
	return rc;

 err:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_csi_del
 
  Description   : This routine traverses the each record in the csi-list 
                  (maintained by si) & deletes them.
 
  Arguments     : cb      - ptr to AvND control block
                  su      - ptr to the AvND SU
                  si_rec  - ptr to the SI record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_su_si_csi_del(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si_rec)
{
	AVND_COMP_CSI_REC *csi_rec = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s' : '%s'", su->name.value, si_rec->name.value);

	/* scan & delete each csi record */
	while (0 != (csi_rec = (AVND_COMP_CSI_REC *)m_NCS_DBLIST_FIND_FIRST(&si_rec->csi_list))) {
		rc = avnd_su_si_csi_rec_del(cb, si_rec->su, si_rec, csi_rec);
		if (NCSCC_RC_SUCCESS != rc)
			goto err;

	}
	TRACE_LEAVE2("%u", rc);
	return rc;

 err:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_csi_rec_del
 
  Description   : This routine deletes a comp-csi relationship record.
 
  Arguments     : cb      - ptr to AvND control block
                  su      - ptr to the AvND SU
                  si_rec  - ptr to the SI record
                  csi_rec - ptr to the CSI record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_su_si_csi_rec_del(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si_rec, AVND_COMP_CSI_REC *csi_rec)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s' : '%s' : '%s'", su->name.value, si_rec->name.value, csi_rec->name.value);

	/* remove from the comp-csi list */
	rc = m_AVND_COMPDB_REC_CSI_REM(*(csi_rec->comp), *csi_rec);
	if (NCSCC_RC_SUCCESS != rc)
		goto err;

	/* if all csi's are removed & the pres state is un-instantiated for
	 * a  npi comp, its time to mark it as healthy 
	 */
	if (m_AVND_COMP_IS_FAILED(csi_rec->comp) &&
	    !m_AVND_COMP_TYPE_IS_PREINSTANTIABLE(csi_rec->comp) &&
	    m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(csi_rec->comp) && csi_rec->comp->csi_list.n_nodes == 0) {
		m_AVND_COMP_FAILED_RESET(csi_rec->comp);
		m_AVND_SEND_CKPT_UPDT_ASYNC_UPDT(cb, csi_rec->comp, AVND_CKPT_COMP_FLAG_CHANGE);
	}

	/* remove from the si-csi list */
	rc = m_AVND_SU_SI_CSI_REC_REM(*si_rec, *csi_rec);
	if (NCSCC_RC_SUCCESS != rc)
		goto err;

	/* 
	 * Free the memory alloced for this record.
	 */
	/* free the csi attributes */
	if (csi_rec->attrs.list)
		free(csi_rec->attrs.list);

	/* free the pg list TBD */
	TRACE_1("Comp-CSI record deletion success, Comp=%s : CSI=%s",csi_rec->comp->name.value,csi_rec->name.value);

	/* finally free this record */
	free(csi_rec);

	TRACE_LEAVE();
	return rc;

 err:
	LOG_CR("Comp-CSI record deletion failed, Comp=%s : CSI=%s",csi_rec->comp->name.value,csi_rec->name.value);
	return rc;
}

/****************************************************************************
  Name          : avnd_su_si_rec_get
 
  Description   : This routine gets the su-si relationship record from the
                  si-list (maintained on su).
 
  Arguments     : cb          - ptr to AvND control block
                  su_name - ptr to the su-name (n/w order)
                  si_name - ptr to the si-name (n/w order)
 
  Return Values : ptr to the su-si record (if any)
 
  Notes         : None
******************************************************************************/
AVND_SU_SI_REC *avnd_su_si_rec_get(AVND_CB *cb, SaNameT *su_name, SaNameT *si_name)
{
	AVND_SU_SI_REC *si_rec = 0;
	AVND_SU *su = 0;

	TRACE_ENTER2("'%s' : '%s'", su_name->value, si_name->value);
	/* get the su record */
	su = m_AVND_SUDB_REC_GET(cb->sudb, *su_name);
	if (!su)
		goto done;

	/* get the si record */
	si_rec = (AVND_SU_SI_REC *)ncs_db_link_list_find(&su->si_list, (uns8 *)si_name);

 done:
	TRACE_LEAVE();
	return si_rec;
}

/****************************************************************************
  Name          : avnd_su_siq_rec_add
 
  Description   : This routine buffers the susi assign message parameters in 
                  the susi queue.
 
  Arguments     : cb    - ptr to AvND control block
                  su    - ptr to the AvND SU
                  param - ptr to the SI parameters
                  rc    - ptr to the operation result
 
  Return Values : ptr to the si queue record
 
  Notes         : None
******************************************************************************/
AVND_SU_SIQ_REC *avnd_su_siq_rec_add(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_PARAM *param, uns32 *rc)
{
	AVND_SU_SIQ_REC *siq = 0;

	*rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER2("'%s'", su->name.value);

	/* alloc the siq rec */
	siq = calloc(1, sizeof(AVND_SU_SIQ_REC));
	if (!siq) {
		*rc = AVND_ERR_NO_MEMORY;
		goto err;
	}

	/* Add to the siq (maintained by su) */
	m_AVND_SUDB_REC_SIQ_ADD(*su, *siq, *rc);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_DLL;
		goto err;
	}

	/* update the param */
	siq->info = *param;

	/* memory transferred to the siq-rec.. nullify it in param */
	param->list = 0;

	TRACE_LEAVE();
	return siq;

 err:
	if (siq)
		free(siq);

	TRACE_LEAVE();
	return 0;
}

/****************************************************************************
  Name          : avnd_su_siq_rec_del
 
  Description   : This routine deletes the buffered susi assign message from 
                  the susi queue.
 
  Arguments     : cb  - ptr to AvND control block
                  su  - ptr to the AvND SU
                  siq - ptr to the si queue rec
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_su_siq_rec_del(AVND_CB *cb, AVND_SU *su, AVND_SU_SIQ_REC *siq)
{
	AVSV_SUSI_ASGN *curr = 0;
	TRACE_ENTER2("'%s'", su->name.value);

	/* delete the comp-csi info */
	while ((curr = siq->info.list) != 0) {
		siq->info.list = curr->next;
		if (curr->attrs.list)
			free(curr->attrs.list);

		free(curr);
	}

	/* free the rec */
	free(siq);

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : avnd_mbcsv_su_si_csi_rec_del
 
  Description   : This routine is a wrapper of avnd_su_si_csi_rec_del.
 
  Arguments     : cb      - ptr to AvND control block
                  su      - ptr to the AvND SU
                  si_rec  - ptr to the SI record
                  csi_rec - ptr to the CSI record
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
uns32 avnd_mbcsv_su_si_csi_rec_del(AVND_CB *cb, AVND_SU *su, AVND_SU_SI_REC *si_rec, AVND_COMP_CSI_REC *csi_rec)
{
	return (avnd_su_si_csi_rec_del(cb, su, si_rec, csi_rec));
}

/****************************************************************************
  Name          : avnd_mbcsv_su_si_csi_rec_add
 
  Description   : This routine is a wrapper of avnd_su_si_csi_rec_add. 
 
  Arguments     : cb     - ptr to AvND control block
                  su     - ptr to the AvND SU
                  si_rec - ptr to the SI record
                  param  - ptr to the CSI parameters
                  rc     - ptr to the operation result
 
  Return Values : ptr to the comp-csi relationship record
 
  Notes         : None
******************************************************************************/
AVND_COMP_CSI_REC *avnd_mbcsv_su_si_csi_rec_add(AVND_CB *cb,
						AVND_SU *su,
						AVND_SU_SI_REC *si_rec, AVND_COMP_CSI_PARAM *param, uns32 *rc)
{
	return (avnd_su_si_csi_rec_add(cb, su, si_rec, param, rc));
}
