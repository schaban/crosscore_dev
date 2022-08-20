#!/bin/sh
if [ ! -d "inc/GL" ]; then mkdir -p inc/GL; fi
for kh in glext.h glcorearb.h wglext.h
do
	wget -O inc/GL/$kh https://registry.khronos.org/OpenGL/api/GL/$kh
done
if [ ! -d "inc/KHR" ]; then mkdir -p inc/KHR; fi
wget -O inc/KHR/khrplatform.h https://registry.khronos.org/EGL/api/KHR/khrplatform.h
