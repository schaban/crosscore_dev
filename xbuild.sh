#!/bin/sh

PROG_DIR=bin/`uname -s`_`uname -m`
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

case `uname -s` in
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
esac

CXX=${CXX:-$DEF_CXX}

DISASM_OPTS=""
case `uname -m` in
	x86_64 | amd64)
		DISASM_OPTS="-M intel"
	;;
esac

echo "Compiling with $CXX..."
$CXX $CXX_OPTS $PROG_OPTS $PROG_INCS $PROG_SRCS $PROG_LIBS -o $PROG_PATH $*
MSG="Failed."
if [ -f $PROG_PATH ]; then
	objdump $DISASM_OPTS -dC $PROG_PATH > $PROG_PATH.txt
	cp -v src/cmd/roof.sh $PROG_DIR
	cp -v src/cmd/roof_low.sh $PROG_DIR
	cp -v src/cmd/lot_low.sh $PROG_DIR
	MSG="Done."
fi
ls $LS_OPTS $PROG_PATH
echo $MSG
