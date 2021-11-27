#include "crosscore.hpp"
#include "scene.hpp"
#include "demo.hpp"

DEMO_PROG_BEGIN

static void torus_exec(ScnObj* pObj) {
	if (!pObj) return;
#if 0
	static float degX = 0.0f;
	static float degY = 0.0f;
	pObj->set_world_quat_pos(nxQuat::from_degrees(degX, degY, 0.0f), cxVec(0.0f, 0.0f, 0.0f));
	degX += 0.5f;
	degY += 1.0f;
#else
	cxQuat qrot = nxQuat::from_degrees(0.0f, 1.0f, 0.0f);
	pObj->xform_world_quat(qrot);
#endif
}

static void init() {
	Scene::add_all_pkg_objs("ex1");
	Scene::set_obj_exec_func("torus", torus_exec);
}

static void view_exec() {
	ScnObj* pObj = Scene::find_obj("torus");
	if (pObj) {
		cxVec tgt = pObj->get_world_pos();
		cxVec pos = tgt + cxVec(0.0f, 0.0f, 8.0f);
		Scene::set_view(pos, tgt);
	}
}

static void set_scene_ctx() {
	Scene::set_spec_dir_to_shadow();
	Scene::set_spec_shadowing(0.9f);
}

static void loop(void* pLoopCtx) {
	set_scene_ctx();
	Scene::exec();
	view_exec();
	Scene::visibility();
	Scene::frame_begin(cxColor(0.25f));
	Scene::draw();
	Scene::frame_end();
}

static void reset() {
	// ...
}

DEMO_REGISTER(ex1);

DEMO_PROG_END
