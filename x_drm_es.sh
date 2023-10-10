#!/bin/sh

DEF_PREFIX=~/Tools/crosscompile/gcc-aarch64/bin/aarch64-none-linux-gnu-

if [ "$#" -gt 0 ]; then
	if [ "$1" = "rv" ]; then
		DEF_PREFIX=~/Tools/crosscompile/gcc-riscv64/bin/riscv64-unknown-linux-gnu-
		shift
	fi
fi

TOOL_PREFIX=${TOOL_PREFIX:-$DEF_PREFIX}

DEF_CXX=${TOOL_PREFIX}g++
DEF_DISASM=${TOOL_PREFIX}objdump

CXX=${CXX:-$DEF_CXX}
CXX_OPTS="-std=c++11 -ffast-math -ftree-vectorize -pthread"
TGT_DIR=${TGT_DIR:-./tgt}

DISASM=${DISASM:-$DEF_DISASM}

if [ ! -d "$TGT_DIR" ]; then
	echo "no target dir"
	exit
fi

TGT_INC=$TGT_DIR/inc
TGT_LIB=$TGT_DIR/lib

PROG_INCS="-I src -I inc -I $TGT_INC -I $TGT_INC/drm -I $TGT_INC/drm/drm"
PROG_SRCS="`ls src/*.cpp`"
PROG_LIBS="-L$TGT_LIB -lgbm -ldrm -lEGL -lGLESv2 -ldl"
PROG_DEFS="-DEGL_API_FB -D__DRM__ -DDRM_ES -DOGLSYS_LINUX_INPUT"

EXE_PATH=$TGT_DIR/crosscore_demo
echo "Compiling $EXE_PATH with $CXX..."
$CXX $CXX_OPTS $PROG_INCS $PROG_SRCS $PROG_DEFS $PROG_LIBS -o $EXE_PATH $*
if [ -f $EXE_PATH ]; then
	echo "Compiled OK, disassembling..."
	$DISASM -SC $EXE_PATH > $EXE_PATH.txt
fi
