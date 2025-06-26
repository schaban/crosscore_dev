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



XD_NOINLINE static void test_tri_bc() {
	cxVec v0(0.0f, 0.0f, 0.0f);
	cxVec v1(1.0f, 0.0f, 0.0f);
	cxVec v2(0.0f, 0.0f, 1.0f);
	cxVec tn(0.0f, 1.0f, 0.0f);
	cxMtx xform = nxMtx::mk_rot_degrees(10.0f, 20.0, 30.0f) * nxMtx::mk_pos(1.1f, 2.2f, 3.3f);
	v0 = xform.calc_pnt(v0);
	v1 = xform.calc_pnt(v1);
	v2 = xform.calc_pnt(v2);
	tn = xform.calc_vec(tn).get_normalized();
	//tpos = xform.calc_pnt(tpos);
	float tu = 0.4f;
	float tv = 0.5f;
	float tw = 1.0f - tu - tv;
	cxVec tpos = v0*tu + v1*tv + v2*tw;
	float d0 = 0.75f;
	float d1 = 0.25f;
	cxVec p0 = tpos + (tn * d0);
	cxVec p1 = tpos - (tn * d1);
	cxVec hpos;
	cxVec hnrm;
	bool hit = nxGeom::seg_tri_intersect_cw(p0, p1, v0, v1, v2, &hpos, &hnrm);
	xt_float4 uvwt = nxGeom::seg_tri_intersect_bc_cw(p0, p1, v0, v1, v2);
	bool hitBC = uvwt.w >= 0.0f;
	if (hit) {
		cxVec uvw = nxGeom::barycentric(hpos, v0, v1, v2);
		if (hitBC) {
			cxVec hposBC = v0*uvwt.x + v1*uvwt.y + v2*uvwt.z;
			cxVec hposT = cxLineSeg(p0, p1).get_inner_pos(uvwt.w);
			float ckDistT = nxVec::dist(hpos, hposT);
			float ckDist = nxVec::dist(hpos, hposBC);
			if (!nxCore::f32_almost_eq(ckDist, 0.0f)) {
				nxCore::dbg_msg("!dist\n");
			}
		} else {
			nxCore::dbg_msg("!hit bc\n");
		}
	} else {
		nxCore::dbg_msg("!hit\n");
	}
}



int main(int argc, char* argv[]) {
	nxApp::init_params(argc, argv);
	init_sys();

	g_silent = nxApp::get_bool_opt("silent", false);

	test_tri_bc();

	nxApp::reset();
	reset_sys();
	return 0;
}
