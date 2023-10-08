#!/bin/sh
DEF_CC=~/Tools/crosscompile/gcc-aarch64/bin/aarch64-none-linux-gnu-gcc
CC=${CC:-$DEF_CC}

echo "Compiling with $CC..."

for lib in drm gbm EGL GLESv2; do
	SRC=mimic_$lib.c
	DST=lib$lib.so
	echo $SRC "->" $DST
	$CC -shared $SRC -o $DST
done

