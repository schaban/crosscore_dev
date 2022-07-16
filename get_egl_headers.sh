#!/bin/sh
if [ ! -d "inc/EGL" ]; then mkdir -p inc/GL; fi
for kh in egl.h eglext.h eglplatform.h
do
	wget -P inc/EGL https://registry.khronos.org/EGL/api/EGL/$kh
done

