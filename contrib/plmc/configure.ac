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

AC_PREREQ([2.59])
AC_INIT([plmc], [0.9], [devel@list.opensaf.org])
AC_CONFIG_AUX_DIR([.])
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([INSTALL])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([$PACKAGE_NAME], [$PACKAGE_VERSION])


#
# Check if rpath is disabled
#
AC_MSG_CHECKING([whether to use rpath])
AC_ARG_ENABLE([rpath],
	[AC_HELP_STRING([--disable-rpath],
		[Patches libtool to not use rpath in the libraries produced.])],
	[],
	[osaf_cv_rpath=yes])
AC_MSG_RESULT([$osaf_cv_rpath])

# Patch libtool to not use rpath if requested.
if test "$osaf_cv_rpath" = "no"; then
	AC_MSG_RESULT([Patching libtool to not use rpath.])
	sed < libtool > libtool-2 \
		's/^hardcode_libdir_flag_spec.*$'/'hardcode_libdir_flag_spec=" -D__LIBTOOL_NO_RPATH__ "/'
	mv libtool-2 libtool
	chmod 755 libtool
fi


#
# Many distros use different init.d directory depending on their LSB compliance
# support. Use this option to point to the init.d directory used by your distro.
# Default to /etc/init.d
#
AC_ARG_WITH([initscriptdir],
        [AS_HELP_STRING([--with-initscriptdir=@<:@DIR@:>@],
                [use DIR as the directory containing the init.d scripts.
		 [default=/etc/init.d]])],
        [
        if test -d $withval; then
                with_initscriptdir=$withval
        else
                AC_ERROR([$withval is an invalid path])
        fi
        ],
        [with_initscriptdir=/etc/init.d])

AC_SUBST([initscriptdir], ["$with_initscriptdir"])


#
# Unconditionaly drop previx from sysconfdir
# and localstatedir
#
if test "$sysconfdir" = '${prefix}/etc' ; then
        sysconfdir="/etc"
fi

AC_SUBST([sysconfdir])

if test "$localstatedir" = '${prefix}/var' ; then
        localstatedir="/var"
fi

AC_SUBST([localstatedir])


#
# Check for configure deps
#
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

AC_CHECK_FUNCS([ \
        bzero \
        inet_ntoa \
        memset \
        socket \
        strstr])

AC_CHECK_HEADERS([ \
        arpa/inet.h \
        netdb.h \
        netinet/in.h \
        sys/ioctl.h \
	sys/socket.h \
	syslog.h])

AC_FUNC_FORK
AC_HEADER_STDBOOL
AC_PROG_CXX
AC_TYPE_PID_T


#
# List the output Makefiles
#
AC_CONFIG_FILES([ \
        Makefile \
        config/Makefile \
        include/Makefile \
        lib/Makefile \
        lib/plmc/Makefile \
        lib/plmc/include/Makefile \
        lib/utils/Makefile \
        plmcd/Makefile \
        scripts/Makefile \
        scripts/plmcboot \
        scripts/plmcd])

AC_OUTPUT
