#      -*- OpenSAF  -*-
#
# (C) Copyright 2008 The OpenSAF Foundation
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
# Author(s): Wind River Systems
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = include

noinst_LTLIBRARIES = libavsv_common.la

libavsv_common_la_CPPFLAGS = \
	-DNCS_EDU=1 \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/osaf/libs/common/avsv/include

libavsv_common_la_SOURCES = \
	avsv_d2nedu.c \
	avsv_nd2ndedu.c \
	avsv_d2nmsg.c \
	avsv_n2avamsg.c \
	avsv_n2avaedu.c \
	avsv_util.c \
	avsv_eduutil.c
