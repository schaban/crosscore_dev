#!/bin/sh
./crosscore_demo -demo:roof -draw:ogl -bump:0 -spec:0 -smap:256 -tlod_bias:0 -nwrk:2 -lowq:1 -vl:1 -adapt:4 -gpu_preload:1 -glsl_echo:1 -w:720 -h:480 $*
