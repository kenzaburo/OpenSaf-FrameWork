/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.z
 *
 * Author(s): GoAhead Software
 *
 */
#ifndef DTM_INTRA_TRANS_H
#define DTM_INTRA_TRANS_H

uns32 dtm_intranode_process_data_msg(uns8 *buffer, uns32 dst_pid, uns16 len);
uns32 dtm_process_rcv_internode_data_msg(uns8 *buffer, uns32 dst_pid, uns16 len);

uns32 dtm_intranode_send_msg(uns16 len, uns8 *buffer, DTM_INTRANODE_PID_INFO * pid_node);

uns32 dtm_intranode_process_rcv_data_msg(uns8 *buffer, uns32 dst_pid, uns16 len);

uns32 dtm_intranode_process_pollout(int fd);

uns32 dtm_intranode_set_poll_fdlist(int fd, uns16 events);

#endif
