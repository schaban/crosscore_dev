// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2021-2022 Sergey Chaban <sergey.chaban@gmail.com>

#include "crosscore.hpp"
#include "scene.hpp"
#include "smprig.hpp"
#include "smpchar.hpp"


void SmpChar::Rig::init(SmpChar* pChar) {
	mpChar = nullptr;
	mpObj = nullptr;
	if (!pChar) return;
	ScnObj* pObj = pChar->mpObj;
	if (!pObj) return;
	mpChar = pChar;
	sxValuesData* pVals = pObj->find_values("params");
	SmpRig::init(pObj, pVals);
}

void SmpChar::Rig::exec() {
	SmpChar* pChar = mpChar;
	if (!pChar) return;
	sxCollisionData* pCol = SmpCharSys::get_collision();
	SmpRig::exec(pCol);
}

void SmpChar::MotLib::init(const Pkg* pPkg) {
	struct {
		sxMotionData** ppMot;
		const char* pName;
	} tbl[] = {
		{ &pStand, "stand" },
		{ &pTurnL, "turn_l" },
		{ &pTurnR, "turn_r" },
		{ &pWalk, "walk" },
		{ &pRetreat, "retreat" },
		{ &pRun, "run" }
	};
	for (size_t i = 0; i < XD_ARY_LEN(tbl); ++i) {
		*tbl[i].ppMot = pPkg ? pPkg->find_motion(tbl[i].pName) : nullptr;
	}
}


static bool char_pkg_ck(const SmpChar* pChr, const char* pPkgName) {
	bool res = false;
	if (pChr && pPkgName) {
		ScnObj* pObj = pChr->mpObj;
		if (pObj) {
			sxModelData* pMdl = pObj->get_model_data();
			if (pMdl) {
				Pkg* pPkg = Scene::find_pkg_for_data(pMdl);
				if (pPkg) {
					res = nxCore::str_eq(pPkg->get_name(), pPkgName);
				}
			}
		}
	}
	return res;
}

bool SmpChar::is_f() const {
	return char_pkg_ck(this, "smp_f");
}

bool SmpChar::is_m() const {
	return char_pkg_ck(this, "smp_m");
}

double SmpChar::get_act_time() const {
	return SmpCharSys::get_current_time() - mActionStartTime;
}

bool SmpChar::check_act_time_out() const {
	return get_act_time() > mActionDuration;
}

void SmpChar::change_act(const int newAct, const double durationSecs) {
	if (!mpObj) return;
	int blendCnt = 15;
	float t = nxCalc::div0(float(blendCnt), SmpCharSys::get_motion_speed() * 2.0f);
	mpObj->init_motion_blend(int(t));
	mAction = newAct;
	mActionStartTime = SmpCharSys::get_current_time();
	set_act_duration_seconds(durationSecs);
}

void SmpChar::add_deg_y(const float dy) {
	if (!mpObj) return;
	mpObj->add_world_deg_y(dy * SmpCharSys::get_motion_speed() * 2.0f);
}

static bool char_obj_adj_for_each(ScnObj* pObj, void* pCharMem) {
	if (!pObj) return true;
	SmpChar* pChar = (SmpChar*)pCharMem;
	if (pChar->mpObj == pObj) return true;
	float rchar = pChar->mpObj->mObjAdjRadius;
	float robj = pObj->mObjAdjRadius;
	if (!(rchar > 0.0f && robj > 0.0f)) return true;
	cxVec objWPos = pObj->get_prev_world_pos();
	cxVec objPos = objWPos;
	cxVec wpos = pChar->mpObj->get_world_pos();
	cxVec npos = wpos;
	cxVec opos = pChar->mpObj->get_prev_world_pos();
	npos.y += pChar->mpObj->mObjAdjYOffs;
	opos.y += pChar->mpObj->mObjAdjYOffs;
	objPos.y += pObj->mObjAdjYOffs;
	cxVec adjPos = npos;
	bool adjFlg = Scene::sph_sph_adj(npos, opos, rchar, objPos, robj, &adjPos);
	if (adjFlg) {
		pChar->mpObj->set_skel_root_local_tx(adjPos.x);
		pChar->mpObj->set_skel_root_local_tz(adjPos.z);
		++pChar->mObjAdjCount;
	}
	return true;
}

void SmpChar::obj_adj() {
	if (!mpObj) return;
	mObjAdjCount = 0;
	Scene::for_each_obj(char_obj_adj_for_each, this);
	if (mObjAdjCount > 0) {
		++mObjTouchCount;
	} else {
		mObjTouchCount = 0;
	}
}

void SmpChar::wall_adj() {
	if (!mpObj) return;
	sxCollisionData* pCol = SmpCharSys::get_collision();
	if (!pCol) {
		mWallTouchCount = 0;
		return;
	}
	cxVec wpos = mpObj->get_world_pos();
	cxVec npos = wpos;
	cxVec opos = mpObj->get_prev_world_pos();
	cxVec apos = wpos;
	float yoffs = mpObj->mObjAdjYOffs;
	npos.y += yoffs;
	opos.y += yoffs;
	float radius = mpObj->mObjAdjRadius * 1.1f;
	bool touchFlg = Scene::wall_touch(mpObj->mpJobCtx, pCol, npos, opos, radius * 1.025f);
	if (touchFlg) {
		++mWallTouchCount;
	} else {
		mWallTouchCount = 0;
	}
	bool adjFlg = Scene::wall_adj(mpObj->mpJobCtx, pCol, npos, opos, radius, &apos);
	if (adjFlg) {
		mpObj->set_skel_root_local_pos(apos.x, wpos.y, apos.z);
	}
}

void SmpChar::ground_adj() {
	ScnObj* pObj = mpObj;
	if (!pObj) return;
	sxCollisionData* pCol = SmpCharSys::get_collision();
	if (!pCol) return;
	cxVec wpos = pObj->get_world_pos();
	float y = Scene::get_ground_height(pCol, wpos);
	pObj->set_skel_root_local_ty(y);
}

void SmpChar::exec_collision() {
	if (!mpObj) return;
	bool objTouchOngoing = mObjTouchCount > 0;
	bool wallTouchOngoing = mWallTouchCount > 0;
	obj_adj();
	wall_adj();
	ground_adj();
	if (mObjTouchCount > 0) {
		if (objTouchOngoing) {
			mObjTouchDuration = SmpCharSys::get_current_time() - mObjTouchStartTime;
		} else {
			mObjTouchStartTime = SmpCharSys::get_current_time();
			mObjTouchDuration = 0.0f;
		}
	} else {
		mObjTouchDuration = 0.0f;
	}
	if (mWallTouchCount > 0) {
		if (wallTouchOngoing) {
			mWallTouchDuration = SmpCharSys::get_current_time() - mWallTouchStartTime;
		} else {
			mWallTouchStartTime = SmpCharSys::get_current_time();
			mWallTouchDuration = 0.0f;
		}
	} else {
		mWallTouchDuration = 0.0f;
	}
}


namespace SmpCharSys {

static bool time_fix_up_obj_func(ScnObj* pObj, void* pMem) {
	SmpChar* pChar = char_from_obj(pObj);
	if (pChar) {
		pChar->mActionStartTime = get_current_time();
	}
	return true;
}

static double get_sys_time_millis() {
	return nxSys::time_micros() / 1000.0;
}

static struct Wk {
	Pkg* mpPkgF;
	Pkg* mpPkgM;
	int mCount;
	sxCollisionData* mpCol;
	float mMotSpeed;
	double mCurrentTime;
	bool mTimeFixUpFlg;
	bool mFixedFreq;
	bool mLowQ;
	cxStopWatch mFramerateStopWatch;
	float mMedianFPS;

	void init() {
		mpPkgF = Scene::load_pkg("smp_f");
		mpPkgM = Scene::load_pkg("smp_m");
		mMotSpeed = 0.5f;
		mTimeFixUpFlg = true;
		mFramerateStopWatch.alloc(10);
		mMedianFPS = 0.0f;
		mCurrentTime = 0.0;
		mFixedFreq = nxApp::get_bool_opt("fixfreq", false);
		mLowQ = nxApp::get_bool_opt("lowq", false);
	}

	void reset() {
		mFramerateStopWatch.free();
	}

	void timer_ctrl() {
		if (!mFixedFreq) {
			mCurrentTime = get_sys_time_millis();
		}

		if (mTimeFixUpFlg) {
			Scene::for_each_obj(time_fix_up_obj_func, nullptr);
			mFramerateStopWatch.begin();
			mTimeFixUpFlg = false;
		} else {
			if (mFramerateStopWatch.end()) {
				double us = mFramerateStopWatch.median();
				mFramerateStopWatch.reset();
				double millis = us / 1000.0;
				double secs = millis / 1000.0;
				double fps = nxCalc::rcp0(secs);
				mMedianFPS = float(fps);
			}
			mFramerateStopWatch.begin();
		}
		if (mFixedFreq) {
			mCurrentTime += 1000.0 / 60.0;
			mMotSpeed = 0.5f;
		} else {
			if (mMedianFPS > 0.0f) {
				if (mMedianFPS >= 57.0f && mMedianFPS <= 62.0f) {
					mMotSpeed = 0.5f;
				} else if (mMedianFPS >= 29.0f && mMedianFPS <= 31.0f) {
					mMotSpeed = 1.0f;
				} else {
					mMotSpeed = nxCalc::div0(60.0f, mMedianFPS) * 0.5f;
				}
			} else {
				mMotSpeed = 0.5f;
			}
		}
	}
} s_wk = {};

static bool s_initFlg = false;

static const char* s_pCharTag = "SmpChar";

void init() {
	if (s_initFlg) return;
	s_wk.init();
	s_initFlg = true;
}

void reset() {
	if (!s_initFlg) return;
	s_wk.reset();
	s_initFlg = false;
}



void start_frame() {
	if (!s_initFlg) return;
	s_wk.timer_ctrl();
}

double get_current_time() {
	return s_wk.mCurrentTime;
}

float get_motion_speed() {
	return s_wk.mMotSpeed * Scene::speed();
}

float get_fps() {
	return s_wk.mMedianFPS;
}

bool obj_is_char(ScnObj* pObj) {
	bool res = false;
	if (pObj) {
		if (pObj->get_ptr_wk<void>(0)) {
			const char* pTag = pObj->get_ptr_wk<const char>(1);
			if (pTag) {
				res = (pTag == s_pCharTag);
			}
		}
	}
	return res;
}

SmpChar* char_from_obj(ScnObj* pObj) {
	return obj_is_char(pObj) ? pObj->get_ptr_wk<SmpChar>(0) : nullptr;
}

static void char_before_mot_func(ScnObj* pObj) {
	SmpChar* pChar = char_from_obj(pObj);
	if (!pChar) return;
	pChar->select_motion();
}

static void char_exec_func(ScnObj* pObj) {
	SmpChar* pChar = char_from_obj(pObj);
	if (!pChar) return;
	if (pChar->mCtrlFunc) {
		pChar->mCtrlFunc(pChar);
	}
	pObj->move(SmpCharSys::get_motion_speed());
}

static void char_del_func(ScnObj* pObj) {
	SmpChar* pChar = char_from_obj(pObj);
	if (!pChar) return;
	nxCore::tMem<SmpChar>::free(pChar);
	pObj->set_ptr_wk<void>(0, nullptr);
	pObj->set_ptr_wk<void>(1, nullptr);
}


static void char_before_blend_func(ScnObj* pObj) {
	SmpChar* pChar = char_from_obj(pObj);
	if (!pChar) return;
	pChar->exec_collision();
	pChar->mRig.exec();
}

static ScnObj* add_char(const Pkg* pPkg, const SmpChar::Descr& descr, SmpChar::CtrlFunc ctrl) {
	ScnObj* pObj = nullptr;
	if (s_initFlg && pPkg) {
		sxModelData* pMdl = pPkg->get_default_model();
		if (pMdl) {
			char nameBuf[64];
			const char* pName = descr.pName;
			if (!pName) {
				XD_SPRINTF(XD_SPRINTF_BUF(nameBuf, sizeof(nameBuf)), "%s@%03d", pPkg->get_name(), s_wk.mCount);
				pName = nameBuf;
			}
			pObj = Scene::add_obj(pMdl, pName);
		}
	}
	if (pObj) {
		float scale = descr.scale;
		if (scale <= 0.0f) scale = 1.0f;
		pObj->set_motion_uniform_scl(scale);
		pObj->set_model_variation(descr.variation);
		if (s_wk.mLowQ) {
			pObj->mDisableShadowRecv = true;
		}
		SmpChar* pChar = nxCore::tMem<SmpChar>::alloc(1, s_pCharTag);
		if (pChar) {
			pObj->set_ptr_wk(0, pChar);
			pObj->set_ptr_wk(1, s_pCharTag);
			pChar->mpObj = pObj;
			pChar->mMotLib.init(pPkg);
			pChar->mRig.init(pChar);
			pObj->mExecFunc = char_exec_func;
			pObj->mDelFunc = char_del_func;
			pObj->mBeforeMotionFunc = char_before_mot_func;
			pObj->mBeforeBlendFunc = char_before_blend_func;
			pObj->mObjAdjRadius = 0.26f * scale;
			pObj->mObjAdjYOffs = 1.0f * scale;
			pChar->mObjTouchCount = 0;
			pChar->mObjTouchStartTime = 0.0;
			pChar->mObjTouchDuration = 0.0;
			pChar->mWallTouchCount = 0;
			pChar->mWallTouchStartTime = 0.0;
			pChar->mWallTouchDuration = 0.0;
			pChar->mCtrlFunc = ctrl;
			pChar->mAction = SmpChar::ACT_STAND;
			pChar->mActionStartTime = s_wk.mFixedFreq ? 0.0 : get_sys_time_millis();
			pChar->set_act_duration_seconds(0.5f);
		}
	}
	if (pObj) {
		++s_wk.mCount;
	}
	return pObj;
}

ScnObj* add_f(const SmpChar::Descr& descr, SmpChar::CtrlFunc ctrl) {
	return add_char(s_wk.mpPkgF, descr, ctrl);
}

ScnObj* add_m(const SmpChar::Descr& descr, SmpChar::CtrlFunc ctrl) {
	return add_char(s_wk.mpPkgM, descr, ctrl);
}

bool obj_is_f(ScnObj* pObj) {
	bool res = false;
	SmpChar* pChr = char_from_obj(pObj);
	if (pChr) {
		res = pChr->is_f();
	}
	return res;
}

bool obj_is_m(ScnObj* pObj) {
	bool res = false;
	SmpChar* pChr = char_from_obj(pObj);
	if (pChr) {
		res = pChr->is_m();
	}
	return res;
}

void set_collision(sxCollisionData* pCol) {
	s_wk.mpCol = pCol;
}

sxCollisionData* get_collision() {
	return s_wk.mpCol;
}

} // SmpCharSys
