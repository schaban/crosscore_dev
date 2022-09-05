// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2019-2021 Sergey Chaban <sergey.chaban@gmail.com>

#include "crosscore.hpp"
#include "oglsys.hpp"
#include "draw.hpp"

#include "ogl/gpu_defs.h"

#ifndef DRW_CACHE_PARAMS
#	define DRW_CACHE_PARAMS 1
#endif

#ifndef DRW_CACHE_PROGS
#	define DRW_CACHE_PROGS 1
#endif

#ifndef DRW_CACHE_BLEND
#	define DRW_CACHE_BLEND 1
#endif

#ifndef DRW_CACHE_DSIDED
#	define DRW_CACHE_DSIDED 1
#endif

#ifndef DRW_LIMIT_JMAP
#	define DRW_LIMIT_JMAP 1
#endif

#ifndef DRW_USE_VAO
#	define DRW_USE_VAO 1
#endif

#ifndef DRW_USE_MEMCMP
#	define DRW_USE_MEMCMP 1
#endif

DRW_IMPL_BEGIN

static bool s_drwInitFlg = false;

static bool s_useMipmaps = true;

static bool s_useVtxLighting = false;

static cxResourceManager* s_pRsrcMgr = nullptr;

struct GPUProg;
static const GPUProg* s_pNowProg = nullptr;

static int s_shadowSize = 0;
static GLuint s_shadowFBO = 0;
static GLuint s_shadowTex = 0;
static GLuint s_shadowDepthBuf = 0;
static bool s_shadowCastDepthTest = true;

static int s_frameBufMode = 0; // 0: def, 1: shadow, 2: screen
static int s_batDrwCnt = 0;
static int s_shadowCastCnt = 0;

static GLuint s_primVBO = 0;
static uint32_t s_maxPrimVtx = 0;
static GLuint s_primIBO = 0;
static uint32_t s_maxPrimIdx = 0;

static GLuint s_quadVBO = 0;
static GLuint s_quadIBO = 0;

static Draw::Font* s_pFont = nullptr;
static GLuint s_fontVBO = 0;
static GLuint s_fontIBO = 0;

static bool s_glslEcho = false;
static const char* s_pGLSLBinSavePath = nullptr;
static const char* s_pGLSLBinLoadPath = nullptr;

static const char* s_pAltGLSL = nullptr;

static bool s_glslNoBaseTex = false;
static bool s_glslNoFog = false;
static bool s_glslNoCC = false;


static void def_tex_lod_bias() {
	static bool flg = false;
	static int bias = 0;
	if (!flg) {
		bias = nxApp::get_int_opt("tlod_bias", -3);
		flg = true;
	}
	OGLSys::set_tex2d_lod_bias(bias);

}

static void def_tex_params(const bool mipmapEnabled, const bool biasEnabled) {
	if (s_useMipmaps && mipmapEnabled) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		if (biasEnabled) {
			def_tex_lod_bias();
		} else {
			OGLSys::set_tex2d_lod_bias(0);
		}
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

static GLuint load_shader(const char* pName) {
	GLuint sid = 0;
	const char* pDataPath = s_pRsrcMgr ? s_pRsrcMgr->get_data_path() : nullptr;
	if (pName) {
		char path[256];
		char* pPath = path;
		size_t pathBufSize = sizeof(path);
		size_t pathSize = (pDataPath ? ::strlen(pDataPath) : 1) + 5 + ::strlen(pName) + 1;
		if (pathSize > pathBufSize) {
			pPath = (char*)nxCore::mem_alloc(pathSize, "glsl_path");
			pathBufSize = pathSize;
		}
		if (pPath && pathBufSize > 0) {
			XD_SPRINTF(XD_SPRINTF_BUF(pPath, pathBufSize), "%s/%s/%s", pDataPath ? pDataPath : ".", s_pAltGLSL ? s_pAltGLSL: "ogl", pName);
		}
		size_t srcSize = 0;
		char* pSrc = nullptr;
		if (pPath) {
			pSrc = (char*)nxCore::bin_load(pPath, &srcSize, false, true);
		}
		if (pSrc) {
			GLenum kind = nxCore::str_ends_with(pName, ".vert") ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
#if OGLSYS_ES
			const char* pPreStr = nullptr;
			if ((s_glslNoBaseTex || s_glslNoFog || s_glslNoCC) && (kind == GL_FRAGMENT_SHADER)) {
				if (!nxCore::str_starts_with(pName, "quad")) {
					if (s_glslNoBaseTex) {
						if (s_glslNoCC) {
							if (s_glslNoFog) {
								pPreStr = "#define DRW_NOBASETEX\n#define DRW_NOFOG\n#define DRW_NOCC\n";
							} else {
								pPreStr = "#define DRW_NOBASETEX\n#define DRW_NOCC\n";
							}
						} else {
							if (s_glslNoFog) {
								pPreStr = "#define DRW_NOBASETEX\n#define DRW_NOFOG\n";
							} else {
								pPreStr = "#define DRW_NOBASETEX\n";
							}
						}
					} else if (s_glslNoFog) {
						if (s_glslNoCC) {
							pPreStr = "#define DRW_NOFOG\n#define DRW_NOCC\n";
						} else {
							pPreStr = "#define DRW_NOFOG\n";
						}
					} else if (s_glslNoCC) {
						pPreStr = "#define DRW_NOCC\n";
					}
				}
			}
			if (pPreStr) {
				size_t preSize = ::strlen(pPreStr);
				size_t altSize = preSize + srcSize;
				char* pAltSrc = (char*)nxCore::mem_alloc(altSize, "glsl:pre+src");
				if (pAltSrc) {
					nxCore::mem_copy(pAltSrc, pPreStr, preSize);
					nxCore::mem_copy(pAltSrc + preSize, pSrc, srcSize);
					sid = OGLSys::compile_shader_str(pAltSrc, altSize, kind);
					nxCore::mem_free(pAltSrc);
				}
			} else {
				sid = OGLSys::compile_shader_str(pSrc, srcSize, kind);
			}
#else
			const char* pPreStr;
#	if defined(OGLSYS_WEB)
			if (kind == GL_VERTEX_SHADER) {
				pPreStr = "#version 100\n#define WEBGL\n";
			} else {
				pPreStr = "#version 100\n";
			}
#	else
			pPreStr = "#version 120\n";
			if ((s_glslNoBaseTex || s_glslNoFog || s_glslNoCC) && (kind == GL_FRAGMENT_SHADER)) {
				if (!nxCore::str_starts_with(pName, "quad")) {
					if (s_glslNoBaseTex) {
						if (s_glslNoCC) {
							if (s_glslNoFog) {
								pPreStr = "#version 120\n#define DRW_NOBASETEX\n#define DRW_NOFOG\n#define DRW_NOCC\n";
							} else {
								pPreStr = "#version 120\n#define DRW_NOBASETEX\n#define DRW_NOCC\n";
							}
						} else {
							if (s_glslNoFog) {
								pPreStr = "#version 120\n#define DRW_NOBASETEX\n#define DRW_NOFOG\n";
							} else {
								pPreStr = "#version 120\n#define DRW_NOBASETEX\n";
							}
						}
					} else if (s_glslNoFog) {
						if (s_glslNoCC) {
							pPreStr = "#version 120\n#define DRW_NOFOG\n#define DRW_NOCC\n";
						} else {
							pPreStr = "#version 120\n#define DRW_NOFOG\n";
						}
					} else if (s_glslNoCC) {
						pPreStr = "#version 120\n#define DRW_NOCC\n";
					}
				}
			}
#	endif
			size_t preSize = ::strlen(pPreStr);
			size_t altSize = preSize + srcSize;
			char* pAltSrc = (char*)nxCore::mem_alloc(altSize, "glsl:pre+src");
			if (pAltSrc) {
				nxCore::mem_copy(pAltSrc, pPreStr, preSize);
				nxCore::mem_copy(pAltSrc + preSize, pSrc, srcSize);
				sid = OGLSys::compile_shader_str(pAltSrc, altSize, kind);
				nxCore::mem_free(pAltSrc);
			}
#endif
			nxCore::bin_unload(pSrc);
		}
		if (pPath != path) {
			nxCore::mem_free(pPath);
			pPath = nullptr;
		}
	}
	return sid;
}

struct VtxLink {
	GLint Pos;
	GLint Oct;
	GLint RelPosW0;
	GLint OctW1W2;
	GLint Clr;
	GLint Tex;
	GLint Wgt;
	GLint Jnt;
	GLint Prm;
	GLint Id;

	static int num_attrs() {
		return int(sizeof(VtxLink) / sizeof(GLint));
	}

	void reset() { nxCore::mem_fill(this, 0xFF, sizeof(*this)); }

	void disable_all() const {
		int n = num_attrs();
		const GLint* p = reinterpret_cast<const GLint*>(this);
		for (int i = 0; i < n; ++i) {
			GLint iattr = p[i];
			if (iattr >= 0) {
				glDisableVertexAttribArray(iattr);
			}
		}
	}

	uint32_t get_mask() const {
		uint32_t msk = 0;
		int n = num_attrs();
		const GLint* p = reinterpret_cast<const GLint*>(this);
		for (int i = 0; i < n; ++i) {
			GLint iattr = p[i];
			msk |= (iattr >= 0 ? 1 : 0) << i;
		}
		return msk;
	}
};

struct ParamLink {
	GLint PosBase;
	GLint PosScale;
	GLint SkinMtx;
	GLint SkinMap;
	GLint World;
	GLint ViewProj;
	GLint ViewPos;
	GLint InvView;
	GLint ScrXform;
	GLint HemiUp;
	GLint HemiUpper;
	GLint HemiLower;
	GLint HemiParam;
	GLint SpecLightDir;
	GLint SpecLightColor;
	GLint VtxHemiUp;
	GLint VtxHemiUpper;
	GLint VtxHemiLower;
	GLint VtxHemiParam;
	GLint ShadowMtx;
	GLint ShadowSize;
	GLint ShadowCtrl;
	GLint ShadowFade;
	GLint BaseColor;
	GLint SpecColor;
	GLint SurfParam;
	GLint BumpParam;
	GLint AlphaCtrl;
	GLint BumpPatUV;
	GLint BumpPatParam;
	GLint FogColor;
	GLint FogParam;
	GLint InvWhite;
	GLint LClrGain;
	GLint LClrBias;
	GLint Exposure;
	GLint InvGamma;
	GLint PrimCtrl;
	GLint QuadVtxPos;
	GLint QuadVtxTex;
	GLint QuadVtxClr;
	GLint FontColor;
	GLint FontXform;
	GLint FontRot;

	void reset() { nxCore::mem_fill(this, 0xFF, sizeof(*this)); }
};

struct SmpLink {
	GLint Base;
	GLint Spec;
	GLint Bump;
	GLint Surf;
	GLint BumpPat;
	GLint Shadow;

	void reset() { nxCore::mem_fill(this, 0xFF, sizeof(*this)); }
};

#define VTX_LINK(_name) mVtxLink._name = glGetAttribLocation(mProgId, "vtx" #_name)
#define PARAM_LINK(_name) mParamLink._name = glGetUniformLocation(mProgId, "gp" #_name)
#define SMP_LINK(_name) mSmpLink._name = glGetUniformLocation(mProgId, "smp" #_name)
#define SET_TEX_UNIT(_name) if (mSmpLink._name >= 0) { glUniform1i(mSmpLink._name, Draw::TEXUNIT_##_name); }

static void gl_param(const GLint loc, const xt_mtx& m) {
	glUniformMatrix4fv(loc, 1, GL_FALSE, m);
}

static void gl_param(const GLint loc, const xt_xmtx& wm) {
	glUniform4fv(loc, 3, wm);
}

static void gl_param(const GLint loc, const xt_float3& v) {
	glUniform3fv(loc, 1, v);
}

static void gl_param(const GLint loc, const xt_float4& v) {
	glUniform4fv(loc, 1, v);
}

enum VtxFmt {
	VtxFmt_none,
	VtxFmt_rigid0,
	VtxFmt_rigid1,
	VtxFmt_skin0,
	VtxFmt_skin1,
	VtxFmt_prim,
	VtxFmt_quad,
	VtxFmt_font,

	VtxFmt_rigid0_vl = VtxFmt_rigid0,
	VtxFmt_rigid1_vl = VtxFmt_rigid1,
	VtxFmt_skin0_vl = VtxFmt_skin0,
	VtxFmt_skin1_vl = VtxFmt_skin1
};

#define DRW_GBIN_SIG XD_FOURCC('g', 'b', 'i', 'n')
#define DRW_GBIN_EXT "gbin"

static void save_gpu_prog_bin(const GLuint pid, const char* pVertName, const char* pFragName) {
	if (!s_pGLSLBinSavePath) return;
	if (!pid) return;
	if (!pVertName) return;
	if (!pFragName) return;
	size_t size = 0;
	GLenum fmt = 0;
	void* pBin = OGLSys::get_prog_bin(pid, &size, &fmt);
	if (!pBin) return;
	if (size < 1 || size > 0x1000000) {
		OGLSys::free_prog_bin(pBin);
		return;
	}
	size_t saveSize = sizeof(uint32_t) * 3 + size;
	uint32_t* pMem = (uint32_t*)nxCore::mem_alloc(saveSize, "GPUProg:save:mem");
	if (!pMem) {
		OGLSys::free_prog_bin(pBin);
		return;
	}
	pMem[0] = DRW_GBIN_SIG;
	pMem[1] = uint32_t(size);
	pMem[2] = uint32_t(fmt);
	nxCore::mem_copy(pMem + 3, pBin, size);
	OGLSys::free_prog_bin(pBin);
	static const char* pExt = DRW_GBIN_EXT;
	char path[128];
	char* pPath = path;
	size_t bufSize = sizeof(path);
	size_t pathSize = ::strlen(s_pGLSLBinSavePath) + 1
	                + ::strlen(pVertName) + 1
	                + ::strlen(pFragName) + 1
	                + ::strlen(pExt) + 1;
	if (pathSize > sizeof(path)) {
		bufSize = pathSize;
		pPath = (char*)nxCore::mem_alloc(pathSize, "GPUProg:save:path");
	}
	if (pPath && bufSize > 0) {
		XD_SPRINTF(XD_SPRINTF_BUF(pPath, bufSize), "%s/%s_%s.%s", s_pGLSLBinSavePath, pVertName, pFragName, pExt);
		nxCore::bin_save(pPath, pMem, saveSize);
	}
	if (pPath != path) {
		nxCore::mem_free(pPath);
	}
	nxCore::mem_free(pMem);
}

static bool load_gpu_prog_bin(const GLuint pid, const char* pVertName, const char* pFragName) {
	bool res = false;
	if (!s_pGLSLBinLoadPath) return res;
	if (!pid) return res;
	if (!pVertName) return res;
	if (!pFragName) return res;
	static const char* pExt = DRW_GBIN_EXT;
	char path[128];
	char* pPath = path;
	size_t bufSize = sizeof(path);
	size_t pathSize = ::strlen(s_pGLSLBinLoadPath) + 1
	                + ::strlen(pVertName) + 1
	                + ::strlen(pFragName) + 1
	                + ::strlen(pExt) + 1;
	if (pathSize > sizeof(path)) {
		bufSize = pathSize;
		pPath = (char*)nxCore::mem_alloc(pathSize, "GPUProg:load:path");
	}
	if (pPath && bufSize > 0) {
		XD_SPRINTF(XD_SPRINTF_BUF(pPath, bufSize), "%s/%s_%s.%s", s_pGLSLBinLoadPath, pVertName, pFragName, pExt);
		size_t fsize = 0;
		void* pBin = nxCore::raw_bin_load(pPath, &fsize);
		if (pBin && fsize > 0x10) {
			uint32_t* pHead = (uint32_t*)pBin;
			if (pHead[0] == DRW_GBIN_SIG) {
				GLsizei len = (GLsizei)pHead[1];
				GLenum fmt = (GLenum)pHead[2];
				res = OGLSys::set_prog_bin(pid, fmt, (const void*)(pHead + 3), len);
			}
		}
		if (pBin) {
			nxCore::bin_unload(pBin);
		}
	}
	if (pPath != path) {
		nxCore::mem_free(pPath);
	}
	return res;
}

struct GPUProg {
	const char* mpVertName;
	const char* mpFragName;
	GLuint mVertSID;
	GLuint mFragSID;
	GLuint mProgId;
	VtxLink mVtxLink;
	ParamLink mParamLink;
	SmpLink mSmpLink;
	VtxFmt mVtxFmt;
	GLuint mVAO;

	template<typename T> struct CachedParam {
		T mVal;
		bool mFlg;

		void reset() {
			mFlg = false;
		}

		void set(const GLint loc, const T& val) {
#if DRW_CACHE_PARAMS
			if (loc >= 0) {
				if (mFlg) {
					bool updateFlg = false;
#	if DRW_USE_MEMCMP
					updateFlg = ::memcmp(&mVal, &val, sizeof(T)) != 0;
#	else
					uint32_t* pVal0 = reinterpret_cast<uint32_t*>(&mVal);
					const uint32_t* pVal1 = reinterpret_cast<const uint32_t*>(&val);
					size_t nflt = sizeof(T) / sizeof(uint32_t);
					for (size_t i = 0; i < nflt; ++i) {
						if (pVal0[i] != pVal1[i]) {
							updateFlg = true;
							break;
						}
					}
#	endif
					if (updateFlg) {
						gl_param(loc, val);
						mVal = val;
					}
				} else {
					gl_param(loc, val);
					mVal = val;
					mFlg = true;
				}
			}
#else
			if (loc >= 0) {
				gl_param(loc, val);
			}
#endif
		}
	};

	struct Cache {
		CachedParam<xt_mtx> mViewProj;
		CachedParam<xt_xmtx> mWorld;
		CachedParam<xt_mtx> mShadowMtx;
		CachedParam<xt_mtx> mInvView;

		CachedParam<xt_float3> mViewPos;

		CachedParam<xt_float3> mPosBase;
		CachedParam<xt_float3> mPosScale;

		CachedParam<xt_float3> mHemiUpper;
		CachedParam<xt_float3> mHemiLower;
		CachedParam<xt_float3> mHemiUp;
		CachedParam<xt_float3> mHemiParam;

		CachedParam<xt_float3> mSpecLightDir;
		CachedParam<xt_float4> mSpecLightColor;

		CachedParam<xt_float3> mVtxHemiUpper;
		CachedParam<xt_float3> mVtxHemiLower;
		CachedParam<xt_float3> mVtxHemiUp;
		CachedParam<xt_float3> mVtxHemiParam;

		CachedParam<xt_float3> mBaseColor;
		CachedParam<xt_float3> mSpecColor;
		CachedParam<xt_float4> mSurfParam;
		CachedParam<xt_float4> mBumpParam;
		CachedParam<xt_float3> mAlphaCtrl;

		CachedParam<xt_float4> mBumpPatUV;
		CachedParam<xt_float4> mBumpPatParam;

		CachedParam<xt_float4> mFogColor;
		CachedParam<xt_float4> mFogParam;

		CachedParam<xt_float4> mShadowSize;
		CachedParam<xt_float4> mShadowCtrl;
		CachedParam<xt_float4> mShadowFade;

		CachedParam<xt_float3> mInvWhite;
		CachedParam<xt_float3> mLClrGain;
		CachedParam<xt_float3> mLClrBias;
		CachedParam<xt_float3> mExposure;
		CachedParam<xt_float3> mInvGamma;

		CachedParam<xt_float4> mPrimCtrl;


		void reset() {
			mViewProj.reset();
			mWorld.reset();
			mShadowMtx.reset();
			mInvView.reset();
			mViewPos.reset();
			mPosBase.reset();
			mPosScale.reset();
			mHemiUpper.reset();
			mHemiLower.reset();
			mHemiUp.reset();
			mHemiParam.reset();
			mSpecLightDir.reset();
			mSpecLightColor.reset();
			mVtxHemiUpper.reset();
			mVtxHemiLower.reset();
			mVtxHemiUp.reset();
			mVtxHemiParam.reset();
			mBaseColor.reset();
			mSpecColor.reset();
			mSurfParam.reset();
			mBumpParam.reset();
			mAlphaCtrl.reset();
			mBumpPatUV.reset();
			mBumpPatParam.reset();
			mFogColor.reset();
			mFogParam.reset();
			mShadowSize.reset();
			mShadowCtrl.reset();
			mShadowFade.reset();
			mInvWhite.reset();
			mLClrGain.reset();
			mLClrBias.reset();
			mExposure.reset();
			mInvGamma.reset();
			mPrimCtrl.reset();
		}
	} mCache;


	void prepare() {
		mVtxLink.reset();
		mParamLink.reset();
		mSmpLink.reset();
		mCache.reset();
		if (!is_valid()) return;

		VTX_LINK(Pos);
		VTX_LINK(Oct);
		VTX_LINK(RelPosW0);
		VTX_LINK(OctW1W2);
		VTX_LINK(Clr);
		VTX_LINK(Tex);
		VTX_LINK(Wgt);
		VTX_LINK(Jnt);
		VTX_LINK(Prm);
		VTX_LINK(Id);

		PARAM_LINK(PosBase);
		PARAM_LINK(PosScale);
		PARAM_LINK(SkinMtx);
		PARAM_LINK(SkinMap);
		PARAM_LINK(World);
		PARAM_LINK(ViewProj);
		PARAM_LINK(InvView);
		PARAM_LINK(ViewPos);
		PARAM_LINK(ScrXform);
		PARAM_LINK(HemiUp);
		PARAM_LINK(HemiUpper);
		PARAM_LINK(HemiLower);
		PARAM_LINK(HemiParam);
		PARAM_LINK(SpecLightDir);
		PARAM_LINK(SpecLightColor);
		PARAM_LINK(VtxHemiUp);
		PARAM_LINK(VtxHemiUpper);
		PARAM_LINK(VtxHemiLower);
		PARAM_LINK(VtxHemiParam);
		PARAM_LINK(ShadowMtx);
		PARAM_LINK(ShadowSize);
		PARAM_LINK(ShadowCtrl);
		PARAM_LINK(ShadowFade);
		PARAM_LINK(BaseColor);
		PARAM_LINK(SpecColor);
		PARAM_LINK(SurfParam);
		PARAM_LINK(BumpParam);
		PARAM_LINK(AlphaCtrl);
		PARAM_LINK(BumpPatUV);
		PARAM_LINK(BumpPatParam);
		PARAM_LINK(FogColor);
		PARAM_LINK(FogParam);
		PARAM_LINK(InvWhite);
		PARAM_LINK(LClrGain);
		PARAM_LINK(LClrBias);
		PARAM_LINK(Exposure);
		PARAM_LINK(InvGamma);
		PARAM_LINK(PrimCtrl);
		PARAM_LINK(QuadVtxPos);
		PARAM_LINK(QuadVtxTex);
		PARAM_LINK(QuadVtxClr);
		PARAM_LINK(FontColor);
		PARAM_LINK(FontXform);
		PARAM_LINK(FontRot);

		SMP_LINK(Base);
		SMP_LINK(Bump);
		SMP_LINK(Spec);
		SMP_LINK(Surf);
		SMP_LINK(Shadow);

		glUseProgram(mProgId);
		SET_TEX_UNIT(Base);
		SET_TEX_UNIT(Bump);
		SET_TEX_UNIT(Spec);
		SET_TEX_UNIT(Surf);
		SET_TEX_UNIT(BumpPat);
		SET_TEX_UNIT(Shadow);
		glUseProgram(0);

		mVAO = DRW_USE_VAO ? OGLSys::gen_vao() : 0;
	}

	void init(VtxFmt vfmt, GLuint vertSID, GLuint fragSID, const char* pVertName, const char* pFragName) {
		mpVertName = pVertName;
		mpFragName = pFragName;
		mVertSID = vertSID;
		mFragSID = fragSID;
		mVtxFmt = vfmt;

		if (s_pGLSLBinLoadPath) {
			mProgId = glCreateProgram();
			if (mProgId) {
				bool loadRes = load_gpu_prog_bin(mProgId, pVertName, pFragName);
				if (!loadRes) {
					glDeleteProgram(mProgId);
					mProgId = 0;
				}
			}
		} else {
			mProgId = OGLSys::link_draw_prog(mVertSID, mFragSID);
		}

		prepare();
		save_bin();
	}

	void reset() {
		if (!is_valid()) return;
		glDetachShader(mProgId, mVertSID);
		glDetachShader(mProgId, mFragSID);
		glDeleteProgram(mProgId);
		mVertSID = 0;
		mFragSID = 0;
		mProgId = 0;
		mVtxLink.reset();
		mParamLink.reset();
		mSmpLink.reset();
		mCache.reset();
		if (mVAO) {
			OGLSys::del_vao(mVAO);
			mVAO = 0;
		}
	}

	bool is_valid() const { return mProgId != 0; }

	bool is_skin() const {
		return mVtxLink.Jnt >= 0;
	}

	void use() const {
#if DRW_CACHE_PROGS
		if (s_pNowProg != this) {
			glUseProgram(mProgId);
			s_pNowProg = this;
		}
#else
		glUseProgram(mProgId);
#endif
	}

	void save_bin() {
		if (!is_valid()) return;
		save_gpu_prog_bin(mProgId, mpVertName, mpFragName);
	}

	void enable_attrs(const int minIdx, const size_t vtxSize = 0) const;

	void disable_attrs() const {
		mVtxLink.disable_all();
	}

	void set_view_proj(const xt_mtx& m) {
		mCache.mViewProj.set(mParamLink.ViewProj, m);
	}

	void set_world(const xt_xmtx& wm) {
		mCache.mWorld.set(mParamLink.World, wm);
	}

	void set_shadow_mtx(const xt_mtx& sm) {
		mCache.mShadowMtx.set(mParamLink.ShadowMtx, sm);
	}

	void set_inv_view(const xt_mtx& m) {
		mCache.mInvView.set(mParamLink.InvView, m);
	}

	void set_view_pos(const xt_float3& pos) {
		mCache.mViewPos.set(mParamLink.ViewPos, pos);
	}

	void set_pos_base(const xt_float3& base) {
		mCache.mPosBase.set(mParamLink.PosBase, base);
	}

	void set_pos_scale(const xt_float3& scl) {
		mCache.mPosScale.set(mParamLink.PosScale, scl);
	}

	void set_hemi_upper(const xt_float3& hupr) {
		mCache.mHemiUpper.set(mParamLink.HemiUpper, hupr);
	}

	void set_hemi_lower(const xt_float3& hlwr) {
		mCache.mHemiLower.set(mParamLink.HemiLower, hlwr);
	}

	void set_hemi_up(const xt_float3& upvec) {
		mCache.mHemiUp.set(mParamLink.HemiUp, upvec);
	}

	void set_hemi_param(const xt_float3& hprm) {
		mCache.mHemiParam.set(mParamLink.HemiParam, hprm);
	}

	void set_spec_light_dir(const xt_float3& dir) {
		mCache.mSpecLightDir.set(mParamLink.SpecLightDir, dir);
	}

	void set_spec_light_color(const xt_float4& c) {
		mCache.mSpecLightColor.set(mParamLink.SpecLightColor, c);
	}

	void set_vtx_hemi_upper(const xt_float3& hupr) {
		mCache.mVtxHemiUpper.set(mParamLink.VtxHemiUpper, hupr);
	}

	void set_vtx_hemi_lower(const xt_float3& hlwr) {
		mCache.mVtxHemiLower.set(mParamLink.VtxHemiLower, hlwr);
	}

	void set_vtx_hemi_up(const xt_float3& upvec) {
		mCache.mVtxHemiUp.set(mParamLink.VtxHemiUp, upvec);
	}

	void set_vtx_hemi_param(const xt_float3& hprm) {
		mCache.mVtxHemiParam.set(mParamLink.VtxHemiParam, hprm);
	}

	void set_base_color(const xt_float3& c) {
		mCache.mBaseColor.set(mParamLink.BaseColor, c);
	}

	void set_spec_color(const xt_float3& c) {
		mCache.mSpecColor.set(mParamLink.SpecColor, c);
	}

	void set_surf_param(const xt_float4& sprm) {
		mCache.mSurfParam.set(mParamLink.SurfParam, sprm);
	}

	void set_bump_param(const xt_float4& bprm) {
		mCache.mBumpParam.set(mParamLink.BumpParam, bprm);
	}

	void set_alpha_ctrl(const xt_float3& actrl) {
		mCache.mAlphaCtrl.set(mParamLink.AlphaCtrl, actrl);
	}

	void set_bump_pat_uv(sxModelData::Material::NormPatternExt* pExt) {
		if (!pExt) return;
		xt_float4 uv;
		uv.set(pExt->offs.x, pExt->offs.y, pExt->scl.x, pExt->scl.y);
		mCache.mBumpPatUV.set(mParamLink.BumpPatUV, uv);
	}

	void set_bump_pat_param(sxModelData::Material::NormPatternExt* pExt) {
		if (!pExt) return;
		xt_float4 param;
		param.set(pExt->factor.x, pExt->factor.y, 0.0f, 0.0f);
		mCache.mBumpPatParam.set(mParamLink.BumpPatParam, param);
	}

	void set_bump_pat(sxModelData::Material::NormPatternExt* pExt) {
		set_bump_pat_uv(pExt);
		set_bump_pat_param(pExt);
	}

	void set_fog_color(const xt_float4& fclr) {
		mCache.mFogColor.set(mParamLink.FogColor, fclr);
	}

	void set_fog_param(const xt_float4 fprm) {
		mCache.mFogParam.set(mParamLink.FogParam, fprm);
	}

	void set_shadow_size(const xt_float4& size) {
		mCache.mShadowSize.set(mParamLink.ShadowSize, size);
	}

	void set_shadow_ctrl(const xt_float4& ctrl) {
		mCache.mShadowCtrl.set(mParamLink.ShadowCtrl, ctrl);
	}

	void set_shadow_fade(const xt_float4& fade) {
		mCache.mShadowFade.set(mParamLink.ShadowFade, fade);
	}

	void set_inv_white(const xt_float3& iw) {
		mCache.mInvWhite.set(mParamLink.InvWhite, iw);
	}

	void set_lclr_gain(const xt_float3& gain) {
		mCache.mLClrGain.set(mParamLink.LClrGain, gain);
	}

	void set_lclr_bias(const xt_float3& bias) {
		mCache.mLClrBias.set(mParamLink.LClrBias, bias);
	}

	void set_exposure(const xt_float3& e) {
		mCache.mExposure.set(mParamLink.Exposure, e);
	}

	void set_inv_gamma(const xt_float3& ig) {
		mCache.mInvGamma.set(mParamLink.InvGamma, ig);
	}

	void set_prim_ctrl(const xt_float4& ctrl) {
		mCache.mPrimCtrl.set(mParamLink.PrimCtrl, ctrl);
	}
};

void GPUProg::enable_attrs(const int minIdx, const size_t vtxSize) const {
	GLsizei stride = (GLsizei)vtxSize;
	if (stride <= 0) {
		switch (mVtxFmt) {
			case VtxFmt_skin0: stride = sizeof(sxModelData::VtxSkinHalf); break;
			case VtxFmt_skin1: stride = sizeof(sxModelData::VtxSkinShort); break;
			case VtxFmt_rigid0: stride = sizeof(sxModelData::VtxRigidHalf); break;
			case VtxFmt_rigid1: stride = sizeof(sxModelData::VtxRigidShort); break;
			case VtxFmt_prim: stride = sizeof(sxPrimVtx); break;
			case VtxFmt_quad: stride = sizeof(float); break;
			case VtxFmt_font: stride = sizeof(xt_float2); break;
			default: break;
		}
	}
	size_t top = minIdx * stride;
	switch (mVtxFmt) {
		case VtxFmt_skin0:
			if (mVtxLink.Pos >= 0) {
				glEnableVertexAttribArray(mVtxLink.Pos);
				glVertexAttribPointer(mVtxLink.Pos, 3, GL_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxSkinHalf, pos)));
			} else {
				return;
			}
			if (mVtxLink.Oct >= 0) {
				glEnableVertexAttribArray(mVtxLink.Oct);
				glVertexAttribPointer(mVtxLink.Oct, 2, GL_HALF_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxSkinHalf, oct)));
			}
			if (mVtxLink.Tex >= 0) {
				glEnableVertexAttribArray(mVtxLink.Tex);
				glVertexAttribPointer(mVtxLink.Tex, 2, GL_HALF_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxSkinHalf, tex)));
			}
			if (mVtxLink.Clr >= 0) {
				glEnableVertexAttribArray(mVtxLink.Clr);
				glVertexAttribPointer(mVtxLink.Clr, 4, GL_HALF_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxSkinHalf, clr)));
			}
			if (mVtxLink.Wgt >= 0) {
				glEnableVertexAttribArray(mVtxLink.Wgt);
				glVertexAttribPointer(mVtxLink.Wgt, 4, GL_UNSIGNED_SHORT, GL_TRUE, stride, (const void*)(top + offsetof(sxModelData::VtxSkinHalf, wgt)));
			}
			if (mVtxLink.Jnt >= 0) {
				glEnableVertexAttribArray(mVtxLink.Jnt);
				glVertexAttribPointer(mVtxLink.Jnt, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxSkinHalf, jnt)));
			}
			break;

		case VtxFmt_rigid0:
			if (mVtxLink.Pos >= 0) {
				glEnableVertexAttribArray(mVtxLink.Pos);
				glVertexAttribPointer(mVtxLink.Pos, 3, GL_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxRigidHalf, pos)));
			} else {
				return;
			}
			if (mVtxLink.Oct >= 0) {
				glEnableVertexAttribArray(mVtxLink.Oct);
				glVertexAttribPointer(mVtxLink.Oct, 2, GL_HALF_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxRigidHalf, oct)));
			}
			if (mVtxLink.Tex >= 0) {
				glEnableVertexAttribArray(mVtxLink.Tex);
				glVertexAttribPointer(mVtxLink.Tex, 2, GL_HALF_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxRigidHalf, tex)));
			}
			if (mVtxLink.Clr >= 0) {
				glEnableVertexAttribArray(mVtxLink.Clr);
				glVertexAttribPointer(mVtxLink.Clr, 4, GL_HALF_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxRigidHalf, clr)));
			}
			break;

		case VtxFmt_skin1:
			if (mVtxLink.RelPosW0 >= 0) {
				glEnableVertexAttribArray(mVtxLink.RelPosW0);
				glVertexAttribPointer(mVtxLink.RelPosW0, 4, GL_UNSIGNED_SHORT, GL_TRUE, stride, (const void*)(top + offsetof(sxModelData::VtxSkinShort, qpos)));
			} else {
				return;
			}
			if (mVtxLink.OctW1W2 >= 0) {
				glEnableVertexAttribArray(mVtxLink.OctW1W2);
				glVertexAttribPointer(mVtxLink.OctW1W2, 4, GL_UNSIGNED_SHORT, GL_TRUE, stride, (const void*)(top + offsetof(sxModelData::VtxSkinShort, oct)));
			}
			if (mVtxLink.Clr >= 0) {
				glEnableVertexAttribArray(mVtxLink.Clr);
				glVertexAttribPointer(mVtxLink.Clr, 4, GL_UNSIGNED_SHORT, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxSkinShort, clr)));
			}
			if (mVtxLink.Tex >= 0) {
				glEnableVertexAttribArray(mVtxLink.Tex);
				glVertexAttribPointer(mVtxLink.Tex, 2, GL_SHORT, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxSkinShort, tex)));
			}
			if (mVtxLink.Jnt >= 0) {
				glEnableVertexAttribArray(mVtxLink.Jnt);
				glVertexAttribPointer(mVtxLink.Jnt, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxSkinShort, jnt)));
			}
			break;

		case VtxFmt_rigid1:
			if (mVtxLink.Pos >= 0) {
				glEnableVertexAttribArray(mVtxLink.Pos);
				glVertexAttribPointer(mVtxLink.Pos, 3, GL_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxRigidShort, pos)));
			} else {
				return;
			}
			if (mVtxLink.Oct >= 0) {
				glEnableVertexAttribArray(mVtxLink.Oct);
				glVertexAttribPointer(mVtxLink.Oct, 2, GL_SHORT, GL_TRUE, stride, (const void*)(top + offsetof(sxModelData::VtxRigidShort, oct)));
			}
			if (mVtxLink.Clr >= 0) {
				glEnableVertexAttribArray(mVtxLink.Clr);
				glVertexAttribPointer(mVtxLink.Clr, 4, GL_UNSIGNED_SHORT, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxRigidShort, clr)));
			}
			if (mVtxLink.Tex >= 0) {
				glEnableVertexAttribArray(mVtxLink.Tex);
				glVertexAttribPointer(mVtxLink.Tex, 2, GL_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxModelData::VtxRigidShort, tex)));
			}
			break;

		case VtxFmt_prim:
			if (mVtxLink.Pos >= 0) {
				glEnableVertexAttribArray(mVtxLink.Pos);
				glVertexAttribPointer(mVtxLink.Pos, 4, GL_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxPrimVtx, pos)));
			} else {
				return;
			}
			if (mVtxLink.Clr >= 0) {
				glEnableVertexAttribArray(mVtxLink.Clr);
				glVertexAttribPointer(mVtxLink.Clr, 4, GL_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxPrimVtx, clr)));
			}
			if (mVtxLink.Tex >= 0) {
				glEnableVertexAttribArray(mVtxLink.Tex);
				glVertexAttribPointer(mVtxLink.Tex, 4, GL_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxPrimVtx, tex)));
			}
			if (mVtxLink.Prm >= 0) {
				glEnableVertexAttribArray(mVtxLink.Prm);
				glVertexAttribPointer(mVtxLink.Prm, 4, GL_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(sxPrimVtx, prm)));
			}
			break;

		case VtxFmt_quad:
			if (mVtxLink.Id >= 0) {
				glEnableVertexAttribArray(mVtxLink.Id);
				glVertexAttribPointer(mVtxLink.Id, 1, GL_FLOAT, GL_FALSE, stride, (const void*)(top));
			}
			break;

		case VtxFmt_font:
			if (mVtxLink.Pos >= 0) {
				glEnableVertexAttribArray(mVtxLink.Pos);
				glVertexAttribPointer(mVtxLink.Pos, 2, GL_FLOAT, GL_FALSE, stride, (const void*)(top));
			}
			break;

		default:
			break;
	}
}

#define GPU_SHADER(_name, _kind) static GLuint s_sdr_##_name##_##_kind = 0;
#include "ogl/shaders.inc"
#undef GPU_SHADER

#define GPU_PROG(_vert, _frag) static GPUProg s_prg_##_vert##_##_frag = {};
#include "ogl/progs.inc"
#undef GPU_PROG


static void prepare_texture(sxTextureData* pTex) {
	if (!pTex) return;
	GLuint* pHandle = pTex->get_gpu_wk<GLuint>();
	if (!pHandle) return;
	if (*pHandle) return;
	glGenTextures(1, pHandle);
	if (*pHandle) {
		glBindTexture(GL_TEXTURE_2D, *pHandle);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pTex->mWidth, pTex->mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pTex->get_data_ptr());
		bool mipmapEnabled = pTex->mipmap_enabled();
		if (s_useMipmaps && mipmapEnabled) {
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		def_tex_params(mipmapEnabled, pTex->lod_bias_enabled());
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

static void release_texture(sxTextureData* pTex) {
	if (!pTex) return;
	GLuint* pHandle = pTex->get_gpu_wk<GLuint>();
	if (pHandle) {
		if (*pHandle) {
			glDeleteTextures(1, pHandle);
		}
		*pHandle = 0;
	}
}

static GLuint get_tex_handle(sxTextureData* pTex) {
	if (!pTex) return 0;
	prepare_texture(pTex);
	GLuint* pHandle = pTex->get_gpu_wk<GLuint>();
	return pHandle ? *pHandle : 0;
}

static void prepare_model(sxModelData* pMdl) {
	if (!pMdl) return;
	GLuint* pBufIds = pMdl->get_gpu_wk<GLuint>();
	GLuint* pBufVB = &pBufIds[0];
	GLuint* pBufIB16 = &pBufIds[1];
	GLuint* pBufIB32 = &pBufIds[2];
	if (*pBufVB && (*pBufIB16 || *pBufIB32)) {
		return;
	}
	if (pMdl->mSknNum > MAX_JNT) {
		const char* pMdlName = pMdl->get_name();
		nxCore::dbg_msg("Model \"%s\": too many skin nodes (%d).\n", pMdlName, pMdl->mSknNum);
	}
	size_t vsize = pMdl->get_vtx_size();
	if (!(*pBufVB) && vsize > 0) {
		const void* pPntData = pMdl->get_pnt_data_top();
		if (pPntData) {
			glGenBuffers(1, pBufVB);
			if (*pBufVB) {
				glBindBuffer(GL_ARRAY_BUFFER, *pBufVB);
				glBufferData(GL_ARRAY_BUFFER, pMdl->mPntNum * vsize, pPntData, GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
		}
	}
	if (!(*pBufIB16) && pMdl->mIdx16Num > 0) {
		const uint16_t* pIdxData = pMdl->get_idx16_top();
		if (pIdxData) {
			glGenBuffers(1, pBufIB16);
			if (*pBufIB16) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *pBufIB16);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, pMdl->mIdx16Num * sizeof(uint16_t), pIdxData, GL_STATIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
		}
	}
	if (!(*pBufIB32) && pMdl->mIdx32Num > 0) {
		const uint32_t* pIdxData = pMdl->get_idx32_top();
		if (pIdxData) {
			glGenBuffers(1, pBufIB32);
			if (*pBufIB32) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *pBufIB32);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, pMdl->mIdx32Num * sizeof(uint32_t), pIdxData, GL_STATIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
		}
	}
}

static void release_model(sxModelData* pMdl) {
	if (!pMdl) return;
	GLuint* pBufIds = pMdl->get_gpu_wk<GLuint>();
	GLuint* pBufVB = &pBufIds[0];
	GLuint* pBufIB16 = &pBufIds[1];
	GLuint* pBufIB32 = &pBufIds[2];
	if (*pBufVB) {
		glDeleteBuffers(1, pBufVB);
		*pBufVB = 0;
	}
	if (*pBufIB16) {
		glDeleteBuffers(1, pBufIB16);
		*pBufIB16 = 0;
	}
	if (*pBufIB32) {
		glDeleteBuffers(1, pBufIB32);
		*pBufIB32 = 0;
	}
	pMdl->clear_tex_wk();
}

static void batch_draw_exec(const sxModelData* pMdl, int ibat, int baseVtx = 0) {
	if (!pMdl) return;
	const sxModelData::Batch* pBat = pMdl->get_batch_ptr(ibat);
	if (!pBat) return;
	const GLuint* pBufIds = pMdl->get_gpu_wk<GLuint>();
	GLuint bufVB = pBufIds[0];
	GLuint bufIB16 = pBufIds[1];
	GLuint bufIB32 = pBufIds[2];
	if (!bufVB) return;
	intptr_t org = 0;
	GLenum typ = GL_NONE;
	if (pBat->is_idx16()) {
		if (!bufIB16) return;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIB16);
		org = pBat->mIdxOrg * sizeof(uint16_t);
		typ = GL_UNSIGNED_SHORT;
	} else {
		if (!bufIB32) return;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIB32);
		org = pBat->mIdxOrg * sizeof(uint32_t);
		typ = GL_UNSIGNED_INT;
	}
	if (baseVtx > 0) {
		OGLSys::draw_tris_base_vtx(pBat->mTriNum, typ, org, baseVtx);
	} else {
		glDrawElements(GL_TRIANGLES, pBat->mTriNum * 3, typ, (const void*)org);
	}
}

static bool s_nowSemi = false;

static void gl_opaq() {
	glDisable(GL_BLEND);
}

static void gl_semi() {
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void set_opaq() {
#if DRW_CACHE_BLEND
	if (s_nowSemi) {
		gl_opaq();
		s_nowSemi = false;
	}
#else
	gl_opaq();
#endif
}

static void set_semi() {
#if DRW_CACHE_BLEND
	if (!s_nowSemi) {
		gl_semi();
		s_nowSemi = true;
	}
#else
	gl_semi();
#endif
}

static bool s_nowDblSided = false;

static void gl_dbl_sided() {
	glDisable(GL_CULL_FACE);
}

static void gl_face_cull() {
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
}

static void set_dbl_sided() {
#if DRW_CACHE_DSIDED
	if (!s_nowDblSided) {
		gl_dbl_sided();
		s_nowDblSided = true;
	}
#else
	gl_dbl_sided();
#endif
}

static void set_face_cull() {
#if DRW_CACHE_DSIDED
	if (s_nowDblSided) {
		gl_face_cull();
		s_nowDblSided = false;
	}
#else
	gl_face_cull();
#endif
	}


static void reset_fb_render_states() {
	gl_opaq();
	s_nowSemi = false;
	gl_face_cull();
	s_nowDblSided = false;
}

static void set_def_framebuf(const bool useDepth = true) {
	if (s_frameBufMode != 0) {
		int w = OGLSys::get_width();
		int h = OGLSys::get_height();
		OGLSys::bind_def_framebuf();
		glViewport(0, 0, w, h);
		glScissor(0, 0, w, h);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		reset_fb_render_states();
		s_frameBufMode = 0;
	}
}

static void set_shadow_framebuf() {
	if (s_shadowFBO) {
		if (s_frameBufMode != 1) {
			glBindFramebuffer(GL_FRAMEBUFFER, s_shadowFBO);
			glViewport(0, 0, s_shadowSize, s_shadowSize);
			glScissor(0, 0, s_shadowSize, s_shadowSize);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			if (s_shadowDepthBuf && s_shadowCastDepthTest) {
				glDepthMask(GL_TRUE);
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LEQUAL);
			} else {
				glDepthMask(GL_FALSE);
				glDisable(GL_DEPTH_TEST);
			}
			reset_fb_render_states();
			s_frameBufMode = 1;
		}
	}
}

static void set_screen_framebuf() {
	if (s_frameBufMode != 2) {
		int w = OGLSys::get_width();
		int h = OGLSys::get_height();
		OGLSys::bind_def_framebuf();
		glViewport(0, 0, w, h);
		glScissor(0, 0, w, h);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
		reset_fb_render_states();
		s_frameBufMode = 2;
	}
}


static void init(int shadowSize, cxResourceManager* pRsrcMgr, Draw::Font* pFont) {
	if (s_drwInitFlg) return;

	if (!pRsrcMgr) return;
	s_pRsrcMgr = pRsrcMgr;
	cxResourceManager::GfxIfc rsrcGfxIfc;
	rsrcGfxIfc.reset();
	rsrcGfxIfc.prepareTexture = prepare_texture;
	rsrcGfxIfc.releaseTexture = release_texture;
	rsrcGfxIfc.prepareModel = prepare_model;
	rsrcGfxIfc.releaseModel = release_model;
	s_pRsrcMgr->set_gfx_ifc(rsrcGfxIfc);

	s_glslEcho = nxApp::get_bool_opt("glsl_echo", false);
	s_pGLSLBinSavePath = nxApp::get_opt("glsl_bin_save");
	s_pGLSLBinLoadPath = s_pGLSLBinSavePath ? nullptr : nxApp::get_opt("glsl_bin_load");

	s_glslNoBaseTex = (s_pGLSLBinSavePath || s_pGLSLBinLoadPath) ? false : nxApp::get_bool_opt("nobasetex", false);
	s_glslNoFog = (s_pGLSLBinSavePath || s_pGLSLBinLoadPath) ? false : nxApp::get_bool_opt("nofog", false);
	s_glslNoCC = (s_pGLSLBinSavePath || s_pGLSLBinLoadPath) ? false : nxApp::get_bool_opt("nocc", false);

	s_pAltGLSL = nxApp::get_opt("alt_glsl");

	s_useVtxLighting = nxApp::get_bool_opt("vl", false);

	if (!s_pGLSLBinLoadPath) {
#define GPU_SHADER(_name, _kind) s_sdr_##_name##_##_kind = load_shader(#_name "." #_kind);
#include "ogl/shaders.inc"
#undef GPU_SHADER
	}

	int prgCnt = 0;
	int prgOK = 0;
	if (s_glslEcho) {
		nxCore::dbg_msg("Initializing GPU progs");
	}
	double prgT0 = nxSys::time_micros();
	int linkMode = s_pGLSLBinLoadPath ? 1 : nxApp::get_int_opt("glsl_link_mode", 0);
	if (linkMode == 1) {
#		define GPU_PROG(_vert_name, _frag_name) s_prg_##_vert_name##_##_frag_name.init(VtxFmt_##_vert_name, s_sdr_##_vert_name##_vert, s_sdr_##_frag_name##_frag, #_vert_name, #_frag_name); ++prgCnt; if (s_prg_##_vert_name##_##_frag_name.is_valid()) {++prgOK; if (s_glslEcho) { nxCore::dbg_msg("."); } } else { nxCore::dbg_msg("GPUProg init error: %s + %s\n", #_vert_name, #_frag_name); }
#		include "ogl/progs.inc"
#		undef GPU_PROG
	} else {
#		define GPU_PROG(_vert_name, _frag_name) ++prgCnt;
#		include "ogl/progs.inc"
#		undef GPU_PROG
		if (prgCnt > 0) {
			GPUProg** ppProg = (GPUProg**)nxCore::mem_alloc(sizeof(GPUProg*)*prgCnt, "GPUProgsList");
			if (ppProg) {
				int iprg = 0;
#				define GPU_PROG(_vert_name, _frag_name) ppProg[iprg] = &s_prg_##_vert_name##_##_frag_name; ppProg[iprg]->mVtxFmt = VtxFmt_##_vert_name; ppProg[iprg]->mVertSID = s_sdr_##_vert_name##_vert; ppProg[iprg]->mFragSID = s_sdr_##_frag_name##_frag; ppProg[iprg]->mpVertName = #_vert_name; ppProg[iprg]->mpFragName = #_frag_name; ++iprg;
#				include "ogl/progs.inc"
#				undef GPU_PROG
				for (iprg = 0; iprg < prgCnt; ++iprg) {
					GPUProg* pProg = ppProg[iprg];
					pProg->mProgId = glCreateProgram();
					if (pProg->mProgId) {
						glAttachShader(pProg->mProgId, pProg->mVertSID);
						glAttachShader(pProg->mProgId, pProg->mFragSID);
						OGLSys::link_prog_id_nock(pProg->mProgId);
						if (s_glslEcho) {
							nxCore::dbg_msg(".");
						}
					}
				}
				for (iprg = 0; iprg < prgCnt; ++iprg) {
					GPUProg* pProg = ppProg[iprg];
					if (pProg->mProgId) {
						if (OGLSys::ck_link_status(pProg->mProgId)) {
							pProg->prepare();
							pProg->save_bin();
						} else {
							glDetachShader(pProg->mProgId, pProg->mVertSID);
							glDetachShader(pProg->mProgId, pProg->mFragSID);
							glDeleteProgram(pProg->mProgId);
							pProg->mProgId = 0;
						}
					}
					if (pProg->is_valid()) {
						++prgOK;
					} else {
						nxCore::dbg_msg("GPUProg init error: %s + %s\n", pProg->mpVertName, pProg->mpFragName);
					}
				}
				nxCore::mem_free(ppProg);
			}
		}
	}
	if (s_glslEcho) {
		nxCore::dbg_msg("\n");
	}
	double prgDT = nxSys::time_micros() - prgT0;

	nxCore::dbg_msg("GPU progs: %d/%d\n", prgOK, prgCnt);
	if (s_glslEcho) {
		nxCore::dbg_msg("GPU progs init time: %.3f seconds\n", prgDT / 1.0e6);
	}

	glGenBuffers(1, &s_quadVBO);
	glGenBuffers(1, &s_quadIBO);
	if (s_quadVBO && s_quadIBO) {
		static GLfloat quadVBData[] = { 0.0f, 1.0f, 2.0f, 3.0f };
		static uint16_t quadIBData[] = { 0, 1, 3, 2 };
		glBindBuffer(GL_ARRAY_BUFFER, s_quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVBData), quadVBData, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_quadIBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIBData), quadIBData, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	if (shadowSize > 0) {
		s_shadowSize = shadowSize;
		glGenFramebuffers(1, &s_shadowFBO);
		if (s_shadowFBO) {
			glGenTextures(1, &s_shadowTex);
			if (s_shadowTex) {
				glBindFramebuffer(GL_FRAMEBUFFER, s_shadowFBO);
				glBindTexture(GL_TEXTURE_2D, s_shadowTex);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s_shadowSize, s_shadowSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_shadowTex, 0);
				glBindTexture(GL_TEXTURE_2D, 0);
				glGenRenderbuffers(1, &s_shadowDepthBuf);
				if (s_shadowDepthBuf) {
					glBindRenderbuffer(GL_RENDERBUFFER, s_shadowDepthBuf);
					glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, s_shadowSize, s_shadowSize);
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_shadowDepthBuf);
				}
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
					glDeleteTextures(1, &s_shadowTex);
					s_shadowTex = 0;
					OGLSys::bind_def_framebuf();
					if (s_shadowDepthBuf) {
						glDeleteRenderbuffers(1, &s_shadowDepthBuf);
						s_shadowDepthBuf = 0;
					}
					glDeleteFramebuffers(1, &s_shadowFBO);
					s_shadowFBO = 0;
				}
			} else {
				glDeleteFramebuffers(1, &s_shadowFBO);
				s_shadowFBO = 0;
			}
		}
	}
	s_shadowCastDepthTest = true;
	s_frameBufMode = -1;
	set_def_framebuf();

	s_pFont = pFont;
	if (pFont) {
		glGenBuffers(1, &s_fontVBO);
		glGenBuffers(1, &s_fontIBO);
		if (s_fontVBO && s_fontIBO) {
			glBindBuffer(GL_ARRAY_BUFFER, s_fontVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(xt_float2) * pFont->numPnts, pFont->pPnts, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_fontIBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * pFont->numTris * 3, pFont->pTris, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	}

	s_pNowProg = nullptr;

	s_drwInitFlg = true;
}

static void reset() {
	if (!s_drwInitFlg) return;

	OGLSys::bind_def_framebuf();

	if (s_shadowDepthBuf) {
		glDeleteRenderbuffers(1, &s_shadowDepthBuf);
		s_shadowDepthBuf = 0;
	}
	if (s_shadowTex) {
		glDeleteTextures(1, &s_shadowTex);
		s_shadowTex = 0;
	}
	if (s_shadowFBO) {
		glDeleteFramebuffers(1, &s_shadowFBO);
		s_shadowFBO = 0;
	}

	if (s_primVBO) {
		glDeleteBuffers(1, &s_primVBO);
		s_primVBO = 0;
		s_maxPrimVtx = 0;
	}

	if (s_primIBO) {
		glDeleteBuffers(1, &s_primIBO);
		s_primIBO = 0;
		s_maxPrimIdx = 0;
	}

	if (s_quadIBO) {
		glDeleteBuffers(1, &s_quadIBO);
		s_quadIBO = 0;
	}
	if (s_quadVBO) {
		glDeleteBuffers(1, &s_quadVBO);
		s_quadVBO = 0;
	}

	if (s_fontIBO) {
		glDeleteBuffers(1, &s_fontIBO);
		s_fontIBO = 0;
	}
	if (s_fontVBO) {
		glDeleteBuffers(1, &s_fontVBO);
		s_fontVBO = 0;
	}

	s_pFont = nullptr;

#define GPU_PROG(_vert_name, _frag_name) s_prg_##_vert_name##_##_frag_name.reset();
#include "ogl/progs.inc"
#undef GPU_PROG

#define GPU_SHADER(_name, _kind) glDeleteShader(s_sdr_##_name##_##_kind); s_sdr_##_name##_##_kind = 0;
#include "ogl/shaders.inc"
#undef GPU_SHADER

	s_pNowProg = nullptr;

	s_drwInitFlg = false;
}

static int get_screen_width() {
	return OGLSys::get_width();
}

static int get_screen_height() {
	return OGLSys::get_height();
}

static cxMtx get_shadow_bias_mtx() {
	static const float bias[4 * 4] = {
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	};
	return nxMtx::from_mem(bias);
}


static void clear_shadow() {
	if (s_shadowFBO && s_frameBufMode == 1) {
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
}

static void begin(const cxColor& clearColor) {
	s_frameBufMode = -1;
	set_shadow_framebuf();
	clear_shadow();
	set_def_framebuf();
	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	s_batDrwCnt = 0;
	s_shadowCastCnt = 0;
}

static void end() {
	OGLSys::swap();
}

static GPUProg* prog_sel(const cxModelWork* pWk, const int ibat, const sxModelData::Material* pMtl, const Draw::Mode mode, const Draw::Context* pCtx) {
	GPUProg* pProg = nullptr;
	sxModelData* pMdl = pWk->mpData;
	if (mode == Draw::DRWMODE_SHADOW_CAST) {
		if (s_shadowFBO && pMtl->mFlags.shadowCast) {
			if (pMdl->half_encoding()) {
				if (pMdl->has_skin()) {
					if (pMtl->mFlags.alpha) {
						pProg = &s_prg_skin0_cast_semi;
					} else {
						pProg = &s_prg_skin0_cast_opaq;
					}
				} else {
					if (pMtl->mFlags.alpha) {
						pProg = &s_prg_rigid0_cast_semi;
					} else {
						pProg = &s_prg_rigid0_cast_opaq;
					}
				}
			} else {
				if (pMdl->has_skin()) {
					if (pMtl->mFlags.alpha) {
						pProg = &s_prg_skin1_cast_semi;
					} else {
						pProg = &s_prg_skin1_cast_opaq;
					}
				} else {
					if (pMtl->mFlags.alpha) {
						pProg = &s_prg_rigid1_cast_semi;
					} else {
						pProg = &s_prg_rigid1_cast_opaq;
					}
				}
			}
		}
	} else {
		bool recvFlg = pMtl->mFlags.shadowRecv && (s_shadowFBO != 0) && (s_shadowCastCnt > 0) && (pCtx->shadow.get_density() > 0.0f);
		bool specFlg = pCtx->glb.useSpec && pCtx->spec.mEnabled && pMtl->mFlags.baseMapSpecAlpha;
		if (recvFlg) {
			if (pWk->mBoundsValid && pWk->mpBatBBoxes) {
				recvFlg = pCtx->ck_bbox_shadow_receive(pWk->mpBatBBoxes[ibat]);
			}
		}
		bool bumpFlg = false;
		if (!s_useVtxLighting && pCtx->glb.useBump && OGLSys::ext_ck_derivatives()) {
			if (pMtl->mBumpScale > 0.0f) {
				int tid = pMtl->mBumpTexId;
				if (tid >= 0) {
					sxModelData::TexInfo* pTexInfo = pMdl->get_tex_info(tid);
					GLuint* pTexHandle = pTexInfo->get_wk<GLuint>();
					if (*pTexHandle == 0) {
						const char* pTexName = pMdl->get_tex_name(tid);
						sxTextureData* pTex = pWk->find_texture(s_pRsrcMgr, pTexName);
						*pTexHandle = get_tex_handle(pTex);
					}
					if (*pTexHandle) {
						bumpFlg = true;
					}
				}
			}
		}
		if ((!pMtl->mFlags.alpha) || (mode == Draw::DRWMODE_DISCARD && !pMtl->mFlags.forceBlend)) {
			// opaq or discard
			if (pMdl->half_encoding()) {
				// fmt0: half
				if (s_useVtxLighting) {
					// fmt0 vl
					if (pMdl->has_skin()) {
						if (pMtl->mFlags.alpha) {
							pProg = recvFlg ? &s_prg_skin0_vl_unlit_discard_sdw : &s_prg_skin0_vl_unlit_discard;
						} else {
							pProg = recvFlg ? &s_prg_skin0_vl_unlit_opaq_sdw : &s_prg_skin0_vl_unlit_opaq;
						}
					} else {
						if (pMtl->mFlags.alpha) {
							pProg = recvFlg ? &s_prg_rigid0_vl_unlit_discard_sdw : &s_prg_rigid0_vl_unlit_discard;
						} else {
							pProg = recvFlg ? &s_prg_rigid0_vl_unlit_opaq_sdw : &s_prg_rigid0_vl_unlit_opaq;
						}
					}
				} else if (bumpFlg) {
					// fmt0 bump
					if (specFlg) {
						if (pMdl->has_skin()) {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_skin0_hemi_spec_bump_discard_sdw : &s_prg_skin0_hemi_spec_bump_discard;
							} else {
								pProg = recvFlg ? &s_prg_skin0_hemi_spec_bump_opaq_sdw : &s_prg_skin0_hemi_spec_bump_opaq;
							}
						} else {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_rigid0_hemi_spec_bump_discard_sdw : &s_prg_rigid0_hemi_spec_bump_discard;
							} else {
								pProg = recvFlg ? &s_prg_rigid0_hemi_spec_bump_opaq_sdw : &s_prg_rigid0_hemi_spec_bump_opaq;
							}
						}
					} else {
						if (pMdl->has_skin()) {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_skin0_hemi_bump_discard_sdw : &s_prg_skin0_hemi_bump_discard;
							} else {
								pProg = recvFlg ? &s_prg_skin0_hemi_bump_opaq_sdw : &s_prg_skin0_hemi_bump_opaq;
							}
						} else {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_rigid0_hemi_bump_discard_sdw : &s_prg_rigid0_hemi_bump_discard;
							} else {
								pProg = recvFlg ? &s_prg_rigid0_hemi_bump_opaq_sdw : &s_prg_rigid0_hemi_bump_opaq;
							}
						}
					}
				} else {
					// fmt0 !bump
					if (specFlg) {
						if (pMdl->has_skin()) {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_skin0_hemi_spec_discard_sdw : &s_prg_skin0_hemi_spec_discard;
							} else {
								pProg = recvFlg ? &s_prg_skin0_hemi_spec_opaq_sdw : &s_prg_skin0_hemi_spec_opaq;
							}
						} else {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_rigid0_hemi_spec_discard_sdw : &s_prg_rigid0_hemi_spec_discard;
							} else {
								pProg = recvFlg ? &s_prg_rigid0_hemi_spec_opaq_sdw : &s_prg_rigid0_hemi_spec_opaq;
							}
						}
					} else {
						if (pMdl->has_skin()) {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_skin0_hemi_discard_sdw : &s_prg_skin0_hemi_discard;
							} else {
								pProg = recvFlg ? &s_prg_skin0_hemi_opaq_sdw : &s_prg_skin0_hemi_opaq;
							}
						} else {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_rigid0_hemi_discard_sdw : &s_prg_rigid0_hemi_discard;
							} else {
								pProg = recvFlg ? &s_prg_rigid0_hemi_opaq_sdw : &s_prg_rigid0_hemi_opaq;
							}
						}
					}
				}
			} else {
				// fmt1: short
				if (s_useVtxLighting) {
					// fmt1 vl
					if (pMdl->has_skin()) {
						if (pMtl->mFlags.alpha) {
							pProg = recvFlg ? &s_prg_skin1_vl_unlit_discard_sdw : &s_prg_skin1_vl_unlit_discard;
						} else {
							pProg = recvFlg ? &s_prg_skin1_vl_unlit_opaq_sdw : &s_prg_skin1_vl_unlit_opaq;
						}
					} else {
						if (pMtl->mFlags.alpha) {
							pProg = recvFlg ? &s_prg_rigid1_vl_unlit_discard_sdw : &s_prg_rigid1_vl_unlit_discard;
						} else {
							pProg = recvFlg ? &s_prg_rigid1_vl_unlit_opaq_sdw : &s_prg_rigid1_vl_unlit_opaq;
						}
					}
				} else if (bumpFlg) {
					// fmt1 bump
					if (specFlg) {
						if (pMdl->has_skin()) {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_skin1_hemi_spec_bump_discard_sdw : &s_prg_skin1_hemi_spec_bump_discard;
							} else {
								pProg = recvFlg ? &s_prg_skin1_hemi_spec_bump_opaq_sdw : &s_prg_skin1_hemi_spec_bump_opaq;
							}
						} else {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_rigid1_hemi_spec_bump_discard_sdw : &s_prg_rigid1_hemi_spec_bump_discard;
							} else {
								pProg = recvFlg ? &s_prg_rigid1_hemi_spec_bump_opaq_sdw : &s_prg_rigid1_hemi_spec_bump_opaq;
							}
						}
					} else {
						if (pMdl->has_skin()) {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_skin1_hemi_bump_discard_sdw : &s_prg_skin1_hemi_bump_discard;
							} else {
								pProg = recvFlg ? &s_prg_skin1_hemi_bump_opaq_sdw : &s_prg_skin1_hemi_bump_opaq;
							}
						} else {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_rigid1_hemi_bump_discard_sdw : &s_prg_rigid1_hemi_bump_discard;
							} else {
								pProg = recvFlg ? &s_prg_rigid1_hemi_bump_opaq_sdw : &s_prg_rigid1_hemi_bump_opaq;
							}
						}
					}
				} else {
					// fmt1 !bump
					if (specFlg) {
						if (pMdl->has_skin()) {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_skin1_hemi_spec_discard_sdw : &s_prg_skin1_hemi_spec_discard;
							} else {
								pProg = recvFlg ? &s_prg_skin1_hemi_spec_opaq_sdw : &s_prg_skin1_hemi_spec_opaq;
							}
						} else {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_rigid1_hemi_spec_discard_sdw : &s_prg_rigid1_hemi_spec_discard;
							} else {
								pProg = recvFlg ? &s_prg_rigid1_hemi_spec_opaq_sdw : &s_prg_rigid1_hemi_spec_opaq;
							}
						}
					} else {
						if (pMdl->has_skin()) {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_skin1_hemi_discard_sdw : &s_prg_skin1_hemi_discard;
							} else {
								pProg = recvFlg ? &s_prg_skin1_hemi_opaq_sdw : &s_prg_skin1_hemi_opaq;
							}
						} else {
							if (pMtl->mFlags.alpha) {
								pProg = recvFlg ? &s_prg_rigid1_hemi_discard_sdw : &s_prg_rigid1_hemi_discard;
							} else {
								pProg = recvFlg ? &s_prg_rigid1_hemi_opaq_sdw : &s_prg_rigid1_hemi_opaq;
							}
						}
					}
				}
			}
		} else {
			// blending
			if (s_useVtxLighting) {
				// vertex lighting
				if (pMdl->half_encoding()) {
					// fmt0: half
					if (pMtl->is_cutout()) {
						if (pMdl->has_skin()) {
							pProg = recvFlg ? &s_prg_skin0_vl_unlit_limit_sdw : &s_prg_skin0_vl_unlit_limit;
						} else {
							pProg = recvFlg ? &s_prg_rigid0_vl_unlit_limit_sdw : &s_prg_rigid0_vl_unlit_limit;
						}
					} else {
						if (pMdl->has_skin()) {
							pProg = recvFlg ? &s_prg_skin0_vl_unlit_semi_sdw : &s_prg_skin0_vl_unlit_semi;
						} else {
							pProg = recvFlg ? &s_prg_rigid0_vl_unlit_semi_sdw : &s_prg_rigid0_vl_unlit_semi;
						}
					}
				} else {
					// fmt1: short
					if (pMtl->is_cutout()) {
						if (pMdl->has_skin()) {
							pProg = recvFlg ? &s_prg_skin1_vl_unlit_limit_sdw : &s_prg_skin1_vl_unlit_limit;
						} else {
							pProg = recvFlg ? &s_prg_rigid1_vl_unlit_limit_sdw : &s_prg_rigid1_vl_unlit_limit;
						}
					} else {
						if (pMdl->has_skin()) {
							pProg = recvFlg ? &s_prg_skin1_vl_unlit_semi_sdw : &s_prg_skin1_vl_unlit_semi;
						} else {
							pProg = recvFlg ? &s_prg_rigid1_vl_unlit_semi_sdw : &s_prg_rigid1_vl_unlit_semi;
						}
					}
				}
			} else {
				if (pMdl->half_encoding()) {
					// fmt0: half
					if (specFlg) {
						if (pMtl->is_cutout()) {
							if (pMdl->has_skin()) {
								pProg = recvFlg ? &s_prg_skin0_hemi_spec_limit_sdw : &s_prg_skin0_hemi_spec_limit;
							} else {
								pProg = recvFlg ? &s_prg_rigid0_hemi_spec_limit_sdw : &s_prg_rigid0_hemi_spec_limit;
							}
						} else {
							if (pMdl->has_skin()) {
								pProg = recvFlg ? &s_prg_skin0_hemi_spec_semi_sdw : &s_prg_skin0_hemi_spec_semi;
							} else {
								pProg = recvFlg ? &s_prg_rigid0_hemi_spec_semi_sdw : &s_prg_rigid0_hemi_spec_semi;
							}
						}
					} else {
						if (pMtl->is_cutout()) {
							if (pMdl->has_skin()) {
								pProg = recvFlg ? &s_prg_skin0_hemi_limit_sdw : &s_prg_skin0_hemi_limit;
							} else {
								pProg = recvFlg ? &s_prg_rigid0_hemi_limit_sdw : &s_prg_rigid0_hemi_limit;
							}
						} else {
							if (pMdl->has_skin()) {
								pProg = recvFlg ? &s_prg_skin0_hemi_semi_sdw : &s_prg_skin0_hemi_semi;
							} else {
								pProg = recvFlg ? &s_prg_rigid0_hemi_semi_sdw : &s_prg_rigid0_hemi_semi;
							}
						}
					}
				} else {
					// fmt1: short
					if (specFlg) {
						if (pMtl->is_cutout()) {
							if (pMdl->has_skin()) {
								pProg = recvFlg ? &s_prg_skin1_hemi_spec_limit_sdw : &s_prg_skin1_hemi_spec_limit;
							} else {
								pProg = recvFlg ? &s_prg_rigid1_hemi_spec_limit_sdw : &s_prg_rigid1_hemi_spec_limit;
							}
						} else {
							if (pMdl->has_skin()) {
								pProg = recvFlg ? &s_prg_skin1_hemi_spec_semi_sdw : &s_prg_skin1_hemi_spec_semi;
							} else {
								pProg = recvFlg ? &s_prg_rigid1_hemi_spec_semi_sdw : &s_prg_rigid1_hemi_spec_semi;
							}
						}
					} else {
						if (pMtl->is_cutout()) {
							if (pMdl->has_skin()) {
								pProg = recvFlg ? &s_prg_skin1_hemi_limit_sdw : &s_prg_skin1_hemi_limit;
							} else {
								pProg = recvFlg ? &s_prg_rigid1_hemi_limit_sdw : &s_prg_rigid1_hemi_limit;
							}
						} else {
							if (pMdl->has_skin()) {
								pProg = recvFlg ? &s_prg_skin1_hemi_semi_sdw : &s_prg_skin1_hemi_semi;
							} else {
								pProg = recvFlg ? &s_prg_rigid1_hemi_semi_sdw : &s_prg_rigid1_hemi_semi;
							}
						}
					}
				}
			}
		}
	}
	return pProg;
}


#define NFLT_JMTX (MAX_JNT_PER_BATCH * 3 * 4)
#define NFLT_JMAP (MAX_JNT)

#define HAS_PARAM(_name) (pProg->mParamLink._name >= 0)

static void batch(cxModelWork* pWk, const int ibat, const Draw::Mode mode, const Draw::Context* pCtx) {
	float ftmp[NFLT_JMTX > NFLT_JMAP ? NFLT_JMTX : NFLT_JMAP];

	if (!pCtx) return;

	if (!pWk) return;
	sxModelData* pMdl = pWk->mpData;
	if (!pMdl) return;
	if (!pMdl->ck_batch_id(ibat)) return;

	Draw::MdlParam* pParam = (Draw::MdlParam*)pWk->mpParamMem;

	bool isShadowCast = (mode == Draw::DRWMODE_SHADOW_CAST);
	bool isDiscard = (mode == Draw::DRWMODE_DISCARD);

	const sxModelData::Batch* pBat = pMdl->get_batch_ptr(ibat);
	const sxModelData::Material* pMtl = pMdl->get_material(pBat->mMtlId);
	if (pWk->mVariation != 0) {
		if (pMdl->mtl_has_swaps(pBat->mMtlId)) {
			const sxModelData::Material* pSwapMtl = pMdl->get_swap_material(pBat->mMtlId, pWk->mVariation);
			if (pSwapMtl) {
				pMtl = pSwapMtl;
			}
		}
	}
	if (!pMtl) return;

	GPUProg* pProg = prog_sel(pWk, ibat, pMtl, mode, pCtx);
	if (!pProg) return;
	if (!pProg->is_valid()) return;

	if (isShadowCast) {
		set_shadow_framebuf();
		OGLSys::enable_msaa(false);
	} else {
		set_def_framebuf();
		OGLSys::enable_msaa(true);
	}

	prepare_model(pMdl);

	pProg->use();

	pProg->set_view_proj(isShadowCast ? pCtx->shadow.mViewProjMtx : pCtx->view.mViewProjMtx);
	pProg->set_view_pos(pCtx->view.mPos);

	if (HAS_PARAM(World)) {
		xt_xmtx wm;
		if (pWk->mpWorldXform) {
			wm = *pWk->mpWorldXform;
		} else {
			wm.identity();
		}
		pProg->set_world(wm);
	}

	if (HAS_PARAM(SkinMtx) && pWk->mpSkinXforms) {
		xt_xmtx* pSkin = (xt_xmtx*)ftmp;
		const int32_t* pJntLst = pMdl->get_batch_jnt_list(ibat);
		for (int i = 0; i < pBat->mJntNum; ++i) {
			pSkin[i] = pWk->mpSkinXforms[pJntLst[i]];
		}
		glUniform4fv(pProg->mParamLink.SkinMtx, JMTX_SIZE, (const GLfloat*)pSkin);
	}

	if (HAS_PARAM(SkinMap)) {
		const int32_t* pJntLst = pMdl->get_batch_jnt_list(ibat);
		if (pJntLst) {
			nxCore::mem_zero(ftmp, NFLT_JMAP * sizeof(float));
			int njnt = pBat->mJntNum;
			for (int i = 0; i < njnt; ++i) {
				ftmp[pJntLst[i]] = float(i);
			}
			for (int i = 0; i < NFLT_JMAP; ++i) {
				ftmp[i] *= 3.0f;
			}
#if DRW_LIMIT_JMAP
			int njmax = pJntLst[njnt - 1] + 1;
			int nv = (njmax >> 2) + ((njmax & 3) != 0 ? 1 : 0);
			glUniform4fv(pProg->mParamLink.SkinMap, nv, ftmp);
#else
			glUniform4fv(pProg->mParamLink.SkinMap, JMAP_SIZE, ftmp);
#endif
		}
	}

	if (HAS_PARAM(PosBase)) {
		cxVec posBase = pMdl->mBBox.get_min_pos();
		pProg->set_pos_base(posBase);
	}

	if (HAS_PARAM(PosScale)) {
		cxVec posScale = pMdl->mBBox.get_size_vec();
		pProg->set_pos_scale(posScale);
	}

	pProg->set_hemi_upper(pCtx->hemi.mUpper);
	pProg->set_hemi_lower(pCtx->hemi.mLower);
	pProg->set_hemi_up(pCtx->hemi.mUp);
	if (HAS_PARAM(HemiParam)) {
		xt_float3 hparam;
		hparam.set(pCtx->hemi.mExp, pCtx->hemi.mGain, 0.0f);
		pProg->set_hemi_param(hparam);
	}

	pProg->set_spec_light_dir(pCtx->spec.mDir);
	if (HAS_PARAM(SpecLightColor)) {
		xt_float4 sclr;
		nxCore::mem_copy(sclr, pCtx->spec.mClr, sizeof(xt_float3));
		sclr.w = pCtx->spec.mShadowing;
		pProg->set_spec_light_color(sclr);
	}

	pProg->set_vtx_hemi_upper(pCtx->hemi.mUpper);
	pProg->set_vtx_hemi_lower(pCtx->hemi.mLower);
	pProg->set_vtx_hemi_up(pCtx->hemi.mUp);
	if (HAS_PARAM(VtxHemiParam)) {
		xt_float3 hparam;
		hparam.set(pCtx->hemi.mExp, pCtx->hemi.mGain, 0.0f);
		pProg->set_vtx_hemi_param(hparam);
	}

	pProg->set_shadow_mtx(pCtx->shadow.mMtx);

	if (HAS_PARAM(ShadowSize)) {
		float ss = float(s_shadowSize);
		xt_float4 shadowSize;
		shadowSize.set(ss, ss, nxCalc::rcp0(ss), nxCalc::rcp0(ss));
		pProg->set_shadow_size(shadowSize);
	}

	if (HAS_PARAM(ShadowCtrl)) {
		xt_float4 shadowCtrl;

		float swght = pMtl->mShadowWght;
		if (pParam) {
			swght += pParam->shadowWeightBias;
		}
		if (swght > 0.0f) {
			swght = nxCalc::max(swght + pCtx->shadow.mWghtBias, 1.0f);
		}

		float soffs = pMtl->mShadowOffs + pCtx->shadow.mOffsBias;
		if (pParam) {
			soffs += pParam->shadowOffsBias;
		}

		float sdens = pMtl->mShadowDensity * pCtx->shadow.get_density();
		if (pParam) {
			sdens *= nxCalc::saturate(pParam->shadowDensScl);
		}

		shadowCtrl.set(soffs, swght, sdens, 0.0f);
		pProg->set_shadow_ctrl(shadowCtrl);
	}

	if (HAS_PARAM(ShadowFade)) {
		xt_float4 shadowFade;
		shadowFade.set(pCtx->shadow.mFadeStart, nxCalc::rcp0(pCtx->shadow.mFadeEnd - pCtx->shadow.mFadeStart), 0.0f, 0.0f);
		pProg->set_shadow_fade(shadowFade);
	}

	if (HAS_PARAM(BaseColor)) {
		xt_float3 baseClr = pMtl->mBaseColor;
		if (pParam) {
			for (int i = 0; i < 3; ++i) {
				baseClr[i] *= pParam->baseColorScl[i];
			}
		}
		pProg->set_base_color(baseClr);
	}

	pProg->set_spec_color(pMtl->mSpecColor);

	if (HAS_PARAM(SurfParam)) {
		xt_float4 sprm;
		sprm.set(pMtl->mRoughness, pMtl->mFresnel, 0.0f, 0.0f);
		pProg->set_surf_param(sprm);
	}

	if (HAS_PARAM(BumpParam)) {
		xt_float4 bprm;
		float sclT = pMtl->mFlags.flipTangent ? -1.0f : 1.0f;
		float sclB = -(pMtl->mFlags.flipBitangent ? -1.0f : 1.0f);
		bprm.set(pMtl->mBumpScale, sclT, sclB, 0.0f);
		pProg->set_bump_param(bprm);
	}

	if (HAS_PARAM(AlphaCtrl)) {
		float alphaLim = isShadowCast ? pMtl->mShadowAlphaLim : pMtl->mAlphaLim;
		if (isDiscard) {
			if (alphaLim <= 0.0f) {
				alphaLim = pMtl->mShadowAlphaLim;
			}
		}
		xt_float3 alphaCtrl;
		alphaCtrl.set(alphaLim, 0.0f, 0.0f);
		pProg->set_alpha_ctrl(alphaCtrl);
	}

	pProg->set_fog_color(pCtx->fog.mColor);
	pProg->set_fog_param(pCtx->fog.mParam);

	pProg->set_inv_white(pCtx->cc.mToneMap.get_inv_white());
	pProg->set_lclr_gain(pCtx->cc.mToneMap.mLinGain);
	pProg->set_lclr_bias(pCtx->cc.mToneMap.mLinBias);
	pProg->set_exposure(pCtx->cc.mExposure);
	pProg->set_inv_gamma(pCtx->cc.get_inv_gamma());

	if (pProg->mSmpLink.Base >= 0) {
		GLuint htex = 0;
		if (s_pRsrcMgr) {
			int tid = pMtl->mBaseTexId;
			if (tid >= 0) {
				sxModelData::TexInfo* pTexInfo = pMdl->get_tex_info(tid);
				GLuint* pTexHandle = pTexInfo->get_wk<GLuint>();
				if (*pTexHandle == 0) {
					const char* pTexName = pMdl->get_tex_name(tid);
					sxTextureData* pTex = pWk->find_texture(s_pRsrcMgr, pTexName);
					*pTexHandle = get_tex_handle(pTex);
				}
				htex = *pTexHandle;
			}
		}
		if (!htex) {
			htex = OGLSys::get_white_tex();
		}
		glActiveTexture(GL_TEXTURE0 + Draw::TEXUNIT_Base);
		glBindTexture(GL_TEXTURE_2D, htex);
	}

	if (pProg->mSmpLink.Bump >= 0 && s_pRsrcMgr) {
		int tid = pMtl->mBumpTexId;
		if (tid >= 0) {
			sxModelData::TexInfo* pTexInfo = pMdl->get_tex_info(tid);
			GLuint* pTexHandle = pTexInfo->get_wk<GLuint>();
			if (*pTexHandle == 0) {
				const char* pTexName = pMdl->get_tex_name(tid);
				sxTextureData* pTex = pWk->find_texture(s_pRsrcMgr, pTexName);
				*pTexHandle = get_tex_handle(pTex);
			}
			if (*pTexHandle) {
				glActiveTexture(GL_TEXTURE0 + Draw::TEXUNIT_Bump);
				glBindTexture(GL_TEXTURE_2D, *pTexHandle);
			}
		}
	}

	if (pProg->mSmpLink.Shadow >= 0) {
		glActiveTexture(GL_TEXTURE0 + Draw::TEXUNIT_Shadow);
		glBindTexture(GL_TEXTURE_2D, s_shadowTex);
	}

	if (isShadowCast || (isDiscard && !pMtl->mFlags.forceBlend)) {
		set_opaq();
	} else {
		if (pMtl->mFlags.alpha) {
			set_semi();
		} else {
			set_opaq();
		}
	}

	if (pMtl->mFlags.dblSided) {
		set_dbl_sided();
	} else {
		set_face_cull();
	}

	GLuint* pBufIds = pMdl->get_gpu_wk<GLuint>();
	GLuint bufVB = pBufIds[0];
	if (!bufVB) return;
	if (pProg->mVAO) {
		OGLSys::bind_vao(pProg->mVAO);
		glBindBuffer(GL_ARRAY_BUFFER, bufVB);
		pProg->enable_attrs(pBat->mMinIdx, pMdl->get_vtx_size());
		batch_draw_exec(pMdl, ibat);
		OGLSys::bind_vao(0);
	} else {
		glBindBuffer(GL_ARRAY_BUFFER, bufVB);
		pProg->enable_attrs(pBat->mMinIdx, pMdl->get_vtx_size());
		batch_draw_exec(pMdl, ibat);
		pProg->disable_attrs();
	}

	++s_batDrwCnt;

	if (isShadowCast) {
		++s_shadowCastCnt;
	}
}

void quad(const Draw::Quad* pQuad) {
	if (!pQuad) return;
	if (pQuad->color.a <= 0.0f) return;
	GPUProg* pProg = &s_prg_quad_quad;
	if (!pProg->is_valid()) return;
	GLuint htex = get_tex_handle(pQuad->pTex);
	if (!htex) {
		htex = OGLSys::get_white_tex();
	}
	set_screen_framebuf();
	set_semi();
	set_face_cull();
	OGLSys::enable_msaa(false);
	pProg->use();
	xt_float4 pos[2];
	const float* pPosSrc = pQuad->pos[0];
	float* pPosDst = pos[0];
	for (int i = 0; i < 8; ++i) {
		pPosDst[i] = pPosSrc[i] + 0.5f;
	}
	float scl[2];
	scl[0] = nxCalc::rcp0(pQuad->refWidth);
	scl[1] = nxCalc::rcp0(pQuad->refHeight);
	for (int i = 0; i < 8; ++i) {
		pPosDst[i] *= scl[i & 1];
	}
	for (int i = 0; i < 4; ++i) {
		int idx = i * 2 + 1;
		pPosDst[idx] = 1.0f - pPosDst[idx];
	}
	for (int i = 0; i < 8; ++i) {
		pPosDst[i] *= 2.0f;
	}
	for (int i = 0; i < 8; ++i) {
		pPosDst[i] -= 1.0f;
	}
	cxColor clr[4];
	float* pClrDst = clr[0].ch;
	const float* pClr = pQuad->color.ch;
	if (pQuad->pClrs) {
		float* pClrSrc = pQuad->pClrs[0].ch;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				*pClrDst++ = pClrSrc[j] * pClr[j];
			}
			pClrSrc += 4;
		}
	} else {
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				*pClrDst++ = pClr[j];
			}
		}
	}
	for (int i = 0; i < 4; ++i) {
		int offs = i * 2;
		float x = pPosDst[offs];
		float y = pPosDst[offs + 1];
		pPosDst[offs] = x*pQuad->rot[0].x + y*pQuad->rot[1].x;
		pPosDst[offs + 1] = x*pQuad->rot[0].y + y*pQuad->rot[1].y;
	}
	if (HAS_PARAM(QuadVtxPos)) {
		glUniform4fv(pProg->mParamLink.QuadVtxPos, 2, pos[0]);
	}
	if (HAS_PARAM(QuadVtxTex)) {
		glUniform4fv(pProg->mParamLink.QuadVtxTex, 2, pQuad->tex[0]);
	}
	if (HAS_PARAM(QuadVtxClr)) {
		glUniform4fv(pProg->mParamLink.QuadVtxClr, 4, clr[0].ch);
	}
	if (HAS_PARAM(InvGamma)) {
		xt_float3 invGamma;
		invGamma.set(nxCalc::rcp0(pQuad->gamma.x), nxCalc::rcp0(pQuad->gamma.y), nxCalc::rcp0(pQuad->gamma.z));
		pProg->set_inv_gamma(invGamma);
	}
	glActiveTexture(GL_TEXTURE0 + Draw::TEXUNIT_Base);
	glBindTexture(GL_TEXTURE_2D, htex);
	if (pProg->mVAO) {
		OGLSys::bind_vao(pProg->mVAO);
	}
	glBindBuffer(GL_ARRAY_BUFFER, s_quadVBO);
	pProg->enable_attrs(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_quadIBO);
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (const void*)0);
	if (pProg->mVAO) {
		OGLSys::bind_vao(0);
	} else {
		pProg->disable_attrs();
	}
}

void symbol(const Draw::Symbol* pSym) {
	Draw::Font* pFont = s_pFont;
	if (!pFont) return;
	cxColor clr = pSym->clr;
	if (clr.a <= 0.0f) return;
	if (!(s_fontVBO && s_fontIBO)) return;
	int sym = pSym->sym;
	if (uint32_t(sym) >= uint32_t(pFont->numSyms)) return;
	GPUProg* pProg = &s_prg_font_font;
	if (!pProg->is_valid()) return;
	Draw::Font::SymInfo* pInfo = &pFont->pSyms[sym];
	set_screen_framebuf();
	if (clr.a < 1.0f) {
		set_semi();
	} else {
		set_opaq();
	}
	set_face_cull();
	OGLSys::enable_msaa(true);
	pProg->use();
	if (HAS_PARAM(FontColor)) {
		xt_float4 fontClr;
		fontClr.set(clr.r, clr.g, clr.b, clr.a);
		glUniform4fv(pProg->mParamLink.FontColor, 1, fontClr);
	}
	float ox = pSym->ox;
	float oy = pSym->oy;
	float sx = pSym->sx;
	float sy = pSym->sy;
	if (HAS_PARAM(FontXform)) {
		xt_float4 fontXform;
		fontXform.set(ox*2.0f - 1.0f, oy*2.0f - 1.0f, sx * 2.0f, sy * 2.0f);
		glUniform4fv(pProg->mParamLink.FontXform, 1, fontXform);
	}
	if (HAS_PARAM(FontRot)) {
		xt_float4 fontRot;
		fontRot.set(pSym->rot[0].x, pSym->rot[1].x, pSym->rot[0].y, pSym->rot[1].y);
		glUniform4fv(pProg->mParamLink.FontRot, 1, fontRot);
	}
	if (pProg->mVAO) {
		OGLSys::bind_vao(pProg->mVAO);
	}
	glBindBuffer(GL_ARRAY_BUFFER, s_fontVBO);
	pProg->enable_attrs(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_fontIBO);
	glDrawElements(GL_TRIANGLES, pInfo->numTris * 3, GL_UNSIGNED_SHORT, (const void*)(pInfo->idxOrg * sizeof(uint16_t)));
	if (pProg->mVAO) {
		OGLSys::bind_vao(0);
	} else {
		pProg->disable_attrs();
	}
}

static bool ck_prim_vtx_range(const uint32_t org, const uint32_t num) {
	return !(org >= s_maxPrimVtx || org + num > s_maxPrimVtx);
}

static bool ck_prim_idx_range(const uint32_t org, const uint32_t num) {
	return !(org >= s_maxPrimIdx || org + num > s_maxPrimIdx);
}

void init_prims(const uint32_t maxVtx, const uint32_t maxIdx) {
	if (!s_drwInitFlg) return;
	if (s_primVBO) return;
	if (maxVtx < 3) return;
	glGenBuffers(1, &s_primVBO);
	if (s_primVBO) {
		glBindBuffer(GL_ARRAY_BUFFER, s_primVBO);
		glBufferData(GL_ARRAY_BUFFER, maxVtx * sizeof(sxPrimVtx), nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		s_maxPrimVtx = maxVtx;

		s_maxPrimIdx = 0;
		if (maxIdx >= 3) {
			glGenBuffers(1, &s_primIBO);
			if (s_primIBO) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_primIBO);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIdx * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				s_maxPrimIdx = maxIdx;
			}
		}
	}
}

static void prim_geom_vtx(const uint32_t org, const uint32_t num, const sxPrimVtx* pSrc) {
	if (!pSrc) return;
	if (!s_primVBO) return;
	if (!ck_prim_vtx_range(org, num)) return;
	glBindBuffer(GL_ARRAY_BUFFER, s_primVBO);
	GLintptr offs = org * sizeof(sxPrimVtx);
	GLsizeiptr len = num * sizeof(sxPrimVtx);
#if 1
	glBufferSubData(GL_ARRAY_BUFFER, offs, len, pSrc);
#else
	void* pDst = glMapBufferRange(GL_ARRAY_BUFFER, offs, len, GL_MAP_WRITE_BIT);
	if (pDst) {
		nxCore::mem_copy(pDst, pSrc, (size_t)len);
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
#endif
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void prim_geom_idx(const uint32_t org, const uint32_t num, const uint16_t* pSrc) {
	if (!pSrc) return;
	if (!s_primIBO) return;
	if (!ck_prim_idx_range(org, num)) return;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_primIBO);
	GLintptr offs = org * sizeof(uint16_t);
	GLsizeiptr len = num * sizeof(uint16_t);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offs, len, pSrc);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void prim_geom(const Draw::PrimGeom* pGeom) {
	if (!pGeom) return;
	prim_geom_vtx(pGeom->vtx.org, pGeom->vtx.num, pGeom->vtx.pSrc);
	prim_geom_idx(pGeom->idx.org, pGeom->idx.num, pGeom->idx.pSrc);
}

void prim(const Draw::Prim* pPrim, const Draw::Context* pCtx) {
	if (!pPrim) return;
	if (!pCtx) return;
	if (!s_primVBO) return;
	GPUProg* pProg = &s_prg_prim_prim;
	if (!pProg->is_valid()) return;
	uint32_t vorg = 0;
	uint32_t vnum = 0;
	uint32_t iorg = 0;
	uint32_t inum = 0;
	if (pPrim->indexed) {
		if (!s_primIBO) return;
		iorg = pPrim->org;
		inum = pPrim->num;
		if (!ck_prim_idx_range(iorg, inum)) return;
	} else {
		vorg = pPrim->org;
		vnum = pPrim->num;
		if (!ck_prim_vtx_range(vorg, vnum)) return;
	}
	GLuint htex = get_tex_handle(pPrim->pTex);
	if (!htex) {
		htex = OGLSys::get_white_tex();
	}
	set_def_framebuf();
	OGLSys::enable_msaa(true);
	if (pPrim->alphaBlend) {
		set_semi();
	} else {
		set_opaq();
	}
	if (pPrim->dblSided) {
		set_dbl_sided();
	} else {
		set_face_cull();
	}
	if (pPrim->depthWrite) {
		glDepthMask(GL_TRUE);
	} else {
		glDepthMask(GL_FALSE);
	}
	pProg->use();

	pProg->set_view_proj(pCtx->view.mViewProjMtx);
	pProg->set_view_pos(pCtx->view.mPos);

	xt_float4 ctrl;
	ctrl.fill(0.0f);
	if (pPrim->type == Draw::PRIMTYPE_SPRITE) {
		ctrl.x = 1.0f;
		if (HAS_PARAM(InvView)) {
			pProg->set_inv_view(pCtx->view.mInvViewMtx);
		}
	} else {
		if (HAS_PARAM(World)) {
			cxMtx m;
			if (pPrim->pMtx) {
				m = *pPrim->pMtx;
			} else {
				m.identity();
			}
			xt_xmtx wm;
			wm = nxMtx::xmtx_from_mtx(m);
			pProg->set_world(wm);
		}
		pProg->set_vtx_hemi_upper(pCtx->hemi.mUpper);
		pProg->set_vtx_hemi_lower(pCtx->hemi.mLower);
		pProg->set_vtx_hemi_up(pCtx->hemi.mUp);
		if (HAS_PARAM(VtxHemiParam)) {
			xt_float3 hparam;
			hparam.set(pCtx->hemi.mExp, pCtx->hemi.mGain, 0.0f);
			pProg->set_vtx_hemi_param(hparam);
		}
	}
	pProg->set_prim_ctrl(ctrl);

	pProg->set_fog_color(pCtx->fog.mColor);
	pProg->set_fog_param(pCtx->fog.mParam);

	pProg->set_inv_white(pCtx->cc.mToneMap.get_inv_white());
	pProg->set_lclr_gain(pCtx->cc.mToneMap.mLinGain);
	pProg->set_lclr_bias(pCtx->cc.mToneMap.mLinBias);
	pProg->set_exposure(pCtx->cc.mExposure);
	pProg->set_inv_gamma(pCtx->cc.get_inv_gamma());

	if (pProg->mSmpLink.Base >= 0) {
		glActiveTexture(GL_TEXTURE0 + Draw::TEXUNIT_Base);
		glBindTexture(GL_TEXTURE_2D, htex);
	}

	if (pProg->mVAO) {
		OGLSys::bind_vao(pProg->mVAO);
	}
	glBindBuffer(GL_ARRAY_BUFFER, s_primVBO);
	pProg->enable_attrs(0);
	if (inum > 0) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_primIBO);
		glDrawElements(GL_TRIANGLES, inum, GL_UNSIGNED_SHORT, (const void*)(iorg * sizeof(uint16_t)));
	} else {
		glDrawArrays(GL_TRIANGLES, vorg, vnum);
	}
	if (pProg->mVAO) {
		OGLSys::bind_vao(0);
	} else {
		pProg->disable_attrs();
	}

	glDepthMask(GL_TRUE);
}


static Draw::Ifc s_ifc;

struct DrwInit {
	DrwInit() {
		nxCore::mem_zero(&s_ifc, sizeof(s_ifc));
		s_ifc.info.pName = "ogl";
		s_ifc.info.needOGLContext = true;
		s_ifc.init = init;
		s_ifc.reset = reset;
		s_ifc.get_screen_width = get_screen_width;
		s_ifc.get_screen_height = get_screen_height;
		s_ifc.get_shadow_bias_mtx = get_shadow_bias_mtx;
		s_ifc.init_prims = init_prims;
		s_ifc.prim_geom = prim_geom;
		s_ifc.begin = begin;
		s_ifc.end = end;
		s_ifc.batch = batch;
		s_ifc.prim = prim;
		s_ifc.quad = quad;
		s_ifc.symbol = symbol;
		Draw::register_ifc_impl(&s_ifc);
	}
} s_drwInit;

DRW_IMPL_END

