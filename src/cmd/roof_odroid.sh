#!/bin/sh
./crosscore_demo -demo:roof -draw:ogl -bump:0 -spec:0 -smap:512 -tlod_bias:0 -nwrk:0 -lowq:1 -vl:1 -adapt:1 -viewrot:1 -glsl_echo:1 -kbd_dev:/dev/input/by-path/`ls /dev/input/by-path | grep "joypad-event"` $*
