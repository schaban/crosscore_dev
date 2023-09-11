// g++ -pthread -I ../.. ../../crosscore.cpp perf_shpano.cpp -o perf_shpano -O3 -flto ...

#include "crosscore.hpp"

#ifndef _SCAN_BUFF_
#	define _SCAN_BUFF_ 0
#endif

#if _SCAN_BUFF_
#	ifndef SCANBUFF_SIZE
#		define SCANBUFF_SIZE 4
#	endif
#endif

static bool g_silent = false;

static void dbgmsg_impl(const char* pMsg) {
	if (g_silent) return;
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

class SrcPanoImg {
protected:
	sxHemisphereLight mHemi;
	cxVec mSunDir;
	int mWidth;
	int mHeight;
	cxColor* mpPixels;

public:
	SrcPanoImg() : mpPixels(nullptr) {
		reset();
	}

	void reset() {
		if (mpPixels) {
			nxCore::mem_free(mpPixels);
			mpPixels = nullptr;
		}
		mWidth = 0;
		mHeight = 0;
	}

	const cxColor* get_pixels() const { return mpPixels; }
	int get_width() const { return mWidth; }
	int get_height() const { return mHeight; }

	void init(int w, int h) {
		mWidth = nxCalc::max(w, 16);
		mHeight = nxCalc::max(h, 8);
		mpPixels = (cxColor*)nxCore::mem_alloc(sizeof(cxColor) * mWidth * mHeight, "SrcPano::Pixels");
		mHemi.reset();
		mHemi.mUp.set(0.0f, 1.0f, 0.0f);
		mHemi.mLower.set(0.33f, 0.44f, 0.22f);
		mHemi.mUpper.set(0.22f, 0.33f, 0.55f);
		mHemi.mExp = 0.65f;
		mSunDir = cxVec(-0.3f, 1.0f, -1.0f).get_normalized();
		nxCalc::panorama_scan(*this, mWidth, mHeight);
	}

	void operator()(const int x, const int y, const float dx, const float dy, const float dz, const float dw) {
		if (!mpPixels) return;
		cxVec dir(dx, dy, dz);
		cxColor c = mHemi.eval(dir);
		float d = mSunDir.dot(dir);
		if (d >= 0.9f) {
			float bright = 10.0f;
			d = nxCalc::ease(nxCalc::fit(d, 0.9f, 1.0f, 0.0f, 0.7f), 5.0f) * bright;
			c.add_rgb(cxColor(1.f*d, 0.75f*d, 0.5f*d));
		}
		mpPixels[y*mWidth + x] = c;
	}

	void save(const char* pPath = "_src.hdr") {
		if (!mpPixels) return;
		nxImage::save_hdr(pPath, mpPixels, mWidth, mHeight);
	}
};

class DstSHCoefs {
protected:
	const SrcPanoImg* mpSrcImg;
	float* mpConsts;
	float* mpWeights;
	float* mpCoefsR;
	float* mpCoefsG;
	float* mpCoefsB;
	float* mpCoefsDir;
	float* mpCoefsEval;
	int mOrder;
#if _SCAN_BUFF_
	float* mpBufCoefsDir;
	float mBufDirX[SCANBUFF_SIZE];
	float mBufDirY[SCANBUFF_SIZE];
	float mBufDirZ[SCANBUFF_SIZE];
	int mBufIdx;
#endif

	void clear_coefs() {
		int ncoefs = num_coefs();
		if (mpCoefsR) {
			for (int i = 0; i < ncoefs; ++i) {
				mpCoefsR[i] = 0.0f;
			}
		}
		if (mpCoefsG) {
			for (int i = 0; i < ncoefs; ++i) {
				mpCoefsG[i] = 0.0f;
			}
		}
		if (mpCoefsB) {
			for (int i = 0; i < ncoefs; ++i) {
				mpCoefsB[i] = 0.0f;
			}
		}
#if _SCAN_BUFF_
	mBufIdx = 0;
#endif
	}

public:
	DstSHCoefs()
	:
	mpSrcImg(nullptr), mpConsts(nullptr), mpWeights(nullptr),
	mpCoefsR(nullptr), mpCoefsG(nullptr), mpCoefsB(nullptr),
	mpCoefsDir(nullptr), mpCoefsEval(nullptr), mOrder(0)
	{}

	int num_coefs() const {
		if (mOrder < 2) return 0;
		return nxCalc::sq(mOrder);
	}

	int order() const {
		return mOrder;
	}

	void init(int order) {
		mOrder = order;
		if (mOrder < 2) return;
		int nconsts = nxSH::calc_consts_num(mOrder);
		mpConsts = (float*)nxCore::mem_alloc(nconsts * sizeof(float), "SH:consts");
		nxSH::calc_consts(mOrder, mpConsts);
		mpWeights = (float*)nxCore::mem_alloc(mOrder * sizeof(float), "SH:weights");
		calc_weights(3.0f);
		int ncoefs = num_coefs();
		mpCoefsR = (float*)nxCore::mem_alloc(ncoefs * sizeof(float), "SH:coefsR");
		mpCoefsG = (float*)nxCore::mem_alloc(ncoefs * sizeof(float), "SH:coefsG");
		mpCoefsB = (float*)nxCore::mem_alloc(ncoefs * sizeof(float), "SH:coefsB");
		mpCoefsDir = (float*)nxCore::mem_alloc(ncoefs * sizeof(float), "SH:coefsDir");
		mpCoefsEval = (float*)nxCore::mem_alloc(ncoefs * sizeof(float), "SH:coefsEval");
#if _SCAN_BUFF_
		mpBufCoefsDir = (float*)nxCore::mem_alloc(ncoefs * SCANBUFF_SIZE * sizeof(float), "SH:bufCoefsDir");
#endif
		clear_coefs();
	}

	void reset() {
		nxCore::mem_free(mpConsts);
		nxCore::mem_free(mpWeights);
		nxCore::mem_free(mpCoefsR);
		nxCore::mem_free(mpCoefsG);
		nxCore::mem_free(mpCoefsB);
		nxCore::mem_free(mpCoefsDir);
		nxCore::mem_free(mpCoefsEval);
		mpConsts = nullptr;
		mpWeights = nullptr;
		mpCoefsR = nullptr;
		mpCoefsG = nullptr;
		mpCoefsB = nullptr;
		mpCoefsDir = nullptr;
		mpCoefsEval = nullptr;
		mOrder = 0;
#if _SCAN_BUFF_
		nxCore::mem_free(mpBufCoefsDir);
		mpBufCoefsDir = nullptr;
#endif
	}

	void calc_weights(const float s, const float scl = 1.0f) {
		if (!mpWeights) return;
		nxSH::calc_weights(mpWeights, mOrder, s, scl);
	}

#if _SCAN_BUFF_
	void operator()(const int x, const int y, const float dx, const float dy, const float dz, const float dw) {
		if (mOrder < 2) return;
		if (!mpSrcImg) return;
		mBufDirX[mBufIdx] = dx;
		mBufDirY[mBufIdx] = dy;
		mBufDirZ[mBufIdx] = dz;
		++mBufIdx;
		if (mBufIdx < SCANBUFF_SIZE) {
			return;
		}

#if (SCANBUFF_SIZE == 4)
		if (mOrder == 3) {
			nxSH::eval_sh3_ary4(mpBufCoefsDir, mBufDirX, mBufDirY, mBufDirZ, mpConsts);
		} else if (mOrder == 6) {
			nxSH::eval_sh6_ary4(mpBufCoefsDir, mBufDirX, mBufDirY, mBufDirZ, mpConsts);
		} else {
			nxSH::eval_ary4(mOrder, mpBufCoefsDir, mBufDirX, mBufDirY, mBufDirZ, mpConsts);
		}
#elif (SCANBUFF_SIZE == 8)
		if (mOrder == 3) {
			nxSH::eval_sh3_ary8(mpBufCoefsDir, mBufDirX, mBufDirY, mBufDirZ, mpConsts);
		} else if (mOrder == 6) {
			nxSH::eval_sh6_ary8(mpBufCoefsDir, mBufDirX, mBufDirY, mBufDirZ, mpConsts);
		} else {
			nxSH::eval_ary8(mOrder, mpBufCoefsDir, mBufDirX, mBufDirY, mBufDirZ, mpConsts);
		}
#endif

		int ncoefs = num_coefs();
		for (int i = 0; i < ncoefs * SCANBUFF_SIZE; ++i) {
			mpBufCoefsDir[i] *= dw;
		}

		float r[SCANBUFF_SIZE];
		float g[SCANBUFF_SIZE];
		float b[SCANBUFF_SIZE];
		const cxColor* pImg = mpSrcImg->get_pixels();
		int w = mpSrcImg->get_width();
		int x0 = x - SCANBUFF_SIZE + 1;
		int ioffs = y*w + x0;
		for (int i = 0; i < SCANBUFF_SIZE; ++i) {
			cxColor c = pImg[ioffs + i];
			r[i] = c.r;
			g[i] = c.g;
			b[i] = c.b;
		}

		int coffs = 0;
		for (int i = 0; i < ncoefs; ++i) {
			for (int j = 0; j < SCANBUFF_SIZE; ++j) {
				mpCoefsR[i] += r[j] * mpBufCoefsDir[coffs + j];
			}
			for (int j = 0; j < SCANBUFF_SIZE; ++j) {
				mpCoefsG[i] += g[j] * mpBufCoefsDir[coffs + j];
			}
			for (int j = 0; j < SCANBUFF_SIZE; ++j) {
				mpCoefsB[i] += b[j] * mpBufCoefsDir[coffs + j];
			}
			coffs += SCANBUFF_SIZE;
		}

		mBufIdx = 0;
	}

#else

	void operator()(const int x, const int y, const float dx, const float dy, const float dz, const float dw) {
		if (mOrder < 2) return;
		if (!mpSrcImg) return;
		int w = mpSrcImg->get_width();
		const cxColor* pImg = mpSrcImg->get_pixels();
		cxColor c = pImg[y*w + x];

		if (mOrder == 3) {
			nxSH::eval_sh3(mpCoefsDir, dx, dy, dz, mpConsts);
		} else if (mOrder == 6) {
			nxSH::eval_sh6(mpCoefsDir, dx, dy, dz, mpConsts);
		} else {
			nxSH::eval(mOrder, mpCoefsDir, dx, dy, dz, mpConsts);
		}

		int ncoefs = num_coefs();
		for (int i = 0; i < ncoefs; ++i) {
			mpCoefsDir[i] *= dw;
		}
		for (int i = 0; i < ncoefs; ++i) {
			mpCoefsR[i] += c.r * mpCoefsDir[i];
		}
		for (int i = 0; i < ncoefs; ++i) {
			mpCoefsG[i] += c.g * mpCoefsDir[i];
		}
		for (int i = 0; i < ncoefs; ++i) {
			mpCoefsB[i] += c.b * mpCoefsDir[i];
		}
	}
#endif

	XD_NOINLINE void from_img(const SrcPanoImg& srcImg) {
		mpSrcImg = &srcImg;
		int w = mpSrcImg->get_width();
		int h = mpSrcImg->get_height();
		clear_coefs();
		float scl = nxCalc::panorama_scan(*this, w, h);
		int ncoefs = num_coefs();
		for (int i = 0; i < ncoefs; ++i) {
			mpCoefsR[i] *= scl;
		}
		for (int i = 0; i < ncoefs; ++i) {
			mpCoefsG[i] *= scl;
		}
		for (int i = 0; i < ncoefs; ++i) {
			mpCoefsB[i] *= scl;
		}
		mpSrcImg = nullptr;
	}

	cxColor eval(const cxVec& v) const {
		int ncoefs = num_coefs();
		nxSH::eval(mOrder, mpCoefsDir, v.x, v.y, v.z, mpConsts);

		float r = 0.0f;
		nxSH::apply_weights(mpCoefsEval, mOrder, mpCoefsR, mpWeights);
		for (int i = 0; i < ncoefs; ++i) {
			r += mpCoefsDir[i] * mpCoefsEval[i];
		}

		float g = 0.0f;
		nxSH::apply_weights(mpCoefsEval, mOrder, mpCoefsG, mpWeights);
		for (int i = 0; i < ncoefs; ++i) {
			g += mpCoefsDir[i] * mpCoefsEval[i];
		}

		float b = 0.0f;
		nxSH::apply_weights(mpCoefsEval, mOrder, mpCoefsB, mpWeights);
		for (int i = 0; i < ncoefs; ++i) {
			b += mpCoefsDir[i] * mpCoefsEval[i];
		}

		return cxColor(r, g, b);
	}

	void print_coefs() const {
		int ncoefs = num_coefs();
		nxCore::dbg_msg("R:");
		for (int i = 0; i < ncoefs; ++i) {
			nxCore::dbg_msg(" %.3f", mpCoefsR[i]);
		}
		nxCore::dbg_msg("\n");
		nxCore::dbg_msg("G:");
		for (int i = 0; i < ncoefs; ++i) {
			nxCore::dbg_msg(" %.3f", mpCoefsG[i]);
		}
		nxCore::dbg_msg("\n");
		nxCore::dbg_msg("B:");
		for (int i = 0; i < ncoefs; ++i) {
			nxCore::dbg_msg(" %.3f", mpCoefsB[i]);
		}
		nxCore::dbg_msg("\n");
	}
};


class DstPanoImg {
protected:
	cxColor* mpPixels;
	int mWidth;
	int mHeight;

public:
	DstPanoImg() : mpPixels(nullptr), mWidth(0), mHeight(0) {
	}

	void init(const int w, const int h) {
		mWidth = nxCalc::max(w, 16);
		mHeight = nxCalc::max(h, 8);
		mpPixels = (cxColor*)nxCore::mem_alloc(sizeof(cxColor) * mWidth * mHeight, "DstPano::Pixels");
	}

	void reset() {
		if (mpPixels) {
			nxCore::mem_free(mpPixels);
			mpPixels = nullptr;
		}
		mWidth = 0;
		mHeight = 0;
	}

	XD_NOINLINE void from_coefs(DstSHCoefs& coefs, const float s) {
		if (!mpPixels) return;
		coefs.calc_weights(s);
		for (int y = 0; y < mHeight; ++y) {
			float v = 1.0f - (float(y) + 0.5f) / float(mHeight);
			for (int x = 0; x < mWidth; ++x) {
				float u = (float(x) + 0.5f) / float(mWidth);
				cxVec dir = nxVec::from_polar_uv(u, v);
				cxColor c = coefs.eval(dir);
				mpPixels[y*mWidth + x] = c;
			}
		}
	}

	void save(const char* pPath = "_dst.hdr") {
		if (!mpPixels) return;
		nxImage::save_hdr(pPath, mpPixels, mWidth, mHeight);
	}
};


int main(int argc, char* argv[]) {
	nxApp::init_params(argc, argv);
	init_sys();

	int panoW = nxApp::get_int_opt("w", 512);
	int panoH = nxApp::get_int_opt("h", 256);
	int shOrder = nxApp::get_int_opt("order", 6);
	float evalS = nxApp::get_float_opt("s", 10.0f);
	bool dumpSrc = nxApp::get_bool_opt("dump_src", false);
	bool dumpDst = nxApp::get_bool_opt("dump_dst", false);
	bool printCoefs = nxApp::get_bool_opt("print_coefs", false);
	g_silent = nxApp::get_bool_opt("silent", false);

	SrcPanoImg srcImg;
	double srcPanoT0 = nxSys::time_micros();
	srcImg.init(panoW, panoH);
	double srcPanoDT = nxSys::time_micros() - srcPanoT0;
	if (dumpSrc) {
		srcImg.save();
	}

	DstSHCoefs dstCoefs;
	dstCoefs.init(shOrder);
	double coefsT0 = nxSys::time_micros();
	dstCoefs.from_img(srcImg);
	double coefsDT = nxSys::time_micros() - coefsT0;

	if (printCoefs) {
		dstCoefs.print_coefs();
	}

	if (dumpDst) {
		DstPanoImg dstImg;
		dstImg.init(panoW, panoH);
		dstImg.from_coefs(dstCoefs, evalS);
		dstImg.save();
		dstImg.reset();
	}

	nxCore::dbg_msg("%d x %d, order = %d\n", srcImg.get_width(), srcImg.get_height(), dstCoefs.order());
#if _SCAN_BUFF_
	nxCore::dbg_msg("bufsize: %d\n", SCANBUFF_SIZE);
#endif
	nxCore::dbg_msg("srcPano DT: %f millis\n", srcPanoDT * 1e-3f);
	nxCore::dbg_msg("coefs DT: %f millis\n", coefsDT * 1e-3f);

	srcImg.reset();
	dstCoefs.reset();

	nxApp::reset();
	nxCore::mem_dbg();
	reset_sys();
	return 0;
}

