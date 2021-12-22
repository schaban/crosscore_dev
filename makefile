PROG_NAME:=crosscore_demo

XCORE_EXE_DIR:=bin/$(shell uname -s)_$(shell uname -m)

$(shell if [ ! -d "tmp/obj" ]; then mkdir -p tmp/obj; fi)
$(shell if [ ! -d "tmp/obj/dbg" ]; then mkdir -p tmp/obj/dbg; fi)
$(shell if [ ! -d "$(XCORE_EXE_DIR)" ]; then mkdir -p $(XCORE_EXE_DIR); fi)

SUFFIXES += .d

XCORE_EXE:=$(XCORE_EXE_DIR)/$(PROG_NAME)
XCORE_EXE_DBG:=$(XCORE_EXE_DIR)/$(PROG_NAME)_dbg
.DEFAULT_GOAL := $(XCORE_EXE)

CPP_FLAGS:=-std=c++11 -march=native -ffast-math -ftree-vectorize -g -Wno-psabi -Wno-deprecated-declarations
CPP_OPTIM:=-O3 -flto
CPP_DEBUG:=-Og -ggdb

ifeq ("$(shell uname -m)", "armv7l")
	CPP_OPTIM+=-mfpu=neon
endif

VULKAN_SO_NAME:=libvulkan.so.1
VULKAN_SO_PATH:=/usr/lib/$(shell uname -m)-linux-gnu/$(VULKAN_SO_NAME)

LINK_LIBS:=-lm -lpthread -lstdc++

XCORE_FLAGS:=-DXD_TSK_NATIVE=1 -DDEF_DEMO="\"roof\"" -I src -I inc
ifeq ("$(wildcard $(VULKAN_SO_PATH))","")
	XCORE_FLAGS += -DDRW_NO_VULKAN=1
else
	XCORE_FLAGS += -DDRW_NO_VULKAN=0
	LINK_LIBS += -l:$(VULKAN_SO_NAME)
endif
ifdef USE_CL
	XCORE_FLAGS += -DOGLSYS_CL=$(USE_CL)
endif

ifeq ("$(shell uname -s)", "OpenBSD")
	LINK_LIBS += -L/usr/X11R6/lib
	XCORE_FLAGS += -I/usr/X11R6/include
else
	LINK_LIBS += -ldl
endif

ifdef VIVANTE_FB
	XCORE_FLAGS += -DEGL_API_FB -DVIVANTE_FB
	LINK_LIBS += -lEGL -lGLESv2 -L/usr/lib/galcore/fb/
else ifdef VIVANTE_DRM
	XCORE_FLAGS += -I/usr/include/libdrm -DEGL_API_FB -D__DRM__ -DDRM_ES
	LINK_LIBS += -lgbm -ldrm -lEGL -lGLESv2 -L/usr/lib/galcore/fb/
else ifdef DRM_ES
	XCORE_FLAGS += -I/usr/include/libdrm -DEGL_API_FB -D__DRM__ -DDRM_ES
	LINK_LIBS += -lgbm -ldrm -lEGL -lGLESv2
else ifdef GLES
	XCORE_FLAGS += -DOGLSYS_ES=1 -DX11
	LINK_LIBS += -lEGL -lGLESv2 -lX11
else
	XCORE_FLAGS += -DX11
	LINK_LIBS += -lX11
endif

LINK_LIBS += -Llib

ifdef LINUX_INPUT
	XCORE_FLAGS += -DOGLSYS_LINUX_INPUT
endif

XCORE_CPP:=$(CXX) $(CPP_OPTIM) $(CPP_FLAGS) $(XCORE_FLAGS)
XCORE_LINK:=$(CXX) $(CPP_OPTIM) $(CPP_FLAGS)

XCORE_CPP_DBG:=$(CXX) $(CPP_DEBUG) $(CPP_FLAGS) $(XCORE_FLAGS)
XCORE_LINK_DBG:=$(CXX) $(CPP_DEBUG) $(CPP_FLAGS)

$(shell if [ ! -d "tmp/dep" ]; then mkdir -p tmp/dep; fi)
$(shell if [ ! -d "tmp/dep/dbg" ]; then mkdir -p tmp/dep/dbg; fi)

SRCS:=$(shell ls -1 src/*.cpp)
DEPS:=$(patsubst src/%.cpp,tmp/dep/%.d,$(SRCS))
DEPS_DBG:=$(patsubst src/%.cpp,tmp/dep/dbg/%.d,$(SRCS))

-include $(DEPS)
-include $(DEPS_DBG)

tmp/dep/%.d: src/%.cpp
	@$(XCORE_CPP) -MM -MT "$(patsubst src/%.cpp,tmp/obj/%.o,$<)" $< -MF $@

tmp/dep/dbg/%.d: src/%.cpp
	@$(XCORE_CPP_DBG) -MM -MT "$(patsubst src/%.cpp,tmp/obj/dbg/%.o,$<)" $< -MF $@


debug: $(XCORE_EXE_DBG)

OBJS:=$(patsubst src/%.cpp,tmp/obj/%.o,$(SRCS))
OBJS_DBG:=$(patsubst src/%.cpp,tmp/obj/dbg/%.o,$(SRCS))

tmp/obj/%.o: src/%.cpp tmp/dep/%.d
	$(info Compiling $<)
	@$(XCORE_CPP) -c -o $@ $<

tmp/obj/dbg/%.o: src/%.cpp tmp/dep/dbg/%.d
	$(info Compiling $< (Debug))
	@$(XCORE_CPP_DBG) -c -o $@ $<

$(XCORE_EXE): $(OBJS)
	$(info Linking $(XCORE_EXE))
	@$(XCORE_LINK) $^ -o $@ $(LINK_LIBS)
	@objdump -d $@ > $@.txt
ifdef ODROID
	@cp src/cmd/roof_odroid.sh $(XCORE_EXE_DIR)
	@cp src/cmd/roof_odroid_kbd.sh $(XCORE_EXE_DIR)
else
	@cp src/cmd/roof.sh $(XCORE_EXE_DIR)
	@cp src/cmd/roof_low.sh $(XCORE_EXE_DIR)
endif

$(XCORE_EXE_DBG): $(OBJS_DBG)
	$(info Linking $(XCORE_EXE_DBG))
	@$(XCORE_LINK_DBG) $^ -o $@ $(LINK_LIBS)
	@objdump -d $@ > $@.txt

.PHONY=clean
clean:
	$(info Cleaning $(XCORE_EXE))
	$(info Removing objs...)
	@rm --force $(OBJS)
	@rm --force $(OBJS_DBG)
	$(info Removing deps...)
	@rm --force $(DEPS)
	@rm --force $(DEPS_DBG)
	$(info Removing exes...)
	@rm --force $(XCORE_EXE)
	@rm --force $(XCORE_EXE).txt
	@rm --force $(XCORE_EXE_DBG)
	@rm --force $(XCORE_EXE_DBG).txt
	$(info Done.)

