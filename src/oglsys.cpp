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

#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "oglsys.hpp"

#if defined(OGLSYS_ANDROID)
#	include <android/window.h>
#elif defined(OGLSYS_X11)
#	include <unistd.h>
#	include <X11/Xlib.h>
#	include <X11/Xutil.h>
#elif defined(OGLSYS_DRM_ES)
#	include <fcntl.h>
#	include <drm.h>
#	include <gbm.h>
#	include <xf86drm.h>
#	include <xf86drmMode.h>
#	include <EGL/egl.h>
#	include <EGL/eglext.h>
#	if defined(OGLSYS_BSD)
#		include <sys/select.h>
#	endif
#elif defined(OGLSYS_WEB)
#	include <emscripten.h>
#	include <SDL.h>
#endif

#if defined(OGLSYS_LINUX_INPUT)
#	include <unistd.h>
#	include <fcntl.h>
#	include <linux/input.h>
#endif

#if !OGLSYS_ES
#	if defined(OGLSYS_WINDOWS)
#		if 0
#		define GET_GL_FN(_name) *(PROC*)&_name = (PROC)get_func_ptr(#_name)
#		define GET_WGL_FN(_name) *(PROC*)&mWGL._name = (PROC)get_func_ptr(#_name)
#		else
		static PROC* proc_addr(void* funcPtr) { return (PROC*)funcPtr; }
#		define GET_GL_FN(_name) *proc_addr(&_name) = (PROC)get_func_ptr(#_name)
#		define GET_WGL_FN(_name) *proc_addr(&mWGL._name) = (PROC)get_func_ptr(#_name)
#		endif
//
#		define OGL_FN(_type, _name) PFNGL##_type##PROC gl##_name;
#		undef OGL_FN_CORE
#		define OGL_FN_CORE
#		undef OGL_FN_EXTRA
#		define OGL_FN_EXTRA
#		include "oglsys.inc"
#		undef OGL_FN
#	elif defined(OGLSYS_X11)
#		include <dlfcn.h>
		typedef void* GLXContext;
		typedef XID GLXDrawable;
#		define GLX_RGBA 4
#		define GLX_DOUBLEBUFFER	5
#		define GLX_DEPTH_SIZE 12
#		define GLX_SAMPLE_BUFFERS 0x186a0
#		define GLX_SAMPLES 0x186a1
#		define GET_GL_FN(_name) *(void**)&_name = mGLX.get_func_ptr(#_name)
#		define OGL_FN(_type, _name) PFNGL##_type##PROC gl##_name;
#		undef OGL_FN_CORE
#		define OGL_FN_CORE
#		undef OGL_FN_EXTRA
#		define OGL_FN_EXTRA
#		include "oglsys.inc"
#		undef OGL_FN
#	elif defined(OGLSYS_DUMMY)
#		define OGL_FN(_type, _name) PFNGL##_type##PROC gl##_name;
#		undef OGL_FN_CORE
#		define OGL_FN_CORE
#		undef OGL_FN_EXTRA
#		define OGL_FN_EXTRA
#		include "oglsys.inc"
#		undef OGL_FN
		namespace OGLSys { void dummyglInit(); }
#	endif
#endif // !OGLSYS_ES

#ifndef DUMMYGL_WINKBD
#	define DUMMYGL_WINKBD 0
#	if defined(OGLSYS_DUMMY)
#		if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#			undef DUMMYGL_WINKBD
#			define DUMMYGL_WINKBD 1
#			define WIN32_LEAN_AND_MEAN 1
#			undef NOMINMAX
#			define NOMINMAX
#			include <Windows.h>
#		endif
#	endif
#endif

#if (!defined(OGLSYS_WINDOWS) && !defined(OGLSYS_DUMMY) && !defined(OGLSYS_APPLE) && !defined(OGLSYS_ANDROID)) || (defined(OGLSYS_DUMMY) && OGLSYS_CL && defined(OGLSYS_LINUX))
#include <dlfcn.h>
namespace OGLSys {
	void* dlopen_from_list(const char** pLibs, const size_t nlibs) {
		void* pLib = nullptr;
		for (size_t i = 0; i < nlibs; ++i) {
			pLib = ::dlopen(pLibs[i], RTLD_LAZY | RTLD_GLOBAL);
			if (pLib) break;
		}
		return pLib;
	}
}
#endif

enum KBD_PUNCT {
	KBD_PUNCT_SPACE = 0,
	KBD_PUNCT_PLUS,
	KBD_PUNCT_MINUS,
	KBD_PUNCT_COMMA,
	KBD_PUNCT_PERIOD,
	KBD_PUNCT_SLASH,
	KBD_PUNCT_COLON,
	_KBD_PUNCT_NUM_
};

static struct KBD_PUNCT_XLAT {
	int code, idx;
} s_kbdPunctTbl[] = {
	{ ' ', KBD_PUNCT_SPACE },
	{ '+', KBD_PUNCT_PLUS },
	{ '-', KBD_PUNCT_PLUS },
	{ ',', KBD_PUNCT_COMMA },
	{ '.', KBD_PUNCT_PERIOD },
};

enum KBD_CTRL {
	KBD_CTRL_UP = 0,
	KBD_CTRL_DOWN,
	KBD_CTRL_LEFT,
	KBD_CTRL_RIGHT,
	KBD_CTRL_TAB,
	KBD_CTRL_BACK,
	KBD_CTRL_LSHIFT,
	KBD_CTRL_LCTRL,
	KBD_CTRL_RSHIFT,
	KBD_CTRL_RCTRL,
	KBD_CTRL_ENTER,
	_KBD_CTRL_NUM_
};

struct KBD_STATE {
	bool digit[10];
	bool alpha['z' - 'a' + 1];
	bool func[12];
	bool punct[_KBD_PUNCT_NUM_];
	bool ctrl[_KBD_CTRL_NUM_];
};

static void oglsys_mem_cpy(void* pDst, const void* pSrc, const size_t len) {
	if (pDst && pSrc) {
		uint8_t* pd = (uint8_t*)pDst;
		const uint8_t* ps = (const uint8_t*)pSrc;
		size_t i;
		for (i = 0; i < len; ++i) {
			*pd++ = *ps++;
		}
	}
}

static void oglsys_mem_set(void* pDst, const int val, const size_t len) {
	if (pDst) {
		uint8_t* pd = (uint8_t*)pDst;
		uint8_t v = (uint8_t)val;
		size_t i;
		for (i = 0; i < len; ++i) {
			*pd++ = v;
		}
	}
}

#ifndef OGLSYS_INTERNAL_STRFN
#	define OGLSYS_INTERNAL_STRFN 0
#endif

static bool oglsys_str_eq(const char* pStr1, const char* pStr2) {
#if OGLSYS_INTERNAL_STRFN
	bool res = (pStr1 == pStr2);
	if (pStr1 && pStr2) {
		const char* p1 = pStr1;
		const char* p2 = pStr2;
		res = true;
		while (true) {
			char c = *p1;
			if (c != *p2) {
				res = false;
				break;
			}
			if (!c) {
				break;
			}
			++p1;
			++p2;
		}
	}
	return res;
#else
	return ::strcmp(pStr1, pStr2) == 0;
#endif
}

static size_t oglsys_str_len(const char* pStr) {
#if OGLSYS_INTERNAL_STRFN
	size_t len = 0;
	if (pStr) {
		const char* p = pStr;
		while (true) {
			if (!*p) break;
			++p;
		}
		len = (size_t)(p - pStr);
	}
	return len;
#else
	return pStr ? ::strlen(pStr) : 0;
#endif
}

#if OGLSYS_ES
typedef GLuint (GL_APIENTRYP OGLSYS_PFNGLGETUNIFORMBLOCKINDEXPROC)(GLuint, const GLchar*);
typedef void (GL_APIENTRYP OGLSYS_PFNGLGETACTIVEUNIFORMBLOCKIVPROC)(GLuint, GLuint, GLenum, GLint*);
typedef void (GL_APIENTRYP OGLSYS_PFNGLGETUNIFORMINDICESPROC)(GLuint, GLsizei, const GLchar* const*, GLuint*);
typedef void (GL_APIENTRYP OGLSYS_PFNGLGETACTIVEUNIFORMSIVPROC)(GLuint, GLsizei, const GLuint*, GLenum, GLint*);
typedef void* (GL_APIENTRYP OGLSYS_PFNGLMAPBUFFERRANGEPROC)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
typedef GLboolean (GL_APIENTRYP OGLSYS_PFNGLUNMAPBUFFERPROC)(GLenum);
typedef void (GL_APIENTRYP OGLSYS_PFNGLUNIFORMBLOCKBINDINGPROC)(GLuint, GLuint, GLuint);
typedef void (GL_APIENTRYP OGLSYS_PFNGLBINDBUFFERBASEPROC)(GLenum, GLuint, GLuint);
#endif

#if defined(OGLSYS_DRM_ES)
#ifndef OGLSYS_DRM_FBID_DYN
#	define OGLSYS_DRM_FBID_DYN 0
#endif

#if !OGLSYS_DRM_FBID_DYN
static uint32_t s_drm_fbid_mem[8] = {};
static size_t s_drm_fbid_mem_idx = 0;
#endif

static uint32_t* alloc_drm_fbid() {
#if OGLSYS_DRM_FBID_DYN
	return (uint32_t*)OGLSys::mem_alloc(sizeof(uint32_t), "OGLSys::DRM::fbid");
#else
	size_t idx = s_drm_fbid_mem_idx;
	size_t n = sizeof(s_drm_fbid_mem) / sizeof(uint32_t);
	++s_drm_fbid_mem_idx;
	s_drm_fbid_mem_idx &= n - 1;
	return &s_drm_fbid_mem[idx];
#endif
}

static void free_drm_fbid(uint32_t* pFbId) {
#if OGLSYS_DRM_FBID_DYN
	if (pFbId) {
		OGLSys::mem_free(pFbId);
	}
#endif
}

static void cb_drm_fb_destroy(gbm_bo* pBO, void* pData) {
	uint32_t* pFbId = (uint32_t*)pData;
	if (pBO && pFbId) {
		uint32_t fbId = *pFbId;
		if (fbId) {
			gbm_device* pDev = gbm_bo_get_device(pBO);
			if (pDev) {
				drmModeRmFB(gbm_device_get_fd(pDev), fbId);
			}
		}
	}
	if (pFbId) {
		free_drm_fbid(pFbId);
	}
}

static void evt_drm_page_flip(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void* pData) {
	bool* pWaitFlg = (bool*)pData;
	*pWaitFlg = false;
}
#endif

static struct OGLSysGlb {
#if defined(OGLSYS_WINDOWS)
	HINSTANCE mhInstance;
	ATOM mClassAtom;
	HWND mhWnd;
	HDC mhDC;
#elif defined(OGLSYS_ANDROID)
	ANativeWindow* mpNativeWnd;
#elif defined(OGLSYS_X11)
	Display* mpXDisplay;
	Window mXWnd;
#elif defined(OGLSYS_VIVANTE_FB)
	EGLNativeDisplayType mVivDisp;
	EGLNativeWindowType mVivWnd;
#elif defined(OGLSYS_DRM_ES)
	int mDrmDevFD;
	fd_set mDrmDevFDSet;
	drmModeConnector* mpDrmConn;
	drmModeEncoder* mpDrmEnc;
	int mDrmModeIdx;
	int mDrmCtrlId;
	gbm_device* mpGbmDev;
	gbm_surface* mpGbmSurf;
	gbm_bo* mpGbmBO;
	uint32_t mDrmFbId;
#elif defined(OGLSYS_WEB)
	SDL_Window* mpSDLWnd;
	SDL_Renderer* mpSDLRdr;
	SDL_GLContext mSDLGLCtx;
	void (*mpWebLoop)(void*);
	void* mpWebLoopCtx;
#endif

#if defined(OGLSYS_LINUX_INPUT)
	int mRawKbdFD;
	int mRawKbdMode;
	input_event mRawKbdEvent;
#endif

	uint64_t mFrameCnt;

	OGLSysIfc mIfc;
	int mWndOrgX;
	int mWndOrgY;
	int mWndW;
	int mWndH;
	int mWidth;
	int mHeight;
	int mNumMSAASamples;

	bool mReduceRes;
	bool mHideSysIcons;
	bool mWithoutCtx;

	GLint mDefFBO;
	GLint mMaxTexSize;

	int mSwapInterval;

	struct DefTexs {
		GLuint black;
		GLuint white;
	} mDefTexs;

	void (*mpfnGlFlush)();
	void (*mpfnGlFinish)();

	struct Exts {
#if OGLSYS_ES
		PFNGLGENQUERIESEXTPROC pfnGenQueries;
		PFNGLDELETEQUERIESEXTPROC pfnDeleteQueries;
		PFNGLQUERYCOUNTEREXTPROC pfnQueryCounter;
		PFNGLGETQUERYOBJECTIVEXTPROC pfnGetQueryObjectiv;
		PFNGLGETQUERYOBJECTUIVEXTPROC pfnGetQueryObjectuiv;
		PFNGLGETQUERYOBJECTUI64VEXTPROC pfnGetQueryObjectui64v;
		PFNGLGETPROGRAMBINARYOESPROC pfnGetProgramBinary;
		PFNGLPROGRAMBINARYOESPROC pfnProgramBinary;
		PFNGLDISCARDFRAMEBUFFEREXTPROC pfnDiscardFramebuffer;
		PFNGLGENVERTEXARRAYSOESPROC pfnGenVertexArrays;
		PFNGLDELETEVERTEXARRAYSOESPROC pfnDeleteVertexArrays;
		PFNGLBINDVERTEXARRAYOESPROC pfnBindVertexArray;
#ifdef OGLSYS_VIVANTE_FB
		void* pfnDrawElementsBaseVertex;
#else
		PFNGLDRAWELEMENTSBASEVERTEXOESPROC pfnDrawElementsBaseVertex;
#endif
		OGLSYS_PFNGLGETUNIFORMBLOCKINDEXPROC pfnGetUniformBlockIndex;
		OGLSYS_PFNGLGETACTIVEUNIFORMBLOCKIVPROC pfnGetActiveUniformBlockiv;
		OGLSYS_PFNGLGETUNIFORMINDICESPROC pfnGetUniformIndices;
		OGLSYS_PFNGLGETACTIVEUNIFORMSIVPROC pfnGetActiveUniformsiv;
		OGLSYS_PFNGLMAPBUFFERRANGEPROC pfnMapBufferRange;
		OGLSYS_PFNGLUNMAPBUFFERPROC pfnUnmapBuffer;
		OGLSYS_PFNGLUNIFORMBLOCKBINDINGPROC pfnUniformBlockBinding;
		OGLSYS_PFNGLBINDBUFFERBASEPROC pfnBindBufferBase;
#endif
		bool bindlessTex;
		bool ASTC_LDR;
		bool S3TC;
		bool DXT1;
		bool disjointTimer;
		bool derivs;
		bool vtxHalf;
		bool idxUInt;
		bool progBin;
		bool discardFB;
		bool mdi;
		bool nvVBUM;
		bool nvUBUM;
		bool nvBindlessMDI;
		bool nvCmdLst;
	} mExts;

	OGLSys::InputHandler mInpHandler;
	void* mpInpHandlerWk;
	float mInpSclX;
	float mInpSclY;

	OGLSysMouseState mMouseState;

	KBD_STATE mKbdState;

	void* mem_alloc(size_t size, const char* pTag) {
		return mIfc.mem_alloc ? mIfc.mem_alloc(size, pTag) : ::malloc(size);
	}

	void mem_free(void* p) {
		if (mIfc.mem_free) {
			mIfc.mem_free(p);
		} else {
			::free(p);
		}
	}

	void dbg_msg(const char* pFmt, ...) {
		if (mIfc.dbg_msg) {
			char buf[1024];
			va_list lst;
			va_start(lst, pFmt);
#ifdef _MSC_VER
			vsprintf_s(buf, sizeof(buf), pFmt, lst);
#else
			vsprintf(buf, pFmt, lst);
#endif
			mIfc.dbg_msg("%s", buf);
			va_end(lst);
		}
	}

	const char* load_glsl(const char* pPath, size_t* pSize) {
		if (mIfc.load_glsl) {
			return mIfc.load_glsl(pPath, pSize);
		} else {
			char* pCode = nullptr;
			size_t size = 0;
			FILE* pFile = nullptr;
			const char* pMode = "rb";
#if defined(_MSC_VER)
			::fopen_s(&pFile, pPath, pMode);
#else
			pFile = ::fopen(pPath, pMode);
#endif
			if (pFile) {
				if (::fseek(pFile, 0, SEEK_END) == 0) {
					size = ::ftell(pFile);
				}
				::fseek(pFile, 0, SEEK_SET);
				if (size) {
					pCode = (char*)mem_alloc(size, "OGLSys:GLSL");
					size_t nread = 0;
					if (pCode) {
						oglsys_mem_set(pCode, 0, size);
						nread = ::fread(pCode, 1, size, pFile);
						if (nread != size) {
							mem_free(pCode);
							pCode = nullptr;
							size = 0;
						}
					}
				}
				::fclose(pFile);
			}
			if (pSize) {
				*pSize = size;
			}
			return pCode;
		}
	}

	void unload_glsl(const char* pCode) {
		if (mIfc.unload_glsl) {
			mIfc.unload_glsl(pCode);
		} else {
			mem_free((void*)pCode);
		}
	}

#if OGLSYS_ES
	struct EGL {
		EGLDisplay display;
		EGLSurface surface;
		EGLContext context;
		EGLConfig config;

		void reset() {
			display = EGL_NO_DISPLAY;
			surface = EGL_NO_SURFACE;
			context = EGL_NO_CONTEXT;
		}
	} mEGL;
#elif defined(OGLSYS_WINDOWS)
	struct WGL {
		HMODULE hLib;
		PROC(APIENTRY *wglGetProcAddress)(LPCSTR);
		HGLRC(APIENTRY *wglCreateContext)(HDC);
		BOOL(APIENTRY *wglDeleteContext)(HGLRC);
		BOOL(APIENTRY *wglMakeCurrent)(HDC, HGLRC);
		HGLRC(WINAPI *wglCreateContextAttribsARB)(HDC, HGLRC, const int*);
		BOOL(WINAPI *wglChoosePixelFormatARB)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
		const char* (APIENTRY *wglGetExtensionsStringEXT)();
		BOOL(APIENTRY *wglSwapIntervalEXT)(int);
		int (APIENTRY *wglGetSwapIntervalEXT)();
		HGLRC hCtx;
		HGLRC hExtCtx;
		bool ok;

		void init_ctx(const HDC hDC, const int pixFmt = 0) {
			if (hDC && wglCreateContext && wglDeleteContext && wglMakeCurrent) {
				PIXELFORMATDESCRIPTOR fmt;
				ZeroMemory(&fmt, sizeof(fmt));
				fmt.nVersion = 1;
				fmt.nSize = sizeof(fmt);
				fmt.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_GENERIC_ACCELERATED | PFD_DOUBLEBUFFER;
				fmt.iPixelType = PFD_TYPE_RGBA;
				fmt.cColorBits = 32;
				fmt.cDepthBits = 24;
				fmt.cStencilBits = 8;
				int ifmt = pixFmt ? pixFmt : ChoosePixelFormat(hDC, &fmt);
				if (0 == ifmt) {
					return;
				}
				if (pixFmt) {
					DescribePixelFormat(hDC, ifmt, sizeof(fmt), &fmt);
				}
				if (!SetPixelFormat(hDC, ifmt, &fmt)) {
					return;
				}
				hCtx = wglCreateContext(hDC);
				if (hCtx) {
					BOOL flg = wglMakeCurrent(hDC, hCtx);
					if (!flg) {
						wglDeleteContext(hCtx);
						hCtx = nullptr;
					}
				}
			}
		}

		void init_ext_ctx(HDC hDC) {
			if (hDC && hCtx && wglCreateContextAttribsARB) {
				int attribsCompat[] = {
					WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
					WGL_CONTEXT_MINOR_VERSION_ARB, 0,
					WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
					0
				};
				hExtCtx = wglCreateContextAttribsARB(hDC, hCtx, attribsCompat);
				if (!hExtCtx) {
					int attribsCore[] = {
						WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
						WGL_CONTEXT_MINOR_VERSION_ARB, 0,
						WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
						0
					};
					hExtCtx = wglCreateContextAttribsARB(hDC, hCtx, attribsCore);
				}
				if (hExtCtx) {
					BOOL flg = wglMakeCurrent(hDC, hExtCtx);
					if (!flg) {
						wglDeleteContext(hExtCtx);
						hExtCtx = nullptr;
						wglMakeCurrent(hDC, hCtx);
					}
				}
			}
		}

		void reset_ctx() {
			if (wglDeleteContext) {
				if (hExtCtx) {
					wglDeleteContext(hExtCtx);
					hExtCtx = nullptr;
				}
				if (hCtx) {
					wglDeleteContext(hCtx);
					hCtx = nullptr;
				}
			}
		}

		bool valid_ctx() const { return (!!hCtx); }

		int get_msaa_pixfmt(const HDC hDC, const int nsmp) {
			int ifmt = 0;
			UINT nfmt = 0;
			if (hDC && wglChoosePixelFormatARB) {
				int attrs[] = {
					WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
					WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
					WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
					WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
					WGL_COLOR_BITS_ARB, 32,
					WGL_DEPTH_BITS_ARB, 24,
					WGL_STENCIL_BITS_ARB, 8,
					WGL_SAMPLE_BUFFERS_ARB, 1,
					WGL_SAMPLES_ARB, nsmp,
					0
				};
				BOOL fmtRes = wglChoosePixelFormatARB(hDC, attrs, NULL, 1, &ifmt, &nfmt);
				if (!fmtRes) {
					ifmt = 0;
				}
			}
			return ifmt;
		}

		void set_swap_interval(const int i) {
			if (wglSwapIntervalEXT) {
				wglSwapIntervalEXT(i);
			}
		}
	} mWGL;
#elif defined(OGLSYS_X11)
	struct GLX {
		typedef XVisualInfo* (*OGLSYS_PFNGLXCHOOSEVISUAL)(Display*, int, int*);
		typedef GLXContext (*OGLSYS_PFNGLXCREATECONTEXT)(Display*, XVisualInfo*, GLXContext, Bool);
		typedef void (*OGLSYS_PFNGLXDESTROYCONTEXT)(Display*, GLXContext);
		typedef Bool (*OGLSYS_PFNGLXMAKECURRENT)(Display*, GLXDrawable, GLXContext);
		typedef void (*OGLSYS_PFNGLXSWAPBUFFERS)(Display*, GLXDrawable);
		typedef void (*OGLSYS_PFNGLXSWAPINTERVAL)(Display*, GLXDrawable, int);

		void* mpLib;
		GLXContext mCtx;
		OGLSYS_PFNGLXCHOOSEVISUAL mpfnGLXChooseVisual;
		OGLSYS_PFNGLXCREATECONTEXT mpfnGLXCreateContext;
		OGLSYS_PFNGLXDESTROYCONTEXT mpfnGLXDestroyContext;
		OGLSYS_PFNGLXMAKECURRENT mpfnGLXMakeCurrent;
		OGLSYS_PFNGLXSWAPBUFFERS mpfnGLXSwapBuffers;
		OGLSYS_PFNGLXSWAPINTERVAL mpfnGLXSwapInterval;

		bool valid() const { return !!mpLib; }

		void* get_func_ptr(const char* pName) {
			void* pFunc = nullptr;
			if (mpLib) {
				pFunc = ::dlsym(mpLib, pName);
			}
			return pFunc;
		}

		void init() {
			static const char* glLibs[] = {
				"libGL.so.1",
				"libGL.so",
				"libGL.so.17.1"
			};
			mpLib = OGLSys::dlopen_from_list(glLibs, OGLSYS_ARY_LEN(glLibs));
			::printf("GLX:mpLib = %p\n", mpLib);
			if (mpLib) {
				mpfnGLXChooseVisual = (OGLSYS_PFNGLXCHOOSEVISUAL)get_func_ptr("glXChooseVisual");
				mpfnGLXCreateContext = (OGLSYS_PFNGLXCREATECONTEXT)get_func_ptr("glXCreateContext");
				mpfnGLXDestroyContext = (OGLSYS_PFNGLXDESTROYCONTEXT)get_func_ptr("glXDestroyContext");
				mpfnGLXMakeCurrent = (OGLSYS_PFNGLXMAKECURRENT)get_func_ptr("glXMakeCurrent");
				mpfnGLXSwapBuffers = (OGLSYS_PFNGLXSWAPBUFFERS)get_func_ptr("glXSwapBuffers");
				mpfnGLXSwapInterval = (OGLSYS_PFNGLXSWAPINTERVAL)get_func_ptr("glXSwapIntervalEXT");
			}
		}

		void reset() {
			if (valid()) {
				::dlclose(mpLib);
				mpLib = nullptr;
			}
		}

		void set_swap_interval(Display* pDisp, GLXDrawable drw, int i) {
			if (mpfnGLXSwapInterval) {
				mpfnGLXSwapInterval(pDisp, drw, i);
			}
		}
	} mGLX;
#endif

#if defined(OGLSYS_DUMMY)
	struct DummyGLWk {
		void (*swap_func)();
	} mDummyGL;
#endif

#if OGLSYS_CL
#	if defined(OGLSYS_WINDOWS)
	HMODULE
#	else
	void*
#	endif
	mLibOCL;
	int mNumFuncsOCL;
	void* (*mpfnTIAllocDDR)(size_t);
	void (*mpfnTIFreeDDR)(void*);
	void* (*mpfnTIAllocMSMC)(size_t);
	void (*mpfnTIFreeMSMC)(void*);
#endif

#if OGLSYS_ES
	bool valid_display() const { return mEGL.display != EGL_NO_DISPLAY; }
	bool valid_surface() const { return mEGL.surface != EGL_NO_SURFACE; }
	bool valid_context() const { return mEGL.context != EGL_NO_CONTEXT; }
	bool valid_ogl() const { return valid_display() && valid_surface() && valid_context(); }
#elif defined(OGLSYS_WINDOWS)
	bool valid_ogl() const { return mWGL.ok; }

	void* get_func_ptr(const char* pName) {
		void* p = (void*)(mWGL.wglGetProcAddress ? mWGL.wglGetProcAddress(pName) : nullptr);
		if (!p) {
			if (mWGL.hLib) {
				p = (void*)GetProcAddress(mWGL.hLib, pName);
			}
		}
		return p;
	}
#elif defined(OGLSYS_X11)
	bool valid_ogl() const { return mGLX.valid(); }
#elif defined(OGLSYS_APPLE)
	bool valid_ogl() const { return true; }
#	elif defined(OGLSYS_WEB)
	bool valid_ogl() const { return true; }
#	elif defined(OGLSYS_DUMMY)
	bool valid_ogl() const { return true; }
#endif

	void stop_ogl() {
#if OGLSYS_ES
		if (valid_display()) {
			eglMakeCurrent(mEGL.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglTerminate(mEGL.display);
		}
#elif defined(OGLSYS_WINDOWS)
		if (valid_ogl()) {
			mWGL.wglMakeCurrent(mhDC, nullptr);
		}
#elif defined(OGLSYS_X11)
		if (valid_ogl()) {
			mGLX.mpfnGLXMakeCurrent(mpXDisplay, None, NULL);
		}
#endif
	}

	void init_wnd();
	void init_ogl();
	void reset_wnd();
	void reset_ogl();

	void handle_ogl_ext(const GLubyte* pStr, const int lenStr);

	void update_mouse_pos(const float x, const float y) {
		mMouseState.mOldX = mMouseState.mNowX;
		mMouseState.mOldY = mMouseState.mNowY;
		mMouseState.mNowX = x * mInpSclX;
		mMouseState.mNowY = y * mInpSclY;
	}

	void update_mouse_pos(const int x, const int y) {
		update_mouse_pos(float(x), float(y));
	}

	void set_mouse_pos(const float x, const float y) {
		mMouseState.mNowX = x * mInpSclX;
		mMouseState.mNowY = y * mInpSclY;
		mMouseState.mOldX = mMouseState.mNowX;
		mMouseState.mOldY = mMouseState.mNowY;
	}

	void set_mouse_pos(const int x, const int y) {
		set_mouse_pos(float(x), float(y));
	}

	void send_input(OGLSysInput* pInp) {
		if (pInp && mInpHandler) {
			if (pInp->act != OGLSysInput::OGLSYS_ACT_NONE) {
				if (pInp->pressure > 1.0f) {
					pInp->pressure = 1.0f;
				}
				pInp->relX = pInp->absX * mInpSclX;
				pInp->relY = pInp->absY * mInpSclY;
				mInpHandler(*pInp, mpInpHandlerWk);
			}
		}
	}

#if defined(OGLSYS_DRM_ES)
	uint32_t prepare_drm_fb() {
		uint32_t* pFbId = nullptr;
		if (mpGbmBO) {
			pFbId = (uint32_t*)gbm_bo_get_user_data(mpGbmBO);
			if (!pFbId) {
				pFbId = alloc_drm_fbid();
				if (pFbId) {
					*pFbId = 0;
					uint32_t w = gbm_bo_get_width(mpGbmBO);
					uint32_t h = gbm_bo_get_height(mpGbmBO);
					uint32_t stride = gbm_bo_get_stride(mpGbmBO);
					uint32_t handle = gbm_bo_get_handle(mpGbmBO).u32;
					if (drmModeAddFB(mDrmDevFD, w, h, 32, 32, stride, handle, pFbId) == 0) {
						gbm_bo_set_user_data(mpGbmBO, pFbId, cb_drm_fb_destroy);
					}
				}
			}
		}
		return pFbId ? *pFbId : 0;
	}

	void exec_drm_flip() {
		if (mpGbmSurf) {
			if (mSwapInterval != 0) {
				bool flipWaitFlg = true;
				drmEventContext evtCtx = {};
				evtCtx.version = DRM_EVENT_CONTEXT_VERSION;
				evtCtx.page_flip_handler = evt_drm_page_flip;
				gbm_bo* pOldBO = mpGbmBO;
				mpGbmBO = gbm_surface_lock_front_buffer(mpGbmSurf);
				uint32_t fbId = prepare_drm_fb();
				if (drmModePageFlip(mDrmDevFD, mDrmCtrlId, fbId, DRM_MODE_PAGE_FLIP_EVENT, &flipWaitFlg) == 0) {
					while (flipWaitFlg) {
						int selRes = select(mDrmDevFD + 1, &mDrmDevFDSet, NULL, NULL, NULL);
						if (selRes < 0) {
							break;
						}
						if (FD_ISSET(0, &mDrmDevFDSet)) {
							break;
						}
						drmHandleEvent(mDrmDevFD, &evtCtx);
					}
					gbm_surface_release_buffer(mpGbmSurf, pOldBO);
				}
			} else {
				if (mpDrmConn) {
					gbm_bo* pOldBO = mpGbmBO;
					mpGbmBO = gbm_surface_lock_front_buffer(mpGbmSurf);
					uint32_t fbId = prepare_drm_fb();
					uint32_t connId = mpDrmConn->connector_id;
					drmModeSetCrtc(mDrmDevFD, mDrmCtrlId, fbId, 0, 0, &connId, 1, NULL);
					gbm_surface_release_buffer(mpGbmSurf, pOldBO);
				}
			}
		}
	}
#endif

	const char* get_opt(const char* pName) {
		const char* pOpt = nullptr;
		if (mIfc.get_opt) {
			pOpt = mIfc.get_opt(pName);
		}
		return pOpt;
	}

	int get_int_opt(const char* pName, const int defVal) {
		int res = defVal;
		const char* pOpt = get_opt(pName);
		if (pOpt) {
			res = ::atoi(pOpt);
		}
		return res;
	}

} GLG = {};

#if defined(OGLSYS_WEB)
static void web_kbd(const SDL_Event& evt) {
	bool* pState = nullptr;
	int idx = -1;
	SDL_Keycode sym = evt.key.keysym.sym;
	switch (sym) {
		case SDLK_UP:
			pState = GLG.mKbdState.ctrl;
			idx = KBD_CTRL_UP;
			break;
		case SDLK_DOWN:
			pState = GLG.mKbdState.ctrl;
			idx = KBD_CTRL_DOWN;
			break;
		case SDLK_LEFT:
			pState = GLG.mKbdState.ctrl;
			idx = KBD_CTRL_LEFT;
			break;
		case SDLK_RIGHT:
			pState = GLG.mKbdState.ctrl;
			idx = KBD_CTRL_RIGHT;
			break;
		case SDLK_TAB:
			pState = GLG.mKbdState.ctrl;
			idx = KBD_CTRL_TAB;
			break;
		case SDLK_BACKSPACE:
			pState = GLG.mKbdState.ctrl;
			idx = KBD_CTRL_BACK;
			break;
		case SDLK_LSHIFT:
			pState = GLG.mKbdState.ctrl;
			idx = KBD_CTRL_LSHIFT;
			break;
		case SDLK_LCTRL:
			pState = GLG.mKbdState.ctrl;
			idx = KBD_CTRL_LCTRL;
			break;
		case SDLK_RSHIFT:
			pState = GLG.mKbdState.ctrl;
			idx = KBD_CTRL_RSHIFT;
			break;
		case SDLK_RCTRL:
			pState = GLG.mKbdState.ctrl;
			idx = KBD_CTRL_RCTRL;
			break;
		case SDLK_RETURN:
			pState = GLG.mKbdState.ctrl;
			idx = KBD_CTRL_ENTER;
			break;
		default:
			if ((uint32_t)sym >= 'a' && (uint32_t)sym <= 'z') {
				pState = GLG.mKbdState.alpha;
				idx = (int)sym - 'a';
			} else if (sym >= SDLK_F1 && sym <= SDLK_F12) {
				pState = GLG.mKbdState.func;
				idx = (uint32_t)sym - (uint32_t)SDLK_F1;
			}
			break;
	}
	if (pState && idx >= 0) {
		pState[idx] = (evt.type == SDL_KEYDOWN);
	}
}

static void web_loop() {
	SDL_Event evt;
	while (SDL_PollEvent(&evt)) {
		if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
			web_kbd(evt);
		} else if (evt.type == SDL_QUIT) {
		}
	}
	if (GLG.mpWebLoop) {
		GLG.mpWebLoop(GLG.mpWebLoopCtx);
	}
	++GLG.mFrameCnt;
}
#endif

static void glg_dbg_info(const char* pInfo, const size_t infoLen) {
	const int infoBlkSize = 512;
	char infoBlk[infoBlkSize + 1];
	infoBlk[infoBlkSize] = 0;
	size_t nblk = infoLen / infoBlkSize;
	for (size_t i = 0; i < nblk; ++i) {
		oglsys_mem_cpy(infoBlk, &pInfo[infoBlkSize * i], infoBlkSize);
		GLG.dbg_msg("%s", infoBlk);
	}
	int endSize = infoLen % infoBlkSize;
	if (endSize) {
		oglsys_mem_cpy(infoBlk, &pInfo[infoBlkSize * nblk], endSize);
		infoBlk[endSize] = 0;
		GLG.dbg_msg("%s", infoBlk);
	}
}

#if defined(OGLSYS_WINDOWS)
static bool wnd_mouse_msg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static UINT mouseMsg[] = {
		WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK,
		WM_MBUTTONDOWN, WM_MBUTTONUP, WM_MBUTTONDBLCLK,
		WM_RBUTTONDOWN, WM_RBUTTONUP, WM_RBUTTONDBLCLK,
		WM_XBUTTONDOWN, WM_XBUTTONUP, WM_XBUTTONDBLCLK,
		WM_MOUSEWHEEL, WM_MOUSEMOVE
	};
	bool mouseFlg = false;
	for (size_t i = 0; i < OGLSYS_ARY_LEN(mouseMsg); ++i) {
		if (mouseMsg[i] == msg) {
			mouseFlg = true;
			break;
		}
	}
	if (mouseFlg) {
		static uint32_t mouseMask[] = {
			MK_LBUTTON, MK_MBUTTON, MK_RBUTTON, MK_XBUTTON1, MK_XBUTTON2
		};
		uint32_t mask = 0;
		uint32_t btnMask = LOWORD(wParam);
		for (size_t i = 0; i < OGLSYS_ARY_LEN(mouseMask); ++i) {
			if (btnMask & mouseMask[i]) {
				mask |= 1 << i;
			}
		}
		GLG.mMouseState.mBtnOld = GLG.mMouseState.mBtnNow;
		GLG.mMouseState.mBtnNow = mask;

		int32_t posX = (int32_t)LOWORD(lParam);
		int32_t posY = (int32_t)HIWORD(lParam);
		int32_t wheel = 0;
		if (msg == WM_MOUSEWHEEL) {
			wheel = (int16_t)HIWORD(wParam);
		}
		GLG.update_mouse_pos(posX, posY);
		GLG.mMouseState.mWheel = wheel;

		OGLSysInput inp;
		inp.device = OGLSysInput::OGLSYS_MOUSE;
		inp.sysDevId = 0;
		inp.pressure = 1.0f;
		inp.act = OGLSysInput::OGLSYS_ACT_NONE;
		inp.id = -1;
		switch (msg) {
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_LBUTTONDBLCLK:
				inp.id = 0;
				break;
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MBUTTONDBLCLK:
				inp.id = 1;
				break;
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_RBUTTONDBLCLK:
				inp.id = 2;
				break;
			case WM_XBUTTONDOWN:
			case WM_XBUTTONUP:
			case WM_XBUTTONDBLCLK:
				inp.id = 3;
				break;
		}
		inp.absX = float(LOWORD(lParam));
		inp.absY = float(HIWORD(lParam));
		switch (msg) {
			case WM_MOUSEMOVE:
				if (GLG.mMouseState.ck_now(OGLSysMouseState::OGLSYS_BTN_LEFT)) {
					inp.id = 0;
					inp.act = OGLSysInput::OGLSYS_ACT_DRAG;
				} else {
					inp.act = OGLSysInput::OGLSYS_ACT_HOVER;
				}
				break;
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_XBUTTONDOWN:
				inp.act = OGLSysInput::OGLSYS_ACT_DOWN;
				break;
			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
			case WM_XBUTTONUP:
				inp.act = OGLSysInput::OGLSYS_ACT_UP;
				break;
		}
		GLG.send_input(&inp);
	}
	return mouseFlg;
}

static bool wnd_kbd_msg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	bool res = false;
	if (msg == WM_KEYDOWN || msg == WM_KEYUP) {
		bool* pState = nullptr;
		int idx = -1;
		bool repFlg = msg == WM_KEYDOWN ? !!((lParam >> 30) & 1) : false;
		if (!repFlg) {
			int code = int(wParam);
			if (code >= '0' && code <= '9') {
				pState = GLG.mKbdState.digit;
				idx = code - '0';
			} else if (code >= 'A' && code <= 'Z') {
				pState = GLG.mKbdState.alpha;
				idx = code - 'A';
			} else if (code >= VK_F1 && code <= VK_F12) {
				pState = GLG.mKbdState.func;
				idx = code - VK_F1;
			} else {
				int scan = (lParam >> 16) & 0xFF;
				bool extFlg = !!((lParam >> 24) & 1);
				if (code == VK_SHIFT) {
					code = scan == 0x2A ? VK_LSHIFT : VK_RSHIFT;
				} else if (code == VK_CONTROL) {
					code = extFlg ? VK_RCONTROL : VK_LCONTROL;
				}
				switch (code) {
					case VK_SPACE:
						pState = GLG.mKbdState.punct;
						idx = KBD_PUNCT_SPACE;
						break;
					case VK_OEM_PLUS:
						pState = GLG.mKbdState.punct;
						idx = KBD_PUNCT_PLUS;
						break;
					case  VK_OEM_MINUS:
						pState = GLG.mKbdState.punct;
						idx = KBD_PUNCT_MINUS;
						break;
					case VK_OEM_COMMA:
						pState = GLG.mKbdState.punct;
						idx = KBD_PUNCT_COMMA;
						break;
					case VK_OEM_PERIOD:
						pState = GLG.mKbdState.punct;
						idx = KBD_PUNCT_PERIOD;
						break;
					case VK_UP:
						pState = GLG.mKbdState.ctrl;
						idx = KBD_CTRL_UP;
						break;
					case VK_DOWN:
						pState = GLG.mKbdState.ctrl;
						idx = KBD_CTRL_DOWN;
						break;
					case VK_LEFT:
						pState = GLG.mKbdState.ctrl;
						idx = KBD_CTRL_LEFT;
						break;
					case VK_RIGHT:
						pState = GLG.mKbdState.ctrl;
						idx = KBD_CTRL_RIGHT;
						break;
					case VK_TAB:
						pState = GLG.mKbdState.ctrl;
						idx = KBD_CTRL_TAB;
						break;
					case VK_BACK:
						pState = GLG.mKbdState.ctrl;
						idx = KBD_CTRL_BACK;
						break;
					case VK_LSHIFT:
						pState = GLG.mKbdState.ctrl;
						idx = KBD_CTRL_LSHIFT;
						break;
					case VK_LCONTROL:
						pState = GLG.mKbdState.ctrl;
						idx = KBD_CTRL_LCTRL;
						break;
					case VK_RSHIFT:
						pState = GLG.mKbdState.ctrl;
						idx = KBD_CTRL_RSHIFT;
						break;
					case VK_RCONTROL:
						pState = GLG.mKbdState.ctrl;
						idx = KBD_CTRL_RCTRL;
						break;
					case VK_RETURN:
						pState = GLG.mKbdState.ctrl;
						idx = KBD_CTRL_ENTER;
						break;
				}
			}
		}
		if (pState && idx >= 0) {
			pState[idx] = (msg == WM_KEYDOWN);
			res = true;
		}
	}
	return res;
}

static bool s_wndQuitOnDestroy = true;

static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	LRESULT res = 0;
	bool mouseFlg = wnd_mouse_msg(hWnd, msg, wParam, lParam);
	bool kbdFlg = wnd_kbd_msg(hWnd, msg, wParam, lParam);
	if (!mouseFlg && !kbdFlg) {
		switch (msg) {
#ifdef OGLSYS_ICON_ID
			case WM_CREATE:
				if (OGLSYS_ICON_ID) {
					HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
					HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(OGLSYS_ICON_ID));
					if (hIcon) {
						SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
					} else {
						res = DefWindowProc(hWnd, msg, wParam, lParam);
					}
				}
				break;
#endif
			case WM_DESTROY:
				if (s_wndQuitOnDestroy) {
					PostQuitMessage(0);
				} else {
					res = DefWindowProc(hWnd, msg, wParam, lParam);
				}
				break;
			case WM_SYSCOMMAND:
				if (!(wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER)) {
					res = DefWindowProc(hWnd, msg, wParam, lParam);
				}
				break;
			default:
				res = DefWindowProc(hWnd, msg, wParam, lParam);
				break;
		}
	}
	return res;
}

static const TCHAR* s_oglClassName = OGLSYS_ES ? _T("SysGLES") : _T("SysOGL");
#elif defined(OGLSYS_ANDROID)

static ANativeWindow* s_pNativeWnd = nullptr;

static void app_cmd(android_app* pApp, int32_t cmd) {
	switch (cmd) {
		case APP_CMD_INIT_WINDOW:
			s_pNativeWnd = pApp->window;
			break;
		case APP_CMD_TERM_WINDOW:
			break;
		case APP_CMD_STOP:
			ANativeActivity_finish(pApp->activity);
			break;
		default:
			break;
	}
}
#elif defined(OGLSYS_X11)
static int wait_MapNotify(Display* pDisp, XEvent* pEvt, char* pArg) {
	return ((pEvt->type == MapNotify) && (pEvt->xmap.window == (Window)pArg)) ? 1 : 0;
}
#endif

void OGLSysGlb::init_wnd() {
#if defined(OGLSYS_WINDOWS)
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.hInstance = mhInstance;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = s_oglClassName;
	wc.lpfnWndProc = wnd_proc;
	wc.cbWndExtra = 0x10;
	mClassAtom = RegisterClassEx(&wc);

	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = mWidth;
	rect.bottom = mHeight;
	int style = WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_GROUP;
	AdjustWindowRect(&rect, style, FALSE);
	mWndW = rect.right - rect.left;
	mWndH = rect.bottom - rect.top;
	char tmpTitle[100];
#if defined(_MSC_VER)
	sprintf_s(tmpTitle, sizeof(tmpTitle),
#else
	sprintf(tmpTitle,
#endif
		"%s: build %s", mWithoutCtx ? "SysWnd" : (OGLSYS_ES ? "OGL(ES)" : "OGL"), __DATE__);
	size_t tlen = oglsys_str_len(tmpTitle);
	TCHAR title[100];
	ZeroMemory(title, sizeof(title));
	for (size_t i = 0; i < tlen; ++i) {
		title[i] = (TCHAR)tmpTitle[i];
	}
	mhWnd = CreateWindowEx(0, s_oglClassName, title, style, mWndOrgX, mWndOrgY, mWndW, mWndH, NULL, NULL, mhInstance, NULL);
	if (mhWnd) {
		ShowWindow(mhWnd, SW_SHOW);
		UpdateWindow(mhWnd);
		mhDC = GetDC(mhWnd);
	}
#elif defined(OGLSYS_ANDROID)
	android_app* pApp = oglsys_get_app();
	if (!pApp) return;

	pApp->onAppCmd = app_cmd;

	while (true) {
		int fd;
		int events;
		android_poll_source* pPollSrc;
		while (true) {
			int id = ALooper_pollAll(0, &fd, &events, (void**)&pPollSrc);
			if (id < 0) break;
			if (pPollSrc) {
				pPollSrc->process(pApp, pPollSrc);
			}
			if (pApp->destroyRequested) return;
			if (s_pNativeWnd) break;
		}
		if (s_pNativeWnd) break;
	}

	mpNativeWnd = s_pNativeWnd;
	s_pNativeWnd = nullptr;

	if (mHideSysIcons) {
		ANativeActivity_setWindowFlags(pApp->activity, AWINDOW_FLAG_FULLSCREEN, 0);
	}

	mWidth = ANativeWindow_getWidth(mpNativeWnd);
	mHeight = ANativeWindow_getHeight(mpNativeWnd);
	mWndW = mWidth;
	mWndH = mHeight;
	mInpSclX = mWidth > 0.0f ? 1.0f / float(mWidth) : 1.0f;
	mInpSclY = mHeight > 0.0f ? 1.0f / float(mHeight) : 1.0f;
	dbg_msg("Full resolution: %d x %d\n", mWidth, mHeight);
	if (mReduceRes) {
		float rscl = (mHeight > 720 ? 720.0f : 360.0f) / float(mHeight);
		if (rscl < 1.0f) {
			mWidth = int(float(mWidth) * rscl);
			mHeight = int(float(mHeight) * rscl);
			ANativeWindow_setBuffersGeometry(mpNativeWnd, mWidth, mHeight, ANativeWindow_getFormat(mpNativeWnd));
			dbg_msg("Using reduced resolution: %d x %d\n", mWidth, mHeight);
		}
	}
#elif defined(OGLSYS_X11)
	mpXDisplay = XOpenDisplay(0);
	if (!mpXDisplay) {
		return;
	}
	int defScr = XDefaultScreen(mpXDisplay);
	int defDepth = DefaultDepth(mpXDisplay, defScr);
	XVisualInfo vi = {};
	XMatchVisualInfo(mpXDisplay, defScr, defDepth, TrueColor, &vi);
	Window rootWnd = RootWindow(mpXDisplay, defScr);
	XSetWindowAttributes wattrs = {};
	wattrs.colormap = XCreateColormap(mpXDisplay, rootWnd, vi.visual, AllocNone);
	wattrs.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | KeyPressMask;
	mWndW = mWidth;
	mWndH = mHeight;
	mXWnd = XCreateWindow(mpXDisplay, rootWnd, mWndOrgX, mWndOrgY, mWndW, mWndH, 0, vi.depth, InputOutput, vi.visual, CWBackPixel | CWEventMask | CWColormap, &wattrs);
	char title[100];
	sprintf(title, mWithoutCtx ? "X11SysWnd" : "X11 %s: build %s", OGLSYS_ES ? "OGL(ES)" : "OGL", __DATE__);
	XMapWindow(mpXDisplay, mXWnd);
	XStoreName(mpXDisplay, mXWnd, title);
	XFlush(mpXDisplay);
	Atom del = XInternAtom(mpXDisplay, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(mpXDisplay, mXWnd, &del, 1);
	XEvent evt;
	XIfEvent(mpXDisplay, &evt, wait_MapNotify, (char*)mXWnd);
	XSelectInput(mpXDisplay, mXWnd, ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | PointerMotionMask);
#elif defined(OGLSYS_VIVANTE_FB)
	mVivDisp = fbGetDisplayByIndex(0);
	int vivW = 0;
	int vivH = 0;
	fbGetDisplayGeometry(mVivDisp, &vivW, &vivH);
	dbg_msg("Vivante display: %d x %d\n", vivW, vivH);
	int vivX = 0;
	int vivY = 0;
	if (mReduceRes) {
		const float vivReduce = 0.5f;
		int fullVivW = vivW;
		int fullVivH = vivH;
		vivW = int(float(fullVivW) * vivReduce);
		vivH = int(float(fullVivH) * vivReduce);
		vivX = (fullVivW - vivW) / 2;
		vivY = (fullVivH - vivH) / 2;
		dbg_msg("Reduced resolution: %d x %d\n", vivW, vivH);
	}
	mWidth = vivW;
	mHeight = vivH;
	mWndW = mWidth;
	mWndH = mHeight;
	mVivWnd = fbCreateWindow(mVivDisp, vivX, vivY, mWidth, mHeight);
#elif defined(OGLSYS_DRM_ES)
	mDrmDevFD = open("/dev/dri/card0", O_RDWR);
	drmModeRes* pRsrcs = mDrmDevFD >= 0 ? drmModeGetResources(mDrmDevFD) : nullptr;
	mpDrmConn = nullptr;
	mpDrmEnc = nullptr;
	mDrmModeIdx = -1;
	mDrmCtrlId = -1;
	FD_ZERO(&mDrmDevFDSet);
	if (pRsrcs) {
		for (int i = 0; i < pRsrcs->count_connectors; ++i) {
			drmModeConnector* pConn = drmModeGetConnector(mDrmDevFD, pRsrcs->connectors[i]);
			if (pConn && (pConn->connection == DRM_MODE_CONNECTED)) {
				dbg_msg("DRM Connector #%d\n", i);
				mpDrmConn = pConn;
				break;
			}
			drmModeFreeConnector(pConn);
		}
	}
	if (mpDrmConn) {
		for (int i = 0; i < pRsrcs->count_encoders; ++i) {
			drmModeEncoder* pEnc = drmModeGetEncoder(mDrmDevFD, pRsrcs->encoders[i]);
			if (pEnc && (pEnc->encoder_id == mpDrmConn->encoder_id)) {
				dbg_msg("DRM Encoder #%d\n", i);
				mpDrmEnc = pEnc;
				mDrmCtrlId = mpDrmEnc->crtc_id;
				break;
			}
			drmModeFreeEncoder(pEnc);
		}
		if (GLG.get_int_opt("ogl_list_video_modes", 0)) {
			dbg_msg("DRM modes:\n");
			for (int i = 0; i < mpDrmConn->count_modes; ++i) {
				drmModeModeInfo* pModeInfo = &mpDrmConn->modes[i];
				dbg_msg(" %d) %d x %d, %dHz\n", i, pModeInfo->hdisplay, pModeInfo->vdisplay, pModeInfo->vrefresh);
			}
		}
		int drmModeIdx = GLG.get_int_opt("ogl_drm_mode", -1);
		if (drmModeIdx < 0 || drmModeIdx >= mpDrmConn->count_modes) {
			for (int i = 0; i < mpDrmConn->count_modes; ++i) {
				if (mpDrmConn->modes[i].type & DRM_MODE_TYPE_PREFERRED) {
					drmModeIdx = i;
					break;
				}
			}
		}
		if (drmModeIdx >= 0 && drmModeIdx < mpDrmConn->count_modes) {
			mDrmModeIdx = drmModeIdx;
			mWidth = mpDrmConn->modes[mDrmModeIdx].hdisplay;
			mHeight = mpDrmConn->modes[mDrmModeIdx].vdisplay;
			mWndW = mWidth;
			mWndH = mHeight;
			dbg_msg("DRM mode #%d (%d x %d)\n", mDrmModeIdx, mWidth, mHeight);
		} else {
			dbg_msg("Invalid DRM mode.\n");
		}
	}
	if (mDrmDevFD >= 0) {
		FD_SET(0, &mDrmDevFDSet);
		FD_SET(mDrmDevFD, &mDrmDevFDSet);
		mpGbmDev = gbm_create_device(mDrmDevFD);
		if (mpGbmDev) {
			mpGbmSurf = gbm_surface_create(mpGbmDev, mWidth, mHeight, GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
		}
		if (mpGbmSurf) {
			dbg_msg("GBM surf @ %p\n", mpGbmSurf);
		}
	}
#elif defined(OGLSYS_WEB)
	mWndW = mWidth;
	mWndH = mHeight;
	SDL_CreateWindowAndRenderer(mWidth, mHeight, SDL_WINDOW_OPENGL, &mpSDLWnd, &mpSDLRdr);
#endif

#if !defined(OGLSYS_ANDROID)
	mInpSclX = mWidth > 0.0f ? 1.0f / float(mWidth) : 1.0f;
	mInpSclY = mHeight > 0.0f ? 1.0f / float(mHeight) : 1.0f;
#endif
}

void OGLSysGlb::reset_wnd() {
#if defined(OGLSYS_WINDOWS)
	if (mhDC) {
		ReleaseDC(mhWnd, mhDC);
		mhDC = NULL;
	}
	if (mhWnd) {
		DestroyWindow(mhWnd);
		mhWnd = NULL;
	}
	UnregisterClass(s_oglClassName, mhInstance);
#elif defined(OGLSYS_X11)
	if (mpXDisplay) {
		XDestroyWindow(mpXDisplay, mXWnd);
		XCloseDisplay(mpXDisplay);
		mpXDisplay = nullptr;
	}
#elif defined(OGLSYS_VIVANTE_FB)
	fbDestroyWindow(mVivWnd);
	fbDestroyDisplay(mVivDisp);
#elif defined(OGLSYS_WEB)
	mpSDLWnd = nullptr;
#endif
}

#ifdef OGLSYS_DRM_ES
#	ifndef OGLSYS_DRM_ES_DEF_ALPHA_SIZE
#		define OGLSYS_DRM_ES_DEF_ALPHA_SIZE 0
#	endif
#	ifndef OGLSYS_DRM_ES_DEF_DEPTH_SIZE
#		define OGLSYS_DRM_ES_DEF_DEPTH_SIZE 16
#	endif
#endif

#if defined(OGLSYS_VIVANTE_FB)
#	define OGLSYS_ES_ALPHA_SIZE 0
#	define OGLSYS_ES_DEPTH_SIZE 16
#elif defined(OGLSYS_DRM_ES)
#	define OGLSYS_ES_ALPHA_SIZE OGLSYS_DRM_ES_DEF_ALPHA_SIZE
#	define OGLSYS_ES_DEPTH_SIZE OGLSYS_DRM_ES_DEF_DEPTH_SIZE
#else
#	define OGLSYS_ES_ALPHA_SIZE 8
#	define OGLSYS_ES_DEPTH_SIZE 24
#endif

void OGLSysGlb::init_ogl() {
	mSwapInterval = 1;
	if (mWithoutCtx) {
		return;
	}
#if OGLSYS_ES
#	if defined(OGLSYS_ANDROID)
	mEGL.display = eglGetDisplay((EGLNativeDisplayType)0);
#	elif defined(OGLSYS_WINDOWS)
	if (mhDC) {
		mEGL.display = eglGetDisplay(mhDC);
	}
#	elif defined(OGLSYS_X11)
	if (mpXDisplay) {
		mEGL.display = eglGetDisplay((EGLNativeDisplayType)mpXDisplay);
	}
#	elif defined(OGLSYS_VIVANTE_FB)
	mEGL.display = eglGetDisplay(mVivDisp);
#elif defined(OGLSYS_DRM_ES)
	PFNEGLGETPLATFORMDISPLAYEXTPROC pfnGetDisp = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
	if (pfnGetDisp) {
		mEGL.display = pfnGetDisp(EGL_PLATFORM_GBM_KHR, mpGbmDev, nullptr);
	}
#	endif
	if (!valid_display()) return;
	int verMaj = 0;
	int verMin = 0;
	bool flg = !!eglInitialize(mEGL.display, &verMaj, &verMin);
	if (!flg) return;
	dbg_msg("EGL %d.%d\n", verMaj, verMin);
	flg = !!eglBindAPI(EGL_OPENGL_ES_API);
	if (flg != EGL_TRUE) return;

	static EGLint cfgAttrs[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, OGLSYS_ES_ALPHA_SIZE,
		EGL_DEPTH_SIZE, OGLSYS_ES_DEPTH_SIZE,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
		EGL_NONE
	};
	EGLint cfgAttrsMSAA[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, OGLSYS_ES_ALPHA_SIZE,
		EGL_DEPTH_SIZE, OGLSYS_ES_DEPTH_SIZE,
		EGL_SAMPLE_BUFFERS, 1,
		EGL_SAMPLES, (EGLint)mNumMSAASamples,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
		EGL_NONE
	};
	EGLint* pCfgAttrs = cfgAttrs;
	if (mNumMSAASamples > 0) {
		pCfgAttrs = cfgAttrsMSAA;
	}
	bool useGLES2 = false;
	EGLint ncfg = 0;
	flg = !!eglChooseConfig(mEGL.display, pCfgAttrs, &mEGL.config, 1, &ncfg);
	if (!flg) {
		static EGLint cfgAttrs2[] = {
			EGL_RED_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_BLUE_SIZE, 8,
			EGL_ALPHA_SIZE, OGLSYS_ES_ALPHA_SIZE,
			EGL_DEPTH_SIZE, OGLSYS_ES_DEPTH_SIZE,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_NONE
		};
		useGLES2 = true;
		flg = !!eglChooseConfig(mEGL.display, cfgAttrs2, &mEGL.config, 1, &ncfg);
	}
	if (flg) flg = ncfg == 1;
	if (!flg) return;
	EGLNativeWindowType hwnd =
#if defined(OGLSYS_ANDROID)
		mpNativeWnd
#elif defined(OGLSYS_WINDOWS)
		mhWnd
#elif defined(OGLSYS_X11)
		(EGLNativeWindowType)mXWnd
#elif defined(OGLSYS_VIVANTE_FB)
		mVivWnd
#elif defined(OGLSYS_DRM_ES)
		mpGbmSurf
#else
		(EGLNativeWindowType)0
#endif
		;
	mEGL.surface = eglCreateWindowSurface(mEGL.display, mEGL.config, hwnd, nullptr);
	if (!valid_surface()) return;

	static EGLint ctxAttrs[] = {
		EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
		EGL_CONTEXT_MINOR_VERSION_KHR, 1,
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};
	static EGLint ctxAttrs2[] = {
			EGL_CONTEXT_CLIENT_VERSION, 2,
			EGL_NONE
	};
	mEGL.context = eglCreateContext(mEGL.display, mEGL.config, nullptr, useGLES2 ? ctxAttrs2 : ctxAttrs);
	if (!valid_context() && !useGLES2) {
		static EGLint ctxAttrs3[] = {
			EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
			EGL_CONTEXT_MINOR_VERSION_KHR, 0,
			EGL_CONTEXT_CLIENT_VERSION, 3,
			EGL_NONE
		};
		mEGL.context = eglCreateContext(mEGL.display, mEGL.config, nullptr, ctxAttrs3);
	}
	if (!valid_context()) return;
	eglMakeCurrent(mEGL.display, mEGL.surface, mEGL.surface, mEGL.context);
	eglSwapInterval(mEGL.display, 1);
#elif defined(OGLSYS_WINDOWS)
	mWGL.ok = false;
	mWGL.hLib = LoadLibraryW(L"opengl32.dll");
	if (!mWGL.hLib) return;
	GET_WGL_FN(wglGetProcAddress);
	if (!mWGL.wglGetProcAddress) return;
	GET_WGL_FN(wglCreateContext);
	if (!mWGL.wglCreateContext) return;
	GET_WGL_FN(wglDeleteContext);
	if (!mWGL.wglDeleteContext) return;
	GET_WGL_FN(wglMakeCurrent);
	if (!mWGL.wglMakeCurrent) return;

	mWGL.init_ctx(mhDC);
	if (!mWGL.valid_ctx()) return;

	GET_WGL_FN(wglCreateContextAttribsARB);
	GET_WGL_FN(wglChoosePixelFormatARB);

	GET_WGL_FN(wglGetExtensionsStringEXT);
	GET_WGL_FN(wglSwapIntervalEXT);
	GET_WGL_FN(wglGetSwapIntervalEXT);

	if (mNumMSAASamples > 0) {
		int msaaFmt = mWGL.get_msaa_pixfmt(mhDC, mNumMSAASamples);
		if (msaaFmt) {
			mWGL.wglMakeCurrent(mhDC, nullptr);
			mWGL.reset_ctx();
			s_wndQuitOnDestroy = false;
			reset_wnd();
			s_wndQuitOnDestroy = true;
			init_wnd();
			mWGL.init_ctx(mhDC, msaaFmt);
			if (!mWGL.valid_ctx()) return;
		}
	}

	mWGL.init_ext_ctx(mhDC);

#	define OGL_FN(_type, _name) GET_GL_FN(gl##_name);
#	undef OGL_FN_CORE
#	define OGL_FN_CORE
#	undef OGL_FN_EXTRA
#	define OGL_FN_EXTRA
#	include "oglsys.inc"
#	undef OGL_FN

	int allCnt = 0;
	int okCnt = 0;
#	define OGL_FN(_type, _name) if (gl##_name) { ++okCnt; } else { dbg_msg("missing OGL core function #%d: %s\n", allCnt, "gl"#_name); } ++allCnt;
#	undef OGL_FN_CORE
#	define OGL_FN_CORE
#	undef OGL_FN_EXTRA
#	include "oglsys.inc"
#	undef OGL_FN
	dbg_msg("OGL core functions: %d/%d\n", okCnt, allCnt);
	if (allCnt != okCnt) {
		mWGL.reset_ctx();
		return;
	}

	allCnt = 0;
	okCnt = 0;
#	define OGL_FN(_type, _name) if (gl##_name) { ++okCnt; } else { dbg_msg("missing OGL extra function #%d: %s\n", allCnt, "gl"#_name); } ++allCnt;
#	undef OGL_FN_CORE
#	undef OGL_FN_EXTRA
#	define OGL_FN_EXTRA
#	include "oglsys.inc"
#	undef OGL_FN
	dbg_msg("OGL extra functions: %d/%d\n", okCnt, allCnt);

	mWGL.set_swap_interval(1);
	mWGL.ok = true;
#elif defined(OGLSYS_X11)
	if (mpXDisplay) {
		mGLX.init();
		GLint viAttrsD16[] = {
			GLX_RGBA,
			GLX_DEPTH_SIZE, 16,
			GLX_DOUBLEBUFFER,
			None
		};
		GLint viAttrs[] = {
			GLX_RGBA,
			GLX_DEPTH_SIZE, 24,
			GLX_DOUBLEBUFFER,
			None
		};
		int useD16Def = 0;
#if defined(OGLSYS_ILLUMOS)
		dbg_msg("!!! illumos fix: 16-bit depth default !!!\n");
		useD16Def = 1;
#endif
		GLint* pViAttrs = viAttrs;
		if (get_int_opt("ogl_d16", useD16Def) != 0) {
			dbg_msg("OGL: using d16!\n");
			pViAttrs = viAttrsD16;
		}
		XVisualInfo* pVI = mGLX.mpfnGLXChooseVisual(mpXDisplay, 0, pViAttrs);
		mGLX.mCtx = mGLX.mpfnGLXCreateContext(mpXDisplay, pVI, NULL, GL_TRUE);
		if (mGLX.mCtx) {
			mGLX.mpfnGLXMakeCurrent(mpXDisplay, mXWnd, mGLX.mCtx);
		}
	}
#	define OGL_FN(_type, _name) GET_GL_FN(gl##_name);
#	undef OGL_FN_CORE
#	define OGL_FN_CORE
#	undef OGL_FN_EXTRA
#	define OGL_FN_EXTRA
#	include "oglsys.inc"
#	undef OGL_FN

	int allCnt = 0;
	int okCnt = 0;
#	define OGL_FN(_type, _name) if (gl##_name) { ++okCnt; } else { dbg_msg("missing OGL core function #%d: %s\n", allCnt, "gl"#_name); } ++allCnt;
#	undef OGL_FN_CORE
#	define OGL_FN_CORE
#	undef OGL_FN_EXTRA
#	include "oglsys.inc"
#	undef OGL_FN
	dbg_msg("OGL core functions: %d/%d\n", okCnt, allCnt);
	if (allCnt != okCnt) {
		mGLX.reset();
		return;
	}

	allCnt = 0;
	okCnt = 0;
#	define OGL_FN(_type, _name) if (gl##_name) { ++okCnt; } else { dbg_msg("missing OGL extra function #%d: %s\n", allCnt, "gl"#_name); } ++allCnt;
#	undef OGL_FN_CORE
#	undef OGL_FN_EXTRA
#	define OGL_FN_EXTRA
#	include "oglsys.inc"
#	undef OGL_FN
	dbg_msg("OGL extra functions: %d/%d\n", okCnt, allCnt);
#elif defined(OGLSYS_DUMMY)
	OGLSys::dummyglInit();
#endif

#if !defined(OGLSYS_WEB)
	*(void**)&mpfnGlFinish = OGLSys::get_proc_addr("glFlush");
	*(void**)&mpfnGlFinish = OGLSys::get_proc_addr("glFinish");
#endif

	dbg_msg("OGL version: %s\n", glGetString(GL_VERSION));
	dbg_msg("OGL platform: %s, %s\n\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mDefFBO);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mMaxTexSize);

	const GLubyte* pExts = glGetString(GL_EXTENSIONS);
	if (pExts) {
		bool dumpExts = get_int_opt("oglsys_dump_exts", 0) != 0;
		int iext = 0;
		int iwk = 0;
		bool extsDone = false;
		char extsDumpStr[64];
		if (dumpExts) {
			dbg_msg("%s\n", "OGL Extensions:");
		}
		while (!extsDone) {
			GLubyte extc = pExts[iwk];
			extsDone = extc == 0;
			if (extsDone || extc == ' ') {
				int extLen = iwk - iext;
				const GLubyte* pExtStr = &pExts[iext];
				iext = iwk + 1;
				if (extLen > 1) {
					if (dumpExts) {
						int dumpLen = int(sizeof(extsDumpStr) - 1);
						if (extLen < dumpLen) {
							dumpLen = extLen;
						}
						for (int i = 0; i < dumpLen; ++i) {
							extsDumpStr[i] = (char)pExtStr[i];
						}
						extsDumpStr[dumpLen] = 0;
						dbg_msg(" %s\n", extsDumpStr);
					}
					handle_ogl_ext(pExtStr, extLen);
				}
			}
			++iwk;
		}
	}


#if OGLSYS_ES
	mExts.pfnGenQueries = (PFNGLGENQUERIESEXTPROC)eglGetProcAddress("glGenQueriesEXT");
	mExts.pfnDeleteQueries = (PFNGLDELETEQUERIESEXTPROC)eglGetProcAddress("glDeleteQueriesEXT");
	mExts.pfnQueryCounter = (PFNGLQUERYCOUNTEREXTPROC)eglGetProcAddress("glQueryCounterEXT");
	mExts.pfnGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVEXTPROC)eglGetProcAddress("glGetQueryObjectivEXT");
	mExts.pfnGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVEXTPROC)eglGetProcAddress("glGetQueryObjectuivEXT");
	mExts.pfnGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VEXTPROC)eglGetProcAddress("glGetQueryObjectui64vEXT");
	mExts.pfnGetProgramBinary = (PFNGLGETPROGRAMBINARYOESPROC)eglGetProcAddress("glGetProgramBinaryOES");
	mExts.pfnProgramBinary = (PFNGLPROGRAMBINARYOESPROC)eglGetProcAddress("glProgramBinaryOES");
	mExts.pfnDiscardFramebuffer = (PFNGLDISCARDFRAMEBUFFEREXTPROC)eglGetProcAddress("glDiscardFramebufferEXT");
	mExts.pfnGenVertexArrays = (PFNGLGENVERTEXARRAYSOESPROC)eglGetProcAddress("glGenVertexArraysOES");
	mExts.pfnDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSOESPROC)eglGetProcAddress("glDeleteVertexArraysOES");
	mExts.pfnBindVertexArray = (PFNGLBINDVERTEXARRAYOESPROC)eglGetProcAddress("glBindVertexArrayOES");
	#ifdef OGLSYS_VIVANTE_FB
	mExts.pfnDrawElementsBaseVertex = nullptr;
	#else
	mExts.pfnDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXOESPROC)eglGetProcAddress("glDrawElementsBaseVertexOES");
	#endif
	mExts.pfnGetUniformBlockIndex = (OGLSYS_PFNGLGETUNIFORMBLOCKINDEXPROC)eglGetProcAddress("glGetUniformBlockIndex");
	mExts.pfnGetActiveUniformBlockiv = (OGLSYS_PFNGLGETACTIVEUNIFORMBLOCKIVPROC)eglGetProcAddress("glGetActiveUniformBlockiv");
	mExts.pfnGetUniformIndices = (OGLSYS_PFNGLGETUNIFORMINDICESPROC)eglGetProcAddress("glGetUniformIndices");
	mExts.pfnGetActiveUniformsiv = (OGLSYS_PFNGLGETACTIVEUNIFORMSIVPROC)eglGetProcAddress("glGetActiveUniformsiv");
	mExts.pfnMapBufferRange = (OGLSYS_PFNGLMAPBUFFERRANGEPROC)eglGetProcAddress("glMapBufferRange");
	mExts.pfnUnmapBuffer = (OGLSYS_PFNGLUNMAPBUFFERPROC)eglGetProcAddress("glUnmapBuffer");
	mExts.pfnUniformBlockBinding = (OGLSYS_PFNGLUNIFORMBLOCKBINDINGPROC)eglGetProcAddress("glUniformBlockBinding");
	mExts.pfnBindBufferBase = (OGLSYS_PFNGLBINDBUFFERBASEPROC)eglGetProcAddress("glBindBufferBase");
#endif

#if defined(OGLSYS_VIVANTE_FB)
	if (mReduceRes) {
		int vivW = 0;
		int vivH = 0;
		fbGetDisplayGeometry(mVivDisp, &vivW, &vivH);
		EGLNativeWindowType vivWnd = fbCreateWindow(mVivDisp, 0, 0, vivW, vivH);
		EGLSurface vivSurf = eglCreateWindowSurface(mEGL.display, mEGL.config, vivWnd, nullptr);
		eglMakeCurrent(mEGL.display, vivSurf, vivSurf, mEGL.context);
		glViewport(0, 0, vivW, vivH);
		glScissor(0, 0, vivW, vivH);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		for (int i = 0; i < 8; ++i) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			eglSwapBuffers(mEGL.display, vivSurf);
		}
		eglDestroySurface(mEGL.display, vivSurf);
		eglMakeCurrent(mEGL.display, mEGL.surface, mEGL.surface, mEGL.context);
		fbDestroyWindow(vivWnd);
		glViewport(0, 0, mWidth, mHeight);
		glScissor(0, 0, mWidth, mHeight);
	}
#endif

#if defined(OGLSYS_DRM_ES)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	eglSwapBuffers(mEGL.display, mEGL.surface);
	mpGbmBO = gbm_surface_lock_front_buffer(mpGbmSurf);
	uint32_t fbId = prepare_drm_fb();
	uint32_t connId = 0;
	drmModeModeInfo* pMode = nullptr;
	if (mDrmModeIdx >= 0 && mpDrmConn) {
		connId = mpDrmConn->connector_id;
		pMode = &mpDrmConn->modes[mDrmModeIdx];
	}
	if (pMode) {
		if (drmModeSetCrtc(mDrmDevFD, mDrmCtrlId, fbId, 0, 0, &connId, 1, pMode) == 0) {
			dbg_msg("DRM mode set: OK\n");
		}
	}
#endif

#if !OGLSYS_ES && !defined(OGLSYS_WEB)
	glDisable(GL_MULTISAMPLE);
#endif
}

void OGLSysGlb::reset_ogl() {
	if (mWithoutCtx) {
		return;
	}
	if (mDefTexs.black) {
		glDeleteTextures(1, &mDefTexs.black);
		mDefTexs.black = 0;
	}
	if (mDefTexs.white) {
		glDeleteTextures(1, &mDefTexs.white);
		mDefTexs.white = 0;
	}
#if OGLSYS_ES
	mEGL.reset();
#elif defined(OGLSYS_WINDOWS)
	mWGL.reset_ctx();
	if (mWGL.hLib) {
		FreeLibrary(mWGL.hLib);
	}
	ZeroMemory(&mWGL, sizeof(mWGL));
#elif defined(OGLSYS_X11)
	if (mGLX.valid()) {
		mGLX.mpfnGLXDestroyContext(mpXDisplay, mGLX.mCtx);
	}
#endif
}


#if defined(_MSC_VER)
	__declspec(noinline)
#elif defined(__GNUC__)
	__attribute__((noinline))
#endif
void OGLSysCfg::clear() {
	oglsys_mem_set((void*)this, 0, sizeof(OGLSysCfg));
}


void OGLSysGlb::handle_ogl_ext(const GLubyte* pStr, const int lenStr) {
	char buf[128];
	if ((size_t)lenStr < sizeof(buf) - 1) {
		oglsys_mem_set(buf, 0, sizeof(buf));
		oglsys_mem_cpy(buf, pStr, lenStr);
		if (oglsys_str_eq(buf, "GL_KHR_texture_compression_astc_ldr")) {
			mExts.ASTC_LDR = true;
		} else if (oglsys_str_eq(buf, "GL_EXT_texture_compression_dxt1")) {
			mExts.DXT1 = true;
		} else if (oglsys_str_eq(buf, "GL_EXT_texture_compression_s3tc")) {
			mExts.S3TC = true;
		} else if (oglsys_str_eq(buf, "GL_EXT_disjoint_timer_query")) {
			mExts.disjointTimer = true;
		} else if (oglsys_str_eq(buf, "GL_ARB_bindless_texture")) {
			mExts.bindlessTex = true;
		} else if (oglsys_str_eq(buf, "GL_OES_standard_derivatives")) {
			mExts.derivs = true;
		} else if (oglsys_str_eq(buf, "GL_OES_vertex_half_float")) {
			mExts.vtxHalf = true;
		} else if (oglsys_str_eq(buf, "GL_OES_element_index_uint")) {
			mExts.idxUInt = true;
		} else if (oglsys_str_eq(buf, "GL_OES_get_program_binary")) {
			mExts.progBin = true;
		} else if (oglsys_str_eq(buf, "GL_EXT_discard_framebuffer")) {
			mExts.discardFB = true;
		} else if (oglsys_str_eq(buf, "GL_ARB_multi_draw_indirect")) {
			mExts.mdi = true;
		} else if (oglsys_str_eq(buf, "GL_NV_vertex_buffer_unified_memory")) {
			mExts.nvVBUM = true;
		} else if (oglsys_str_eq(buf, "GL_NV_uniform_buffer_unified_memory")) {
			mExts.nvUBUM = true;
		} else if (oglsys_str_eq(buf, "GL_NV_bindless_multi_draw_indirect")) {
			mExts.nvBindlessMDI = true;
		} else if (oglsys_str_eq(buf, "GL_NV_command_list")) {
			mExts.nvCmdLst = true;
		}
	}
}

#if defined(OGLSYS_X11)
static bool xbtn_xlat(const XEvent& evt, OGLSysMouseState::BTN* pBtn) {
	bool res = true;
	switch (evt.xbutton.button) {
		case 1: *pBtn = OGLSysMouseState::OGLSYS_BTN_LEFT; break;
		case 2: *pBtn = OGLSysMouseState::OGLSYS_BTN_MIDDLE; break;
		case 3: *pBtn = OGLSysMouseState::OGLSYS_BTN_RIGHT; break;
		default: res = false; break;
	}
	return res;
}

static int32_t xbtn_wheel(const XEvent& evt) {
	int32_t res = 0;
	switch (evt.xbutton.button) {
		case 4: res = 1; break;
		case 5: res = -1; break;
		default: break;
	}
	return res;
}
#endif

void OGLSysParamsBuf::init(GLuint progId, const GLchar* pName, GLuint binding, const GLchar** ppFieldNames, const int numFields) {
	mpStorage = nullptr;
	mBufHandle = 0;
#if OGLSYS_ES
	if (!GLG.mExts.pfnGetUniformBlockIndex) {
		return;
	}
#endif
	mpName = pName;
	mProgId = progId;
#if OGLSYS_ES
	mBlockIdx = GLG.mExts.pfnGetUniformBlockIndex(mProgId, pName);
#else
	mBlockIdx = glGetUniformBlockIndex(mProgId, pName);
#endif
	if (mBlockIdx == GL_INVALID_INDEX) {
		return;
	}
	glGenBuffers(1, &mBufHandle);
	mBindingIdx = binding;
#if OGLSYS_ES
	GLG.mExts.pfnGetActiveUniformBlockiv(mProgId, mBlockIdx, GL_UNIFORM_BLOCK_DATA_SIZE, &mSize);
#else
	glGetActiveUniformBlockiv(mProgId, mBlockIdx, GL_UNIFORM_BLOCK_DATA_SIZE, &mSize);
#endif
	mpStorage = GLG.mem_alloc(mSize, "OGLSys:PrmsBuf:storage");
	if (mpStorage) {
		oglsys_mem_set(mpStorage, 0, mSize);
	}
	glBindBuffer(GL_UNIFORM_BUFFER, mBufHandle);
	glBufferData(GL_UNIFORM_BUFFER, mSize, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	mNumFields = numFields;
	mpFieldNames = ppFieldNames;
	mpFieldIndices = (GLuint*)GLG.mem_alloc(mNumFields * sizeof(GLuint), "OGLSys:PrmsBuf:indices");
	if (mpFieldIndices) {
#if OGLSYS_ES
		GLG.mExts.pfnGetUniformIndices(mProgId, mNumFields, mpFieldNames, mpFieldIndices);
#else
		glGetUniformIndices(mProgId, mNumFields, mpFieldNames, mpFieldIndices);
#endif
	}
	mpFieldOffsets = (GLint*)GLG.mem_alloc(mNumFields * sizeof(GLint), "OGLSys:PrmsBuf:offsets");
	if (mpFieldOffsets) {
#if OGLSYS_ES
		GLG.mExts.pfnGetActiveUniformsiv(mProgId, mNumFields, mpFieldIndices, GL_UNIFORM_OFFSET, mpFieldOffsets);
#else
		glGetActiveUniformsiv(mProgId, mNumFields, mpFieldIndices, GL_UNIFORM_OFFSET, mpFieldOffsets);
#endif
	}
	mpFieldTypes = (GLint*)GLG.mem_alloc(mNumFields * sizeof(GLint), "OGLSys:PrmsBuf:types");
	if (mpFieldTypes) {
#if OGLSYS_ES
		GLG.mExts.pfnGetActiveUniformsiv(mProgId, mNumFields, mpFieldIndices, GL_UNIFORM_TYPE, mpFieldTypes);
#else
		glGetActiveUniformsiv(mProgId, mNumFields, mpFieldIndices, GL_UNIFORM_TYPE, mpFieldTypes);
#endif
	}
	mUpdateFlg = true;
}

void OGLSysParamsBuf::reset() {
	if (mBufHandle) {
		glDeleteBuffers(1, &mBufHandle);
		mBufHandle = 0;
	}
	if (mNumFields) {
		GLG.mem_free(mpFieldIndices);
		mpFieldIndices = nullptr;
		GLG.mem_free(mpFieldOffsets);
		mpFieldOffsets = nullptr;
		GLG.mem_free(mpFieldTypes);
		mpFieldTypes = nullptr;
		mNumFields = 0;
	}
	if (mpStorage) {
		GLG.mem_free(mpStorage);
		mpStorage = nullptr;
	}
}

int OGLSysParamsBuf::find_field(const char* pName) {
	int idx = -1;
	if (pName && mpFieldNames) {
		for (int i = 0; i < mNumFields; ++i) {
			if (oglsys_str_eq(mpFieldNames[i], pName)) {
				idx = i;
				break;
			}
		}
	}
	return idx;
}

static void prmsbuf_set_flt(OGLSysParamsBuf& buf, float* pFlt, const float val) {
	if (*pFlt != val) {
		*pFlt = val;
		buf.mUpdateFlg = true;
	}
}

void OGLSysParamsBuf::set(const int idx, const float x) {
	if (ck_field_idx(idx)) {
		GLint offs = mpFieldOffsets[idx];
		if (offs >= 0) {
			void* pData = ((uint8_t*)mpStorage) + offs;
			GLfloat* pFlt = (GLfloat*)pData;
			switch (mpFieldTypes[idx]) {
				case GL_FLOAT:
					prmsbuf_set_flt(*this, pFlt, x);
					break;
				case GL_FLOAT_VEC2:
					prmsbuf_set_flt(*this, &pFlt[0], x);
					prmsbuf_set_flt(*this, &pFlt[1], x);
					break;
				case GL_FLOAT_VEC3:
					prmsbuf_set_flt(*this, &pFlt[0], x);
					prmsbuf_set_flt(*this, &pFlt[1], x);
					prmsbuf_set_flt(*this, &pFlt[2], x);
					break;
				case GL_FLOAT_VEC4:
					prmsbuf_set_flt(*this, &pFlt[0], x);
					prmsbuf_set_flt(*this, &pFlt[1], x);
					prmsbuf_set_flt(*this, &pFlt[2], x);
					prmsbuf_set_flt(*this, &pFlt[3], x);
					break;
				default:
					break;
			}
		}
	}
}

void OGLSysParamsBuf::set(const int idx, const float x, const float y) {
	if (ck_field_idx(idx)) {
		GLint offs = mpFieldOffsets[idx];
		if (offs >= 0) {
			void* pData = ((uint8_t*)mpStorage) + offs;
			GLfloat* pFlt = (float*)pData;
			switch (mpFieldTypes[idx]) {
				case GL_FLOAT:
					prmsbuf_set_flt(*this, pFlt, x);
					break;
				case GL_FLOAT_VEC2:
					prmsbuf_set_flt(*this, &pFlt[0], x);
					prmsbuf_set_flt(*this, &pFlt[1], y);
					break;
				case GL_FLOAT_VEC3:
					prmsbuf_set_flt(*this, &pFlt[0], x);
					prmsbuf_set_flt(*this, &pFlt[1], y);
					prmsbuf_set_flt(*this, &pFlt[2], 0.0f);
					break;
				case GL_FLOAT_VEC4:
					prmsbuf_set_flt(*this, &pFlt[0], x);
					prmsbuf_set_flt(*this, &pFlt[1], y);
					prmsbuf_set_flt(*this, &pFlt[2], 0.0f);
					prmsbuf_set_flt(*this, &pFlt[3], 0.0f);
					break;
				default:
					break;
			}
		}
	}
}

void OGLSysParamsBuf::set(const int idx, const float x, const float y, const float z) {
	if (ck_field_idx(idx)) {
		GLint offs = mpFieldOffsets[idx];
		if (offs >= 0) {
			void* pData = ((uint8_t*)mpStorage) + offs;
			GLfloat* pFlt = (float*)pData;
			switch (mpFieldTypes[idx]) {
				case GL_FLOAT:
					prmsbuf_set_flt(*this, pFlt, x);
					break;
				case GL_FLOAT_VEC2:
					prmsbuf_set_flt(*this, &pFlt[0], x);
					prmsbuf_set_flt(*this, &pFlt[1], y);
					break;
				case GL_FLOAT_VEC3:
					prmsbuf_set_flt(*this, &pFlt[0], x);
					prmsbuf_set_flt(*this, &pFlt[1], y);
					prmsbuf_set_flt(*this, &pFlt[2], z);
					break;
				case GL_FLOAT_VEC4:
					prmsbuf_set_flt(*this, &pFlt[0], x);
					prmsbuf_set_flt(*this, &pFlt[1], y);
					prmsbuf_set_flt(*this, &pFlt[2], z);
					prmsbuf_set_flt(*this, &pFlt[3], 0.0f);
					break;
				default:
					break;
			}
		}
	}
}

void OGLSysParamsBuf::set(const int idx, const float x, const float y, const float z, const float w) {
	if (ck_field_idx(idx)) {
		GLint offs = mpFieldOffsets[idx];
		if (offs >= 0) {
			void* pData = ((uint8_t*)mpStorage) + offs;
			GLfloat* pFlt = (float*)pData;
			switch (mpFieldTypes[idx]) {
				case GL_FLOAT:
					prmsbuf_set_flt(*this, pFlt, x);
					break;
				case GL_FLOAT_VEC2:
					prmsbuf_set_flt(*this, &pFlt[0], x);
					prmsbuf_set_flt(*this, &pFlt[1], y);
					break;
				case GL_FLOAT_VEC3:
					prmsbuf_set_flt(*this, &pFlt[0], x);
					prmsbuf_set_flt(*this, &pFlt[1], y);
					prmsbuf_set_flt(*this, &pFlt[2], z);
					break;
				case GL_FLOAT_VEC4:
					prmsbuf_set_flt(*this, &pFlt[0], x);
					prmsbuf_set_flt(*this, &pFlt[1], y);
					prmsbuf_set_flt(*this, &pFlt[2], z);
					prmsbuf_set_flt(*this, &pFlt[3], w);
					break;
				default:
					break;
			}
		}
	}
}

void OGLSysParamsBuf::send() {
	if (mpStorage && mUpdateFlg) {
		glBindBuffer(GL_UNIFORM_BUFFER, mBufHandle);
#if OGLSYS_ES
		void* pDst = GLG.mExts.pfnMapBufferRange(GL_UNIFORM_BUFFER, 0, mSize, GL_MAP_WRITE_BIT);
#else
		void* pDst = glMapBufferRange(GL_UNIFORM_BUFFER, 0, mSize, GL_MAP_WRITE_BIT);
#endif
		if (pDst) {
			oglsys_mem_cpy(pDst, mpStorage, mSize);
#if OGLSYS_ES
			GLG.mExts.pfnUnmapBuffer(GL_UNIFORM_BUFFER);
#else
			glUnmapBuffer(GL_UNIFORM_BUFFER);
#endif
		}
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		mUpdateFlg = false;
	}
}

void OGLSysParamsBuf::bind() {
#if OGLSYS_ES
	GLG.mExts.pfnUniformBlockBinding(mProgId, mBlockIdx, mBindingIdx);
	GLG.mExts.pfnBindBufferBase(GL_UNIFORM_BUFFER, mBindingIdx, mBufHandle);
#else
	glUniformBlockBinding(mProgId, mBlockIdx, mBindingIdx);
	glBindBufferBase(GL_UNIFORM_BUFFER, mBindingIdx, mBufHandle);
#endif
}

static void create_const_tex(GLuint* pHandle, uint32_t rgba) {
	if (!pHandle) return;
	if (*pHandle) return;
	glGenTextures(1, pHandle);
	if (*pHandle) {
		glBindTexture(GL_TEXTURE_2D, *pHandle);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &rgba);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

#if defined(OGLSYS_LINUX_INPUT)
static void oglsys_linux_input() {
	if (GLG.mRawKbdFD >= 0) {
		size_t readSize = sizeof(input_event);
		bool done = false;
		while (!done) {
			size_t nread = ::read(GLG.mRawKbdFD, &GLG.mRawKbdEvent, readSize);
			if (nread == readSize) {
				if (GLG.mRawKbdEvent.type == EV_KEY) {
					bool* pState = nullptr;
					int idx = -1;
					switch (GLG.mRawKbdEvent.code) {
						case KEY_UP:
						case BTN_DPAD_UP:
							pState = GLG.mKbdState.ctrl;
							idx = KBD_CTRL_UP;
							break;
						case KEY_DOWN:
						case BTN_DPAD_DOWN:
							pState = GLG.mKbdState.ctrl;
							idx = KBD_CTRL_DOWN;
							break;
						case KEY_LEFT:
						case BTN_DPAD_LEFT:
							pState = GLG.mKbdState.ctrl;
							idx = KBD_CTRL_LEFT;
							break;
						case KEY_RIGHT:
						case BTN_DPAD_RIGHT:
							pState = GLG.mKbdState.ctrl;
							idx = KBD_CTRL_RIGHT;
							break;
						case KEY_TAB:
						case BTN_NORTH:
							pState = GLG.mKbdState.ctrl;
							idx = KBD_CTRL_TAB;
							break;
						case KEY_BACKSPACE:
						case BTN_WEST:
							pState = GLG.mKbdState.ctrl;
							idx = KBD_CTRL_BACK;
							break;
						case KEY_LEFTSHIFT:
						case BTN_TL:
							pState = GLG.mKbdState.ctrl;
							idx = KBD_CTRL_LSHIFT;
							break;
						case KEY_RIGHTSHIFT:
						case BTN_TR:
							pState = GLG.mKbdState.ctrl;
							idx = KBD_CTRL_RSHIFT;
							break;
						case KEY_LEFTCTRL:
						case BTN_TL2:
							pState = GLG.mKbdState.ctrl;
							idx = KBD_CTRL_LCTRL;
							break;
						case KEY_RIGHTCTRL:
						case BTN_TR2:
							pState = GLG.mKbdState.ctrl;
							idx = KBD_CTRL_RCTRL;
							break;
						case KEY_ENTER:
						case BTN_EAST:
							pState = GLG.mKbdState.ctrl;
							idx = KBD_CTRL_ENTER;
							break;
						case KEY_SPACE:
						case BTN_SOUTH:
							pState = GLG.mKbdState.punct;
							idx = KBD_PUNCT_SPACE;
							break;
						case KEY_F1:
						case 704:
							pState = GLG.mKbdState.func;
							idx = 0;
							break;
						case KEY_F2:
						case 705:
							pState = GLG.mKbdState.func;
							idx = 1;
							break;
						case KEY_F3:
						case 706:
							pState = GLG.mKbdState.func;
							idx = 2;
							break;
						case KEY_F4:
						case 707:
							pState = GLG.mKbdState.func;
							idx = 3;
							break;
						case KEY_F5:
						case 708:
							pState = GLG.mKbdState.func;
							idx = 4;
							break;
						case KEY_F6:
						case 709:
							pState = GLG.mKbdState.func;
							idx = 5;
							break;
						case KEY_F7:
							pState = GLG.mKbdState.func;
							idx = 6;
							break;
						case KEY_F8:
							pState = GLG.mKbdState.func;
							idx = 7;
							break;
						case KEY_F9:
							pState = GLG.mKbdState.func;
							idx = 8;
							break;
						case KEY_F10:
							pState = GLG.mKbdState.func;
							idx = 9;
							break;
						case KEY_F11:
							pState = GLG.mKbdState.func;
							idx = 10;
							break;
						case KEY_F12:
							pState = GLG.mKbdState.func;
							idx = 11;
							break;

						default:
							if (GLG.mRawKbdMode == 1) {
								GLG.dbg_msg("OdJoy unhandled btn: %d\n", GLG.mRawKbdEvent.code);
							}
							break;
					}
					if (pState && idx >= 0) {
						pState[idx] = (GLG.mRawKbdEvent.value != 0);
					}
				} else {
					done = true;
				}
			} else {
				done = true;
			}
		}
	}
}
#endif


namespace OGLSys {

	bool s_initFlg = false;

	void init(const OGLSysCfg& cfg) {
		if (s_initFlg) return;
		void* pGLG = &GLG;
		oglsys_mem_set(pGLG, 0, sizeof(GLG));
#ifdef OGLSYS_WINDOWS
		::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)wnd_proc, &GLG.mhInstance);
#endif
		GLG.mIfc = cfg.ifc;
		GLG.mWndOrgX = cfg.x;
		GLG.mWndOrgY = cfg.y;
		GLG.mWidth = cfg.width;
		GLG.mHeight = cfg.height;
		GLG.mNumMSAASamples = cfg.msaa;
		GLG.mReduceRes = cfg.reduceRes;
		GLG.mHideSysIcons = cfg.hideSysIcons;
		GLG.mWithoutCtx = cfg.withoutCtx;
		GLG.init_wnd();
		GLG.init_ogl();
#if defined(OGLSYS_LINUX_INPUT)
		GLG.mRawKbdFD = -1;
		GLG.mRawKbdMode = 0;
		const char* pKbdDevName = GLG.get_opt("kbd_dev");
		if (pKbdDevName) {
			GLG.mRawKbdFD = ::open(pKbdDevName, O_RDONLY | O_NONBLOCK);
			if (GLG.mRawKbdFD >= 0) {
				const char* pOdJoyCkStr = "-odroidgo2-joypad-";
				size_t lenOdJoyCkStr = oglsys_str_len(pOdJoyCkStr);
				size_t lenKbdDevName = oglsys_str_len(pKbdDevName);
				if (lenKbdDevName >= lenOdJoyCkStr) {
					bool foundOdJoy = false;
					size_t nck = lenKbdDevName - lenOdJoyCkStr + 1;
					for (size_t i = 0; i < nck; ++i) {
						if (::memcmp(pKbdDevName + i, pOdJoyCkStr, lenOdJoyCkStr) == 0) {
							foundOdJoy = true;
							break;
						}
					}
					if (foundOdJoy) {
						GLG.mRawKbdMode = 1;
					}
				}
			}
		}
#endif
		s_initFlg = true;
	}

	void reset() {
		if (!s_initFlg) return;
#if defined(OGLSYS_LINUX_INPUT)
		if (GLG.mRawKbdFD >= 0) {
			::close(GLG.mRawKbdFD);
		}
		GLG.mRawKbdFD = -1;
#endif
		GLG.reset_ogl();
		GLG.reset_wnd();
		s_initFlg = false;
	}

	void stop() {
		GLG.stop_ogl();
	}

	void swap() {
#if OGLSYS_ES
		if (GLG.mExts.pfnDiscardFramebuffer && GLG.mExts.discardFB) {
			static GLenum exts[] = { GL_DEPTH_EXT, GL_STENCIL_EXT };
			GLG.mExts.pfnDiscardFramebuffer(GL_FRAMEBUFFER, OGLSYS_ARY_LEN(exts), exts);
		}
		eglSwapBuffers(GLG.mEGL.display, GLG.mEGL.surface);
#	if defined(OGLSYS_DRM_ES)
		GLG.exec_drm_flip();
#	endif
#elif defined(OGLSYS_WINDOWS)
		::SwapBuffers(GLG.mhDC);
#elif defined(OGLSYS_X11)
		GLG.mGLX.mpfnGLXSwapBuffers(GLG.mpXDisplay, GLG.mXWnd);
#elif defined(OGLSYS_DUMMY)
		if (GLG.mDummyGL.swap_func) {
			GLG.mDummyGL.swap_func();
		}
#endif
	}

	void loop(void(*pLoop)(void*), void* pLoopCtx) {
#if defined(OGLSYS_WINDOWS)
		MSG msg;
		bool done = false;
		while (!done) {
			if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)) {
				if (GetMessage(&msg, NULL, 0, 0)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				} else {
					done = true;
					break;
				}
			} else {
				if (pLoop) {
					pLoop(pLoopCtx);
				}
				++GLG.mFrameCnt;
			}
		}
#elif defined(OGLSYS_ANDROID)
		android_app* pApp = oglsys_get_app();
		if (!pApp) return;
		while (true) {
			int fd;
			int events;
			android_poll_source* pPollSrc;
			while (true) {
				int id = ALooper_pollAll(0, &fd, &events, (void**)&pPollSrc);
				if (id < 0) break;
				if (id == LOOPER_ID_INPUT) {
					if (pApp->inputQueue && AInputQueue_hasEvents(pApp->inputQueue)) {
						AInputEvent* pInpEvt;
						while (true) {
							pInpEvt = nullptr;
							int32_t inpRes = AInputQueue_getEvent(pApp->inputQueue, &pInpEvt);
							if (inpRes < 0) break;
							if (!pInpEvt) break;
							int32_t inpEvtType = AInputEvent_getType(pInpEvt);
							int32_t evtHandled = 0;
							OGLSysInput inp;
							inp.act = OGLSysInput::OGLSYS_ACT_NONE;
							if (inpEvtType == AINPUT_EVENT_TYPE_MOTION) {
								inp.device = OGLSysInput::OGLSYS_TOUCH;
								inp.sysDevId = AInputEvent_getDeviceId(pInpEvt);
								int32_t act = AMotionEvent_getAction(pInpEvt);
								size_t ntouch = AMotionEvent_getPointerCount(pInpEvt);
								if (act == AMOTION_EVENT_ACTION_MOVE) {
									inp.act = OGLSysInput::OGLSYS_ACT_DRAG;
									for (int i = 0; i < ntouch; ++i) {
										inp.id = AMotionEvent_getPointerId(pInpEvt, i);
										inp.absX = AMotionEvent_getX(pInpEvt, i);
										inp.absY = AMotionEvent_getY(pInpEvt, i);
										inp.pressure = AMotionEvent_getPressure(pInpEvt, i);
										if (inp.id == 0) {
											GLG.update_mouse_pos(inp.absX, inp.absY);
										}
										GLG.send_input(&inp);
									}
								} else if (act == AMOTION_EVENT_ACTION_DOWN || act == AMOTION_EVENT_ACTION_UP) {
									inp.act = act == AMOTION_EVENT_ACTION_DOWN ? OGLSysInput::OGLSYS_ACT_DOWN : OGLSysInput::OGLSYS_ACT_UP;
									inp.id = AMotionEvent_getPointerId(pInpEvt, 0);
									inp.absX = AMotionEvent_getX(pInpEvt, 0);
									inp.absY = AMotionEvent_getY(pInpEvt, 0);
									inp.pressure = AMotionEvent_getPressure(pInpEvt, 0);
									if (inp.id == 0) {
										GLG.set_mouse_pos(inp.absX, inp.absY);
										uint32_t mask = 0;
										if (inp.act == OGLSysInput::OGLSYS_ACT_DOWN) {
											mask = 1 << OGLSysMouseState::OGLSYS_BTN_LEFT;
										}
										GLG.mMouseState.mBtnOld = GLG.mMouseState.mBtnNow;
										GLG.mMouseState.mBtnNow = mask;
									}
									GLG.send_input(&inp);
								}
								evtHandled = 1;
							}
							AInputQueue_finishEvent(pApp->inputQueue, pInpEvt, evtHandled);
						}
					}
				}
				if (pPollSrc) {
					pPollSrc->process(pApp, pPollSrc);
				}
			}
			if (pApp->destroyRequested) break;
			if (pLoop) {
				pLoop(pLoopCtx);
			}
			++GLG.mFrameCnt;
		}
#elif defined(OGLSYS_X11)
		XEvent evt;
		bool done = false;
		while (!done) {
			int kcode = 0;
			KeySym ksym;
			KeySym* pKeySym = nullptr;
			Atom* pAtom = nullptr;
			int cnt = 0;
			bool* pKbdState = nullptr;
			int kbdIdx = -1;
			OGLSysMouseState::BTN btn;
			while (XPending(GLG.mpXDisplay)) {
				XNextEvent(GLG.mpXDisplay, &evt);
				GLG.mMouseState.mWheel = xbtn_wheel(evt);
				switch (evt.type) {
					case ButtonPress:
						if (xbtn_xlat(evt, &btn)) {
							send_mouse_down(btn, float(evt.xbutton.x), float(evt.xbutton.y), true);
						}
						break;
					case ButtonRelease:
						if (xbtn_xlat(evt, &btn)) {
							send_mouse_up(btn, float(evt.xbutton.x), float(evt.xbutton.y), true);
						}
						break;
					case MotionNotify:
						send_mouse_move(float(evt.xbutton.x), float(evt.xbutton.y));
						break;
					case KeyPress:
					case KeyRelease:
						pKeySym = XGetKeyboardMapping(GLG.mpXDisplay, evt.xkey.keycode, 1, &cnt);
						ksym = *pKeySym;
						XFree(pKeySym);
						kcode = ksym & 0xFF;
						if (kcode == 0x1B) { /* ESC */
							done = true;
						}
						pKbdState = nullptr;
						kbdIdx = -1;
						if (kcode >= 'a' && kcode <= 'z') {
							pKbdState = GLG.mKbdState.alpha;
							kbdIdx = kcode - 'a';
						} else if (kcode >= 190 && kcode <= 201) {
							pKbdState = GLG.mKbdState.func;
							kbdIdx = kcode - 190;
						} else {
							switch (kcode) {
								case 8:
									pKbdState = GLG.mKbdState.ctrl;
									kbdIdx = KBD_CTRL_BACK;
									break;
								case 9:
									pKbdState = GLG.mKbdState.ctrl;
									kbdIdx = KBD_CTRL_TAB;
									break;
								case 81:
									pKbdState = GLG.mKbdState.ctrl;
									kbdIdx = KBD_CTRL_LEFT;
									break;
								case 82:
									pKbdState = GLG.mKbdState.ctrl;
									kbdIdx = KBD_CTRL_UP;
									break;
								case 83:
									pKbdState = GLG.mKbdState.ctrl;
									kbdIdx = KBD_CTRL_RIGHT;
									break;
								case 84:
									pKbdState = GLG.mKbdState.ctrl;
									kbdIdx = KBD_CTRL_DOWN;
									break;
								case 225:
									pKbdState = GLG.mKbdState.ctrl;
									kbdIdx = KBD_CTRL_LSHIFT;
									break;
								case 226:
									pKbdState = GLG.mKbdState.ctrl;
									kbdIdx = KBD_CTRL_RSHIFT;
									break;
								case 227:
									pKbdState = GLG.mKbdState.ctrl;
									kbdIdx = KBD_CTRL_LCTRL;
									break;
								case 228:
									pKbdState = GLG.mKbdState.ctrl;
									kbdIdx = KBD_CTRL_RCTRL;
									break;
								case 13:
									pKbdState = GLG.mKbdState.ctrl;
									kbdIdx = KBD_CTRL_ENTER;
									break;

								case 32:
									pKbdState = GLG.mKbdState.punct;
									kbdIdx = KBD_PUNCT_SPACE;
									break;

								default:
									break;
							}
						}
						if (pKbdState && kbdIdx >= 0) {
							pKbdState[kbdIdx] = (evt.type == KeyPress);
						}
						break;
					case ClientMessage:
						if (XGetWMProtocols(GLG.mpXDisplay, GLG.mXWnd, &pAtom, &cnt)) {
							if ((Atom)evt.xclient.data.l[0] == *pAtom) {
								done = true;
							}
						}
						break;
				}
			}
			if (pLoop) {
				pLoop(pLoopCtx);
			}
			++GLG.mFrameCnt;
		}
#elif defined(OGLSYS_WEB)
		GLG.mpWebLoop = pLoop;
		GLG.mpWebLoopCtx = pLoopCtx;
		emscripten_set_main_loop(web_loop, 0, 1);
#else
		bool done = false;
		while (!done) {
#			if defined(OGLSYS_LINUX_INPUT)
			oglsys_linux_input();
#			endif
			if (GLG.mKbdState.func[3]) { /* [F4] */
				done = true;
			}
#if DUMMYGL_WINKBD
			if (::GetAsyncKeyState(VK_ESCAPE) & 1) {
				done = true;
			}
#endif
			if (pLoop) {
				pLoop(pLoopCtx);
			}
			++GLG.mFrameCnt;
		}
#endif
	}

	bool valid() {
		return GLG.valid_ogl();
	}

	void flush() {
		if (GLG.mpfnGlFlush) {
			GLG.mpfnGlFlush();
		}
	}

	void finish() {
		if (GLG.mpfnGlFinish) {
			GLG.mpfnGlFinish();
		}
	}

	bool is_es() {
		return (OGLSYS_ES != 0);
	}

	bool is_web() {
#if defined(OGLSYS_WEB)
		return true;
#else
		return false;
#endif
	}

	bool is_dummy() {
#if defined(OGLSYS_DUMMY)
		return true;
#else
		return false;
#endif
	}

	void* get_window() {
#if defined(OGLSYS_ANDROID)
		return GLG.mpNativeWnd;
#elif defined(OGLSYS_WINDOWS)
		return GLG.mhWnd;
#elif defined(OGLSYS_X11)
		return (void*)GLG.mXWnd;
#else
		return nullptr;
#endif
	}

	void* get_display() {
#if defined(OGLSYS_X11)
		return GLG.mpXDisplay;
#else
		return nullptr;
#endif
	}

	void* get_instance() {
#if defined(OGLSYS_WINDOWS)
		return GLG.mhInstance;
#else
		return nullptr;
#endif
	}

	void* mem_alloc(size_t size, const char* pTag) {
		return GLG.mem_alloc(size, pTag);
	}

	void mem_free(void* p) {
		if (p) {
			GLG.mem_free(p);
		}
	}

	void set_dummygl_swap_func(void (*swap_func)()) {
#if defined(OGLSYS_DUMMY)
		GLG.mDummyGL.swap_func = swap_func;
#endif
	}

	void set_swap_interval(const int ival) {
		GLG.mSwapInterval = ival;
#if OGLSYS_ES
		eglSwapInterval(GLG.mEGL.display, ival);
#elif defined(OGLSYS_WINDOWS)
		GLG.mWGL.set_swap_interval(ival);
#elif defined(OGLSYS_X11)
		GLG.mGLX.set_swap_interval(GLG.mpXDisplay, GLG.mXWnd, ival);
#endif
	}

	void bind_def_framebuf() {
		if (valid()) {
			glBindFramebuffer(GL_FRAMEBUFFER, GLG.mDefFBO);
		}
	}

	int get_width() {
		return GLG.mWidth;
	}

	int get_height() {
		return GLG.mHeight;
	}

	uint64_t get_frame_count() {
		return GLG.mFrameCnt;
	}

	void* get_proc_addr(const char* pName) {
		void* pAddr = nullptr;
		if (pName) {
#if OGLSYS_ES
			pAddr = (void*)eglGetProcAddress(pName);
#else
#	if defined(OGLSYS_WINDOWS)
			pAddr = GLG.get_func_ptr(pName);
#	elif defined(OGLSYS_X11)
			pAddr = GLG.mGLX.get_func_ptr(pName);
#	endif
#endif
		}
		return pAddr;
	}

	GLuint compile_shader_str(const char* pSrc, size_t srcSize, GLenum kind) {
		GLuint sid = 0;
		if (valid() && pSrc && srcSize > 0) {
			sid = glCreateShader(kind);
			if (sid) {
				GLint len[1] = { (GLint)srcSize };
				glShaderSource(sid, 1, (const GLchar* const*)&pSrc, len);
				glCompileShader(sid);
				GLint status = 0;
				glGetShaderiv(sid, GL_COMPILE_STATUS, &status);
				if (!status) {
					GLint infoLen = 0;
					glGetShaderiv(sid, GL_INFO_LOG_LENGTH, &infoLen);
					if (infoLen > 0) {
						char* pInfo = (char*)GLG.mem_alloc(infoLen, "OGLSys:ShaderInfo");
						if (pInfo) {
							glGetShaderInfoLog(sid, infoLen, &infoLen, pInfo);
							glg_dbg_info(pInfo, infoLen);
							GLG.mem_free(pInfo);
							pInfo = nullptr;
						}
					}
					glDeleteShader(sid);
					sid = 0;
				}
			}
		}
		return sid;
	}

	GLuint compile_shader_file(const char* pSrcPath, GLenum kind) {
		GLuint sid = 0;
		if (valid()) {
			size_t size = 0;
			const char* pSrc = GLG.load_glsl(pSrcPath, &size);
			sid = compile_shader_str(pSrc, size, kind);
			GLG.unload_glsl(pSrc);
			pSrc = nullptr;
		}
		return sid;
	}

	GLuint link_draw_prog(GLuint sidVert, GLuint sidFrag) {
		GLuint pid = 0;
		if (sidVert && sidFrag) {
			GLuint sids[] = { sidVert, sidFrag };
			pid = link_prog(sids, 2);
		}
		return pid;
	}

	GLuint link_prog(const GLuint* pSIDs, const int nSIDs) {
		GLuint pid = 0;
		if (pSIDs && nSIDs > 0) {
			pid = glCreateProgram();
			if (pid) {
				for (int i = 0; i < nSIDs; ++i) {
					GLuint sid = pSIDs[i];
					if (sid) {
						glAttachShader(pid, sid);
					}
				}
				bool linkRes = link_prog_id(pid);
				if (!linkRes) {
					for (int i = 0; i < nSIDs; ++i) {
						GLuint sid = pSIDs[i];
						if (sid) {
							glDetachShader(pid, sid);
						}
					}
					glDeleteProgram(pid);
					pid = 0;
				}
			}
		}
		return pid;
	}

	void link_prog_id_nock(const GLuint pid) {
		if (pid) {
#if !OGLSYS_ES && !defined(OGLSYS_WEB) && !defined(OGLSYS_APPLE)
			if (glProgramParameteri) {
				glProgramParameteri(pid, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
			}
#endif
			glLinkProgram(pid);
		}
	}

	bool ck_link_status(const GLuint pid) {
		bool res = false;
		if (pid) {
			GLint status = 0;
			glGetProgramiv(pid, GL_LINK_STATUS, &status);
			if (status) {
				res = true;
			} else {
				GLint infoLen = 0;
				glGetProgramiv(pid, GL_INFO_LOG_LENGTH, &infoLen);
				if (infoLen > 0) {
					char* pInfo = (char*)GLG.mem_alloc(infoLen, "OGLSys:ProgInfo");
					if (pInfo) {
						glGetProgramInfoLog(pid, infoLen, &infoLen, pInfo);
						const int infoBlkSize = 512;
						char infoBlk[infoBlkSize + 1];
						infoBlk[infoBlkSize] = 0;
						int nblk = infoLen / infoBlkSize;
						for (int i = 0; i < nblk; ++i) {
							oglsys_mem_cpy(infoBlk, &pInfo[infoBlkSize * i], infoBlkSize);
							GLG.dbg_msg("%s", infoBlk);
						}
						int endSize = infoLen % infoBlkSize;
						if (endSize) {
							oglsys_mem_cpy(infoBlk, &pInfo[infoBlkSize * nblk], endSize);
							infoBlk[endSize] = 0;
							GLG.dbg_msg("%s", infoBlk);
						}
						GLG.mem_free(pInfo);
						pInfo = nullptr;
					}
				}
			}
		}
		return res;
	}

	bool link_prog_id(const GLuint pid) {
		bool res = false;
		if (pid) {
			link_prog_id_nock(pid);
			res = ck_link_status(pid);
		}
		return res;
	}

	void* get_prog_bin(GLuint pid, size_t* pSize, GLenum* pFmt) {
		void* pBin = nullptr;
		GLenum fmt = 0;
		size_t size = 0;
		if (pid != 0) {
#if OGLSYS_ES
			if (GLG.mExts.progBin && GLG.mExts.pfnGetProgramBinary) {
				GLsizei len = 0;
				glGetProgramiv(pid, GL_PROGRAM_BINARY_LENGTH_OES, &len);
				if (len > 0) {
					size = (size_t)len;
					pBin = GLG.mem_alloc(size, "OGLSys:ProgBin");
					if (pBin) {
						GLG.mExts.pfnGetProgramBinary(pid, len, &len, &fmt, pBin);
					} else {
						size = 0;
					}
				}
			}
#elif defined(OGLSYS_APPLE)
			// TODO
#elif defined(OGLSYS_WEB)
			/* no-op */
#else
			if (glGetProgramBinary) {
				GLsizei len = 0;
				glGetProgramiv(pid, GL_PROGRAM_BINARY_LENGTH, &len);
				if (len > 0) {
					size = (size_t)len;
					pBin = GLG.mem_alloc(size, "OGLSys:ProgBin");
					if (pBin) {
						glGetProgramBinary(pid, len, &len, &fmt, pBin);
					} else {
						size = 0;
					}
				}
			}
#endif
		}
		if (pSize) {
			*pSize = size;
		}
		if (pFmt) {
			*pFmt = fmt;
		}
		return pBin;
	}

	void free_prog_bin(void* pBin) {
		if (pBin) {
			GLG.mem_free(pBin);
		}
	}

	bool set_prog_bin(GLuint pid, GLenum fmt, const void* pBin, GLsizei len) {
		bool res = false;
		if (pid != 0 && pBin && len > 0) {
#if OGLSYS_ES
			if (GLG.mExts.progBin && GLG.mExts.pfnProgramBinary) {
				GLG.mExts.pfnProgramBinary(pid, fmt, pBin, len);
				GLint status = 0;
				glGetProgramiv(pid, GL_LINK_STATUS, &status);
				res = status != GL_FALSE;
			}
#elif defined(OGLSYS_APPLE)
			// TODO
#elif defined(OGLSYS_WEB)
			/* no-op */
#else
			if (glProgramBinary) {
				glProgramBinary(pid, fmt, pBin, len);
				GLint status = 0;
				glGetProgramiv(pid, GL_LINK_STATUS, &status);
				res = status != GL_FALSE;
			}
#endif
		}
		return res;
	}

	void enable_msaa(const bool flg) {
		if (!GLG.valid_ogl()) return;
#if !OGLSYS_ES && !defined(OGLSYS_WEB)
		if (flg) {
			glEnable(GL_MULTISAMPLE);
		} else {
			glDisable(GL_MULTISAMPLE);
		}
#endif
	}

	GLuint create_timestamp() {
		GLuint handle = 0;
#if OGLSYS_ES
		if (0) {
			if (GLG.mExts.disjointTimer && GLG.mExts.pfnGenQueries) {
				GLG.mExts.pfnGenQueries(1, &handle);
			}
		}
#elif defined(OGLSYS_APPLE)
		//glGenQueries(1, &handle);
#elif defined(OGLSYS_WEB)
		/* no-op */
#else
		if (glGenQueries) {
			glGenQueries(1, &handle);
		}
#endif
		return handle;
	}

	void delete_timestamp(GLuint handle) {
#if OGLSYS_ES
		if (handle != 0 && GLG.mExts.disjointTimer && GLG.mExts.pfnDeleteQueries) {
			GLG.mExts.pfnDeleteQueries(1, &handle);
		}
#elif defined(OGLSYS_APPLE)
		if (handle != 0) {
			glDeleteQueries(1, &handle);
		}
#elif defined(OGLSYS_WEB)
		/* no-op */
#else
		if (handle != 0 && glDeleteQueries) {
			glDeleteQueries(1, &handle);
		}
#endif
	}

	void put_timestamp(GLuint handle) {
#if OGLSYS_ES
		if (handle != 0 && GLG.mExts.disjointTimer && GLG.mExts.pfnQueryCounter) {
			GLG.mExts.pfnQueryCounter(handle, GL_TIMESTAMP_EXT);
		}
#elif defined(OGLSYS_APPLE)
		if (handle != 0) {
			glQueryCounter(handle, GL_TIMESTAMP);
		}
#elif defined(OGLSYS_WEB)
		/* no-op */
#else
		if (handle != 0 && glQueryCounter) {
			glQueryCounter(handle, GL_TIMESTAMP);
		}
#endif
	}

	GLuint64 get_timestamp(GLuint handle) {
		GLuint64 val = 0;
#if OGLSYS_ES
		if (handle != 0 && GLG.mExts.disjointTimer) {
			GLint err = 0;
			glGetIntegerv(GL_GPU_DISJOINT_EXT, &err);
			if (!err) {
				if (GLG.mExts.pfnGetQueryObjectui64v) {
					GLG.mExts.pfnGetQueryObjectui64v(handle, GL_QUERY_RESULT, &val);
				} else if (GLG.mExts.pfnGetQueryObjectuiv) {
					GLuint tval = 0;
					GLG.mExts.pfnGetQueryObjectuiv(handle, GL_QUERY_RESULT, &tval);
					val = tval;
				}
			}
		}
#elif defined(OGLSYS_APPLE)
		if (handle != 0) {
			glGetQueryObjectui64v(handle, GL_QUERY_RESULT, &val);
		}
#elif defined(OGLSYS_WEB)
		/* no-op */
#else
		if (handle != 0) {
			if (glGetQueryObjectui64v) {
				glGetQueryObjectui64v(handle, GL_QUERY_RESULT, &val);
			} else if (glGetQueryObjectuiv) {
				GLuint tval = 0;
				glGetQueryObjectuiv(handle, GL_QUERY_RESULT, &tval);
				val = tval;
			}
		}
#endif
		return val;
	}

	void wait_timestamp(GLuint handle) {
#if OGLSYS_ES
		if (handle != 0 && GLG.mExts.disjointTimer && GLG.mExts.pfnGetQueryObjectiv) {
			GLint flg = 0;
			while (!flg) {
				GLG.mExts.pfnGetQueryObjectiv(handle, GL_QUERY_RESULT_AVAILABLE, &flg);
			}
		}
#elif defined(OGLSYS_APPLE)
		if (handle != 0) {
			GLint flg = 0;
			while (!flg) {
				glGetQueryObjectiv(handle, GL_QUERY_RESULT_AVAILABLE, &flg);
			}
		}
#elif defined(OGLSYS_WEB)
		/* no-op */
#else
		if (handle != 0) {
			if (glGetQueryObjectui64v && glGetQueryObjectiv) {
				GLint flg = 0;
				while (!flg) {
					glGetQueryObjectiv(handle, GL_QUERY_RESULT_AVAILABLE, &flg);
				}
			}
		}
#endif
	}

	GLuint gen_vao() {
		GLuint vao = 0;
#if OGLSYS_ES
		if (GLG.mExts.pfnGenVertexArrays != nullptr) {
			GLG.mExts.pfnGenVertexArrays(1, &vao);
		}
#elif defined(OGLSYS_APPLE)
		/* no-op */
#elif defined(OGLSYS_WEB)
		/* no-op */
#else
		if (glGenVertexArrays != nullptr) {
			glGenVertexArrays(1, &vao);
		}
#endif
		return vao;
	}

	void bind_vao(const GLuint vao) {
#if OGLSYS_ES
		if (GLG.mExts.pfnBindVertexArray != nullptr) {
			GLG.mExts.pfnBindVertexArray(vao);
		}
#elif defined(OGLSYS_APPLE)
		/* no-op */
#elif defined(OGLSYS_WEB)
		/* no-op */
#else
		if (glBindVertexArray != nullptr) {
			glBindVertexArray(vao);
		}
#endif
	}

	void del_vao(const GLuint vao) {
#if OGLSYS_ES
		if (GLG.mExts.pfnDeleteVertexArrays != nullptr) {
			GLG.mExts.pfnDeleteVertexArrays(1, &vao);
		}
#elif defined(OGLSYS_APPLE)
		/* no-op */
#elif defined(OGLSYS_WEB)
		/* no-op */
#else
		if (glDeleteVertexArrays != nullptr) {
			glDeleteVertexArrays(1, &vao);
		}
#endif
	}


	void draw_tris_base_vtx(const int ntris, const GLenum idxType, const intptr_t ibOrg, const int baseVtx) {
#if OGLSYS_ES
		#if !defined(OGLSYS_VIVANTE_FB)
		if (GLG.mExts.pfnDrawElementsBaseVertex != nullptr) {
			GLG.mExts.pfnDrawElementsBaseVertex(GL_TRIANGLES, ntris * 3, idxType, (const void*)ibOrg, baseVtx);
		}
		#endif
#elif defined(OGLSYS_APPLE)
		/* no-op */
#elif defined(OGLSYS_WEB)
		/* no-op */
#else
		if (glDrawElementsBaseVertex != nullptr) {
			glDrawElementsBaseVertex(GL_TRIANGLES, ntris * 3, idxType, (const void*)ibOrg, baseVtx);
		}
#endif
	}


	GLuint get_black_tex() {
		if (s_initFlg && !GLG.mWithoutCtx && GLG.valid_ogl()) {
			create_const_tex(&GLG.mDefTexs.black, 0);
		}
		return GLG.mDefTexs.black;
	}

	GLuint get_white_tex() {
		if (s_initFlg && !GLG.mWithoutCtx && GLG.valid_ogl()) {
			create_const_tex(&GLG.mDefTexs.white, 0xFFFFFFFFU);
		}
		return GLG.mDefTexs.white;
	}

	void set_tex2d_lod_bias(const int bias) {
#if !OGLSYS_ES && !defined(OGLSYS_WEB)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, bias);
#endif
	}

	GLint get_max_vtx_texs() {
		GLint num = 0;
		glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &num);
		return num;
	}


	bool ext_ck_bindless_texs() {
		return GLG.mExts.bindlessTex;
	}

	bool ext_ck_derivatives() {
#if OGLSYS_ES
		return GLG.mExts.derivs;
#else
		return true;
#endif
	}

	bool ext_ck_vtx_half() {
#if OGLSYS_ES
		return GLG.mExts.vtxHalf;
#elif defined(OGLSYS_WEB)
		return false;
#else
		return true;
#endif
	}

	bool ext_ck_idx_uint() {
#if OGLSYS_ES
		return GLG.mExts.idxUInt;
#elif defined(OGLSYS_WEB)
		return false;
#else
		return true;
#endif
	}

	bool ext_ck_ASTC_LDR() {
		return GLG.mExts.ASTC_LDR;
	}

	bool ext_ck_S3TC() {
		return GLG.mExts.S3TC;
	}

	bool ext_ck_DXT1() {
		return GLG.mExts.DXT1;
	}

	bool ext_ck_prog_bin() {
#if OGLSYS_ES
		return GLG.mExts.progBin;
#elif defined(OGLSYS_APPLE)
		return GLG.mExts.progBin;
#elif defined(OGLSYS_WEB)
		return false;
#else
		return glGetProgramBinary != nullptr;
#endif
	}

	bool ext_ck_shader_bin() {
#if OGLSYS_ES
		return false;
#elif defined(OGLSYS_APPLE)
		return false;
#elif defined(OGLSYS_WEB)
		return false;
#else
		return glShaderBinary != nullptr;
#endif
	}

	bool ext_ck_spv() {
		bool res = false;
		if (ext_ck_shader_bin()) {
#if OGLSYS_ES
#elif defined(OGLSYS_APPLE)
#elif defined(OGLSYS_WEB)
#else
			GLint nfmt = 0;
			glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &nfmt);
			if (nfmt > 0) {
				GLint fmtLst[16];
				GLint* pLst = fmtLst;
				if ((size_t)nfmt > OGLSYS_ARY_LEN(fmtLst)) {
					pLst = (GLint*)GLG.mem_alloc(nfmt * sizeof(GLint), "OGLSys:BinFmts");
				}
				if (pLst) {
					glGetIntegerv(GL_SHADER_BINARY_FORMATS, pLst);
					for (int i = 0; i < nfmt; ++i) {
						if (pLst[i] == GL_SHADER_BINARY_FORMAT_SPIR_V_ARB) {
							res = true;
							break;
						}
					}
				}
				if (pLst != fmtLst) {
					GLG.mem_free(pLst);
					pLst = nullptr;
				}
			}
#endif
		}
		return res;
	}

	bool ext_ck_vao() {
		bool res = false;
#if OGLSYS_ES
		if (GLG.mExts.pfnGenVertexArrays != nullptr) {
			res = true;
		}
#elif defined(OGLSYS_APPLE)
#elif defined(OGLSYS_WEB)
#else
		if (glGenVertexArrays != nullptr) {
			res = true;
		}
#endif
		return res;
	}

	bool ext_ck_vtx_base() {
		bool res = false;
#if OGLSYS_ES
		if (GLG.mExts.pfnDrawElementsBaseVertex != nullptr) {
			res = true;
		}
#elif defined(OGLSYS_APPLE)
#elif defined(OGLSYS_WEB)
#else
		if (glDrawElementsBaseVertex != nullptr) {
			res = true;
		}
#endif
		return res;
	}

	bool ext_ck_mdi() {
		return GLG.mExts.mdi;
	}

	bool ext_ck_nv_vbum() {
		return GLG.mExts.nvVBUM;
	}

	bool ext_ck_nv_ubum() {
		return GLG.mExts.nvUBUM;
	}

	bool ext_ck_nv_bindless_mdi() {
		return GLG.mExts.nvBindlessMDI;
	}

	bool ext_ck_nv_cmd_list() {
		return GLG.mExts.nvCmdLst;
	}


	bool get_key_state(const char code) {
		bool* pState = nullptr;
		int idx = -1;
		if (code >= '0' && code <= '9') {
			pState = GLG.mKbdState.digit;
			idx = code - '0';
		} else if (code >= 'A' && code <= 'Z') {
			pState = GLG.mKbdState.alpha;
			idx = code - 'A';
		} else if (code >= 'a' && code <= 'z') {
			pState = GLG.mKbdState.alpha;
			idx = code - 'a';
		} else {
			const int npunct = OGLSYS_ARY_LEN(s_kbdPunctTbl);
			for (int i = 0; i < npunct; ++i) {
				if (s_kbdPunctTbl[i].code == code) {
					pState = GLG.mKbdState.punct;
					idx = s_kbdPunctTbl[i].idx;
				}
			}
		}
		return (pState && idx >= 0) ? pState[idx] : false;
	}

	void xlat_key_name(const char* pName, const size_t nameLen, bool** ppState, int* pIdx) {
		bool* pState = nullptr;
		int idx = -1;
		if (pName[0] == 'F') {
			if (nameLen == 2) {
				int code = pName[1];
				if ((code >= '1' && code <= '9')) {
					pState = GLG.mKbdState.func;
					idx = code - '1';
				}
			} else if (nameLen == 3) {
				int code = pName[2];
				if (pName[1] == '1' && (code >= '0' && code <= '2')) {
					pState = GLG.mKbdState.func;
					idx = (code - '0') + 9;
				}
			}
		}
		if (!pState) {
			if (oglsys_str_eq(pName, "UP")) {
				pState = GLG.mKbdState.ctrl;
				idx = KBD_CTRL_UP;
			} else if (oglsys_str_eq(pName, "DOWN")) {
				pState = GLG.mKbdState.ctrl;
				idx = KBD_CTRL_DOWN;
			} else if (oglsys_str_eq(pName, "LEFT")) {
				pState = GLG.mKbdState.ctrl;
				idx = KBD_CTRL_LEFT;
			} else if (oglsys_str_eq(pName, "RIGHT")) {
				pState = GLG.mKbdState.ctrl;
				idx = KBD_CTRL_RIGHT;
			} else if (oglsys_str_eq(pName, "TAB")) {
				pState = GLG.mKbdState.ctrl;
				idx = KBD_CTRL_TAB;
			} else if (oglsys_str_eq(pName, "BACK")) {
				pState = GLG.mKbdState.ctrl;
				idx = KBD_CTRL_BACK;
			} else if (oglsys_str_eq(pName, "LSHIFT")) {
				pState = GLG.mKbdState.ctrl;
				idx = KBD_CTRL_LSHIFT;
			} else if (oglsys_str_eq(pName, "LCTRL")) {
				pState = GLG.mKbdState.ctrl;
				idx = KBD_CTRL_LCTRL;
			} else if (oglsys_str_eq(pName, "RSHIFT")) {
				pState = GLG.mKbdState.ctrl;
				idx = KBD_CTRL_RSHIFT;
			} else if (oglsys_str_eq(pName, "RCTRL")) {
				pState = GLG.mKbdState.ctrl;
				idx = KBD_CTRL_RCTRL;
			} else if (oglsys_str_eq(pName, "ENTER")) {
				pState = GLG.mKbdState.ctrl;
				idx = KBD_CTRL_ENTER;
			}
		}
		if (ppState) {
			*ppState = pState;
		}
		if (pIdx) {
			*pIdx = idx;
		}
	}

	bool get_key_state(const char* pName) {
		bool res = false;
		size_t nameLen = oglsys_str_len(pName);
		if (nameLen == 1) {
			res = get_key_state(pName[0]);
		} else if (nameLen > 0) {
			bool* pState = nullptr;
			int idx = -1;
			xlat_key_name(pName, nameLen, &pState, &idx);
			if (pState && idx >= 0) {
				res = pState[idx];
			}
		}
		return res;
	}

	void set_key_state(const char* pName, const bool state) {
		bool* pState = nullptr;
		int idx = -1;
		size_t nameLen = oglsys_str_len(pName);
		if (nameLen > 0) {
			if (nameLen == 1) {
				int code = int(pName[0]);
				if (code >= '0' && code <= '9') {
					pState = GLG.mKbdState.digit;
					idx = code - '0';
				} else if (code >= 'A' && code <= 'Z') {
					pState = GLG.mKbdState.alpha;
					idx = code - 'A';
				} else if (code >= 'a' && code <= 'z') {
					pState = GLG.mKbdState.alpha;
					idx = code - 'a';
				} else {
					const int npunct = OGLSYS_ARY_LEN(s_kbdPunctTbl);
					for (int i = 0; i < npunct; ++i) {
						if (s_kbdPunctTbl[i].code == code) {
							pState = GLG.mKbdState.punct;
							idx = s_kbdPunctTbl[i].idx;
						}
					}
				}
			} else {
				xlat_key_name(pName, nameLen, &pState, &idx);
			}
		}
		if (pState && idx >= 0) {
			pState[idx] = state;
		}
	}

	void set_input_handler(InputHandler handler, void* pWk) {
		GLG.mInpHandler = handler;
		GLG.mpInpHandlerWk = pWk;
	}

	OGLSysMouseState get_mouse_state() {
		return GLG.mMouseState;
	}

	void set_inp_btn_id(OGLSysInput* pInp, OGLSysMouseState::BTN btn) {
		switch (btn) {
			case OGLSysMouseState::OGLSYS_BTN_LEFT:
				pInp->id = 0;
				break;
			case OGLSysMouseState::OGLSYS_BTN_MIDDLE:
				pInp->id = 1;
				break;
			case OGLSysMouseState::OGLSYS_BTN_RIGHT:
				pInp->id = 2;
				break;
			case OGLSysMouseState::OGLSYS_BTN_X:
				pInp->id = 3;
				break;
			case OGLSysMouseState::OGLSYS_BTN_Y:
				pInp->id = 4;
				break;
			default:
				pInp->id = -1;
				break;
		}
	}

	void send_mouse_down(OGLSysMouseState::BTN btn, float absX, float absY, bool updateFlg) {
		uint32_t mask = 1U << btn;
		GLG.mMouseState.mBtnOld = GLG.mMouseState.mBtnNow;
		GLG.mMouseState.mBtnNow &= ~mask;
		GLG.mMouseState.mBtnNow |= mask;
		OGLSysInput inp;
		inp.device = OGLSysInput::OGLSYS_MOUSE;
		inp.sysDevId = 0;
		inp.pressure = 1.0f;
		inp.absX = absX;
		inp.absY = absY;
		inp.act = OGLSysInput::OGLSYS_ACT_NONE;
		set_inp_btn_id(&inp, btn);
		if (inp.id != -1) {
			inp.act = OGLSysInput::OGLSYS_ACT_DOWN;
		}
		if (updateFlg) {
			GLG.update_mouse_pos(absX, absY);
		} else {
			GLG.set_mouse_pos(absX, absY);
		}
		GLG.send_input(&inp);
	}

	void send_mouse_up(OGLSysMouseState::BTN btn, float absX, float absY, bool updateFlg) {
		uint32_t mask = 1U << btn;
		GLG.mMouseState.mBtnOld = GLG.mMouseState.mBtnNow;
		GLG.mMouseState.mBtnNow &= ~mask;
		OGLSysInput inp;
		inp.device = OGLSysInput::OGLSYS_MOUSE;
		inp.sysDevId = 0;
		inp.pressure = 1.0f;
		inp.absX = absX;
		inp.absY = absY;
		inp.act = OGLSysInput::OGLSYS_ACT_NONE;
		set_inp_btn_id(&inp, btn);
		if (inp.id != -1) {
			inp.act = OGLSysInput::OGLSYS_ACT_UP;
		}
		if (updateFlg) {
			GLG.update_mouse_pos(absX, absY);
		} else {
			GLG.set_mouse_pos(absX, absY);
		}
		GLG.send_input(&inp);
	}

	void send_mouse_move(float absX, float absY) {
		OGLSysInput inp;
		inp.device = OGLSysInput::OGLSYS_MOUSE;
		inp.sysDevId = 0;
		inp.pressure = 1.0f;
		inp.absX = absX;
		inp.absY = absY;
		inp.id = -1;
		if (GLG.mMouseState.ck_now(OGLSysMouseState::OGLSYS_BTN_LEFT)) {
			inp.id = 0;
			inp.act = OGLSysInput::OGLSYS_ACT_DRAG;
		} else {
			inp.act = OGLSysInput::OGLSYS_ACT_HOVER;
		}
		GLG.update_mouse_pos(absX, absY);
		GLG.send_input(&inp);
	}


#if defined(OGLSYS_DUMMY)
	namespace dummygl {

		GLuint s_ibuf = 0;
		GLuint s_ismp = 0;
		GLuint s_itex = 0;
		GLuint s_isdr = 0;
		GLuint s_iprg = 0;
		GLuint s_ifbo = 0;
		GLuint s_irbo = 0;

		void APIENTRY Viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
		}

		void APIENTRY Scissor(GLint x, GLint y, GLsizei width, GLsizei height) {
		}

		void APIENTRY GetIntegerv(GLenum name, GLint* pData) {
			if (!pData) return;
			switch (name) {
				case GL_FRAMEBUFFER_BINDING:
					*pData = 0;
					break;
				case GL_MAX_TEXTURE_SIZE:
					*pData = 4096;
					break;
				default:
					*pData = 0;
					break;
			}
		}

		const GLubyte* APIENTRY GetString(GLenum name) {
			if (name == GL_VERSION) {
				return (GLubyte*)"dummygl 0.1";
			} else if (name == GL_VENDOR) {
				return (GLubyte*)"OGLSys labs";
			} else if (name == GL_RENDERER) {
				return (GLubyte*)"blackhole renderer";
			} else if (name == GL_EXTENSIONS) {
				return (GLubyte*)"";
			}
			return (GLubyte*)"";
		}

		void APIENTRY LineWidth(GLfloat width) {
		}

		void APIENTRY ColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
		}

		void APIENTRY DepthMask(GLboolean flag) {
		}

		void APIENTRY DepthFunc(GLenum func) {
		}

		void APIENTRY StencilMask(GLuint mask) {
		}

		void APIENTRY StencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
		}

		void APIENTRY StencilFunc(GLenum func, GLint ref, GLuint mask) {
		}

		void APIENTRY StencilMaskSeparate(GLenum face, GLuint mask) {
		}

		void APIENTRY StencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
		}

		void APIENTRY StencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {
		}

		void APIENTRY ClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
		}

		void APIENTRY ClearDepth(GLdouble depth) {
		}

		void APIENTRY ClearStencil(GLint s) {
		}

		void APIENTRY Clear(GLbitfield mask) {
		}

		void APIENTRY Enable(GLenum cap) {
		}

		void APIENTRY Disable(GLenum cap) {
		}

		void APIENTRY FrontFace(GLenum mode) {
		}

		void APIENTRY CullFace(GLenum mode) {
		}

		void APIENTRY BlendEquation(GLenum mode) {
		}

		void APIENTRY BlendFunc(GLenum sfactor, GLenum dfactor) {
		}

		void APIENTRY BlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
		}

		void APIENTRY BlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
		}

		void APIENTRY GenBuffers(GLsizei n, GLuint* pBuffers) {
			if (n > 0 && pBuffers) {
				for (GLsizei i = 0; i < n; ++i) {
					GLuint ibuf = s_ibuf++;
					if (ibuf == 0) ibuf = s_ibuf++;
					pBuffers[i] = ibuf;
				}
			}
		}

		void APIENTRY BindBuffer(GLenum target, GLuint buffer) {
		}

		void APIENTRY BufferData(GLenum target, GLsizeiptr size, const void* pData, GLenum usage) {
		}

		void APIENTRY BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* pData) {
		}

		void APIENTRY BindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
		}

		void APIENTRY DeleteBuffers(GLsizei n, const GLuint* pBuffers) {
		}

		void APIENTRY GenSamplers(GLsizei n, GLuint* pSamplers) {
			if (n > 0 && pSamplers) {
				for (GLsizei i = 0; i < n; ++i) {
					GLuint ismp = s_ismp++;
					if (ismp == 0) ismp = s_ismp++;
					pSamplers[i] = ismp;
				}
			}
		}

		void APIENTRY DeleteSamplers(GLsizei count, const GLuint* pSamplers) {
		}

		void APIENTRY SamplerParameteri(GLuint sampler, GLenum pname, GLint param) {
		}

		void APIENTRY BindSampler(GLuint unit, GLuint sampler) {
		}

		void APIENTRY GenTextures(GLsizei n, GLuint* pTextures) {
			if (n > 0 && pTextures) {
				for (GLsizei i = 0; i < n; ++i) {
					GLuint itex = s_itex++;
					if (itex == 0) itex = s_itex++;
					pTextures[i] = itex;
				}
			}
		}

		void APIENTRY DeleteTextures(GLsizei n, const GLuint* pTextures) {
		}

		void APIENTRY BindTexture(GLenum target, GLuint texture) {
		}

		void APIENTRY TexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pPixels) {
		}

		void APIENTRY TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
		}

		void APIENTRY TexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
		}

		void APIENTRY TexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pPixels) {
		}

		void APIENTRY TexParameteri(GLenum target, GLenum pname, GLint param) {
		}

		void APIENTRY ActiveTexture(GLenum texture) {
		}

		void APIENTRY GenerateMipmap(GLenum target) {
		}

		void APIENTRY PixelStorei(GLenum pname, GLint param) {
		}

		GLuint APIENTRY CreateShader(GLenum type) {
			GLuint isdr = 0;
			bool typeOk = type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER;
			if (typeOk) {
				isdr = s_isdr++;
				if (isdr == 0) isdr = s_isdr++;
			}
			return isdr;
		}

		void APIENTRY ShaderSource(GLuint shader, GLsizei count, const GLchar* const* ppString, const GLint* pLength) {
		}

		void APIENTRY CompileShader(GLuint shader) {
		}

		void APIENTRY DeleteShader(GLuint shader) {
		}

		void APIENTRY GetShaderiv(GLuint shader, GLenum pname, GLint* pParams) {
			if (shader && pParams) {
				switch (pname) {
					case GL_COMPILE_STATUS:
						*pParams = GL_TRUE;
						break;
					default:
						*pParams = 0;
						break;
				}
			} else if (pParams) {
				*pParams = 0;
			}
		}

		void APIENTRY GetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* pLength, GLchar* pInfoLog) {
			if (pLength) {
				*pLength = 0;
			}
		}

		void APIENTRY AttachShader(GLuint program, GLuint shader) {
		}

		void APIENTRY DetachShader(GLuint program, GLuint shader) {
		}

		GLuint APIENTRY CreateProgram() {
			GLuint iprg = s_iprg++;
			if (iprg == 0) iprg = s_iprg++;
			return iprg;
		}

		void APIENTRY LinkProgram(GLuint program) {
		}

		void APIENTRY DeleteProgram(GLuint program) {
		}

		void APIENTRY UseProgram(GLuint program) {
		}

		void APIENTRY GetProgramiv(GLuint program, GLenum pname, GLint* pParams) {
			if (program && pParams) {
				switch (pname) {
					case GL_LINK_STATUS:
						*pParams = GL_TRUE;
						break;
					default:
						*pParams = 0;
						break;
				}
			} else if (pParams) {
				*pParams = 0;
			}
		}

		void APIENTRY GetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* pLength, GLchar* pInfoLog) {
			if (pLength) {
				*pLength = 0;
			}
		}

		GLint APIENTRY GetUniformLocation(GLuint program, const GLchar* pName) {
			GLint loc = 0;
			if (program) {
				loc = 1;
			}
			return loc;
		}

		void APIENTRY Uniform1i(GLint location, GLint v0) {
		}

		void APIENTRY Uniform1f(GLint location, GLfloat v0) {
		}

		void APIENTRY Uniform1fv(GLint location, GLsizei count, const GLfloat* pValue) {
		}

		void APIENTRY Uniform2fv(GLint location, GLsizei count, const GLfloat* pValue) {
		}

		void APIENTRY Uniform3fv(GLint location, GLsizei count, const GLfloat* pValue) {
		}

		void APIENTRY Uniform4fv(GLint location, GLsizei count, const GLfloat* pValue) {
		}

		void APIENTRY UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* pValue) {
		}

		GLint APIENTRY GetAttribLocation(GLuint program, const GLchar* name) {
			return 0;
		}

		GLuint APIENTRY GetUniformBlockIndex(GLuint program, const GLchar* pUniformBlockName) {
			GLuint idx = GL_INVALID_INDEX;
			if (program && pUniformBlockName) {
				idx = 0;
			}
			return idx;
		}

		void APIENTRY UniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
		}

		void APIENTRY VertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
		}

		void APIENTRY EnableVertexAttribArray(GLuint index) {
		}

		void APIENTRY DisableVertexAttribArray(GLuint index) {
		}

		void APIENTRY VertexAttrib4fv(GLuint index, const GLfloat* pVal) {
		}

		void APIENTRY VertexAttrib4iv(GLuint index, const GLint* pVal) {
		}

		void APIENTRY BindBufferBase(GLenum target, GLuint index, GLuint buffer) {
		}

		void APIENTRY DrawArrays(GLenum mode, GLint first, GLsizei count) {
		}

		void APIENTRY DrawElements(GLenum mode, GLsizei count, GLenum type, const void* pIndices) {
		}

		void APIENTRY GenFramebuffers(GLsizei n, GLuint* pFramebuffers) {
			if (n > 0 && pFramebuffers) {
				for (GLsizei i = 0; i < n; ++i) {
					GLuint ifbo = s_ifbo++;
					if (ifbo == 0) ifbo = s_ifbo++;
					pFramebuffers[i] = ifbo;
				}
			}
		}

		void APIENTRY DeleteFramebuffers(GLsizei n, const GLuint* pFramebuffers) {
		}

		void APIENTRY BindFramebuffer(GLenum target, GLuint framebuffer) {
		}

		void APIENTRY FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
		}

		void APIENTRY FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
		}

		GLenum APIENTRY CheckFramebufferStatus(GLenum target) {
			return GL_FRAMEBUFFER_COMPLETE;
		}

		void APIENTRY BlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
		}

		void APIENTRY GenRenderbuffers(GLsizei n, GLuint* pRenderbuffers) {
			if (n > 0 && pRenderbuffers) {
				for (GLsizei i = 0; i < n; ++i) {
					GLuint irbo = s_irbo++;
					if (irbo == 0) irbo = s_irbo++;
					pRenderbuffers[i] = irbo;
				}
			}
		}

		void APIENTRY DeleteRenderbuffers(GLsizei n, const GLuint* pRenderbuffers) {
		}

		void APIENTRY BindRenderbuffer(GLenum target, GLuint renderbuffer) {
		}

		void APIENTRY RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
		}

		GLenum APIENTRY GetError() {
			return GL_NO_ERROR;
		}

	} // dummygl

#undef OGL_FN_CORE
#define OGL_FN_CORE
#undef OGL_FN_EXTRA
#define OGL_FN(_type, _name) gl##_name = dummygl::_name;

#if defined(_MSC_VER)
	__declspec(noinline)
#elif defined(__GNUC__)
	__attribute__((noinline))
#endif
		void dummyglInit() {
#		include "oglsys.inc"
		}

#undef OGL_FN
#endif // OGLSYS_DUMMY


	namespace CL {
#if OGLSYS_CL
#		define OGLSYS_CL_FN(_name) cl_api_cl##_name _name;
#		include "oglsys.inc"
#		undef OGLSYS_CL_FN
#endif

		void init() {
#if OGLSYS_CL
			int allCnt = 0;
			int okCnt = 0;

			GLG.mpfnTIAllocDDR = nullptr;
			GLG.mpfnTIFreeDDR = nullptr;
			GLG.mpfnTIAllocMSMC = nullptr;
			GLG.mpfnTIFreeMSMC = nullptr;
#	if defined(OGLSYS_WINDOWS)
			GLG.mLibOCL = LoadLibraryW(L"OpenCL.dll");
			if (GLG.mLibOCL) {
#			define OGLSYS_CL_FN(_name) *(void**)&_name = (void*)GetProcAddress(GLG.mLibOCL, "cl" #_name); ++allCnt; if (_name) ++okCnt;
#			include "oglsys.inc"
#			undef OGLSYS_CL_FN
			}
#	else
			static const char* clLibs[] = {
				"libTIOpenCL.so",
				"libOpenCL.so",
				"libOpenCL.so.1"
			};
			GLG.mLibOCL = OGLSys::dlopen_from_list(clLibs, OGLSYS_ARY_LEN(clLibs));
			if (GLG.mLibOCL) {
#			define OGLSYS_CL_FN(_name) *(void**)&_name = (void*)::dlsym(GLG.mLibOCL, "cl" #_name); ++allCnt; if (_name) ++okCnt;
#			include "oglsys.inc"
#			undef OGLSYS_CL_FN
				*(void**)&GLG.mpfnTIAllocDDR = (void*)::dlsym(GLG.mLibOCL, "__malloc_ddr");
				*(void**)&GLG.mpfnTIFreeDDR = (void*)::dlsym(GLG.mLibOCL, "__free_ddr");
				*(void**)&GLG.mpfnTIAllocMSMC = (void*)::dlsym(GLG.mLibOCL, "__malloc_msmc");
				*(void**)&GLG.mpfnTIFreeMSMC = (void*)::dlsym(GLG.mLibOCL, "__free_msmc");
			}
#	endif
			GLG.mNumFuncsOCL = okCnt;
#endif

#if !defined(OGLSYS_WEB)
#	if OGLSYS_CL
			GLG.dbg_msg("OpenCL functions: %d/%d\n", okCnt, allCnt);
#	endif
#endif
		}

		void reset() {
#if OGLSYS_CL
#	if defined(OGLSYS_WINDOWS)
			if (GLG.mLibOCL) {
				FreeLibrary(GLG.mLibOCL);
			}
			GLG.mLibOCL = NULL;
#	else
			if (GLG.mLibOCL) {
				::dlclose(GLG.mLibOCL);
			}
			GLG.mLibOCL = NULL;
#	endif
#endif
		}

		bool valid() {
#if OGLSYS_CL
			return GLG.mLibOCL != NULL && GLG.mNumFuncsOCL > 0;
#else
			return false;
#endif
		}

		bool ck_ext(const char* pExts, const char* pExtName) {
			bool found = false;
#if OGLSYS_CL
			if (!pExts || !pExtName) return false;
			int iext = 0;
			int iwk = 0;
			bool done = false;
			size_t nameLen = oglsys_str_len(pExtName);
			while (!done) {
				char extc = pExts[iwk];
				done = extc == 0;
				if (done || extc == ' ') {
					size_t extLen = iwk - iext;
					const char* pExtStr = &pExts[iext];
					iext = iwk + 1;
					if (extLen > 1) {
						if (nameLen == extLen) {
							if (::memcmp(pExtStr, pExtName, extLen) == 0) {
								found = true;
								break;
							}
						}
					}
				}
				++iwk;
			}
#endif
			return found;
		}

		bool ck_device_ext(Device dev, const char* pExtName) {
			bool found = false;
#if OGLSYS_CL
			if (pExtName && valid() && dev && GetDeviceInfo) {
				char exts[1024];
				size_t extsSize = 0;
				cl_int res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_EXTENSIONS, 0, NULL, &extsSize);
				if (res == CL_SUCCESS) {
					char* pExts = exts;
					if (extsSize > OGLSYS_ARY_LEN(exts)) {
						pExts = (char*)OGLSys::mem_alloc(extsSize, "OGLSys:CL:tmpDevExts");
					}
					if (pExts) {
						res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_EXTENSIONS, extsSize, pExts, NULL);
						if (res == CL_SUCCESS) {
							found = ck_ext(pExts, pExtName);
						}
						if (pExts != exts) {
							OGLSys::mem_free(pExts);
						}
					}
				}
			}
#endif
			return found;
		}

		PlatformList* get_platform_list() {
			PlatformList* pLst = nullptr;
#if OGLSYS_CL
			cl_uint num = 0;
			cl_platform_id idBuf[8];
			cl_platform_id* pIds = nullptr;
			cl_int res = valid() && GetPlatformIDs ? GetPlatformIDs(0, NULL, &num) : CL_INVALID_VALUE;
			if (res == CL_SUCCESS && num > 0) {
				if (num > OGLSYS_ARY_LEN(idBuf)) {
					pIds = (cl_platform_id*)OGLSys::mem_alloc(num * sizeof(cl_platform_id), "OGLSys:CL:tmpPlatIDs");
				} else {
					pIds = idBuf;
				}
			}
			if (pIds) {
				GetPlatformIDs(num, pIds, NULL);
				size_t strSize = 0;
				for (size_t i = 0; i < (size_t)num; ++i) {
					size_t prmSize = 0;
					GetPlatformInfo(pIds[i], CL_PLATFORM_VERSION, 0, NULL, &prmSize);
					strSize += prmSize;
					GetPlatformInfo(pIds[i], CL_PLATFORM_NAME, 0, NULL, &prmSize);
					strSize += prmSize;
					GetPlatformInfo(pIds[i], CL_PLATFORM_VENDOR, 0, NULL, &prmSize);
					strSize += prmSize;
					GetPlatformInfo(pIds[i], CL_PLATFORM_EXTENSIONS, 0, NULL, &prmSize);
					strSize += prmSize;
				}
				size_t lstSize = sizeof(PlatformList) + (sizeof(PlatformList::Entry) * (num - 1));
				size_t memSize = lstSize + strSize;
				pLst = (PlatformList*)OGLSys::mem_alloc(memSize, "OGLSys:CL:PlatList");
				if (pLst) {
					oglsys_mem_set(pLst, 0, memSize);
					pLst->num = num;
					char* pStrs = (char*)pLst + lstSize;
					char profileBuf[32];
					static const char* pFullPrf = "FULL_PROFILE";
					size_t fullPrfSize = oglsys_str_len(pFullPrf) + 1;
					for (size_t i = 0; i < (size_t)num; ++i) {
						size_t prmSize = 0;
						GetPlatformInfo(pIds[i], CL_PLATFORM_PROFILE, sizeof(profileBuf), profileBuf, &prmSize);
						bool fullFlg = false;
						if (prmSize == fullPrfSize && memcmp(profileBuf, pFullPrf, fullPrfSize) == 0) {
							fullFlg = true;
						}
						pLst->entries[i].plat = pIds[i];
						pLst->entries[i].fullProfile = fullFlg;
						GetPlatformInfo(pIds[i], CL_PLATFORM_VERSION, 0, NULL, &prmSize);
						GetPlatformInfo(pIds[i], CL_PLATFORM_VERSION, prmSize, pStrs, NULL);
						pLst->entries[i].pVer = pStrs;
						pStrs += prmSize;
						GetPlatformInfo(pIds[i], CL_PLATFORM_NAME, 0, NULL, &prmSize);
						GetPlatformInfo(pIds[i], CL_PLATFORM_NAME, prmSize, pStrs, NULL);
						pLst->entries[i].pName = pStrs;
						pStrs += prmSize;
						GetPlatformInfo(pIds[i], CL_PLATFORM_VENDOR, 0, NULL, &prmSize);
						GetPlatformInfo(pIds[i], CL_PLATFORM_VENDOR, prmSize, pStrs, NULL);
						pLst->entries[i].pVendor = pStrs;
						pStrs += prmSize;
						GetPlatformInfo(pIds[i], CL_PLATFORM_EXTENSIONS, 0, NULL, &prmSize);
						GetPlatformInfo(pIds[i], CL_PLATFORM_EXTENSIONS, prmSize, pStrs, NULL);
						pLst->entries[i].pExts = pStrs;
						pStrs += prmSize;
						pLst->entries[i].numDevs = get_num_devices(pIds[i]);
						pLst->entries[i].numCPU = get_num_cpu_devices(pIds[i]);
						pLst->entries[i].numGPU = get_num_gpu_devices(pIds[i]);
						pLst->entries[i].numAcc = get_num_acc_devices(pIds[i]);
						pLst->entries[i].defDev = NULL;
						if (pLst->entries[i].numDevs > 0) {
							cl_device_id defDev;
							res = GetDeviceIDs(pIds[i], CL_DEVICE_TYPE_DEFAULT, 1, &defDev, NULL);
							if (res == CL_SUCCESS) {
								pLst->entries[i].defDev = defDev;
								if (GetDeviceInfo) {
									cl_device_type devType;
									res = GetDeviceInfo(defDev, CL_DEVICE_TYPE, sizeof(devType), &devType, NULL);
									if (res == CL_SUCCESS) {
										pLst->entries[i].coprFlg = !(devType & CL_DEVICE_TYPE_CPU);
									}
								}
							}
						}
					}
				}
			}
			if (pIds != idBuf) {
				OGLSys::mem_free(pIds);
			}
#endif
			return pLst;
		}

		void free_platform_list(PlatformList* pLst) {
			OGLSys::mem_free(pLst);
		}

		Device get_device(Platform plat, const uint32_t idx) {
			Device dev = NULL;
#if OGLSYS_CL
			uint32_t ndev = get_num_devices(plat);
			if (idx < ndev) {
				cl_device_id devs[8];
				cl_device_id* pDevs = devs;
				uint32_t nreq = idx + 1;
				if (nreq > OGLSYS_ARY_LEN(devs)) {
					pDevs = (cl_device_id*)OGLSys::mem_alloc(sizeof(cl_device_id) * nreq, "OGLSys:CL:tmpDevs");
				}
				if (pDevs) {
					cl_int res = GetDeviceIDs((cl_platform_id)plat, CL_DEVICE_TYPE_ALL, nreq, pDevs, NULL);
					if (res == CL_SUCCESS) {
						dev = (Device)pDevs[idx];
					}
					if (pDevs != devs) {
						OGLSys::mem_free(pDevs);
					}
				}
			}
#endif
			return dev;
		}

		uint32_t get_num_devices(Platform plat) {
			uint32_t num = 0;
#if OGLSYS_CL
			if (valid() && GetDeviceIDs) {
				cl_int res = GetDeviceIDs((cl_platform_id)plat, CL_DEVICE_TYPE_ALL, 0, NULL, &num);
				if (res != CL_SUCCESS) {
					num = 0;
				}
			}
#endif
			return num;
		}

		uint32_t get_num_cpu_devices(Platform plat) {
			uint32_t num = 0;
#if OGLSYS_CL
			if (valid() && GetDeviceIDs) {
				cl_int res = GetDeviceIDs((cl_platform_id)plat, CL_DEVICE_TYPE_CPU, 0, NULL, &num);
				if (res != CL_SUCCESS) {
					num = 0;
				}
			}
#endif
			return num;
		}

		uint32_t get_num_gpu_devices(Platform plat) {
			uint32_t num = 0;
#if OGLSYS_CL
			if (valid() && GetDeviceIDs) {
				cl_int res = GetDeviceIDs((cl_platform_id)plat, CL_DEVICE_TYPE_GPU, 0, NULL, &num);
				if (res != CL_SUCCESS) {
					num = 0;
				}
			}
#endif
			return num;
		}

		uint32_t get_num_acc_devices(Platform plat) {
			uint32_t num = 0;
#if OGLSYS_CL
			if (valid() && GetDeviceIDs) {
				cl_int res = GetDeviceIDs((cl_platform_id)plat, CL_DEVICE_TYPE_ACCELERATOR, 0, NULL, &num);
				if (res != CL_SUCCESS) {
					num = 0;
				}
			}
#endif
			return num;
		}

		uint32_t get_device_max_units(Device dev) {
			uint32_t num = 0;
#if OGLSYS_CL
			if (valid() && GetDeviceInfo) {
				cl_uint maxUnits = 0;
				cl_int res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(maxUnits), &maxUnits, NULL);
				if (res == CL_SUCCESS) {
					num = maxUnits;
				}
			}
#endif
			return num;
		}

		uint32_t get_device_max_freq(Device dev) {
			uint32_t num = 0;
#if OGLSYS_CL
			if (valid() && GetDeviceInfo) {
				cl_uint maxFreq = 0;
				cl_int res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(maxFreq), &maxFreq, NULL);
				if (res == CL_SUCCESS) {
					num = maxFreq;
				}
			}
#endif
			return num;
		}

		double get_device_global_mem_size(Device dev) {
			double size = 0.0;
#if OGLSYS_CL
			if (valid() && GetDeviceInfo) {
				cl_ulong memSize = 0;
				cl_int res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(memSize), &memSize, NULL);
				if (res == CL_SUCCESS) {
					size = double(memSize) / 1024.0;
				}
			}
#endif
			return size;
		}

		double get_device_local_mem_size(Device dev) {
			double size = 0.0;
#if OGLSYS_CL
			if (valid() && GetDeviceInfo) {
				cl_ulong memSize = 0;
				cl_int res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(memSize), &memSize, NULL);
				if (res == CL_SUCCESS) {
					size = double(memSize) / 1024.0;
				}
			}
#endif
			return size;
		}

		double get_device_fast_mem_size(Device dev) {
			double size = 0.0;
#if OGLSYS_CL
			if (valid() && GetDeviceInfo) {
				if (ck_device_ext(dev, "cl_ti_msmc_buffers")) {
					cl_ulong memSize = 0;
					cl_device_info param = 0x4060; /* CL_DEVICE_MSMC_MEM_SIZE_TI */
					cl_int res = GetDeviceInfo((cl_device_id)dev, param, sizeof(memSize), &memSize, NULL);
					if (res == CL_SUCCESS) {
						size = double(memSize) / 1024.0;
					}
				}
			}
#endif
			return size;
		}

		bool device_has_local_mem(Device dev) {
			bool loc = false;
#if OGLSYS_CL
			if (valid() && GetDeviceInfo) {
				cl_device_local_mem_type memType;
				cl_int res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_LOCAL_MEM_TYPE, sizeof(memType), &memType, NULL);
				if (res == CL_SUCCESS) {
					loc = memType == CL_LOCAL;
				}
			}
#endif
			return loc;
		}

		bool device_supports_fp16(Device dev) {
			return ck_device_ext(dev, "cl_khr_fp16");
		}

		bool device_supports_fp64(Device dev) {
			return ck_device_ext(dev, "cl_khr_fp64");
		}

		bool device_is_byte_addressable(Device dev) {
			return ck_device_ext(dev, "cl_khr_byte_addressable_store");
		}

		bool device_is_intel_gpu(Device dev) {
			bool ck = false;
#if OGLSYS_CL
			if (valid() && dev) {
				size_t size = 0;
				char str[32];
				cl_int res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_VENDOR, sizeof(str), str, &size);
				if (res == CL_SUCCESS) {
					static const char* pIntelCk = "Intel(";
					size_t ckSize = oglsys_str_len(pIntelCk);
					if (size > ckSize && ::memcmp(str, pIntelCk, ckSize) == 0) {
						cl_device_type type;
						res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_TYPE, sizeof(type), &type, NULL);
						if (res == CL_SUCCESS) {
							ck = !!(type & CL_DEVICE_TYPE_GPU);
						}
					}
				}
			}
#endif
			return ck;
		}

		bool device_is_vivante_gpu(Device dev) {
			bool ck = false;
#if OGLSYS_CL
			if (valid() && dev) {
				size_t size = 0;
				char str[32];
				cl_int res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_VENDOR, sizeof(str), str, &size);
				if (res == CL_SUCCESS) {
					static const char* pVivCk = "Vivante ";
					size_t ckSize = oglsys_str_len(pVivCk);
					if (size > ckSize && ::memcmp(str, pVivCk, ckSize) == 0) {
						cl_device_type type;
						res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_TYPE, sizeof(type), &type, NULL);
						if (res == CL_SUCCESS) {
							ck = !!(type & CL_DEVICE_TYPE_GPU);
						}
					}
				}
			}
#endif
			return ck;
		}

		bool device_is_arm_gpu(Device dev) {
			bool ck = false;
#if OGLSYS_CL
			if (valid() && dev) {
				size_t size = 0;
				char str[32];
				cl_int res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_VENDOR, sizeof(str), str, &size);
				if (res == CL_SUCCESS) {
					static const char* pARMCk = "ARM";
					size_t ckSize = oglsys_str_len(pARMCk);
					if (size > ckSize && ::memcmp(str, pARMCk, ckSize) == 0) {
						cl_device_type type;
						res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_TYPE, sizeof(type), &type, NULL);
						if (res == CL_SUCCESS) {
							if (type & CL_DEVICE_TYPE_GPU) {
								if (ck_device_ext(dev, "cl_arm_core_id")) {
									ck = true;
								}
							}
						}
					}
				}
			}
#endif
			return ck;
		}

		void print_device_exts(Device dev) {
#if OGLSYS_CL
			if (!dev) return;
			size_t extsSize = 0;
			cl_int res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_EXTENSIONS, 0, NULL, &extsSize);
			if (res == CL_SUCCESS) {
				char* pExts = (char*)OGLSys::mem_alloc(extsSize, "OGLSys:CL:tmpDevExts");
				if (pExts) {
					res = GetDeviceInfo((cl_device_id)dev, CL_DEVICE_EXTENSIONS, extsSize, pExts, NULL);
					if (res == CL_SUCCESS) {
						::printf("device exts: %s\n", pExts);
					}
					OGLSys::mem_free(pExts);
				}
			}
#endif
		}

		Context create_device_context(Device dev) {
			Context ctx = NULL;
#if OGLSYS_CL
			if (valid() && dev) {
				cl_device_id devId = (cl_device_id)dev;
				cl_int err = 0;
				ctx = (Context)CreateContext(NULL, 1, &devId, NULL, NULL, &err);
			}
#endif
			return ctx;
		}

		void destroy_device_context(Context ctx) {
#if OGLSYS_CL
			if (valid() && ctx) {
				cl_uint refCnt = 0;
				cl_int res = GetContextInfo((cl_context)ctx, CL_CONTEXT_REFERENCE_COUNT, sizeof(refCnt), &refCnt, NULL);
				if (res == CL_SUCCESS) {
					for (cl_uint i = 0; i < refCnt; ++i) {
						res = ReleaseContext((cl_context)ctx);
						if (res != CL_SUCCESS) break;
					}
				}
			}
#endif
		}

		Device device_from_context(Context ctx) {
			Device dev = NULL;
#if OGLSYS_CL
			if (valid() && ctx) {
				cl_device_id clDev = NULL;
				cl_int res = GetContextInfo((cl_context)ctx, CL_CONTEXT_DEVICES, sizeof(clDev), &clDev, NULL);
				if (res == CL_SUCCESS) {
					dev = (Device)clDev;
				}
			}
#endif
			return dev;
		}

		Buffer create_host_mem_buffer(Context ctx, void* p, const size_t size, const bool read, const bool write) {
			Buffer buf = NULL;
#if OGLSYS_CL
			if (valid() && ctx && (p && size) && (read || write)) {
				cl_mem_flags flags = CL_MEM_WRITE_ONLY;
				if (read && write) {
					flags = CL_MEM_READ_WRITE;
				} else if (read && !write) {
					flags = CL_MEM_READ_ONLY;
				}
				flags |= CL_MEM_USE_HOST_PTR;
				cl_int res = 0;
				cl_mem mem = CreateBuffer((cl_context)ctx, flags, size, p, &res);
				if (res == CL_SUCCESS) {
					buf = (Buffer)mem;
				}
			}
#endif
			return buf;

		}

		void release_buffer(Buffer buf) {
#if OGLSYS_CL
			if (valid() && buf) {
				ReleaseMemObject((cl_mem)buf);
			}
#endif
		}

		void* alloc_svm(Context ctx, size_t size) {
			void* p = nullptr;
#if OGLSYS_CL
			if (valid() && GLG.mpfnTIAllocDDR && ctx && size > 0) {
				if (ck_device_ext(device_from_context(ctx), "cl_ti_clmalloc")) {
					p = GLG.mpfnTIAllocDDR(size);
				}
			}
#endif
			return p;
		}

		void free_svm(Context ctx, void* p) {
#if OGLSYS_CL
			if (valid() && GLG.mpfnTIFreeDDR && ctx && p) {
				if (ck_device_ext(device_from_context(ctx), "cl_ti_clmalloc")) {
					GLG.mpfnTIFreeDDR(p);
				}
			}
#endif
		}

		void* alloc_fast(Context ctx, size_t size) {
			void* p = nullptr;
#if OGLSYS_CL
			if (valid() && GLG.mpfnTIAllocMSMC && ctx && size > 0) {
				if (ck_device_ext(device_from_context(ctx), "cl_ti_msmc_buffers")) {
					p = GLG.mpfnTIAllocMSMC(size);
				}
			}
#endif
			return p;
		}

		void free_fast(Context ctx, void* p) {
#if OGLSYS_CL
			if (valid() && GLG.mpfnTIFreeMSMC && ctx && p) {
				if (ck_device_ext(device_from_context(ctx), "cl_ti_msmc_buffers")) {
					GLG.mpfnTIFreeMSMC(p);
				}
			}
#endif
		}

		Queue create_queue(Context ctx) {
			Queue que = NULL;
#if OGLSYS_CL
			if (valid() && ctx) {
				cl_device_id devId = NULL;
				cl_int res = GetContextInfo((cl_context)ctx, CL_CONTEXT_DEVICES, sizeof(devId), &devId, NULL);
				if (res == CL_SUCCESS) {
					cl_command_queue cq = CreateCommandQueue((cl_context)ctx, devId, 0, &res);
					if (res == CL_SUCCESS) {
						que = (Queue)cq;
					}
				}
			}
#endif
			return que;
		}

		void release_queue(Queue que) {
#if OGLSYS_CL
			if (valid() && que) {
				ReleaseCommandQueue((cl_command_queue)que);
			}
#endif
		}

		void flush_queue(Queue que) {
#if OGLSYS_CL
			if (valid() && que) {
				Flush((cl_command_queue)que);
			}
#endif
		}

		void finish_queue(Queue que) {
#if OGLSYS_CL
			if (valid() && que) {
				Finish((cl_command_queue)que);
			}
#endif
		}

		void update_host_mem_in_buffer(Queue que, Buffer buf, void* p, const size_t size) {
#if OGLSYS_CL
			if (valid() && que && buf) {
				EnqueueWriteBuffer((cl_command_queue)que, (cl_mem)buf, CL_TRUE, 0, size, p, 0, NULL, NULL);
			}
#endif
		}

		void update_host_mem_out_buffer(Queue que, Buffer buf, void* p, const size_t size) {
#if OGLSYS_CL
			if (valid() && que && buf) {
				EnqueueReadBuffer((cl_command_queue)que, (cl_mem)buf, CL_TRUE, 0, size, p, 0, NULL, NULL);
			}
#endif
		}

		void exec_kernel(Queue que, Kernel kern, const int numUnits, Event* pEvt) {
			if (pEvt) {
				*pEvt = NULL;
			}
#if OGLSYS_CL
			if (valid() && que && kern && numUnits > 0) {
				size_t numWkUnits = (size_t)numUnits;
				cl_int res = EnqueueNDRangeKernel((cl_command_queue)que, (cl_kernel)kern, 1, NULL, &numWkUnits, NULL, 0, NULL, (cl_event*)pEvt);
				if (res != CL_SUCCESS) {
				}
			}
#endif
		}

		Kernel create_kernel_from_src(Context ctx, const char* pSrc, const char* pEntryName, const char* pOpts) {
			Kernel kern = NULL;
#if OGLSYS_CL
			if (valid() && ctx && pSrc) {
				cl_int res = 0;
				cl_program prog = CreateProgramWithSource((cl_context)ctx, 1, &pSrc, NULL, &res);
				if (res == CL_SUCCESS) {
					res = BuildProgram(prog, 0, NULL, pOpts, NULL, NULL);
					if (res == CL_SUCCESS) {
						cl_kernel clkn = CreateKernel(prog, pEntryName, &res);
						if (res == CL_SUCCESS) {
							kern = (Kernel)clkn;
						}
					} else {
						cl_device_id dev = NULL;
						res = GetContextInfo((cl_context)ctx, CL_CONTEXT_DEVICES, sizeof(dev), &dev, NULL);
						if (res == CL_SUCCESS) {
							size_t logSize = 0;
							res = GetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
							if (res == CL_SUCCESS) {
								char* pLog = (char*)GLG.mem_alloc(logSize, "OGLSys:CL:buildLog");
								if (pLog) {
									res = GetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, logSize, pLog, NULL);
									if (res == CL_SUCCESS) {
										glg_dbg_info(pLog, logSize);
									}
									GLG.mem_free(pLog);
									pLog = nullptr;
								}
							}
						}
					}
				}
			}
#endif
			return kern;
		}

		void release_kernel(Kernel kern) {
#if OGLSYS_CL
			if (valid() && kern) {
				cl_program prog = NULL;
				cl_int res = GetKernelInfo((cl_kernel)kern, CL_KERNEL_PROGRAM, sizeof(prog), &prog, NULL);
				if (res == CL_SUCCESS) {
					ReleaseKernel((cl_kernel)kern);
					ReleaseProgram(prog);
				}
			}
#endif
		}

		void set_kernel_arg(Kernel kern, uint32_t idx, const void* pVal, size_t size) {
#if OGLSYS_CL
			if (valid() && kern && size > 0 && pVal) {
				SetKernelArg((cl_kernel)kern, (cl_uint)idx, size, pVal);
			}
#endif
		}

		void set_kernel_int_arg(Kernel kern, uint32_t idx, const int val) {
#if OGLSYS_CL
			cl_int clVal = (cl_int)val;
			set_kernel_arg(kern, idx, &clVal, sizeof(clVal));
#endif
		}

		void set_kernel_float_arg(Kernel kern, uint32_t idx, const float val) {
#if OGLSYS_CL
			cl_float clVal = (cl_float)val;
			set_kernel_arg(kern, idx, &clVal, sizeof(clVal));
#endif
		}

		void set_kernel_float3_arg(Kernel kern, uint32_t idx, const float x, const float y, const float z) {
#if OGLSYS_CL
			cl_float3 clVal;
			clVal.s[0] = x;
			clVal.s[1] = y;
			clVal.s[2] = z;
			set_kernel_arg(kern, idx, &clVal, sizeof(clVal));
#endif
		}

		void set_kernel_buffer_arg(Kernel kern, uint32_t idx, const Buffer buf) {
#if OGLSYS_CL
			cl_mem clVal = (cl_mem)buf;
			set_kernel_arg(kern, idx, &clVal, sizeof(clVal));
#endif
		}

		void wait_event(Event evt) {
#if OGLSYS_CL
			if (valid() && evt) {
				cl_event e = (cl_event)evt;
				WaitForEvents(1, &e);
			}
#endif
		}

		void release_event(Event evt) {
#if OGLSYS_CL
			if (valid() && evt) {
				cl_uint refCnt = 0;
				cl_int res = GetEventInfo((cl_event)evt, CL_EVENT_REFERENCE_COUNT, sizeof(refCnt), &refCnt, NULL);
				if (res == CL_SUCCESS && refCnt > 0) {
					ReleaseEvent((cl_event)evt);
				}
			}
#endif
		}

		void wait_events(Event* pEvts, const int n) {
#if OGLSYS_CL
			if (valid() && pEvts && n > 0) {
				WaitForEvents((cl_uint)n, (const cl_event*)pEvts);
			}
#endif
		}

		bool event_ck_queued(Event evt) {
			bool ck = false;
#if OGLSYS_CL
			if (valid() && evt) {
				cl_int execStatus = 0;
				cl_int res = GetEventInfo((cl_event)evt, CL_EVENT_COMMAND_EXECUTION_STATUS,
				                          sizeof(execStatus), &execStatus, NULL);
				if (res == CL_SUCCESS) {
					ck = execStatus == CL_QUEUED;
				}
			}
#endif
			return ck;
		}

		bool event_ck_submitted(Event evt) {
			bool ck = false;
#if OGLSYS_CL
			if (valid() && evt) {
				cl_int execStatus = 0;
				cl_int res = GetEventInfo((cl_event)evt, CL_EVENT_COMMAND_EXECUTION_STATUS,
				                          sizeof(execStatus), &execStatus, NULL);
				if (res == CL_SUCCESS) {
					ck = execStatus == CL_SUBMITTED;
				}
			}
#endif
			return ck;
		}

		bool event_ck_running(Event evt) {
			bool ck = false;
#if OGLSYS_CL
			if (valid() && evt) {
				cl_int execStatus = 0;
				cl_int res = GetEventInfo((cl_event)evt, CL_EVENT_COMMAND_EXECUTION_STATUS,
				                          sizeof(execStatus), &execStatus, NULL);
				if (res == CL_SUCCESS) {
					ck = execStatus == CL_RUNNING;
				}
			}
#endif
			return ck;
		}

		bool event_ck_complete(Event evt) {
			bool ck = evt ? false : true;
#if OGLSYS_CL
			if (valid() && evt) {
				cl_int execStatus = 0;
				cl_int res = GetEventInfo((cl_event)evt, CL_EVENT_COMMAND_EXECUTION_STATUS,
				                          sizeof(execStatus), &execStatus, NULL);
				if (res == CL_SUCCESS) {
					ck = execStatus == CL_COMPLETE;
				}
			}
#endif
			return ck;
		}

	} // CL

} // OGLSys
