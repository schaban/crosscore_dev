#!/bin/sh

XDATA_ARC=xcore_web.tar.xz
XDATA_URL="https://schaban.github.io/crosscore_web_demo/$XDATA_ARC"
XDATA_TMP="xcore_web"
XDATA_BIN="$XDATA_TMP/bin"
XDATA_DIR="$XDATA_BIN/data"

DL_MODE="NONE"

case `uname -s` in
	Linux)
		if [ -x "`command -v wget`" ]; then
			DL_MODE="WGET_EXT"
		elif [ -x "`command -v curl`" ]; then
			DL_MODE="CURL"
		fi
	;;
	Darwin | MINGW* | CYGWIN*)
		DL_MODE="CURL"
	;;
	OpenBSD)
		DL_MODE="WGET_STD"
	;;
	*)
		if [ -x "`command -v curl`" ]; then
			DL_MODE="CURL"
		elif [ -x "`command -v wget`" ]; then
			DL_MODE="WGET_STD"
		fi
	;;
esac

case $DL_MODE in
	WGET_EXT)
		wget -q -O - $XDATA_URL | (tar --strip-components=1 -xvJf - $XDATA_DIR)
	;;
	CURL)
		if [ ! -d "bin" ]; then mkdir -p bin; fi
		if [ ! -d "$XDATA_TMP" ]; then mkdir -p $XDATA_TMP; fi
		curl $XDATA_URL -o $XDATA_TMP/$XDATA_ARC
		tar -xvJf $XDATA_TMP/$XDATA_ARC $XDATA_DIR
		mv $XDATA_DIR bin
		rm -f $XDATA_TMP/$XDATA_ARC
		rmdir -p $XDATA_BIN
	;;
	WGET_STD)
		if [ ! -d "bin" ]; then mkdir -p bin; fi
		wget -q -O - $XDATA_URL | (xzcat -) | (tar -xvf - $XDATA_DIR)
		mv $XDATA_DIR bin
		rmdir -p $XDATA_BIN
	;;
	*)
		echo "Unknown d/l method."
		exit 1
	;;
esac

