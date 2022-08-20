#!/bin/sh
VK_H_REPO="https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vulkan"
VK_INC="inc/vulkan"
if [ ! -d "$VK_INC" ]; then mkdir -p $VK_INC; fi
for kh in vulkan.h vk_platform.h vulkan_core.h vulkan_win32.h vulkan_xlib.h
do
	wget -O $VK_INC/$kh $VK_H_REPO/$kh
done

