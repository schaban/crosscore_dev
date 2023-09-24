// g++ -pthread -I ../.. ../../crosscore.cpp perf_isect.cpp -o perf_isect -O3 -flto

#include "crosscore.hpp"

static float g_hitpct = 0.7f;
static float g_radius = 1.0f;
static float g_height = 1.0f;
static int g_nprims = 10;
static int g_nrays = 10;
static int g_dump = 0;
static bool g_silent = false;

static cxVec* s_pGeoPts = nullptr;
static int32_t* s_pGeoIdx = nullptr;
static cxVec* s_pRayPts = nullptr;
static bool* s_pHitRes = nullptr;
static cxVec* s_pHitPts = nullptr;

static int s_nhits = 0;

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

static xt_int4 get_quad_vertices(const int qi) {
	xt_int4 ivtx;
	ivtx.fill(-1);
	if (qi < g_nprims) {
		int i0 = qi;
		int i1 = (i0 + 1) % g_nprims;
		int i2 = i1 + g_nprims;
		int i3 = i0 + g_nprims;
		ivtx.set(i0, i1, i2, i3);
	}
	return ivtx;
}

static void build_quads() {
	if (g_nprims < 3) return;
	int npnt = g_nprims * 2;
	s_pGeoPts = (cxVec*)nxCore::mem_alloc(npnt*sizeof(cxVec), "geo:pts");
	cxVec dv(0.0f, 0.0f, -1.0f);
	float dy = 0.0f;
	for (int i = 0; i < g_nprims; ++i) {
		s_pGeoPts[i] = nxVec::rot_deg_y(dv, dy) * g_radius;
		dy += 360.0f / float(g_nprims);
	}
	for (int i = 0; i < g_nprims; ++i) {
		int itop = i + g_nprims;
		s_pGeoPts[itop] = s_pGeoPts[i];
		s_pGeoPts[itop].y += g_height;
	}
	int nidx = g_nprims * 4;
	s_pGeoIdx = (int32_t*)nxCore::mem_alloc(nidx*sizeof(int), "geo:idx");
	for (int i = 0; i < g_nprims; ++i) {
		xt_int4 ivtx = get_quad_vertices(i);
		for (int j = 0; j < 4; ++j) {
			s_pGeoIdx[i*4 + j] = ivtx[j];
		}
	}
}

static void build_rays() {
	if (g_nrays < 1) return;
	int npnt = g_nrays * 2;
	s_pRayPts = (cxVec*)nxCore::mem_alloc(npnt*sizeof(cxVec), "ray:pts");
	cxVec dv(0.0f, 0.0f, -1.0f);
	float dy = 0.0f;
	float oy = g_height*0.5f;
	for (int i = 0; i < g_nrays; ++i) {
		float scl = 1.5f;
		if (i >= int(float(g_nrays) * g_hitpct)) {
			scl = 0.5f;
		}
		cxVec p0(0.0f);
		cxVec p1 = nxVec::rot_deg_y(dv, dy + 2.0f) * g_radius * scl;
		p0 += p1.get_normalized() * (g_radius * 0.05f);
		p0.y += oy;
		p1.y += oy;
		s_pRayPts[i*2] = p0;
		s_pRayPts[i*2 + 1] = p1;
		dy += 360.0f / float(g_nrays);
	}
}

static void write_quads(FILE* pOut = stdout) {
	if (g_dump != 1) return;
	if (!pOut) return;
	if (!s_pGeoPts) return;
	int npnt = g_nprims * 2;
	::fprintf(pOut, "PGEOMETRY V5\n");
	::fprintf(pOut, "NPoints %d NPrims %d\n", npnt, g_nprims);
	::fprintf(pOut, "NPointGroups 0 NPrimGroups 0\n");
	::fprintf(pOut, "NPointAttrib 0 NVertexAttrib 0 NPrimAttrib 0 NAttrib 0\n");
	for (int i = 0; i < npnt; ++i) {
		cxVec pos = s_pGeoPts[i];
		::fprintf(pOut, "%f %f %f 1\n", pos.x, pos.y, pos.z);
	}
	::fprintf(pOut, "Run %d Poly\n", g_nprims);
	for (int i = 0; i < g_nprims; ++i) {
		::fprintf(pOut, " 4 <");
		for (int j = 0; j < 4; ++j) {
			::fprintf(pOut, " %i", s_pGeoIdx[i*4 + j]);
		}
		::fprintf(pOut, "\n");
	}
	::fprintf(pOut, "beginExtra\n");
	::fprintf(pOut, "endExtra\n");
}

static void write_rays(FILE* pOut = stdout) {
	if (g_dump != 2) return;
	if (!pOut) return;
	if (!s_pRayPts) return;
	int npnt = g_nrays * 2;
	::fprintf(pOut, "PGEOMETRY V5\n");
	::fprintf(pOut, "NPoints %d NPrims %d\n", npnt, g_nrays);
	::fprintf(pOut, "NPointGroups 0 NPrimGroups 0\n");
	::fprintf(pOut, "NPointAttrib 0 NVertexAttrib 0 NPrimAttrib 0 NAttrib 0\n");
	for (int i = 0; i < npnt; ++i) {
		cxVec pos = s_pRayPts[i];
		::fprintf(pOut, "%f %f %f 1\n", pos.x, pos.y, pos.z);
	}
	::fprintf(pOut, "Run %d Poly\n", g_nrays);
	for (int i = 0; i < g_nrays; ++i) {
		::fprintf(pOut, " 2 > %d %d\n", i*2, i*2 + 1);
	}
	::fprintf(pOut, "beginExtra\n");
	::fprintf(pOut, "endExtra\n");
}

static void init_hits() {
	int npnt = g_nrays;
	s_pHitPts = (cxVec*)nxCore::mem_alloc(npnt*sizeof(cxVec), "hit:pts");
	s_pHitRes = (bool*)nxCore::mem_alloc(npnt*sizeof(bool), "hit:res");
	for (int i = 0; i < npnt; ++i) {
		s_pHitPts[i].fill(-1e5f);
		s_pHitRes[i] = false;
	}
}

XD_NOINLINE static void exec_ray_quad_isect(const int iray, const int iquad) {
	cxVec p0 = s_pRayPts[iray*2];
	cxVec p1 = s_pRayPts[iray*2 + 1];
	int i0 = s_pGeoIdx[iquad*4];
	int i1 = s_pGeoIdx[iquad*4 + 1];
	int i2 = s_pGeoIdx[iquad*4 + 2];
	int i3 = s_pGeoIdx[iquad*4 + 3];
	cxVec v0 = s_pGeoPts[i0];
	cxVec v1 = s_pGeoPts[i1];
	cxVec v2 = s_pGeoPts[i2];
	cxVec v3 = s_pGeoPts[i3];
	cxVec hitPos;
	bool res = nxGeom::seg_quad_intersect_cw(p0, p1, v0, v1, v2, v3, &hitPos);
	if (res) {
		s_pHitPts[iray] = hitPos;
		s_pHitRes[iray] = true;
		++s_nhits;
	}
}

XD_NOINLINE static void calc_hits() {
	double t0 = nxSys::time_micros();
	s_nhits = 0;
	for (int iray = 0; iray < g_nrays; ++iray) {
		for (int iquad = 0; iquad < g_nprims; ++iquad) {
			exec_ray_quad_isect(iray, iquad);
		}
	}
	double dt = nxSys::time_micros() - t0;
	nxCore::dbg_msg("%d rays x %d prims = %d intersection checks, %d%% hits\n", g_nrays, g_nprims, g_nrays * g_nprims, int(g_hitpct*1e2f));
	nxCore::dbg_msg("dt: %f millis\n", dt * 1e-3f);
}

static void dump_hits(FILE* pOut = stdout) {
	if (g_dump != 3) return;
	::fprintf(pOut, "PGEOMETRY V5\n");
	::fprintf(pOut, "NPoints %d NPrims 0\n", s_nhits);
	::fprintf(pOut, "NPointGroups 0 NPrimGroups 0\n");
	::fprintf(pOut, "NPointAttrib 0 NVertexAttrib 0 NPrimAttrib 0 NAttrib 0\n");
	for (int iray = 0; iray < g_nrays; ++iray) {
		if (s_pHitRes[iray]) {
			cxVec pos = s_pHitPts[iray];
			::fprintf(pOut, "%f %f %f 1\n", pos.x, pos.y, pos.z);
		}
	}
	::fprintf(pOut, "beginExtra\n");
	::fprintf(pOut, "endExtra\n");
}

int main(int argc, char* argv[]) {
	nxApp::init_params(argc, argv);
	init_sys();

	g_radius = nxApp::get_float_opt("radius", 1.0f);
	g_hitpct = nxApp::get_float_opt("hitpct", 0.7f);
	g_nprims = nxApp::get_int_opt("nprims", 100);
	g_nrays = nxApp::get_int_opt("nrays", g_nprims * 100);
	g_dump = nxApp::get_int_opt("dump", 0);
	g_silent = nxApp::get_bool_opt("silent", false);

	build_quads();
	write_quads();

	build_rays();
	write_rays();

	init_hits();
	calc_hits();
	dump_hits();

	nxApp::reset();
	//nxCore::mem_dbg();
	reset_sys();
	return 0;
}
