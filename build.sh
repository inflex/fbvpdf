#!/bin/sh
export G=`git rev-list HEAD --count`
echo $G
make -f Makefile.general GIT_BUILD='${G}' 
if [ $? -eq 0 ]; then
	if [ "$1" == "linux" ]; then
		mv build/release/mupdf-gl build/release/mupdf-linux
	elif [ "$1" == "osx" ]; then
		mv build/release/mupdf-gl build/release/mupdf-macos
	fi
fi
