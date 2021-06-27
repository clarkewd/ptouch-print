#
#	ptouch-print - Print labels with images or text on a Brother P-Touch
#	
#	Copyright (C) 2015-2021 Dominic Radermacher <dominic@familie-radermacher.ch>
#
#	This program is free software; you can redistribute it and/or modify it
#	under the terms of the GNU General Public License version 3 as
#	published by the Free Software Foundation
#	
#	This program is distributed in the hope that it will be useful, but
#	WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#	See the GNU General Public License for more details.
#	
#	You should have received a copy of the GNU General Public License
#	along with this program; if not, write to the Free Software Foundation,
#	Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ptouch], [m4_esyscmd_s([build-aux/git-version-gen])], [dominic@familie-radermacher.ch])
AC_CONFIG_SRCDIR([src/libptouch.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AM_INIT_AUTOMAKE
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.19)

# Checks for libraries.
AC_CHECK_LIB([gd], [gdImageStringFT])
AC_CHECK_LIB([usb-1.0], [libusb_init])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h libintl.h stdint.h stdlib.h string.h])
AC_CHECK_HEADERS([gd.h], [], [AC_MSG_ERROR([libgd headers missing - maybe you need to install package libgd-dev, gd-dev or gd-devel?])])
AC_CHECK_HEADERS([libusb-1.0/libusb.h], [], [AC_MSG_ERROR([libusb headers missing - maybe you need to install package libusb-dev, libusb-devel or libusb-1.0-0-dev?])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset setlocale strpbrk strtol])

AC_CONFIG_FILES([Makefile po/Makefile.in])
AC_OUTPUT
