#include "crosscore.hpp"

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



XD_NOINLINE void mtx_mul(xt_half* pRes, const xt_half* pMtx1, const xt_half* pMtx2, const int N) {
	nxLA::mul_mm_h(pRes, pMtx1, pMtx2, N, N, N);

}



int main(int argc, char* argv[]) {
	nxApp::init_params(argc, argv);
	init_sys();

	int N = nxApp::get_int_opt("N", 100);
	g_silent = nxApp::get_bool_opt("silent", false);

	size_t mtxMemSize = N*N*sizeof(xt_half);
	xt_half* pMtx = (xt_half*)nxCore::mem_alloc(mtxMemSize);
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < N; ++j) {
			float xi = float(i + 1);
			float xj = float(j + 1);
			float xn1 = float(N + 1);
			float val = mth_sqrtf(2.0f / xn1) * mth_sinf((xi*xj*XD_PI) / xn1);
			xt_half h;
			h.set(val);
			pMtx[i*N + j] = h;
		}
	}
	xt_half* pMtx2 = (xt_half*)nxCore::mem_alloc(mtxMemSize);
	nxCore::mem_copy(pMtx2, pMtx, mtxMemSize);
	xt_half* pRes = (xt_half*)nxCore::mem_alloc(mtxMemSize);

	double t0 = nxSys::time_micros();
	mtx_mul(pRes, pMtx, pMtx2, N);
	double dt = nxSys::time_micros() - t0;
	nxCore::dbg_msg("dt: %f millis\n", dt * 1e-3f);


	float s = 0.0f;
	for (int i = 0; i < N*N; ++i) {
		s += pRes[i].get();
	}
	bool ok = (mth_roundf(s) == N);

	nxCore::dbg_msg("test: %s\n", ok ? "pass" : "fail");


	nxApp::reset();
	reset_sys();
	return 0;
}
