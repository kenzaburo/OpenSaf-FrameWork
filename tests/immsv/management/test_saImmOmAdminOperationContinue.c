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

#include <poll.h>
#include <unistd.h>
#include <pthread.h>
#include "immtest.h"

void saImmOmAdminOperationContinue_01(void)
{
    TRACE_ENTER();
    const SaNameT objectName = {strlen("opensafImm=opensafImm,safApp=safImmService"), "opensafImm=opensafImm,safApp=safImmService"};
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaImmContinuationIdT continuationId = 666;
    SaAisErrorT operationReturnValue=SA_AIS_OK;
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    test_validate(saImmOmAdminOperationContinue(
                                                ownerHandle,
                                                &objectName,
                                                continuationId,
                                                &operationReturnValue),
        SA_AIS_ERR_NOT_SUPPORTED);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    TRACE_LEAVE();
}

void saImmOmAdminOperationContinueAsync_01(void)
{
    TRACE_ENTER();
    const SaNameT objectName = {strlen("opensafImm=opensafImm,safApp=safImmService"), "opensafImm=opensafImm,safApp=safImmService"};
    const SaImmAdminOwnerNameT adminOwnerName = (SaImmAdminOwnerNameT) __FUNCTION__;
    SaImmAdminOwnerHandleT ownerHandle;
    SaInvocationT invocation=999;
    SaImmContinuationIdT continuationId = 666;
    safassert(saImmOmInitialize(&immOmHandle, &immOmCallbacks, &immVersion), SA_AIS_OK);
    safassert(saImmOmAdminOwnerInitialize(immOmHandle, adminOwnerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
    test_validate(saImmOmAdminOperationContinueAsync(
                                                     ownerHandle,
                                                     invocation,
                                                     &objectName,
                                                     continuationId), 
        SA_AIS_ERR_NOT_SUPPORTED);
    safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
    safassert(saImmOmFinalize(immOmHandle), SA_AIS_OK);
    TRACE_LEAVE();
}

__attribute__ ((constructor)) static void saImmOmAdminOperationContinue_constructor(void)
{
    test_suite_add(7, "Administrative Operations Invocation");
    test_case_add(7, saImmOmAdminOperationContinue_01, "saImmOmAdminOperationContinue - SA_AIS_ERR_NOT_SUPPORTED");
    test_case_add(7, saImmOmAdminOperationContinueAsync_01, "saImmOmAdminOperationContinueAsync - SA_AIS_ERR_NOT_SUPPORTED");
}

