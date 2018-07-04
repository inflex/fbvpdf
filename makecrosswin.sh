#!/bin/bash
set -x

export G=`git rev-list HEAD --count`
echo $G

# Build the needed tools for the cross compile
echo "Making 'GENERATE' tools for linux."
make -f Makefile.general generate
MR=$?
echo "Make GENERATE exit $MR"

# Build the cross compile
echo "Now building actual Windows version..."
make -f Makefile.mingw OS=w64_x86-cross-mingw32 GIT_BUILD='${G}'  $1
MR=$?
if [ $MR -eq 0 ]; then
	mv build/release/mupdf-gl build/release/mupdf-win.exe
fi
exit $MR
