// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2021 Sergey Chaban <sergey.chaban@gmail.com>

#include "crosscore.hpp"
#include "scene.hpp"
#include "smpchar.hpp"

void SmpChar::Rig::LegInfo::init(const ScnObj* pObj, const char side) {
	char jname[64];
	if (pObj) {
		XD_SPRINTF(XD_SPRINTF_BUF(jname, sizeof(jname)), "j_Hip_%c", side);
		inodeTop = pObj->find_skel_node_id(jname);
		XD_SPRINTF(XD_SPRINTF_BUF(jname, sizeof(jname)), "j_Knee_%c", side);
		inodeRot = pObj->find_skel_node_id(jname);
		XD_SPRINTF(XD_SPRINTF_BUF(jname, sizeof(jname)), "j_Ankle_%c", side);
		inodeEnd = pObj->find_skel_node_id(jname);
	} else {
		inodeTop = -1;
		inodeRot = -1;
		inodeEnd = -1;
	}
}

void SmpChar::Rig::init(SmpChar* pChar) {
	mpChar = nullptr;
	if (!pChar) return;
	ScnObj* pObj = pChar->mpObj;
	if (!pObj) return;
	mpChar = pChar;
	mScale = 1.0f;
	mLegL.init(pObj, 'L');
	mLegR.init(pObj, 'R');
	struct {
		Rig::SupJnt* pJnt;
		const char* pName;
		const char* pDstName;
		const char* pSrcName;
		char axis;
	} tbl [] = {
		{ &mShoulderL, "Shoulder_L", "s_Shoulder_L", "j_Shoulder_L", 'z' },
		{ &mShoulderR, "Shoulder_R", "s_Shoulder_R", "j_Shoulder_R", 'z' },
		{ &mElbowL, "Elbow_L", "s_Elbow_L", "j_Elbow_L", 'y' },
		{ &mElbowR, "Elbow_R", "s_Elbow_R", "j_Elbow_R", 'y' },
		{ &mForearmL, "Forearm_L", "s_Forearm_L", "j_Wrist_L", 'x' },
		{ &mForearmR, "Forearm_R", "s_Forearm_R", "j_Wrist_R", 'x' },
		{ &mHipL, "Hip_L", "s_Hip_L", "j_Hip_L", 'x' },
		{ &mHipR, "Hip_R", "s_Hip_R", "j_Hip_R", 'x' },
		{ &mKneeL, "Knee_L", "s_Knee_L", "j_Knee_L", 'x' },
		{ &mKneeR, "Knee_R", "s_Knee_R", "j_Knee_R", 'x' }
	};
	for (int i = 0; i < XD_ARY_LEN(tbl); ++i) {
		tbl[i].pJnt->reset();
	}
	mAnkleHeight = 0.01f;
	sxValuesData* pVals = pObj->find_values("params");
	if (pVals) {
		sxValuesData::Group rigGrp = pVals->find_grp("rig");
		if (rigGrp.is_valid()) {
			for (int i = 0; i < XD_ARY_LEN(tbl); ++i) {
				int idst = pObj->find_skel_node_id(tbl[i].pDstName);
				int isrc = pObj->find_skel_node_id(tbl[i].pSrcName);
				int ival = rigGrp.find_val_idx(tbl[i].pName);
				if (pObj->ck_skel_id(idst) && pObj->ck_skel_id(isrc) && rigGrp.ck_val_idx(ival)) {
					SupJnt* pJnt = tbl[i].pJnt;
					pJnt->param = rigGrp.get_val_f3(ival);
					pJnt->idst = uint8_t(idst);
					pJnt->isrc = uint8_t(isrc);
					pJnt->axis = tbl[i].axis;
					pJnt->enabled = true;
				}
			}
			int ival = rigGrp.find_val_idx("AnkleHeight");
			if (rigGrp.ck_val_idx(ival)) {
				mAnkleHeight = rigGrp.get_val_f(ival);
			}
		}
	}
}

void SmpChar::Rig::exec() {
	SmpChar* pChar = mpChar;
	if (!pChar) return;
	ScnObj* pObj = mpChar->mpObj;
	if (!pObj) return;

	sxCollisionData* pCol = SmpCharSys::get_collision();
	cxMotionWork* pMotWk = pObj->mpMotWk;
	if (pCol && pMotWk) {
		for (int i = 0; i < 2; ++i) {
			LegInfo* pLeg = i ? &mLegL : &mLegR;
			if (pLeg->inodeEnd >= 0) {
				cxMtx effW = pMotWk->calc_node_world_mtx(pLeg->inodeEnd);
				cxVec legPos = effW.get_translation();
				cxVec legUp = legPos;
				cxVec legDn = legUp;
				legUp.y += 0.5f;
				legDn.y -= 0.5f;
				cxLineSeg seg(legUp, legDn);
				sxCollisionData::NearestHit hit = pCol->nearest_hit(seg);
				if (hit.count > 0) {
					float hy = hit.pos.y + (mAnkleHeight * mScale);
					float ay = legPos.y;
					if (hy > legPos.y) {
						ay = hy;
					}
					cxVec effPos(legPos.x, ay, legPos.z);
					pMotWk->adjust_leg(effPos, pLeg->inodeTop, pLeg->inodeRot, pLeg->inodeEnd, -1);
				}
			}
		}
	}

	SupJnt* jnts[] = {
		&mShoulderL,
		&mShoulderR,
		&mElbowL,
		&mElbowR,
		&mForearmL,
		&mForearmR,
		&mHipL,
		&mHipR,
		&mKneeL,
		&mKneeR
	};
	for (int i = 0; i < XD_ARY_LEN(jnts); ++i) {
		SupJnt* pJnt = jnts[i];
		if (pJnt->enabled) {
			cxQuat q = pObj->get_skel_local_quat(pJnt->isrc);
			cxVec degs = q.get_rot_degrees();
			float ang = 0.0f;
			int idx = -1;
			switch (pJnt->axis) {
				case 'x': idx = 0; break;
				case 'y': idx = 1; break;
				case 'z': idx = 2; break;
				default: break;
			}
			if (idx >= 0) {
				ang = degs[idx];
				ang += pJnt->param[0];
				ang *= pJnt->param[1];
				ang += pJnt->param[2];
				degs.zero();
				degs.set_at(idx, ang);
				q.set_rot_degrees(degs);
				pObj->set_skel_local_quat(pJnt->idst, q);
			}
		}
	}
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
	for (int i = 0; i < XD_ARY_LEN(tbl); ++i) {
		*tbl[i].ppMot = pPkg ? pPkg->find_motion(tbl[i].pName) : nullptr;
	}
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
	bool touchFlg = Scene::wall_adj(mpObj->mpJobCtx, pCol, npos, opos, radius * 1.025f, nullptr);
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
			pChar->mRig.mScale = scale;
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

ScnObj* add_f(const SmpChar::Descr& descr, SmpChar::CtrlFunc ctrl) { return add_char(s_wk.mpPkgF, descr, ctrl); }
ScnObj* add_m(const SmpChar::Descr& descr, SmpChar::CtrlFunc ctrl) { return add_char(s_wk.mpPkgM, descr, ctrl); }

void set_collision(sxCollisionData* pCol) {
	s_wk.mpCol = pCol;
}

sxCollisionData* get_collision() {
	return s_wk.mpCol;
}

} // SmpCharSys
