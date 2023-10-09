#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
	DRM_MODE_CONNECTED = 1,
	DRM_MODE_DISCONNECTED,
	DRM_MODE_UNKNOWNCONNECTION
} drmModeConnection;

typedef enum {
	DRM_MODE_SUBPIXEL_UNKNOWN  = 1,
	DRM_MODE_SUBPIXEL_HORIZONTAL_RGB,
	DRM_MODE_SUBPIXEL_HORIZONTAL_BGR,
	DRM_MODE_SUBPIXEL_VERTICAL_RGB,
	DRM_MODE_SUBPIXEL_VERTICAL_BGR,
	DRM_MODE_SUBPIXEL_NONE
} drmModeSubPixel;

struct _drmModeModeInfo {
	uint32_t clock;
	uint16_t hdisplay, hsync_start, hsync_end, htotal, hskew;
	uint16_t vdisplay, vsync_start, vsync_end, vtotal, vscan;

	uint32_t vrefresh;

	uint32_t flags;
	uint32_t type;
	char name[DRM_DISPLAY_MODE_LEN];
};

typedef struct _drmModeModeInfo drmModeModeInfo;

struct _drmModeEncoder {
	uint32_t encoder_id;
	uint32_t encoder_type;
	uint32_t crtc_id;
	uint32_t possible_crtcs;
	uint32_t possible_clones;
};

struct _drmModeConnector {
	uint32_t connector_id;
	uint32_t encoder_id;
	uint32_t connector_type;
	uint32_t connector_type_id;
	drmModeConnection connection;
	uint32_t mmWidth, mmHeight;
	drmModeSubPixel subpixel;

	int count_modes;
	drmModeModeInfo* modes;

	int count_props;
	uint32_t* props;
        uint64_t* prop_values;

	int count_encoders;
	uint32_t* encoders;
};

struct _drmModeRes {
	int count_fbs;
	uint32_t* fbs;

	int count_crtcs;
	uint32_t* crtcs;

	int count_connectors;
	uint32_t* connectors;

	int count_encoders;
	uint32_t* encoders;

	uint32_t min_width, max_width;
	uint32_t min_height, max_height;
};

typedef struct _drmModeConnector drmModeConnector;
typedef struct _drmModeEncoder drmModeEncoder;
typedef struct _drmModeRes drmModeRes;

int drmModeAddFB(int fd, uint32_t w, uint32_t h, uint8_t d, uint8_t bpp, uint32_t pitch, uint32_t bo_handle, uint32_t* pBuff_id);

void drmModeFreeConnector(drmModeConnector* p);

void drmModeFreeEncoder(drmModeEncoder* p);

drmModeConnector* drmModeGetConnector(int fd, uint32_t connId);

drmModeEncoder* drmModeGetEncoder(int fd, uint32_t encId);

drmModeRes* drmModeGetResources(int fd);

int drmModePageFlip(int fd, uint32_t icrtc, uint32_t ifb, uint32_t flags, void* pData);

int drmModeRmFB(int fd, uint32_t buffId);

int drmModeSetCrtc(int fd, uint32_t crtcId, uint32_t buffId, uint32_t x, uint32_t y, uint32_t* pConns, int count, drmModeModeInfo* pMmode);

#if defined(__cplusplus)
}
#endif
