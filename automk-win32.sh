#!/bin/sh
# Run this script to make and generate the win32 version of gipmsg

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
./configure --disable-shared $1

if [ -f "Makefile" ]; then
  echo
  echo Running make...
  make
fi

if [ -f "src/gipmsg.exe" ]; then
  echo
  echo Running strip...
  strip src/gipmsg.exe -o gipmsg.exe
  echo
  echo Creating release...
  if [ ! -d "release" ]; then
    mkdir release
  fi
  mv -f gipmsg.exe ./release
  cp -rf pixmaps/icons ./release
  cp -rf pixmaps/emotions ./release
  echo
  echo Done.
  echo
fi
