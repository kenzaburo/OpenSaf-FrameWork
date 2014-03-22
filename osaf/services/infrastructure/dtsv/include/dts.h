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

  DESCRIPTION: The master include for all DTS and user *.C files.
 
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#ifndef DTS_H
#define DTS_H

/* Get general definitions.....*/

#include <ncsgl_defs.h>

/* Get target's suite of header files...*/


#include "dts_tgt.h"

/* From /base/common/inc */

#include "ncs_svd.h"
#include "usrbuf.h"
#include "ncsft.h"
#include "ncs_ubaid.h"
#include "ncsencdec_pub.h"
#include "ncs_stack.h"
#include "ncs_log.h"
#include "ncs_lib.h"
#include "ncs_util.h"

#include "ncs_mda_pvt.h"
#include "ncs_mda_papi.h"
#include "mds_papi.h"
/*Smik - included ncs_edu_pub.h */
#include "ncs_edu_pub.h"
/* From targsvcs/common/inc */

#include "mds_papi.h"

#include "saAis.h"

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "mbcsv_papi.h"
#include "dts_opt.h"
#include "dtsv_msg.h"
#include "dts_papi.h"
#include "dta_api.h"
#include "dta_papi.h"
#include "dts_log.h"
#include "dts_api.h"
#include "dts_mem.h"
#include "dts_def.h"
#include "sysf_log.h"

/*#include "dts_ckp.h"*/
#include "dts_ckpt_msg.h"

#include "dts_pvt.h"
#include "dtsv_mem.h"
#include "dts_dl_api.h"
#include "dts_amf.h"

#include "dts_ckp.h"
#include "dts_ckpt_updt.h"
#include "dts_ckpt_edu.h"

#endif
