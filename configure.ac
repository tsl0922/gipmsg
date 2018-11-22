#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([gipmsg], [0.3], [tsl0922@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/main.c])
AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Initialize libtool
LT_PREREQ([2.2.6])

# Checks for libraries.
PKG_CHECK_MODULES([GIPMSG], [
		glib-2.0,
		gthread-2.0,
		gtk+-2.0,
		libnotify
])

case "$host_os" in
	cygwin*|mingw*)
		LIBS+=" -lws2_32 -liphlpapi -mwindows"
		;;
	darwin*)
		CFLAGS+=" -I/usr/local/opt/libiconv/include"
		LIBS+=" -L/usr/local/opt/libiconv/lib -liconv"
		;;
esac

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h locale.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([inet_ntoa memchr memset mkdir setlocale socket strcasecmp strchr strdup strerror strndup strpbrk strrchr strspn strtol])

# Gettext stuff.
IT_PROG_INTLTOOL([0.40.0])
AC_SUBST([GETTEXT_PACKAGE], [gipmsg])
AM_GNU_GETTEXT_VERSION([1.11])
AM_GLIB_GNU_GETTEXT
AC_DEFINE([GETTEXT_PACKAGE],[PACKAGE_TARNAME], [Define to the gettext package name.])

AM_PROG_LIBTOOL

AC_CONFIG_FILES([Makefile src/Makefile pixmaps/Makefile po/Makefile.in])

AC_OUTPUT
