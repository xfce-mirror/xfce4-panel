#!/bin/sh
#
# $Id: autogen.sh 25025 2007-02-24 09:54:07Z jasper $
#
# Copyright (c) 2002-2005
#         The Xfce development team. All rights reserved.
#
# Written for Xfce by Benedikt Meurer <benny@xfce.org>.
#

(type xdt-autogen) >/dev/null 2>&1 || {
  cat >&2 <<EOF
autogen.sh: You don't seem to have the Xfce development tools installed on
            your system, which are required to build this software.
            Please install the xfce4-dev-tools package first, it is available
            from http://www.xfce.org/.
EOF
  exit 1
}

# verify that po/LINGUAS is present
(test -f po/LINGUAS) >/dev/null 2>&1 || {
  cat >&2 <<EOF
autogen.sh: The file po/LINGUAS could not be found. Please check your snapshot
            or try to checkout again.
EOF
  exit 1
}

# substitute revision and linguas
if test -d .git/svn; then
 revision=`LC_ALL=C git-svn find-rev HEAD`
else
 revision=`LC_ALL=C svn info $0 | awk '/^Revision: / {printf "%05d\n",
$2}'`
fi
sed -e "s/@REVISION@/${revision}/g" \
   < "configure.in.in" > "configure.in"

xdt-autogen $@

# xdt-autogen clean does not remove all generated files
(test x"clean" = x"$1") && {
  rm -f configure.in
  rm -f INSTALL
} || true
# vi:set ts=2 sw=2 et ai:
