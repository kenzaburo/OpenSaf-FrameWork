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

#ifndef GLND_API_H
#define GLND_API_H

EXTERN_C uns32 glnd_se_lib_create(uns8 pool_id);
EXTERN_C uns32 glnd_se_lib_destroy(void);
EXTERN_C void glnd_process_mbx(GLND_CB *cb, SYSF_MBX *mbx);

EXTERN_C GLND_RESOURCE_INFO *glnd_resource_node_find(GLND_CB *glnd_cb, SaLckResourceIdT res_id);

#endif
