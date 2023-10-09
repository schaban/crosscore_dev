#ifndef _GBM_MIMIC_H_
#define _GBM_MIMIC_H_ 1

#define __GBM__ 1

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define __gbm_fourcc_code(a,b,c,d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define GBM_FORMAT_XRGB8888 __gbm_fourcc_code('X', 'R', '2', '4')

#define GBM_BO_USE_SCANOUT 1
#define GBM_BO_USE_RENDERING (1 << 2)

struct gbm_surface;
struct gbm_device;
struct gbm_bo;

union gbm_bo_handle {
	void* ptr;
	int32_t s32;
	uint32_t u32;
	int64_t s64;
	uint64_t u64;
};

struct gbm_device* gbm_bo_get_device(struct gbm_bo* pBO);

union gbm_bo_handle gbm_bo_get_handle(struct gbm_bo* pBO);

uint32_t gbm_bo_get_height(struct gbm_bo* pBO);

uint32_t gbm_bo_get_stride(struct gbm_bo* pBO);

void* gbm_bo_get_user_data(struct gbm_bo* pBO);

uint32_t gbm_bo_get_width(struct gbm_bo* pBO);

void gbm_bo_set_user_data(struct gbm_bo* pBO, void* pData, void (*destroy_user_data)(gbm_bo*, void*));

struct gbm_device* gbm_create_device(int fd);

int gbm_device_get_fd(struct gbm_device* pDev);

struct gbm_surface* gbm_surface_create(struct gbm_device* pDev, uint32_t width, uint32_t height, uint32_t format, uint32_t flags);

struct gbm_bo* gbm_surface_lock_front_buffer(struct gbm_surface* pSurf);

void gbm_surface_release_buffer(struct gbm_surface* pSurf, struct gbm_bo* pBO);

#if defined(__cplusplus)
}
#endif

#endif /* _GBM_MIMIC_H_ */
