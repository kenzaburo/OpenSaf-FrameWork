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

  DESCRIPTION: The master include for all DTA and user *.C files.
 
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#ifndef DTA_H
#define DTA_H

/* Get general definitions.....*/

#include <ncsgl_defs.h>

/* Get target's suite of header files...*/


#include "dta_tgt.h"

/* From /base/common/inc */

#include "ncs_svd.h"
#include "usrbuf.h"
#include "ncsft.h"
#include "ncs_ubaid.h"
#include "ncsencdec_pub.h"
#include "ncs_stack.h"
#include "ncs_log.h"
#include "ncs_lib.h"

/* From targsvcs/common/inc */

#include "mds_papi.h"

/* From /base/products/rms/inc */

#include "dta_opt.h"
#include "dtsv_msg.h"
#include "dta_mem.h"
#include "dta_papi.h"
#include "dta_api.h"
#include "dta_pvt.h"
#include "dtsv_mem.h"
#include "dta_dl_api.h"

#endif
