# Based off xorg configure.ac packages (xinput and evdev)
# Modified by Ryan Huffman <ryanhuffman@gmail.com>

# Copyright 2007 Peter Hutterer <peter@cs.unisa.edu.au>
# Process this file with autoconf to create configure.

AC_PREREQ([2.57])
AC_INIT(tmd,[0.1.0], [],tmd)

AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE

# Require xorg-macros: XORG_CWARNFLAGS, XORG_CHANGELOG
m4_ifndef([XORG_MACROS_VERSION], [AC_FATAL([must install xorg-macros 1.2 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.2)

AM_CONFIG_HEADER(config.h)

AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_INSTALL

PKG_CHECK_MODULES(XORG, xorg-server xproto $REQUIRED_MODULES)
AC_SUBST(XORG_CFLAGS)
AC_SUBST(XORG_LIBS)

# XI2 support
PKG_CHECK_MODULES(XI2, [xi >= 1.2.99] [inputproto >= 1.9.99.12])

AC_SUBST(VERSION)

CFLAGS="$CFLAGS $XORG_CFLAGS $XI2_CFLAGS"
LIBS="$LIBS $XORG_LIBS $XI2_LIBS"

XORG_MANPAGE_SECTIONS
XORG_RELEASE_VERSION

AC_OUTPUT([Makefile
           src/Makefile]) 
