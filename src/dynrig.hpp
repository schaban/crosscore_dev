struct Ponytail {
	struct Adj {
		cxSphere mSph;
		int mJntId;
	};

	struct Node {
		cxMtx mWMtx;
		cxMtx mPrevWMtx;
		cxVec mPos;
		cxVec mPrevPos;
		cxVec mAdd;
		float mMaxDist;
		float mForce;
		float mRate;
		float mLimitTheta;
		float mLimitPhi;
		int mJntId;
		uint32_t mAttr;
	};

	ScnObj* mpObj;
	Node mNodes[12];
	Adj mAdjs[8];
	int mNumNodes;
	int mNumAdjs;

	void init(ScnObj* pObj, const char* pPrefix = nullptr);
	void reset_nodes();
	void move();
};

struct LegInfo {
	float effY;
	float effOffsX;
	float effOffsZ;
	int inodeTop;
	int inodeRot;
	int inodeEnd;
	int inodeExt;
	int inodeEff;

	void init(const ScnObj* pObj, const char side, const bool ext);
};

struct SupportJntInfo {
	struct Jnts {
		int elbowJntL;
		int elbowSupL;
		int elbowJntR;
		int elbowSupR;
		int wristL;
		int forearmL;
		int wristR;
		int forearmR;
		int hipJntL;
		int hipSupL;
		int hipJntR;
		int hipSupR;
		int kneeJntL;
		int kneeSupL;
		int kneeJntR;
		int kneeSupR;
	};
	struct Params {
		float elbowInfl;
		float wristInfl;
		float hipInfl;
		float kneeInfl;
	};

	Jnts jnts;
	Params params;

	void init(const ScnObj* pObj, Params* pParams = nullptr);
};

namespace DynRig {

void calc_forearm_twist(ScnObj* pObj, const int wristId, const int forearmId, const float wristInfluence = 0.5f);
void calc_forearm_twist(ScnObj* pObj, const char* pWristName, const char* pForearmName, const float wristInfluence = 0.5f);
void calc_forearm_twist_l(ScnObj* pObj, const float wristInfluence = 0.5f);
void calc_forearm_twist_r(ScnObj* pObj, const float wristInfluence = 0.5f);

void calc_elbow_bend(ScnObj* pObj, const int elbowJntId, const int elbowSupId, const float influence = 0.5f);
void calc_elbow_bend(ScnObj* pObj, const char* pElbowJntName, const char* pElbowSupName, const float influence = 0.5f);
void calc_elbow_bend_l(ScnObj* pObj, const float influence = 0.5f);
void calc_elbow_bend_r(ScnObj* pObj, const float influence = 0.5f);

void reset_shoulder_rot(ScnObj* pObj, const int shoulderId);
void reset_shoulder_rot(ScnObj* pObj, const char* pName);
void reset_shoulder_rot_l(ScnObj* pObj);
void reset_shoulder_rot_r(ScnObj* pObj);

void calc_shoulder_axis_rot(ScnObj* pObj, const int shoulderJntId, const int shoulderId, const float shoulderJntInfluence, const int axisIdx, const bool useQuats = false);
void calc_shoulder_axis_rot(ScnObj* pObj, const char* pShoulderJntName, const char* pShoulderName, const float shoulderJntInfluence, const int axisIdx);
void calc_shoulder_axis_rot_l(ScnObj* pObj, const float shoulderJntInfluence, const int axisIdx);
void calc_shoulder_axis_rot_r(ScnObj* pObj, const float shoulderJntInfluence, const int axisIdx);

void calc_hip_adj(ScnObj* pObj, const int hipJntId, const int hipSupId, const float influence);
void calc_hip_adj(ScnObj* pObj, const char* pHipJntName, const char* pHipSupName, const float influence);
void calc_hip_adj_l(ScnObj* pObj, const float influence);
void calc_hip_adj_r(ScnObj* pObj, const float influence);

void calc_knee_adj(ScnObj* pObj, const int kneeJntId, const int kneeSupId, const float influence);
void calc_knee_adj(ScnObj* pObj, const char* pKneeJntName, const char* pKneeSupName, const float influence);
void calc_knee_adj_l(ScnObj* pObj, const float influence);
void calc_knee_adj_r(ScnObj* pObj, const float influence);

void calc_thigh_adj(ScnObj* pObj, const int hipJntId, const int thighUprId, const float influenceUpr, const int thighLwrId, const float influenceLwr);
void calc_thigh_adj(ScnObj* pObj, const char* pHipJntName, const char* pThighUprName, const float influenceUpr, const char* pThighLwrName, const float influenceLwr);
void calc_thigh_adj_l(ScnObj* pObj, const float influenceUpr, const float influenceLwr);
void calc_thigh_adj_r(ScnObj* pObj, const float influenceUpr, const float influenceLwr);

void calc_eyelids_blink(ScnObj* pObj, const float yopen, const float yclosed, const float t, const float p1 = 0.65f, const float p2 = 0.95f);

void adjust_leg(ScnObj* pObj, sxCollisionData::NearestHit (*gndHitFunc)(const cxLineSeg& seg, void* pData), void* pFuncData, LegInfo* pLeg);
void adjust_leg(ScnObj* pObj, sxCollisionData* pCol, LegInfo* pLeg);

void calc_sup_jnts(ScnObj* pObj, const SupportJntInfo* pInfo);

} // DynRig

