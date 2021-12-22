#!/bin/sh
gmake CXX=clang -j$(nproc) $*
