#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define DRM_EVENT_CONTEXT_VERSION 4

typedef struct _drmEventContext {
	int version;
	void (*vblank_handler)(int fd, unsigned int seq, unsigned int tv_sec, unsigned int tv_usec, void* pData);
	void (*page_flip_handler)(int fd, unsigned int seq, unsigned int tv_sec, unsigned int tv_usec, void* pData);
	void (*page_flip_handler2)(int fd, unsigned int seq, unsigned int tv_sec, unsigned int tv_usec, unsigned int icrtc, void* pData);
	void (*sequence_handler)(int fd, uint64_t seq, uint64_t ns, uint64_t data);
} drmEventContext;

int drmHandleEvent(int fd, drmEventContext* pCtx);

#if defined(__cplusplus)
}
#endif
