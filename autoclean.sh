#!/bin/sh

make distclean

echo -n "Cleaning generated files..."
rm -f config.* configure aclocal.m4
rm -f compile depcomp ltmain.sh missing install-sh
rm -f po/*.gmo
rm -f $(find -name Makefile.in) po/Makefile.in.in
echo "done."

