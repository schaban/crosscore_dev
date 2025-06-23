#!/bin/sh

DEF_CXX="g++"
CXX=${CXX:-$DEF_CXX}

OPTI_OPTS="-O3 -ffast-math -flto"

CXX_CMD="$CXX -pthread -I .. ../crosscore.cpp $OPTI_OPTS"

$CXX_CMD tst_nnmul_h.cpp -o tst_nnmul_h $*
