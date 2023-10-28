#!/bin/sh

clear

echo
echo -------- wasm NxN mul
node $* perf_nnmul.js -nmuls:100

echo
echo -------- wasm seg intersect
node $* perf_isect.js

echo
echo -------- wasm make AABB-tree
node $* perf_mkbvh.js -n:100000

echo
echo -------- SH pano
node $* perf_shpano.js -w:1024 -h:512
