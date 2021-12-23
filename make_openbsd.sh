#!/bin/sh
gmake CXX=clang -j$(getconf NPROCESSORS_ONLN) $*
