#!/bin/bash
#
# Build linux binary on linux
set -x

if [ "$1" = "clean" ]; then
	make -f Makefile.general clean
	exit 0
fi

export G=`git rev-list HEAD --count`
echo $G
ZPR="fbvpdf-R${G}-linux.tar.gz"
make -j4 -f Makefile.general GIT_BUILD='${G}'  $1
MR=$?
if [ $MR -eq 0 ]; then
	mv build/release/mupdf-gl build/release/mupdf-linux
	if [ -d fbvpdf ]; then
		rm -rf fbvpdf
	fi
	mkdir fbvpdf
	cp build/release/mupdf-linux fbvpdf/fbvpdf
	tar zcvf ${ZPR} fbvpdf
fi
exit $MR
