#!/bin/sh
if [ ! -d "inc/GLES2" ]; then mkdir -p inc/GLES2; fi
for kh in gl2.h gl2ext.h gl2platform.h
do
	wget -O inc/GLES2/$kh https://registry.khronos.org/OpenGL/api/GLES2/$kh
done

if [ ! -d "inc/GLES3" ]; then mkdir -p inc/GLES3; fi
for kh in gl3.h gl31.h gl32.h gl3platform.h
do
	wget -O inc/GLES3/$kh https://registry.khronos.org/OpenGL/api/GLES3/$kh
done
