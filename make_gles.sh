#!/bin/sh
make GLES=1 -j$(nproc) $*
