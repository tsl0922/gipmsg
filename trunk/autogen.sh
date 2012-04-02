#!/bin/sh
# Run this to generate all the initial makefiles, etc.

intltoolize --copy --force --automake
libtoolize --force --copy
aclocal
autoheader
automake --add-missing --copy --gnu
autoconf
./configure
