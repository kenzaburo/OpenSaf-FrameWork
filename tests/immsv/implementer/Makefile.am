#      -*- OpenSAF  -*-
#
# (C) Copyright 2009 The OpenSAF Foundation
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
# file and program are licensed under High-Availability Operating 
# Environment Software License Version 1.4.
# Complete License can be accesseble from below location.
# http://www.opensaf.org/license 
# See the Copying file included with the OpenSAF distribution for
# full licensing terms.
#
# Author(s): Ericsson AB
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = immoitest

immoitest_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/tests/immsv/common \
	-I$(top_srcdir)/tests/unit_test_fw/inc

immoitest_SOURCES = \
	$(top_srcdir)/tests/unit_test_fw/src/util.c \
	$(top_srcdir)/tests/unit_test_fw/src/utest.c \
	$(top_srcdir)/tests/immsv/common/immtest.c \
	test_saImmOiInitialize_2.c \
	test_saImmOiSelectionObjectGet.c \
	test_saImmOiDispatch.c \
	test_saImmOiFinalize.c \
	test_saImmOiImplementerSet.c \
	test_saImmOiImplementerClear.c \
	test_saImmOiClassImplementerSet.c \
	test_saImmOiClassImplementerRelease.c \
	test_saImmOiObjectImplementerSet.c \
	test_saImmOiObjectImplementerRelease.c \
	test_saImmOiRtObjectCreate_2.c \
	test_saImmOiRtObjectDelete.c \
	test_saImmOiRtObjectUpdate_2.c \
	test_SaImmOiRtAttrUpdateCallbackT.c \
	test_SaImmOiCcb.c \
	test_SaImmOiAdminOperation.c

immoitest_LDADD = \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOi.la \
	$(top_builddir)/osaf/libs/saf/libSaImm/libSaImmOm.la
