#!/bin/sh

aclocal --verbose \
  && autoheader \
  && automake-1.6 --add-missing --copy --include-deps --foreign --gnu --verbose \
  && autoconf \
  && ./configure $@
