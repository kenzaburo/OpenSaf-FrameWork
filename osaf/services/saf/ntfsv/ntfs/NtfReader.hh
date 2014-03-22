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


#ifndef __NTF_READER_HH
#define __NTF_READER_HH

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */
#include "NtfNotification.hh"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */
// typedef std::deque<ntfsv_send_not_req_t> readerNotificationListT;
// typedef std::deque<ntfsv_send_not_req_t>::iterator
//         readerNotificationListTIter;
// typedef std::deque<ntfsv_send_not_req_t>::reverse_iterator
//         readerNotReverseIterT;

typedef std::deque<NtfNotification> readerNotificationListT;
typedef std::deque<NtfNotification>::iterator readerNotificationListTIter;
typedef std::deque<NtfNotification>::reverse_iterator readerNotReverseIterT;
/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */
class NtfLogger;

class NtfReader
{
public:
    NtfReader(NtfLogger& ntfLogger, unsigned int readerId);
//    virtual ~NtfReader();
    NtfNotification next(SaNtfSearchDirectionT direction,
                                           SaAisErrorT* error);
    void setCollection(readerNotificationListT& coll);
    unsigned int getId();

private:
    readerNotificationListT coll_;
    readerNotificationListTIter ffIter;
    bool lastRead;

    unsigned int readerId_;
};

#endif // NTF_READER_HH

