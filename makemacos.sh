#!/bin/bash
set -x

brew install sdl2
export G=`git rev-list HEAD --count`
echo $G
make -f Makefile.macos GIT_BUILD='${G}'  $1
MR=$?
if [ $MR -eq 0 ]; then
	mv build/release/mupdf-gl build/release/mupdf-macos
	./make-bundle.sh
fi
exit $MR