#!/bin/bash

# Setup the files that are necessary
if [[ $# -eq 1 ]] && [[ $1 == "clean" ]]; then
  echo "Cleaning up automake files"
  rm -v aclocal.m4
  rm -v compile
  rm -v config.guess
  rm -v config.status
  rm -v config.sub
  rm -v configure
  rm -v depcomp
  rm -v install-sh
  rm -v libtool
  rm -v ltmain.sh
  rm -v Makefile.in
  rm -v missing
else
  echo "Initializing automake"
  libtoolize && aclocal && autoconf && automake --add-missing
fi
