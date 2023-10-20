// g++ -pthread -I ../.. ../../crosscore.cpp perf_mkbvh.cpp -o perf_mkbvh -O3 -flto


#include "crosscore.hpp"


static bool g_silent = false;
static int g_dump = 0;

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

static int g_nsrc = 10;
static cxVec* s_pPts = nullptr;
static float* s_pRadii = nullptr;

static void make_geo() {
	int n = g_nsrc;
	float xz = mth_sqrtf((float)n);
	float rbase = xz * 0.1f;
	s_pPts = (cxVec*)nxCore::mem_alloc(n * sizeof(cxVec));
	for (int i = 0; i < n; ++i) {
		float cx = nxCore::rng_f01();
		float cy = nxCore::rng_f01();
		float cz = nxCore::rng_f01();
		cx = nxCalc::fit(cx, 0.0f, 1.0f, -xz*0.5f, xz*0.5f);
		cy = nxCalc::fit(cy, 0.0f, 1.0f, -0.5f, 0.5f);
		cz = nxCalc::fit(cz, 0.0f, 1.0f, -xz*0.5f, xz*0.5f);
		s_pPts[i].set(cx, cy, cz);
	}
	s_pRadii = (float*)nxCore::mem_alloc(n * sizeof(float));
	for (int i = 0; i < n; ++i) {
		float r = nxCore::rng_f01();
		r = nxCalc::fit(r, 0.0f, 1.0f, rbase*0.1f, rbase*0.3f);
		s_pRadii[i] = r;
	}
}

static void dump_geo(FILE* pOut = stdout) {
	if (g_dump != 1) return;
	if (!pOut) return;
	int n = g_nsrc;
	::fprintf(pOut, "PGEOMETRY V5\n");
	::fprintf(pOut, "NPoints %d NPrims %d\n", n, n);
	::fprintf(pOut, "NPointGroups 0 NPrimGroups 0\n");
	::fprintf(pOut, "NPointAttrib 0 NVertexAttrib 0 NPrimAttrib 0 NAttrib 0\n");
	for (int i = 0; i < n; ++i) {
		cxVec pos = s_pPts[i];
		::fprintf(pOut, "%f %f %f 1\n", pos.x, pos.y, pos.z);
	}
	for (int i = 0; i < n; ++i) {
		float r = s_pRadii[i];
		::fprintf(pOut, "Sphere %d %f 0 0 0 0 %f 0 %f 0\n", i, r, -r, r);
	}
	::fprintf(pOut, "beginExtra\n");
	::fprintf(pOut, "endExtra\n");
}

static cxAABB* s_pSrcBoxes = nullptr;

static void make_src_boxes() {
	int n = g_nsrc;
	s_pSrcBoxes = (cxAABB*)nxCore::mem_alloc(n * sizeof(cxAABB));
	for (int i = 0; i < n; ++i) {
		cxVec sc = s_pPts[i];
		float r = s_pRadii[i];
		s_pSrcBoxes[i].from_sph(cxSphere(sc, r));
	}
}

static void dump_box_pts(cxAABB& bb, FILE* pOut = stdout) {
	cxVec vmin = bb.get_min_pos();
	cxVec vmax = bb.get_max_pos();

	::fprintf(pOut, "%f %f %f 1\n", vmin.x, vmin.y, vmin.z);
	::fprintf(pOut, "%f %f %f 1\n", vmin.x, vmax.y, vmin.z);
	::fprintf(pOut, "%f %f %f 1\n", vmax.x, vmax.y, vmin.z);
	::fprintf(pOut, "%f %f %f 1\n", vmax.x, vmin.y, vmin.z);

	::fprintf(pOut, "%f %f %f 1\n", vmin.x, vmin.y, vmax.z);
	::fprintf(pOut, "%f %f %f 1\n", vmin.x, vmax.y, vmax.z);
	::fprintf(pOut, "%f %f %f 1\n", vmax.x, vmax.y, vmax.z);
	::fprintf(pOut, "%f %f %f 1\n", vmax.x, vmin.y, vmax.z);
}

static void dump_src_boxes(FILE* pOut = stdout) {
	if (g_dump != 2) return;
	if (!pOut) return;
	int n = g_nsrc;
	int npnt = n * 8;
	int npol = n * 12;
	::fprintf(pOut, "PGEOMETRY V5\n");
	::fprintf(pOut, "NPoints %d NPrims %d\n", npnt, npol);
	::fprintf(pOut, "NPointGroups 0 NPrimGroups 0\n");
	::fprintf(pOut, "NPointAttrib 0 NVertexAttrib 0 NPrimAttrib 0 NAttrib 0\n");
	
	for (int i = 0; i < n; ++i) {
		dump_box_pts(s_pSrcBoxes[i]);
	}

	::fprintf(pOut, "Run %d Poly\n", npol);
	for (int i = 0; i < n; ++i) {
		int org = i*8;
		for (int j = 0; j < 2; ++j) {
			for (int k = 0; k < 4; ++k) {
				int v0 = org + j*4 + k;
				int v1 = v0 + (k < 3 ? 1 : -3);
				::fprintf(pOut, " 2 < %d %d\n", v0, v1);
			}
		}
		for (int j = 0; j < 4; ++j) {
			::fprintf(pOut, " 2 < %d %d\n", org + j, org + j + 4);
		}
	}

	::fprintf(pOut, "beginExtra\n");
	::fprintf(pOut, "endExtra\n");
}

static cxAABB* s_pNodeBoxes = nullptr;
static int32_t* s_pNodeInfos = nullptr;
static int32_t* s_pIdxWk = nullptr;

static void alloc_tree_mem() {
	int nsrc = g_nsrc;
	int nnodes = nsrc*2 - 1;
	s_pNodeBoxes = (cxAABB*)nxCore::mem_alloc(nnodes * sizeof(cxAABB));
	s_pNodeInfos = (int32_t*)nxCore::mem_alloc(nnodes * 2 * sizeof(int32_t));
	s_pIdxWk = (int32_t*)nxCore::mem_alloc(nsrc * sizeof(int32_t));
}

XD_NOINLINE static void make_tree() {
	nxGeom::build_aabb_tree(s_pSrcBoxes, g_nsrc, s_pNodeBoxes, s_pNodeInfos, s_pIdxWk);
}

static void dump_tree(FILE* pOut = stdout) {
	if (g_dump != 3) return;
	if (!pOut) return;
	int n = g_nsrc*2 - 1;
	int npnt = n * 8;
	int npol = n * 12;
	::fprintf(pOut, "PGEOMETRY V5\n");
	::fprintf(pOut, "NPoints %d NPrims %d\n", npnt, npol);
	::fprintf(pOut, "NPointGroups 0 NPrimGroups 0\n");
	::fprintf(pOut, "NPointAttrib 0 NVertexAttrib 0 NPrimAttrib 0 NAttrib 0\n");
	
	for (int i = 0; i < n; ++i) {
		dump_box_pts(s_pNodeBoxes[i]);
	}

	::fprintf(pOut, "Run %d Poly\n", npol);
	for (int i = 0; i < n; ++i) {
		int org = i*8;
		for (int j = 0; j < 2; ++j) {
			for (int k = 0; k < 4; ++k) {
				int v0 = org + j*4 + k;
				int v1 = v0 + (k < 3 ? 1 : -3);
				::fprintf(pOut, " 2 < %d %d\n", v0, v1);
			}
		}
		for (int j = 0; j < 4; ++j) {
			::fprintf(pOut, " 2 < %d %d\n", org + j, org + j + 4);
		}
	}

	::fprintf(pOut, "beginExtra\n");
	::fprintf(pOut, "endExtra\n");
}

int main(int argc, char* argv[]) {
	nxApp::init_params(argc, argv);
	init_sys();

	g_silent = nxApp::get_bool_opt("silent", false);
	g_dump = nxApp::get_int_opt("dump", 0);
	g_nsrc = nxCalc::max(nxApp::get_int_opt("n", 10), 10);

	make_geo();
	dump_geo();

	make_src_boxes();
	dump_src_boxes();

	nxCore::dbg_msg("Building tree for %d boxes...\n", g_nsrc);
	alloc_tree_mem();
	double t0 = nxSys::time_micros();
	make_tree();
	double dt = nxSys::time_micros() - t0;
	nxCore::dbg_msg("dt: %f millis\n", dt * 1e-3f);
	dump_tree();


	nxApp::reset();
	//nxCore::mem_dbg();
	reset_sys();
	return 0;
}
