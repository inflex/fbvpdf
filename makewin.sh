#!/bin/bash
set -x

export G=`git rev-list HEAD --count`
echo $G

make -f Makefile.win GIT_BUILD='${G}' 
MR=$?
if [ $MR -eq 0 ]; then
	mv build/release/mupdf-gl.exe mupdf-win.exe
fi
exit $MR
