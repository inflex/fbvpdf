#!/bin/sh
export G=`git rev-list HEAD --count`
echo $G
make -f Makefile.general GIT_BUILD='${G}' $1
