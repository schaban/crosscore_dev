#!/bin/sh
if [ ! -d "inc/EGL" ]; then mkdir -p inc/EGL; fi
for kh in egl.h eglext.h eglplatform.h
do
	wget -O inc/EGL/$kh https://registry.khronos.org/EGL/api/EGL/$kh
done

