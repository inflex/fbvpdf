#!/bin/sh
PREFIX=i686-w64-mingw32
LCC=${PREFIX}-gcc
LAR=${PREFIX}-ar
LWDRES=${PREFIX}-windres
export CROSSCOMPILE=yes
make OS=MINGW CC=${LCC} AR=${LAR} WINDRES=${LWDRES}
