// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2019-2022 Sergey Chaban <sergey.chaban@gmail.com>

#define SCN_EXEC_PRIO_BITS 3
#define SCN_NUM_EXEC_PRIO (1 << (SCN_EXEC_PRIO_BITS))

#ifndef SCN_CMN_PKG_NAME
#	define SCN_CMN_PKG_NAME nullptr
#endif

#ifndef SCN_SCR_CMN_TEX
#	define SCN_SCR_CMN_TEX "scr_common_BASE"
#endif

typedef cxResourceManager::Pkg Pkg;

struct ScnCfg {
	const char* pAppPath;
	const char* pDataDir;
	int shadowMapSize;
	int numWorkers;
	size_t localHeapSize;
	bool useSpec;
	bool useBump;

	void set_defaults();
};

#define SCN_OBJ_SPARE_VARS_NUM 16
#define SCN_OBJ_SPARE_PTRS_NUM 8

struct ScnObj {
public:
	typedef void (*MoveFunc)(ScnObj*);
	typedef void (*ExecFunc)(ScnObj*);
	typedef void (*DelFunc)(ScnObj*);
	typedef void (*DrawCallbackFunc)(ScnObj*);
	typedef void (*BatchCallbackFunc)(ScnObj*, const int);

	struct Priority {
		uint32_t exec : SCN_EXEC_PRIO_BITS;
	};

	struct InstInfo {
		float x, y, z, dx, dy, dz;
		int variation;
		cxQuat get_quat() const { return nxQuat::from_degrees(dx, dy, dz); }
		cxVec get_pos() const { return cxVec(x, y, z); }
	};

private:
	ScnObj() {}

public:
	sxJob mJob;
	char* mpName;
	cxModelWork* mpMdlWk;
	cxMotionWork* mpMotWk;
	const sxJobContext* mpJobCtx;
	MoveFunc mBeforeMotionFunc;
	MoveFunc mAfterMotionFunc;
	MoveFunc mBeforeBlendFunc;
	MoveFunc mAfterBlendFunc;
	MoveFunc mWorldFunc;
	ExecFunc mExecFunc;
	DelFunc mDelFunc;
	DrawCallbackFunc mPreOpaqFunc;
	DrawCallbackFunc mPostOpaqFunc;
	BatchCallbackFunc mBatchPreDrawFunc;
	BatchCallbackFunc mBatchPostDrawFunc;
	sxMotionData* mpMoveMot;
	sxValuesData* mpVals;
	sxJob* mpBatJobs;
	Priority mPriority;
	uint32_t mTag;
	bool mDisableDraw;
	bool mDisableShadowCast;
	bool mDisableShadowRecv;
	float mObjAdjYOffs;
	float mObjAdjRadius;
	int32_t mMotExecSync;
	bool mSplitMoveReqFlg;
	int mRoutine[4];
	int mCounter[4];
	int32_t mIntWk[SCN_OBJ_SPARE_VARS_NUM];
	float mFltWk[SCN_OBJ_SPARE_VARS_NUM];
	void* mPtrWk[SCN_OBJ_SPARE_PTRS_NUM];

	bool ck_name(const char* pName) const { return nxCore::str_eq(mpName, pName); }

	void set_routine(const int r0, const int r1 = 0, const int r2 = 0, const int r3 = 0) {
		mRoutine[0] = r0;
		mRoutine[1] = r1;
		mRoutine[2] = r2;
		mRoutine[3] = r3;
	}

	void set_exec_priority(const int prio) {
		mPriority.exec = nxCalc::clamp(prio, 0, SCN_NUM_EXEC_PRIO - 1);
	}

	sxModelData* get_model_data() { return mpMdlWk ? mpMdlWk->mpData : nullptr; }
	const sxModelData* get_model_data() const { return mpMdlWk ? mpMdlWk->mpData : nullptr; }
	bool has_skel() const { return mpMdlWk && mpMdlWk->has_skel(); }
	int find_skel_node_id(const char* pName) const { return mpMdlWk ? mpMdlWk->find_skel_node_id(pName) : -1; }
	bool ck_skel_id(const int iskl) const { return mpMdlWk ? mpMdlWk->ck_skel_id(iskl) : false; }

	int get_batches_num() const { return mpMdlWk ? mpMdlWk->get_batches_num() : 0; }
	const char* get_batch_mtl_name(const int ibat) const;

	void set_base_color_scl(const float r, const float g, const float b);
	void set_base_color_scl(const float s);
	void set_shadow_offs_bias(const float bias);
	void set_shadow_weight_bias(const float bias);
	void set_shadow_density_scl(const float scl);

	void set_texture_pkg(cxResourceManager::Pkg* pTexPkg);

	void clear_int_wk();
	void clear_flt_wk();
	void clear_ptr_wk();
	bool ck_int_wk_idx(const int idx) { return idx >= 0 && (size_t)idx < XD_ARY_LEN(mIntWk); }
	bool ck_flt_wk_idx(const int idx) { return idx >= 0 && (size_t)idx < XD_ARY_LEN(mFltWk); }
	bool ck_ptr_wk_idx(const int idx) { return idx >= 0 && (size_t)idx < XD_ARY_LEN(mPtrWk); }

	void set_vptr_wk(const int idx, void* p) {
		if (ck_ptr_wk_idx(idx)) {
			mPtrWk[idx] = p;
		}
	}

	template<typename T> void set_ptr_wk(const int idx, T* p) {
		set_vptr_wk(idx, (void*)p);
	}

	template<typename T> T* get_ptr_wk(const int idx) {
		T* p = nullptr;
		if (ck_ptr_wk_idx(idx)) {
			p = (T*)mPtrWk[idx];
		}
		return p;
	}

	sxMotionData* find_motion(const char* pMotName);
	sxValuesData* find_values(const char* pValName);
	int get_current_motion_num_frames() const;
	float get_motion_frame() const;
	void set_motion_frame(const float frame);
	void exec_motion(const sxMotionData* pMot, const float frameAdd);
	void sync_motion();
	void init_motion_blend(const int duration);
	void exec_motion_blend();
	void set_motion_uniform_scl(const float scl);
	float get_motion_uniform_scl() const;
	void set_motion_height_offs(const float offs);
	float get_motion_height_offs() const;

	void update_world();
	void update_skin();
	void update_bounds();
	void update_visibility();
	void update_batch_vilibility(const int ibat);

	void move(const float frameAdd) { move(nullptr, frameAdd); }
	void move(const sxMotionData* pMot, const float frameAdd);
	void move_sub();

	cxMtx get_skel_local_mtx(const int iskl) const;
	cxMtx get_skel_local_rest_mtx(const int iskl) const;
	cxMtx get_skel_prev_world_mtx(const int iskl) const;
	cxMtx get_skel_root_prev_world_mtx() const;
	cxMtx calc_skel_world_mtx(const int iskl, cxMtx* pNodeParentMtx = nullptr) const;
	cxMtx calc_skel_world_rest_mtx(const int iskl) const { return mpMdlWk ? mpMdlWk->calc_skel_node_world_rest_mtx(iskl) : nxMtx::identity(); }

	cxQuat get_skel_local_quat(const int iskl, const bool clean = false) const;
	cxQuat get_skel_local_rest_quat(const int iskl, const bool clean = false) const;
	cxVec get_skel_local_pos(const int iskl) const;
	cxVec get_skel_local_rest_pos(const int iskl) const;

	void set_skel_local_mtx(const int iskl, const cxMtx& mtx);
	void reset_skel_local_mtx(const int iskl);

	void reset_skel_local_quat(const int iskl);
	void set_skel_local_quat(const int iskl, const cxQuat& quat);
	void set_skel_local_quat_pos(const int iskl, const cxQuat& quat, const cxVec& pos);
	void set_skel_local_ty(const int iskl, const float y);

	void set_skel_root_local_tx(const float x);
	void set_skel_root_local_ty(const float y);
	void set_skel_root_local_tz(const float z);
	void set_skel_root_local_pos(const float x, const float y, const float z);

	void set_world_quat(const cxQuat& quat);
	void set_world_quat_pos(const cxQuat& quat, const cxVec& pos);
	void set_world_mtx(const cxMtx& mtx);
	cxMtx get_world_mtx() const;
	cxVec get_world_pos() const;
	float get_world_deg_x() const;
	void add_world_deg_x(const float xadd);
	float get_world_deg_y() const;
	void add_world_deg_y(const float yadd);
	float get_world_deg_z() const;
	void add_world_deg_z(const float zadd);
	void xform_world_deg_xyz(const float dx, const float dy, const float dz, exRotOrd rord = exRotOrd::XYZ);
	void xform_world_quat(const cxQuat& q);
	cxQuat get_world_quat() const;
	cxAABB get_world_bbox() const;
	cxVec get_center_pos() const;
	cxMtx get_prev_world_mtx() const;
	cxVec get_prev_world_pos() const;

	void set_model_variation(const int id) {
		if (mpMdlWk) mpMdlWk->mVariation = id;
	}

	int get_model_variation() const {
		return mpMdlWk ? mpMdlWk->mVariation : 0;
	}

	void hide_mtl(const char* pMtlName, const bool hide = true) {
		if (mpMdlWk) mpMdlWk->hide_mtl(pMtlName, hide);
	}

	void draw_opaq();
	void draw_semi(const bool discard = true);
	void draw(const bool discard = true);
	void shadow_cast();
};

namespace Scene {

void init(const ScnCfg& cfg);
void reset();
const char* get_draw_ifc_name();
cxResourceManager* get_rsrc_mgr();
const char* get_data_path();

Pkg* load_pkg(const char* pName);
Pkg* find_pkg(const char* pName);
Pkg* find_pkg_for_data(sxData* pData);
sxModelData* find_model_in_pkg(Pkg* pPkg, const char* pMdlName);
sxModelData* get_pkg_default_model(Pkg* pPkg);
sxTextureData* find_texture_in_pkg(Pkg* pPkg, const char* pTexName);
sxTextureData* find_texture_for_model(sxModelData* pMdl, const char* pTexName);
sxMotionData* find_motion_in_pkg(Pkg* pPkg, const char* pMotName);
sxMotionData* find_motion_for_model(sxModelData* pMdl, const char* pMotName);
sxCollisionData* find_collision_in_pkg(Pkg* pPkg, const char* pColName);
int get_num_mdls_in_pkg(Pkg* pPkg);
void prepare_pkg_gfx(Pkg* pPkg);
void release_pkg_gfx(Pkg* pPkg);
void prepare_all_gfx();
void release_all_gfx();
void unload_pkg(Pkg* pPkg);
void unload_all_pkgs();
void release_texture(sxTextureData* pTex);

sxData* load_data_file(const char* pRelPath);
void unload_data_file(sxData* pData);
sxImageData* load_img(const char* pRelPath);
sxTextureData* load_tex(const char* pRelPath);
sxGeometryData* load_geo(const char* pRelPath);
sxRigData* load_rig(const char* pRelPath);
sxKeyframesData* load_kfr(const char* pRelPath);
sxValuesData* load_vals(const char* pRelPath);
sxExprLibData* load_expr_lib(const char* pRelPath);

void frame_begin(const cxColor& clearColor = cxColor(0.0f, 0.0f, 0.0f, 1.0f));
void frame_end();
uint64_t get_frame_count();

void push_ctx();
void pop_ctx();

int get_num_workers();
int get_num_active_workers();
int get_visibility_job_lvl();
int get_wrk_jobs_done_cnt(const int lvl, const int wrkId);
int get_lvl_jobs_done_cnt(const int lvl);

void alloc_local_heaps(const size_t localHeapSize);
void free_local_heaps();
void purge_local_heaps();
cxHeap* get_local_heap(const int id);
cxHeap* get_job_local_heap(const sxJobContext* pJobCtx);

void enable_split_move(const bool flg);
bool is_split_move_enabled();

void alloc_global_heap(const size_t globalHeapSize);
void free_global_heap();
void purge_global_heap();
cxHeap* get_global_heap();
void* glb_mem_alloc(const size_t size, const uint32_t tag);
void glb_mem_free(void* pMem);

void mem_info();
void thermal_info();
void battery_info();

void glb_rng_reset();
void glb_rng_seed(const uint64_t seed);
uint64_t glb_rng_next();
float glb_rng_f01();

void set_view(const cxVec& pos, const cxVec& tgt, const cxVec& up = cxVec(0.0f, 1.0f, 0.0f));
void set_view_range(const float znear, const float zfar);
void set_deg_FOVY(const float fovy);
const cxFrustum* get_view_frustum_ptr();
cxMtx get_view_mtx();
cxMtx get_view_proj_mtx();
cxMtx get_proj_mtx();
cxMtx get_inv_view_mtx();
cxMtx get_inv_proj_mtx();
cxMtx get_inv_view_proj_mtx();
cxMtx get_view_mode_mtx();
cxVec get_view_pos();
cxVec get_view_tgt();
cxVec get_view_dir();
cxVec get_view_up();
float get_view_near();
float get_view_far();
int get_screen_width();
int get_screen_height();
int get_view_mode_width();
int get_view_mode_height();
sxView::Mode get_view_mode();
bool is_rot_view();

bool is_sphere_visible(const cxSphere& sph, const bool exact = true);

bool is_shadow_uniform();
void set_shadow_uniform(const bool flg);
void set_shadow_density(const float dens);
void set_shadow_density_bias(const float bias);
void set_shadow_fade(const float start, const float end);
void set_shadow_proj_params(const float size, const float margin, const float dist = 50.0f);
void set_shadow_dir(const cxVec& sdir);
void set_shadow_dir_degrees(const float dx, const float dy);
cxVec get_shadow_dir();
cxMtx get_shadow_view_proj_mtx();

void set_hemi_upper(const float r, const float g, const float b);
void set_hemi_lower(const float r, const float g, const float b);
void set_hemi_const(const float r, const float g, const float b);
void set_hemi_const(const float val);
void scl_hemi_upper(const float s);
void scl_hemi_lower(const float s);
void set_hemi_up(const cxVec& v);
void set_hemi_exp(const float e);
void set_hemi_gain(const float g);
void reset_hemi();

void set_spec_dir(const cxVec& v);
void set_spec_dir_to_shadow();
void set_spec_rgb(const float r, const float g, const float b);
void set_spec_shadowing(const float s);

void set_fog_rgb(const float r, const float g, const float b);
void set_fog_density(const float a);
void set_fog_range(const float start, const float end);
void set_fog_curve(const float cp1, const float cp2);
void set_fog_linear();
void reset_fog();

void set_gamma(const float gamma);
void set_gamma_rgb(const float r, const float g, const float b);
void set_exposure(const float e);
void set_exposure_rgb(const float r, const float g, const float b);
void set_linear_white(const float val);
void set_linear_white_rgb(const float r, const float g, const float b);
void set_linear_gain(const float val);
void set_linear_gain_rgb(const float r, const float g, const float b);
void set_linear_bias(const float val);
void set_linear_bias_rgb(const float r, const float g, const float b);
void reset_color_correction();

ScnObj* add_obj(sxModelData* pMdl, const char* pName = nullptr);
ScnObj* add_obj(Pkg* pPkg, const char* pName = nullptr);
ScnObj* add_obj(const char* pName);
ScnObj* find_obj(const char* pName);
void del_obj(ScnObj* pObj);
void del_all_objs();
int add_all_pkg_objs(Pkg* pPkg, const char* pNamePrefix = nullptr);
int add_all_pkg_objs(const char* pPkgName, const char* pNamePrefix = nullptr);
void for_all_pkg_models(Pkg* pPkg, void (*func)(sxModelData*, void*), void* pFuncData);
void add_obj_instances(const char* pPkgName, const ScnObj::InstInfo* pInstInfos, const int num, const char* pName = nullptr);
int get_num_objs();
void for_each_obj(bool (*func)(ScnObj*, void*), void* pWkMem);
void set_obj_exec_func(const char* pName, ScnObj::ExecFunc exec);
void set_obj_del_func(const char* pName, ScnObj::DelFunc del);
cxVec get_obj_world_pos(const char* pName);
cxVec get_obj_center_pos(const char* pName);

void init_prims(const uint32_t maxVtx, const uint32_t maxIdx);
void prim_verts(const uint32_t org, const uint32_t num, const sxPrimVtx* pSrc);
void prim_geom(const uint32_t vorg, const uint32_t vnum, const sxPrimVtx* pVtxSrc, const uint32_t iorg, const uint32_t inum, const uint16_t* pIdxSrc);
void tris_semi_dsided(const uint32_t vtxOrg, const uint32_t triNum, cxMtx* pMtx, sxTextureData* pTex);
void tris_semi(const uint32_t vtxOrg, const uint32_t triNum, cxMtx* pMtx, sxTextureData* pTex);
void idx_tris_semi_dsided(const uint32_t idxOrg, const uint32_t triNum, cxMtx* pMtx, sxTextureData* pTex);
void sprite_tris(const uint32_t vtxOrg, const uint32_t triNum, sxTextureData* pTex);

void set_ref_scr_size(const float w, const float h);
float get_ref_scr_width();
float get_ref_scr_height();
void set_quad_gamma(const float gval);
void set_quad_gamma_rgb(const float r, const float g, const float b);
void quad(const xt_float2 pos[4], const xt_float2 tex[4], const cxColor clr, sxTextureData* pTex = nullptr, cxColor* pClrs = nullptr);

void set_font_size(const float w, const float h);
void set_font_symbol_spacing(const float val);
void set_font_space_width(const float val);
float get_font_width();
float get_font_height();
void symbol(const int sym, const float ox, const float oy, const cxColor clr = cxColor(1.0f), const cxColor* pOutClr = nullptr);
void symbol_str(const char* pStr, const float ox, const float oy, const cxColor clr = cxColor(1.0f));

float get_ground_height(sxCollisionData* pCol, const cxVec pos, const float offsTop = 1.8f, const float offsBtm = 0.5f);
bool wall_adj_base(const sxJobContext* pJobCtx, sxCollisionData* pCol, const cxVec& newPos, const cxVec& oldPos, const float radius, cxVec* pAdjPos, const float wallSlopeLim = 0.7f);
bool wall_adj(const sxJobContext* pJobCtx, sxCollisionData* pCol, const cxVec& newPos, const cxVec& oldPos, const float radius, cxVec* pAdjPos, const float wallSlopeLim = 0.7f, const float errParam = 0.5f);
bool wall_touch(const sxJobContext* pJobCtx, sxCollisionData* pCol, const cxVec& newPos, const cxVec& oldPos, const float radius, const float wallSlopeLim = 0.7f);

bool sph_sph_adj(const cxVec& newPos, const cxVec& oldPos, float radius, const cxVec& staticPos, const float staticRadius, cxVec* pAdjPos, const float reflectFactor = 0.5f, const float margin = 0.0f);
bool sph_cap_adj(const cxVec& newPos, const cxVec& oldPos, float radius, const cxVec& staticPos0, const cxVec& staticPos1, float staticRadius, cxVec* pAdjPos, const float reflectFactor = 0.5f, const float margin = 0.0f);

void exec();
void visibility();
void draw(bool discard = true);

void print(const float x, const float y, const cxColor& clr, const char* pStr);

float speed();

} // Scene
