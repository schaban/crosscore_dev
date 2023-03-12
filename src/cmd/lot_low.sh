#!/bin/sh
NWRK=2
SYSNAME=`uname -s`
case $SYSNAME in
	Linux)
		NWRK=$(nproc)
	;;
	OpenBSD)
		NWRK=`sysctl -n hw.ncpuonline`
	;;
	FreeBSD)
		NWRK=`sysctl -n kern.smp.cpus`
		echo "FreeBSD scheduler: `sysctl -n kern.sched.name`"
	;;
	SunOS)
		NCPUS=`psrinfo -t`
		NCORES=`psrinfo -tc`
		echo "$NCPUS CPUs on $NCORES cores"
		NWRK=$NCPUS
	;;
esac
PROGDIR=`dirname $0`
$PROGDIR/crosscore_demo -nwrk:$NWRK -demo:lot -mode:1 -draw:ogl -bump:0 -spec:0 -w:720 -h:480 -smap:256 -tlod_bias:0 -vl:1 -lowq:1 -adapt:1 $*
