#ifndef mac_ifc_h
#define mac_ifc_h

#ifdef __cplusplus
extern "C" {
#endif

void mac_start(int argc, const char* argv[]);
void mac_init(const char* pAppPath);
void mac_exec();
void mac_stop();

int mac_get_int_opt(const char* pName);
bool mac_get_bool_opt(const char* pName);
float mac_get_float_opt(const char* pName);
int mac_get_width_opt();
int mac_get_height_opt();

void mac_mouse_down(int btn, float x, float y);
void mac_mouse_up(int btn, float x, float y);
void mac_mouse_move(float x, float y);

void mac_kbd(const char* pName, const bool state);

#ifdef __cplusplus
}
#endif

#endif /* mac_ifc_h */
