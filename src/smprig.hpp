// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2021-2022 Sergey Chaban <sergey.chaban@gmail.com>

struct SmpRig {
	struct LegInfo {
		int inodeTop;
		int inodeRot;
		int inodeEnd;

		void init(const ScnObj* pObj, const char side);
	};

	struct SupJnt {
		xt_float3 param;
		uint8_t idst;
		uint8_t isrc;
		char axis;
		bool enabled;

		void reset() {
			param.fill(0.0f);
			idst = 0;
			isrc = 0;
			axis = 0;
			enabled = false;
		}
	};

	SupJnt mShoulderL;
	SupJnt mShoulderR;
	SupJnt mElbowL;
	SupJnt mElbowR;
	SupJnt mForearmL;
	SupJnt mForearmR;
	SupJnt mHipL;
	SupJnt mHipR;
	SupJnt mKneeL;
	SupJnt mKneeR;
	LegInfo mLegL;
	LegInfo mLegR;
	float mAnkleHeight;
	float mScale;
	ScnObj* mpObj;

	void init(ScnObj* pObj, sxValuesData* pVals);
	void exec(sxCollisionData* pCol);
};
