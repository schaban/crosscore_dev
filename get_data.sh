#!/bin/sh
wget -q -O - https://schaban.github.io/crosscore_web_demo/xcore_web.tar.xz | (tar --strip-components=1 -xvJf - xcore_web/bin/data)
