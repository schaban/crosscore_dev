#!/bin/sh
PROGDIR=`dirname $0`
$PROGDIR/crosscore_demo -demo:roof -draw:ogl -bump:1 -spec:1 -w:1200 -h:700 -smap:4096 -spersp:1 -adapt:1 $*
