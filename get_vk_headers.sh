#!/bin/sh

TMP_DIR=tmp
INC_DIR=inc

if [ ! -d "$TMP_DIR" ]; then mkdir -p $TMP_DIR; fi
if [ ! -d "$INC_DIR" ]; then mkdir -p $INC_DIR; fi

BUILD_DIR=$PWD

cd $TMP_DIR
git clone --depth 1 https://github.com/KhronosGroup/Vulkan-Headers.git
cd $BUILD_DIR
cp -r $TMP_DIR/Vulkan-Headers/include/vk_video $INC_DIR
cp -r $TMP_DIR/Vulkan-Headers/include/vulkan $INC_DIR
