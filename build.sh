#!/bin/bash
set -x

LOS=${TRAVIS_OS_NAME:-linux}
export G=`git rev-list HEAD --count`
echo $G
make -f Makefile.general GIT_BUILD='${G}'  $1
MR=$?
if [ $MR -eq 0 ]; then
	if [ "$LOS" == "linux" ]; then
		mv build/release/mupdf-gl build/release/mupdf-linux
	elif [ "$LOS" == "osx" ]; then
		mv build/release/mupdf-gl build/release/mupdf-macos
	fi
fi
exit $MR
