#!/bin/sh

SYS_ARCH=`uname -m`
DISASM_OPTS=""
case $SYS_ARCH in
	x86_64 | amd64 | i386 | i686 | i86pc)
		DISASM_OPTS="-M intel"
	;;
esac

if [ "$#" -gt 0 ]; then
	objdump $DISASM_OPTS -SC $1 > $1.txt
fi
