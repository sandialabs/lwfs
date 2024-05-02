#! /bin/bash 

# Use this script to bootstrap by creating all the needed configure scripts.
# Use "./autogen.sh --force" to force every script to be recreated, even 
# if it doesn't look it is necessary for any given script.

# Include our directory of autoconf macros
export ACLOCAL="aclocal -I $PWD/m4"

# autoreconf calls aclocal, autoheader, libtoolize, ... when needed. 
autoreconf --install $@
