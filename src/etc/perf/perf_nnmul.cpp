// g++ -pthread -I ../.. ../../crosscore.cpp perf_nnmul.cpp -o perf_nnmul -O3 -flto -march=native -D_MTX_N_=100
// (AArch: -mcpu=native)
// ./perf_nnmul -nmuls:1000

#include "crosscore.hpp"

#ifndef _MTX_T_
#	define _MTX_T_ float
#endif

#ifndef _MTX_N_
#	define _MTX_N_ 5
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

static void fill_mtx(_MTX_T_* pMtx) {
	for (int i = 0; i < _MTX_N_; ++i) {
		for (int j = 0; j < _MTX_N_; ++j) {
			_MTX_T_ xi = _MTX_T_(i + 1);
			_MTX_T_ xj = _MTX_T_(j + 1);
			_MTX_T_ xn1 = _MTX_T_(_MTX_N_ + 1);
			pMtx[i*_MTX_N_ + j] = _MTX_T_( mth_sqrt(_MTX_T_(2) / xn1) * mth_sin((xi*xj*XD_PI) / xn1) );
		}
	}
}

static void print_mtx(_MTX_T_* pMtx) {
	for (int i = 0; i < _MTX_N_; ++i) {
		for (int j = 0; j < _MTX_N_; ++j) {
			nxCore::dbg_msg(" %.4f", pMtx[i*_MTX_N_ + j]);
		}
		nxCore::dbg_msg("\n");
	}
}

XD_NOINLINE static void mtx_mul(_MTX_T_* pRes, _MTX_T_* pA, _MTX_T_* pB) {
	nxLA::mul_mm<_MTX_T_, _MTX_T_, _MTX_T_>(pRes, pA, pB, _MTX_N_, _MTX_N_, _MTX_N_);
}

static void mtx_cpy(_MTX_T_* pDst, _MTX_T_* pSrc) {
	nxLA::mtx_cpy<_MTX_T_, _MTX_T_>(pDst, pSrc, _MTX_N_, _MTX_N_);
}

template<typename T> void print_mtx_elem_t() { nxCore::dbg_msg("???"); }
template<> void print_mtx_elem_t<float>() { nxCore::dbg_msg("f32"); }
template<> void print_mtx_elem_t<double>() { nxCore::dbg_msg("f64"); }

int main(int argc, char* argv[]) {
	nxApp::init_params(argc, argv);
	init_sys();

	int nmuls = nxApp::get_int_opt("nmuls", 100);
	bool printFlg = nxApp::get_bool_opt("print", false);
	g_silent = nxApp::get_bool_opt("silent", false);

	_MTX_T_* pA = nxLA::mtx_alloc<_MTX_T_>(_MTX_N_, _MTX_N_);
	_MTX_T_* pB = nxLA::mtx_alloc<_MTX_T_>(_MTX_N_, _MTX_N_);
	_MTX_T_* pRes = nxLA::mtx_alloc<_MTX_T_>(_MTX_N_, _MTX_N_);
	fill_mtx(pA);
	fill_mtx(pB);

	nxCore::dbg_msg("Multiplying %dx%d matrices %d times.\n", _MTX_N_, _MTX_N_, nmuls);
	nxCore::dbg_msg("Element type: ");
	print_mtx_elem_t<_MTX_T_>();
	nxCore::dbg_msg("\n");
	double t0 = nxSys::time_micros();
	for (int i = 0; i < nmuls; ++i) {
		mtx_mul(pRes, pA, pB);
		mtx_cpy(pA, pRes);
	}
	double dt = nxSys::time_micros() - t0;
	nxCore::dbg_msg("dt: %f millis\n", dt * 1e-3f);

	if (printFlg) {
		print_mtx(pRes);
	}

	nxCore::mem_free(pA);
	nxCore::mem_free(pB);
	nxCore::mem_free(pRes);

	nxApp::reset();
	nxCore::mem_dbg();
	reset_sys();
	return 0;
}
