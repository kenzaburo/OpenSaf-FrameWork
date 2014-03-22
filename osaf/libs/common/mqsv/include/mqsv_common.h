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
  
  MQSv event definitions.
    
******************************************************************************
*/
#ifndef MQSV_COMMON_H
#define MQSV_COMMON_H

#define MQSV_LITTLE_ENDIAN 0
#define MQSV_BIG_ENDIAN    1

#define MQSV_DSEND_EVENT 1	/*Direct Send Event */
#define MQSV_NOT_DSEND_EVENT 0	/*Not a Direct Send Event */

#define m_MQSV_REVERSE_ENDIAN_LL(p8, endianness) (        \
   (!endianness ? (                            \
      ((uns64)((uns8*)(p8))[0] <<56) |        \
      ((uns64)((uns8*)(p8))[1] <<48) |        \
      ((uns64)((uns8*)(p8))[2] <<40) |        \
      ((uns64)((uns8*)(p8))[3] <<32) |        \
      ((uns64)((uns8*)(p8))[4] <<24) |        \
      ((uns64)((uns8*)(p8))[5] <<16) |        \
      ((uns64)((uns8*)(p8))[6] <<8 ) |        \
      ((uns64)((uns8*)(p8))[7]     )          \
     )                                        \
    :(                                        \
      ((uns64)((uns8*)(p8))[7] <<56) |        \
      ((uns64)((uns8*)(p8))[6] <<48) |        \
      ((uns64)((uns8*)(p8))[5] <<40) |        \
      ((uns64)((uns8*)(p8))[4] <<32) |        \
      ((uns64)((uns8*)(p8))[3] <<24) |        \
      ((uns64)((uns8*)(p8))[2] <<16) |        \
      ((uns64)((uns8*)(p8))[1] <<8 ) |        \
      ((uns64)((uns8*)(p8))[0]     )          \
     )                                        \
   ))

#define m_MQSV_REVERSE_ENDIAN_L(p8, endianness) (        \
   (!endianness ? (                            \
      ((uns32)((uns8*)(p8))[0] <<24) |        \
      ((uns32)((uns8*)(p8))[1] <<16) |        \
      ((uns32)((uns8*)(p8))[2] <<8 ) |        \
      ((uns32)((uns8*)(p8))[3]     )          \
     )                                        \
    :(                                        \
      ((uns32)((uns8*)(p8))[3] <<24) |        \
      ((uns32)((uns8*)(p8))[2] <<16) |        \
      ((uns32)((uns8*)(p8))[1] <<8 ) |        \
      ((uns32)((uns8*)(p8))[0]     )          \
     )                                        \
   ))

#define m_MQSV_REVERSE_ENDIAN_S(p8, endianness) (        \
   (!endianness ? (                            \
      ((uns16)((uns8*)(p8))[0] <<8 ) |        \
      ((uns16)((uns8*)(p8))[1]     )          \
     )                                        \
    :(                                        \
      ((uns16)((uns8*)(p8))[1] <<8 ) |        \
      ((uns16)((uns8*)(p8))[0]     )          \
     )                                        \
   ))

EXTERN_C uns32 machineEndianness(void);
EXTERN_C uns32 mqsv_listenerq_msg_send(SaMsgQueueHandleT listenerhandle);
EXTERN_C NCS_PHY_SLOT_ID mqsv_get_phy_slot_id(MDS_DEST dest);
typedef unsigned int MSG_FRMT_VER;

#define m_MQSV_CONVERT_SATIME_TEN_MILLI_SEC(t)      (t)/(10000000)	/* 10^7 */
/* New categories Specific to MQSV Logging Used by MQA/MQD/MQND */
#define NCSFL_LC_MQSV_INIT       0x00008000	/* MQSV Liblifecycle logging */
#define NCSFL_LC_MQSV_Q_MGMT     0x00004000	/* MQSV Send Receive flow logging */
#define NCSFL_LC_MQSV_QGRP_MGMT  0x00002000	/* MQSV Group Apis flows */
#define NCSFL_LC_MQSV_SEND_RCV   0x00001000	/* MQSV New Logs */

/* DTSv versioning support */
#define MQSV_LOG_VERSION 4

#endif
