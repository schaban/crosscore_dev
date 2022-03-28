#!/bin/sh
XDATA_ARC=xcore_web.tar.xz
XDATA_URL="https://schaban.github.io/crosscore_web_demo/$XDATA_ARC"
XDATA_TMP="xcore_web"
XDATA_BIN="$XDATA_TMP/bin"
XDATA_DIR="$XDATA_BIN/data"

case `uname -s` in
	Linux)
		wget -q -O - $XDATA_URL | (tar --strip-components=1 -xvJf - $XDATA_DIR)
	;;
	Darwin | MINGW* | CYGWIN*)
		if [ ! -d "bin" ]; then mkdir -p bin; fi
		if [ ! -d "$XDATA_TMP" ]; then mkdir -p $XDATA_TMP; fi
		curl $XDATA_URL -o $XDATA_TMP/$XDATA_ARC
		tar -xvJf $XDATA_TMP/$XDATA_ARC $XDATA_DIR
		mv $XDATA_DIR bin
		rm -f $XDATA_TMP/$XDATA_ARC
		rmdir -p $XDATA_BIN
	;;
	*)
		if [ ! -d "bin" ]; then mkdir -p bin; fi
		wget -q -O - $XDATA_URL | (xzcat -) | (tar -xvf - $XDATA_DIR)
		mv $XDATA_DIR bin
		rmdir -p $XDATA_BIN
	;;
esac
