dnl  Copyright 2007 Peter Hutterer <peter@cs.unisa.edu.au>
dnl Process this file with autoconf to create configure.

AC_PREREQ([2.57])
AC_INIT(tmd,[1.4.99.1], [],tmd)
AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE

# Require xorg-macros: XORG_CWARNFLAGS, XORG_CHANGELOG
m4_ifndef([XORG_MACROS_VERSION], [AC_FATAL([must install xorg-macros 1.2 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.2)

AM_CONFIG_HEADER(config.h)

AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_INSTALL

XORG_CWARNFLAGS

# Checks for extensions
XORG_DRIVER_CHECK_EXT(XINPUT, inputproto)

# Checks for pkg-config packages
PKG_CHECK_MODULES(XORG, xorg-server xproto $REQUIRED_MODULES)
AC_SUBST(XORG_CFLAGS)
AC_SUBST(XORG_LIBS)

# Checks for pkg-config packages
PKG_CHECK_MODULES(TMD, x11 xext [xi >= 1.2] [inputproto >= 1.5])

# XI2 support
PKG_CHECK_MODULES(XI2, [xi >= 1.2.99] [inputproto >= 1.9.99.12],
                  HAVE_XI2="yes"; AC_DEFINE(HAVE_XI2, 1, [XI2 available]),
                  HAVE_XI2="no");
AM_CONDITIONAL(HAVE_XI2, [ test "$HAVE_XI2" = "yes" ])

AC_SUBST(TMD_CFLAGS)
AC_SUBST(TMD_LIBS)
AC_SUBST(HAVE_XI2)

AC_SUBST(VERSION)

CFLAGS="$CFLAGS $XORG_CFLAGS $TMD_CFLAGS"
LIBS="$LIBS $XORG_LIBS $TMD_LIBS"

XORG_MANPAGE_SECTIONS
XORG_RELEASE_VERSION
XORG_CHANGELOG

AC_OUTPUT([Makefile
           src/Makefile])
