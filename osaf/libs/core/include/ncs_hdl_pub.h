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

  DESCRIPTION: Abstractions and APIs for NCS_HDL service, an access-safe,
  use-safe means to fetch and use fleeting and/or volitile objects.

  Abstractions and APIs for the 'Local Persistence Guard' service which is
  also an access-safe, use-safe mens to fetch and use a fleeting object
  that happens to live off another object. It provides that same type of
  protection as the Handle Manager, but does not require the de-coupling
  of handle and object.
 

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#ifndef NCS_HDL_PUB_H
#define NCS_HDL_PUB_H

#include "ncs_svd.h"

#ifdef  __cplusplus
extern "C" {
#endif

/************************************************************************/
/* Pre-ordained Pool ID Names                                           */
/************************************************************************/
#define NCSHM_POOL_LOCAL   0	/* ment for non-shared, local handles      */
#define NCSHM_POOL_1       1
#define NCSHM_POOL_2       2
#define NCSHM_POOL_3       3
#define NCSHM_POOL_4       4
#define NCSHM_POOL_5       5
#define NCSHM_POOL_6       6
#define NCSHM_POOL_7       7
#define NCSHM_POOL_8       8

/* Public Pool IDs to be used */
	typedef enum {
		NCS_HM_POOL_ID_COMMON = NCSHM_POOL_LOCAL,	/* Pool 0(for LEAP/MDS services) */
		NCS_HM_POOL_ID_NCS,	/* Pool 1(for NCS services) */
		NCS_HM_POOL_ID_APS,	/* Pool 2(for APS subsystems) */
		NCS_HM_POOL_ID_EXTERNAL1,	/* Pool 3(for applications) */
		NCS_HM_POOL_ID_EXTERNAL2,	/* Pool 4(for applications) */
		NCS_HM_POOL_ID_EXTERNAL3,	/* Pool 5(for applications) */
		NCS_HM_POOL_ID_RESERVED1,	/* Pool 6(reserved) */
		NCS_HM_POOL_ID_RESERVED2,	/* Pool 7(reserved) */
		NCS_HM_POOL_ID_RESERVED3,	/* Pool 8(reserved) */
		NCS_HM_POOL_ID_MAX	/* Invalid Pool ID */
	} NCS_HM_POOL_ID;

/***************************************************************************
 *
 * P u b l i c    H a n d l e  M g r    A P I s
 *
 ***************************************************************************/

	EXTERN_C uns32 ncshm_init(void);

	EXTERN_C void ncshm_delete(void);

/* p_id is the pool ID from where the handles would be created from. */
	EXTERN_C uns32 ncshm_create_hdl(uns8 p_id, NCS_SERVICE_ID id, NCSCONTEXT save);

	EXTERN_C uns32 ncshm_declare_hdl(uns32 hdl, NCS_SERVICE_ID id, NCSCONTEXT save);

	EXTERN_C NCSCONTEXT ncshm_destroy_hdl(NCS_SERVICE_ID id, uns32 hdl);

	EXTERN_C NCSCONTEXT ncshm_take_hdl(NCS_SERVICE_ID id, uns32 hdl);

	EXTERN_C void ncshm_give_hdl(uns32 hdl);

/************************************************************************/
/* NCSLPG_OBJ - this structure is embedded in known, persistent thing    */
/************************************************************************/

	typedef struct ncslpg_obj {
		NCS_BOOL open;	/* Is the object (still) open/available     */
		uns8 inhere;	/* use-count of clients 'inside' object now */

	} NCSLPG_OBJ;		/* Local Persistence Guard */

/***************************************************************************
 *
 * P u b l i c    L o c a l  P e r s i s t e n c e  G u a r d   A P I s
 *
 ***************************************************************************/

	EXTERN_C NCS_BOOL ncslpg_take(NCSLPG_OBJ *pg);
	EXTERN_C uns32 ncslpg_give(NCSLPG_OBJ *pg, uns32 ret);
	EXTERN_C uns32 ncslpg_create(NCSLPG_OBJ *pg);
	EXTERN_C NCS_BOOL ncslpg_destroy(NCSLPG_OBJ *pg);

#ifdef  __cplusplus
}
#endif

#endif
