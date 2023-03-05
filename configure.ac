AC_PREREQ([2.71])
AC_INIT([ssdp-advertiser],[1.0],[tim@metaverse.systems])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CXX
AM_INIT_AUTOMAKE([subdir-objects])

PKG_CHECK_MODULES([GSSDP], gssdp-1.6,
    [AC_DEFINE([HAVE_GSSDP], [1], [Use gssdp])],
    [PKG_CHECK_MODULES([GSSDP], gssdp-1.2)]
)
PKG_CHECK_MODULES([CURL], libcurl)
PKG_CHECK_MODULES([XML], pugixml)
PKG_CHECK_MODULES([SYSTEMD], libsystemd)

AC_ARG_WITH([systemdsystemunitdir],
     [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [systemd])],,
     [with_systemdsystemunitdir=auto])
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"], [
     def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)

     AS_IF([test "x$def_systemdsystemunitdir" = "x"],
   [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
    [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])])
    with_systemdsystemunitdir=no],
   [with_systemdsystemunitdir="$def_systemdsystemunitdir"])])
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
      [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])])
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdsystemunitdir" != "xno"])

AC_CONFIG_FILES([Makefile src/Makefile systemd/Makefile systemd/ssdp-advertiser.service])
AC_OUTPUT
