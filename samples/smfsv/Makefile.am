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
# Author(s): Wind River Systems 
#

include $(top_srcdir)/Makefile.common

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = campaigns

EXTRA_DIST = \
	campaign_howtowrite.xml \
	offline_install.sh \
	offline_remove.sh \
	online_install.sh \
	online_remove.sh \
	README

bin_PROGRAMS = smf_demo_agent

smf_demo_agent_CPPFLAGS = \
	-DNCS_SAF=1 \
	$(AM_CPPFLAGS)

smf_demo_agent_SOURCES = \
	smf_demo.c

smf_demo_agent_LDADD = \
	@SAF_AIS_SMF_LIBS@		   
