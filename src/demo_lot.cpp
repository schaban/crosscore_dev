#include "crosscore.hpp"
#include "scene.hpp"
#include "demo.hpp"
#include "smprig.hpp"
#include "smpchar.hpp"

DEMO_PROG_BEGIN

static cxStopWatch s_execStopWatch;
static float s_medianExecMillis = -1.0f;
static int s_exerep = 1;

static struct STAGE {
	Pkg* pPkg;
	sxCollisionData* pCol;
} s_stage = {};

static void add_stg_obj(sxModelData* pMdl, void* pWkData) {
	ScnObj* pObj = Scene::add_obj(pMdl);
	if (pObj) {
		pObj->set_base_color_scl(1.0f);
	}
}

static void init_stage() {
	Pkg* pPkg = Scene::load_pkg("slope_room");
	s_stage.pPkg = pPkg;
	s_stage.pCol = nullptr;
	if (pPkg) {
		Scene::for_all_pkg_models(pPkg, add_stg_obj, &s_stage);
		SmpCharSys::set_collision(pPkg->find_collision("col"));
	}
}

static void char_roam_ctrl(SmpChar* pChar) {
	if (!pChar) return;
	double objTouchDT = pChar->get_obj_touch_duration_secs();
	double wallTouchDT = pChar->get_wall_touch_duration_secs();
	switch (pChar->mAction) {
		case SmpChar::ACT_STAND:
			if (pChar->check_act_time_out()) {
				if ((Scene::glb_rng_next() & 0x3F) < 0xF) {
					pChar->change_act(SmpChar::ACT_RUN, 2.0f);
				} else {
					pChar->change_act(SmpChar::ACT_WALK, 5.0f);
					pChar->reset_wall_touch();
				}
			}
			break;
		case SmpChar::ACT_WALK:
			if (pChar->check_act_time_out() || objTouchDT > 0.2 || wallTouchDT > 0.25) {
				if (objTouchDT > 0 && ((Scene::glb_rng_next() & 0x3F) < 0x1F)) {
					pChar->change_act(SmpChar::ACT_RETREAT, 0.5f);
				} else {
					float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 0.5f, 2.0f);
					if (Scene::glb_rng_next() & 0x1) {
						pChar->change_act(SmpChar::ACT_TURN_L, t);
					} else {
						pChar->change_act(SmpChar::ACT_TURN_R, t);
					}
				}
			}
			break;
		case SmpChar::ACT_RUN:
			if (pChar->check_act_time_out() || wallTouchDT > 0.1 || objTouchDT > 0.1) {
				pChar->change_act(SmpChar::ACT_STAND, 2.0f);
			}
			break;
		case SmpChar::ACT_RETREAT:
			if (pChar->check_act_time_out() || wallTouchDT > 0.1) {
				pChar->change_act(SmpChar::ACT_STAND, 2.0f);
			}
			break;
		case SmpChar::ACT_TURN_L:
		case SmpChar::ACT_TURN_R:
			if (pChar->check_act_time_out()) {
				pChar->change_act(SmpChar::ACT_STAND, 1.0f);
			}
			break;
	}
}

static void init_chars() {
	SmpChar::Descr descr;
	descr.reset();
	bool disableSl = nxApp::get_bool_opt("scl_off", false);

	float x = -5.57f;
	for (int i = 0; i < 10; ++i) {
		descr.variation = i;
		if (disableSl) {
			descr.scale = 1.0f;
		} else {
			if (!(i & 1)) {
				descr.scale = 0.95f;
			} else {
				descr.scale = 1.0f;
			}
		}
		ScnObj* pObj = SmpCharSys::add_f(descr, char_roam_ctrl);
		pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), cxVec(x, 0.0f, 0.0f));
		x += 0.7f;
	}

	cxVec pos(-4.6f, 0.0f, 7.0f);
	cxQuat q = nxQuat::from_degrees(0.0f, 110.0f, 0.0f);
	cxVec add = q.apply(cxVec(0.7f, 0.0f, 0.0f));
	for (int i = 0; i < 10; ++i) {
		descr.variation = i;
		if (disableSl) {
			descr.scale = 1.0f;
		} else {
			if (!(i & 1)) {
				descr.scale = 1.0f;
			} else {
				descr.scale = 1.04f;
			}
		}
		ScnObj* pObj = SmpCharSys::add_m(descr, char_roam_ctrl);
		if (pObj) {
			pObj->set_world_quat_pos(q, pos);
		}
		pos += add;
	}

	int mode = nxApp::get_int_opt("mode", 0);
	if (mode == 1) {
		cxVec pos(-8.0f, 0.0f, -2.0f);
		cxVec add(0.75f, 0.0f, 0.0f);
		cxQuat q = nxQuat::identity();
		for (int i = 0; i < 20; ++i) {
			ScnObj* pObj = nullptr;
			descr.scale = 1.0f;
			descr.variation = 9 - (i >> 1);
			if (i & 1) {
				pObj = SmpCharSys::add_f(descr, char_roam_ctrl);
			} else {
				pObj = SmpCharSys::add_m(descr, char_roam_ctrl);
			}
			if (pObj) {
				pObj->set_world_quat_pos(q, pos);
			}
			pos += add;
			if (i == 9) {
				pos.x = -8.0f;
				pos.z += -1.25f;
			}
		}
	}
}


static void init() {
	//Scene::alloc_global_heap(1024 * 1024 * 2);
	Scene::alloc_local_heaps(1024 * 1024 * 2);
	SmpCharSys::init();
	init_chars();
	init_stage();
	s_execStopWatch.alloc(120);
	Scene::enable_split_move(nxApp::get_bool_opt("split_move", false));
	nxCore::dbg_msg("Scene::split_move: %s\n", Scene::is_split_move_enabled() ? "Yes" : "No");
	s_exerep = nxCalc::clamp(nxApp::get_int_opt("exerep", 1), 1, 100);
	nxCore::dbg_msg("exerep: %d\n", s_exerep);
	nxCore::dbg_msg("Scene::speed: %.2f\n", Scene::speed());
}

static struct ViewWk {
	float t;
	float dt;

	void update() {
		t += dt * SmpCharSys::get_motion_speed() * 2.0f;
		if (t > 1.0f) {
			t = 1.0f;
			dt = -dt;
		} else if (t <= 0.0f) {
			t = 0.0f;
			dt = -dt;
		}
	}
} s_view;

static void view_exec() {
	if (s_view.dt == 0.0f) {
		s_view.t = 0.0f;
		s_view.dt = 0.0025f;
	}
	float tz = nxCalc::ease(s_view.t);
	cxVec tgt = cxVec(0.0f, 1.0f, nxCalc::fit(tz, 0.0f, 1.0f, 0.0f, 6.0f));
	cxVec pos = cxVec(3.9f, 1.9f, 3.5f);
	Scene::set_view(pos, tgt);
	s_view.update();
}

static void set_scene_ctx() {
	Scene::set_shadow_dir_degrees(70.0f, 140.0f);
	Scene::set_shadow_proj_params(20.0f, 10.0f, 50.0f);
	Scene::set_shadow_fade(28.0f, 35.0f);
	Scene::set_shadow_uniform(false);

	Scene::set_spec_dir_to_shadow();
	Scene::set_spec_shadowing(0.75f);

	Scene::set_hemi_upper(2.5f, 2.46f, 2.62f);
	Scene::set_hemi_lower(0.32f, 0.28f, 0.26f);
	Scene::set_hemi_up(Scene::get_shadow_dir().neg_val());
	Scene::set_hemi_exp(2.5f);
	Scene::set_hemi_gain(0.7f);

	Scene::set_fog_rgb(0.748f, 0.79f, 0.87f);
	Scene::set_fog_density(1.0f);
	Scene::set_fog_range(1.0f, 200.0f);
	Scene::set_fog_curve(0.1f, 0.1f);

	Scene::set_exposure_rgb(0.75f, 0.85f, 0.5f);

	Scene::set_linear_white_rgb(0.65f, 0.65f, 0.55f);
	Scene::set_linear_gain(1.32f);
	Scene::set_linear_bias(-0.025f);
}

static void draw_2d() {
	char str[512];
	const char* fpsStr = "FPS: ";
	const char* exeStr = "EXE: ";
	float refSizeX = 550.0f;
	float refSizeY = 350.0f;
	Scene::set_ref_scr_size(refSizeX, refSizeY);
	if (Scene::get_view_mode_width() < Scene::get_view_mode_height()) {
		Scene::set_ref_scr_size(refSizeY, refSizeX);
	}

	float sx = 10.0f;
	float sy = Scene::get_ref_scr_height() - 20.0f;

	xt_float2 bpos[4];
	xt_float2 btex[4];
	btex[0].set(0.0f, 0.0f);
	btex[1].set(1.0f, 0.0f);
	btex[2].set(1.0f, 1.0f);
	btex[3].set(0.0f, 1.0f);
	float bx = 4.0f;
	float by = 4.0f;
	float bw = 280.0f;
	float bh = 12.0f;
	cxColor bclr(0.0f, 0.0f, 0.0f, 0.75f);
	bpos[0].set(sx - bx, sy - by);
	bpos[1].set(sx + bw + bx, sy - by);
	bpos[2].set(sx + bw + bx, sy + bh + by);
	bpos[3].set(sx - bx, sy + bh + by);
	Scene::quad(bpos, btex, bclr);

	float fps = SmpCharSys::get_fps();
	if (fps <= 0.0f) {
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s--", fpsStr);
	} else {
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s%.2f", fpsStr, fps);
	}
	Scene::print(sx, sy, cxColor(0.1f, 0.75f, 0.1f, 1.0f), str);
	sx += 120.0f;

	float exe = s_medianExecMillis;
	if (exe <= 0.0f) {
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s--", exeStr);
	} else {
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s%.4f millis", exeStr, exe);
	}
	Scene::print(sx, sy, cxColor(0.5f, 0.4f, 0.1f, 1.0f), str);
}

static void profile_start() {
	s_execStopWatch.begin();
}

static void profile_end() {
	if (s_execStopWatch.end()) {
		double us = s_execStopWatch.median();
		s_execStopWatch.reset();
		double millis = us / 1000.0;
		s_medianExecMillis = float(millis);
		Scene::thermal_info();
		Scene::battery_info();
	}
}

static void scn_exec() {
	for (int i = 0; i < s_exerep; ++i) {
		Scene::exec();
	}
}

static void loop(void* pLoopCtx) {
	SmpCharSys::start_frame();
	set_scene_ctx();
	profile_start();
	scn_exec();
	view_exec();
	Scene::visibility();
	profile_end();
	Scene::frame_begin(cxColor(0.25f));
	Scene::draw();
	draw_2d();
	Scene::frame_end();
}

static void reset() {
	SmpCharSys::reset();
	s_execStopWatch.free();
}

DEMO_REGISTER(lot);

DEMO_PROG_END
