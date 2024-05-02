#!/bin/sh -x

aclocal -I ${PWD}/m4
autoheader
libtoolize --force
automake -c --include-deps --add-missing --force-missing
autoconf

