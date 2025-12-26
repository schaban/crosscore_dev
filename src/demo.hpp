#define DEMO_PROG_BEGIN namespace {
#define DEMO_PROG_END }

#define DEMO_REGISTER(_name) \
static Demo::Ifc s_ifc; \
struct DemoReg { \
	DemoReg() { \
		nxCore::mem_zero(&s_ifc, sizeof(s_ifc)); \
		s_ifc.info.pName = #_name; \
		s_ifc.init = init; \
		s_ifc.loop = loop; \
		s_ifc.reset = reset; \
		Demo::register_demo(&s_ifc); \
	} \
} s_demoReg

#define DEMO_ADD_CHR(name) Demo::add_scn_obj(#name, name##_init, name##_exec, name##_del)

namespace Demo {

struct Ifc {
	struct Info {
		const char* pName;
		void* pData;
		int result;
	} info;
	void (*init)();
	void (*loop)(void*);
	void (*reset)();
};

int32_t register_demo(Ifc* pIfc);
Ifc* find_demo(const char* pName);
Ifc* get_demo();

ScnObj* add_scn_obj(const char* pName, void (init)(ScnObj*) = nullptr, ScnObj::ExecFunc exec = nullptr, ScnObj::DelFunc del = nullptr);

} // Demo


