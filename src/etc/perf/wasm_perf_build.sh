#!/bin/sh

DEF_CXX="emcc -s ASSERTIONS=0 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -lnodefs.js"
CXX=${CXX:-$DEF_CXX}

OPTI_OPTS="-O3 -flto"

CXX_CMD="$CXX -I ../.. ../../crosscore.cpp $OPTI_OPTS"

$CXX_CMD -D_MTX_N_=100 perf_nnmul.cpp -o perf_nnmul.js $*
$CXX_CMD perf_isect.cpp -o perf_isect.js $*
$CXX_CMD perf_mkbvh.cpp -o perf_mkbvh.js $*
$CXX_CMD perf_shpano.cpp -o perf_shpano.js $*
