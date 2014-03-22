#ifndef PLMS_UTILS_H
#define PLMS_UTILS_H


uns32 plms_anc_chld_dep_adm_flag_is_set(PLMS_ENTITY *, PLMS_GROUP_ENTITY *);
uns32 plms_is_chld(PLMS_ENTITY *,PLMS_ENTITY *);
void plms_affected_ent_list_get( PLMS_ENTITY *, PLMS_GROUP_ENTITY **,SaBoolT);
uns32 plms_chld_get(PLMS_ENTITY *, PLMS_GROUP_ENTITY **);
uns32 plms_aff_he_find(PLMS_GROUP_ENTITY *, PLMS_GROUP_ENTITY **);
uns32 plms_ent_grp_list_add(PLMS_ENTITY *, PLMS_ENTITY_GROUP_INFO_LIST **);
void plms_ent_to_ent_list_add(PLMS_ENTITY *,PLMS_GROUP_ENTITY **);
uns32 plms_no_aff_ent_in_grp_get(PLMS_ENTITY_GROUP_INFO *,PLMS_GROUP_ENTITY *);
void plms_inv_to_trk_list_free(PLMS_INVOCATION_TO_TRACK_INFO *);
void plms_ent_list_free(PLMS_GROUP_ENTITY *);
void plms_ent_grp_list_free(PLMS_ENTITY_GROUP_INFO_LIST *);
uns32 plms_rdness_flag_is_set(PLMS_ENTITY *,SaPlmReadinessFlagsT);
uns32 plms_is_rdness_state_set(PLMS_ENTITY *,SaPlmReadinessStateT);
void plms_aff_ent_exp_rdness_state_ow(PLMS_GROUP_ENTITY *);
void plms_ent_exp_rdness_state_ow(PLMS_ENTITY *);
void plms_aff_ent_mark_unmark(PLMS_GROUP_ENTITY *, SaBoolT);
void plms_trk_info_free(PLMS_TRACK_INFO *);
SaBoolT plms_is_ent_in_ent_list(PLMS_GROUP_ENTITY *,PLMS_ENTITY *);
uns32 plms_move_ent_to_insvc(PLMS_ENTITY *,SaUint8T *);
void plms_move_chld_ent_to_insvc(PLMS_ENTITY *,PLMS_GROUP_ENTITY **,
SaUint8T,SaUint8T);
void plms_move_dep_ent_to_insvc(PLMS_GROUP_ENTITY *,PLMS_GROUP_ENTITY **,
SaUint8T);
uns32 plms_min_dep_is_ok(PLMS_ENTITY *);
uns32 plms_is_anc_in_deact_cnxt(PLMS_ENTITY *);
uns32 plms_mngt_lost_clear_cbk_call(PLMS_ENTITY *,uns32);
uns32 plms_ent_enable(PLMS_ENTITY *,uns32, uns32);
uns32 plms_ent_isolate(PLMS_ENTITY *,uns32,uns32);
uns32 plms_peer_async_send(PLMS_ENTITY *,SaPlmChangeStepT,SaPlmTrackCauseT);
void plms_aff_ent_flag_mark_unmark(PLMS_GROUP_ENTITY *,SaBoolT);



SaNtfIdentifierT plms_readiness_flag_clear(PLMS_ENTITY *,PLMS_ENTITY *,
				SaUint16T,SaUint16T);
SaNtfIdentifierT plms_readiness_flag_mark_unmark(PLMS_ENTITY *,
				SaPlmReadinessFlagsT,SaBoolT,PLMS_ENTITY *,
				SaUint16T,SaUint16T);
SaNtfIdentifierT plms_readiness_state_set(PLMS_ENTITY *,SaPlmReadinessStateT,
				PLMS_ENTITY *,SaUint16T,SaUint16T);
SaNtfIdentifierT plms_op_state_set(PLMS_ENTITY *,SaPlmOperationalStateT,
				PLMS_ENTITY *,SaUint16T,SaUint16T);
SaNtfIdentifierT plms_admin_state_set(PLMS_ENTITY *,uns32,PLMS_ENTITY *,
				SaUint16T,SaUint16T);
SaNtfIdentifierT plms_presence_state_set(PLMS_ENTITY *,uns32,PLMS_ENTITY *,
				SaUint16T,SaUint16T);
void plms_ent_list_grp_list_add(PLMS_GROUP_ENTITY *,
				PLMS_ENTITY_GROUP_INFO_LIST **);
void plms_aff_dep_ent_list_get(PLMS_ENTITY *,PLMS_GROUP_ENTITY *,
                                PLMS_GROUP_ENTITY **,PLMS_GROUP_ENTITY **,
				SaBoolT);
void plms_aff_chld_ent_list_get(PLMS_ENTITY *,PLMS_ENTITY *,
                                PLMS_GROUP_ENTITY **,PLMS_GROUP_ENTITY **,
				SaBoolT);
void plms_aff_ent_list_imnt_failure_get(PLMS_ENTITY *,PLMS_GROUP_ENTITY **,
				SaBoolT);
uns32 plms_ee_chld_of_aff_he_find(PLMS_ENTITY *, PLMS_GROUP_ENTITY *,
				PLMS_GROUP_ENTITY *, PLMS_GROUP_ENTITY **);
uns32 plms_ee_not_chld_of_aff_he_find(PLMS_GROUP_ENTITY *,PLMS_GROUP_ENTITY *,
				PLMS_GROUP_ENTITY **);
uns32 plms_grp_is_in_grp_list(PLMS_ENTITY_GROUP_INFO *,
				PLMS_ENTITY_GROUP_INFO_LIST *);
void plms_inv_to_trk_grp_add(PLMS_INVOCATION_TO_TRACK_INFO **,
				PLMS_INVOCATION_TO_TRACK_INFO *);
void plms_inv_to_cbk_in_grp_trk_rmv(PLMS_ENTITY_GROUP_INFO *,
				PLMS_TRACK_INFO *);
void plms_inv_to_cbk_in_grp_inv_rmv(PLMS_ENTITY_GROUP_INFO *,
				SaInvocationT);
PLMS_INVOCATION_TO_TRACK_INFO * plms_inv_to_cbk_in_grp_find(
				PLMS_INVOCATION_TO_TRACK_INFO *,SaInvocationT);
void plms_grp_aff_ent_fill(SaPlmReadinessTrackedEntityT *, 
				PLMS_ENTITY_GROUP_INFO *,PLMS_GROUP_ENTITY *,
				SaPlmGroupChangesT,SaPlmChangeStepT);
void plms_aff_ent_grp_exp_rdness_state_mark(PLMS_ENTITY_GROUP_INFO_LIST *,
				SaPlmHEAdminStateT,SaPlmReadinessFlagsT);
void plms_aff_ent_exp_rdness_state_mark(PLMS_GROUP_ENTITY *,SaPlmHEAdminStateT,
				SaPlmReadinessFlagsT);
void plms_ent_from_ent_list_rem(PLMS_ENTITY *, PLMS_GROUP_ENTITY **);

void plms_he_np_clean_up(PLMS_ENTITY *,PLMS_EPATH_TO_ENTITY_MAP_INFO **);

void plms_ent_exp_rdness_status_clear(PLMS_ENTITY *);
void plms_aff_ent_exp_rdness_status_clear(PLMS_GROUP_ENTITY *);

void plms_aff_chld_list_imnt_failure_get(PLMS_ENTITY *, PLMS_ENTITY *,
					SaUint32T , PLMS_GROUP_ENTITY **);
void plms_aff_dep_list_imnt_failure_get(PLMS_ENTITY *,PLMS_GROUP_ENTITY *,
					SaUint32T ,PLMS_GROUP_ENTITY **);

SaUint32T plms_attr_imm_update(PLMS_ENTITY *,SaInt8T *,SaImmValueTypeT ,SaUint64T);
SaUint32T plms_attr_sastring_imm_update(PLMS_ENTITY *,SaInt8T *,SaStringT,
SaImmAttrModificationTypeT);
SaUint32T plms_attr_saname_imm_update(PLMS_ENTITY *,SaInt8T *,SaNameT,
SaImmAttrModificationTypeT);
void plms_dep_immi_flag_check_and_set(PLMS_ENTITY *, PLMS_GROUP_ENTITY **,SaUint32T *);
#endif

