#ifndef mac_ifc_h
#define mac_ifc_h

#ifdef __cplusplus
extern "C" {
#endif

void mac_init(const char* pAppPath, int w, int h);
void mac_exec();
void mac_stop();

void mac_mouse_down(int btn, float x, float y);
void mac_mouse_up(int btn, float x, float y);
void mac_mouse_move(float x, float y);

void mac_kbd(const char* pName, const bool state);

#ifdef __cplusplus
}
#endif

#endif /* mac_ifc_h */
