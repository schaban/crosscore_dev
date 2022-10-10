#!/bin/sh

if [ ! -d "tmp/obj" ]; then mkdir -p tmp/obj; fi
if [ ! -d "bin/mac" ]; then mkdir -p bin/mac; fi

MAC_EXE_DIR=bin/mac
MAC_EXE=$MAC_EXE_DIR/crosscore_demo
MAC_LIBS="-framework Foundation -framework Cocoa -framework OpenGL -Xlinker -w"
MAC_OBJ=tmp/obj/mac_main.o

rm -f $MAC_OBJ
rm -f $MAC_EXE

echo "Compiling macOS adapter..."
clang -I src/mac -Wno-deprecated-declarations src/mac/mac_main.m -c -o $MAC_OBJ
if [ ! -f $MAC_OBJ ]; then
	echo "Failed."
	exit 1
fi
echo "OK"

echo "Compiling main code..."
clang++ -std=c++11 -I src/mac -DDEF_DEMO="\"roof\"" `ls src/*.cpp` $MAC_OBJ -o $MAC_EXE $MAC_LIBS $*
if [ ! -f $MAC_EXE ]; then
	echo "Failed."
	exit 1
fi
echo "OK"
cp -v src/cmd/roof.sh $MAC_EXE_DIR
cp -v src/cmd/roof_low.sh $MAC_EXE_DIR
ls -Go $MAC_EXE
