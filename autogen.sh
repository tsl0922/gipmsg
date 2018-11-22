#!/bin/bash

intltoolize --copy --force --automake
case `uname` in
	Darwin) glibtoolize --force --copy ;;
	*) libtoolize --force --copy ;;
esac
aclocal
autoheader
automake --add-missing --copy --gnu
autoconf