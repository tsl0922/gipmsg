#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([gipmsg], [0.2], [tsl0922@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/main.c])

#Checks for platforms
AC_CANONICAL_HOST
dnl check for win32 platform
AC_MSG_CHECKING([for native win32 platform])
case "$host" in
  *-*-mingw*)
    os_win32=yes
    ;;
  *)
    os_win32=no
    ;;
esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, [test "$os_win32" = "yes"])

if test "$os_win32" = "yes"; then
  AC_DEFINE(WIN32, [1], [Defined if os is native Win32])
fi

dnl check for linux platform
AC_MSG_CHECKING([for linux platform])
case "$host" in
  *-*-linux*)
    os_linux=yes
    ;;
  *)
    os_linux=no
    ;;  
esac
AC_MSG_RESULT([$os_linux])
AM_CONDITIONAL(OS_LINUX, [test "$os_linux" = "yes"])

if test "$os_linux" = "yes"; then
  AC_DEFINE(_LINUX, [1], [Defined if os is Linux])
fi

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT(disable-static)

# Checks for libraries.
PKG_CHECK_MODULES([GIPMSG], [
		glib-2.0,
		gthread-2.0,
		gtk+-2.0
])

if test "$os_win32" = "yes"; then
  LIBS="-lws2_32 -liphlpapi $LIBS"
fi
# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h locale.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([inet_ntoa memchr memset setlocale socket strcasecmp strdup strncasecmp strndup strpbrk strspn strtol])

# Gettext stuff.
IT_PROG_INTLTOOL([0.40.0])
AC_SUBST([GETTEXT_PACKAGE], [gipmsg])
AM_GNU_GETTEXT_VERSION([0.17])
AM_GLIB_GNU_GETTEXT
AC_DEFINE([GETTEXT_PACKAGE],[PACKAGE_TARNAME], [Define to the gettext package name.])

AM_PROG_LIBTOOL

AC_CONFIG_FILES([Makefile src/Makefile po/Makefile.in tests/Makefile])
AC_OUTPUT