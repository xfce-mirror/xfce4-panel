#!/bin/sh

if test -z "$*"; then
  echo "**Message**: I am going to add --enable-maintainer-mode to \`configure'."
  echo "If you wish to pass any other to it, please specify them on the"
  echo \`$0\'" command line."
  echo

  conf_flags="--enable-maintainer-mode"
else
  unset conf_flags
fi

libtoolize --copy --force \
  && aclocal --verbose \
  && autoheader \
  && automake --add-missing --copy --include-deps --foreign --gnu --verbose \
  && autoconf \
  && ./configure $conf_flags $@
