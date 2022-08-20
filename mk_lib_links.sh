#!/bin/sh
if [ ! -d "lib" ]; then mkdir -p lib; fi

case `uname -m` in
	armv7l)
		libDir=/lib/arm-linux-gnueabihf
	;;
	*)
		libDir=/lib/`uname -m`-linux-gnu
	;;
esac

for lib in X11 EGL GLESv2 vulkan
do
	libBase=$libDir/lib$lib.so
	libPath=`ls -1 $libBase.[0-9]*.* | head -1`
	if [ -f "$libPath" ]
	then
		echo $libPath
		ln -s $libPath ./lib/lib$lib.so
	fi
done

