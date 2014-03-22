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

#ifndef CPD_SBEDU_H
#define CPD_SBEDU_H

#define FUNC_NAME(DS) cpsv_edp_##DS##_info

uns32 FUNC_NAME(CPD_NODE_REF_INFO) ();
uns32 FUNC_NAME(CPD_A2S_CKPT_CREATE) ();
uns32 FUNC_NAME(CPD_A2S_CKPT_UNLINK) ();
uns32 FUNC_NAME(CPSV_CKPT_RDSET) ();
uns32 FUNC_NAME(CPSV_CKPT_DEST_INFO) ();
uns32 FUNC_NAME(CPD_A2S_CKPT_USR_INFO) ();

EXTERN_C uns32 cpd_sb_proc_ckpt_create(CPD_CB *cb, CPD_MBCSV_MSG *msg);
EXTERN_C uns32 cpd_process_sb_msg(CPD_CB *cb, CPD_MBCSV_MSG *msg);

EXTERN_C uns32 cpd_a2s_ckpt_create(CPD_CB *cb, CPD_CKPT_INFO_NODE *ckpt_node);
EXTERN_C uns32 cpd_mbcsv_init(CPD_CB *cb);
EXTERN_C uns32 cpd_mbcsv_open(CPD_CB *cb);
EXTERN_C uns32 cpd_mbcsv_selobj_get(CPD_CB *cb);
EXTERN_C uns32 cpd_mbcsv_async_update(CPD_CB *cb, CPD_MBCSV_MSG *msg);
EXTERN_C uns32 cpd_mbcsv_register(CPD_CB *cb);
EXTERN_C uns32 cpd_mbcsv_callback(NCS_MBCSV_CB_ARG *arg);
EXTERN_C uns32 cpd_mbcsv_finalize(CPD_CB *cb);
EXTERN_C uns32 cpd_mbcsv_chgrole(CPD_CB *cb);

#endif
