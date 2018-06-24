#!/bin/sh
export G=`git rev-list HEAD --count`
echo $G
make -f Makefile.general GIT_BUILD='${G}' $1
if [ $? -eq 0 ]; then
	if [ ${OS_TYPE} == "linux" ]; then
		mv build/release/mupdf-gl build/release/mupdf-linux
	elif [ ${OS_TYPE} == "macos" ]; then
		mv build/release/mupdf-gl build/release/mupdf-macos
	fi
fi
