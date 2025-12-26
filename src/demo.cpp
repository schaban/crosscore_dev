#include "crosscore.hpp"
#include "scene.hpp"
#include "demo.hpp"

#ifndef DEF_DEMO
#	define DEF_DEMO "default"
#endif

namespace Demo {

static Ifc* s_ifcLst[64];

int32_t register_demo(Ifc* pIfc) {
	static int32_t s_idx = 0;
	int idx = s_idx;
	if (pIfc) {
		idx = nxSys::atomic_inc(&s_idx) - 1;
		int maxIfcs = (int)XD_ARY_LEN(s_ifcLst);
		if (idx < maxIfcs) {
			s_ifcLst[idx] = pIfc;
			pIfc->info.result = 0;
		} else {
			nxSys::atomic_dec(&s_idx);
			idx = -1;
			pIfc->info.result = 99;
		}
	}
	return idx;
}

Ifc* find_demo(const char* pName) {
	Ifc* pIfc = nullptr;
	if (pName) {
		int n = register_demo(nullptr);
		for (int i = 0; i < n; ++i) {
			if (nxCore::str_eq(pName, s_ifcLst[i]->info.pName)) {
				pIfc = s_ifcLst[i];
				break;
			}
		}
	}
	return pIfc;
}

Ifc* get_demo() {
	static const char* s_pDefName = DEF_DEMO;
	const char* pName = s_pDefName;
	const char* pReqName = nxApp::get_opt("demo");
	if (pReqName) {
		pName = pReqName;
	}
	Ifc* pIfc = find_demo(pName);
	if (!pIfc) {
		pName = s_pDefName;
		pIfc = find_demo(pName);
	}
	return pIfc;
}

ScnObj* add_scn_obj(const char* pName, void (init)(ScnObj*), ScnObj::ExecFunc exec, ScnObj::DelFunc del) {
	ScnObj* pObj = Scene::add_obj(pName);
	if (pObj) {
		if (init) {
			init(pObj);
		}
		pObj->mExecFunc = exec;
		pObj->mDelFunc = del;
	}
	return pObj;
}

} // Demo
