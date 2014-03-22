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

  This module contains the extern declaration for string arrays 
  provided by avsv_logstr.c
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVSV_LOGSTR_H
#define AVSV_LOGSTR_H

#if (NCS_DTS == 1)
EXTERN_C uns32 avsv_log_str_lib_req(NCS_LIB_REQ_INFO *req_info);
EXTERN_C const NCSFL_STR avsv_seapi_set[];
EXTERN_C const NCSFL_STR avsv_mds_set[];
EXTERN_C const NCSFL_STR avsv_srm_set[];
EXTERN_C const NCSFL_STR avsv_edu_set[];
EXTERN_C const NCSFL_STR avsv_lock_set[];
EXTERN_C const NCSFL_STR avsv_cb_set[];
EXTERN_C const NCSFL_STR avsv_cbk_set[];
EXTERN_C const NCSFL_STR avsv_mbx_set[];
EXTERN_C const NCSFL_STR avsv_task_set[];
EXTERN_C const NCSFL_STR avsv_pat_set[];
#endif   /* (NCS_DTS == 1) */

#endif   /* AVSV_LOGSTR_H */
