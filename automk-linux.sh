#!/bin/sh
# Run this script to make and install the linux version of gipmsg

if [ ! -f "configure" ]; then
  echo
  echo Running intltoolize...
  intltoolize --copy --force --automake
  echo
  echo Running libtoolize...
  libtoolize --force --copy
  echo
  echo Running aclocal...
  aclocal
  echo
  echo Running autoheader...
  autoheader
  echo
  echo Running automake...
  automake --add-missing --copy --gnu
  echo
  echo Running autoconf...
  autoconf
fi

if [ -f "config.h" ]; then
  echo
  echo Cleanning Up...
  echo
  make distclean
fi

echo
echo Running configure...
./configure --prefix=/usr $1

if [ -f "Makefile" ]; then
  echo
  echo Running make...
  make
fi

if [ -f "src/gipmsg" ]; then
  echo
  echo Running strip...
  strip src/gipmsg
  echo
  echo Installing...
  sudo make install
  echo
  echo Done.
  echo
fi
