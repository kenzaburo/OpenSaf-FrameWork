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
#ifndef DTM_INTRA_DISC_H
#define DTM_INTRA_DISC_H

#define DTM_INTRANODE_MSG_SIZE      1400
#define DTM_INTERNODE_MSG_SIZE      DTM_INTRANODE_MSG_SIZE

/* 2 -len(0), 4 - iden(2), 1- ver(6), 1-msg type(7), 4- server_type (8),
			4 -server lower(12), 4 -server lower(16), 8 -ref(20), 4 - nodeid(28),
			4 - pid(32) */
#define DTM_LIB_UP_MSG_SIZE 34

#define DTM_LIB_DOWN_MSG_SIZE DTM_LIB_UP_MSG_SIZE

#define DTM_LIB_UP_MSG_SIZE_FULL (DTM_LIB_UP_MSG_SIZE+2)

#define DTM_LIB_DOWN_MSG_SIZE_FULL DTM_LIB_UP_MSG_SIZE_FULL

/* 2 -len(0), 4 - iden(2), 1- ver(6), 1-msg type(7), 4- node_id (8),
			8 -ref_val(12) */

#define DTM_LIB_NODE_UP_MSG_SIZE 18

#define DTM_LIB_NODE_DOWN_MSG_SIZE DTM_LIB_NODE_UP_MSG_SIZE

#define DTM_LIB_NODE_UP_MSG_SIZE_FULL (DTM_LIB_NODE_UP_MSG_SIZE+2)

#define DTM_LIB_NODE_DOWN_MSG_SIZE_FULL DTM_LIB_NODE_UP_MSG_SIZE_FULL

typedef enum dtm_svc_install_scope {
	DTM_SVC_INSTALL_SCOPE_PCON = 1,
	DTM_SVC_INSTALL_SCOPE_NODE = 2,
	DTM_SVC_INSTALL_SCOPE_CHASSIS = 3,
	DTM_SVC_INSTALL_SCOPE_NONE = 4,
} DTM_SVC_INSTALL_SCOPE;

typedef DTM_SVC_INSTALL_SCOPE DTM_SVC_SUBSCR_SCOPE;

typedef struct dtm_lib_up_msg {
	uns32 server_type;
	uns32 server_instance_lower;
	uns32 server_instance_upper;
	uns64 ref_val;
	NODE_ID node_id;
	uns32 process_id;
} DTM_LIB_UP_MSG;

typedef DTM_LIB_UP_MSG DTM_LIB_DOWN_MSG;

typedef struct dtm_lib_node_up_msg {
	NODE_ID node_id;
	uns64 ref_val;
} DTM_LIB_NODE_UP_MSG;

typedef DTM_LIB_NODE_UP_MSG DTM_LIB_NODE_DOWN_MSG;

typedef struct dtm_pid_svc_installed_info {
	struct dtm_pid_svc_installed_info *next;
	uns32 server_type;
	uns32 server_instance_lower;
	uns32 server_instance_upper;
	DTM_SVC_INSTALL_SCOPE install_scope;
} DTM_PID_SVC_INSTALLED_INFO;

typedef struct dtm_pid_svc_subscr_info {
	struct dtm_pid_svc_subscr_info *next;
	uns32 server_type;
	uns64 ref_hdl;
} DTM_PID_SVC_SUSBCR_INFO;

typedef struct dtm_intranode_unsent_msgs {
	struct dtm_intranode_unsent_msgs *next;
	uns16 len;
	uns8 *buffer;
} DTM_INTRANODE_UNSENT_MSGS;

typedef struct dtm_intranode_pid_info {
	/* Indexing info */
	NCS_PATRICIA_NODE pid_node;
	NCS_PATRICIA_NODE fd_node;
	/* Explicit key for fast-access */
	uns32 pid;
	int accepted_fd;
	SYSF_MBX mbx;
	int mbx_fd;
	NODE_ID node_id;
	DTM_INTRANODE_UNSENT_MSGS *msgs_hdr;
	DTM_INTRANODE_UNSENT_MSGS *msgs_tail;
	DTM_PID_SVC_INSTALLED_INFO *svc_installed_list;
	DTM_PID_SVC_SUSBCR_INFO *subscr_list;
	/* Message related */
	uns16 bytes_tb_read;
	uns16 buff_total_len;
	uns8 len_buff[2];
	uns8 num_by_read_for_len_buff;
	uns8 *buffer;
} DTM_INTRANODE_PID_INFO;

typedef struct dtm_intranode_node_db {
	struct dtm_intranode_node_db *next;
	NODE_ID node_id;
	char node_name[255];
	SYSF_MBX mbx;
	int fd;
	NCS_PATRICIA_TREE dtm_rem_node_svc_tree;	/* Tree of service install info */
} DTM_INTRANODE_NODE_DB;

extern DTM_INTRANODE_NODE_DB *dtm_node_list_db;

typedef struct dtm_svc_list {
	struct dtm_svc_list *next;
	uns32 server_inst_lower;
	uns32 server_inst_higher;
	NODE_ID node_id;
	uns32 process_id;
	DTM_SVC_INSTALL_SCOPE install_scope;
} DTM_SVC_LIST;

typedef struct dtm_svc_install_info {
	/* Indexing info */
	NCS_PATRICIA_NODE svc_install_node;
	/* Explicit key for fast-access */
	uns32 server_type;
	DTM_SVC_LIST *svc_list;
} DTM_SVC_INSTALL_INFO;

typedef struct dtm_subscriber_list {
	struct dtm_subscriber_list *next;
	uns32 pid;
	int connected_fd;
	uns64 subscr_ref_hdl;
	SYSF_MBX mbx;
	DTM_SVC_SUBSCR_SCOPE subscr_scope;
} DTM_SUBSCRIBER_LIST;

typedef struct dtm_svc_subscr_info {
	/* Indexing info */
	NCS_PATRICIA_NODE svc_subscr_node;
	/* Explicit key for fast-access */
	uns32 server_type;
	DTM_SUBSCRIBER_LIST *subscriber_list;
} DTM_SVC_SUBSCR_INFO;

typedef struct dtm_node_subscr_info {
	struct dtm_node_subscr_info *next;
	NODE_ID node_id;
	uns32 process_id;
	uns64 subtn_ref_val;
} DTM_NODE_SUBSCR_INFO;

extern DTM_NODE_SUBSCR_INFO *dtm_node_subscr_list;

uns32 dtm_intranode_process_pid_msg(uns8 *buffer, int fd);
uns32 dtm_intranode_process_bind_msg(uns8 *buffer, int fd);
uns32 dtm_intranode_process_unbind_msg(uns8 *buffer, int fd);
uns32 dtm_intranode_process_subscribe_msg(uns8 *buff, int fd);
uns32 dtm_intranode_process_unsubscribe_msg(uns8 *buff, int fd);
uns32 dtm_intranode_process_node_subscribe_msg(uns8 *buff, int fd);
uns32 dtm_intranode_process_node_unsubscribe_msg(uns8 *buff, int fd);

uns32 dtm_process_internode_service_up_msg(uns8 *buffer, uns16 len, NODE_ID node_id);
uns32 dtm_process_internode_service_down_msg(uns8 *buffer, uns16 len, NODE_ID node_id);
uns32 dtm_intranode_process_node_up(NODE_ID node_id, char *node_name, SYSF_MBX mbx);
uns32 dtm_intranode_process_node_down(NODE_ID node_id);

uns32 dtm_intranode_process_pid_down(int fd);

DTM_INTRANODE_PID_INFO *dtm_intranode_get_pid_info_using_fd(int fd);
DTM_INTRANODE_PID_INFO *dtm_intranode_get_pid_info_using_pid(uns32 pid);

#endif
