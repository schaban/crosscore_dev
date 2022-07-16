#!/bin/sh
if [ ! -d "inc/GL" ]; then mkdir -p inc/GL; fi
for kh in glext.h glcorearb.h wglext.h
do
	wget -P inc/GL https://registry.khronos.org/OpenGL/api/GL/$kh
done
if [ ! -d "inc/KHR" ]; then mkdir -p inc/KHR; fi
wget -P inc/KHR https://registry.khronos.org/EGL/api/KHR/khrplatform.h
