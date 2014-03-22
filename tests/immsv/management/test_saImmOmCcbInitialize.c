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

#include "immtest.h"

void saImmOmCcbInitialize_01(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion),
        SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE,
        &ownerHandle), SA_AIS_OK);

    test_validate(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbInitialize_02(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE,
        &ownerHandle), SA_AIS_OK);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);

    /* already finalized ownerHandle */
    rc = saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle);
    if (rc != SA_AIS_ERR_BAD_HANDLE)
        goto done;

    /* invalid ownerHandle */
    rc = saImmOmCcbInitialize(-1, 0, &ccbHandle);

done:
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

void saImmOmCcbInitialize_03(void)
{
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmCcbHandleT ccbHandle;

    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE,
        &ownerHandle), SA_AIS_OK);

    test_validate(saImmOmCcbInitialize(ownerHandle, -1, &ccbHandle),
        SA_AIS_ERR_INVALID_PARAM);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
}

extern void saImmOmCcbObjectCreate_01(void);
extern void saImmOmCcbObjectCreate_01_bad(void);
extern void saImmOmCcbObjectCreate_02(void);
extern void saImmOmCcbObjectCreate_03(void);
extern void saImmOmCcbObjectCreate_04(void);
extern void saImmOmCcbObjectCreate_05(void);
extern void saImmOmCcbObjectCreate_06(void);
extern void saImmOmCcbObjectCreate_07(void);
extern void saImmOmCcbObjectDelete_01(void);
extern void saImmOmCcbObjectDelete_02(void);
extern void saImmOmCcbObjectDelete_03(void);
extern void saImmOmCcbObjectDelete_04(void);
extern void saImmOmCcbObjectDelete_05(void);
extern void saImmOmCcbObjectModify_2_01(void);
extern void saImmOmCcbObjectModify_2_02(void);
extern void saImmOmCcbObjectModify_2_03(void);
extern void saImmOmCcbObjectModify_2_04(void);
extern void saImmOmCcbObjectModify_2_05(void);
extern void saImmOmCcbObjectModify_2_06(void);
extern void saImmOmCcbApply_01(void);
extern void saImmOmCcbApply_02(void);
extern void saImmOmCcbFinalize_01(void);
extern void saImmOmCcbFinalize_02(void);

__attribute__ ((constructor)) static void saImmOmInitialize_constructor(void)
{
    test_suite_add(6, "Configuration Changes");
    test_case_add(6, saImmOmCcbInitialize_01, "saImmOmCcbInitialize - SA_AIS_OK");
    test_case_add(6, saImmOmCcbInitialize_02, "saImmOmCcbInitialize - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(6, saImmOmCcbInitialize_02, "saImmOmCcbInitialize - SA_AIS_ERR_INVALID_PARAM");

    test_case_add(6, saImmOmCcbObjectCreate_01, "saImmOmCcbObjectCreate - SA_AIS_OK");
//    test_case_add(6, saImmOmCcbObjectCreate_01_bad, "saImmOmCcbObjectCreate - SA_AIS_OK, bad...");
    test_case_add(6, saImmOmCcbObjectCreate_02, "saImmOmCcbObjectCreate - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(6, saImmOmCcbObjectCreate_03, "saImmOmCcbObjectCreate - SA_AIS_ERR_INVALID_PARAM");
    test_case_add(6, saImmOmCcbObjectCreate_04, "saImmOmCcbObjectCreate - SA_AIS_ERR_BAD_OPERATION");
    test_case_add(6, saImmOmCcbObjectCreate_05, "saImmOmCcbObjectCreate - SA_AIS_ERR_EXIST");
    test_case_add(6, saImmOmCcbObjectCreate_06, "saImmOmCcbObjectCreate - SA_AIS_ERR_NOT_EXIST");
    test_case_add(6, saImmOmCcbObjectCreate_07, "saImmOmCcbObjectCreate (association class) - SA_AIS_OK");

    test_case_add(6, saImmOmCcbObjectDelete_01, "saImmOmCcbObjectDelete - SA_AIS_OK");
    test_case_add(6, saImmOmCcbObjectDelete_02, "saImmOmCcbObjectDelete - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(6, saImmOmCcbObjectDelete_03, "saImmOmCcbObjectDelete - SA_AIS_ERR_BAD_OPERATION");
    test_case_add(6, saImmOmCcbObjectDelete_04, "saImmOmCcbObjectDelete - SA_AIS_ERR_NOT_EXIST");
    test_case_add(6, saImmOmCcbObjectDelete_05, "saImmOmCcbObjectDelete - SA_AIS_ERR_BUSY");

    test_case_add(6, saImmOmCcbObjectModify_2_01, "saImmOmCcbObjectModify_2 - SA_AIS_OK");
    test_case_add(6, saImmOmCcbObjectModify_2_02, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(6, saImmOmCcbObjectModify_2_03, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_INVALID_PARAM");
    test_case_add(6, saImmOmCcbObjectModify_2_04, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_BAD_OPERATION");
    test_case_add(6, saImmOmCcbObjectModify_2_05, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_NOT_EXIST");
    test_case_add(6, saImmOmCcbObjectModify_2_05, "saImmOmCcbObjectModify_2 - SA_AIS_ERR_BUSY");

    test_case_add(6, saImmOmCcbApply_01, "saImmOmCcbApply - SA_AIS_OK");
    test_case_add(6, saImmOmCcbApply_02, "saImmOmCcbApply - SA_AIS_ERR_BAD_HANDLE");

    test_case_add(6, saImmOmCcbFinalize_01, "saImmOmCcbFinalize - SA_AIS_OK");
    test_case_add(6, saImmOmCcbFinalize_02, "saImmOmCcbFinalize - SA_AIS_ERR_BAD_HANDLE");
}

