#!/bin/sh
if [ ! -d "inc/EGL" ]; then mkdir -p inc/GL; fi
for kh in egl.h eglext.h eglplatform.h
do
	wget -P inc/EGL https://www.khronos.org/registry/EGL/api/EGL/$kh
done

