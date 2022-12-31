AC_PREREQ([2.71])
AC_INIT([ssdp-advertiser],[1.0],[tim@metaverse.systems])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CXX
AM_INIT_AUTOMAKE([subdir-objects])

PKG_CHECK_MODULES([GSSDP], gssdp-1.6)
PKG_CHECK_MODULES([CURL], libcurl)
PKG_CHECK_MODULES([XML], pugixml)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
