#      -*- OpenSAF  -*-
#
# (C) Copyright 2010 The OpenSAF Foundation
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
# under the GNU Lesser General Public License Version 2.1, February 1999.
# The complete license can be accessed from the following location:
# http://opensource.org/licenses/lgpl-license.php
# See the Copying file included with the OpenSAF distribution for full
# licensing terms.
#
# Author(s):  Emerson Network Power
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = include

noinst_LTLIBRARIES = libclmsv_common.la

libclmsv_common_la_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        -I$(top_srcdir)/osaf/libs/common/clmsv/include

libclmsv_common_la_SOURCES = \
        clmsv_enc_dec.c
