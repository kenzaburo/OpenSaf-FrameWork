#include "mbcsv_api.h"
#include "tet_api.h"
uns32 mbcstm_get_set_purposes(uns32 svc_index, uns32 ssn_index, uns32 asys,
                              uns32 action,NCS_MBCSV_OBJ obj_type, 
                              uns32 obj_val, MBCSTM_CB_TEST cb_check,
                              uns32 wait, uns32 peer_count,
                              uns64 peer_anchor, MBCSTM_FSM_STATES state_check);

uns32 mbcstm_get_set_purposes(uns32 svc_index, uns32 ssn_index, uns32 asys,
                              uns32 action,NCS_MBCSV_OBJ obj_type, 
                              uns32 obj_val, MBCSTM_CB_TEST cb_check,
                              uns32 wait, uns32 peer_count,
                              uns64 peer_anchor, MBCSTM_FSM_STATES state_check)
{
  uns32 test_result = NCSCC_RC_FAILURE;
  MBCSTM_PEERS_DATA peers;
  
  uns32        old_value;
  /*change*/
  uns32 mbcstm_check_inv(MBCSTM_CHECK , uns32 , uns32 , void *);
  int PASS=0;

  memset(&peers, '\0', sizeof(MBCSTM_PEERS_DATA));
  
  if(mbcstm_cb.sys == MBCSTM_SVC_INS3 || mbcstm_cb.sys == MBCSTM_SVC_INS4)
    {
      sleep(2);
      if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
        goto final;
      
      test_result = NCSCC_RC_SUCCESS; 
      goto final;
    }
  if(asys == mbcstm_cb.sys)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
    }
  if(mbcstm_svc_registration(svc_index) != NCSCC_RC_SUCCESS)
    goto final;
  if(mbcstm_ssn_open(svc_index,ssn_index) != NCSCC_RC_SUCCESS)
    goto final;
  sleep(1);
  
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    {
      if(action == NCS_MBCSV_OP_OBJ_SET)
        {
          if(obj_type ==  NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF)
            mbcstm_cb.svces[svc_index].ssns[ssn_index].ws_flag = obj_val;
          else
            mbcstm_cb.svces[svc_index].ssns[ssn_index].ws_timer = obj_val;
        }
      else
        {
          if(obj_type ==  NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF)
            old_value = mbcstm_cb.svces[svc_index].ssns[ssn_index].ws_flag;
          else
            old_value = mbcstm_cb.svces[svc_index].ssns[ssn_index].ws_timer;
          printf("\n old values %d", old_value);
        }
      
      if(mbcstm_svc_obj(svc_index, ssn_index,action,obj_type) 
         != NCSCC_RC_SUCCESS)
        goto final;
      else
        {
          printf("\n svc obj function successfully");
          PASS=1;
        }
      
      if(action == NCS_MBCSV_OP_OBJ_GET)
        {
          if(obj_type ==  NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF)
            {
              if(mbcstm_cb.svces[svc_index].ssns[ssn_index].ws_flag 
                 != old_value)
                goto final;
            }
          else
            {
              if(mbcstm_cb.svces[svc_index].ssns[ssn_index].ws_timer 
                 != old_value)
                goto final;
            }
        }
      
    }
  sleep(2);
  mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, svc_index, ssn_index, &peers);
  if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
    goto final;
  test_result = NCSCC_RC_SUCCESS;
 final:
  mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test = MBCSTM_CB_NO_TEST;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_flag = 0;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].ws_flag = TRUE;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].ws_timer =
    MBCSTM_TIMER_SEND_WARM_SYNC_PERIOD*100;
  mbcstm_svc_finalize (svc_index);
  sleep(2);
  if(PASS)
    test_result = NCSCC_RC_SUCCESS;
  return(test_result);    
}

void mbcstm_set_warm_sync_off_active()
{
  uns32 svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_set_warm_sync_off_active";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_get_set_purposes( svc_index, ssn_index,MBCSTM_SVC_INS1,
                               NCS_MBCSV_OP_OBJ_SET,
                               NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF,
                               FALSE,MBCSTM_CB_NO_TEST,1,3,MBCSTM_SSN_ANC2,
                               MBCSTM_ACT_STATE_MULTIPLE_ACTIVE ) 
      !=  NCSCC_RC_SUCCESS)
    {
      final_res = TET_MBCSV_FAIL;
      cases_failed++;
    }
  else
    {
      final_res = TET_MBCSV_PASS;
      cases_passed++;
    }
  
  mbcstm_test_print(case_num, case_name,case_disc, exp_res, final_res);
}

void mbcstm_set_warm_sync_off_standby()
{
  uns32 svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_set_warm_sync_off_standby";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_get_set_purposes( svc_index, ssn_index,MBCSTM_SVC_INS2,
                               NCS_MBCSV_OP_OBJ_SET,
                               NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF,
                               FALSE,MBCSTM_CB_NO_TEST,1,3,MBCSTM_SSN_ANC2,
                               MBCSTM_ACT_STATE_MULTIPLE_ACTIVE )
      !=  NCSCC_RC_SUCCESS)
    
    {
      final_res = TET_MBCSV_FAIL;
      cases_failed++;
    }
  else
    {
      final_res = TET_MBCSV_PASS;
      cases_passed++;
    } 
  mbcstm_test_print(case_num, case_name,case_disc, exp_res, final_res);
}                                 


void mbcstm_set_warm_sync_on_standby()
{
  uns32 svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_set_warm_sync_on_standby";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_get_set_purposes( svc_index, ssn_index,MBCSTM_SVC_INS2,
                               NCS_MBCSV_OP_OBJ_SET,
                               NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF,
                               TRUE,MBCSTM_CB_NO_TEST,1,3,MBCSTM_SSN_ANC2,
                               MBCSTM_ACT_STATE_MULTIPLE_ACTIVE )
      !=  NCSCC_RC_SUCCESS)
    
    {
      final_res = TET_MBCSV_FAIL;
      cases_failed++;
    }
  else
    {
      final_res = TET_MBCSV_PASS;
      cases_passed++;
    }     
  mbcstm_test_print(case_num, case_name,case_disc, exp_res, final_res);
}


void mbcstm_get_warm_sync_on_off()
{
  uns32 svc_index = 1, ssn_index = 1;
  char case_name[] = "mbcstm_get_warm_sync_on_off";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_get_set_purposes( svc_index, ssn_index,MBCSTM_SVC_INS2,
                               NCS_MBCSV_OP_OBJ_GET,
                               NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF,
                               TRUE,MBCSTM_CB_NO_TEST,1,3,MBCSTM_SSN_ANC2,
                               MBCSTM_ACT_STATE_MULTIPLE_ACTIVE )
      !=  NCSCC_RC_SUCCESS)
    
    {
      final_res = TET_MBCSV_FAIL;
      cases_failed++;
    }
  else
    {
      final_res = TET_MBCSV_PASS;
      cases_passed++;
    }                                        
  mbcstm_test_print(case_num, case_name,case_disc, exp_res, final_res);
}


void mbcstm_set_warm_sync_timer_active()
{
  uns32 svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_set_warm_sync_timer_active";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_get_set_purposes( svc_index, ssn_index,MBCSTM_SVC_INS1,
                               NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC,
                               1000,MBCSTM_CB_NO_TEST,1,3,MBCSTM_SSN_ANC2,
                               MBCSTM_ACT_STATE_MULTIPLE_ACTIVE ) 
      !=  NCSCC_RC_SUCCESS)
    {
      final_res = TET_MBCSV_FAIL;
      cases_failed++;
    }
  else
    {
      final_res = TET_MBCSV_PASS;
      cases_passed++;
    }
  
  mbcstm_test_print(case_num, case_name,case_disc, exp_res, final_res);
}


void mbcstm_set_warm_sync_timer_standby()
{
  uns32 svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_set_warm_timer_standby";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_get_set_purposes( svc_index, ssn_index,MBCSTM_SVC_INS2,
                               NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC,
                               1000,MBCSTM_CB_NO_TEST,1,3,MBCSTM_SSN_ANC2,
                               MBCSTM_ACT_STATE_MULTIPLE_ACTIVE)
      !=  NCSCC_RC_SUCCESS)
    {
      final_res = TET_MBCSV_FAIL;
      cases_failed++;
    }
  else
    {
      final_res = TET_MBCSV_PASS;
      cases_passed++;
    }                                          
  mbcstm_test_print(case_num, case_name,case_disc, exp_res, final_res);
}



void mbcstm_get_warm_sync_timer()
{
  uns32 svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_get_warm_sync_timer";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_get_set_purposes( svc_index, ssn_index,MBCSTM_SVC_INS2,
                               NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_TMR_WSYNC,
                               1000,MBCSTM_CB_NO_TEST,1,3,MBCSTM_SSN_ANC2,
                               MBCSTM_ACT_STATE_MULTIPLE_ACTIVE)
      !=  NCSCC_RC_SUCCESS)
    
    {
      final_res = TET_MBCSV_FAIL;
      cases_failed++;
    }
  else
    {
      final_res = TET_MBCSV_PASS;
      cases_passed++;
    }                                                    
  mbcstm_test_print(case_num, case_name,case_disc, exp_res, final_res);
}

