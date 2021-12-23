#!/bin/sh
XDATA_URL="https://schaban.github.io/crosscore_web_demo/xcore_web.tar.xz"
XDATA_BIN="xcore_web/bin"
XDATA_DIR="$XDATA_BIN/data"

if [ "$(uname -s)" = "Linux" ]; then
	wget -q -O - $XDATA_URL | (tar --strip-components=1 -xvJf - $XDATA_DIR)
else
	if [ ! -d "bin" ]; then mkdir -p bin; fi
	wget -q -O - $XDATA_URL | (xzcat -) | (tar -xvf - $XDATA_DIR)
	mv $XDATA_DIR bin
	rmdir -p $XDATA_BIN
fi
