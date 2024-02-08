/*
 * OpenGL system interface
 * Author: Sergey Chaban <sergey.chaban@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2019-2022 Sergey Chaban
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef OGLSYS_ES
#	define OGLSYS_ES 0
#endif

#include <cstdint>

#if defined(__OpenBSD__)
#	define OGLSYS_BSD
#	define OGLSYS_OPENBSD
#elif defined(__FreeBSD__)
#	define OGLSYS_BSD
#	define OGLSYS_FREEBSD
#elif defined(__linux__)
#	define OGLSYS_LINUX
#elif defined(__sun__)
#	define OGLSYS_SUNOS
#	if defined(__svr4__)
#		define OGLSYS_SOLARIS
#	endif
#	if defined(__illumos__)
#		define OGLSYS_ILLUMOS
#	endif
#endif

#if defined(ANDROID)
#	define OGLSYS_ANDROID
#	undef OGLSYS_ES
#	define OGLSYS_ES 1
#	define GL_GLEXT_PROTOTYPES
#	include <EGL/egl.h>
#	include <EGL/eglext.h>
#	include <GLES/gl.h>
#	include <GLES2/gl2.h>
#	include <GLES2/gl2ext.h>
#	include <GLES3/gl3.h>
#	include <GLES3/gl31.h>
#	include <GLES3/gl32.h>
#	include <android_native_app_glue.h>
android_app* oglsys_get_app();
#elif defined(X11)
#	define OGLSYS_X11
#	if OGLSYS_ES
#		if OGLSYS_USE_DYNAMICGLES
#			define DYNAMICGLES_NO_NAMESPACE
#			define DYNAMICEGL_NO_NAMESPACE
#			include <DynamicGles.h>
#		else
#			define GL_GLEXT_PROTOTYPES
#			include <EGL/egl.h>
#			include <EGL/eglext.h>
#			include <GLES2/gl2.h>
#			include <GLES2/gl2ext.h>
#			include <GLES3/gl3.h>
#			include <GLES3/gl31.h>
#			include <GLES3/gl32.h>
#		endif
#	else
#		include <GL/glcorearb.h>
#		include <GL/glext.h>
#		define OGL_FN(_type, _name) extern PFNGL##_type##PROC gl##_name;
#		undef OGL_FN_CORE
#		define OGL_FN_CORE
#		undef OGL_FN_EXTRA
#		define OGL_FN_EXTRA
#		include "oglsys.inc"
#		undef OGL_FN
#	endif
#elif defined(VIVANTE_FB)
#	define OGLSYS_VIVANTE_FB
#	undef OGLSYS_ES
#	define OGLSYS_ES 1
#	include <EGL/egl.h>
#	include <EGL/eglext.h>
#	include <EGL/eglvivante.h>
#	include <GLES/gl.h>
#	include <GLES/glext.h>
#	include <GLES2/gl2.h>
#	include <GLES2/gl2ext.h>
#	include <GLES3/gl3.h>
#elif defined(DRM_ES)
#	define OGLSYS_DRM_ES
#	undef OGLSYS_ES
#	define OGLSYS_ES 1
#	include <GLES2/gl2.h>
#	include <GLES2/gl2ext.h>
#	include <GLES3/gl3.h>
#elif defined(OGLSYS_WEB)
#	undef OGLSYS_ES
#	define OGLSYS_ES 0
#	include <GLES2/gl2.h>
#	include <GLES2/gl2ext.h>
#	include <GLES3/gl3.h>
#	ifndef GL_HALF_FLOAT
#		define GL_HALF_FLOAT 0x140B
#	endif
#elif defined(DUMMY_GL)
#	define WIN32_LEAN_AND_MEAN 1
#	undef NOMINMAX
#	define NOMINMAX
#	define OGLSYS_DUMMY
#	include <GL/glcorearb.h>
#	include <GL/glext.h>
#	define OGL_FN(_type, _name) extern PFNGL##_type##PROC gl##_name;
#	undef OGL_FN_CORE
#	define OGL_FN_CORE
#	undef OGL_FN_EXTRA
#	define OGL_FN_EXTRA
#	include "oglsys.inc"
#	undef OGL_FN
#elif defined(__APPLE__)
#	define OGLSYS_APPLE
#	define OGLSYS_MACOS
#	include <OpenGL/gl3.h>
#else
#	define OGLSYS_WINDOWS
#	undef _WIN32_WINNT
#	define _WIN32_WINNT 0x0500
#	if OGLSYS_ES
#		define DYNAMICGLES_NO_NAMESPACE
#		define DYNAMICEGL_NO_NAMESPACE
#		include <DynamicGles.h>
#	else
#		define WIN32_LEAN_AND_MEAN 1
#		undef NOMINMAX
#		define NOMINMAX
#		include <Windows.h>
#		include <tchar.h>
#		include <GL/glcorearb.h>
#		include <GL/glext.h>
#		include <GL/wglext.h>
#		define OGL_FN(_type, _name) extern PFNGL##_type##PROC gl##_name;
#		undef OGL_FN_CORE
#		define OGL_FN_CORE
#		undef OGL_FN_EXTRA
#		define OGL_FN_EXTRA
#		include "oglsys.inc"
#		undef OGL_FN
#endif
#endif

#if defined(OGLSYS_CL)
#	if !defined(OGLSYS_WINDOWS) && !defined(__linux__)
#		undef OGLSYS_CL
#		define OGLSYS_CL 0
#	endif
#else
#	define OGLSYS_CL 0
#endif

#if OGLSYS_CL
#	define CL_TARGET_OPENCL_VERSION 120
#	include <CL/cl_icd.h>
#endif

#define OGLSYS_ARY_LEN(_arr) (sizeof((_arr)) / sizeof((_arr)[0]))

struct OGLSysIfc {
	void* (*mem_alloc)(size_t size, const char* pTag);
	void (*mem_free)(void* p);
	void (*dbg_msg)(const char* pFmt, ...);
	const char* (*load_glsl)(const char* pPath, size_t* pSize);
	void (*unload_glsl)(const char* pCode);
	const char* (*get_opt)(const char* pName);

	OGLSysIfc() {
		mem_alloc = nullptr;
		mem_free = nullptr;
		dbg_msg = nullptr;
		load_glsl = nullptr;
		unload_glsl = nullptr;
		get_opt = nullptr;
	}
};

struct OGLSysCfg {
	int x;
	int y;
	int width;
	int height;
	int msaa;
	OGLSysIfc ifc;
	bool reduceRes;
	bool hideSysIcons;
	bool withoutCtx;

	void clear();
};

struct OGLSysMouseState {
	enum BTN {
		OGLSYS_BTN_LEFT = 0,
		OGLSYS_BTN_MIDDLE,
		OGLSYS_BTN_RIGHT,
		OGLSYS_BTN_X,
		OGLSYS_BTN_Y
	};

	uint32_t mBtnOld;
	uint32_t mBtnNow;
	float mNowX;
	float mNowY;
	float mOldX;
	float mOldY;
	int32_t mWheel;

	bool ck_now(BTN id) const { return !!(mBtnNow & (1 << (uint32_t)id)); }
	bool ck_old(BTN id) const { return !!(mBtnOld & (1 << (uint32_t)id)); }
	bool ck_trg(BTN id) const { return !!((mBtnNow & (mBtnNow ^ mBtnOld)) & (1 << (uint32_t)id)); }
	bool ck_chg(BTN id) const { return !!((mBtnNow ^ mBtnOld) & (1 << (uint32_t)id)); }
};

struct OGLSysInput {
	enum ACT {
		OGLSYS_ACT_NONE,
		OGLSYS_ACT_DRAG,
		OGLSYS_ACT_HOVER,
		OGLSYS_ACT_DOWN,
		OGLSYS_ACT_UP
	};

	enum DEVICE {
		OGLSYS_MOUSE,
		OGLSYS_TOUCH
	};

	int device;
	int act;
	int id;
	int sysDevId;
	float absX;
	float absY;
	float relX;
	float relY;
	float pressure;
};

struct OGLSysParamsBuf {
	const GLchar* mpName;
	GLuint mProgId;
	GLuint mBlockIdx;
	GLuint mBufHandle;
	GLuint mBindingIdx;
	GLint mSize;
	int mNumFields;
	const GLchar** mpFieldNames;
	GLuint* mpFieldIndices;
	GLint* mpFieldOffsets;
	GLint* mpFieldTypes;
	void* mpStorage;
	bool mUpdateFlg;

	void init(GLuint progId, const GLchar* pName, GLuint binding, const GLchar** ppFieldNames, const int numFields);
	void reset();
	int find_field(const char* pName);
	bool ck_field_idx(const int idx) const { return idx >= 0 && idx < mNumFields; }
	void set(const int idx, const float x);
	void set(const char* pName, const float x) { set(find_field(pName), x); }
	void set(const int idx, const float x, const float y);
	void set(const char* pName, const float x, const float y) { set(find_field(pName), x, y); };
	void set(const int idx, const float x, const float y, const float z);
	void set(const char* pName, const float x, const float y, const float z) { set(find_field(pName), x, y, z); };
	void set(const int idx, const float x, const float y, const float z, const float w);
	void set(const char* pName, const float x, const float y, const float z, const float w) { set(find_field(pName), x, y, z, w); };
	void send();
	void bind();
};

namespace OGLSys {

	typedef void (*InputHandler)(const OGLSysInput& inp, void* pWk);

	void init(const OGLSysCfg& cfg);
	void reset();
	void stop();
	void quit();
	void swap();
	void loop(void (*pLoop)(void*), void* pLoopCtx = nullptr);
	bool valid();

	void flush();
	void finish();

	bool is_es();
	bool is_web();
	bool is_dummy();

	void* get_window();
	void* get_display();
	void* get_instance();

	void* mem_alloc(size_t size, const char* pTag);
	void mem_free(void* p);

	void set_dummygl_swap_func(void (*swap_func)());

	void set_swap_interval(const int ival);
	void bind_def_framebuf();

	int get_width();
	int get_height();
	uint64_t get_frame_count();

	void* get_proc_addr(const char* pName);

	GLuint compile_shader_str(const char* pSrc, size_t srcSize, GLenum kind);
	GLuint compile_shader_file(const char* pSrcPath, GLenum kind);
	GLuint link_draw_prog(GLuint sidVert, GLuint sidFrag);
	GLuint link_prog(const GLuint* pSIDs, const int nSIDs);
	void link_prog_id_nock(const GLuint pid);
	bool ck_link_status(const GLuint pid);
	bool link_prog_id(const GLuint pid);
	void* get_prog_bin(GLuint pid, size_t* pSize, GLenum* pFmt);
	void free_prog_bin(void* pBin);
	bool set_prog_bin(GLuint pid, GLenum fmt, const void* pBin, GLsizei len);

	void enable_msaa(const bool flg);

	GLuint create_timestamp();
	void delete_timestamp(GLuint handle);
	void put_timestamp(GLuint handle);
	GLuint64 get_timestamp(GLuint handle);
	void wait_timestamp(GLuint handle);

	GLuint gen_vao();
	void bind_vao(const GLuint vao);
	void del_vao(const GLuint vao);

	void draw_tris_base_vtx(const int ntris, const GLenum idxType, const intptr_t ibOrg, const int baseVtx);

	GLuint get_black_tex();
	GLuint get_white_tex();
	void set_tex2d_lod_bias(const int bias);
	GLint get_max_vtx_texs();

	bool ext_ck_bindless_texs();
	bool ext_ck_derivatives();
	bool ext_ck_vtx_half();
	bool ext_ck_idx_uint();
	bool ext_ck_ASTC_LDR();
	bool ext_ck_S3TC();
	bool ext_ck_DXT1();
	bool ext_ck_prog_bin();
	bool ext_ck_shader_bin();
	bool ext_ck_spv();
	bool ext_ck_vao();
	bool ext_ck_vtx_base();
	bool ext_ck_mdi();
	bool ext_ck_nv_vbum();
	bool ext_ck_nv_ubum();
	bool ext_ck_nv_bindless_mdi();
	bool ext_ck_nv_cmd_list();

	bool get_key_state(const char code);
	bool get_key_state(const char* pName);

	void set_key_state(const char* pName, const bool state);

	void set_input_handler(InputHandler handler, void* pWk);

	OGLSysMouseState get_mouse_state();
	void send_mouse_down(OGLSysMouseState::BTN btn, float absX, float absY, bool updateFlg);
	void send_mouse_up(OGLSysMouseState::BTN btn, float absX, float absY, bool updateFlg);
	void send_mouse_move(float absX, float absY);

	namespace CL {
#if OGLSYS_CL
#		define OGLSYS_CL_FN(_name) extern cl_api_cl##_name _name;
#		include "oglsys.inc"
#		undef OGLSYS_CL_FN
#endif

		typedef void* Platform;
		typedef void* Device;
		typedef void* Context;
		typedef void* Buffer;
		typedef void* Queue;
		typedef void* Kernel;
		typedef void* Event;

		struct PlatformList {
			size_t num;
			struct Entry {
				Platform plat;
				const char* pVer;
				const char* pName;
				const char* pVendor;
				const char* pExts;
				Device defDev;
				uint32_t numDevs;
				uint32_t numCPU;
				uint32_t numGPU;
				uint32_t numAcc;
				bool fullProfile;
				bool coprFlg;
			} entries[1];
		};

		void init();
		void reset();
		bool valid();
		PlatformList* get_platform_list();
		void free_platform_list(PlatformList* pLst);
		Device get_device(Platform plat, const uint32_t idx = 0);
		uint32_t get_num_devices(Platform plat);
		uint32_t get_num_cpu_devices(Platform plat);
		uint32_t get_num_gpu_devices(Platform plat);
		uint32_t get_num_acc_devices(Platform plat);
		uint32_t get_device_max_units(Device dev);
		uint32_t get_device_max_freq(Device dev);
		double get_device_global_mem_size(Device dev);
		double get_device_local_mem_size(Device dev);
		double get_device_fast_mem_size(Device dev);
		bool device_has_local_mem(Device dev);
		bool device_supports_fp16(Device dev);
		bool device_supports_fp64(Device dev);
		bool device_is_byte_addressable(Device dev);
		bool device_is_intel_gpu(Device dev);
		bool device_is_vivante_gpu(Device dev);
		bool device_is_arm_gpu(Device dev);
		void print_device_exts(Device dev);
		Context create_device_context(Device dev);
		void destroy_device_context(Context ctx);
		Device device_from_context(Context ctx);
		Buffer create_host_mem_buffer(Context ctx, void* p, const size_t size, const bool read, const bool write);
		inline Buffer create_host_mem_in_buffer(Context ctx, void* p, const size_t size) { return create_host_mem_buffer(ctx, p, size, true, false); }
		inline Buffer create_host_mem_out_buffer(Context ctx, void* p, const size_t size) { return create_host_mem_buffer(ctx, p, size, false, true); }
		void release_buffer(Buffer buf);
		void* alloc_svm(Context ctx, size_t size);
		void free_svm(Context ctx, void* p);
		void* alloc_fast(Context ctx, size_t size);
		void free_fast(Context ctx, void* p);
		Queue create_queue(Context ctx);
		void release_queue(Queue que);
		void flush_queue(Queue que);
		void finish_queue(Queue que);
		void update_host_mem_in_buffer(Queue que, Buffer buf, void* p, const size_t size);
		void update_host_mem_out_buffer(Queue que, Buffer buf, void* p, const size_t size);
		void exec_kernel(Queue que, Kernel kern, const int numUnits, Event* pEvt = nullptr);
		Kernel create_kernel_from_src(Context ctx, const char* pSrc, const char* pEntryName, const char* pOpts = nullptr);
		void release_kernel(Kernel kern);
		void set_kernel_arg(Kernel kern, uint32_t idx, const void* pVal, size_t size);
		void set_kernel_int_arg(Kernel kern, uint32_t idx, const int val);
		void set_kernel_float_arg(Kernel kern, uint32_t idx, const float val);
		void set_kernel_float3_arg(Kernel kern, uint32_t idx, const float x, const float y, const float z);
		void set_kernel_buffer_arg(Kernel kern, uint32_t idx, const Buffer buf);
		void wait_event(Event evt);
		void release_event(Event evt);
		void wait_events(const Event* pEvts, const int n);
		bool event_ck_queued(Event evt);
		bool event_ck_submitted(Event evt);
		bool event_ck_running(Event evt);
		bool event_ck_complete(Event evt);
	} // CL
}  // OGLSys
