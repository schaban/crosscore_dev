#!/bin/sh
DEF_CC=~/Tools/crosscompile/gcc-aarch64/bin/aarch64-none-linux-gnu-gcc
CC=${CC:-$DEF_CC}

LIB_DIR=lib

if [ ! -d $LIB_DIR ]; then
	mkdir -p $LIB_DIR
fi

echo "Compiling with $CC..."

for lib in drm gbm EGL GLESv2; do
	SRC=mimic_$lib.c
	DST=$LIB_DIR/lib$lib.so
	echo $SRC "->" $DST
	$CC -shared $SRC -o $DST
done

