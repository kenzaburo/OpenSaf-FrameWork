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
 * Author(s): Ericsson AB
 *
 */

#ifndef LGS_CKPT_H
#define LGS_CKPT_H

#include "lgs.h"

#define LGS_MBCSV_VERSION 1
#define LGS_MBCSV_VERSION_MIN 1

/* Checkpoint message types(Used as 'reotype' w.r.t mbcsv)  */

/* Checkpoint update messages are processed similar to lgsv internal
 * messages handling, so the types are defined such that the values 
 * can be mapped straight with lgsv message types
 */

typedef enum {
	LGS_CKPT_CLIENT_INITIALIZE = 0,
	LGS_CKPT_CLIENT_FINALIZE = 1,
	LGS_CKPT_CLIENT_DOWN = 2,
	LGS_CKPT_LOG_WRITE = 3,
	LGS_CKPT_OPEN_STREAM = 4,
	LGS_CKPT_CLOSE_STREAM = 5,
	LGS_CKPT_CFG_STREAM = 6,
	LGS_CKPT_MSG_MAX
} lgsv_ckpt_msg_type_t;

/* Structures for Checkpoint data(to be replicated at the standby) */

/* Initialize checkpoint record, used in cold/async checkpoint updates */
typedef struct {
	uns32 client_id;	/* Client Id at Active */
	MDS_DEST mds_dest;	/* Handy when an LGA instance goes away */
	lgs_stream_list_t *stream_list;
} lgs_ckpt_initialize_msg_t;

/* Finalize checkpoint record, used in cold/async checkpoint updates */
typedef struct {
	uns32 client_id;	/* Client Id at Active */
} lgs_ckpt_finalize_msg_t;

typedef struct {
	uns32 streamId;
	uns32 recordId;
	uns32 curFileSize;
	char *logFileCurrent;
} lgs_ckpt_write_log_t;

typedef struct {
	uns32 streamId;
	uns32 clientId;
	/* correspond to SaLogFileCreateAttributes */
	char *logFile;		/* logfile name */
	char *logPath;		/* logfile path */
	uns64 maxFileSize;	/* max file size configurable */
	int32 maxLogRecordSize;	/* constant size of the records */
	int32 logFileFullAction;
	int32 maxFilesRotated;	/* max number of rotation files configurable */
	char *fileFmt;
	/* end correspond to SaLogFileCreateAttributes */
	char *logStreamName;
	uns64 creationTimeStamp;
	uns32 numOpeners;
	char *logFileCurrent;
	logStreamTypeT streamType;
	uint32_t logRecordId;	/* log record indentifier increased for each record */
} lgs_ckpt_stream_open_t;

typedef struct {
	uns32 streamId;
	uns32 clientId;
} lgs_ckpt_stream_close_t;

/* Checkpoint message containing lgs data of a particular type.
 * Used during cold and async updates.
 */
typedef struct {
	lgsv_ckpt_msg_type_t ckpt_rec_type;	/* Type of lgs data carried in this checkpoint */
	uns32 num_ckpt_records;	/* =1 for async updates,>=1 for cold sync */
	uns32 data_len;		/* Total length of encoded checkpoint data of this type */
} lgsv_ckpt_header_t;

typedef struct {
	char *name;
	char *fileName;
	char *pathName;
	SaUint64T maxLogFileSize;
	SaUint32T fixedLogRecordSize;
	SaBoolT haProperty;	/* app log stream only */
	SaLogFileFullActionT logFullAction;
	SaUint32T logFullHaltThreshold;	/* !app log stream */
	SaUint32T maxFilesRotated;
	char *logFileFormat;
	SaUint32T severityFilter;
	char *logFileCurrent;
} lgs_ckpt_stream_cfg_t;

typedef struct {
	lgsv_ckpt_header_t header;
	union {
		lgs_ckpt_initialize_msg_t initialize_client;
		lgs_ckpt_finalize_msg_t finalize_client;
		lgs_ckpt_write_log_t write_log;
		MDS_DEST agent_dest;
		lgs_ckpt_stream_open_t stream_open;
		lgs_ckpt_stream_close_t stream_close;
		lgs_ckpt_stream_cfg_t stream_cfg;
	} ckpt_rec;
} lgsv_ckpt_msg_t;

typedef uns32 (*LGS_CKPT_HDLR) (lgs_cb_t *cb, lgsv_ckpt_msg_t *data);
uns32 lgs_mbcsv_init(lgs_cb_t *lgs_cb);
uns32 lgs_mbcsv_change_HA_state(lgs_cb_t *cb);
uns32 lgs_mbcsv_dispatch(NCS_MBCSV_HDL mbcsv_hdl);
uns32 lgs_ckpt_send_async(lgs_cb_t *cb, lgsv_ckpt_msg_t *ckpt_rec, uns32 action);

#endif   /* !LGSV_CKPT_H */
