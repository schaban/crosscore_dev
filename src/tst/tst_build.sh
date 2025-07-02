#!/bin/sh

DEF_CXX="g++"
CXX=${CXX:-$DEF_CXX}

# -DXD_HAS_F16=1
# x86_64: -mf16c -mfma
# aarch64: -march=armv8.2-a+fp16

OPTI_OPTS="-O3 -ffast-math -flto"

CXX_CMD="$CXX -pthread -std=c++11 -I .. ../crosscore.cpp $OPTI_OPTS"

$CXX_CMD tst_nnmul_h.cpp -o tst_nnmul_h $*
