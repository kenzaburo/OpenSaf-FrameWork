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
  AvND Protection Group (PG) Tracking related definitions.
  
******************************************************************************
*/

#ifndef AVND_PG_H
#define AVND_PG_H

/***************************************************************************
 **********  S T R U C T U R E / E N U M  D E F I N I T I O N S  ***********
 ***************************************************************************/

/* pg track key declaration */
typedef struct avnd_pg_trk_key {
	SaAmfHandleT req_hdl;	/* amf handle */
	MDS_DEST mds_dest;	/* mds dest of the prc that started pg tracking */
} AVND_PG_TRK_KEY;

/* pg track info declaration */
typedef struct avnd_pg_trk_info {
	AVND_PG_TRK_KEY key;	/* pg key */
	uns8 flags;		/* track flags */
	NCS_BOOL is_syn;	/* indicates if the appl synchronously waits 
				   for the notification */
	MDS_SYNC_SND_CTXT mds_ctxt;	/* the context for the synchronous api */
} AVND_PG_TRK_INFO;

/* pg track declaration */
typedef struct avnd_pg_trk {
	NCS_DB_LINK_LIST_NODE pg_dll_node;	/* pg dll node */
	AVND_PG_TRK_INFO info;	/* track info */
} AVND_PG_TRK;

/* pg member declaration */
typedef struct avnd_pg_mem {
	NCS_DB_LINK_LIST_NODE pg_dll_node;	/* pg dll node */

	/* member info */
	SaAmfProtectionGroupNotificationT info;	/* comp-name is the index */

	NCS_BOOL mem_exist;	/* Used while processing fail-over message */
} AVND_PG_MEM;

/* pg declaration */
typedef struct avnd_pg {
	NCS_PATRICIA_NODE tree_node;	/* pg tree node (key is csi name) */
	SaNameT csi_name;	/* pg identifier (csi name) */

	NCS_BOOL is_exist;	/* indicates if this csi exists in the cluster */
	NCS_DB_LINK_LIST mem_list;	/* current members that belong to this pg */

	/* track list for this pg  */
	NCS_DB_LINK_LIST trk_list;
} AVND_PG;

/***************************************************************************
 ******************  M A C R O   D E F I N I T I O N S  ********************
 ***************************************************************************/

/* Macros for managing pg track flags */
#define m_AVND_PG_TRK_IS_CURRENT(x)      (((x)->info.flags) & SA_TRACK_CURRENT)
#define m_AVND_PG_TRK_IS_CHANGES(x)      (((x)->info.flags) & SA_TRACK_CHANGES)
#define m_AVND_PG_TRK_IS_CHANGES_ONLY(x) (((x)->info.flags) & SA_TRACK_CHANGES_ONLY)

#define m_AVND_PG_TRK_CURRENT_SET(x)      (((x)->info.flags) |= SA_TRACK_CURRENT)
#define m_AVND_PG_TRK_CHANGES_SET(x)      (((x)->info.flags) |= SA_TRACK_CHANGES)
#define m_AVND_PG_TRK_CHANGES_ONLY_SET(x) (((x)->info.flags) |= SA_TRACK_CHANGES_ONLY)

#define m_AVND_PG_TRK_CURRENT_RESET(x)      (((x)->info.flags) &= ~SA_TRACK_CURRENT)
#define m_AVND_PG_TRK_CHANGES_RESET(x)      (((x)->info.flags) &= ~SA_TRACK_CHANGES)
#define m_AVND_PG_TRK_CHANGES_ONLY_RESET(x) (((x)->info.flags) &= ~SA_TRACK_CHANGES_ONLY)

/* macro to get the PG record from the PG database */
#define m_AVND_PGDB_REC_GET(pgdb, csi_name_net) \
           (AVND_PG *)ncs_patricia_tree_get(&(pgdb), (uns8 *)&(csi_name_net))

/* macro to get the next PG record from the PG database */
#define m_AVND_PGDB_REC_GET_NEXT(pgdb, csi_name_net) \
           (AVND_PG *)ncs_patricia_tree_getnext(&(pgdb), (uns8 *)&(csi_name_net))

/* macro to get the PG track record from the PG database */
#define m_AVND_PGDB_TRK_REC_GET(pg, key) \
           (AVND_PG_TRK *)ncs_db_link_list_find(&((pg).trk_list), \
                                                (uns8 *)&(key) )

/* macro to get the PG member record from the PG database */
#define m_AVND_PGDB_MEM_REC_GET(pg, comp_name) \
           (AVND_PG_MEM *)ncs_db_link_list_find(&((pg).mem_list), \
                                                (uns8 *)&(comp_name) )

/***************************************************************************
 ******  E X T E R N A L   F U N C T I O N   D E C L A R A T I O N S  ******
 ***************************************************************************/

EXTERN_C uns32 avnd_pgdb_init(struct avnd_cb_tag *);
EXTERN_C uns32 avnd_pgdb_destroy(struct avnd_cb_tag *);
EXTERN_C AVND_PG *avnd_pgdb_rec_add(struct avnd_cb_tag *, SaNameT *, uns32 *);
EXTERN_C uns32 avnd_pgdb_rec_del(struct avnd_cb_tag *, SaNameT *);

EXTERN_C AVND_PG_TRK *avnd_pgdb_trk_rec_add(struct avnd_cb_tag *, AVND_PG *, AVND_PG_TRK_INFO *);
EXTERN_C void avnd_pgdb_trk_rec_del(struct avnd_cb_tag *, AVND_PG *, AVND_PG_TRK_KEY *);
EXTERN_C void avnd_pgdb_trk_rec_del_all(struct avnd_cb_tag *, AVND_PG *);

EXTERN_C AVND_PG_MEM *avnd_pgdb_mem_rec_add(struct avnd_cb_tag *, AVND_PG *, SaAmfProtectionGroupNotificationT *);
EXTERN_C AVND_PG_MEM *avnd_pgdb_mem_rec_rmv(struct avnd_cb_tag *, AVND_PG *, SaNameT *);
EXTERN_C void avnd_pgdb_mem_rec_del(struct avnd_cb_tag *, AVND_PG *, SaNameT *);
EXTERN_C void avnd_pgdb_mem_rec_del_all(struct avnd_cb_tag *, AVND_PG *);

EXTERN_C void avnd_pg_finalize(struct avnd_cb_tag *, SaAmfHandleT, MDS_DEST *);

#endif   /* !AVND_PG_H */
