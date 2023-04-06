// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2021-2022 Sergey Chaban <sergey.chaban@gmail.com>

struct SmpChar {

	enum Action {
		ACT_STAND,
		ACT_TURN_L,
		ACT_TURN_R,
		ACT_WALK,
		ACT_RETREAT,
		ACT_RUN
	};

	typedef void (*CtrlFunc)(SmpChar*);

	struct Descr {
		const char* pName;
		int variation;
		float scale;

		void reset() {
			pName = nullptr;
			variation = 0;
			scale = 1.0f;
		}
	};

	struct Rig : public SmpRig {
		SmpChar* mpChar;

		void init(SmpChar* pChar);
		void exec();
	};

	struct MotLib {
		sxMotionData* pStand;
		sxMotionData* pTurnL;
		sxMotionData* pTurnR;
		sxMotionData* pWalk;
		sxMotionData* pRetreat;
		sxMotionData* pRun;

		void init(const Pkg* pPkg);
	};


	Rig mRig;
	MotLib mMotLib;
	ScnObj* mpObj;
	CtrlFunc mCtrlFunc;
	int32_t mAction;
	double mActionStartTime;
	double mActionDuration;
	int mObjAdjCount;
	int mObjTouchCount;
	int mWallTouchCount;
	double mObjTouchStartTime;
	double mObjTouchDuration;
	double mWallTouchStartTime;
	double mWallTouchDuration;

	void set_motion(sxMotionData* pMot) {
		if (mpObj) {
			mpObj->mpMoveMot = pMot ? pMot : mMotLib.pStand;
		}
	}

	void select_motion() {
		sxMotionData* pMot = nullptr;
		switch (mAction) {
			case ACT_STAND: pMot = mMotLib.pStand; break;
			case ACT_TURN_L: pMot = mMotLib.pTurnL; break;
			case ACT_TURN_R: pMot = mMotLib.pTurnR; break;
			case ACT_WALK: pMot = mMotLib.pWalk; break;
			case ACT_RETREAT: pMot = mMotLib.pRetreat; break;
			case ACT_RUN: pMot = mMotLib.pRun; break;
			default: break;
		}
		set_motion(pMot);
	}

	void set_act_duration_seconds(const double secs) {
		mActionDuration = nxCalc::max(secs, 0.0) * 1000.0;
	}

	double get_act_time() const;
	bool check_act_time_out() const;
	void change_act(const int newAct, const double durationSecs);
	void add_deg_y(const float dy);

	double get_obj_touch_duration_secs() const { return mObjTouchDuration / 1000.0; }
	double get_wall_touch_duration_secs() const { return mWallTouchDuration / 1000.0; }

	void reset_wall_touch() {
		mWallTouchCount = 0;
		mWallTouchDuration = 0.0;
	}

	template<typename T> void set_ptr_wk(const int idx, T* p) {
		if (mpObj && idx >= 0) {
			mpObj->set_ptr_wk(idx + 2, p);
		}
	}

	template<typename T> T* get_ptr_wk(const int idx) {
		T* p = nullptr;
		if (mpObj && idx >= 0) {
			p = mpObj->get_ptr_wk<T>(idx);
		}
		return p;
	}

	void obj_adj();
	void wall_adj();
	void ground_adj();
	void exec_collision();
};


namespace SmpCharSys {

	void init();
	void reset();

	void start_frame();
	double get_current_time();
	float get_motion_speed();
	float get_fps();

	bool obj_is_char(ScnObj* pObj);
	SmpChar* char_from_obj(ScnObj* pObj);
	ScnObj* add_f(const SmpChar::Descr& descr, SmpChar::CtrlFunc ctrl = nullptr);
	ScnObj* add_m(const SmpChar::Descr& descr, SmpChar::CtrlFunc ctrl = nullptr);

	void set_collision(sxCollisionData* pCol);
	sxCollisionData* get_collision();

} // SmpCharSys
