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

 MODULE NAME: ncs_edu.h

 REVISION HISTORY:

 Date     Version  Name          Description
 -------- -------  ------------  --------------------------------------------
 27-11-03 1.00A    MCG(SAI)      Original

 ..............................................................................

 DESCRIPTION:
  This module contains definitions/prototypes required for the EDU library.

 ******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef NCS_EDU_H
#define NCS_EDU_H

#include "ncs_edu_pub.h"

#define m_NCS_EDU_ASSERT        assert(0)

/* EDU Protocol Marker used in EDU header */
#define EDU_PROT_MARKER 0xed

/* Bitmap for EDU-options in the EDU-header */
#define EDU_UBUF_USE_DIFF_EDP       0x01
#define EDU_ALLOW_PARTIAL_DECODE    0x02	/* Allows one-time partial 
						   decode of USRBUF */

/* TLV Header size(for Type and Length) */
#define EDU_TLV_HDR_SIZE            3

/* EDU internal operation types. */
#if (NCS_EDU_VERBOSE_PRINT == 1)
#define EDP_OP_TYPE_PP      0xffff0001
#endif
#define EDP_OP_TYPE_ADMIN   0xffff0002

/* Administrative operations on the EDU programs. Responsibility of EDU
   only. */
typedef enum {
	NCS_EDU_ADMIN_OP_TYPE_COMPILE,
	NCS_EDU_ADMIN_OP_TYPE_GET_ATTRB,
	NCS_EDU_ADMIN_OP_TYPE_GET_LL_NEXT_OFFSET,
	NCS_EDU_ADMIN_OP_TYPE_MAX
} NCS_EDU_ADMIN_OP_TYPE;

typedef struct ncs_edu_admin_compile_info_tag {
	uns32 i_dummy;
	uns32 *o_retval;
} NCS_EDU_ADMIN_COMPILE_INFO;

typedef struct ncs_edu_admin_get_attrb_info_tag {
	uns32 *o_attrb;		/* Storage of this (most likely) to be given by EDU(not application) */
} NCS_EDU_ADMIN_GET_ATTRB_INFO;

typedef struct ncs_edu_admin_get_ll_next_offset_tag {
	uns32 *o_next_offset;	/* Storage, the responsibility of EDU */
} NCS_EDU_ADMIN_GET_LL_NEXT_OFFSET;

typedef struct ncs_edu_admin_op_tag {
	NCS_EDU_ADMIN_OP_TYPE adm_op_type;
	union {
		NCS_EDU_ADMIN_COMPILE_INFO compile_info;
		NCS_EDU_ADMIN_GET_ATTRB_INFO get_attrb;	/* Get attributes of program,
							   like PTR | ARR | LINKEDLIST, etc. */
		NCS_EDU_ADMIN_GET_LL_NEXT_OFFSET get_ll_offset;	/* Location of "next" pointer of
								   linked list. */
	} info;
} NCS_EDU_ADMIN_OP_INFO;

/********************* EDU Internal Macros *********************/
#define m_NCS_EDU_TKN_INIT(edu_tkn) ncs_edu_tkn_init(edu_tkn)

#define m_NCS_EDU_TKN_FLUSH(edu_tkn) ncs_edu_tkn_flush(edu_tkn)

#define m_NCS_EDU_RUN_EDP(edu_hdl, edu_tkn, rule, edp, ptr, dcnt, buf_env, optype, o_err) \
    ncs_edu_run_edp(edu_hdl, edu_tkn, rule, edp, ptr, dcnt, buf_env, optype, o_err)

#define m_NCS_EDU_EXEC_RULE(edu_hdl, edu_tkn, hdl_node, rule, ptr, ptr_data_len, buf_env, optype, o_err) \
    ncs_edu_exec_rule(edu_hdl, edu_tkn, hdl_node, rule, ptr, ptr_data_len, buf_env, optype, o_err)

#define m_NCS_EDU_RUN_TEST_LL_RULE(rule, ptr, optype, o_err) \
    ncs_edu_run_test_ll_rule(rule, ptr, optype, o_err)

#define m_NCS_EDU_RUN_TEST_CONDITION(edu_hdl, rule, ptr, buf_env, optype, o_err) \
    ncs_edu_run_test_condition(edu_hdl, rule, ptr, buf_env, optype, o_err)

#define m_NCS_EDU_RUN_VER_GE(edu_hdl, rule, ptr, buf_env, optype, o_err) \
    ncs_edu_run_version_ge(edu_hdl, rule, ptr, buf_env, optype, o_err)

#define m_NCS_EDU_RUN_VER_USR(edu_hdl, rule, ptr, buf_env, optype, o_err) \
    ncs_edu_run_version_usr(edu_hdl, rule, ptr, buf_env, optype, o_err)

#define m_NCS_EDU_GET_REFCOUNT_OF_TESTABLE_FIELD(inst_store, rule) \
    ncs_edu_get_refcount_of_testable_field(inst_store, rule)
/********************* EDU Internal Macros *********************/

/************ EDU internal macro-related functions. ************/
EXTERN_C void ncs_edu_tkn_init(EDU_TKN *edu_tkn);

EXTERN_C void ncs_edu_tkn_flush(EDU_TKN *edu_tkn);

EXTERN_C uns32 ncs_edu_run_edcompile_on_edp(EDU_HDL *edu_hdl, EDU_HDL_NODE *hdl_node, EDU_ERR *o_err);

EXTERN_C uns32 ncs_edu_perform_pp_op(EDU_HDL *edu_hdl,
				     EDU_PROG_HANDLER edp,
				     EDU_BUF_ENV *buf_env, EDP_OP_TYPE op,
				     EDU_ERR *o_err, uns8 var_cnt, int *var_array);

EXTERN_C uns32 ncs_edu_perform_enc_op(EDU_HDL *edu_hdl, EDU_PROG_HANDLER edp,
				      EDU_BUF_ENV *buf_env, uns32 *cnt, NCSCONTEXT arg,
				      EDU_ERR *o_err, uns8 var_cnt, int *var_array);

EXTERN_C uns32 ncs_edu_perform_dec_op(EDU_HDL *edu_hdl, EDU_PROG_HANDLER edp,
				      EDU_BUF_ENV *buf_env, uns32 *cnt, NCSCONTEXT arg,
				      EDU_ERR *o_err, uns8 var_cnt, int *var_array);

EXTERN_C uns32
ncs_edu_run_edp(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn, EDU_INST_SET *rule,
		EDU_PROG_HANDLER edp, NCSCONTEXT ptr, uns32 *cnt,
		EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err);

EXTERN_C int
ncs_edu_exec_rule(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
		  EDU_HDL_NODE *hdl_node, EDU_INST_SET *rule,
		  NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err);

EXTERN_C EDU_LABEL
ncs_edu_run_rules_for_enc(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			  EDU_HDL_NODE *hdl_node, EDU_INST_SET *prog,
			  NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err, int instr_count);

EXTERN_C EDU_LABEL
ncs_edu_run_rules_for_dec(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			  EDU_HDL_NODE *hdl_node, EDU_INST_SET *prog,
			  NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err, int instr_count);

EXTERN_C EDU_LABEL
ncs_edu_run_rules_for_pp(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			 EDU_HDL_NODE *hdl_node, EDU_INST_SET *prog,
			 NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err, int instr_count);

EXTERN_C uns32
ncs_edu_run_rules_for_compile(EDU_HDL *edu_hdl, EDU_HDL_NODE *hdl_node,
			      EDU_INST_SET *prog, NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_ERR *o_err, int instr_count);

EXTERN_C uns32
ncs_edu_perform_exec_action(EDU_HDL *edu_hdl, EDU_TKN *edu_tkn,
			    EDU_HDL_NODE *hdl_node, EDU_INST_SET *rule,
			    EDP_OP_TYPE optype, NCSCONTEXT ptr,
			    uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err);

EXTERN_C uns32
ncs_edu_perform_exec_action_on_non_ptr(EDU_HDL *edu_hdl,
				       EDU_TKN *edu_tkn, EDU_HDL_NODE *hdl_node,
				       EDU_INST_SET *rule, EDP_OP_TYPE optype,
				       NCSCONTEXT ptr, uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err);

EXTERN_C uns32
ncs_edu_prfm_enc_on_non_ptr(EDU_HDL *edu_hdl,
			    EDU_TKN *edu_tkn, EDU_HDL_NODE *hdl_node,
			    EDU_INST_SET *rule, NCSCONTEXT ptr,
			    uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err);

EXTERN_C uns32
ncs_edu_prfm_dec_on_non_ptr(EDU_HDL *edu_hdl,
			    EDU_TKN *edu_tkn, EDU_HDL_NODE *hdl_node,
			    EDU_INST_SET *rule, NCSCONTEXT ptr,
			    uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err);

#if (NCS_EDU_VERBOSE_PRINT == 1)
EXTERN_C uns32
ncs_edu_prfm_pp_on_non_ptr(EDU_HDL *edu_hdl,
			   EDU_TKN *edu_tkn, EDU_HDL_NODE *hdl_node,
			   EDU_INST_SET *rule, NCSCONTEXT ptr,
			   uns32 *ptr_data_len, EDU_BUF_ENV *buf_env, EDU_ERR *o_err);
#endif

EXTERN_C NCS_BOOL ncs_edu_is_edp_builtin(EDU_PROG_HANDLER prog);

EXTERN_C NCS_BOOL ncs_edu_return_builtin_edp_size(EDU_PROG_HANDLER prog, uns32 *o_size);

EXTERN_C EDU_LABEL ncs_edu_run_test_ll_rule(EDU_INST_SET *rule, NCSCONTEXT ptr, EDP_OP_TYPE optype, EDU_ERR *o_err);

EXTERN_C int
ncs_edu_run_test_condition(EDU_HDL *edu_hdl,
			   EDU_INST_SET *rule, NCSCONTEXT ptr,
			   EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err);

EXTERN_C int
ncs_edu_run_version_ge(EDU_HDL *edu_hdl,
		       EDU_INST_SET *rule, NCSCONTEXT ptr, EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err);

EXTERN_C int
ncs_edu_run_version_usr(EDU_HDL *edu_hdl,
			EDU_INST_SET *rule, NCSCONTEXT ptr, EDU_BUF_ENV *buf_env, EDP_OP_TYPE optype, EDU_ERR *o_err);
EXTERN_C uns32
ncs_edu_validate_and_gen_test_instr_rec_list(EDP_TEST_INSTR_REC **head,
					     EDU_INST_SET *rules_head, int instr_count, EDU_ERR *o_err);

EXTERN_C void ncs_edu_free_test_instr_rec_list(EDP_TEST_INSTR_REC *head);

EXTERN_C uns32 ncs_edu_get_refcount_of_testable_field(EDP_TEST_INSTR_REC *inst_store, EDU_INST_SET *rule);

EXTERN_C void ncs_edu_log_msg(char *string);

EXTERN_C void ncs_edu_skip_space(EDU_TLV_ENV *tlv_env, uns32 cnt);

EXTERN_C uns32 ncs_encode_tlv_8bit(uns8 **stream, uns32 val);

EXTERN_C uns32 ncs_encode_tlv_16bit(uns8 **stream, uns32 val);

EXTERN_C uns32 ncs_encode_tlv_32bit(uns8 **stream, uns32 val);

EXTERN_C uns32 ncs_encode_tlv_64bit(uns8 **stream, uns64 val);

EXTERN_C uns32 ncs_encode_tlv_n_32bit(uns8 **stream, uns32 *val_ptr, uns16 n_count);

EXTERN_C uns32 ncs_encode_tlv_n_16bit(uns8 **stream, uns16 *val_ptr, uns16 n_count);

EXTERN_C uns32 ncs_encode_tlv_n_octets(uns8 **stream, uns8 *val, uns16 count);

EXTERN_C uns32 ncs_decode_tlv_32bit(uns8 **stream);

EXTERN_C uns64 ncs_decode_tlv_64bit(uns8 **stream);

EXTERN_C uns16 ncs_decode_tlv_16bit(uns8 **stream);

EXTERN_C uns16 ncs_decode_tlv_n_32bit(uns8 **stream, uns32 *dest);

EXTERN_C uns16 ncs_decode_tlv_n_16bit(uns8 **stream, uns16 *dest);

EXTERN_C uns8 ncs_decode_tlv_8bit(uns8 **stream);

EXTERN_C uns8 *ncs_decode_tlv_n_octets(uns8 *src, uns8 *dest, uns32 count);

EXTERN_C uns8 *ncs_copy_tlv_n_octets(uns8 *src, uns8 *dest, uns32 count);

/************ EDU internal macro-related functions. ************/

#endif   /* ifndef NCS_EDU_H */
