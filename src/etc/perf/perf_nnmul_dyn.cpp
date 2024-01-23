// g++ -pthread -I ../.. ../../crosscore.cpp perf_nnmul_dyn.cpp -o perf_nnmul_dyn -O3 -flto -march=native
// (AArch: -mcpu=native)

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

static void fill_mtx(float* pMtx, const int N) {
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < N; ++j) {
			float xi = float(i + 1);
			float xj = float(j + 1);
			float xn1 = float(N + 1);
			pMtx[i*N + j] = mth_sqrtf(2.0f / xn1) * mth_sinf((xi*xj*XD_PI) / xn1);
		}
	}
}

static void print_mtx(float* pMtx, const int N) {
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < N; ++j) {
			nxCore::dbg_msg(" %.4f", pMtx[i*N + j]);
		}
		nxCore::dbg_msg("\n");
	}
}

XD_NOINLINE static void mtx_mul(float* pRes, const float* pA, const float* pB, const int N) {
	nxLA::mul_mm_f(pRes, pA, pB, N, N, N);
}

int main(int argc, char* argv[]) {
	nxApp::init_params(argc, argv);
	init_sys();

	int N = nxApp::get_int_opt("N", 100);
	int nmuls = nxApp::get_int_opt("nmuls", 100);
	bool printFlg = nxApp::get_bool_opt("print", false);
	g_silent = nxApp::get_bool_opt("silent", false);

	float* pA = nxLA::mtx_alloc<float>(N, N);
	float* pB = nxLA::mtx_alloc<float>(N, N);
	float* pRes = nxLA::mtx_alloc<float>(N, N);
	fill_mtx(pA, N);
	fill_mtx(pB, N);

	nxCore::dbg_msg("Multiplying %dx%d f32 matrices %d times.\n", N, N, nmuls);
	double t0 = nxSys::time_micros();
	for (int i = 0; i < nmuls; ++i) {
		mtx_mul(pRes, pA, pB, N);
		nxCore::mem_copy(pA, pRes, N*N*sizeof(float));
	}
	double dt = nxSys::time_micros() - t0;
	nxCore::dbg_msg("dt: %f millis\n", dt * 1e-3f);

	if (printFlg) {
		print_mtx(pRes, N);
	}

	nxCore::mem_free(pA);
	nxCore::mem_free(pB);
	nxCore::mem_free(pRes);

	nxApp::reset();
	nxCore::mem_dbg();
	reset_sys();
	return 0;
}
