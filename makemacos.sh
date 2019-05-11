#!/bin/bash
set -x

#brew install sdl2
export G=`git rev-list HEAD --count`
echo $G
make -f Makefile.macos GIT_BUILD='${G}'  $1
MR=$?
if [ $MR -eq 0 ]; then
	mv build/release/mupdf-gl build/release/mupdf-macos
	./make-macos-bundle.sh
	cp Info.plist fbvpdf.app/Contents
#	/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister -f fbvpdf.app
fi
exit $MR
