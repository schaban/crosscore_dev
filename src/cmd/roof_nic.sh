#!/bin/sh

NIC_TTY=/dev/pts/2

TTY_SIZE=`stty -F $NIC_TTY size`

NIC_W=`echo $TTY_SIZE | cut -d " " -f2`
NIC_H=$((`echo $TTY_SIZE | cut -d " " -f1` - 1))
echo "Nic vision:" $NIC_W "x" $NIC_H

PROGDIR=`dirname $0`
$PROGDIR/crosscore_demo -demo:roof -draw:ogl -bump:0 -spec:0 -smap:512 -tlod_bias:0 -lowq:1 -adapt:1 -nic_tty:$NIC_TTY -w:$NIC_W -h:$NIC_H $*
