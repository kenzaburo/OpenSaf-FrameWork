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

#ifndef GLD_MDS_H
#define GLD_MDS_H

/*****************************************************************************/

#define SVC_SUBPART_VER uns32
#define GLD_PVT_SUBPART_VERSION 1

/********************Service Sub part Versions*********************************/
/* GLD - GLND */
#define GLD_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT 1
#define GLD_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT 1
#define GLD_WRT_GLND_SUBPART_VER_RANGE \
            (GLD_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT - \
        GLD_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT + 1)
/********************************************************************/

uns32 gld_mds_callback(NCSMDS_CALLBACK_INFO *info);
uns32 gld_mds_svc_evt(GLSV_GLD_CB *cb, MDS_CALLBACK_SVC_EVENT_INFO *rcv_info);
uns32 gld_mds_rcv(GLSV_GLD_CB *cb, MDS_CALLBACK_RECEIVE_INFO *rcv_info);
uns32 gld_mds_cpy(GLSV_GLD_CB *cb, MDS_CALLBACK_COPY_INFO *cpy_info);
uns32 gld_mds_vdest_create(GLSV_GLD_CB *cb);
uns32 gld_mds_vdest_destroy(GLSV_GLD_CB *cb);
uns32 gld_mds_init(GLSV_GLD_CB *cb);
uns32 gld_mds_shut(GLSV_GLD_CB *cb);
uns32 gld_mds_change_role(GLSV_GLD_CB *cb, V_DEST_RL role);

#endif
