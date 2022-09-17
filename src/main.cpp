#include "crosscore.hpp"
#include "oglsys.hpp"
#include "scene.hpp"
#include "draw.hpp"

#include "demo.hpp"

#ifdef OGLSYS_MACOS
#	include "mac_ifc.h"
#endif

#ifdef OGLSYS_VIVANTE_FB
static const bool c_defBump = false;
static const bool c_defSpec = false;
static const bool c_defReduce = true;
#elif OGLSYS_ES
static const bool c_defBump = false;
static const bool c_defSpec = false;
static const bool c_defReduce = false;
#else
static const bool c_defBump = true;
static const bool c_defSpec = true;
static const bool c_defReduce = false;
#endif

static int c_defW = 1200;
static int c_defH = 700;

extern "C" {

// web-interface functions

void wi_set_key_state(const char* pName, const int state) {
	if (OGLSys::get_frame_count() > 0) {
		//if (pName) { nxCore::dbg_msg("%s:%d\n", pName, state); }
		OGLSys::set_key_state(pName, !!state);
	}
}

} // wi

#ifdef USE_XROM_ARCHIVE

#define XROM_SIG XD_FOURCC('X', 'R', 'O', 'M')
#define XROM_PATH_PREFIX "rom:"

struct ROMHead {
	uint32_t sig;
	uint32_t size;
	uint32_t hsize;
	uint32_t nfiles;
};

struct ROMFileInfo {
	struct Path {
		uint32_t tag; /* len, hash */
		char str[60];

		void reset() {
			tag = 0;
			for (size_t i = 0; i < sizeof(str); ++i) {
				str[i] = 0;
			}
		}

		void set(const char* pStr) {
			reset();
			if (pStr) {
				size_t len = nxCore::str_len(pStr);
				if (len <= sizeof(str)) {
					tag = uint32_t((uint16_t)len) | (uint32_t(nxCore::str_hash16(pStr)) << 16);
					nxCore::mem_copy(str, pStr, len);
				}
			}
		}
		size_t len() const {
			return nxCalc::min(int(tag & 0xFFFF), int(sizeof(str)));
		}
	};

	Path path;
	uint32_t offs;
	uint32_t size;
	uint32_t reserved;
	uint32_t reserved2;
};

extern "C" uint8_t _binary_xrom_start;
static ROMHead* s_pROM = (ROMHead*)&_binary_xrom_start;

XD_NOINLINE static bool rom_valid() {
	return s_pROM && (s_pROM->sig == XROM_SIG);
}

static xt_fhandle rom_fopen(const char* pPath) {
	xt_fhandle fh = nullptr;
	if (pPath && rom_valid()) {
		ROMHead* pROM = s_pROM;
		const char* pPrefix = "./" XROM_PATH_PREFIX "/";
		if (nxCore::str_starts_with(pPath, pPrefix)) {
			const char* pSearchPath = pPath + nxCore::str_len(pPrefix);
			ROMFileInfo::Path search;
			search.set(pSearchPath);
			ROMFileInfo* pInfo = nullptr;
			ROMFileInfo* pInfoROM = (ROMFileInfo*)(pROM + 1);
			for (uint32_t i = 0; i < pROM->nfiles; ++i) {
				if (pInfoROM[i].path.tag == search.tag) {
					if (nxCore::mem_eq(pInfoROM[i].path.str, search.str, search.len())) {
						fh = (xt_fhandle)&pInfoROM[i];
						break;
					}
				}
			}
		}
	}
	return fh;
}

static void rom_fclose(xt_fhandle fh) {
}

static size_t rom_fsize(xt_fhandle fh) {
	size_t size = 0;
	if (fh && rom_valid()) {
		ROMHead* pROM = s_pROM;
		ROMFileInfo* pInfoTop = (ROMFileInfo*)(pROM + 1);
		ROMFileInfo* pInfoEnd = pInfoTop + pROM->nfiles;
		ROMFileInfo* pInfo = (ROMFileInfo*)fh;
		if (pInfo >= pInfoTop && pInfo < pInfoEnd) {
			size = pInfo->size;
		} else {
			nxCore::dbg_msg("rom_fsize: invalid handle\n");
		}
	}
	return size;
}

static size_t rom_fread(xt_fhandle fh, void* pDst, size_t nbytes) {
	size_t nread = 0;
	if (rom_valid() && fh && pDst && nbytes > 0) {
		ROMHead* pROM = s_pROM;
		ROMFileInfo* pInfoTop = (ROMFileInfo*)(pROM + 1);
		ROMFileInfo* pInfoEnd = pInfoTop + pROM->nfiles;
		ROMFileInfo* pInfo = (ROMFileInfo*)fh;
		if (pInfo >= pInfoTop && pInfo < pInfoEnd) {
			void* pSrc = XD_INCR_PTR(pROM, pInfo->offs);
			nread = pInfo->size;
			nxCore::mem_copy(pDst, pSrc, nread);
		} else {
			nxCore::dbg_msg("rom_fread: invalid handle\n");
		}
	}
	return nread;
}
#endif // USE_XROM_ARCHIVE


static void dbgmsg_impl(const char* pMsg) {
	::printf("%s", pMsg);
	::fflush(stdout);
}

static void init_sys() {
	sxSysIfc sysIfc;
	nxCore::mem_zero(&sysIfc, sizeof(sysIfc));
	sysIfc.fn_dbgmsg = dbgmsg_impl;
#ifdef USE_XROM_ARCHIVE
	sysIfc.fn_fopen = rom_fopen;
	sysIfc.fn_fclose = rom_fclose;
	sysIfc.fn_fread = rom_fread;
	sysIfc.fn_fsize = rom_fsize;
#endif
	nxSys::init(&sysIfc);
}

static void reset_sys() {
}

static void* oglsys_mem_alloc(size_t size, const char* pTag) {
	return nxCore::mem_alloc(size, pTag);
}

static void oglsys_mem_free(void* pMem) {
	nxCore::mem_free(pMem);
}

static const char* oglsys_get_opt(const char* pName) {
	return nxApp::get_opt(pName);
}

static void init_ogl(const int x, const int y, const int w, const int h, const int msaa) {
	OGLSysCfg cfg;
	cfg.clear();
	cfg.x = x;
	cfg.y = y;
	cfg.width = w;
	cfg.height = h;
	cfg.msaa = msaa;
	cfg.reduceRes = nxApp::get_bool_opt("reduce", c_defReduce);
	cfg.ifc.dbg_msg = nxCore::dbg_msg;
	cfg.ifc.mem_alloc = oglsys_mem_alloc;
	cfg.ifc.mem_free = oglsys_mem_free;
	cfg.ifc.get_opt = oglsys_get_opt;
	Draw::Ifc* pDrawIfc = Draw::get_ifc_impl();
	if (pDrawIfc) {
		cfg.withoutCtx = !pDrawIfc->info.needOGLContext;
	}
	OGLSys::init(cfg);
	OGLSys::CL::init();
	OGLSys::set_swap_interval(nxApp::get_int_opt("swap", 1));
	//nxCore::dbg_msg("SPIR-V support: %d\n", OGLSys::ext_ck_spv());
}

static void reset_ogl() {
	OGLSys::CL::reset();
	OGLSys::reset();
}


static void init_scn(const char* pAppPath) {
	const int defSmapScl =
#if OGLSYS_ES
		1
#else
		2
#endif
	;

	const char* pAltDataDir =
#ifdef USE_XROM_ARCHIVE
	XROM_PATH_PREFIX
#else
	nxApp::get_opt("data")
#endif
	;
	ScnCfg scnCfg;
	scnCfg.set_defaults();
	if (pAltDataDir) {
		scnCfg.pDataDir = pAltDataDir;
	}
	scnCfg.pAppPath = pAppPath;
	scnCfg.shadowMapSize = nxApp::get_int_opt("smap", 1024 * defSmapScl);
	scnCfg.numWorkers = nxApp::get_int_opt("nwrk", 4);
	scnCfg.useBump = nxApp::get_bool_opt("bump", c_defBump);
	scnCfg.useSpec = nxApp::get_bool_opt("spec", c_defSpec);
	nxCore::dbg_msg("#workers: %d\n", scnCfg.numWorkers);
	nxCore::dbg_msg("shadow size: %d\n", scnCfg.shadowMapSize);
	Scene::init(scnCfg);

	if (nxApp::get_int_opt("spersp", 0) != 0) {
		Scene::set_shadow_uniform(false);
	}
}

static void exec_demo() {
	Demo::Ifc* pIfc = Demo::get_demo();
	if (pIfc) {
		nxCore::dbg_msg("executing demo program: %s\n", pIfc->info.pName);
		pIfc->init();
		Scene::mem_info();
		OGLSys::loop(pIfc->loop, &pIfc->info);
		pIfc->reset();
	}
}


#ifdef OGLSYS_MACOS
static Demo::Ifc* s_pIfc = nullptr;
static bool s_macStartFlg = false;

void mac_start(int argc, const char* argv[]) {
	nxApp::init_params(argc, (char**)argv);
	s_macStartFlg = true;
}

void mac_init(const char* pAppPath) {
	if (!s_macStartFlg) {
		return;
	}

	int w = mac_get_width_opt();
	int h = mac_get_height_opt();

	init_sys();
	init_ogl(0, 0, w, h, true);
	init_scn(pAppPath);

	s_pIfc = Demo::get_demo();
	if (s_pIfc) {
		nxCore::dbg_msg("executing demo program: %s\n", s_pIfc->info.pName);
		s_pIfc->init();
		Scene::mem_info();
	}
}

void mac_exec() {
	if (s_pIfc && s_pIfc->loop) {
		s_pIfc->loop(&s_pIfc->info);
	}
}

void mac_stop() {
	s_macStartFlg = false;

	if (s_pIfc) {
		s_pIfc->reset();
		s_pIfc = nullptr;
	}

	Scene::reset();
	reset_ogl();

	nxApp::reset();
	nxCore::mem_dbg();
}

int mac_get_int_opt(const char* pName) {
	int val = 0;
	if (s_macStartFlg && pName) {
		val = nxApp::get_int_opt(pName, 0);
	}
	return val;
}

bool mac_get_bool_opt(const char* pName) {
	bool val = false;
	if (s_macStartFlg && pName) {
		val = nxApp::get_bool_opt(pName, false);
	}
	return val;
}

float mac_get_float_opt(const char* pName) {
	float val = 0.0f;
	if (s_macStartFlg && pName) {
		val = nxApp::get_float_opt(pName, 0.0f);
	}
	return val;
}

int mac_get_width_opt() {
	int w = c_defW;
	if (s_macStartFlg) {
		w = nxCalc::max(32, nxApp::get_int_opt("w", c_defW));
	}
	return w;
}

int mac_get_height_opt() {
	int h = c_defH;
	if (s_macStartFlg) {
		h = nxCalc::max(32, nxApp::get_int_opt("h", c_defH));
	}
	return h;
}

static float mac_mouse_y(float y) {
	return float(OGLSys::get_height()) - y;
}

void mac_mouse_down(int btn, float x, float y) {
	OGLSys::send_mouse_down((OGLSysMouseState::BTN)btn, x, mac_mouse_y(y), false);
}

void mac_mouse_up(int btn, float x, float y) {
	OGLSys::send_mouse_up((OGLSysMouseState::BTN)btn, x, mac_mouse_y(y), false);
}

void mac_mouse_move(float x, float y) {
	OGLSys::send_mouse_move(x, mac_mouse_y(y));
}

void mac_kbd(const char* pName, const bool state) {
	OGLSys::set_key_state(pName, state);
}

#else

int main(int argc, char* argv[]) {
	nxApp::init_params(argc, argv);
	init_sys();

	float scrScl = 1.0f;
	int x = 10;
	int y = 10;
	int w = c_defW;
	int h = c_defH;

	w = int(float(w) * scrScl);
	h = int(float(h) * scrScl);

	w = nxCalc::max(32, nxApp::get_int_opt("w", w));
	h = nxCalc::max(32, nxApp::get_int_opt("h", h));

	int msaa = nxApp::get_int_opt("msaa", 0);

	init_ogl(x, y, w, h, msaa);
	init_scn(argv[0]);

	exec_demo();

	Scene::reset();
	reset_ogl();

	nxApp::reset();
	nxCore::mem_dbg();

	return 0;
}

#endif
