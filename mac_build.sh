#!/bin/sh
if [ ! -d "tmp/obj" ]; then mkdir -p tmp/obj; fi
if [ ! -d "bin/mac" ]; then mkdir -p bin/mac; fi
MAC_EXE=bin/mac/x.out
MAC_LIBS="-framework OpenGL -framework Foundation -framework Cocoa"
rm -f $MAC_EXE
clang -I src/mac src/mac/mac_main.m -c -o tmp/obj/mac_main.o
clang++ -std=c++11 -I src/mac -DDEF_DEMO="\"roof\"" `ls src/*.cpp` tmp/obj/mac_main.o -o $MAC_EXE $MAC_LIBS $*
echo "-> exe: " `ls -o $MAC_EXE`
