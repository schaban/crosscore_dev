// g++ -pthread -I ../.. ../../crosscore.cpp xgeo2pano.cpp -o xgeo2pano -O3 -flto -march=native

#include "crosscore.hpp"

#define USE_PANO_SCAN 0

static void dbgmsg_impl(const char* pMsg) {
	::fprintf(stderr, "%s", pMsg);
	::fflush(stderr);
}
                
static void init_sys() {
	sxSysIfc sysIfc;
	nxCore::mem_zero(&sysIfc, sizeof(sysIfc));
	sysIfc.fn_dbgmsg = dbgmsg_impl;
	nxSys::init(&sysIfc);
}

static void reset_sys() {
}


static struct PANO_WK {
	sxGeometryData* mpGeo;
	cxVec* mpTris; // { v0, v1, v2 }[]
	cxVec* mpRays; // { dir }[]
	xt_float4* mpHits;
	int* mpIndices;
	cxColor* mpColors;
	cxVec mOrg;
	cxVec mCenter;
	float mMaxDist;
	int mNumTris;
	int mNumRays;
	int mPanoW;
	int mPanoH;

	void init(sxGeometryData* pGeo, const int panoW = 512, const int panoH = 256) {
		mpGeo = nullptr;
		mNumTris = 0;
		mNumRays = 0;
		mPanoW = panoW;
		mPanoH = panoH;
		mpRays = nullptr;
		mMaxDist = 0.0f;
		mOrg.zero();
		if (!pGeo) {
			nxCore::dbg_msg("PANO: no input geo\n");
			return;
		}
		if (!pGeo->is_all_tris()) {
			nxCore::dbg_msg("PANO: input geo must be triangulated\n");
			return;
		}
		if (!pGeo->has_BVH()) {
			nxCore::dbg_msg("Warning: no BVH in the input geo\n");
		}
		mpGeo = pGeo;
		mCenter = mpGeo->mBBox.get_center();
		mNumTris = mpGeo->get_pol_num();
		nxCore::dbg_msg("geo: %d triangles\n", mNumTris);
		nxCore::dbg_msg("pano: %dx%d\n", mPanoW, mPanoH);
		mpTris = nxCore::tMem<cxVec>::alloc(mNumTris * 3, "Pano:tris");
		if (!mpTris) {
			nxCore::dbg_msg("PANO: tri alloc error\n");
			return;
		}
		for (int i = 0; i < mNumTris; ++i) {
			sxGeometryData::Polygon pol = mpGeo->get_pol(i);
			int itri = i * 3;
			for (int j = 0; j < 3; ++j) {
				mpTris[itri + j] = pol.get_vtx_pos(j);
			}
		}
		mMaxDist = mpGeo->mBBox.get_bounding_radius() * 2.0f;
		mNumRays = mPanoW * mPanoH;
		mpRays = nxCore::tMem<cxVec>::alloc(mNumRays, "Pano:rays");
		if (!mpRays) {
			if (!mpTris) {
				nxCore::dbg_msg("PANO: rays alloc error\n");
				return;
			}
		}
#if USE_PANO_SCAN
		nxCalc::panorama_scan(*this, mPanoW, mPanoH);
#else
		for (int y = 0; y < mPanoH; ++y) {
			float v = 1.0f - (float(y) + 0.5f) / float(mPanoH);
			for (int x = 0; x < mPanoW; ++x) {
				float u = (float(x) + 0.5f) / float(mPanoW);
				cxVec dir = nxVec::from_polar_uv(u, v);
				int idx = y*mPanoW + x;
				mpRays[idx] = dir * mMaxDist;
			}
		}
#endif
		mpHits = nxCore::tMem<xt_float4>::alloc(mNumRays, "Pano:hits");
		mpIndices = nxCore::tMem<int>::alloc(mNumRays, "Pano:idx");
		mpColors = nxCore::tMem<cxColor>::alloc(mNumRays, "Pano:colors");
	}

	void reset() {
		if (mpTris) {
			nxCore::tMem<cxVec>::free(mpTris, mNumTris * 3);
		}
		if (mpRays) {
			nxCore::tMem<cxVec>::free(mpRays, mNumRays);
		}
		if (mpHits) {
			nxCore::tMem<xt_float4>::free(mpHits, mNumRays);
		}
		if (mpIndices) {
			nxCore::tMem<int>::free(mpIndices, mNumRays);
		}
		if (mpColors) {
			nxCore::tMem<cxColor>::free(mpColors, mNumRays);
		}
		mpGeo = nullptr;
		mpTris = nullptr;
		mpRays = nullptr;
		mpHits = nullptr;
		mpIndices = nullptr;
		mpColors = nullptr;
		mNumTris = 0;
		mNumRays = 0;
		mPanoW = 0;
		mPanoH = 0;
	}

	cxLineSeg get_seg(const int idx) const {
		cxLineSeg seg;
		if (mpRays && idx >= 0 && idx < mNumRays) {
			seg.set_org_dir(mOrg, mpRays[idx]);
		} else {
			seg.zero();
		}
		return seg;
	}

#if USE_PANO_SCAN
	void operator()(const int x, const int y, const float dx, const float dy, const float dz, const float dw) {
		if (!mpRays) return;
		int idx = y*mPanoW + x;
		mpRays[idx].set(dx, dy, dz);
		mpRays[idx].scl(mMaxDist);
	}
#endif

	void exec();
	void save(const char* pPath);
} WK;

class SegHitFn : public sxGeometryData::LeafFunc {
public:
	float mMinT;
	int mSegIdx;
	int mHitCnt;

	SegHitFn() : mMinT(2.0f), mSegIdx(-1), mHitCnt(0) {}

	virtual bool operator()(const sxGeometryData::Polygon& pol, const sxGeometryData::BVH::Node* pNode) {
		if (mSegIdx >= 0) {
			cxLineSeg seg = WK.get_seg(mSegIdx);
			int ipol = pol.get_id();
			int itri = ipol * 3;
			cxVec v0 = WK.mpTris[itri + 0];
			cxVec v1 = WK.mpTris[itri + 1];
			cxVec v2 = WK.mpTris[itri + 2];
			xt_float4 uvwt = nxGeom::seg_tri_intersect_bc_cw(seg.get_pos0(), seg.get_pos1(), v0, v1, v2);
			if (uvwt.w >= 0.0f) {
				++mHitCnt;
				if (uvwt.w < mMinT) {
					mMinT = uvwt.w;
					WK.mpHits[mSegIdx] = uvwt;
					WK.mpIndices[mSegIdx] = ipol;
				}
			}
		}
		return true;
	}
};

XD_NOINLINE void PANO_WK::exec() {
	if (!mpGeo) return;
	if (!mpHits) return;
	if (!mpColors) return;
	for (int i = 0; i < mNumRays; ++i) {
		mpHits[i].fill(0.0f);
		mpIndices[i] = -1;
	}
	SegHitFn hitFn;
	for (int i = 0; i < mNumRays; ++i) {
		hitFn.mSegIdx = i;
		hitFn.mMinT = 2.0f;
		mpGeo->leaf_hit_query(get_seg(i), hitFn);
	}
	for (int i = 0; i < mNumRays; ++i) {
		cxColor vclr[3];
		int triIdx = mpIndices[i];
		cxColor clr(0.0f);
		if (mpGeo->ck_pol_idx(triIdx)) {
			xt_float4 uvwt = mpHits[i];
			sxGeometryData::Polygon pol = mpGeo->get_pol(triIdx);
			for (int j = 0; j < 3; ++j) {
				int ipnt = pol.get_vtx_pnt_id(j);
				vclr[j] = mpGeo->get_pnt_color(ipnt, false);
			}
			for (int j = 0; j < 3; ++j) {
				vclr[j].scl_rgb(uvwt.get_at(j));
			}
			for (int j = 0; j < 3; ++j) {
				clr.add_rgb(vclr[j]);
			}
		}
		mpColors[i] = clr;
	}
}

void PANO_WK::save(const char* pPath) {
	if (!pPath) return;
	if (!mpColors) return;
	nxImage::save_hdr(pPath, mpColors, mPanoW, mPanoH);
}

int main(int argc, char* argv[]) {
	nxApp::init_params(argc, argv);
	init_sys();

	const char* pInGeoPath = nxApp::get_opt("in");
	if (!pInGeoPath) {
		pInGeoPath = "env.xgeo";
	}
	nxCore::dbg_msg("Input geo: %s\n", pInGeoPath);

	auto pGeo = nxData::load_as<sxGeometryData>(pInGeoPath);
	WK.init(pGeo);
	double t0 = nxSys::time_micros();
	WK.exec();
	double dt = nxSys::time_micros() - t0;
	nxCore::dbg_msg("exec dt: %f millis\n", dt * 1e-3f);

	const char* pOutImgPath = nxApp::get_opt("out");
	if (!pOutImgPath) {
		pOutImgPath = "xgeo_pano.hdr";
	}
	nxCore::dbg_msg("Output img: %s\n", pOutImgPath);
	WK.save(pOutImgPath);

	WK.reset();
	reset_sys();
	return 0;
}
