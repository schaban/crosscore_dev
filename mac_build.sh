#!/bin/sh
if [ ! -d "tmp/obj" ]; then mkdir -p tmp/obj; fi
if [ ! -d "bin/mac" ]; then mkdir -p bin/mac; fi
MAC_FWS="-framework OpenGL -framework Foundation -framework Cocoa"
clang -I src/mac src/mac/mac_main.m -c -o tmp/obj/mac_main.o
clang++ -std=c++11 -I src/mac -DDEF_DEMO="\"roof\"" `ls src/*.cpp` tmp/obj/mac_main.o -o bin/mac/x.out $MAC_FWS $*
