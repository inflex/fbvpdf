#!/bin/sh
export G=`git rev-list HEAD --count`
echo $G
make GIT_BUILD='${G}'
