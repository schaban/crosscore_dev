// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2021-2022 Sergey Chaban <sergey.chaban@gmail.com>

#include "crosscore.hpp"
#include "scene.hpp"
#include "smprig.hpp"

void SmpRig::LegInfo::init(const ScnObj* pObj, const char side) {
	char jname[64];
	bool disableIK = nxApp::get_bool_opt("ik_off", false);
	if (!disableIK && pObj) {
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

void SmpRig::init(ScnObj* pObj, sxValuesData* pVals) {
	mpObj = nullptr;
	if (!pObj) return;
	mpObj = pObj;
	mLegL.init(pObj, 'L');
	mLegR.init(pObj, 'R');
	struct {
		SupJnt* pJnt;
		const char* pName;
		const char* pDstName;
		const char* pSrcName;
		char axis;
	} tbl[] = {
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
	for (size_t i = 0; i < XD_ARY_LEN(tbl); ++i) {
		tbl[i].pJnt->reset();
	}
	mAnkleHeight = 0.01f;
	bool disableSupJnt = nxApp::get_bool_opt("sjnt_off", false);
	if (pVals) {
		sxValuesData::Group rigGrp = pVals->find_grp("rig");
		if (rigGrp.is_valid()) {
			if (!disableSupJnt) {
				for (size_t i = 0; i < XD_ARY_LEN(tbl); ++i) {
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
			}
			int ival = rigGrp.find_val_idx("AnkleHeight");
			if (rigGrp.ck_val_idx(ival)) {
				mAnkleHeight = rigGrp.get_val_f(ival);
			}
			ival = rigGrp.find_val_idx("HeightOffs");
			if (rigGrp.ck_val_idx(ival)) {
				float hoffs = rigGrp.get_val_f(ival);
				mpObj->set_motion_height_offs(hoffs);
			}
		}
	}
}

void SmpRig::exec(sxCollisionData* pCol) {
	ScnObj* pObj = mpObj;
	if (!pObj) return;
	cxMotionWork* pMotWk = pObj->mpMotWk;
	if (pCol && pMotWk) {
		float scale = pObj->get_motion_uniform_scl();
		float ankleHeight = mAnkleHeight * scale;
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
					float hy = hit.pos.y + ankleHeight;
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
	for (size_t i = 0; i < XD_ARY_LEN(jnts); ++i) {
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
