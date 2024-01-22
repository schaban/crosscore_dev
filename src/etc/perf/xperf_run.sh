#!/bin/sh

clear

echo
echo -------- NxN mul static
./perf_nnmul -nmuls:100

echo
echo -------- NxN mul dynamic
./perf_nnmul_dyn -nmuls:100

echo
echo -------- seg intersect
./perf_isect

echo
echo -------- make AABB-tree
./perf_mkbvh -n:100000

echo
echo -------- SH pano
./perf_shpano -w:1024 -h:512
