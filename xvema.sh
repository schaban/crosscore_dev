#!/bin/sh

VEMA_DIR=etc/vema
if [ ! -d "$VEMA_DIR" ]; then
	mkdir -p $VEMA_DIR
fi

VEMA_URL=https://raw.githubusercontent.com/schaban/schaban.github.io/main/vema
for srcfile in vema.c vema.h
do
	if [ ! -f "$VEMA_DIR/$srcfile" ]; then
		echo "Downloading $srcfile"
		curl $VEMA_URL/$srcfile -o $VEMA_DIR/$srcfile
	fi
done

OUT_DIR=bin/xvema
if [ ! -d "$OUT_DIR" ]; then
	mkdir -p $OUT_DIR
fi
X_EXE=$OUT_DIR/crosscore_demo
rm -f $X_EXE

COMPILER_MODE=0
VEMA_MTX_MODE=""
while getopts "c:m" opt;
do
	case $opt in
		c)
			COMPILER_MODE=$OPTARG
		;;
		m)
			VEMA_MTX_MODE="-DXD_USE_VEMA_MTX"
		;;
	esac
done

# gcc|clang
case $COMPILER_MODE in
	0)
		_CC_=gcc
		_CXX_=g++
	;;
	1)
		_CC_=clang
		_CXX_=clang++
	;;
	2)
		_CC_=gcc-11
		_CXX_=g++-11
	;;
esac
echo "Compiling with \e[1m"$_CC_"/"$_CXX_"\e[m"

VEMA_ARCH=""
case `uname -m` in
	x86_64)
		VEMA_ARCH="-DVEMA_GCC_SSE -DVEMA_AVX"
	;;
	aarch64)
		VEMA_ARCH="-DVEMA_GCC_AARCH64_ASM -DVEMA_ARM_VSQRT"
	;;
	armv7l)
		VEMA_ARCH="-DVEMA_GCC_BUILTINS -DVEMA_NEON -mfpu=neon"
	;;
esac

OPTI_FLGS="-march=native -O3 -ffast-math -ftree-vectorize -flto"
VEMA_FLGS="-DXD_USE_VEMA -DVEMA_NO_CLIB $VEMA_ARCH $VEMA_MTX_MODE"
$_CC_ -std=c99 $OPTI_FLGS $VEMA_FLGS $VEMA_DIR/vema.c -g -c -o $VEMA_DIR/vema.o
$_CXX_ -std=c++11 $OPTI_FLGS $VEMA_FLGS -DOGLSYS_ES=0 -DOGLSYS_CL=0 -DDRW_NO_VULKAN=1 -DXD_TSK_NATIVE=1 -g -I src -I inc -I $VEMA_DIR -DX11 `ls src/*.cpp` $VEMA_DIR/vema.o -o $X_EXE -ldl -lX11 -lpthread
if [ -f $X_EXE ]; then
	objdump -d $X_EXE > $X_EXE.txt
	cp -v src/cmd/roof.sh $OUT_DIR
fi
ls -ho --color=always $X_EXE
