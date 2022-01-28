#!/bin/sh
NWRK=4
SYSNAME=`uname -s`
case $SYSNAME in
	Linux)
		NWRK=$(nproc)
	;;
	OpenBSD)
		NWRK=`sysctl -n hw.ncpuonline`
	;;
esac
./crosscore_demo -nwrk:$NWRK -demo:lot -mode:1 -draw:ogl -bump:0 -spec:0 -w:720 -h:480 -smap:256 -tlod_bias:0 -vl:1 -lowq:1 -adapt:1 $*
