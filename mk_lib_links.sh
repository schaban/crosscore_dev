#!/bin/sh
if [ ! -d "lib" ]; then mkdir -p lib; fi
for lib in X11 EGL GLESv2
do
	libBase=/lib/`uname -m`-linux-gnu/lib$lib.so
	libPath=`ls -1 $libBase.[0-9]*.* | head -1`
	echo $libPath
	ln -s $libPath ./lib/lib$lib.so
done

