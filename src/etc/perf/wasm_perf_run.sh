#!/bin/sh

XNODE=${XNODE:-node}

clear

echo
echo -------- wasm NxN mul
$XNODE $* perf_nnmul.js -nmuls:100

echo
echo -------- wasm seg intersect
$XNODE $* perf_isect.js

echo
echo -------- wasm make AABB-tree
$XNODE $* perf_mkbvh.js -n:100000

echo
echo -------- SH pano
$XNODE $* perf_shpano.js -w:1024 -h:512
