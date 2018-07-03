#!/bin/bash
set -x

export G=`git rev-list HEAD --count`
echo $G

# Build the needed tools for the cross compile
make -f Makefile.general generate

# Build the cross compile
make -f Makefile.mingw OS=w64_x86-cross-mingw32 GIT_BUILD='${G}'  $1
MR=$?
if [ $MR -eq 0 ]; then
	mv build/release/mupdf-gl build/release/mupdf-win.exe
fi
exit $MR
