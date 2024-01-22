#!/bin/sh

DEF_CXX="g++"
CXX=${CXX:-$DEF_CXX}

OPTI_OPTS="-O3 -ffast-math -flto"

CXX_CMD="$CXX -pthread -I ../.. ../../crosscore.cpp $OPTI_OPTS"

$CXX_CMD -D_MTX_N_=100 perf_nnmul.cpp -o perf_nnmul $*
$CXX_CMD perf_nnmul_dyn.cpp -o perf_nnmul_dyn $*
$CXX_CMD perf_isect.cpp -o perf_isect $*
$CXX_CMD perf_mkbvh.cpp -o perf_mkbvh $*
$CXX_CMD perf_shpano.cpp -o perf_shpano $*
