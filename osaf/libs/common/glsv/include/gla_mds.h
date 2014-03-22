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

  GLA - GLND communication related definitions.
  
******************************************************************************
*/

#ifndef GLA_MDS_H
#define GLA_MDS_H

/*** Extern function declarations ***/

EXTERN_C uns32 gla_mds_register(struct gla_cb_tag *cb);

EXTERN_C void gla_mds_unregister(struct gla_cb_tag *cb);

EXTERN_C uns32 gla_mds_msg_sync_send(struct gla_cb_tag *cb, GLSV_GLND_EVT *i_evt, GLSV_GLA_EVT **o_evt, uns32 timeout);

EXTERN_C uns32 gla_mds_msg_async_send(struct gla_cb_tag *cb, GLSV_GLND_EVT *i_evt);

EXTERN_C void glsv_gla_evt_free(GLSV_GLA_EVT *gla_evt);

EXTERN_C uns32 gla_agent_register(GLA_CB *cb);
EXTERN_C uns32 gla_agent_unregister(GLA_CB *cb);
/*****************************************************************************/

#define GLA_PVT_SUBPART_VERSION 1

/********************Service Sub part Versions*********************************/
/* GLA - GLND */
#define GLA_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT 1
#define GLA_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT 1
#define GLA_WRT_GLND_SUBPART_VER_RANGE \
            (GLA_WRT_GLND_SUBPART_VER_AT_MAX_MSG_FMT - \
        GLA_WRT_GLND_SUBPART_VER_AT_MIN_MSG_FMT + 1)

/********************************************************************************/

#endif   /* !GLA_MDS_H */
