// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2019-2021 Sergey Chaban <sergey.chaban@gmail.com>

#ifndef DRW_NO_VULKAN
#	define DRW_NO_VULKAN 0
#endif

#define DRW_IMPL_BEGIN namespace {
#define DRW_IMPL_END }

namespace Draw {

	enum Mode {
		DRWMODE_STD,
		DRWMODE_DISCARD,
		DRWMODE_SHADOW_CAST
	};

	enum TexUnit {
		TEXUNIT_Base = 0,
		TEXUNIT_Bump,
		TEXUNIT_Spec,
		TEXUNIT_Surf,
		TEXUNIT_BumpPat,
		TEXUNIT_Shadow,
		TEXUNIT_COUNT
	};

	enum PrimType {
		PRIMTYPE_POLY = 0,
		PRIMTYPE_SPRITE = 1
	};

	struct Font {
		struct SymInfo {
			xt_float2 size;
			int idxOrg;
			int numTris;
		};
		int numSyms;
		int numPnts;
		int numTris;
		xt_float2* pPnts;
		uint16_t* pTris;
		SymInfo* pSyms;
	};

	inline float clip_gamma(const float gamma) { return nxCalc::max(gamma, 0.01f); }

	struct MdlParam {
		xt_float3 baseColorScl;
		float shadowOffsBias;
		float shadowWeightBias;
		float shadowDensScl;

		void reset() {
			baseColorScl.fill(1.0f);
			shadowOffsBias = 0.0f;
			shadowWeightBias = 0.0f;
			shadowDensScl = 1.0f;
		}
	};

	struct Context {
		sxView view;

		sxHemisphereLight hemi;

		struct Spec {
			cxVec mDir;
			xt_float3 mClr;
			float mShadowing;
			bool mEnabled;

			void set_dir(const cxVec& v) {
				mDir = v.get_normalized();
			}

			void set_rgb(const float r, const float g, const float b) {
				mClr.x = r;
				mClr.y = g;
				mClr.z = b;
			}

			void reset() {
				mDir.set(0.0f, 0.0f, -1.0f);
				mClr.fill(1.0f);
				mShadowing = 1.0f;
				mEnabled = true;
			}
		} spec;

		sxFog fog;

		struct CC {
			sxToneMap mToneMap;
			xt_float3 mGamma;
			xt_float3 mExposure;

			void set_gamma(const float gval) {
				mGamma.fill(clip_gamma(gval));
			}

			void set_gamma_rgb(const float r, const float g, const float b) {
				mGamma.set(clip_gamma(r), clip_gamma(g), clip_gamma(b));
			}

			void set_exposure(const float e) {
				mExposure.fill(e);
			}

			void set_exposure_rgb(const float r, const float g, const float b) {
				mExposure.set(r, g, b);
			}

			void set_linear_white(const float val) {
				mToneMap.set_linear_white(val);
			}

			void set_linear_white_rgb(const float r, const float g, const float b) {
				mToneMap.set_linear_white_rgb(r, g, b);
			}

			void set_linear_gain(const float val) {
				mToneMap.set_linear_gain(val);
			}

			void set_linear_gain_rgb(const float r, const float g, const float b) {
				mToneMap.set_linear_gain_rgb(r, g, b);
			}

			void set_linear_bias(const float val) {
				mToneMap.set_linear_bias(val);
			}

			void set_linear_bias_rgb(const float r, const float g, const float b) {
				mToneMap.set_linear_bias_rgb(r, g, b);
			}

			xt_float3 get_inv_gamma() const {
				xt_float3 invGamma;
				invGamma.set(nxCalc::rcp0(mGamma.x), nxCalc::rcp0(mGamma.y), nxCalc::rcp0(mGamma.z));
				return invGamma;
			}

			void reset() {
				mToneMap.reset();
				set_exposure(-1.0f);
				set_gamma(2.2f);
			}
		} cc;

		struct Shadow {
			cxMtx mViewProjMtx;
			cxMtx mMtx;
			cxVec mDir;
			float mDens;
			float mDensBias;
			float mOffsBias;
			float mWghtBias;
			float mFadeStart;
			float mFadeEnd;

			void set_dir(const cxVec& v) {
				mDir = v.get_normalized();
			}

			void set_dir_degrees(const float dx, const float dy) {
				set_dir(nxQuat::from_degrees(dx, dy, 0.0f).apply(nxVec::get_axis(exAxis::PLUS_Z)));
			}

			float get_density() const {
				return nxCalc::max(mDens + mDensBias, 0.0f);
			}

			void reset() {
				set_dir_degrees(70, 140);
				mDens = 1.0f;
				mDensBias = 0.0f;
				mOffsBias = 0.0f;
				mWghtBias = 0.0f;
				mFadeStart = 0.0f;
				mFadeEnd = 0.0f;
				mViewProjMtx.identity();
				mMtx.identity();
			}
		} shadow;

		struct Glb {
			bool useSpec;
			bool useBump;

			void reset() {
				useSpec = true;
				useBump = true;
			}
		} glb;

		struct Ext {
			xt_float4* pArgs;
			size_t numArgs;

			void reset() {
				pArgs = nullptr;
				numArgs = 0;
			}
		} ext;

		void reset() {
			view.reset();
			hemi.reset();
			spec.reset();
			fog.reset();
			cc.reset();
			shadow.reset();
			glb.reset();
			ext.reset();
		}

		bool ck_bbox_shadow_receive(const cxAABB& bbox) const {
			float sfadeStart = shadow.mFadeStart;
			float sfadeEnd = shadow.mFadeEnd;
			bool recvFlg = true;
			if (sfadeEnd > 0.0f && sfadeEnd > sfadeStart) {
				cxVec vpos = view.mPos;
				cxVec npos = bbox.closest_pnt(vpos);
				if (nxVec::dist(vpos, npos) > sfadeEnd) {
					recvFlg = false;
				}
			}
			return recvFlg;
		}

	};

	struct PrimGeom {
		struct {
			uint32_t org;
			uint32_t num;
			const sxPrimVtx* pSrc;
		} vtx;
		struct {
			uint32_t org;
			uint32_t num;
			const uint16_t* pSrc;
		} idx;
	};

	struct Prim {
		sxTextureData* pTex;
		xt_texcoord texOffs;
		xt_rgba clr;
		cxMtx* pMtx;
		uint32_t org;
		uint32_t num;
		PrimType type;
		bool indexed;
		bool dblSided;
		bool alphaBlend;
		bool depthWrite;
	};

	struct Quad {
		xt_float2 pos[4];
		xt_float2 tex[4];
		xt_float2 rot[2];
		cxColor color;
		xt_float3 gamma;
		float refWidth;
		float refHeight;
		sxTextureData* pTex;
		cxColor* pClrs;

		void clear() {
			nxCore::mem_zero((void*)this, sizeof(Quad));
		}

		void set_gamma(const float gval) {
			gamma.fill(clip_gamma(gval));
		}

		void set_gamma_rgb(const float r, const float g, const float b) {
			gamma.set(clip_gamma(r), clip_gamma(g), clip_gamma(b));
		}
	};

	struct Symbol {
		cxColor clr;
		xt_float2 rot[2];
		int sym;
		float ox;
		float oy;
		float sx;
		float sy;
	};

	struct Ifc {
		struct Info {
			const char* pName;
			bool needOGLContext;
		} info;

		void (*init)(const int shadowSize, cxResourceManager* pRsrcMgr, Font* pFont);
		void (*reset)();

		int (*get_screen_width)();
		int (*get_screen_height)();

		cxMtx (*get_shadow_bias_mtx)();

		void (*init_prims)(const uint32_t maxVtx, const uint32_t maxIdx);
		void (*prim_geom)(const PrimGeom* pGeom);

		void (*begin)(const cxColor& clearColor);
		void (*end)();

		void (*batch)(cxModelWork* pWk, const int ibat, const Mode mode, const Context* pCtx);
		void (*prim)(const Prim* pPrim, const Context* pCtx);
		void (*quad)(const Quad* pQuad);
		void (*symbol)(const Symbol* pSym);
	};

	int32_t register_ifc_impl(Ifc* pIfc);
	Ifc* find_ifc_impl(const char* pName);
	Ifc* get_ifc_impl();

} // Draw
