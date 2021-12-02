#include "crosscore.hpp"
#include "scene.hpp"
#include "demo.hpp"
#include "dynrig.hpp"
#include "oglsys.hpp"

#define SAMPLING_PERIOD (120*2)


DEMO_PROG_BEGIN

#define STG_NAME "roof"
#define COL_NAME "col"

#define EXM_EVAL_PREFIX "eval_"

// PtrWk indices
#define CHR_WK_IDX 0
#define ACTR_DLG_NAME_IDX 1

static cxStopWatch s_frameStopWatch;
static cxStopWatch s_execStopWatch;
static cxStopWatch s_freqStopWatch;

static bool s_enableExecProfiling = false;

static float s_medianFPS = -1.0f;
static float s_motSpeed = 0.5f;
static double s_runningTimeMicros = 0.0;
static bool s_echoFPS = false;

static bool s_lowq = false;
static bool s_stdqPerspSdw = false;

static int s_adapt = 0;

static float s_viewDist = 0.0f;

#if 0
#define KEY_GET(_name) if (OGLSys::get_key_state(#_name)) mask |= 1ULL << _name
#else
#define KEY_GET(_name) process_key(&mask, _name, #_name)
#endif

static struct KBD_CTRL {
	enum {
		UP = 0,
		DOWN,
		LEFT,
		RIGHT,
		LCTRL,
		LSHIFT,
		TAB,
		BACK,
		ENTER,
		SPACE,
		NUM_KEYS
	};
	uint64_t mNow;
	uint64_t mOld;
	bool mSkip;

	void init() {
		mNow = 0;
		mOld = 0;
		mSkip = false;
	}

	const char* get_alt_key_name(const int code) {
		static struct {
			int code;
			const char* pName;
		} tbl[] = {
			{ UP, "W" }, { DOWN, "S" }, { LEFT, "A" }, { RIGHT, "D" }, {SPACE, " "}
		};
		const char* pAltName = nullptr;
		for (int i = 0; i < XD_ARY_LEN(tbl); ++i) {
			if (code == tbl[i].code) {
				pAltName = tbl[i].pName;
				break;
			}
		}
		return pAltName;
	}

	void process_key(uint64_t* pMask, const int code, const char* pName) {
		if (!pMask || !pName) return;
		if (OGLSys::get_key_state(pName)) *pMask |= 1ULL << code;
		const char* pAltName = get_alt_key_name(code);
		if (pAltName) {
			if (OGLSys::get_key_state(pAltName)) *pMask |= 1ULL << code;
		}
	}

	void update() {
		if (mSkip) return;
		mOld = mNow;
		uint64_t mask = 0;
		KEY_GET(UP);
		KEY_GET(DOWN);
		KEY_GET(LEFT);
		KEY_GET(RIGHT);
		KEY_GET(LCTRL);
		KEY_GET(LSHIFT);
		KEY_GET(TAB);
		KEY_GET(BACK);
		KEY_GET(ENTER);
		KEY_GET(SPACE);
		mNow = mask;
	}

	bool ck_now(int id) const { return !!(mNow & (1ULL << id)); }
	bool ck_old(int id) const { return !!(mOld & (1ULL << id)); }
	bool ck_trg(int id) const { return !!((mNow & (mNow ^ mOld)) & (1ULL << id)); }
	bool ck_chg(int id) const { return !!((mNow ^ mOld) & (1ULL << id)); }
} s_kbdCtrl;

struct VIEW_WK {
	cxVec prevPos;
	cxVec prevTgt;
	cxVec pos;
	cxVec tgt;
	int viewMode;
	int tgtMode;
};

static VIEW_WK* s_pViewWk = nullptr;

class ScenarioExprCtx;
typedef cxStrMap<int> ScenarioVarMap;

enum class eScenarioState {
	CTRL,
	EXAMINE,
	TALK
};

#define SCENARIO_TXT_BUF_SIZE (32 * 1024)
#define SCENARIO_MAX_VARS 128
#define SCENARIO_MAX_EXECS 10

static double get_running_time_secs() {
	double us = s_runningTimeMicros;
	double millis = us / 1000.0;
	double secs = millis / 1000.0;
	return secs;
}

struct SCENARIO_WK {
	eScenarioState mStateOld;
	eScenarioState mStateNow;
	sxExprLibData* mpExprLib;
	ScenarioExprCtx* mpExprCtx;
	sxValuesData* mpExmVals;
	sxValuesData* mpVarVals;
	ScenarioVarMap* mpVarsMap;
	const char* mpExmName;
	const char* mpExmText;
	const char* mpExmTextName;
	int mExmPolId;

	ScnObj* mpPlyr;

	ScnObj* mpTalkTgt;
	float mTalkTgtDist;
	bool mCanTalk;

	int mExmCntsNum;
	struct {
		const char* pName;
		int cnt;
	} mExmCnts[128];

	int mNumVars;
	float mVars[SCENARIO_MAX_VARS];
	const char* mpVarNames[SCENARIO_MAX_VARS];

	char mTxtBuf[SCENARIO_TXT_BUF_SIZE];

	int find_exm_cnt(const char* pName) {
		int idx = -1;
		if (pName) {
			for (int i = 0; i < mExmCntsNum; ++i) {
				if (nxCore::str_eq(mExmCnts[i].pName, pName)) {
					idx = i;
					break;
				}
			}
		}
		return idx;
	}

	float get_val(const char* pVarName) const {
		float val = 0.0f;
		if (pVarName && mpVarsMap) {
			int varIdx = -1;
			if (mpVarsMap->get(pVarName, &varIdx)) {
				if (varIdx >= 0 && varIdx < SCENARIO_MAX_VARS) {
					val = mVars[varIdx];
				}
			}
		}
		return val;
	}

	bool get_flg(const char* pVarName) const {
		return !!get_val(pVarName);
	}

	float eval_expr(const char* pExprName);
};


static SCENARIO_WK* s_pScenarioWk = nullptr;

static bool scenario_eval_exec(SCENARIO_WK* pWk, const char* pExec);

static void scenario_set_player(SCENARIO_WK* pWk, ScnObj* pPlyrObj) {
	if (pWk) {
		pWk->mpPlyr = pPlyrObj;
	}
}

class ScenarioExprCtx : public sxCompiledExpression::ExecIfc {
protected:
	sxCompiledExpression::Stack mStk;

public:
	float mRes;

public:
	ScenarioExprCtx() : mRes(0.0f) {
		mStk.alloc(128);
	}

	~ScenarioExprCtx() {
		mStk.free();
	}

	sxCompiledExpression::Stack* get_stack() { return &mStk; }

	void set_result(const float val) { mRes = val; }

	float ch(const sxCompiledExpression::String& path) {
		float val = 0.0f;
		SCENARIO_WK* pWk = s_pScenarioWk;
		const char* pVarsPath = "../ProgState/Vars/";
		const char* pExmCntsPath = "../ProgState/ExmCnts/";
		if (path.starts_with(pVarsPath)) {
			if (pWk && pWk->mpVarsMap) {
				const char* pVarName = path.mpChars + ::strlen(pVarsPath);
				int varIdx = -1;
				if (pWk->mpVarsMap->get(pVarName, &varIdx)) {
					if (varIdx >= 0 && varIdx < SCENARIO_MAX_VARS) {
						val = pWk->mVars[varIdx];
					}
				}
			}
		} else if (path.starts_with(pExmCntsPath)) {
			int exmCntIdx = pWk->find_exm_cnt(path.mpChars + ::strlen(pExmCntsPath));
			if (exmCntIdx >= 0) {
				val = float(pWk->mExmCnts[exmCntIdx].cnt);
			}
		}
		return val;
	}

	float var(const sxCompiledExpression::String& name) {
		float val = 0.0f;
		if (name.eq("$F")) {
			val = float(Scene::get_frame_count());
		} if (name.eq("$T")) {
			val = float(get_running_time_secs());
		}
		return val;
	}


	int get_int_result() const { return int(mRes); }
};

float SCENARIO_WK::eval_expr(const char* pExprName) {
	float val = 0.0f;
	if (mpExprLib && mpExprCtx) {
		int exprIdx = mpExprLib->find_expr_idx("expr_roof", pExprName);
		if (exprIdx >= 0) {
			sxExprLibData::Entry ent = mpExprLib->get_entry(exprIdx);
			if (ent.is_valid()) {
				const sxCompiledExpression* pExpr = ent.get_expr();
				if (pExpr && pExpr->is_valid()) {
					pExpr->exec(*mpExprCtx);
					val = mpExprCtx->mRes;
				}
			}
		}
	}
	return val;
}


static int adj_counter_to_speed(const int count) {
	int res = count;
	if (count > 0) {
		res = int(nxCalc::div0(float(count), s_motSpeed * Scene::speed() * 2.0f));
	}
	return res;
}

//
// chr
//

enum ROUTINE {
	RTN_STAND,
	RTN_TURN_L,
	RTN_TURN_R,
	RTN_WALK,
	RTN_RETREAT,
	RTN_RUN
};

static const int s_blinkPat0[] = { 200, 200, 10, 200, 200 };
static const int s_blinkPat1[] = { 200, 200, 10, 200, 100, 100, 200, 10, 20, 200, 100, 200 };
#define BLINK_PAT(_pat) { (int)XD_ARY_LEN(_pat), _pat }

static struct BlinkPat {
	int len;
	const int* pPat;
} s_blinkPats[] = {
	BLINK_PAT(s_blinkPat0),
	BLINK_PAT(s_blinkPat1)
};

struct Blink {
	int mPatNo;
	int mPatIdx;
	int mPatCtr;
	int mAnimCtr;
	int mState;
	float mEyelidOpenY;
	float mEyelidClosedY;

	void init(ScnObj* pObj, const int patNo = -1) {
		if (!pObj) {
			mPatNo = -1;
			return;
		}
		int npats = XD_ARY_LEN(s_blinkPats);
		if (patNo < 0) {
			mPatNo = int(Scene::glb_rng_next() & 0xFF) % npats;
		} else {
			mPatNo = nxCalc::clamp(patNo, 0, npats - 1);
		}
		mPatIdx = int(Scene::glb_rng_next() & 0xFF) % s_blinkPats[mPatNo].len;
		mPatCtr = 0;
		mAnimCtr = 0;
		mState = 0;
		mEyelidOpenY = pObj->get_skel_local_rest_pos(pObj->find_skel_node_id("f_Eyelid_L")).y;
		mEyelidClosedY = mEyelidOpenY - 0.012f;
	}

	void exec(ScnObj* pObj);
};

struct LookAt {
	ScnObj* pSelf;
	ScnObj* pTgt;
	float limDist;
	float limRX;
	float limRY;
	float prevRX;
	float prevRY;
	bool cut;

	void exec() {
		if (!pSelf) return;
		if (!pTgt) return;
		int itgt = pTgt->find_skel_node_id("j_Head");
		if (!pTgt->ck_skel_id(itgt)) return;
		int idst = pSelf->find_skel_node_id("j_Neck");
		if (!pSelf->ck_skel_id(idst)) return;

		float rx = 0.0f;
		float ry = 0.0f;
		//pTgt->sync_motion();
		cxMtx tw = pTgt->calc_skel_world_mtx(itgt);
		cxMtx pw;
		cxMtx dw = pSelf->calc_skel_world_mtx(idst, &pw);
		cxVec wv = (tw.get_translation() - dw.get_translation());
		float dist = wv.mag();
		if (dist < limDist) {
			wv.normalize();
			cxVec lv = dw.get_inverted().calc_vec(wv);
			rx = lv.elevation() * 0.25f;
			ry = lv.azimuth();
			if (::fabsf(lv.y) > limRX) {
				rx = 0.0f;
			}
			if (lv.z < limRY) {
				if (cut || (dist > limDist*0.75f)) {
					ry = 0.0f;
				} else {
					lv.z = limRY;
					lv.normalize();
					ry = lv.azimuth();
				}
			}
		}
		if (Scene::get_frame_count() > 0) {
			rx = nxCalc::approach(prevRX, rx, adj_counter_to_speed(40));
			ry = nxCalc::approach(prevRY, ry, adj_counter_to_speed(30));
		}
		prevRX = rx;
		prevRY = ry;
		cxMtx rm;
		rm.set_rot(rx*0.25f, ry*0.75f, 0.0f);
		cxMtx lm = dw * rm * pw.get_inverted();
		pSelf->set_skel_local_quat(idst, lm.to_quat());

		idst = pSelf->find_skel_node_id("j_Head");
		if (pSelf->ck_skel_id(idst)) {
			dw = pSelf->calc_skel_world_mtx(idst, &pw);
			rm.set_rot(rx*0.75f, ry*0.25f, 0.0f);
			lm = dw * rm * pw.get_inverted();
			pSelf->set_skel_local_quat(idst, lm.to_quat());
		}
	}
};

struct MotLib {
	sxMotionData* pStand;
	sxMotionData* pTurnL;
	sxMotionData* pTurnR;
	sxMotionData* pWalk;
	sxMotionData* pRetreat;
	sxMotionData* pRun;
};

struct ChrWk {
	LegInfo legL;
	LegInfo legR;
	float hipAdjInfl;
	float kneeAdjInfl;
	float thighAdjUprInfl;
	float thighAdjLwrInfl;
	float elbowAdjInfl;
	bool prevPosFlg;
	float wallAdjRadius;
	int wallHitCnt;
	int objHitCnt;
	int wallTouchCnt;
	Blink blink;
	LookAt lookAt;
	MotLib motLib;
};

void Blink::exec(ScnObj* pObj) {
	if (!pObj) return;
	if (uint32_t(mPatNo) >= uint32_t(XD_ARY_LEN(s_blinkPats))) return;
	const int* pPat = s_blinkPats[mPatNo].pPat;
	int patLen = s_blinkPats[mPatNo].len;
	if (mState) {
		int animLen = 40;
		if (pPat[mPatIdx] < 100) {
			animLen = 30;
		}
		float t = float(mAnimCtr) / float(animLen);
		t *= s_motSpeed * Scene::speed() * 2.0f;
		DynRig::calc_eyelids_blink(pObj, mEyelidOpenY, mEyelidClosedY, t);
		++mAnimCtr;
		if (mAnimCtr > animLen) {
			mState = 0;
			mAnimCtr = 0;
		}
	} else {
		DynRig::calc_eyelids_blink(pObj, mEyelidOpenY, mEyelidOpenY, 0.0f);
		if (mPatCtr > pPat[mPatIdx]) {
			mState = 1;
			mPatCtr = 0;
			mAnimCtr = 0;
			mPatIdx = (mPatIdx + 1) % patLen;
		} else {
			++mPatCtr;
		}
	}
}

static void chr_legs_adj(ScnObj* pObj) {
#if 1
	if (!pObj) return;
	ChrWk* pWk = pObj->get_ptr_wk<ChrWk>(CHR_WK_IDX);
	if (!pWk) return;
	Pkg* pStgPkg = Scene::find_pkg(STG_NAME);
	if (!pStgPkg) return;
	sxCollisionData* pCol = pStgPkg->find_collision(COL_NAME);
	if (!pCol) return;
	DynRig::adjust_leg(pObj, pCol, &pWk->legL);
	DynRig::adjust_leg(pObj, pCol, &pWk->legR);
#endif
}

static void chr_ground_adj(ScnObj* pObj) {
	if (!pObj) return;
	Pkg* pStgPkg = Scene::find_pkg(STG_NAME);
	if (!pStgPkg) return;
	cxVec wpos = pObj->get_world_pos();
	float gndY = Scene::get_ground_height(pStgPkg->find_collision(COL_NAME), wpos);
	pObj->set_skel_root_local_ty(gndY);
}

static void chr_walls_adj(ScnObj* pObj, const float yoffs = 1.0f) {
	if (!pObj) return;
	Pkg* pStgPkg = Scene::find_pkg(STG_NAME);
	if (!pStgPkg) return;
	sxCollisionData* pCol = pStgPkg->find_collision(COL_NAME);
	if (!pCol) return;
	ChrWk* pWk = pObj->get_ptr_wk<ChrWk>(CHR_WK_IDX);
	if (!pWk) return;
	cxVec wpos = pObj->get_world_pos();
	cxVec npos = wpos;
	cxVec opos = pObj->get_prev_world_pos();
	if (pWk) {
		if (!pWk->prevPosFlg) {
			opos = wpos;
			pWk->prevPosFlg = true;
		}
	}
	float oy = opos.y;
	npos.y += yoffs;
	opos.y += yoffs;
	const sxJobContext* pJobCtx = pObj->mpJobCtx;
	cxVec apos = npos;
	float r = pWk->wallAdjRadius;
	bool touchFlg = Scene::wall_adj(pJobCtx, pCol, npos, opos, r * 1.025f, nullptr);
	if (touchFlg) {
		++pWk->wallTouchCnt;
	} else {
		pWk->wallTouchCnt = 0;
	}
	bool adjFlg = Scene::wall_adj_base(pJobCtx, pCol, npos, opos, r, &apos);
	if (adjFlg) {
		float wy = wpos.y + yoffs;
		cxLineSeg aseg(cxVec(opos.x, wy, opos.z), cxVec(apos.x, wy, apos.z));
		sxCollisionData::NearestHit ahit = pCol->nearest_hit(aseg);
		if (ahit.count > 0) {
			cxVec epos = nxVec::lerp(cxVec(opos.x, wpos.y, opos.z), cxVec(ahit.pos.x, wpos.y, ahit.pos.z), 0.5f);
			aseg.set(cxVec(opos.x, wy, opos.z), cxVec(epos.x, wy, epos.z));
			ahit = pCol->nearest_hit(aseg);
			if (ahit.count > 0) {
				pObj->set_skel_root_local_pos(opos.x, oy, opos.z);
			} else {
				pObj->set_skel_root_local_pos(epos.x, epos.y, epos.z);
			}
		} else {
			pObj->set_skel_root_local_pos(apos.x, wpos.y, apos.z);
		}
		++pWk->wallHitCnt;
	} else {
		pWk->wallHitCnt = 0;
	}
}

static bool obj_obj_for_each_func(ScnObj* pObj, void* pWkMem) {
	if (!pWkMem) return false;
	if (!pObj->mpMotWk) return true;
	ScnObj* pSelf = (ScnObj*)pWkMem;
	if (pObj == pSelf) return true;
	if (pObj->mObjAdjRadius < 1.0e-5f) return true;
	ChrWk* pSelfWk = pSelf->get_ptr_wk<ChrWk>(CHR_WK_IDX);
	if (!pSelfWk) return true;
	cxVec objWPos = pObj->get_skel_root_prev_world_mtx().get_translation();
	cxVec objPos = objWPos;
	cxVec wpos = pSelf->get_world_pos();
	cxVec npos = wpos;
	cxVec opos = pSelf->get_skel_root_prev_world_mtx().get_translation();
	npos.y += pSelf->mObjAdjYOffs;
	opos.y += pSelf->mObjAdjYOffs;
	objPos.y += pObj->mObjAdjYOffs;
	cxVec adjPos = npos;
	bool flg = Scene::sph_sph_adj(npos, opos, pSelf->mObjAdjRadius, objPos, pObj->mObjAdjRadius, &adjPos);
	if (flg) {
		pSelf->set_skel_root_local_tx(adjPos.x);
		pSelf->set_skel_root_local_tz(adjPos.z);
		++pSelfWk->objHitCnt;
	} else {
		pSelfWk->objHitCnt = 0;
	}
	return true;
}

static void chr_objs_adj(ScnObj* pObj) {
	if (!pObj) return;
	Scene::for_each_obj(obj_obj_for_each_func, pObj);
}

static void chr_before_blend(ScnObj* pObj) {
	if (!pObj) return;
	chr_ground_adj(pObj);
	chr_objs_adj(pObj);
	chr_walls_adj(pObj);
	chr_legs_adj(pObj);
	ChrWk* pWk = pObj->get_ptr_wk<ChrWk>(CHR_WK_IDX);
	if (pWk) {
		DynRig::calc_hip_adj_l(pObj, pWk->hipAdjInfl);
		DynRig::calc_hip_adj_r(pObj, pWk->hipAdjInfl);
		DynRig::calc_knee_adj_l(pObj, pWk->kneeAdjInfl);
		DynRig::calc_knee_adj_r(pObj, pWk->kneeAdjInfl);
		DynRig::calc_thigh_adj_l(pObj, pWk->thighAdjUprInfl, pWk->thighAdjLwrInfl);
		DynRig::calc_thigh_adj_r(pObj, pWk->thighAdjUprInfl, pWk->thighAdjLwrInfl);
		DynRig::calc_elbow_bend_l(pObj, pWk->elbowAdjInfl);
		DynRig::calc_elbow_bend_r(pObj, pWk->elbowAdjInfl);
		pWk->lookAt.exec();
	}
}

static ChrWk* chr_init(ScnObj* pObj) {
	if (!pObj) return nullptr;
	if (s_lowq) {
		pObj->mDisableShadowRecv = true;
	}
	pObj->set_ptr_wk(0, nxCore::mem_alloc(sizeof(ChrWk), "ChrWk"));
	ChrWk* pWk = pObj->get_ptr_wk<ChrWk>(CHR_WK_IDX);
	if (pWk) {
		::memset(pWk, 0, sizeof(ChrWk));
		pWk->lookAt.pSelf = pObj;
		pWk->lookAt.limDist = 2.0f;
		pWk->lookAt.limRX = 0.3f;
		pWk->lookAt.limRY = 0.5f;
		pWk->lookAt.cut = true;

		pWk->motLib.pStand = pObj->find_motion("stand");
		pWk->motLib.pTurnL = pObj->find_motion("turn90_l");
		pWk->motLib.pTurnR = pObj->find_motion("turn90_r");
		pWk->motLib.pWalk = pObj->find_motion("walk");
		pWk->motLib.pRetreat = pObj->find_motion("retreat");
		pWk->motLib.pRun = pObj->find_motion("run");
		
	}
	return pWk;
}

static int chr_find_nearest_mot_frame(ScnObj* pObj, const sxMotionData* pMtd, const char* pNodeName) {
	int frm = 0;
	if (pObj && pObj->mpMdlWk && pObj->mpMotWk && pNodeName && pMtd) {
		int inode = pObj->find_skel_node_id(pNodeName);
		int itop = pObj->find_skel_node_id("n_Center");
		if (pObj->ck_skel_id(inode) && pObj->ck_skel_id(itop)) {
			xt_xmtx nodeXform = pObj->mpMdlWk->mpData->calc_skel_node_chain_xform(inode, itop, pObj->mpMotWk->mpXformsL);
			cxVec sklPos = nxMtx::xmtx_get_pos(nodeXform);
			int n = pMtd->mFrameNum - 2;
			int nearFrm = 0;
			float nearDist = 0.0f;
			for (int i = 0; i < n; ++i) {
				xt_xmtx motXform = pObj->mpMotWk->eval_skel_node_chain_xform(pMtd, inode, itop, float(i));
				cxVec motPos = nxMtx::xmtx_get_pos(motXform);
				if (i > 0) {
					float dist = nxVec::dist(sklPos, motPos);
					if (dist < nearDist) {
						nearFrm = i;
						nearDist = dist;
					}
				} else {
					nearDist = nxVec::dist(sklPos, motPos);
				}
			}
			frm = nearFrm;
		}
	}
	return frm;
}

static sxMotionData* chr_get_rtn_motion(ScnObj* pObj) {
	sxMotionData* pMtd = nullptr;
	if (pObj) {
		ChrWk* pWk = pObj->get_ptr_wk<ChrWk>(CHR_WK_IDX);
		if (pWk) {
			pMtd = pWk->motLib.pStand;
			switch (pObj->mRoutine[0]) {
				case RTN_WALK:
					pMtd = pWk->motLib.pWalk;
					break;
				case RTN_RUN:
					pMtd = pWk->motLib.pRun;
					break;
				case RTN_TURN_L:
					pMtd = pWk->motLib.pTurnL;
					break;
				case RTN_TURN_R:
					pMtd = pWk->motLib.pTurnR;
					break;
				case RTN_RETREAT:
					pMtd = pWk->motLib.pRetreat;
					break;
			}
		}
		if (!pMtd) {
			pMtd = pObj->find_motion("stand");
		}
	}
	return pMtd;
}


static void actr_set_dlg_name(ScnObj* pObj, const char* pName) {
	if (!pObj) return;
	pObj->set_ptr_wk(ACTR_DLG_NAME_IDX, pName);
}

static const char* actr_get_dlg_name(ScnObj* pObj) {
	if (!pObj) return nullptr;
	if (!pObj->mpVals) return nullptr;
	return pObj->get_ptr_wk<const char>(ACTR_DLG_NAME_IDX);
}

static const char* actr_get_name(ScnObj* pObj) {
	const char* pName = "<actr>";
	if (pObj) {
		sxValuesData* pVals = pObj->mpVals;
		if (pVals) {
			sxValuesData::Group infoGrp = pVals->find_grp("info");
			if (infoGrp.is_valid()) {
				int nameIdx = infoGrp.find_val_idx("name");
				if (infoGrp.ck_val_idx(nameIdx)) {
					pName = infoGrp.get_val_s(nameIdx);
				} else if (s_pScenarioWk) {
					int nameFlgIdx = infoGrp.find_val_idx("name_flg");
					if (infoGrp.ck_val_idx(nameFlgIdx)) {
						const char* pFlgName = infoGrp.get_val_s(nameFlgIdx);
						if (pFlgName) {
							bool nameFlg = s_pScenarioWk->get_flg(pFlgName);
							if (nameFlg) {
								int nameTrueIdx = infoGrp.find_val_idx("name_flg1");
								if (infoGrp.ck_val_idx(nameTrueIdx)) {
									pName = infoGrp.get_val_s(nameTrueIdx);
								}
							} else {
								int nameFalseIdx = infoGrp.find_val_idx("name_flg0");
								if (infoGrp.ck_val_idx(nameFalseIdx)) {
									pName = infoGrp.get_val_s(nameFalseIdx);
								}
							}
						}
					} else {
						// var selector
					}
				}
			}
		}
	}
	return pName;
}

static bool actr_can_talk(ScnObj* pObj) {
	bool canTalk = false;
	if (pObj) {
		sxValuesData* pVals = pObj->mpVals;
		if (pVals) {
			sxValuesData::Group infoGrp = pVals->find_grp("info");
			if (infoGrp.is_valid()) {
				int flgIdx = infoGrp.find_val_idx("canTalk");
				if (infoGrp.ck_val_idx(flgIdx)) {
					canTalk = !!infoGrp.get_val_i(flgIdx);
				}
			}
		}
	}
	return canTalk;
}

static void actr_dlg_init(ScnObj* pObj) {
	if (!pObj) return;
	sxValuesData* pVals = pObj->mpVals;
	if (!pVals) return;
	actr_set_dlg_name(pObj, nullptr);
	sxValuesData::Group iniGrp = pVals->find_grp("init");
	if (!iniGrp.is_valid()) return;
	int dlgIdx = iniGrp.find_val_idx("dlg_start");
	if (!iniGrp.ck_val_idx(dlgIdx)) return;
	const char* pDlgName = iniGrp.get_val_s(dlgIdx);
	actr_set_dlg_name(pObj, pDlgName);
}

static bool actr_dlg_next(ScnObj* pObj) {
	bool contFlg = false;
	int execCnt = 0;
	if (pObj) {
		sxValuesData* pVals = pObj->mpVals;
		if (pVals) {
			const char* pDglName = actr_get_dlg_name(pObj);
			actr_set_dlg_name(pObj, nullptr);
			if (pDglName) {
				sxValuesData::Group dlgGrp = pVals->find_grp(pDglName);
				if (dlgGrp.is_valid()) {
					bool endFlg = false;
					int nextIdx = dlgGrp.find_val_idx("next");
					if (!dlgGrp.ck_val_idx(nextIdx)) {
						nextIdx = dlgGrp.find_val_idx("end");
						if (dlgGrp.ck_val_idx(nextIdx)) {
							endFlg = true;
						}
					}
					if (dlgGrp.ck_val_idx(nextIdx)) {
						const char* pNextName = dlgGrp.get_val_s(nextIdx);
						if (pNextName) {
							contFlg = !endFlg;
							actr_set_dlg_name(pObj, pNextName);
						}
					}
					char execName[64];
					for (int i = 0; i < SCENARIO_MAX_EXECS; ++i) {
						XD_SPRINTF(XD_SPRINTF_BUF(execName, sizeof(execName)), "exec%d", i);
						int execIdx = dlgGrp.find_val_idx(execName);
						if (dlgGrp.ck_val_idx(execIdx)) {
							const char* pExec = dlgGrp.get_val_s(execIdx);
							if (scenario_eval_exec(s_pScenarioWk, pExec)) {
								++execCnt;
							}
						}
					}
				}
			}
		}
	}
	return contFlg;
}



// Den
static void Den_bat_pre_draw(ScnObj* pObj, const int ibat) {
	if (!pObj) return;
	const char* pMtlName = pObj->get_batch_mtl_name(ibat);
	bool flg = false;
	if (pMtlName) {
		flg = nxCore::str_eq(pMtlName, "head") || nxCore::str_eq(pMtlName, "arms");
	}
	//flg = true;/////
	if (flg) {
		Scene::push_ctx();
		//Scene::scl_hemi_upper(1.275f);
		Scene::scl_hemi_upper(0.9f);
		Scene::scl_hemi_lower(0.49f);
		Scene::set_hemi_exp(2.5f);
		//Scene::set_hemi_up(nxVec::rot_deg_z(nxVec::get_axis(exAxis::PLUS_Y), -30.0f));
	}
}

static void Den_bat_post_draw(ScnObj* pObj, const int ibat) {
	if (!pObj) return;
	const char* pMtlName = pObj->get_batch_mtl_name(ibat);
	bool flg = false;
	if (pMtlName) {
		flg = nxCore::str_eq(pMtlName, "head") || nxCore::str_eq(pMtlName, "arms");
	}
	//flg = true;/////
	if (flg) {
		Scene::pop_ctx();
	}
}

static void Den_init(ScnObj* pObj) {
	if (!pObj) return;
	ChrWk* pWk = chr_init(pObj);
	if (pWk) {
		bool extFlg = false;
		pWk->legL.init(pObj, 'L', extFlg);
		pWk->legR.init(pObj, 'R', extFlg);
		//pWk->hipAdjInfl = 0.7f;
		//pWk->kneeAdjInfl = 0.3f;
		//pWk->thighAdjUprInfl = -0.3f;
		//pWk->thighAdjLwrInfl = -0.5f;
		pWk->wallAdjRadius = 0.2f;
		pWk->blink.init(pObj, 1);
		pWk->lookAt.limDist = 3.0f;
		pWk->lookAt.limRY = 0.65f;
		pWk->lookAt.cut = false;
	}
	pObj->mObjAdjYOffs = 0.5f;
	pObj->mObjAdjRadius = 0.45f;
	pObj->mBeforeBlendFunc = chr_before_blend;
	pObj->mRoutine[0] = RTN_STAND;
	pObj->mRoutine[1] = 0;
	pObj->mFltWk[0] = 0.0f;
	pObj->mCounter[0] = 0;
	pObj->mCounter[1] = 0;
	pObj->set_base_color_scl(0.8f);////////
	//pObj->set_shadow_offs_bias(0.001f);
	//pObj->set_shadow_weight_bias(1200.0f - 1000);
	if (s_stdqPerspSdw) {
		pObj->set_shadow_weight_bias(200.0f);
	}
	pObj->set_shadow_density_scl(0.75f);
	pObj->mBatchPreDrawFunc = Den_bat_pre_draw;
	pObj->mBatchPostDrawFunc = Den_bat_post_draw;
	//pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, -60.0f, 0.0f), cxVec(0.8f, 0.0f, 3.5f));
	pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, -110.0f, 0.0f), cxVec(0.8f, 0.0f, 2.5f));
	pObj->move(pObj->find_motion("face_def"), 0.0f);
	pObj->move(pObj->find_motion("hands_def"), 0.0f);
	scenario_set_player(s_pScenarioWk, pObj);
}

static void obj_mot_blend(ScnObj* pObj, const int duration, const float frame = 0.0f) {
	if (pObj) {
		float t = nxCalc::div0(float(duration), s_motSpeed * Scene::speed() * 2.0f);
		pObj->set_motion_frame(frame);
		pObj->init_motion_blend(int(t));
	}
}

static void obj_add_deg_y(ScnObj* pObj, const float dy) {
	if (pObj) {
		pObj->add_world_deg_y(dy * s_motSpeed * Scene::speed() * 2.0f);
	}
}

static bool ck_run_modifier() {
	return s_kbdCtrl.ck_now(KBD_CTRL::LSHIFT) || s_kbdCtrl.ck_now(KBD_CTRL::SPACE);
}

static void Den_exec_ctrl(ScnObj* pObj) {
	if (!pObj) return;
	ChrWk* pWk = pObj->get_ptr_wk<ChrWk>(CHR_WK_IDX);
	switch (pObj->mRoutine[0]) {
		case RTN_STAND:
			if (s_kbdCtrl.ck_trg(KBD_CTRL::UP)) {
				if (ck_run_modifier()) {
					pObj->mRoutine[0] = RTN_RUN;
					obj_mot_blend(pObj, 12);
				} else {
					pObj->mRoutine[0] = RTN_WALK;
					obj_mot_blend(pObj, 20);
				}
			} else if (s_kbdCtrl.ck_trg(KBD_CTRL::DOWN)) {
				pObj->mRoutine[0] = RTN_RETREAT;
				obj_mot_blend(pObj, 20);
			} else if (s_kbdCtrl.ck_now(KBD_CTRL::LEFT)) {
				pObj->mRoutine[0] = RTN_TURN_L;
				obj_mot_blend(pObj, 30);
			} else if (s_kbdCtrl.ck_now(KBD_CTRL::RIGHT)) {
				pObj->mRoutine[0] = RTN_TURN_R;
				obj_mot_blend(pObj, 30);
			}
			break;
		case RTN_WALK:
			if (s_kbdCtrl.ck_now(KBD_CTRL::UP)) {
				if (ck_run_modifier()) {
					int runFrm = 0;
					if (pWk) {
						runFrm = chr_find_nearest_mot_frame(pObj, pWk->motLib.pRun, "j_Toe_L");
						//nxCore::dbg_msg("walk->run: %d -> %d\n", int(pObj->mpMotWk->mEvalFrame), runFrm);
					}
					pObj->mRoutine[0] = RTN_RUN;
					obj_mot_blend(pObj, 8, float(runFrm));
				} else {
					if (s_kbdCtrl.ck_now(KBD_CTRL::LEFT)) {
						obj_add_deg_y(pObj, 0.5f);
					} else if (s_kbdCtrl.ck_now(KBD_CTRL::RIGHT)) {
						obj_add_deg_y(pObj, -0.5f);
					}
				}
			} else if (s_kbdCtrl.ck_trg(KBD_CTRL::DOWN)) {
				pObj->mRoutine[0] = RTN_RETREAT;
				obj_mot_blend(pObj, 20);
			} else {
				pObj->mRoutine[0] = RTN_STAND;
				obj_mot_blend(pObj, 20);
			}
			break;
		case RTN_RETREAT:
			if (s_kbdCtrl.ck_now(KBD_CTRL::DOWN)) {
				if (s_kbdCtrl.ck_now(KBD_CTRL::LEFT)) {
					obj_add_deg_y(pObj, 0.5f);
				} else if (s_kbdCtrl.ck_now(KBD_CTRL::RIGHT)) {
					obj_add_deg_y(pObj, -0.5f);
				}
			} else {
				pObj->mRoutine[0] = RTN_STAND;
				obj_mot_blend(pObj, 20);
			}
			break;
		case RTN_RUN:
			if (s_kbdCtrl.ck_now(KBD_CTRL::UP)) {
				if (ck_run_modifier()) {
					if (s_kbdCtrl.ck_now(KBD_CTRL::LEFT)) {
						obj_add_deg_y(pObj, 1.0f);
					} else if (s_kbdCtrl.ck_now(KBD_CTRL::RIGHT)) {
						obj_add_deg_y(pObj, -1.0f);
					}
				} else {
					int walkFrm = 0;
					if (pWk) {
						walkFrm = chr_find_nearest_mot_frame(pObj, pWk->motLib.pWalk, "j_Toe_L");
						//nxCore::dbg_msg("run->walk: %d -> %d\n", int(pObj->mpMotWk->mEvalFrame), walkFrm);
					}
					pObj->mRoutine[0] = RTN_WALK;
					obj_mot_blend(pObj, 12, float(walkFrm));
				}
			} else {
				pObj->mRoutine[0] = RTN_STAND;
				obj_mot_blend(pObj, 14);
			}
			break;
		case RTN_TURN_L:
			if (s_kbdCtrl.ck_now(KBD_CTRL::LEFT)) {
				if (s_kbdCtrl.ck_trg(KBD_CTRL::UP)) {
					if (ck_run_modifier()) {
						pObj->mRoutine[0] = RTN_RUN;
						obj_mot_blend(pObj, 8);
					} else {
						pObj->mRoutine[0] = RTN_WALK;
						obj_mot_blend(pObj, 20);
					}
				}
			} else {
				pObj->mRoutine[0] = RTN_STAND;
				obj_mot_blend(pObj, 30);
			}
			break;
		case RTN_TURN_R:
			if (s_kbdCtrl.ck_now(KBD_CTRL::RIGHT)) {
				if (s_kbdCtrl.ck_trg(KBD_CTRL::UP)) {
					if (ck_run_modifier()) {
						pObj->mRoutine[0] = RTN_RUN;
						obj_mot_blend(pObj, 8);
					} else {
						pObj->mRoutine[0] = RTN_WALK;
						obj_mot_blend(pObj, 20);
					}
				}
			} else {
				pObj->mRoutine[0] = RTN_STAND;
				obj_mot_blend(pObj, 30);
			}
			break;
	}
}

static void Den_exec(ScnObj* pObj) {
	if (!pObj) return;
	ChrWk* pWk = pObj->get_ptr_wk<ChrWk>(CHR_WK_IDX);
	if (pWk) {
		pWk->blink.exec(pObj);
		cxVec wpos = pObj->get_prev_world_pos();
		ScnObj* pTgt1 = Scene::find_obj("Zoe");
		float dist1 = pTgt1 ? nxVec::dist(pTgt1->get_prev_world_pos(), wpos) : 1e8f;
		ScnObj* pTgt2 = Scene::find_obj("Nic");
		float dist2 = pTgt2 ? nxVec::dist(pTgt2->get_prev_world_pos(), wpos) : 1e8f;
		pWk->lookAt.pTgt = dist1 < dist2 ? pTgt1 : pTgt2;
		if (pWk->lookAt.pTgt) {
			pWk->lookAt.limRY = 0.65f;
			if (pWk->lookAt.pTgt->get_world_bbox().get_size_vec().y < 1.0f) {
				pWk->lookAt.limRY = 1.25f;
			}
		}
	}

	SCENARIO_WK* pScenario = s_pScenarioWk;
	if (pScenario) {
		if (pScenario->mStateNow == eScenarioState::EXAMINE || pScenario->mStateNow == eScenarioState::TALK) {
			if (pObj->mRoutine[0] != RTN_STAND) {
				pObj->mRoutine[0] = RTN_STAND;
				obj_mot_blend(pObj, 30);
			}
		} else {
			Den_exec_ctrl(pObj);
		}
	} else {
		Den_exec_ctrl(pObj);
	}
	
	sxMotionData* pMtd = chr_get_rtn_motion(pObj);
	pObj->move(pMtd, s_motSpeed * Scene::speed());
}

static void Den_del(ScnObj* pObj) {
	nxCore::mem_free(pObj->get_ptr_wk<void>(CHR_WK_IDX));
}


// ~~~~~~~~~~~~~~ Zoe

static void Zoe_bat_pre_draw(ScnObj* pObj, const int ibat) {
	if (!pObj) return;
	const char* pMtlName = pObj->get_batch_mtl_name(ibat);
	if (pMtlName && nxCore::str_eq(pMtlName, "head")) {
		Scene::push_ctx();
		Scene::scl_hemi_upper(0.7f);
		Scene::scl_hemi_lower(2.0f);
		Scene::set_hemi_exp(2.0f - 0.5f);
	}
}

static void Zoe_bat_post_draw(ScnObj* pObj, const int ibat) {
	if (!pObj) return;
	const char* pMtlName = pObj->get_batch_mtl_name(ibat);
	if (pMtlName && nxCore::str_eq(pMtlName, "head")) {
		Scene::pop_ctx();
	}
}

static void Zoe_init(ScnObj* pObj) {
	if (!pObj) return;
	ChrWk* pWk = chr_init(pObj);
	if (pWk) {
		bool extFlg = !false;
		pWk->legL.init(pObj, 'L', extFlg);
		pWk->legR.init(pObj, 'R', extFlg);
		//pWk->hipAdjInfl = 0.7f;
		//pWk->kneeAdjInfl = 0.3f;
		pWk->elbowAdjInfl = 0.7f;
		pWk->wallAdjRadius = 0.2f;
		pWk->blink.init(pObj, 0);
	}
	pObj->mObjAdjYOffs = 0.5f;
	pObj->mObjAdjRadius = 0.4f;
	pObj->mBeforeBlendFunc = chr_before_blend;
	pObj->mRoutine[0] = RTN_WALK;
	pObj->mRoutine[1] = 0;
	pObj->mFltWk[0] = 0.0f;
	pObj->mCounter[0] = 0;
	pObj->mCounter[1] = 0;
	pObj->set_base_color_scl(0.65f, 0.62f, 0.6f);
	pObj->set_shadow_density_scl(0.75f);
	pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, -100.0f -50, 0.0f), cxVec(1.0f, 0.0f, 9.2f));
	//pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, -80.0f, 0.0f), cxVec(2.0f, 1.0f, -5.5f));
	pObj->set_motion_uniform_scl(1.02f);////////
	pObj->move(pObj->find_motion("mouth"), 0.0f);
	pObj->move(pObj->find_motion("arms"), 0.0f);
	pObj->set_shadow_weight_bias(1200.0f - 1000);
	pObj->set_shadow_density_scl(0.5f);
	pObj->mBatchPreDrawFunc = Zoe_bat_pre_draw;
	pObj->mBatchPostDrawFunc = Zoe_bat_post_draw;
	pObj->mpVals = Scene::load_vals("etc/Zoe.xval");
	actr_dlg_init(pObj);
}

static int adj_hit_count_to_speed(const int count) {
	int res = count;
	if (count > 0) {
		res = int(::roundf(nxCalc::div0(float(count), s_motSpeed * Scene::speed() * 2.0f)));
	}
	return res;
}

static void Zoe_exec_roam(ScnObj* pObj) {
	ChrWk* pWk = pObj->get_ptr_wk<ChrWk>(CHR_WK_IDX);
	if (!pWk) {
		pObj->mRoutine[0] = RTN_STAND;
		return;
	}
	switch (pObj->mRoutine[0]) {
		case RTN_STAND:
			if (pObj->mCounter[0] > 0) {
				--pObj->mCounter[0];
			} else {
				pObj->mCounter[0] = adj_counter_to_speed(40 + int(Scene::glb_rng_next() & 0x1F));
				if (int(Scene::glb_rng_next() & 0xFF) < 0x5F) {
					pObj->mCounter[0] += adj_counter_to_speed(60);
				}
				pObj->mRoutine[0] = (Scene::glb_rng_next() & 1) ? RTN_TURN_L : RTN_TURN_R;
				obj_mot_blend(pObj, 20);
			}
			break;
		case RTN_TURN_L:
		case RTN_TURN_R:
			if (pObj->mCounter[0] > 0) {
				--pObj->mCounter[0];
			} else {
				pObj->mRoutine[0] = RTN_WALK;
				pObj->mRoutine[1] = 0;
				pObj->mCounter[0] = 0;
				pWk->wallHitCnt = 0;
				pWk->objHitCnt = 0;
				pWk->wallTouchCnt = 0;
				obj_mot_blend(pObj, 20);
			}
			break;
		case RTN_WALK:
			if (pWk->wallTouchCnt > adj_hit_count_to_speed(10) || pWk->objHitCnt > adj_hit_count_to_speed(8)) {
				pObj->mRoutine[0] = RTN_STAND;
				pObj->mCounter[0] = adj_counter_to_speed(30 + int(Scene::glb_rng_next() & 0x3F));
				obj_mot_blend(pObj, 20);
			} else {
				if (pObj->mRoutine[1]) {
					if (pObj->mCounter[1] > 0) {
						obj_add_deg_y(pObj, pObj->mFltWk[0]);
						--pObj->mCounter[1];
					} else {
						pObj->mRoutine[1] = 0;
						pObj->mFltWk[0] = 0.0f;
					}
				} else {
					++pObj->mCounter[0];
					if (pObj->mCounter[0] > adj_counter_to_speed(1000)) {
						bool rotFlg = int(Scene::glb_rng_next() & 0xFF) < 0x10;
						if (rotFlg) {
							pObj->mRoutine[1] = 1;
							float ryAdd = 0.5f;
							if (Scene::glb_rng_next() & 1) {
								ryAdd = -ryAdd;
							}
							pObj->mFltWk[0] = ryAdd;
							pObj->mCounter[0] = 0;
							pObj->mCounter[1] = adj_counter_to_speed(10 + int(Scene::glb_rng_next() & 0x1F));
						}
					}
				}
			}
			break;
	}
}

static void Zoe_exec(ScnObj* pObj) {
	if (!pObj) return;
	ChrWk* pWk = pObj->get_ptr_wk<ChrWk>(CHR_WK_IDX);
	if (pWk) {
		pWk->blink.exec(pObj);
		pWk->lookAt.pTgt = Scene::find_obj("Den");
	}
#if 1
	if (s_pScenarioWk) {
		if (s_pScenarioWk->mStateNow == eScenarioState::TALK) {
			if (pObj->mRoutine[0] != RTN_STAND) {
				pObj->mRoutine[0] = RTN_STAND;
				obj_mot_blend(pObj, 30);
			}
		} else {
			Zoe_exec_roam(pObj);
		}
	} else {
		Zoe_exec_roam(pObj);
	}
	sxMotionData* pMtd = chr_get_rtn_motion(pObj);
	pObj->move(pMtd, s_motSpeed * Scene::speed());
#else
	sxMotionData* pMot = pObj->find_motion("stand");
	pObj->move(pMot, s_motSpeed * Scene::speed());
#endif
}

static void Zoe_del(ScnObj* pObj) {
	if (pObj) {
		if (pObj->mpVals) {
			Scene::unload_data_file(pObj->mpVals);
			pObj->mpVals = nullptr;
		}
		nxCore::mem_free(pObj->get_ptr_wk<void>(CHR_WK_IDX));
	}
}


// ~~~~~~~~~~~~~~ Nic

static void Nic_before_blend(ScnObj* pObj) {
	if (!pObj) return;
	ChrWk* pWk = pObj->get_ptr_wk<ChrWk>(CHR_WK_IDX);
	if (pWk) {
		pWk->lookAt.limDist = 10.0f;
		pWk->lookAt.limRY = 0.15f;
		pWk->lookAt.limRX = 0.02f;
		pWk->lookAt.cut = false;
		pWk->lookAt.exec();
	}
}

static void Nic_init(ScnObj* pObj) {
	if (!pObj) return;
	chr_init(pObj);
	pObj->mBeforeBlendFunc = Nic_before_blend;
	pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, 30.0f, 0.0f), cxVec(-7.3f, 0.82f, -10.1f));
	if (s_stdqPerspSdw) {
		pObj->set_shadow_weight_bias(200.0f);
	} else {
		pObj->set_shadow_weight_bias(400.0f);
	}
	pObj->set_shadow_density_scl(0.75f);
	pObj->mpVals = Scene::load_vals("etc/Nic.xval");
	actr_dlg_init(pObj);
}


static void Nic_exec(ScnObj* pObj) {
	if (!pObj) return;
	ChrWk* pWk = pObj->get_ptr_wk<ChrWk>(CHR_WK_IDX);
	if (pWk) {
		pWk->blink.exec(pObj);
		pWk->lookAt.pTgt = Scene::find_obj("Den");
	}
	sxMotionData* pMot = pObj->find_motion("default");
	pObj->move(pMot, s_motSpeed * Scene::speed());
}

static void Nic_del(ScnObj* pObj) {
	if (pObj) {
		if (pObj->mpVals) {
			Scene::unload_data_file(pObj->mpVals);
			pObj->mpVals = nullptr;
		}
		nxCore::mem_free(pObj->get_ptr_wk<void>(CHR_WK_IDX));
	}
}


static void init_view(VIEW_WK* pView) {
	if (!pView) return;
	::memset(pView, 0, sizeof(VIEW_WK));
	pView->tgtMode = nxApp::get_int_opt("tgt_mode", 0) != 0 ? 1 : 0;
}

static void init_scenario(SCENARIO_WK* pWk) {
	if (!pWk) return;
	::memset(pWk, 0, sizeof(SCENARIO_WK));
	pWk->mpVarVals = Scene::load_vals("etc/vars.xval");
	pWk->mpExmVals = Scene::load_vals("etc/exm_roof.xval");
	pWk->mpExprLib = Scene::load_expr_lib("etc/expr_roof.xcel");
	if (pWk->mpVarVals) {
		sxValuesData::Group varsGrp = pWk->mpVarVals->find_grp("vars");
		if (varsGrp.is_valid()) {
			int nvarsIdx = varsGrp.find_val_idx("nvars");
			if (varsGrp.ck_val_idx(nvarsIdx)) {
				int nvars = varsGrp.get_val_i(nvarsIdx);
				if (nvars > SCENARIO_MAX_VARS) {
					nvars = SCENARIO_MAX_VARS;
				}
				if (nvars > 0) {
					pWk->mpVarsMap = ScenarioVarMap::create("Scenario:VarMap", nvars);
					char varName[32];
					for (int i = 0; i < nvars; ++i) {
						XD_SPRINTF(XD_SPRINTF_BUF(varName, sizeof(varName)), "var%d", i + 1); // var ids are 1-based
						int varIdx = varsGrp.find_val_idx(varName);
						if (varsGrp.ck_val_idx(varIdx)) {
							const char* pVarName = varsGrp.get_val_s(varIdx);
							if (pVarName) {
								pWk->mpVarsMap->add(pVarName, i);
								pWk->mpVarNames[i] = pVarName;
							}
						}
					}
				}
				pWk->mNumVars = nvars;
			}
		}
	}
	if (pWk->mpExprLib) {
		pWk->mpExprCtx = nxCore::tMem<ScenarioExprCtx>::alloc(1, "Scenerio:ExprCtx");
	}
	pWk->mStateNow = eScenarioState::CTRL;
}

static void init() {
	s_enableExecProfiling = nxApp::get_bool_opt("exec_profiling", false);
	s_echoFPS = nxApp::get_bool_opt("echo_fps", false);
	s_lowq = nxApp::get_bool_opt("lowq", false);
	s_stdqPerspSdw = nxApp::get_bool_opt("spersp", false);
	s_adapt = nxApp::get_int_opt("adapt", 0);
	s_viewDist = nxApp::get_float_opt("view_dist", 0.0f);
	Scene::alloc_local_heaps(1024 * 1024 * 2);
	Scene::init_prims(1000, 1000);

	s_pViewWk = (VIEW_WK*)nxCore::mem_alloc(sizeof(VIEW_WK), "ViewWk");
	init_view(s_pViewWk);

	s_pScenarioWk = (SCENARIO_WK*)nxCore::mem_alloc(sizeof(SCENARIO_WK), "ScenarioWk");
	init_scenario(s_pScenarioWk);

	s_frameStopWatch.alloc(SAMPLING_PERIOD);
	if (s_enableExecProfiling) {
		s_execStopWatch.alloc(SAMPLING_PERIOD);
	}
	if (s_adapt == 4) {
		int freqSmps = nxCalc::max(nxApp::get_int_opt("freq_smps", 4), 2);
		s_freqStopWatch.alloc(freqSmps);
	}

	s_medianFPS = -1.0f;

	DEMO_ADD_CHR(Den);
	DEMO_ADD_CHR(Zoe);
	DEMO_ADD_CHR(Nic);

	ScnObj* pStgObj = Scene::add_obj(STG_NAME);
	if (pStgObj) {
		if (s_lowq) {
			pStgObj->mDisableShadowCast = true;
		}
		if (s_stdqPerspSdw) {
			pStgObj->set_shadow_weight_bias(100.0f);
		}
	}

	cxResourceManager* pRsrcMgr = Scene::get_rsrc_mgr();
	if (pRsrcMgr) {
		if (nxApp::get_bool_opt("gpu_preload", false)) {
			nxCore::dbg_msg("RsrcMgr: preloading all GPU resources.\n");
			pRsrcMgr->prepare_all_gfx();
		}
	}

	Scene::glb_rng_reset();

	Scene::enable_split_move(nxApp::get_bool_opt("split_move", true));
	nxCore::dbg_msg("Scene::split_move: %s\n", Scene::is_split_move_enabled() ? "Yes" : "No");

	s_kbdCtrl.init();
}

static void set_fog0() {
	Scene::set_fog_rgb(0.748f, 0.74f, 1.2965f);
	Scene::set_fog_density(1.0f);
	Scene::set_fog_range(1.0f, 2000.0f);
	Scene::set_fog_curve(0.1f, 0.1f);
}

static void set_fog() {
	Scene::set_fog_rgb(0.748f, 0.74f, 0.65f);
	Scene::set_fog_density(1.0f);
	Scene::set_fog_range(20.0f, 2000.0f);
	Scene::set_fog_curve(0.21f, 0.3f);
}

static void set_scene_ctx() {
	Scene::reset_color_correction();

#if 0
	Scene::set_linear_white_rgb(0.5f, 0.5f, 0.95f);
	Scene::set_linear_gain(1.32f);
	Scene::set_linear_bias(-0.002f * 2);

	Scene::set_gamma_rgb(2.0f, 2.0f, 1.95f);
	Scene::set_exposure_rgb(4.0f, 4.0f, 3.8f);
#else
	float whiteOffs = 0.1f;
	Scene::set_linear_white_rgb(0.5f + whiteOffs, 0.45f + whiteOffs, 0.95f + whiteOffs);
	Scene::set_linear_gain(1.25f);
	Scene::set_linear_bias(-0.002f);

	float g0 = 0.0f;
	Scene::set_gamma_rgb(2.12f - g0, 2.05f - g0, 1.95f - g0);
	Scene::set_exposure_rgb(4.5f, 4.0f, 3.5f);

	SCENARIO_WK* pScenario = s_pScenarioWk;
	if (pScenario) {
		if (pScenario->mStateNow == eScenarioState::EXAMINE) {
			Scene::set_exposure_rgb(1.5f, 1.0f, 0.5f);
		} else if (pScenario->mStateNow == eScenarioState::TALK) {
			Scene::set_exposure_rgb(1.25f, 0.8f, 0.5f);
		}
	}
#endif

	if (s_lowq) {
		Scene::set_shadow_density(1.1f);
		Scene::set_shadow_fade(17.0f, 20.0f);
		Scene::set_shadow_proj_params(10.0f, 20.0f, 50.0f);
		Scene::set_shadow_uniform(false);
	} else {
		Scene::set_shadow_density(1.1f);
		Scene::set_shadow_fade(35.0f, 40.0f);
		Scene::set_shadow_proj_params(40.0f, 40.0f, 100.0f);
		if (s_stdqPerspSdw) {
			Scene::set_shadow_proj_params(5.0f, 20.0f, 50.0f);
			Scene::set_shadow_uniform(false);
		} else {
			Scene::set_shadow_uniform(true);
		}
	}

	set_fog();

	Scene::set_hemi_upper(1.1f, 1.09f, 1.12f);
	Scene::set_hemi_lower(0.12f, 0.08f, 0.06f);
	cxVec up = cxVec(0.0f, 1.0f, 0.0f);
	//up = nxVec::rot_deg(up, -20, 30, 0);
	Scene::set_hemi_up(up);
	Scene::set_hemi_exp(2.5f);
	Scene::set_hemi_gain(1.0f);
	if (0) {
		Scene::set_hemi_exp(3.75f);
		Scene::set_hemi_gain(2.0f);
	}
	Scene::set_shadow_dir_degrees(50.0f, 110.0f);
	Scene::set_spec_dir_to_shadow();
	Scene::set_spec_shadowing(0.9f);
}

static void view_exec() {
	VIEW_WK* pWk = s_pViewWk;
	if (!pWk) return;
	if (s_kbdCtrl.ck_trg(KBD_CTRL::BACK)) {
		pWk->viewMode ^= 1;
	}
	if (s_kbdCtrl.ck_trg(KBD_CTRL::TAB)) {
		pWk->tgtMode ^= 1;
	}
	ScnObj* pObj = Scene::find_obj(pWk->tgtMode ? "Zoe" : "Den");
	if (!pObj) {
		pObj = Scene::find_obj("Den");
	}
	if (pObj) {
		static float dy = -80.0f;
		cxVec wpos = pObj->get_world_pos();
		cxAABB bbox = pObj->get_world_bbox();
		float cy = bbox.get_center().y;
		float yoffs = cy - bbox.get_min_pos().y;
		pWk->tgt = wpos + cxVec(0.0f, yoffs + 0.6f, 0.0f);
		cxVec offs = cxVec(1.0f, 1.0f, 6.0f);
		if (s_viewDist != 0.0f) {
			offs = offs.get_normalized() * s_viewDist;
		}
		if (pWk->viewMode) {
			offs = cxVec(-4.0f, 4.0f, 12.0f);
		}
		dy = nxCalc::fit(wpos.z, -12.5f, 12.5f, -140.0f, -50.0f);// -140.0f, -50.0f
		if (dy >= -140.0f && dy <= -120.0f) {
			dy = nxCalc::fit(dy, -140.0f, -120.0f, -105.0f, -90.0f);
		}
		pWk->pos = pWk->tgt + nxQuat::from_degrees(0, dy, 0).apply(offs);
		pWk->pos.z = nxCalc::max(pWk->pos.z, -10.0f);
		if (Scene::get_frame_count() > 0) {
			pWk->pos = nxCalc::approach(pWk->prevPos, pWk->pos, adj_counter_to_speed(30));
			pWk->tgt = nxCalc::approach(pWk->prevTgt, pWk->tgt, adj_counter_to_speed(8));
		}
		pWk->prevPos = pWk->pos;
		pWk->prevTgt = pWk->tgt;
		Scene::set_view(pWk->pos, pWk->tgt);
	}
	Scene::set_view_range(0.5f, 100.0f *2);
	//Scene::set_deg_FOVY(25.0f);

	//Scene::set_spec_dir(Scene::get_view_dir());////////////spec headlight
}

static void timer_ctrl() {
	static bool initFlg = false;
	static double startMicros = 0.0f;
	if (initFlg) {
		s_runningTimeMicros = nxSys::time_micros() - startMicros;
		if (s_frameStopWatch.end()) {
			double us = s_frameStopWatch.median();
			s_frameStopWatch.reset();
			double millis = us / 1000.0;
			double secs = millis / 1000.0;
			double fps = nxCalc::rcp0(secs);
			s_medianFPS = float(fps);
			if (s_echoFPS) {
				nxCore::dbg_msg("median frame time %.3f millis (%.2f FPS)\n", millis, fps);
			}
			Scene::thermal_info();
			Scene::battery_info();
		}
		s_frameStopWatch.begin();
	} else {
		startMicros = nxSys::time_micros();
		s_frameStopWatch.begin();
		initFlg = true;
	}
}

static bool is_small_screen() {
	bool res = false;
	if (Scene::get_view_mode_width() > Scene::get_view_mode_height()) {
		res = Scene::get_view_mode_width() < 500;
	}
	return res;
}

static void draw_msg(
	const char* pMsg,
	const float tboxOX, const float tboxOY,
	const float tboxW, const float tboxH,
	const float msgOX, const float msgOY,
	const float fontW, const float fontH,
	const cxColor& tboxClr, const cxColor& tboxOutClr,
	const cxColor& msgClr, const bool footer
) {
	Scene::set_font_size(tboxW, tboxH);
	Scene::symbol(94, tboxOX, tboxOY, tboxClr, &tboxOutClr);
	if (!s_pScenarioWk) return;
	if (!pMsg) return;
	Scene::set_font_size(fontW, fontH);
	float msgX = msgOX;
	float msgY = msgOY;
	const char* pMsgSrc = pMsg;
	char* pMsgBuf = s_pScenarioWk->mTxtBuf;
	size_t msgLen = 0;
	bool emitFlg = false;
	bool endFlg = false;
	bool nextLine = false;
	while (true) {
		char c = *pMsgSrc++;
		if (c == 0) {
			emitFlg = true;
			endFlg = true;
		} else if (c == 0xA) {
			nextLine = true;
			emitFlg = true;
		} else {
			if (msgLen < SCENARIO_TXT_BUF_SIZE - 1) {
				pMsgBuf[msgLen] = c;
				++msgLen;
			}
		}
		if (emitFlg) {
			if (msgLen > 0) {
				pMsgBuf[msgLen] = 0;
				Scene::symbol_str(pMsgBuf, msgX, msgY, msgClr);
				msgLen = 0;
			}
			emitFlg = false;
		}
		if (endFlg) {
			break;
		}
		if (nextLine) {
			msgY -= fontH + 4.0f;
			nextLine = false;
		}
	}
	if (footer) {
		Scene::symbol_str("* * *", tboxOX + (tboxW / 2.0f) - (fontW * 1.5f), tboxOY + 8.0f, msgClr);
	}
}

static void draw_examine_text() {
	SCENARIO_WK* pWk = s_pScenarioWk;
	if (!pWk) return;

	cxColor tboxClr(1.0f, 1.0f, 1.0f, 0.75f);
	cxColor tboxOutClr(0.0f, 0.0f, 0.0f, 0.5f);
	cxColor msgClr(0.39f, 0.35f, 0.35f, 1.0f);
	if (nxCore::str_starts_with(pWk->mpExmName, EXM_EVAL_PREFIX)) {
		float tboxW = 320.0f;
		float tboxH = 80.0f;
		float fontW = 20.0f;
		float fontH = 24.0f;
		float tboxOX = (Scene::get_ref_scr_width() - tboxW) / 2.0f;
		float tboxOY = (Scene::get_ref_scr_height() - tboxH) / 2.0f;
		float msgOX = tboxOX + 16.0f;
		float msgOY = tboxOY + tboxH - fontH - 24.0f;
		char* pEvalBuf = &pWk->mTxtBuf[SCENARIO_TXT_BUF_SIZE / 2];
		float val = pWk->eval_expr(pWk->mpExmText);
		int ival = int(val);
		if (val == float(ival)) {
			XD_SPRINTF(XD_SPRINTF_BUF(pEvalBuf, (SCENARIO_TXT_BUF_SIZE / 2)), "%s: %d", pWk->mpExmText, ival);
		} else {
			XD_SPRINTF(XD_SPRINTF_BUF(pEvalBuf, (SCENARIO_TXT_BUF_SIZE / 2)), "%s: %.5f", pWk->mpExmText, val);
		}
		draw_msg(
			pEvalBuf,
			tboxOX, tboxOY, tboxW, tboxH,
			msgOX, msgOY, fontW, fontH,
			tboxClr, tboxOutClr, msgClr,
			false
		);
	} else {
		float tboxW = 580.0f;
		float tboxH = 200.0f;
		float fontW = 20.0f;
		float fontH = 24.0f;
		if (is_small_screen()) {
			tboxW += 80.0f;
			tboxH += 10.0f;
			fontW += 2.25f;
			fontH += 2.0f;
		}
		float tboxOX = (Scene::get_ref_scr_width() - tboxW) / 2.0f;
		float tboxOY = (Scene::get_ref_scr_height() - tboxH) / 2.0f;
		float msgOX = tboxOX + 8.0f;
		float msgOY = tboxOY + tboxH - fontH - 16.0f;
		draw_msg(
			pWk->mpExmText,
			tboxOX, tboxOY, tboxW, tboxH,
			msgOX, msgOY, fontW, fontH,
			tboxClr, tboxOutClr, msgClr,
			true
		);
	}
}

static void draw_talk_dlg0() {
	cxColor tboxClr(0.2f, 0.2f, 0.3f, 0.75f);
	cxColor tboxOutClr(1.0f, 0.0f, 0.0f, 0.5f);
	cxColor msgClr(0.99f, 0.35f, 0.35f, 1.0f);
	float tboxW = 320.0f;
	float tboxH = 200.0f;
	float tboxOX = (Scene::get_ref_scr_width() - tboxW) / 2.0f;
	float tboxOY = (Scene::get_ref_scr_height() - tboxH) / 2.0f;
	Scene::set_font_size(tboxW, tboxH);
	Scene::symbol(94, tboxOX, tboxOY, tboxClr, &tboxOutClr);

	float fontW = 20.0f;
	float fontH = 24.0f;
	Scene::set_font_size(fontW, fontH);
	Scene::symbol_str("> Under construction <", tboxOX + (tboxW / 2.0f) - (fontW * 4.5f), tboxOY + (tboxH / 2.0f), msgClr);
}

static void draw_talk_dlg() {
	if (!s_pScenarioWk || !s_pScenarioWk->mpTalkTgt || !s_pScenarioWk->mpTalkTgt->mpVals) {
		draw_talk_dlg0();
		return;
	}
	const char* pDlgName = actr_get_dlg_name(s_pScenarioWk->mpTalkTgt);
	if (!pDlgName) {
		draw_talk_dlg0();
		return;
	}
	sxValuesData::Group dlgGrp = s_pScenarioWk->mpTalkTgt->mpVals->find_grp(pDlgName);
	if (!dlgGrp.is_valid()) {
		draw_talk_dlg0();
		return;
	}
	int textIdx = dlgGrp.find_val_idx("text");
	if (!dlgGrp.ck_val_idx(textIdx)) {
		draw_talk_dlg0();
		return;
	}
	const char* pTextSrc = dlgGrp.get_val_s(textIdx);
	if (!pTextSrc) {
		draw_talk_dlg0();
		return;
	}
	cxColor tboxClr(1.0f, 1.0f, 1.0f, 0.75f);
	cxColor tboxOutClr(0.0f, 0.0f, 0.0f, 0.5f);
	cxColor msgClr(0.39f, 0.35f, 0.35f, 1.0f);
	float tboxW = 528.0f;
	float tboxH = 200.0f;
	float fontW = 20.0f;
	float fontH = 20.0f;
	if (is_small_screen()) {
		tboxW += 100.0f;
		tboxH += 40.0f;
		fontW += 4.0f;
		fontH += 4.0f;
	}
	float tboxOX = (Scene::get_ref_scr_width() - tboxW) / 2.0f;
	float tboxOY = (Scene::get_ref_scr_height() - tboxH) / 2.0f;
	float msgOX = tboxOX + 20.0f;
	float msgOY = tboxOY + tboxH - fontH - 40.0f;
	draw_msg(
		pTextSrc,
		tboxOX, tboxOY, tboxW, tboxH,
		msgOX, msgOY, fontW, fontH,
		tboxClr, tboxOutClr, msgClr,
		true
	);
	if (pDlgName) {
		const char* pChrName = nullptr;
		if (nxCore::str_starts_with(pDlgName, "plyr_")) {
			pChrName = "Den";
		} else if (nxCore::str_starts_with(pDlgName, "actr_")) {
			pChrName = actr_get_name(s_pScenarioWk->mpTalkTgt);
		}
		if (pChrName) {
			cxColor chrNameClr(0.1f, 0.2f, 0.3f, 1.0f);
			Scene::symbol_str(pChrName, msgOX - 8.0f, msgOY + fontH + 8.0f, chrNameClr);
		}
	}
}

static void draw_2d() {
	char str[512];
	const char* fpsStr = "FPS: ";
	float refSizeX = 800;
	float refSizeY = 600;
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
	float bw = 80.0f + 124.0f + 106.0f + 10.0f;
	float bh = 12.0f;
	cxColor bclr(0.0f, 0.0f, 0.0f, 0.75f);
	bpos[0].set(sx - bx, sy - by);
	bpos[1].set(sx + bw + bx, sy - by);
	bpos[2].set(sx + bw + bx, sy + bh + by);
	bpos[3].set(sx - bx, sy + bh + by);
	Scene::quad(bpos, btex, bclr);

	if (s_medianFPS < 0.0f) {
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s--", fpsStr);
	} else {
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s%.2f", fpsStr, s_medianFPS);
	}
	cxColor clr(0.1f, 0.75f, 0.1f, 1.0f);
	//Scene::print(sx + 1.0f, sy + 1.0f, cxColor(0.0f, 0.0f, 0.0f, 1.0f), str);
	Scene::print(sx, sy, clr, str);

	if (s_pScenarioWk) {
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "Can Talk: %s", s_pScenarioWk->mCanTalk ? "Yes" : "No");
		sx += 100.0f;
		Scene::print(sx, sy, cxColor(0.5f, 0.5f, 0.1f), str);

		if (0) {
			XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "Exm Pol: %d", s_pScenarioWk->mExmPolId);
			sx += 120.0f;
			Scene::print(sx, sy, cxColor(0.6f, 0.5f, 0.2f), str);
		} else {
			XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s", "Exm:");
			sx += 120.0f;
			clr = cxColor(0.6f, 0.6f, 0.2f);
			Scene::print(sx, sy, clr, str);
			const char* pExmName = "none";
			if (s_pScenarioWk->mpExmName) {
				clr.set(1.2f, 0.2f, 0.1f);
				if (nxCore::str_starts_with(s_pScenarioWk->mpExmName, EXM_EVAL_PREFIX)) {
					pExmName = "<eval>";
				} else {
					pExmName = s_pScenarioWk->mpExmName;
				}
			}
			XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "%s", pExmName);
			sx += 40.0f;
			Scene::print(sx, sy, clr, str);
		}
	}

	sx = 5.0f;
	sy = 6.0f;
	bw = 486.0f;
	bh = 50.0f;
	bclr.set(1.0f, 1.0f, 1.0f, 0.75f);
	bpos[0].set(sx, sy);
	bpos[1].set(sx + bw, sy);
	bpos[2].set(sx + bw, sy + bh);
	bpos[3].set(sx, sy + bh);
	//Scene::quad(bpos, btex, bclr);
	cxColor outClr(0.0f, 0.0f, 0.0f, 0.5f);

	if (s_pScenarioWk) {
		if (s_pScenarioWk->mStateNow == eScenarioState::CTRL) {
			if (s_pViewWk && s_pViewWk->tgtMode == 0) {
				if (s_pScenarioWk->mCanTalk) {
					/* talk indicator */
					Scene::set_font_size(40.0f, 44.0f);
					cxColor talkColor(1.0f, 0.22f, 0.0f, 1.0f);
					talkColor.to_nonlinear();
					Scene::symbol(96, 372.0f, 350.0f, talkColor, &outClr);
				} else if (s_pScenarioWk->mpExmName) {
					/* examine indicator */
					Scene::set_font_size(120.0f, 124.0f);
					Scene::symbol(95, 400.0f, 350.0f, cxColor(1.0f, 0.2f, 0.1f, 1.0f), &outClr);
				}
			}
		} else if (s_pScenarioWk->mStateNow == eScenarioState::EXAMINE) {
			draw_examine_text();
		} else if (s_pScenarioWk->mStateNow == eScenarioState::TALK) {
			draw_talk_dlg();
		}
	}
}

class ExmPolsHitFunc : public sxGeometryData::HitFunc {
private:
	SCENARIO_WK* mpWk;

public:
	ExmPolsHitFunc(SCENARIO_WK* pWk) : mpWk(pWk) {
		if (pWk) {
			pWk->mExmPolId = -1;
		}
	}

	virtual bool operator()(const sxGeometryData::Polygon& pol, const cxVec& hitPos, const cxVec& hitNrm, float hitDist) {
		if (mpWk) {
			mpWk->mExmPolId = pol.get_id();
		}
		return false;
	}
};

static void scenario_resolve_exm_text() {
	SCENARIO_WK* pWk = s_pScenarioWk;
	if (!pWk) return;
	pWk->mpExmText = nullptr;
	pWk->mpExmTextName = nullptr;
	if (!pWk->mpExmName) return;
	if (!pWk->mpExmVals) return;
	if (nxCore::str_starts_with(pWk->mpExmName, EXM_EVAL_PREFIX)) {
		pWk->mpExmText = pWk->mpExmName + ::strlen(EXM_EVAL_PREFIX);
		return;
	}
	sxValuesData::Group grp = pWk->mpExmVals->find_grp("exm_roof");
	if (!grp.is_valid()) return;
	char nameBuf[128];
	XD_SPRINTF(XD_SPRINTF_BUF(nameBuf, sizeof(nameBuf)), "exm_%s", pWk->mpExmName);
	int valIdx = grp.find_val_idx(nameBuf);
	if (grp.ck_val_idx(valIdx)) {
		pWk->mpExmTextName = pWk->mpExmVals->get_str(pWk->mpExmVals->find_str(nameBuf));
		pWk->mpExmText = grp.get_val_s(valIdx);
	}
	if (!pWk->mpExmText) {
		if (pWk->mpExprLib && pWk->mpExprCtx) {
			int exprIdx = pWk->mpExprLib->find_expr_idx("expr_roof", nameBuf);
			if (exprIdx >= 0) {
				sxExprLibData::Entry ent = pWk->mpExprLib->get_entry(exprIdx);
				if (ent.is_valid()) {
					const sxCompiledExpression* pExpr = ent.get_expr();
					if (pExpr && pExpr->is_valid()) {
						pExpr->exec(*pWk->mpExprCtx);
						int exmId = pWk->mpExprCtx->get_int_result();
						XD_SPRINTF(XD_SPRINTF_BUF(nameBuf, sizeof(nameBuf)), "exm_%s%d", pWk->mpExmName, exmId);
						valIdx = grp.find_val_idx(nameBuf);
						if (grp.ck_val_idx(valIdx)) {
							pWk->mpExmTextName = pWk->mpExmVals->get_str(pWk->mpExmVals->find_str(nameBuf));
							pWk->mpExmText = grp.get_val_s(valIdx);
						}
					}
				}
			}
		}
	}
}

static bool scenario_actr_talk_ck(ScnObj* pObj, void* pWkMem) {
	if (!pObj) return false;
	SCENARIO_WK* pWk = (SCENARIO_WK*)pWkMem;
	if (!pWk) return false;
	ScnObj* pPlyr = pWk->mpPlyr;
	if (!pPlyr) return false;
	ScnObj* pActr = pObj;
	if (!actr_can_talk(pActr)) return true;
	cxMtx wmtxPlyr = pPlyr->get_world_mtx();
	cxMtx wmtxActr = pActr->get_world_mtx();
	cxVec wposPlyr = wmtxPlyr.get_translation();
	cxVec wposActr = wmtxActr.get_translation();
	cxVec wdirPlyr = wmtxPlyr.get_row_vec(2);
	cxVec wdirActr = wmtxActr.get_row_vec(2);
	float dist = nxVec::dist(wposPlyr, wposActr);
	if (dist < 1.5f) {
		cxVec lpos = wmtxActr.get_inverted().calc_pnt(wposPlyr);
		if (lpos.z > 0.0f) {
			float tcos = wdirPlyr.dot(wdirActr);
			if (tcos < 0.0f) {
				tcos = nxCalc::clamp(tcos, -1.0f, 1.0f);
				float talkAng = XD_RAD2DEG(::acosf(tcos));
				if (talkAng > 100.0f) {
					pWk->mCanTalk = true;
					if (pWk->mTalkTgtDist < 0.0f || dist < pWk->mTalkTgtDist) {
						pWk->mTalkTgtDist = dist;
						pWk->mpTalkTgt = pActr;
					}
				}
			}
		}
	}
	return true;
}

static void scenario_exec_ctrl(SCENARIO_WK* pWk) {
	if (!pWk) return;
	pWk->mpExmName = nullptr;
	pWk->mpExmText = nullptr;
	pWk->mCanTalk = false;
	pWk->mpTalkTgt = nullptr;
	pWk->mTalkTgtDist = -1.0f;
	ScnObj* pPlyr = pWk->mpPlyr;
	if (!pPlyr) return;
	cxMtx wmtxPlyr = pPlyr->get_world_mtx();

	Scene::for_each_obj(scenario_actr_talk_ck, pWk);

	if (pWk->mCanTalk && pWk->mpTalkTgt) {
		if (s_kbdCtrl.ck_trg(KBD_CTRL::ENTER)) {
			if (actr_get_dlg_name(pWk->mpTalkTgt)) {
				pWk->mStateNow = eScenarioState::TALK;
			}
		}
		return;
	}

	Pkg* pPkg = Scene::find_pkg(STG_NAME);
	if (pPkg) {
		sxGeometryData* pExmPols = pPkg->find_geometry("exm_pols");
		if (pExmPols) {
			ExmPolsHitFunc exmHit(pWk);
			float exmDist = 1.2f;
			float exmHeight = 0.5f;
			cxVec exmOrg = wmtxPlyr.get_translation();
			exmOrg.y += exmHeight;
			cxVec exmVec = wmtxPlyr.calc_vec(cxVec(0.0f, 0.0f, exmDist));
			exmVec.y += exmHeight;
			cxLineSeg exmSeg(exmOrg, exmOrg + exmVec);
			pExmPols->hit_query(exmSeg, exmHit);
			if (pWk->mExmPolId >= 0) {
				int nameAttr = pExmPols->find_pol_attr("name");
				if (nameAttr >= 0) {
					pWk->mpExmName = pExmPols->get_attr_val_s(nameAttr, sxGeometryData::eAttrClass::POLYGON, pWk->mExmPolId);
				}
			}

			if (pWk->mpExmName) {
				if (s_kbdCtrl.ck_trg(KBD_CTRL::ENTER)) {
					scenario_resolve_exm_text();
					int exmCntIdx = pWk->find_exm_cnt(pWk->mpExmName);
					if (exmCntIdx < 0) {
						exmCntIdx = pWk->mExmCntsNum;
						pWk->mExmCnts[exmCntIdx].pName = pWk->mpExmName;
						pWk->mExmCnts[exmCntIdx].cnt = 0;
						++pWk->mExmCntsNum;
					}
					if (exmCntIdx >= 0) {
						++pWk->mExmCnts[exmCntIdx].cnt;
					}
					if (pWk->mpExmText) {
						pWk->mStateNow = eScenarioState::EXAMINE;
					}
				}
			}
		}
	}
}

static void print_scenario_vars(SCENARIO_WK* pWk) {
	if (!pWk) return;
	nxCore::dbg_msg("%s", "Scenario vars:\n");
	for (int i = 0; i < pWk->mNumVars; ++i) {
		nxCore::dbg_msg(" [%d]: %s = %f\n", i, pWk->mpVarNames[i], pWk->mVars[i]);
	}
}

static bool scenario_eval_exec(SCENARIO_WK* pWk, const char* pExec) {
	bool execFlg = false;
	if (pExec && pWk->mpVarsMap && pWk->mpExprLib) {
		size_t idx = 0;
		size_t execLen = ::strlen(pExec);
		if (execLen > 2) {
			char c = 0;
			size_t execDstOrg = 0;
			while (true) {
				c = pExec[idx++];
				if (c == 0) break;
				if (c == ' ' || c == '=') break;
			}
			size_t execDstLen = idx - 1;
			if (c > 0) {
				size_t execSrcOrg = 0;
				while (true) {
					c = pExec[idx++];
					if (c == 0) break;
					if (c != ' ' || c != '=') {
						execSrcOrg = idx - 1;
						break;
					}
				}
				if (execSrcOrg > 0) {
					size_t execSrcLen = 0;
					while (true) {
						c = pExec[idx++];
						if (c == 0 || c == ' ') {
							execSrcLen = idx - execSrcOrg;
							break;
						}
					}
					if (execSrcLen > 0) {
						char* pBuf = pWk->mTxtBuf;
						::memcpy(pBuf, &pExec[execDstOrg], execDstLen);
						pBuf[execDstLen] = 0;
						int dstIdx = -1;
						if (pWk->mpVarsMap->get(pBuf, &dstIdx) && dstIdx >= 0 && dstIdx < SCENARIO_MAX_VARS) {
							::memcpy(pBuf, &pExec[execSrcOrg], execSrcLen);
							pBuf[execSrcLen] = 0;
							int srcIdx = pWk->mpExprLib->find_expr_idx("expr_roof", pBuf);
							if (srcIdx >= 0) {
								sxExprLibData::Entry ent = pWk->mpExprLib->get_entry(srcIdx);
								if (ent.is_valid()) {
									const sxCompiledExpression* pExpr = ent.get_expr();
									if (pExpr && pExpr->is_valid()) {
										pExpr->exec(*pWk->mpExprCtx);
										float val = pWk->mpExprCtx->mRes;
										pWk->mVars[dstIdx] = val;
										execFlg = true;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return execFlg;
}

static void scenario_exec_examine(SCENARIO_WK* pWk) {
	if (!pWk) return;
	int execCnt = 0;
	if (s_kbdCtrl.ck_trg(KBD_CTRL::ENTER)) {
		if (pWk->mpExmVals && pWk->mpExmTextName && pWk->mpVarsMap && pWk->mpExprLib) {
			sxValuesData::Group grp = pWk->mpExmVals->find_grp("exm_roof");
			if (grp.is_valid() && ::strlen(pWk->mpExmTextName) <= 32) {
				char execName[64];
				for (int i = 0; i < SCENARIO_MAX_EXECS; ++i) {
					XD_SPRINTF(XD_SPRINTF_BUF(execName, sizeof(execName)), "exec_%s_%d", pWk->mpExmTextName, i);
					int execIdx = grp.find_val_idx(execName);
					if (grp.ck_val_idx(execIdx)) {
						const char* pExec = grp.get_val_s(execIdx);
						if (scenario_eval_exec(pWk, pExec)) {
							++execCnt;
						}
					}
				}
			}
		}
		pWk->mStateNow = eScenarioState::CTRL;
	}
}

static void scenario_exec_talk(SCENARIO_WK* pWk) {
	if (!pWk) return;
	if (s_kbdCtrl.ck_trg(KBD_CTRL::ENTER)) {
		bool contFlg = actr_dlg_next(pWk->mpTalkTgt);
		if (!contFlg) {
			pWk->mStateNow = eScenarioState::CTRL;
		}
	}
}

static void scenario_exec() {
	SCENARIO_WK* pWk = s_pScenarioWk;
	if (!pWk) return;
	if (pWk->mStateNow == eScenarioState::EXAMINE) {
		scenario_exec_examine(pWk);
	} else if (pWk->mStateNow == eScenarioState::TALK) {
		scenario_exec_talk(pWk);
	} else {
		scenario_exec_ctrl(pWk);
	}
}

static void prim_test_tri() {
	ScnObj* pObj = Scene::find_obj("Den");
	if (!pObj) return;
	int ijnt = pObj->find_skel_node_id("j_Head");
	cxMtx wm = pObj->calc_skel_world_mtx(ijnt);
	sxPrimVtx tri[3];
	cxVec nrm(0.0f, 0.0f, 1.0f);
	for (int i = 0; i < 3; ++i) {
		tri[i].prm.fill(0.0f);
		tri[i].encode_normal(nrm);
		tri[i].clr.set(0.2f, 0.3f, 0.2f, 0.45f);
		tri[i].tex.fill(0.0f);
	}
	tri[0].pos.set(-1.0f, 0.0f, 0.0, 1.0f);
	tri[1].pos.set(0.0, 1.0f, 0.0, 1.0f);
	tri[2].pos.set(1.0f, 0.0f, 0.0, 1.0f);
	Scene::prim_verts(0, 3, tri);
	Scene::tris_semi_dsided(0, 1, &wm, nullptr);
}

static void prim_test_spr() {
	ScnObj* pObj = Scene::find_obj("Den");
	if (!pObj) return;
	int ijnt = pObj->find_skel_node_id("j_Head");
	cxMtx wm = pObj->calc_skel_world_mtx(ijnt);
	cxVec pos = wm.get_translation();
	pos.y += 0.3f;

	sxPrimVtx tri[3];
	float scl = 0.25f;
	float alpha = 0.6f;
	static float rot = 0.0f;
	for (int i = 0; i < 3; ++i) {
		tri[i].clr.set(0.2f + float(i) * 0.2f, 0.3f, 0.2f, alpha);
		tri[i].tex.fill(0.0f);
		tri[i].pos.set(pos.x, pos.y, pos.z, scl);
	}
	tri[0].prm.set(0.0f, 0.0f, 0.0, rot);
	tri[1].prm.set(-0.1f, 1.0f, 0.0, rot);
	tri[2].prm.set(0.1f, 1.0f, 0.0, rot);
	Scene::prim_verts(0, 3, tri);
	Scene::sprite_tris(0, 1, nullptr);
	rot += 0.02f * (s_motSpeed * 2.0f);
}

static void prim_test_quad() {
	ScnObj* pObj = Scene::find_obj("Den");
	if (!pObj) return;
	int ijnt = pObj->find_skel_node_id("j_Head");
	cxMtx wm = pObj->calc_skel_world_mtx(ijnt);
	sxPrimVtx vtx[4];
	cxVec nrm(0.0f, 0.0f, 1.0f);
	for (int i = 0; i < 4; ++i) {
		vtx[i].prm.fill(0.0f);
		vtx[i].encode_normal(nrm);
		vtx[i].clr.set(0.2f, 0.3f, 0.2f, 0.5f);
		vtx[i].tex.fill(0.0f);
	}
	vtx[0].pos.set(-0.5f, 0.5f, 0.0, 1.0f);
	vtx[1].pos.set(0.5f, 0.5f, 0.0, 1.0f);
	vtx[2].pos.set(0.5f, -0.5f, 0.0, 1.0f);
	vtx[3].pos.set(-0.5f, -0.5f, 0.0, 1.0f);
	uint16_t idx[] = { 0, 1, 2, 0, 2, 3 };
	Scene::prim_geom(0, 4, vtx, 0, 6, idx);
	Scene::idx_tris_semi_dsided(0, 2, &wm, nullptr);
}

static void prim_test() {
	prim_test_tri();
}

static void loop(void* pLoopCtx) {
	static double usStart = 0.0;
	static double usEnd = 0.0;
	timer_ctrl();
	if (s_pScenarioWk) {
		s_pScenarioWk->mStateOld = s_pScenarioWk->mStateNow;
	}
	s_kbdCtrl.update();
	set_scene_ctx();
	if (s_enableExecProfiling) {
		s_execStopWatch.begin();
	}
	Scene::exec();
	scenario_exec();
	view_exec();
	Scene::visibility();
	if (s_enableExecProfiling) {
		if (s_execStopWatch.end()) {
			double us = s_execStopWatch.median();
			s_execStopWatch.reset();
			double millis = us / 1000.0;
			nxCore::dbg_msg("(median exec time %.4f millis)\n", millis);
		}
	}
	Scene::frame_begin(cxColor(0.7f, 0.72f, 0.73f));
	Scene::draw();
	//prim_test();////////////
	draw_2d();
	Scene::frame_end();
	if (s_adapt == 3) {
		OGLSys::finish();
	}
	if (s_adapt) {
		usEnd = nxSys::time_micros();
	}
	if (s_adapt == 4) {
		if (OGLSys::get_frame_count() > 0) {
			if (s_freqStopWatch.end()) {
				double us = s_freqStopWatch.median();
				s_freqStopWatch.reset();
				double millis = us / 1000.0;
				double secs = millis / 1000.0;
				float fps = float(nxCalc::rcp0(secs));
				if (fps >= 57.0f && fps <= 62.0f) {
					s_motSpeed = 0.5f;
				} else if (fps >= 29.0f && fps <= 31.0f) {
					s_motSpeed = 1.0f;
				} else {
					s_motSpeed = nxCalc::div0(60.0f, fps) * 0.5f;
				}
			}
			s_freqStopWatch.begin();
		} else {
			s_freqStopWatch.begin();
			s_motSpeed = 0.5f;
		}
	} else if (s_adapt && OGLSys::get_frame_count() > 0) {
		const double refMillis = 1000.0 / 60.0;
		double frameMillis = (usEnd - usStart) / 1000.0;
		double frameSecs = frameMillis / 1000.0;
		double frameFPS = nxCalc::rcp0(frameSecs);
		if (s_adapt == 1) {
#if defined(OGLSYS_WEB)
			if (frameFPS < 40.0f) {
				s_motSpeed = 1.0f;
			} else {
				s_motSpeed = 0.5f;
			}
#else
			if (frameFPS >= 57.0f && frameFPS <= 62.0f) {
				s_motSpeed = 0.5f;
			} else {
				s_motSpeed = float(0.5 * (frameMillis / refMillis));
			}
#endif
		} else {
			if (frameMillis < refMillis) {
				double sleepMillis = ::round(refMillis - frameMillis);
				if (sleepMillis > 0.0) {
					nxSys::sleep_millis((uint32_t)sleepMillis);
				}
			}
		}
	}
	if (s_adapt) {
		usStart = nxSys::time_micros();
	}
}

static void reset() {
	s_frameStopWatch.free();
	if (s_enableExecProfiling) {
		s_execStopWatch.free();
	}
	if (s_adapt == 4) {
		s_freqStopWatch.free();
	}
	nxCore::mem_free(s_pViewWk);
	s_pViewWk = nullptr;
	if (s_pScenarioWk) {
		if (s_pScenarioWk->mpVarsMap) {
			ScenarioVarMap::destroy(s_pScenarioWk->mpVarsMap);
		}
		if (s_pScenarioWk->mpVarVals) {
			Scene::unload_data_file(s_pScenarioWk->mpVarVals);
		}
		if (s_pScenarioWk->mpExmVals) {
			Scene::unload_data_file(s_pScenarioWk->mpExmVals);
		}
		if (s_pScenarioWk->mpExprCtx) {
			nxCore::tMem<ScenarioExprCtx>::free(s_pScenarioWk->mpExprCtx);
		}
		if (s_pScenarioWk->mpExprLib) {
			Scene::unload_data_file(s_pScenarioWk->mpExprLib);
		}
		nxCore::mem_free(s_pScenarioWk);
		s_pScenarioWk = nullptr;
	}
}

DEMO_REGISTER(roof);

DEMO_PROG_END
