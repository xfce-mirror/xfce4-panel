#!/bin/sh

libtoolize --copy --force \
  && aclocal --verbose \
  && autoheader \
  && automake --add-missing --copy --include-deps --foreign --gnu --verbose \
  && autoconf \
  && ./configure $@
