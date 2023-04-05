#!/bin/sh

SYS_NAME=`uname -s`
SYS_ARCH=`uname -m`

if [ "$SYS_NAME" = "Darwin" ]; then
	./mac_build.sh $*
	exit
fi

PROG_DIR=bin/${SYS_NAME}_${SYS_ARCH}
PROG_NAME=crosscore_demo
PROG_PATH=$PROG_DIR/$PROG_NAME

if [ ! -d $PROG_DIR ]; then mkdir -p $PROG_DIR; fi

CXX_OPTS="-std=c++11 -ffast-math -ftree-vectorize -pthread"

PROG_OPTS="-DX11"
PROG_INCS="-I src -I inc"
PROG_LIBS="-lX11"
PROG_SRCS="`ls src/*.cpp`"

DEF_CXX="g++"

LS_OPTS="-lho"
CP_OPTS="-v"

case $SYS_NAME in
	Linux)
		PROG_LIBS="$PROG_LIBS -ldl"
		LS_OPTS="-ho --color=always"
	;;
	OpenBSD)
		PROG_INCS="$PROG_INCS -I/usr/X11R6/include"
		PROG_LIBS="$PROG_LIBS -L/usr/X11R6/lib"
		DEF_CXX="clang++"
	;;
	FreeBSD)
		PROG_INCS="$PROG_INCS -I/usr/local/include"
		PROG_LIBS="$PROG_LIBS -L/usr/local/lib"
		DEF_CXX="clang++"
	;;
	SunOS)
		PROG_LIBS="$PROG_LIBS -static-libgcc -static-libstdc++"
		CP_OPTS=""
	;;
	Haiku)
		PROG_LIBS=""
	;;
esac

DL_CMD=""
if [ -x "`command -v wget`" ]; then
	DL_CMD="wget -O"
elif [ -x "`command -v curl`" ]; then
	DL_CMD="curl -o"
fi

VEMA_DIR=etc/vema
VEMA_SRCS=""
VEMA_OPTS=""
USE_VEMA=${XBUILD_VEMA:-0}
if [ $USE_VEMA -ne 0 ]; then
	if [ -n "$DL_CMD" ]; then
		mkdir -p $VEMA_DIR
		VEMA_URL=https://schaban.github.io/vema
		if [ ! -f "$VEMA_DIR/vema.h" ]; then
			echo "Downloading vema header..."
			$DL_CMD $VEMA_DIR/vema.h $VEMA_URL/vema.h
		fi
		if [ ! -f "$VEMA_DIR/vema.cpp" ]; then
			echo "Downloading vema source..."
			$DL_CMD $VEMA_DIR/vema.cpp $VEMA_URL/vema.c
		fi
		if [ -f "$VEMA_DIR/vema.h" ] && [ -f "$VEMA_DIR/vema.cpp" ]; then
			echo "Using vema."
			VEMA_SRCS="$VEMA_DIR/vema.cpp"
			VEMA_OPTS="-I $VEMA_DIR -DXD_USE_VEMA -DVEMA_NO_CLIB"
		else
			USE_VEMA=0
		fi
	else
		USE_VEMA=0
	fi
fi

CXX=${CXX:-$DEF_CXX}

DISASM_OPTS=""
case $SYS_ARCH in
	x86_64 | amd64 | i386 | i686 | i86pc)
		DISASM_OPTS="-M intel"
	;;
esac

echo "Compiling with $CXX..."
rm -f $PROG_PATH
$CXX $CXX_OPTS $VEMA_OPTS $PROG_OPTS $PROG_INCS $VEMA_SRCS $PROG_SRCS $PROG_LIBS -o $PROG_PATH $*
MSG="Failed."
if [ -f $PROG_PATH ]; then
	objdump $DISASM_OPTS -dC $PROG_PATH > $PROG_PATH.txt
	cp $CP_OPTS src/cmd/roof.sh $PROG_DIR
	cp $CP_OPTS src/cmd/roof_low.sh $PROG_DIR
	cp $CP_OPTS src/cmd/lot_low.sh $PROG_DIR
	MSG="Done."
fi
ls $LS_OPTS $PROG_PATH
echo $MSG
