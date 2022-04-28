/*
 * crosscore utility library
 * Author: Sergey Chaban <sergey.chaban@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2019-2022 Sergey Chaban
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#if defined(_WINDOWS) || defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#	define XD_SYS_WINDOWS
#elif defined(__APPLE__)
#	define XD_SYS_APPLE
#elif defined(__linux) || defined(__linux__) || defined(__gnu_linux__) || defined(linux)
#	define XD_SYS_LINUX
#elif defined(ANDROID)
#	define XD_SYS_ANDROID
#elif defined(__OpenBSD__)
#	define XD_SYS_BSD
#	define XD_SYS_OPENBSD
#elif defined(__FreeBSD__)
#	define XD_SYS_BSD
#	define XD_SYS_FREEBSD
#elif defined(__NetBSD__)
#	define XD_SYS_BSD
#	define XD_SYS_NETBSD
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_MSC_VER)
#	include <malloc.h>
#	include <intrin.h>
#	pragma intrinsic(_InterlockedIncrement)
#	pragma intrinsic(_InterlockedDecrement)
#	pragma intrinsic(_InterlockedExchangeAdd)
#	define XD_MSC_ATOMIC
#	pragma intrinsic(_byteswap_ulong)
#endif

#ifdef XD_USE_OMP
#	include <omp.h>
#endif

#ifdef XD_USE_VEMA
#	include "vema.h"
	template<typename T> inline T mth_isnan(T x) { return false; }
#	define mth_frexpf VEMA_FN(FrExpF)
#	define mth_ldexpf VEMA_FN(LdExpF)
#	define mth_floorf VEMA_FN(FloorF)
#	define mth_ceilf VEMA_FN(CeilF)
#	define mth_roundf VEMA_FN(RoundF)
#	define mth_truncf VEMA_FN(TruncF)
#	define mth_fmodf VEMA_FN(ModF)
#	define mth_fabsf VEMA_FN(AbsF)
#	define mth_sqrtf VEMA_FN(SqrtF)
#	define mth_sinf VEMA_FN(SinF)
#	define mth_cosf VEMA_FN(CosF)
#	define mth_tanf VEMA_FN(TanF)
#	define mth_asinf VEMA_FN(ArcSinF)
#	define mth_acosf VEMA_FN(ArcCosF)
#	define mth_atanf VEMA_FN(ArcTanF)
#	define mth_atan2f VEMA_FN(ArcTan2F)
#	define mth_log10f VEMA_FN(Log10F)
#	define mth_logf VEMA_FN(LogF)
#	define mth_expf VEMA_FN(ExpF)
#	define mth_powf VEMA_FN(PowF)
#	define mth_floor VEMA_FN(FloorD)
#	define mth_ceil VEMA_FN(CeilD)
#	define mth_round VEMA_FN(RoundD)
#	define mth_fabs VEMA_FN(AbsD)
#	define mth_sqrt VEMA_FN(SqrtD)
#	define mth_pow VEMA_FN(PowD)
#	define mth_sin VEMA_FN(SinD)
#	define mth_cos VEMA_FN(CosD)
#	define mth_asin VEMA_FN(ArcSinD)
#	define mth_acos VEMA_FN(ArcCosD)
#else
#	define _USE_MATH_DEFINES
#	include <math.h>
#	define mth_isnan isnan
#	define mth_frexpf frexpf
#	define mth_ldexpf ldexpf
#	define mth_floorf floorf
#	define mth_ceilf ceilf
#	define mth_roundf roundf
#	define mth_truncf truncf
#	define mth_fmodf fmodf
#	define mth_fabsf fabsf
#	define mth_sqrtf sqrtf
#	define mth_sinf sinf
#	define mth_cosf cosf
#	define mth_tanf tanf
#	define mth_asinf asinf
#	define mth_acosf acosf
#	define mth_atanf atanf
#	define mth_atan2f atan2f
#	define mth_log10f log10f
#	define mth_logf logf
#	define mth_expf expf
#	define mth_powf powf
#	define mth_floor floor
#	define mth_ceil ceil
#	define mth_round round
#	define mth_fabs fabs
#	define mth_sqrt sqrt
#	define mth_pow pow
#	define mth_sin sin
#	define mth_cos cos
#	define mth_asin asin
#	define mth_acos acos
#endif
#include <float.h>

#include <new>

#ifndef XD_INLINE
#	define XD_INLINE inline
#endif

#ifndef XD_NOINLINE
#	if defined(_MSC_VER)
#		define XD_NOINLINE __declspec(noinline)
#	elif defined(__GNUC__)
#		define XD_NOINLINE __attribute__((noinline))
#	else
#		define XD_NOINLINE
#	endif
#endif

#ifndef XD_FORCEINLINE
#	if defined(_MSC_VER)
#		define XD_FORCEINLINE __forceinline
#	elif defined(__GNUC__)
#		define XD_FORCEINLINE __inline__ __attribute__((__always_inline__))
#	else
#		define XD_FORCEINLINE inline
#	endif
#endif

#ifndef XD_USE_LA
#	define XD_USE_LA 1
#endif

#define XD_MAX_PATH 4096

#define XD_FOURCC(c1, c2, c3, c4) ( (uint32_t)((((uint8_t)(c4))<<24)|(((uint8_t)(c3))<<16)|(((uint8_t)(c2))<<8)|((uint8_t)(c1))) )
#define XD_INCR_PTR(_ptr, _inc) ( &((uint8_t*)(_ptr))[_inc] )
#define XD_ARY_LEN(_arr) (sizeof((_arr)) / sizeof((_arr)[0]))
#define XD_ALIGN(_x, _n) ( ((uintptr_t)(_x) + ((_n) - 1)) & (~((_n) - 1)) )

#define XD_BIT_ARY_SIZE(_storage_t, _nbits) ((((int)(_nbits)-1)/(sizeof(_storage_t)*8))+1)
#define XD_BIT_ARY_DECL(_storage_t, _name, _nbits) _storage_t _name[XD_BIT_ARY_SIZE(_storage_t, _nbits)]
#define XD_BIT_ARY_IDX(_storage_t, _no) ((_no) / (sizeof(_storage_t)*8))
#define XD_BIT_ARY_MASK(_storage_t, _no) (1 << ((_no)&((sizeof(_storage_t)*8) - 1)))
#define XD_BIT_ARY_ST(_storage_t, _ary, _no) (_ary)[XD_BIT_ARY_IDX(_storage_t, _no)] |= XD_BIT_ARY_MASK(_storage_t, _no)
#define XD_BIT_ARY_CL(_storage_t, _ary, _no) (_ary)[XD_BIT_ARY_IDX(_storage_t, _no)] &= ~XD_BIT_ARY_MASK(_storage_t, _no)
#define XD_BIT_ARY_SW(_storage_t, _ary, _no) (_ary)[XD_BIT_ARY_IDX(_storage_t, _no)] ^= XD_BIT_ARY_MASK(_storage_t, _no)
#define XD_BIT_ARY_CK(_storage_t, _ary, _no) (!!((_ary)[XD_BIT_ARY_IDX(_storage_t, _no)] & XD_BIT_ARY_MASK(_storage_t, _no)))

#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif
#define XD_PI ((float)M_PI)

#define XD_DEG2RAD(_deg) ( (_deg) * (XD_PI / 180.0f) )
#define XD_RAD2DEG(_rad) ( (_rad) * (180.0f / XD_PI) )

#ifdef _MSC_VER
#	define XD_SPRINTF ::sprintf_s
#	define XD_SPRINTF_BUF(_buf, _bufsize)  (_buf), (_bufsize)
#else
#	define XD_SPRINTF ::sprintf
#	define XD_SPRINTF_BUF(_buf, _bufsize)  (_buf)
#endif

typedef void* xt_fhandle;

struct sxSysIfc {
	void* (*fn_malloc)(size_t);
	void (*fn_free)(void*);
	xt_fhandle (*fn_fopen)(const char*);
	void (*fn_fclose)(xt_fhandle);
	size_t (*fn_fsize)(xt_fhandle);
	size_t (*fn_fread)(xt_fhandle, void*, size_t);
	void (*fn_dbgmsg)(const char*);
};

struct sxLock;
struct sxSignal;
struct sxWorker;

typedef void (*xt_worker_func)(void*);

class cxMotionWork;
class cxModelWork;

namespace nxSys {

void init(sxSysIfc* pIfc);

void* malloc(size_t);
void free(void*);
xt_fhandle fopen(const char*);
void fclose(xt_fhandle);
size_t fsize(xt_fhandle);
size_t fread(xt_fhandle, void*, size_t);
void dbgmsg(const char*);

FILE* fopen_w_txt(const char* fpath);
FILE* fopen_w_bin(const char* fpath);

double time_micros();
void sleep_millis(uint32_t millis);

sxLock* lock_create();
void lock_destroy(sxLock* pLock);
bool lock_acquire(sxLock* pLock);
bool lock_release(sxLock* pLock);

sxSignal* signal_create();
void signal_destroy(sxSignal* pSig);
bool signal_wait(sxSignal* pSig);
bool signal_set(sxSignal* pSig);
bool signal_reset(sxSignal* pSig);

sxWorker* worker_create(xt_worker_func func, void* pData);
void worker_destroy(sxWorker* pWrk);
void worker_exec(sxWorker* pWrk);
void worker_wait(sxWorker* pWrk);
void worker_stop(sxWorker* pWrk);

#if defined(XD_MSC_ATOMIC)
inline int32_t atomic_inc(int32_t* p) { return int32_t(_InterlockedIncrement((long*)p)); }
inline int32_t atomic_dec(int32_t* p) { return int32_t(_InterlockedDecrement((long*)p)); }
inline int32_t atomic_add(int32_t* p, const int32_t val) { return int32_t(_InterlockedExchangeAdd((long*)p, (long)val)) + val; }
#else
int32_t atomic_inc(int32_t* p);
int32_t atomic_dec(int32_t* p);
int32_t atomic_add(int32_t* p, const int32_t val);
#endif

bool is_LE();

inline uint32_t bswap_u32(uint32_t x) {
#if defined(_MSC_VER)
	return _byteswap_ulong(x);
#else
	uint32_t b0 = x & 0xFF;
	uint32_t b1 = (x >> 8) & 0xFF;
	uint32_t b2 = (x >> 16) & 0xFF;
	uint32_t b3 = (x >> 24) & 0xFF;
	return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
#endif
}

inline void mem_bswap_u16(uint16_t* pMem) {
	uint8_t* pBytes = (uint8_t*)pMem;
	uint8_t b0 = pBytes[0];
	uint8_t b1 = pBytes[1];
	pBytes[0] = b1;
	pBytes[1] = b0;
}

inline void mem_bswap_i16(int16_t* pMem) {
	mem_bswap_u16((uint16_t*)pMem);
}

inline void mem_bswap_u32(uint32_t* pMem) {
	uint8_t* pBytes = (uint8_t*)pMem;
	uint8_t b0 = pBytes[0];
	uint8_t b1 = pBytes[1];
	uint8_t b2 = pBytes[2];
	uint8_t b3 = pBytes[3];
	pBytes[0] = b3;
	pBytes[1] = b2;
	pBytes[2] = b1;
	pBytes[3] = b0;
}

inline void mem_bswap_i32(int32_t* pMem) {
	mem_bswap_u32((uint32_t*)pMem);
}

} // nxSys


struct sxRNG { uint64_t s[2]; };


namespace nxCore {

void* mem_alloc(const size_t size, const char* pTag = "XMEM", int alignment = 0x10);
void* mem_realloc(void* pMem, size_t newSize, int alignment = 0x10);
void* mem_resize(void* pMem, float factor, int alignment = 0x10);
void mem_free(void* pMem);
size_t mem_size(void* pMem);
const char* mem_tag(void* pMem);
void mem_dbg();
uint64_t mem_allocated_bytes();
uint64_t mem_peak_bytes();
void dbg_break(const char* pMsg);
void dbg_msg(const char* pFmt, ...);
void* bin_load(const char* pPath, size_t* pSize = nullptr, bool appendPath = false, bool unpack = false);
void bin_unload(void* pMem);
void bin_save(const char* pPath, const void* pMem, size_t size);
uint32_t str_hash32(const char* pStr);
uint16_t str_hash16(const char* pStr);
bool str_eq(const char* pStrA, const char* pStrB);
bool str_starts_with(const char* pStr, const char* pPrefix);
bool str_ends_with(const char* pStr, const char* pPostfix);
char* str_dup(const char* pSrc, const char* pTag = "xStr");

template <typename T> struct tMem {
	static void ctor(T* pTop, int n = 1) {
		T* pObj = pTop;
		for (int i = 0; i < n; ++i) {
			void* pMem = pObj;
			::new(pMem) T;
			++pObj;
		}
	}

	static void dtor(T* pMem, int n = 1) {
		T* pObj = pMem + (n - 1);
		for (int i = 0; i < n; ++i) {
			pObj->~T();
			--pObj;
		}
	}

	static T* alloc(int n = 1, const char* pTag = "xMem:new") {
		void* pMem = mem_alloc(sizeof(T)*n, pTag);
		T* pTop = reinterpret_cast<T*>(pMem);
		ctor(pTop, n);
		return pTop;
	}

	static void free(T* pMem, int n = 1) {
		if (pMem) {
			dtor(pMem, n);
			mem_free(pMem);
		}
	}
};

inline bool is_pow2(uint32_t val) {
	return (val & (val - 1)) == 0;
}

inline uintptr_t align_pad(uintptr_t x, int a) {
	return ((x + (a - 1)) / a) * a;
}

uint16_t float_to_half(const float x);
float half_to_float(const uint16_t h);
float f32_set_bits(const uint32_t x);
uint32_t f32_get_bits(const float x);
inline uint32_t f32_get_exp_bits(const float x) { return (f32_get_bits(x) >> 23) & 0xFF; }
inline int f32_get_exp(const float x) { return (int)f32_get_exp_bits(x) - 127; }
inline float f32_mk_nan() { return f32_set_bits(0xFFC00000); }
bool f32_almost_eq(const float x, const float y, const float tol = 0.0001f);

uint32_t fetch_bits32(const uint8_t* pTop, const uint32_t org, const uint32_t len);

inline int clz32(uint32_t x) {
	if (x == 0) return 32;
	int n = 0;
	int s = 16;
	while (s) {
		uint32_t m = ((1U << s) - 1) << (32 - s);
		if ((x & m) == 0) {
			n += s;
			x <<= s;
		}
		s >>= 1;
	}
	return n;
}

inline int ctz32(uint32_t x) {
	if (x == 0) return 32;
	int n = 0;
	int s = 16;
	while (s) {
		uint32_t m = (1U << s) - 1;
		if ((x & m) == 0) {
			n += s;
			x >>= s;
		}
		s >>= 1;
	}
	return n;
}

void rng_seed(sxRNG* pState, uint64_t seed);
inline void rng_seed(uint64_t seed) { rng_seed(nullptr, seed); }
uint64_t rng_next(sxRNG* pState = nullptr);
float rng_f01(sxRNG* pState = nullptr);

template <typename PRIO_T, typename ITEM_T, const int N> class tStaticPrioList {
protected:
	int mCur;

	int find_slot(const PRIO_T prio) const {
		uint32_t cnt = mCur;
		const Entry* pTop = mEntries;
		const Entry* p = pTop;
		while (cnt > 1) {
			uint32_t imid = cnt / 2;
			const Entry* pMid = &p[imid];
			p = prio > pMid->prio ? p : pMid;
			cnt -= imid;
		}
		return (int)(p - pTop) + ((p != pTop) & (p->prio < prio));
	}

public:
	struct Entry {
		PRIO_T prio;
		ITEM_T item;
	};

	Entry mEntries[N];

	tStaticPrioList() {
		reset();
	}

	void reset() { mCur = 0; }

	void fill(const PRIO_T prio, const ITEM_T item) {
		for (int i = 0; i < N; ++i) {
			mEntries[i].prio = prio;
			mEntries[i].item = item;
		}
	}

	int add(const PRIO_T prio, const ITEM_T item) {
		if (mCur >= N) return -1;
		Entry ent;
		ent.prio = prio;
		ent.item = item;
		int idx = 0;
		if (mCur > 0) {
			idx = find_slot(prio);
			if (idx == mCur - 1) {
				if (prio > mEntries[idx].prio) {
					mEntries[mCur] = mEntries[mCur - 1];
					idx = mCur - 1;
				} else {
					idx = mCur;
				}
			} else {
				int j;
				if (prio > mEntries[idx].prio) {
					--idx;
				}
				for (j = mCur; --j >= idx + 1;) {
					mEntries[j + 1] = mEntries[j];
				}
				++idx;
			}
		}
		mEntries[idx] = ent;
		++mCur;
		return idx;
	}

	int get_size() const { return N; }
	int get_count() const { return mCur; }
};

} // nxCore


class cxHeap {
protected:
	struct Block {
		uint32_t blkTag;
		uint32_t ownerTag;
		size_t size;
		Block* pPrev;
		Block* pNext;
		Block* pPrevSub;
		Block* pNextSub;
	};

	size_t mSize;
	size_t mBlkUsed;
	size_t mBlkFree;
	uint32_t mBlkNum;
	uint32_t mAlign;
	Block* mpBlkTop;
	Block* mpBlkFree;
	Block* mpBlkLast;

	cxHeap() {}

	void init(Block* pBlk);

public:
	void* alloc(const size_t size, const uint32_t tag = XD_FOURCC('H', 'M', 'E', 'M'));
	void free(void* p);
	void purge();

	static cxHeap* create(const size_t size, const char* pName = "xHeap", const uint32_t align = 0x10);
	static void destroy(cxHeap* pHeap);
};


class cxBrigade;
struct sxJobContext;
struct sxJobQueue;

typedef void (*xt_job_func)(const sxJobContext*);

struct sxJob {
	xt_job_func mFunc;
	void* mpData;
	int32_t mId;
	union {
		uint8_t mState[4];
		int32_t mParam;
	};
};

struct sxJobContext {
	sxJob* mpJob;
	cxBrigade* mpBrigade;
	int mWrkId;
	int mJobsDone;
	int mJobOrg;
	int mJobEnd;
};

class cxBrigade {
public:
	enum class SchedulingMode {
		DYNAMIC = 0,
		STATIC = 1
	};

protected:
	cxBrigade() {}

	sxJobQueue* mpQue;
	sxWorker** mppWrk;
	sxJobContext* mpJobCtx;
	void* mpDoneHandles;
	int mWrkNum;
	int mActiveWrkNum;
	SchedulingMode mSchedMode;

public:
	sxJobQueue* get_queue() { return mpQue; }
	void exec(sxJobQueue* pQue);
	void wait();
	bool ck_worker_id(const int wrkId) const { return unsigned(wrkId) < unsigned(mWrkNum); }
	int get_workers_num() const { return mWrkNum; }
	void set_active_workers_num(const int num);
	int get_active_workers_num() const { return mActiveWrkNum; }
	void reset_active_workers();
	int get_jobs_done_count(const int wrkId) const;
	sxJobContext* get_job_context(const int wrkId);
	SchedulingMode get_scheduling_mode() const { return mSchedMode; }
	bool is_dynamic_scheduling() const { return mSchedMode == SchedulingMode::DYNAMIC; }
	bool is_static_scheduling() const { return mSchedMode == SchedulingMode::STATIC; }
	void set_dynamic_scheduling() { mSchedMode = SchedulingMode::DYNAMIC; }
	void set_static_scheduling() { mSchedMode = SchedulingMode::STATIC; }

	static cxBrigade* create(int wrkNum);
	static void destroy(cxBrigade* pBgd);
};

namespace nxTask {

sxJobQueue* queue_create(int slotsNum);
void queue_destroy(sxJobQueue* pQue);
void queue_add(sxJobQueue* pQue, sxJob* pJob);
void queue_count_adjust(sxJobQueue* pQue, int count);
void queue_purge(sxJobQueue* pQue);
int queue_get_max_job_num(sxJobQueue* pQue);
int queue_get_job_count(sxJobQueue* pQue);
void queue_exec(sxJobQueue* pQue, cxBrigade* pBgd);

} // nxTask

namespace nxCalc {

template<typename T> inline T min(T x, T y) { return (x < y ? x : y); }
template<typename T> inline T min(T x, T y, T z) { return min(x, min(y, z)); }
template<typename T> inline T min(T x, T y, T z, T w) { return min(x, min(y, z, w)); }
template<typename T> inline T max(T x, T y) { return (x > y ? x : y); }
template<typename T> inline T max(T x, T y, T z) { return max(x, max(y, z)); }
template<typename T> inline T max(T x, T y, T z, T w) { return max(x, max(y, z, w)); }

template<typename T> inline T clamp(T x, T lo, T hi) { return max(min(x, hi), lo); }
template<typename T> inline T saturate(T x) { return clamp(x, T(0), T(1)); }

template<typename T> inline T sq(T x) { return x*x; }
template<typename T> inline T cb(T x) { return x*x*x; }

template<typename T> inline T div0(T x, T y) { return y != T(0) ? x / y : T(0); }
template<typename T> inline T rcp0(T x) { return div0(T(1), x); }

template<typename T> inline T tfabs(T x) { return T(::mth_fabs((double)x)); }
inline float tfabs(float x) { return ::mth_fabsf(x); }

template<typename T> inline T tsqrt(T x) { return T(::mth_sqrt((double)x)); }
inline float tsqrt(float x) { return ::mth_sqrtf(x); }

template<typename T> inline T tsin(T x) { return T(::mth_sin((double)x)); }
inline float tsin(float x) { return ::mth_sinf(x); }

template<typename T> inline T tcos(T x) { return T(::mth_cos((double)x)); }
inline float tcos(float x) { return ::mth_cosf(x); }

template<typename T> inline T tpow(T x, T y) { return T(::mth_pow((double)x, (double)y)); }
inline float tpow(float x, float y) { return ::mth_powf(x, y); }

template<typename T> inline T ipow(const T x, const int n) {
	T wx = x;
	int wn = n;
	if (n < 0) {
		wx = rcp0(x);
		wn = -n;
	}
	T res = T(1);
	do {
		if ((wn & 1) != 0) res *= wx;
		wx *= wx;
		wn >>= 1;
	} while (wn > 0);
	return res;
}

inline float hypot(const float x, const float y) {
	float m = max(::mth_fabsf(x), ::mth_fabsf(y));
	float im = rcp0(m);
	return ::mth_sqrtf(sq(x*im) + sq(y*im)) * m;
}

inline double hypot(const double x, const double y) {
	double m = max(::mth_fabs(x), ::mth_fabs(y));
	double im = rcp0(m);
	return ::mth_sqrt(sq(x*im) + sq(y*im)) * m;
}

inline float cb_root(const float x) {
	float a = ::mth_fabsf(x);
	float r = ::mth_expf(::mth_logf(a) * (1.0f / 3.0f));
	return x < 0.0f ? -r : r;
}

inline float lerp(const float a, const float b, const float t) { return a + (b - a)*t; }

inline float sinc(const float x) {
	if (::mth_fabsf(x) < 1.0e-4f) {
		return 1.0f;
	}
	return (::mth_sinf(x) / x);
}

template<typename T> inline T approach(const T& val, const T& dst, const int t) {
	float ft = float(t < 1 ? 1 : t);
	return (val*(ft - 1.0f) + dst) * (1.0f / ft);
}

inline float ease(const float t, const float e = 1.0f) {
	float x = 0.5f - 0.5f*::mth_cosf(t*XD_PI);
	if (e != 1.0f) {
		x = ::mth_powf(x, e);
	}
	return x;
}

float ease_crv(const float p1, const float p2, const float t);

float hermite(const float p0, const float m0, const float p1, const float m1, const float t);

float fit(const float val, const float oldMin, const float oldMax, const float newMin, const float newMax);

float calc_fovy(const float focal, const float aperture, const float aspect);

template<typename FN> float panorama_scan(FN& fn, int w, int h) {
	float da = (float)((2.0*XD_PI / w) * (XD_PI / h));
	float iw = 1.0f / (float)w;
	float ih = 1.0f / (float)h;
	float sum = 0.0f;
	for (int y = 0; y < h; ++y) {
		float v = 1.0f - (y + 0.5f)*ih;
		float dw = da * ::mth_sinf(XD_PI * v);
		float ay = (v - 1.0f) * XD_PI;
		float sy = ::mth_sinf(ay);
		float cy = ::mth_cosf(ay);
		float ax0 = iw * XD_PI;
		float rsx = ::mth_sinf(ax0);
		float rcx = ::mth_cosf(ax0);
		float rax = 2.0f * sq(rsx);
		float rbx = ::mth_sinf(ax0 * 2.0f);
		for (int x = 0; x < w; ++x) {
			float sx = rsx;
			float cx = rcx;
			float dx = cx * sy;
			float dy = cy;
			float dz = sx * sy;
			float isx = rsx - (rax*rsx - rbx*rcx);
			float icx = rcx - (rax*rcx + rbx*rsx);
			rsx = isx;
			rcx = icx;
			fn(x, y, dx, dy, dz, dw);
			sum += dw;
		}
	}
	return (XD_PI * 4.0f) * rcp0(sum);
}

inline bool is_even(const int32_t x) { return (x & 1) == 0; }
inline bool is_odd(const int32_t x) { return (x & 1) != 0; }
bool is_prime(const int32_t x);
int32_t prime(const int32_t x);

double median(double* pData, const size_t num);

} // nxCalc

namespace nxLA {

template<typename T> inline T* mtx_alloc(int M, int N) {
	int a = nxCalc::min(int(sizeof(T) * 4), 0x10);
	return reinterpret_cast<T*>(nxCore::mem_alloc(M * N * sizeof(T), "xLA:mtx", a));
}

template<typename DST_T, typename SRC_T> inline void mtx_cpy(DST_T* pDst, const SRC_T* pSrc, int M, int N) {
	int n = M * N;
	for (int i = 0; i < n; ++i) {
		pDst[i] = DST_T(pSrc[i]);
	}
}

template<typename T> inline T* vec_alloc(int N) {
	int a = nxCalc::min(int(sizeof(T) * 4), 0x10);
	return reinterpret_cast<T*>(nxCore::mem_alloc(N * sizeof(T), "xLA:vec", a));
}

template<typename DST_T, typename SRC_T> inline void vec_cpy(DST_T* pDst, const SRC_T* pSrc, int N) {
	for (int i = 0; i < N; ++i) {
		pDst[i] = DST_T(pSrc[i]);
	}
}

template<typename DST_T, typename SRC_L_T, typename SRC_R_T>
XD_FORCEINLINE
void mul_mm(DST_T* pDst, const SRC_L_T* pSrc1, const SRC_R_T* pSrc2, const int M, const int N, const int P) {
	for (int i = 0; i < M; ++i) {
		int ra = i * N;
		int rr = i * P;
		DST_T s = DST_T(pSrc1[ra]);
		for (int k = 0; k < P; ++k) {
			pDst[rr + k] = DST_T(pSrc2[k]) * s;
		}
	}
	for (int i = 0; i < M; ++i) {
		int ra = i * N;
		int rr = i * P;
		for (int j = 1; j < N; ++j) {
			int rb = j * P;
			DST_T s = DST_T(pSrc1[ra + j]);
			for (int k = 0; k < P; ++k) {
				pDst[rr + k] += DST_T(pSrc2[rb + k]) * s;
			}
		}
	}
}

template<typename DST_VEC_T, typename SRC_VEC_T, typename MTX_T>
inline void mul_vm(DST_VEC_T* pDstVec, const SRC_VEC_T* pSrcVec, const MTX_T* pMtx, const int M, const int N) {
	mul_mm(pDstVec, pSrcVec, pMtx, 1, M, N);
}

template<typename DST_VEC_T, typename MTX_T, typename SRC_VEC_T>
inline void mul_mv(DST_VEC_T* pDstVec, const MTX_T* pMtx, const SRC_VEC_T* pSrcVec, const int M, const int N) {
#if 0
	mul_mm(pDstVec, pMtx, pSrcVec, M, N, 1);
#else
	for (int i = 0; i < M; ++i) {
		pDstVec[i] = DST_VEC_T(0);
	}
	for (int i = 0; i < M; ++i) {
		int r = i * N;
		for (int j = 0; j < N; ++j) {
			pDstVec[i] = DST_VEC_T(pDstVec[i] + pMtx[r + j]*pSrcVec[j]);
		}
	}
#endif
}

template<typename DST_T, typename SRC_L_T, typename SRC_R_T>
inline void mul_md(DST_T* pDst, const SRC_L_T* pSrc, const SRC_R_T* pDiag, const int M, const int N) {
	for (int i = 0; i < N; ++i) {
		int idx = i;
		SRC_R_T d = pDiag[i];
		for (int j = 0; j < M; ++j) {
			pDst[idx] = DST_T(pSrc[idx] * d);
			idx += N;
		}
	}
}

template<typename DST_T, typename SRC_L_T, typename SRC_R_T>
inline void mul_dm(DST_T* pDst, const SRC_L_T* pDiag, const SRC_R_T* pSrc, const int M, const int N) {
	for (int i = 0; i < M; ++i) {
		int idx = i*N;
		SRC_R_T d = pDiag[i];
		for (int j = 0; j < N; ++j) {
			pDst[idx + j] = DST_T(pSrc[idx + j] * d);
		}
	}
}

template<typename DST_T, typename SRC_L_T, typename SRC_R_T>
void add_mm(DST_T* pDst, const SRC_L_T* pSrc1, const SRC_R_T* pSrc2, const int M, const int N) {
	for (int i = 0; i < M * N; ++i) {
		pDst[i] = DST_T(pSrc1[i] + pSrc2[i]);
	}
}

template<typename DST_T, typename SRC_L_T, typename SRC_R_T>
void sub_mm(DST_T* pDst, const SRC_L_T* pSrc1, const SRC_R_T* pSrc2, const int M, const int N) {
	for (int i = 0; i < M * N; ++i) {
		pDst[i] = DST_T(pSrc1[i] - pSrc2[i]);
	}
}

template<typename T, typename ACC_T = T>
XD_FORCEINLINE
ACC_T dot_col_col(const T* pMtx, const int N, const int icol1, const int icol2, const int iorg, const int iend) {
	ACC_T s = ACC_T(0);
	const T* pOrg = pMtx + (iorg*N);
	const T* pCol1 = pOrg + icol1;
	const T* pCol2 = pOrg + icol2;
	for (int i = iorg; i <= iend; ++i) {
		ACC_T a = ACC_T(*pCol1);
		ACC_T b = ACC_T(*pCol2);
		s += a * b;
		pCol1 += N;
		pCol2 += N;
	}
	return s;
}

template<typename T>
XD_FORCEINLINE
void zero_col(T* pMtx, const int N, const int icol, const int iorg, const int iend) {
	T* p = pMtx + (iorg*N) + icol;
	for (int i = iorg; i <= iend; ++i) {
		*p = T(0);
		p += N;
	}
}

template<typename T>
XD_FORCEINLINE
void neg_col(T* pMtx, const int N, const int icol, const int iorg, const int iend) {
	T* p = pMtx + (iorg*N) + icol;
	for (int i = iorg; i <= iend; ++i) {
		T x = *p;
		*p = -x;
		p += N;
	}
}

template<typename T>
XD_FORCEINLINE
void scl_col(T* pMtx, const int N, const int icol, const int iorg, const int iend, const T s) {
	T* p = pMtx + (iorg*N) + icol;
	for (int i = iorg; i <= iend; ++i) {
		*p *= s;
		p += N;
	}
}

template<typename T>
XD_FORCEINLINE
void elim_col(T* pMtx, const int N, const int idst, const int isrc, const int iorg, const int iend, const T s) {
	T* pOrg = pMtx + (iorg*N);
	T* pDst = pOrg + idst;
	T* pSrc = pOrg + isrc;
	for (int i = iorg; i <= iend; ++i) {
		*pDst += *pSrc * s;
		pDst += N;
		pSrc += N;
	}
}

template<typename T>
XD_FORCEINLINE
void rot_col(T* pMtx, const int N, const int icol1, const int icol2, const int iorg, const int iend, const T s, const T c) {
	T* pOrg = pMtx + (iorg*N);
	T* pCol1 = pOrg + icol1;
	T* pCol2 = pOrg + icol2;
	for (int i = iorg; i <= iend; ++i) {
		T x = *pCol1;
		T y = *pCol2;
		*pCol1 = x*c + y*s;
		*pCol2 = y*c - x*s;
		pCol1 += N;
		pCol2 += N;
	}
}

template<typename T>
inline
void swap_cols(T* pMtx, const int N, const int icol1, const int icol2, const int iorg, const int iend) {
	T* pOrg = pMtx + (iorg*N);
	T* pCol1 = pOrg + icol1;
	T* pCol2 = pOrg + icol2;
	for (int i = iorg; i <= iend; ++i) {
		T t = *pCol1;
		*pCol1 = *pCol2;
		*pCol2 = t;
		pCol1 += N;
		pCol2 += N;
	}
}

template<typename T, typename ACC_T = T>
XD_FORCEINLINE
ACC_T dot_row_row(const T* pMtx, const int N, const int irow1, const int irow2, const int iorg, const int iend) {
	ACC_T s = ACC_T(0);
	const T* pRow1 = pMtx + (irow1*N) + iorg;
	const T* pRow2 = pMtx + (irow2*N) + iorg;
	for (int i = iorg; i <= iend; ++i) {
		ACC_T a = ACC_T(*pRow1);
		ACC_T b = ACC_T(*pRow2);
		s += a * b;
		++pRow1;
		++pRow2;
	}
	return s;
}

template<typename T>
XD_FORCEINLINE
void zero_row(T* pMtx, const int N, const int irow, const int iorg, const int iend) {
	T* p = pMtx + (irow*N) + iorg;
	for (int i = iorg; i <= iend; ++i) {
		*p++ = T(0);
	}
}

template<typename T>
XD_FORCEINLINE
void neg_row(T* pMtx, const int N, const int irow, const int iorg, const int iend) {
	T* p = pMtx + (irow*N) + iorg;
	for (int i = iorg; i <= iend; ++i) {
		T x = *p;
		*p = -x;
		++p;
	}
}

template<typename T>
XD_FORCEINLINE
void scl_row(T* pMtx, const int N, const int irow, const int iorg, const int iend, const T s) {
	T* p = pMtx + (irow*N) + iorg;
	for (int i = iorg; i <= iend; ++i) {
		*p++ *= s;
	}
}

template<typename T>
XD_FORCEINLINE
void elim_row(T* pMtx, const int N, const int idst, const int isrc, const int iorg, const int iend, const T s) {
	T* pDst = pMtx + (idst*N) + iorg;
	T* pSrc = pMtx + (isrc*N) + iorg;
	for (int i = iorg; i <= iend; ++i) {
		*pDst += *pSrc * s;
		++pDst;
		++pSrc;
	}
}

template<typename T, typename ACC_T = T>
XD_FORCEINLINE
ACC_T dot_row_col(const T* pRowMtx, const T* pColMtx, const int N, const int irow, const int icol, const int iorg, const int iend) {
	ACC_T s = ACC_T(0);
	const T* pRow = pRowMtx + (irow*N) + iorg;
	const T* pCol = pColMtx + (iorg*N) + icol;
	for (int i = iorg; i <= iend; ++i) {
		ACC_T a = ACC_T(*pRow);
		ACC_T b = ACC_T(*pCol);
		s += a * b;
		++pRow;
		pCol += N;
	}
	return s;
}

template<typename T> inline void identity(T* pMtx, const int N) {
	for (int i = 0; i < N * N; ++i) {
		pMtx[i] = T(0);
	}
	for (int i = 0; i < N; ++i) {
		pMtx[(i * N) + i] = T(1);
	}
}

template<typename T>
inline
bool transpose(T* pMtx, const int M, const int N, uint32_t* pFlgWk = nullptr) {
	if (M == N) {
		for (int i = 0; i < N - 1; ++i) {
			for (int j = i + 1; j < N; ++j) {
				int ij = i*N + j;
				int ji = j*N + i;
				T t = pMtx[ij];
				pMtx[ij] = pMtx[ji];
				pMtx[ji] = t;
			}
		}
	} else {
		XD_BIT_ARY_DECL(uint32_t, flg, 64*32);
		uint32_t* pFlg = flg;
		size_t flgSize = XD_BIT_ARY_SIZE(uint32_t, M*N);
		size_t maxSize = XD_ARY_LEN(flg);
		if (flgSize > maxSize) {
			pFlg = pFlgWk;
		}
		if (!pFlg) {
			nxSys::dbgmsg("LA::transpose - insufficient memory.\n");
			return false;
		}
		for (size_t i = 0; i < flgSize; ++i) {
			pFlg[i] = 0;
		}
		int q = M*N - 1;
		for (int i = 1; i < q;) {
			int i0 = i;
			int org = i;
			T t = pMtx[org];
			do {
				int i1 = (i*M) % q;
				T t1 = pMtx[i1];
				pMtx[i1] = t;
				t = t1;
				XD_BIT_ARY_ST(uint32_t, pFlg, i);
				i = i1;
			} while (i != org);
			for (i = i0 + 1; i < q && XD_BIT_ARY_CK(uint32_t, pFlg, i); ++i) {}
		}
	}
	return true;
}

template<typename T>
XD_FORCEINLINE
bool inv_gj(T* pMtx, const int N, int* pWk /* [N*3] */) {
	int* pPiv = pWk;
	int* pCol = pWk + N;
	int* pRow = pWk + N*2;
	int ir = 0;
	int ic = 0;
	for (int i = 0; i < N; ++i) {
		pPiv[i] = 0;
	}
	for (int i = 0; i < N; ++i) {
		T amax = 0;
		for (int j = 0; j < N; ++j) {
			if (pPiv[j] != 1) {
				int rj = j * N;
				for (int k = 0; k < N; ++k) {
					if (0 == pPiv[k]) {
						T a = pMtx[rj + k];
						if (a < 0) a = -a;
						if (a >= amax) {
							amax = a;
							ir = j;
							ic = k;
						}
					}
				}
			}
		}
		++pPiv[ic];
		if (ir != ic) {
			int rr = ir * N;
			int rc = ic * N;
			for (int j = 0; j < N; ++j) {
				T t = pMtx[rr + j];
				pMtx[rr + j] = pMtx[rc + j];
				pMtx[rc + j] = t;
			}
		}
		pRow[i] = ir;
		pCol[i] = ic;
		int rc = ic * N;
		T piv = pMtx[rc + ic];
		if (piv == 0) return false; /* singular */
		T ipiv = 1 / piv;
		pMtx[rc + ic] = 1;
		for (int j = 0; j < N; ++j) {
			pMtx[rc + j] *= ipiv;
		}
		for (int j = 0; j < N; ++j) {
			if (j != ic) {
				int rj = j * N;
				T d = pMtx[rj + ic];
				pMtx[rj + ic] = 0;
				T* pDst = &pMtx[rj];
				T* pSrc = &pMtx[rc];
				for (int k = 0; k < N; ++k) {
					*pDst++ -= *pSrc++ * d;
				}
			}
		}
	}
	for (int i = N; --i >= 0;) {
		ir = pRow[i];
		ic = pCol[i];
		if (ir != ic) {
			for (int j = 0; j < N; ++j) {
				int rj = j * N;
				T t = pMtx[rj + ir];
				pMtx[rj + ir] = pMtx[rj + ic];
				pMtx[rj + ic] = t;
			}
		}
	}
	return true;
}

template<typename T>
XD_FORCEINLINE
bool lu_decomp(T* pMtx, const int N, T* pWk /* [N] */, int* pIdx /* [N] */, int* pDetSgn = nullptr) {
	int dsgn = 1;
	T* pScl = pWk;
	for (int i = 0; i < N; ++i) {
		T scl = 0;
		int ri = i * N;
		for (int j = 0; j < N; ++j) {
			T a = pMtx[ri + j];
			if (a < 0) a = -a;
			if (a > scl) scl = a;
		}
		if (scl == 0) {
			if (pDetSgn) {
				*pDetSgn = 0;
			}
			return false;
		}
		pScl[i] = scl;
	}
	for (int i = 0; i < N; ++i) {
		pScl[i] = 1 / pScl[i];
	}
	int imax = 0;
	for (int k = 0; k < N; ++k) {
		int rk = k * N;
		T amax = 0;
		for (int i = k; i < N; ++i) {
			T a = pMtx[(i * N) + k];
			if (a < 0) a = -a;
			a *= pScl[i];
			if (amax <= a) {
				amax = a;
				imax = i;
			}
		}
		if (k != imax) {
			int rm = imax * N;
			for (int j = 0; j < N; ++j) {
				T t = pMtx[rm + j];
				pMtx[rm + j] = pMtx[rk + j];
				pMtx[rk + j] = t;
			}
			dsgn = -dsgn;
			pScl[imax] = pScl[k];
		}
		if (pIdx) {
			pIdx[k] = imax;
		}
		if (pMtx[rk + k] == 0) {
			pMtx[rk + k] = T(1.0e-16);
		}
		for (int i = k + 1; i < N; ++i) {
			int ri = i * N;
			T s = pMtx[ri + k] / pMtx[rk + k];
			pMtx[ri + k] = s;
			T* pDst = &pMtx[ri + k + 1];
			T* pSrc = &pMtx[rk + k + 1];
			for (int j = k + 1; j < N; ++j) {
				*pDst++ -= *pSrc++ * s;
			}
		}
	}
	if (pDetSgn) {
		*pDetSgn = dsgn;
	}
	return true;
}

template<typename T>
XD_FORCEINLINE
void lu_solve(T* pAns, const T* pLU, const T* pRHS, const int* pIdx, const int N) {
	if (pAns != pRHS) {
		for (int i = 0; i < N; ++i) {
			pAns[i] = pRHS[i];
		}
	}
	T s = 0;
	int ii = -1;
	for (int i = 0; i < N; ++i) {
		int ri = i * N;
		int idx = pIdx[i];
		s = pAns[idx];
		pAns[idx] = pAns[i];
		if (ii < 0) {
			if (s != 0) {
				ii = i;
			}
		} else {
			for (int j = ii; j < i; ++j) {
				s -= pLU[ri + j] * pAns[j];
			}
		}
		pAns[i] = s;
	}
	for (int i = N; --i >= 0;) {
		int ri = i * N;
		s = pAns[i];
		for (int j = i + 1; j < N; ++j) {
			s -= pLU[ri + j] * pAns[j];
		}
		pAns[i] = s / pLU[ri + i];
	}
}

template<typename T>
XD_FORCEINLINE
T lu_det(const T* pLU, const int N, const int dsgn) {
	T det = T(1);
	for (int i = 0; i < N; ++i) {
		det *= pLU[i*N + i];
	}
	if (dsgn < 0) { det = -det; }
	return det;
}

template<typename T>
inline
bool symm_ldlt_decomp(T* pMtx, const int N, T* pDet /* [N] */, int* pIdx /* [N] */) {
	const T zthr = T(1.0e-16);
	const T aprm = T((1.0 + ::mth_sqrt(17.0)) / 8.0);
	int eposi = 0;
	int enega = 0;
	pIdx[N - 1] = N - 1;
	int i = 0;
	while (i < N - 1) {
		int ri = i * N;
		int i1 = i + 1;
		int i2 = i + 2;
		T aii = pMtx[ri + i];
		if (aii < 0) aii = -aii;
		pIdx[i] = i;
		T lambda = pMtx[ri + i1];
		if (lambda < 0) lambda = -lambda;
		int j = i1;
		for (int k = i2; k < N; ++k) {
			T aik = pMtx[ri + k];
			if (aik < 0) aik = -aik;
			if (aik > lambda) {
				j = k;
				lambda = aik;
			}
		}
		int rj = j * N;
		bool flg = true;
		if (aii < aprm*lambda) {
			T sigma = lambda;
			for (int k = i1; k < j; ++k) {
				T akj = pMtx[k*N + j];
				if (akj < 0) akj = -akj;
				if (akj > sigma) sigma = akj;
			}
			for (int k = j + 1; k < N; ++k) {
				T ajk = pMtx[rj + k];
				if (ajk < 0) ajk = -ajk;
				if (ajk > sigma) sigma = ajk;
			}
			if (aii*sigma < lambda) {
				T ajj = pMtx[rj + j];
				if (ajj < 0) ajj = -ajj;
				if (ajj > aprm*sigma) {
					for (int k = j + 1; k < N; ++k) {
						T t = pMtx[rj + k];
						pMtx[rj + k] = pMtx[ri + k];
						pMtx[ri + k] = t;
					}
					for (int k = i1; k < j; ++k) {
						int rk = k*N;
						T t = pMtx[ri + k];
						pMtx[ri + k] = pMtx[rk + j];
						pMtx[rk + j] = t;
					}
					T t = pMtx[ri + i];
					pMtx[ri + i] = pMtx[rj + j];
					pMtx[rj + j] = t;
					pIdx[i] = j;
				} else {
					int ri1 = i1*N;
					if (j > i1) {
						for (int k = j + 1; k < N; ++k) {
							T t = pMtx[rj + k];
							pMtx[rj + k] = pMtx[ri1 + k];
							pMtx[ri1 + k] = t;
						}
						for (int k = i2; k < j; ++k) {
							int rk = k*N;
							T t = pMtx[ri1 + k];
							pMtx[ri1 + k] = pMtx[rk + j];
							pMtx[rk + j] = t;
						}
						T t = pMtx[ri + i];
						pMtx[ri + i] = pMtx[rj + j];
						pMtx[rj + j] = t;
						t = pMtx[ri + j];
						pMtx[ri + j] = pMtx[ri + i1];
						pMtx[ri + i1] = t;
					}
					T t = pMtx[ri + i1];
					T det = pMtx[ri + i]*pMtx[ri1 + i1] - t*t;
					T idet = T(1) / det;
					aii = pMtx[ri + i] * idet;
					T aii1 = pMtx[ri + i1] * idet;
					T ai1i1 = pMtx[ri1 + i1] * idet;
					pIdx[i] = j;
					pIdx[i1] = -1;
					pDet[i] = 1;
					pDet[i1] = det;
					for (int k = i2; k < N; ++k) {
						T u = aii1*pMtx[ri1 + k] - ai1i1*pMtx[ri + k];
						T v = aii1*pMtx[ri + k] - aii*pMtx[ri1 + k];
						int rk = k*N;
						T* pDst = &pMtx[rk + k];
						T* pSrc = &pMtx[ri + k];
						for (int m = k; m < N; ++m) {
							*pDst++ += *pSrc++ * u;
						}
						pDst = &pMtx[rk + k];
						pSrc = &pMtx[ri1 + k];
						for (int m = k; m < N; ++m) {
							*pDst++ += *pSrc++ * v;
						}
						pMtx[ri + k] = u;
						pMtx[ri1 + k] = v;
					}
					i = i2;
					++eposi;
					++enega;
					flg = false;
				}
			}
		}
		if (flg) {
			aii = pMtx[ri + i];
			if (aii < 0) aii = -aii;
			if (aii > zthr) {
				aii = pMtx[ri + i];
				if (aii > 0) {
					++eposi;
				} else {
					++enega;
				}
				pDet[i] = aii;
			}
			T s = -T(1) / aii;
			for (int k = i1; k < N; ++k) {
				int rk = k*N;
				T t = pMtx[ri + k] * s;
				T* pDst = &pMtx[rk + k];
				T* pSrc = &pMtx[ri + k];
				for (int m = k; m < N; ++m) {
					*pDst++ += *pSrc++ * t;
				}
				pMtx[ri + k] = t;
			}
			i = i1;
		}
	}
	if (i == N - 1) {
		int ri = i*N;
		T aii = pMtx[ri + i];
		if (aii < 0) aii = -aii;
		if (aii > zthr) {
			aii = pMtx[ri + i];
			if (aii > 0) {
				++eposi;
			} else {
				++enega;
			}
		}
		pDet[i] = pMtx[i*N + i];
	}
	return (eposi + enega) == N;
}

template<typename T>
inline
void symm_ldlt_solve(T* pAns, const T* pMtx, const T* pRHS, const T* pDet /* [N] */, const int* pIdx /* [N] */, const int N) {
	if (pAns != pRHS) {
		for (int i = 0; i < N; ++i) {
			pAns[i] = pRHS[i];
		}
	}
	int i = 0;
	while (i < N - 1) {
		int ri = i*N;
		int idx = pIdx[i];
		T tp = pAns[idx];
		if (pIdx[i + 1] >= 0) {
			pAns[idx] = pAns[i];
			pAns[i] = tp / pMtx[ri + i];
			for (int j = i + 1; j < N; ++j) {
				pAns[j] += pMtx[ri + j] * tp;
			}
			i = i + 1;
		} else {
			int i1 = i + 1;
			int ri1 = i1*N;
			T t = pAns[i];
			pAns[idx] = pAns[i1];
			T idet = T(1) / pDet[i1];
			pAns[i] = (pMtx[ri1 + i1]*t - pMtx[ri + i1]*tp) * idet;
			pAns[i1] = (pMtx[ri + i]*tp - pMtx[ri + i1]*t) * idet;
			for (int j = i + 2; j < N; ++j) {
				pAns[j] += pMtx[ri + j] * t;
			}
			for (int j = i + 2; j < N; ++j) {
				pAns[j] += pMtx[ri1 + j] * tp;
			}
			i = i + 2;
		}
	}
	if (i == N - 1) {
		pAns[i] /= pMtx[i*N + i];
		i = N - 2;
	} else {
		i = N - 3;
	}
	while (i >= 0) {
		int ii = pIdx[i] >= 0 ? i : i - 1;
		for (int j = ii; j <= i; ++j) {
			int rj = j*N;
			T s = pAns[j];
			for (int k = i + 1; k < N; ++k) {
				s += pMtx[rj + k] * pAns[k];
			}
			pAns[j] = s;
		}
		int idx = pIdx[ii];
		T t = pAns[i];
		pAns[i] = pAns[idx];
		pAns[idx] = t;
		i = ii - 1;
	}
}

template<typename T>
inline
bool sv_decomp(T* pU, T* pS, T* pV, T* pWk, const T* pA, const int M, const int N, const bool uflg = true, const bool sort = true, const int niter = 200) {
	if (pU != pA) {
		mtx_cpy(pU, pA, M, N);
	}

	T g = T(0);
	T scl = T(0);
	T nrm = T(0);
	T s, c, f, h;
	T* pCell;
	for (int i = 0; i < N; ++i) {
		int iorg = i;
		int iend = M - 1;
		pWk[i] = g * scl;
		g = T(0);
		s = T(0);
		scl = T(0);
		if (i < M) {
			pCell = pU + (iorg * N) + i;
			for (int j = iorg; j <= iend; ++j) {
				scl += nxCalc::tfabs(*pCell);
				pCell += N;
			}
		}
		if (scl != T(0)) {
			scl_col(pU, N, i, iorg, iend, T(1) / scl);
			s = dot_col_col(pU, N, i, i, iorg, iend);
			f = pU[i*N + i];
			g = nxCalc::tsqrt(s);
			if (f > T(0)) g = -g;
			h = f*g - s;
			h = T(1) / h;
			pU[i*N + i] -= g;
			for (int j = i + 1; j < N; ++j) {
				s = dot_col_col(pU, N, i, j, iorg, iend);
				elim_col(pU, N, j, i, iorg, iend, s * h);
			}
			scl_col(pU, N, i, iorg, iend, scl);
		}
		pS[i] = g * scl;
		g = T(0);
		s = T(0);
		iorg = i + 1;
		iend = N - 1;
		scl = T(0);
		if (i < M && i != N - 1) {
			pCell = pU + (i*N) + iorg;
			for (int j = iorg; j <= iend; ++j) {
				scl += nxCalc::tfabs(*pCell);
				++pCell;
			}
		}
		if (scl != T(0)) {
			scl_row(pU, N, i, iorg, iend, T(1) / scl);
			s = dot_row_row(pU, N, i, i, iorg, iend);
			f = pU[i*N + iorg];
			g = nxCalc::tsqrt(s);
			if (f > T(0)) g = -g;
			h = f*g - s;
			pU[i*N + iorg] -= g;
			s = T(1) / h;
			pCell = pU + (i*N);
			for (int j = iorg; j <= iend; ++j) {
				pWk[j] = pCell[j] * s;
			}
			for (int j = i + 1; j < M; ++j) {
				s = dot_row_row(pU, N, i, j, iorg, iend);
				pCell = pU + (j*N) + iorg;
				for (int k = iorg; k <= iend; ++k) {
					*pCell += pWk[k] * s;
					++pCell;
				}
			}
			scl_row(pU, N, i, iorg, iend, scl);
		}
		s = nxCalc::tfabs(pS[i]) + nxCalc::tfabs(pWk[i]);
		nrm = nxCalc::max(nrm, s);
	}

	if (pV) {
		for (int i = N; --i >= 0;) {
			if (i < N - 1) {
				if (g) {
					for (int j = i + 1; j < N; ++j) {
						pV[j*N + i] = pU[i*N + j] / pU[i*N + i + 1];
					}
					scl_col(pV, N, i, i + 1, N - 1, T(1) / g);
					for (int j = i + 1; j < N; ++j) {
						s = dot_row_col(pU, pV, N, i, j, i + 1, N - 1);
						elim_col(pV, N, j, i, i + 1, N - 1, s);
					}
				}
				zero_row(pV, N, i, i + 1, N - 1);
				zero_col(pV, N, i, i + 1, N - 1);
			}
			pV[i*N + i] = T(1);
			g = pWk[i];
		}
	}

	if (uflg) {
		for (int i = nxCalc::min(M, N); --i >= 0;) {
			if (i < N - 1) {
				zero_row(pU, N, i, i + 1, N - 1);
			}
			g = pS[i];
			if (g) {
				g = T(1) / g;
				for (int j = i + 1; j < N; ++j) {
					s = dot_col_col(pU, N, i, j, i + 1, M - 1);
					s /= pU[i*N + i];
					elim_col(pU, N, j, i, i, M - 1, s * g);
				}
				scl_col(pU, N, i, i, M - 1, g);
			} else {
				zero_col(pU, N, i, i, M - 1);
			}
			pU[i*N + i] += T(1);
		}
	}

	for (int k = N; --k >= 0;) {
		for (int itr = 0; itr < niter; ++itr) {
			int idx = 0;
			bool flg = true;
			for (int i = k; i >= 0; --i) {
				idx = i;
				if (i == 0) {
					flg = false;
					break;
				}
				if (nxCalc::tfabs(pWk[i]) + nrm == nrm) {
					flg = false;
					break;
				}
				if (nxCalc::tfabs(pS[i - 1]) + nrm == nrm) {
					break;
				}
			}
			if (flg) {
				s = T(1);
				c = T(0);
				for (int i = idx; i <= k; ++i) {
					f = pWk[i] * s;
					pWk[i] *= c;
					if (nxCalc::tfabs(f) + nrm == nrm) {
						break;
					}
					g = pS[i];
					h = nxCalc::hypot(f, g);
					pS[i] = h;
					h = T(1) / h;
					c = g * h;
					s = -f * h;
					if (uflg) {
						rot_col(pU, N, idx - 1, i, 0, M - 1, s, c);
					}
				}
			}
			T z = pS[k];
			if (idx == k) {
				if (z < T(0)) {
					pS[k] = -z;
					if (pV) {
						neg_col(pV, N, k, 0, N - 1);
					}
				}
				break;
			}
			if (itr == niter - 1) {
				return false;
			}
			g = pWk[k - 1];
			h = pWk[k];
			f = (g - h) * (g + h);
			T x = pS[idx];
			T y = pS[k - 1];
			f += (y - z) * (y + z);
			f /= T(2) * h * y;
			g = nxCalc::hypot(f, T(1));
			f += f >= 0 ? g : -g;
			f = y / f;
			f -= h;
			f *= h;
			f += (x - z) * (x + z);
			f /= x;

			s = T(1);
			c = T(1);
			for (int i = idx; i < k; ++i) {
				g = pWk[i + 1];
				y = pS[i + 1];
				h = g * s;
				g *= c;
				z = nxCalc::hypot(f, h);
				pWk[i] = z;
				T iz = T(1) / z;
				c = f * iz;
				s = h * iz;
				f = x*c + g*s;
				g = g*c - x*s;
				h = y * s;
				y *= c;
				if (pV) {
					rot_col(pV, N, i, i + 1, 0, N - 1, s, c);
				}
				z = nxCalc::hypot(f, h);
				pS[i] = z;
				if (z) {
					iz = T(1) / z;
					c = f * iz;
					s = h * iz;
				}
				f = g*c + y*s;
				x = y*c - g*s;
				if (uflg) {
					rot_col(pU, N, i, i + 1, 0, M - 1, s, c);
				}
			}
			pWk[idx] = T(0);
			pWk[k] = f;
			pS[k] = x;
		}
	}

	if (uflg && pV) {
		for (int i = 0; i < N; ++i) {
			int nc = 0;
			pCell = pU + i;
			for (int j = 0; j < M; ++j) {
				nc += *pCell < T(0) ? 1 : 0;
				pCell += N;
			}
			pCell = pV + i;
			for (int j = 0; j < N; ++j) {
				nc += *pCell < T(0) ? 1 : 0;
				pCell += N;
			}
			if (nc > (M + N) / 2) {
				neg_col(pU, N, i, 0, M - 1);
				neg_col(pV, N, i, 0, N - 1);
			}
		}
	}

	if (sort) {
		int left = 0;
		int right = N - 1;
		int start = left;
		do {
			if (left < right) {
				for (int i = left; i < right; ++i) {
					if (pS[i] < pS[i + 1]) {
						start = i;
						T t = pS[i];
						pS[i] = pS[i + 1];
						pS[i + 1] = t;
						if (uflg) {
							swap_cols(pU, N, i, i + 1, 0, M - 1);
						}
						if (pV) {
							swap_cols(pV, N, i, i + 1, 0, N - 1);
						}
					}
				}
			}
			right = start;
			if (left < start) {
				for (int i = start; left < i; --i) {
					if (pS[i - 1] < pS[i]) {
						T t = pS[i];
						pS[i] = pS[i - 1];
						pS[i - 1] = t;
						if (uflg) {
							swap_cols(pU, N, i, i - 1, 0, M - 1);
						}
						if (pV) {
							swap_cols(pV, N, i, i - 1, 0, N - 1);
						}
					}
				}
			}
			left = start;
		} while (start < right);
	}

	return true;
}

} // nxLA

typedef int32_t xt_int;
typedef float xt_float;

struct xt_float2 {
	float x, y;

	void set(const float xval, const float yval) { x = xval; y = yval; }
	void fill(const float val) { x = val; y = val; }
	void scl(const float s) { x *= s; y *= s; }

	operator float* () { return reinterpret_cast<float*>(this); }
	operator const float* () const { return reinterpret_cast<const float*>(this); }
};

struct xt_float3 {
	float x, y, z;

	void set(const float xval, const float yval, const float zval) { x = xval; y = yval; z = zval; }
	void fill(const float val) { x = val; y = val; z = val; }
	void scl(const float s) { x *= s; y *= s; z *= s; }

	float get_at(const int i) const {
		float res = nxCore::f32_mk_nan();
		switch (i) {
			case 0: res = x; break;
			case 1: res = y; break;
			case 2: res = z; break;
		}
		return res;
	}

	void set_at(const int i, const float val) {
		switch (i) {
			case 0: x = val; break;
			case 1: y = val; break;
			case 2: z = val; break;
		}
	}

	operator float* () { return reinterpret_cast<float*>(this); }
	operator const float* () const { return reinterpret_cast<const float*>(this); }
};

struct xt_float4 {
	float x, y, z, w;

	void set(const float xval, const float yval, const float zval, const float wval) { x = xval; y = yval; z = zval; w = wval; }

	void fill(const float val) {
		for (int i = 0; i < 4; ++i) {
			(*this)[i] = val;
		}
	}

	void scl(const float s) {
		for (int i = 0; i < 4; ++i) {
			(*this)[i] *= s;
		}
	}

	float get_at(const int i) const {
		float res = nxCore::f32_mk_nan();
		switch (i) {
			case 0: res = x; break;
			case 1: res = y; break;
			case 2: res = z; break;
			case 3: res = w; break;
		}
		return res;
	}

	void set_at(int at, float val) {
		switch (at) {
			case 0: x = val; break;
			case 1: y = val; break;
			case 2: z = val; break;
			case 3: w = val; break;
			default: break;
		}
	}

	operator float* () { return reinterpret_cast<float*>(this); }
	operator const float* () const { return reinterpret_cast<const float*>(this); }
};

struct xt_mtx {
	float m[4][4];

	void identity();
	void zero();

	operator float* () { return reinterpret_cast<float*>(this); }
	operator const float* () const { return reinterpret_cast<const float*>(this); }
};

struct xt_xmtx {
	float m[3][4];

	void identity();
	void zero();

	operator float* () { return reinterpret_cast<float*>(this); }
	operator const float* () const { return reinterpret_cast<const float*>(this); }
};

struct xt_int2 {
	int32_t x, y;

	void set(const int32_t xval, const int32_t yval) { x = xval; y = yval; }
	void fill(const int32_t val) { x = val; y = val; }

	operator int32_t* () { return reinterpret_cast<int32_t*>(this); }
	operator const int32_t* () const { return reinterpret_cast<const int32_t*>(this); }
};

struct xt_int3 {
	int32_t x, y, z;

	void set(const int32_t xval, const int32_t yval, const int32_t zval) { x = xval; y = yval; z = zval; }
	void fill(const int32_t val) { x = val; y = val; z = val; }

	void set_at(const int at, const int32_t val) {
		switch (at) {
			case 0: x = val; break;
			case 1: y = val; break;
			case 2: z = val; break;
			default: break;
		}
	}

	operator int32_t* () { return reinterpret_cast<int32_t*>(this); }
	operator const int32_t* () const { return reinterpret_cast<const int32_t*>(this); }
};

struct xt_int4 {
	int32_t x, y, z, w;

	void set(const int32_t xval, const int32_t yval, const int32_t zval, const int32_t wval) { x = xval; y = yval; z = zval; w = wval; }
	void fill(const int32_t val) { x = val; y = val; z = val; w = val; }

	void scl(const int32_t s) {
		for (int i = 0; i < 4; ++i) {
			(*this)[i] *= s;
		}
	}

	void set_at(const int at, const int32_t val) {
		switch (at) {
			case 0: x = val; break;
			case 1: y = val; break;
			case 2: z = val; break;
			case 3: w = val; break;
			default: break;
		}
	}

	operator int32_t* () { return reinterpret_cast<int32_t*>(this); }
	operator const int32_t* () const { return reinterpret_cast<const int32_t*>(this); }
};

struct xt_half {
	uint16_t x;

	void set(const float f);
	float get() const;
};

struct xt_half2 {
	uint16_t x, y;

	void set(const float fx, const float fy);
	void set(const xt_float2& fv) { set(fv.x, fv.y); }
	xt_float2 get() const;
};

struct xt_half3 {
	uint16_t x, y, z;

	void set(float fx, float fy, float fz);
	void set(const xt_float3& fv) { set(fv.x, fv.y, fv.z); }
	xt_float3 get() const;
};

struct xt_half4 {
	uint16_t x, y, z, w;

	void set(const float fx, const float fy, const float fz, const float fw);
	void set(const xt_float4& fv) { set(fv.x, fv.y, fv.z, fv.w); }
	xt_float4 get() const;
};

struct xt_texcoord {
	float u, v;

	void set(const float u, const float v) { this->u = u; this->v = v; }
	void set(const xt_float2& uv) { set(uv.x, uv.y); }
	void scl(const float s) { u *= s; v *= s; }
	void fill(const float val) { u = val; v = val; }
	void flip_v() { v = 1.0f - v; }
	void negate_v() { v = -v; }
	void scroll(const xt_texcoord& step);
	void encode_half(uint16_t* pDst) const;

	operator float* () { return reinterpret_cast<float*>(this); }
	operator const float* () const { return reinterpret_cast<const float*>(this); }
};

struct xt_rgba {
	union {
		struct { float r, g, b, a; };
		float ch[4];
	};
};

union uxVal32 {
	int32_t i;
	uint32_t u;
	float f;
	uint8_t b[4];
};

union uxVal64 {
	int64_t i;
	uint64_t u;
	double f;
	uint8_t b[8];
};


enum class exAnimChan : int8_t {
	UNKNOWN = -1,
	TX = 0,
	TY = 1,
	TZ = 2,
	RX = 3,
	RY = 4,
	RZ = 5,
	SX = 6,
	SY = 7,
	SZ = 8
};

enum class exTransformOrd : uint8_t {
	SRT = 0,
	STR = 1,
	RST = 2,
	RTS = 3,
	TSR = 4,
	TRS = 5
};

enum class exRotOrd : uint8_t {
	XYZ = 0,
	XZY = 1,
	YXZ = 2,
	YZX = 3,
	ZXY = 4,
	ZYX = 5
};

enum class exAxis : uint8_t {
	PLUS_X = 0,
	MINUS_X = 1,
	PLUS_Y = 2,
	MINUS_Y = 3,
	PLUS_Z = 4,
	MINUS_Z = 5
};


class cxVec;
class cxMtx;
class cxQuat;

class cxVec : public xt_float3 {
public:
	cxVec() {}
	cxVec(float x, float y, float z) { set(x, y, z); }
	cxVec(float s) { fill(s); }

	cxVec(const cxVec& v) = default;
	cxVec& operator=(const cxVec& v) = default;

	void zero() { fill(0.0f); }

	void parse(const char* pStr);
	void from_mem(const float* pSrc) { set(pSrc[0], pSrc[1], pSrc[2]); }
	void to_mem(float* pDst) const {
		pDst[0] = x;
		pDst[1] = y;
		pDst[2] = z;
	}

	float operator [](int i) const { return get_at(i); }

	void add(const cxVec& v) {
		x += v.x;
		y += v.y;
		z += v.z;
	}
	void sub(const cxVec& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
	}
	void mul(const cxVec& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
	}
	void div(const cxVec& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
	}
	void scl(float s) {
		x *= s;
		y *= s;
		z *= s;
	}
	void neg() {
		x = -x;
		y = -y;
		z = -z;
	}
	void abs() {
		x = ::mth_fabsf(x);
		y = ::mth_fabsf(y);
		z = ::mth_fabsf(z);
	}

	cxVec neg_val() const {
		cxVec v = *this;
		v.neg();
		return v;
	}

	cxVec abs_val() const {
		cxVec v = *this;
		v.abs();
		return v;
	}

	cxVec& operator += (const cxVec& v) { add(v); return *this; }
	cxVec& operator -= (const cxVec& v) { sub(v); return *this; }
	cxVec& operator *= (const cxVec& v) { mul(v); return *this; }
	cxVec& operator /= (const cxVec& v) { div(v); return *this; }

	cxVec& operator *= (float s) { scl(s); return *this; }
	cxVec& operator /= (float s) { scl(1.0f / s); return *this; }

	void div0(const cxVec& v) {
		x = nxCalc::div0(x, v.x);
		y = nxCalc::div0(y, v.y);
		z = nxCalc::div0(z, v.z);
	}

	void div0(float s) { scl(s != 0.0f ? 1.0f / s : 0.0f); }

	cxVec inv_val() const {
		cxVec v(1.0f);
		v.div0(*this);
		return v;
	}
	void inv() { *this = inv_val(); }

	float min_elem(bool absFlg = false) const {
		cxVec v = *this;
		if (absFlg) {
			v.abs();
		}
		return nxCalc::min(v.x, v.y, v.z);
	}

	float min_abs_elem() const { return min_elem(true); }

	float max_elem(bool absFlg = false) const {
		cxVec v = *this;
		if (absFlg) {
			v.abs();
		}
		return nxCalc::max(v.x, v.y, v.z);
	}

	float max_abs_elem() const { return max_elem(true); }

	float mag2() const { return dot(*this); }
	float mag_fast() const { return ::mth_sqrtf(mag2()); }
	float mag() const;
	float length() const { return mag(); }

	void normalize(const cxVec& v);
	void normalize() { normalize(*this); }
	cxVec get_normalized() const {
		cxVec n;
		n.normalize(*this);
		return n;
	}

	float azimuth() const { return ::mth_atan2f(x, z); }
	float elevation() const { return -::mth_atan2f(y, nxCalc::hypot(x, z)); }

	void min(const cxVec& v) {
		x = nxCalc::min(x, v.x);
		y = nxCalc::min(y, v.y);
		z = nxCalc::min(z, v.z);
	}

	void min(const cxVec& v1, const cxVec& v2) {
		x = nxCalc::min(v1.x, v2.x);
		y = nxCalc::min(v1.y, v2.y);
		z = nxCalc::min(v1.z, v2.z);
	}

	void max(const cxVec& v) {
		x = nxCalc::max(x, v.x);
		y = nxCalc::max(y, v.y);
		z = nxCalc::max(z, v.z);
	}

	void max(const cxVec& v1, const cxVec& v2) {
		x = nxCalc::max(v1.x, v2.x);
		y = nxCalc::max(v1.y, v2.y);
		z = nxCalc::max(v1.z, v2.z);
	}

	void clamp(const cxVec& v, const cxVec& vmin, const cxVec& vmax) {
		x = nxCalc::clamp(v.x, vmin.x, vmax.x);
		y = nxCalc::clamp(v.y, vmin.y, vmax.y);
		z = nxCalc::clamp(v.z, vmin.z, vmax.z);
	}

	void clamp(const cxVec& vmin, const cxVec& vmax) {
		x = nxCalc::clamp(x, vmin.x, vmax.x);
		y = nxCalc::clamp(y, vmin.y, vmax.y);
		z = nxCalc::clamp(z, vmin.z, vmax.z);
	}

	void saturate(const cxVec& v) {
		x = nxCalc::saturate(v.x);
		y = nxCalc::saturate(v.y);
		z = nxCalc::saturate(v.z);
	}

	void saturate() {
		x = nxCalc::saturate(x);
		y = nxCalc::saturate(y);
		z = nxCalc::saturate(z);
	}

	cxVec get_clamped(const cxVec& vmin, const cxVec& vmax) const {
		cxVec v;
		v.clamp(*this, vmin, vmax);
		return v;
	}

	cxVec get_saturated() const {
		cxVec v;
		v.saturate(*this);
		return v;
	}

	float dot(const cxVec& v) const {
		return x*v.x + y*v.y + z*v.z;
	}

	void cross(const cxVec& v1, const cxVec& v2) {
		float v10 = v1.x;
		float v11 = v1.y;
		float v12 = v1.z;
		float v20 = v2.x;
		float v21 = v2.y;
		float v22 = v2.z;
		x = v11*v22 - v12*v21;
		y = v12*v20 - v10*v22;
		z = v10*v21 - v11*v20;
	}

	void lerp(const cxVec& v1, const cxVec& v2, float t) {
		x = nxCalc::lerp(v1.x, v2.x, t);
		y = nxCalc::lerp(v1.y, v2.y, t);
		z = nxCalc::lerp(v1.z, v2.z, t);
	}

	xt_float4 get_pnt() const {
		xt_float4 pnt;
		pnt.set(x, y, z, 1.0f);
		return pnt;
	}

	xt_float2 encode_octa() const;
	void decode_octa(const xt_float2& oct);

	bool all_gt0() const { return x > 0.0f && y > 0.0f && z > 0.0f; }
	bool all_ge0() const { return x >= 0.0f && y >= 0.0f && z >= 0.0f; }
	bool all_lt0() const { return x < 0.0f && y < 0.0f && z < 0.0f; }
	bool all_le0() const { return x <= 0.0f && y <= 0.0f && z <= 0.0f; }
};

inline cxVec operator + (const cxVec& v1, const cxVec& v2) { cxVec v = v1; v.add(v2); return v; }
inline cxVec operator - (const cxVec& v1, const cxVec& v2) { cxVec v = v1; v.sub(v2); return v; }
inline cxVec operator * (const cxVec& v1, const cxVec& v2) { cxVec v = v1; v.mul(v2); return v; }
inline cxVec operator / (const cxVec& v1, const cxVec& v2) { cxVec v = v1; v.div(v2); return v; }
inline cxVec operator * (const cxVec& v0, float s) { cxVec v = v0; v.scl(s); return v; }
inline cxVec operator * (float s, const cxVec& v0) { cxVec v = v0; v.scl(s); return v; }

namespace nxVec {

inline cxVec from_mem(const float* pSrc) {
	cxVec v;
	v.from_mem(pSrc);
	return v;
}

inline cxVec cross(const cxVec& v1, const cxVec& v2) {
	cxVec cv;
	cv.cross(v1, v2);
	return cv;
}

inline float scalar_triple(const cxVec& v0, const cxVec& v1, const cxVec& v2) {
	return cross(v0, v1).dot(v2);
}

inline cxVec lerp(const cxVec& v1, const cxVec& v2, float t) {
	cxVec v;
	v.lerp(v1, v2, t);
	return v;
}

inline cxVec min(const cxVec& v1, const cxVec& v2) {
	cxVec v;
	v.min(v1, v2);
	return v;
}

inline cxVec max(const cxVec& v1, const cxVec& v2) {
	cxVec v;
	v.max(v1, v2);
	return v;
}

inline cxVec rcp0(const cxVec& v) {
	return cxVec(nxCalc::rcp0(v.x), nxCalc::rcp0(v.y), nxCalc::rcp0(v.z));
}

float dist2(const cxVec& pos0, const cxVec& pos1);
float dist(const cxVec& pos0, const cxVec& pos1);

cxVec get_axis(exAxis axis);
cxVec from_polar_uv(float u, float v);
cxVec reflect(const cxVec& vec, const cxVec& nrm);

cxVec decode_octa(const xt_float2& oct);
cxVec decode_octa(const xt_half2& oct);
cxVec decode_octa(const int16_t oct[2]);
cxVec decode_octa(const uint16_t oct[2]);

cxVec rot_sc_x(const cxVec& v, const float s, const float c);
cxVec rot_sc_y(const cxVec& v, const float s, const float c);
cxVec rot_sc_z(const cxVec& v, const float s, const float c);
cxVec rot_rad_x(const cxVec& v, const float rx);
cxVec rot_rad_y(const cxVec& v, const float ry);
cxVec rot_rad_z(const cxVec& v, const float rz);
cxVec rot_deg_x(const cxVec& v, const float dx);
cxVec rot_deg_y(const cxVec& v, const float dy);
cxVec rot_deg_z(const cxVec& v, const float dz);
cxVec rot_rad(const cxVec& v, const float rx, const float ry, const float rz, exRotOrd rord = exRotOrd::XYZ);
cxVec rot_deg(const cxVec& v, const float dx, const float dy, const float dz, exRotOrd rord = exRotOrd::XYZ);

} // nxVec


class cxMtx : public xt_mtx {
protected:
	void set_rot_n(const cxVec& axis, float ang);

public:
	cxMtx() {}

	void identity_sr();

	void from_mem(const float* pSrc) { ::memcpy(this, pSrc, sizeof(cxMtx)); }
	void to_mem(float* pDst) const { ::memcpy(pDst, this, sizeof(cxMtx)); }

	void from_quat(const cxQuat& q);
	void from_quat_and_pos(const cxQuat& qrot, const cxVec& vtrans);
	cxQuat to_quat() const;

	void transpose();
	void transpose(const cxMtx& mtx);
	void transpose_sr();
	void transpose_sr(const cxMtx& mtx);
	void invert();
	void invert(const cxMtx& m);
	void invert_fast();
	void invert_lu();
	void invert_lu_hi();

	void mul(const cxMtx& mtx);
	void mul(const cxMtx& m1, const cxMtx& m2);
	void add(const cxMtx& mtx);
	void add(const cxMtx& m1, const cxMtx& m2);
	void sub(const cxMtx& mtx);
	void sub(const cxMtx& m1, const cxMtx& m2);

	void rev_mul(const cxMtx& mtx);

	cxVec get_translation() const {
		return cxVec(m[3][0], m[3][1], m[3][2]);
	}

	cxVec get_row_vec(int i) const {
		cxVec v(0.0f);
		switch (i) {
			case 0: v.set(m[0][0], m[0][1], m[0][2]); break;
			case 1: v.set(m[1][0], m[1][1], m[1][2]); break;
			case 2: v.set(m[2][0], m[2][1], m[2][2]); break;
			case 3: v.set(m[3][0], m[3][1], m[3][2]); break;
		}
		return v;
	}

	cxVec get_col_vec(int j) const {
		cxVec v(0.0f);
		switch (j) {
			case 0: v.set(m[0][0], m[1][0], m[2][0]); break;
			case 1: v.set(m[0][1], m[1][1], m[2][1]); break;
			case 2: v.set(m[0][2], m[1][2], m[2][2]); break;
			case 3: v.set(m[0][3], m[1][3], m[2][3]); break;
		}
		return v;
	}

	void set_translation(float tx, float ty, float tz) {
		m[3][0] = tx;
		m[3][1] = ty;
		m[3][2] = tz;
		m[3][3] = 1.0f;
	}

	void set_translation(const cxVec& tv) {
		set_translation(tv.x, tv.y, tv.z);
	}

	void mk_translation(const cxVec& tv) {
		identity_sr();
		set_translation(tv);
	}

	cxMtx get_transposed() const {
		cxMtx m;
		m.transpose(*this);
		return m;
	}

	cxMtx get_transposed_sr() const {
		cxMtx m;
		m.transpose_sr(*this);
		return m;
	}

	cxMtx get_inverted() const {
		cxMtx m;
		m.invert(*this);
		return m;
	}

	cxMtx get_sr() const {
		cxMtx m = *this;
		m.set_translation(cxVec(0.0f));
		return m;
	}

	void from_upvec(const cxVec& n);

	void orient_zy(const cxVec& axisZ, const cxVec& axisY, bool normalizeInput = true);
	void orient_zx(const cxVec& axisZ, const cxVec& axisX, bool normalizeInput = true);

	void set_rot_frame(const cxVec& axisX, const cxVec& axisY, const cxVec& axisZ);
	void set_rot(const cxVec& axis, float ang);
	void set_rot_x(float rx);
	void set_rot_y(float ry);
	void set_rot_z(float rz);
	void set_rot(float rx, float ry, float rz, exRotOrd ord = exRotOrd::XYZ);
	void set_rot_degrees(const cxVec& r, exRotOrd ord = exRotOrd::XYZ);

	bool is_valid_rot(float tol = 1.0e-3f) const;
	cxVec get_rot(exRotOrd ord = exRotOrd::XYZ) const;
	cxVec get_rot_degrees(exRotOrd ord = exRotOrd::XYZ) const { return get_rot(ord) * XD_RAD2DEG(1.0f); }

	void mk_scl(float sx, float sy, float sz);
	void mk_scl(const cxVec& sv) { mk_scl(sv.x, sv.y, sv.z); }
	void mk_scl(float s) { mk_scl(s, s, s); }

	cxVec get_scl(cxMtx* pMtxRT = nullptr, exTransformOrd xord = exTransformOrd::SRT) const;

	void calc_xform(const cxMtx& mtxT, const cxMtx& mtxR, const cxMtx& mtxS, exTransformOrd ord = exTransformOrd::SRT);

	void mk_view(const cxVec& pos, const cxVec& tgt, const cxVec& upvec, cxMtx* pInv = nullptr);
	void mk_proj(float fovy, float aspect, float znear, float zfar);

	cxVec calc_vec(const cxVec& v) const;
	cxVec calc_pnt(const cxVec& v) const;
	xt_float4 apply(const xt_float4& qv) const;

	float norm(bool hprec = true) const;
	float det(bool hprec = true) const;
	float det_sr(bool hprec = true) const;
};

inline cxMtx operator * (const cxMtx& m1, const cxMtx& m2) { cxMtx m = m1; m.mul(m2); return m; }
inline cxMtx operator + (const cxMtx& m1, const cxMtx& m2) { cxMtx m = m1; m.add(m2); return m; }
inline cxMtx operator - (const cxMtx& m1, const cxMtx& m2) { cxMtx m = m1; m.sub(m2); return m; }

namespace nxMtx {

inline cxMtx identity() {
	cxMtx m;
	m.identity();
	return m;
}

inline cxMtx from_mem(const float* pSrc) {
	cxMtx m;
	if (pSrc) {
		m.from_mem(pSrc);
	} else {
		m.identity();
	}
	return m;
}

inline cxMtx orient_zy(const cxVec& axisZ, const cxVec& axisY, bool normalizeInput = true) {
	cxMtx m;
	m.orient_zy(axisZ, axisY, normalizeInput);
	return m;
}

inline cxMtx orient_zx(const cxVec& axisZ, const cxVec& axisX, bool normalizeInput = true) {
	cxMtx m;
	m.orient_zx(axisZ, axisX, normalizeInput);
	return m;
}

inline cxMtx from_axis_angle(const cxVec& axis, float ang) {
	cxMtx m;
	m.set_rot(axis, ang);
	return m;
}

inline cxMtx from_quat_and_pos(const cxQuat& rot, const cxVec& pos) {
	cxMtx m;
	m.from_quat_and_pos(rot, pos);
	return m;
}

inline cxMtx mk_rot_frame(const cxVec& axisX, const cxVec& axisY, const cxVec& axisZ) {
	cxMtx m;
	m.set_rot_frame(axisX, axisY, axisZ);
	return m;
}

inline cxMtx mk_rot(const cxVec& axis, const float ang) {
	cxMtx m;
	m.set_rot(axis, ang);
	return m;
}

inline cxMtx mk_rot_x(const float rx) {
	cxMtx m;
	m.set_rot_x(rx);
	return m;
}

inline cxMtx mk_rot_y(const float ry) {
	cxMtx m;
	m.set_rot_y(ry);
	return m;
}

inline cxMtx mk_rot_z(const float rz) {
	cxMtx m;
	m.set_rot_z(rz);
	return m;
}

inline cxMtx mk_rot(const float rx, const float ry, const float rz, const exRotOrd ord = exRotOrd::XYZ) {
	cxMtx m;
	m.set_rot(rx, ry, rz, ord);
	return m;
}

inline cxMtx mk_rot_degrees(const cxVec& r, const exRotOrd ord = exRotOrd::XYZ) {
	cxMtx m;
	m.set_rot_degrees(r, ord);
	return m;
}

inline cxMtx mk_rot_degrees(const float dx, const float dy, const float dz, const exRotOrd ord = exRotOrd::XYZ) {
	return mk_rot_degrees(cxVec(dx, dy, dz), ord);
}

inline cxMtx mk_scl(const cxVec sv) {
	cxMtx m;
	m.mk_scl(sv);
	return m;
}

inline cxMtx mk_pos(const cxVec& v) {
	cxMtx m;
	m.mk_translation(v);
	return m;
}

inline cxMtx mk_pos(const float x, const float y, const float z) {
	return mk_pos(cxVec(x, y, z));
}

inline cxMtx mk_translation(const cxVec& tv) {
	cxMtx m;
	m.mk_translation(tv);
	return m;
}

void clean_rotations(cxMtx* pMtx, const int n);

inline xt_xmtx xmtx_identity() {
	xt_xmtx xm;
	xm.identity();
	return xm;
}

cxMtx mtx_from_xmtx(const xt_xmtx& xm);
xt_xmtx xmtx_from_mtx(const cxMtx& m);

cxVec xmtx_calc_vec(const xt_xmtx& m, const cxVec& v);
cxVec xmtx_calc_pnt(const xt_xmtx& m, const cxVec& v);
xt_xmtx xmtx_concat(const xt_xmtx& a, const xt_xmtx& b);
xt_xmtx xmtx_basis(const cxVec& ax, const cxVec& ay, const cxVec& az, const cxVec& pos);
xt_xmtx xmtx_from_quat_pos(const cxQuat& quat, const cxVec& pos);
xt_xmtx xmtx_from_deg_pos(const float dx, const float dy, const float dz, const cxVec& pos, exRotOrd rord = exRotOrd::XYZ);
cxVec xmtx_get_pos(const xt_xmtx& xm);
void xmtx_set_pos(xt_xmtx& xm, const cxVec& pos);

void dump_hgeo(FILE* pOut, const cxMtx* pMtx, const int n, float scl = 1.0f);
void dump_hgeo(const char* pOutPath, const cxMtx* pMtx, const int n, float scl = 1.0f);

} // nxMtx


class cxQuat : public xt_float4 {
public:
	cxQuat() {}
	cxQuat(float xval, float yval, float zval, float wval) { set(xval, yval, zval, wval); }

	cxQuat(const cxQuat& q) = default;
	cxQuat& operator=(const cxQuat& q) = default;

	void zero() { fill(0.0f); }
	void identity() { set(0.0f, 0.0f, 0.0f, 1.0f); }

	cxVec get_axis_x() const { return cxVec(1.0f - (2.0f*y*y) - (2.0f*z*z), (2.0f*x*y) + (2.0f*w*z), (2.0f*x*z) - (2.0f*w*y)); }
	cxVec get_axis_y() const { return cxVec((2.0f*x*y) - (2.0f*w*z), 1.0f - (2.0f*x*x) - (2.0f*z*z), (2.0f*y*z) + (2.0f*w*x)); }
	cxVec get_axis_z() const { return cxVec((2.0f*x*z) + (2.0f*w*y), (2.0f*y*z) - (2.0f*w*x), 1.0f - (2.0f*x*x) - (2.0f*y*y)); }

	void from_mem(const float* pSrc) { set(pSrc[0], pSrc[1], pSrc[2], pSrc[3]); }

	void from_mtx(const cxMtx& m);
	cxMtx to_mtx() const;
	float get_axis_ang(cxVec* pAxis) const;
	cxVec get_log_vec() const;
	void from_log_vec(const cxVec& lvec, bool nrmFlg = true);
	void from_vecs(const cxVec& vfrom, const cxVec& vto);

	float get_real_part() const { return w; }
	cxVec get_imag_part() const { return cxVec(x, y, z); }
	float get_scalar_part() const { return get_real_part(); }
	cxVec get_vector_part() const { return get_imag_part(); }
	void from_parts(const cxVec& imag, float real) { set(imag.x, imag.y, imag.z, real); }

	cxMtx get_col_mtx() const;
	cxMtx get_row_mtx() const;

	float dot(const cxQuat& q) const { return x*q.x + y*q.y + z*q.z + w*q.w; }

	float mag2() const { return dot(*this); }
	float mag() const { return ::mth_sqrtf(mag2()); }

	void mul(const cxQuat& qa, const cxQuat& qb) {
		float tx = qa.w*qb.x + qa.x*qb.w + qa.y*qb.z - qa.z*qb.y;
		float ty = qa.w*qb.y + qa.y*qb.w + qa.z*qb.x - qa.x*qb.z;
		float tz = qa.w*qb.z + qa.z*qb.w + qa.x*qb.y - qa.y*qb.x;
		float tw = qa.w*qb.w - qa.x*qb.x - qa.y*qb.y - qa.z*qb.z;
		set(tx, ty, tz, tw);
	}

	void mul(const cxQuat& q) { mul(*this, q); }

	void add(const cxQuat& qa, const cxQuat& qb) {
		for (int i = 0; i < 4; ++i) {
			(*this)[i] = qa[i] + qb[i];
		}
	}

	void add(const cxQuat& q) {
		for (int i = 0; i < 4; ++i) {
			(*this)[i] += q[i];
		}
	}

	void sub(const cxQuat& qa, const cxQuat& qb) {
		for (int i = 0; i < 4; ++i) {
			(*this)[i] = qa[i] - qb[i];
		}
	}

	void sub(const cxQuat& q) {
		for (int i = 0; i < 4; ++i) {
			(*this)[i] -= q[i];
		}
	}

	void scale(float s) { scl(s); }

	void scale(const cxQuat& q, float s) {
		*this = q;
		scale(s);
	}

	cxQuat get_scaled(float s) const {
		cxQuat q;
		q.scale(*this, s);
		return q;
	}

	void normalize() {
		float len = mag();
		if (len > 0.0f) {
			float rlen = 1.0f / len;
			scale(rlen);
		}
	}

	void normalize(const cxQuat& q) {
		*this = q;
		normalize();
	}

	cxQuat get_normalized() const {
		cxQuat q;
		q.normalize(*this);
		return q;
	}

	void conjugate() {
		x = -x;
		y = -y;
		z = -z;
	}

	void conjugate(const cxQuat& q) {
		x = -q.x;
		y = -q.y;
		z = -q.z;
		w = q.w;
	}

	cxQuat get_conjugate() const {
		cxQuat q;
		q.conjugate(*this);
		return q;
	}

	void invert() {
		float lsq = mag2();
		float rlsq = nxCalc::rcp0(lsq);
		conjugate();
		scale(rlsq);
	}

	void invert(const cxQuat& q) {
		*this = q;
		invert();
	}

	cxQuat get_inverted() const {
		cxQuat q;
		q.invert(*this);
		return q;
	}

	void negate() {
		x = -x;
		y = -y;
		z = -z;
		w = -w;
	}

	void negate(const cxQuat& q) {
		x = -q.x;
		y = -q.y;
		z = -q.z;
		w = -q.w;
	}

	cxQuat get_negated() const {
		cxQuat q;
		q.negate(*this);
		return q;
	}

	void set_rot(const cxVec& axis, float ang);
	void set_rot_x(float rx);
	void set_rot_y(float ry);
	void set_rot_z(float rz);
	void set_rot(float rx, float ry, float rz, exRotOrd ord = exRotOrd::XYZ);
	void set_rot_degrees(float dx, float dy, float dz, exRotOrd ord = exRotOrd::XYZ);
	void set_rot_degrees(const cxVec& r, exRotOrd ord = exRotOrd::XYZ);

	cxVec get_rot(exRotOrd ord = exRotOrd::XYZ) const;
	cxVec get_rot_degrees(exRotOrd ord = exRotOrd::XYZ) const { return get_rot(ord) * XD_RAD2DEG(1.0f); }

	void slerp(const cxQuat& q1, const cxQuat& q2, float t);

	void lerp(const cxQuat& q1, const cxQuat& q2, float t) {
		const float* p1 = q1;
		const float* p2 = q2;
		float* p = *this;
		for (int i = 0; i < 4; ++i) {
			p[i] = p1[i] + (p2[i] - p1[i])*t;
		}
	}

	cxVec apply(const cxVec& v) const;

	cxQuat& operator += (const cxQuat& q) { add(q); return *this; }
	cxQuat& operator -= (const cxQuat& q) { sub(q); return *this; }
	cxQuat& operator *= (const cxQuat& q) { mul(q); return *this; }
	cxQuat& operator *= (const float s) { scl(s); return *this; }

	cxQuat get_closest_x() const;
	cxQuat get_closest_y() const;
	cxQuat get_closest_z() const;
	cxQuat get_closest_xy() const;
	cxQuat get_closest_yx() const;
	cxQuat get_closest_xz() const;
	cxQuat get_closest_zx() const;
	cxQuat get_closest_yz() const;
	cxQuat get_closest_zy() const;
};

inline cxQuat operator * (const cxQuat& q1, const cxQuat& q2) { cxQuat q = q1; q.mul(q2); return q; }
inline cxQuat operator + (const cxQuat& q1, const cxQuat& q2) { cxQuat q = q1; q.add(q2); return q; }
inline cxQuat operator - (const cxQuat& q1, const cxQuat& q2) { cxQuat q = q1; q.sub(q2); return q; }
inline cxQuat operator * (const cxQuat& q, const float s) { cxQuat r = q; r.scl(s); return r; }

namespace nxQuat {

inline cxQuat identity() {
	cxQuat q;
	q.identity();
	return q;
}

inline cxQuat zero() {
	cxQuat q;
	q.zero();
	return q;
}
	
inline cxQuat slerp(const cxQuat& q1, const cxQuat& q2, float t) {
	cxQuat q;
	q.slerp(q1, q2, t);
	return q;
}

inline cxQuat from_radians(float rx, float ry, float rz, exRotOrd ord = exRotOrd::XYZ) {
	cxQuat q;
	q.set_rot(rx, ry, rz, ord);
	return q;
}

inline cxQuat from_degrees(float dx, float dy, float dz, exRotOrd ord = exRotOrd::XYZ) {
	cxQuat q;
	q.set_rot_degrees(dx, dy, dz, ord);
	return q;
}

inline cxQuat from_log_vec(const cxVec& v, bool nrmFlg = true) {
	cxQuat q;
	q.from_log_vec(v, nrmFlg);
	return q;
}

inline cxQuat from_mtx(const cxMtx& mtx) {
	cxQuat q;
	q.from_mtx(mtx);
	return q;
}

cxQuat log(const cxQuat& q);
cxQuat exp(const cxQuat& q);
cxQuat pow(const cxQuat& q, float x);

float arc_dist(const cxQuat& a, const cxQuat& b);

} // nxQuat


class cxDualQuat {
protected:
	cxQuat mReal;
	cxQuat mDual;

public:

	void set(const cxQuat& rot, const cxVec& pos) {
		mReal = rot;
		mDual = cxQuat(pos.x, pos.y, pos.z, 0.0f) * mReal * 0.5f; // RT
	}

	void set_tr(const cxQuat& rot, const cxVec& pos) {
		mReal = rot;
		mDual = mReal * cxQuat(pos.x, pos.y, pos.z, 0.0f) * 0.5f; // TR
	}

	void set_parts(const cxQuat& qreal, const cxQuat& qdual) {
		mReal = qreal;
		mDual = qdual;
	}

	cxQuat get_real_part() const { return mReal; }
	cxQuat get_dual_part() const { return mDual; }

	xt_float2 dot(const cxDualQuat& dq) const {
		xt_float2 dd;
		float r = mReal.dot(dq.mReal);
		float d = mReal.dot(dq.mDual) + mDual.dot(dq.mReal);
		dd.set(r, d);
		return dd;
	}

	xt_float2 mag() const {
		float rm = mReal.mag();
		float dm = mReal.dot(mDual) * nxCalc::rcp0(rm);
		xt_float2 mag;
		mag.set(rm, dm);
		return mag;
	}

	void normalize(bool fast = false);

	void mul(const cxDualQuat& dq1, const cxDualQuat& dq2);
	void mul(const cxDualQuat& dq);

	cxVec get_pos() const {
		return (mDual.get_scaled(2.0f) * mReal.get_conjugate()).get_vector_part();
	}

	cxMtx to_mtx() const {
		cxMtx m = mReal.to_mtx();
		m.set_translation(get_pos());
		return m;
	}

	cxMtx get_inv_mtx() const {
		cxDualQuat idq;
		idq.mReal = mReal.get_conjugate();
		idq.mDual = mDual.get_conjugate();
		return idq.to_mtx();
	}

	cxVec calc_pnt(const cxVec& pos) const;

	cxVec calc_vec(const cxVec& vec) const {
		return mReal.apply(vec);
	}

	void interpolate(const cxDualQuat& dqA, const cxDualQuat& dqB, float t);
};

namespace nxDualQuat {

inline cxDualQuat from_parts(const cxQuat& qreal, const cxQuat& qdual) {
	cxDualQuat dq;
	dq.set_parts(qreal, qdual);
	return dq;
}

} // nxDualQuat


class cxLineSeg;
class cxPlane;
class cxSphere;
class cxAABB;
class cxCapsule;

namespace nxGeom {

inline cxVec tri_normal_cw(const cxVec& v0, const cxVec& v1, const cxVec& v2) {
	return nxVec::cross(v0 - v1, v2 - v1).get_normalized();
}

inline cxVec tri_normal_ccw(const cxVec& v0, const cxVec& v1, const cxVec& v2) {
	return nxVec::cross(v1 - v0, v2 - v0).get_normalized();
}

inline float signed_tri_area_2d(const float ax, const float ay, const float bx, const float by, const float cx, const float cy) {
	return (ax - cx)*(by - cy) - (ay - cy)*(bx - cx);
}

bool seg_seg_overlap_2d(const float s0x0, const float s0y0, const float s0x1, const float s0y1, const float s1x0, const float s1y0, const float s1x1, const float s1y1);
bool seg_plane_intersect(const cxVec& p0, const cxVec& p1, const cxPlane& pln, float* pT = nullptr, cxVec* pHitPos = nullptr);
bool seg_sph_intersect(const cxVec& p0, const cxVec& p1, const cxSphere& sph, float* pT = nullptr, cxVec* pHitPos = nullptr);
bool seg_sph_intersect(const cxVec& p0, const cxVec& p1, const cxVec& c, const float r, float* pT = nullptr, cxVec* pHitPos = nullptr);
bool seg_quad_intersect_cw(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& v3, cxVec* pHitPos = nullptr, cxVec* pHitNrm = nullptr);
bool seg_quad_intersect_cw_n(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& v3, const cxVec& nrm, cxVec* pHitPos = nullptr);
bool seg_quad_intersect_ccw(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& v3, cxVec* pHitPos = nullptr, cxVec* pHitNrm = nullptr);
bool seg_quad_intersect_ccw_n(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& v3, const cxVec& nrm, cxVec* pHitPos = nullptr);
bool seg_polyhedron_intersect(const cxVec& p0, const cxVec& p1, const cxPlane* pPln, int plnNum, float* pFirst = nullptr, float* pLast = nullptr);
bool seg_tri_intersect_cw_n(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& nrm, cxVec* pHitPos = nullptr);
bool seg_tri_intersect_cw(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, cxVec* pHitPos = nullptr, cxVec* pHitNrm = nullptr);
bool seg_tri_intersect_ccw_n(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& nrm, cxVec* pHitPos = nullptr);
bool seg_tri_intersect_ccw(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, cxVec* pHitPos = nullptr, cxVec* pHitNrm = nullptr);
xt_float4 seg_tri_intersect_bc_cw(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2);
xt_float4 seg_tri_intersect_bc_ccw(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2);
cxVec barycentric(const cxVec& pos, const cxVec& v0, const cxVec& v1, const cxVec& v2);
cxVec barycentric_uv(const cxVec& pos, const cxVec& v0, const cxVec& v1, const cxVec& v2);
float quad_dist2(const cxVec& pos, const cxVec vtx[4]);
int quad_convex_ck(const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& v3);
void update_nrm_newell(cxVec* pNrm, cxVec* pVtxI, cxVec* pVtxJ);
cxVec poly_normal_cw(cxVec* pVtx, const int vtxNum);
cxVec poly_normal_ccw(cxVec* pVtx, const int vtxNum);

inline float dir_line_pnt_closest(const cxVec& lp, const cxVec& dir, const cxVec& pnt, cxVec* pClosestPos = nullptr) {
	cxVec vec = pnt - lp;
	float t = nxCalc::div0(vec.dot(dir), dir.dot(dir));
	if (pClosestPos) {
		*pClosestPos = lp + dir*t;
	}
	return t;
}

inline float line_pnt_closest(const cxVec& lp0, const cxVec& lp1, const cxVec& pnt, cxVec* pClosestPos = nullptr, cxVec* pLineDir = nullptr) {
	cxVec dir = lp1 - lp0;
	cxVec vec = pnt - lp0;
	float t = nxCalc::div0(vec.dot(dir), dir.dot(dir));
	if (pClosestPos) {
		*pClosestPos = lp0 + dir*t;
	}
	if (pLineDir) {
		*pLineDir = dir;
	}
	return t;
}

inline cxVec seg_pnt_closest(const cxVec& sp0, const cxVec& sp1, const cxVec& pnt) {
	cxVec dir;
	float t = nxCalc::saturate(line_pnt_closest(sp0, sp1, pnt, nullptr, &dir));
	return sp0 + dir*t;
}

cxVec tri_pnt_closest(const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& pnt);

float seg_seg_dist2(const cxVec& s0p0, const cxVec& s0p1, const cxVec& s1p0, const cxVec& s1p1, cxVec* pBridgeP0 = nullptr, cxVec* pBridgeP1 = nullptr);

inline bool pnt_in_tri(const cxVec& pos, const cxVec& v0, const cxVec& v1, const cxVec& v2) {
	cxVec a = v0 - pos;
	cxVec b = v1 - pos;
	cxVec c = v2 - pos;
	cxVec u = nxVec::cross(b, c);
	if (u.dot(nxVec::cross(c, a)) < 0.0f) return false;
	if (u.dot(nxVec::cross(a, b)) < 0.0f) return false;
	return true;
}

inline bool pnt_in_aabb(const cxVec& pos, const cxVec& min, const cxVec& max) {
	if (pos.x < min.x || pos.x > max.x) return false;
	if (pos.y < min.y || pos.y > max.y) return false;
	if (pos.z < min.z || pos.z > max.z) return false;
	return true;
}

inline bool pnt_in_sph(const cxVec& pos, const cxVec& sphc, const float sphr, float* pSqDist = nullptr) {
	float d2 = nxVec::dist2(pos, sphc);
	float r2 = nxCalc::sq(sphr);
	if (pSqDist) {
		*pSqDist = d2;
	}
	return d2 <= r2;
}

inline bool pnt_in_cap(const cxVec& pos, const cxVec& cp0, const cxVec& cp1, const float cr, float* pSqDist = nullptr) {
	float d2 = nxVec::dist2(pos, seg_pnt_closest(cp0, cp1, pos));
	float r2 = nxCalc::sq(cr);
	if (pSqDist) {
		*pSqDist = d2;
	}
	return d2 <= r2;
}

inline bool aabb_aabb_overlap(const cxVec& min0, const cxVec& max0, const cxVec& min1, const cxVec& max1) {
	if (min0.x > max1.x || max0.x < min1.x) return false;
	if (min0.y > max1.y || max0.y < min1.y) return false;
	if (min0.z > max1.z || max0.z < min1.z) return false;
	return true;
}

inline bool sph_sph_overlap(const cxVec& s0c, const float s0r, const cxVec& s1c, const float s1r) {
	float cdd = nxVec::dist2(s0c, s1c);
	float rsum = s0r + s1r;
	return cdd <= nxCalc::sq(rsum);
}

inline bool sph_aabb_overlap(const cxVec& sc, const float sr, const cxVec& bmin, const cxVec& bmax) {
	cxVec pos = sc.get_clamped(bmin, bmax);
	float dd = nxVec::dist2(sc, pos);
	return dd <= nxCalc::sq(sr);
}

inline bool sph_cap_overlap(const cxVec& sc, const float sr, const cxVec& cp0, const cxVec& cp1, const float cr, cxVec* pCapAxisPos = nullptr) {
	cxVec pos = seg_pnt_closest(cp0, cp1, sc);
	bool flg = sph_sph_overlap(sc, sr, pos, cr);
	if (flg && pCapAxisPos) {
		*pCapAxisPos = pos;
	}
	return flg;
}

inline bool cap_cap_overlap(const cxVec& c0p0, const cxVec& c0p1, const float cr0, const cxVec& c1p0, const cxVec& c1p1, const float cr1) {
	float dist2 = seg_seg_dist2(c0p0, c0p1, c1p0, c1p1);
	float rsum2 = nxCalc::sq(cr0 + cr1);
	return dist2 <= rsum2;
}

bool cap_aabb_overlap(const cxVec& cp0, const cxVec& cp1, const float cr, const cxVec& bmin, const cxVec& bmax);

bool tri_aabb_overlap(const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& bmin, const cxVec& bmax);
bool tri_aabb_overlap(const cxVec vtx[3], const cxVec& bmin, const cxVec& bmax);

float sph_region_weight(const cxVec& pos, const cxVec& center, const float attnStart, const float attnEnd);
float cap_region_weight(const cxVec& pos, const cxVec& capPos0, const cxVec& capPos1, const float attnStart, const float attnEnd);
float obb_region_weight(const cxVec& pos, const cxMtx& invMtx, const cxVec& attnStart, const cxVec& attnEnd);

float sph_convex_dist(
	const cxMtx& xformSph, const float radius,
	const cxMtx& xformCvx, const cxVec* pPts, const int npts,
	const uint16_t* pTriPids, const cxVec* pTriNrms, const int ntri,
	cxVec* pSphPnt = nullptr, cxVec* pCvxPnt = nullptr, cxVec* pSepVec = nullptr,
	const int xformMode = 0, const bool cw = false
);

} // nxGeom


class cxLineSeg {
protected:
	cxVec mPos0;
	cxVec mPos1;

public:
	cxLineSeg() {}
	cxLineSeg(const cxVec& p0, const cxVec& p1) { set(p0, p1); }

	cxVec get_pos0() const { return mPos0; }
	cxVec* get_pos0_ptr() { return &mPos0; }
	void set_pos0(const cxVec& p0) { mPos0 = p0; }
	cxVec get_pos1() const { return mPos1; }
	cxVec* get_pos1_ptr() { return &mPos1; }
	void set_pos1(const cxVec& p1) { mPos1 = p1; }
	void set_org_dir(const cxVec& org, const cxVec& dir) { mPos0 = org; mPos1 = org + dir; }
	void set(const cxVec& p0, const cxVec& p1) { mPos0 = p0; mPos1 = p1; }
	void zero() { mPos0.fill(0.0f); mPos1.fill(0.0f); }
	cxVec get_inner_pos(float t) const { return nxVec::lerp(mPos0, mPos1, nxCalc::saturate(t)); }
	cxVec get_center() const { return (mPos0 + mPos1) * 0.5f; }
	cxVec get_dir() const { return mPos1 - mPos0; }
	cxVec get_dir_nrm() const { return get_dir().get_normalized(); }
	float len2() const { return nxVec::dist2(mPos0, mPos1); }
	float len() const { return nxVec::dist(mPos0, mPos1); }
};

class cxPlane {
protected:
	cxVec mABC;
	float mD;

public:
	cxPlane() {}

	void calc(const cxVec& pos, const cxVec& nrm);
	cxVec get_normal() const { return mABC; }
	float get_D() const { return mD; }
	float signed_dist(const cxVec& pos) const { return pos.dot(get_normal()) - get_D(); }
	float dist(const cxVec& pos) const { return ::mth_fabsf(signed_dist(pos)); }
	bool pnt_in_front(const cxVec& pos) const { return signed_dist(pos) >= 0.0f; }

	bool seg_intersect(const cxVec& p0, const cxVec& p1, float* pT = nullptr, cxVec* pHitPos = nullptr) const {
		return nxGeom::seg_plane_intersect(p0, p1, *this, pT, pHitPos);
	}

	bool intersect(const cxLineSeg& seg, float* pT = nullptr, cxVec* pHitPos = nullptr) const {
		return seg_intersect(seg.get_pos0(), seg.get_pos1(), pT, pHitPos);
	}
};

class cxSphere {
protected:
	xt_float4 mSph;

public:
	cxSphere() {}
	cxSphere(const cxVec& c, float r) {
		mSph.set(c.x, c.y, c.z, r);
	}
	cxSphere(float x, float y, float z, float r) {
		mSph.set(x, y, z, r);
	}

	float get_radius() const {
		return mSph.w;
	}

	void set_radius(float r) {
		mSph.w = r;
	}

	cxVec get_center() const {
		cxVec c;
		c.set(mSph.x, mSph.y, mSph.z);
		return c;
	}

	void set_center(const cxVec& c) {
		mSph.x = c.x;
		mSph.y = c.y;
		mSph.z = c.z;
	}

	bool contains(const cxVec& pos, float* pSqDist = nullptr) const { return nxGeom::pnt_in_sph(pos, get_center(), get_radius(), pSqDist); }

	bool overlaps(const cxSphere& sph) const { return nxGeom::sph_sph_overlap(get_center(), get_radius(), sph.get_center(), sph.get_radius()); }
	bool overlaps(const cxAABB& box) const;
	bool overlaps(const cxCapsule& cap, cxVec* pCapAxisPos = nullptr) const;

	bool seg_intersect(const cxVec& p0, const cxVec& p1, float* pT = nullptr, cxVec* pHitPos = nullptr) const {
		return nxGeom::seg_sph_intersect(p0, p1, *this, pT, pHitPos);
	}

	bool intersect(const cxLineSeg& seg, float* pT = nullptr, cxVec* pHitPos = nullptr) const {
		return seg_intersect(seg.get_pos0(), seg.get_pos1(), pT, pHitPos);
	}
};

class cxAABB {
protected:
	cxVec mMin;
	cxVec mMax;

public:
	cxAABB() {}
	cxAABB(const cxVec& p0, const cxVec& p1) { set(p0, p1); }

	void init() {
		mMin.fill(FLT_MAX);
		mMax.fill(-FLT_MAX);
	}

	void set(const cxVec& pnt) {
		mMin = pnt;
		mMax = pnt;
	}

	void set(const cxVec& p0, const cxVec& p1) {
		mMin.min(p0, p1);
		mMax.max(p0, p1);
	}

	cxVec get_min_pos() const { return mMin; }
	cxVec get_max_pos() const { return mMax; }
	cxVec get_center() const { return ((mMin + mMax) * 0.5f); }
	cxVec get_size_vec() const { return mMax - mMin; }
	cxVec get_radius_vec() const { return get_size_vec() * 0.5f; }
	float get_bounding_radius() const { return get_radius_vec().mag(); }

	void add_pnt(const cxVec& p) {
		mMin.min(p);
		mMax.max(p);
	}

	void merge(const cxAABB& box) {
		mMin.min(box.mMin);
		mMax.max(box.mMax);
	}

	void transform(const cxAABB& box, const cxMtx& mtx);
	void transform(const cxMtx& mtx) { transform(*this, mtx); }

	void from_seg(const cxLineSeg& seg) { set(seg.get_pos0(), seg.get_pos1()); }
	void from_sph(const cxSphere& sph);

	cxVec closest_pnt(const cxVec& pos, bool onFace = true) const;

	bool contains(const cxVec& pos) const { return nxGeom::pnt_in_aabb(pos, mMin, mMax); }

	bool seg_ck(const cxVec& p0, const cxVec& p1) const;
	bool seg_ck(const cxLineSeg& seg) const { return seg_ck(seg.get_pos0(), seg.get_pos1()); }

	bool overlaps(const cxSphere& sph) const { return sph.overlaps(*this); }
	bool overlaps(const cxAABB& box) const { return nxGeom::aabb_aabb_overlap(mMin, mMax, box.mMin, box.mMax); }
};

class cxCapsule {
protected:
	cxVec mPos0;
	cxVec mPos1;
	float mRadius;

public:
	cxCapsule() {}
	cxCapsule(const cxVec& p0, const cxVec& p1, float radius) { set(p0, p1, radius); }

	void set(const cxVec& p0, const cxVec& p1, float radius) {
		mPos0 = p0;
		mPos1 = p1;
		mRadius = radius;
	}

	cxVec get_pos0() const { return mPos0; }
	cxVec get_pos1() const { return mPos1; }
	float get_radius() const { return mRadius; }
	cxVec get_center() const { return (mPos0 + mPos1) * 0.5f; }
	float get_bounding_radius() const { return nxVec::dist(mPos0, mPos1)*0.5f + mRadius; }

	bool contains(const cxVec& pos, float* pSqDist = nullptr) const { return nxGeom::pnt_in_cap(pos, get_pos0(), get_pos1(), get_radius(), pSqDist); }

	bool overlaps(const cxSphere& sph, cxVec* pAxisPos = nullptr) const { return sph.overlaps(*this, pAxisPos); }
	bool overlaps(const cxAABB& box) const;
	bool overlaps(const cxCapsule& cap) const;
};

class cxFrustum {
protected:
	cxVec mPnt[8];
	cxVec mNrm[6];
	cxPlane mPlane[6];

	void calc_normals();
	void calc_planes();

public:
	cxFrustum() {}

	void init(const cxMtx& mtx, float fovy, float aspect, float znear, float zfar);

	cxVec get_center() const;
	cxVec get_vertex(const int idx) const;
	cxPlane get_near_plane() const { return mPlane[0]; }
	cxPlane get_far_plane() const { return mPlane[5]; }
	cxPlane get_left_plane() const { return mPlane[1]; }
	cxPlane get_right_plane() const { return mPlane[3]; }
	cxPlane get_top_plane() const { return mPlane[2]; }
	cxPlane get_bottom_plane() const { return mPlane[4]; }

	bool cull(const cxSphere& sph) const;
	bool overlaps(const cxSphere& sph) const;
	bool cull(const cxAABB& box) const;
	bool overlaps(const cxAABB& box) const;

	void dump_geo(FILE* pOut) const;
	void dump_geo(const char* pOutPath) const;
};


class cxColor : public xt_rgba {
public:
	cxColor() {}
	cxColor(float val) { set(val); }
	cxColor(float r, float g, float b, float a = 1.0f) { set(r, g, b, a); }

	void set(float r, float g, float b, float a = 1.0f) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	void set(float val) { set(val, val, val); }

	void set(const cxVec& v) {
		set(v.x, v.y, v.z);
	}

	void set(const xt_float4& f) {
		set(f.x, f.y, f.z, f.w);
	}

	void set(const xt_half4& h) {
		set(h.get());
	}

	void zero() {
		for (int i = 0; i < 4; ++i) {
			ch[i] = 0.0f;
		}
	}

	void zero_rgb() {
		for (int i = 0; i < 3; ++i) {
			ch[i] = 0.0f;
		}
		a = 1.0f;
	}

	float luma() const { return r*0.299f + g*0.587f + b*0.114f; }
	float luminance() const { return r*0.212671f + g*0.71516f + b*0.072169f; }
	float average() const { return (r + g + b) / 3.0f; }
	float min() const { return nxCalc::min(r, g, b); }
	float max() const { return nxCalc::max(r, g, b); }

	void clip_neg() {
		for (int i = 0; i < 4; ++i) {
			ch[i] = nxCalc::max(ch[i], 0.0f);
		}
	}

	void scl(float s) {
		for (int i = 0; i < 4; ++i) {
			ch[i] *= s;
		}
	}

	void scl_rgb(float s) {
		r *= s;
		g *= s;
		b *= s;
	}

	void scl_rgb(float sr, float sg, float sb) {
		r *= sr;
		g *= sg;
		b *= sb;
	}

	void scl_rgb(const cxColor& c) {
		r *= c.r;
		g *= c.g;
		b *= c.b;
	}

	void add(const cxColor& c) {
		for (int i = 0; i < 4; ++i) {
			ch[i] += c.ch[i];
		}
	}

	void add_rgb(const cxColor& c) {
		r += c.r;
		g += c.g;
		b += c.b;
	}

	void to_nonlinear(float gamma = 2.2f) {
		if (gamma <= 0.0f || gamma == 1.0f) return;
		float igamma = 1.0f / gamma;
		for (int i = 0; i < 3; ++i) {
			if (ch[i] <= 0.0f) {
				ch[i] = 0.0f;
			} else {
				ch[i] = ::mth_powf(ch[i], igamma);
			}
		}
	}

	void to_linear(float gamma = 2.2f) {
		if (gamma <= 0.0f || gamma == 1.0f) return;
		for (int i = 0; i < 3; ++i) {
			if (ch[i] > 0.0f) {
				ch[i] = ::mth_powf(ch[i], gamma);
			}
		}
	}

	cxVec YCgCo() const;
	void from_YCgCo(const cxVec& ygo);
	cxVec TMI() const;
	void from_TMI(const cxVec& tmi);
	cxVec XYZ(cxMtx* pRGB2XYZ = nullptr) const;
	void from_XYZ(const cxVec& xyz, cxMtx* pXYZ2RGB = nullptr);
	cxVec xyY(cxMtx* pRGB2XYZ = nullptr) const;
	void from_xyY(const cxVec& xyY, cxMtx* pXYZ2RGB = nullptr);
	cxVec Lab(cxMtx* pRGB2XYZ = nullptr) const;
	void from_Lab(const cxVec& lab, cxMtx* pRGB2XYZ = nullptr, cxMtx* pXYZ2RGB = nullptr);
	cxVec Lch(cxMtx* pRGB2XYZ = nullptr) const;
	void from_Lch(const cxVec& lch, cxMtx* pRGB2XYZ = nullptr, cxMtx* pXYZ2RGB = nullptr);

	uint32_t encode_rgbe() const;
	void decode_rgbe(uint32_t rgbe);
	uint32_t encode_bgre() const;
	void decode_bgre(uint32_t bgre);
	uint32_t encode_rgbi() const;
	void decode_rgbi(uint32_t rgbi);
	uint32_t encode_bgri() const;
	void decode_bgri(uint32_t bgri);
	uint32_t encode_rgba8() const;
	void decode_rgba8(uint32_t rgba);
	uint32_t encode_bgra8() const;
	void decode_bgra8(uint32_t bgra);
	uint16_t encode_bgr565() const;
	void decode_bgr565(uint16_t bgr);
};

namespace nxColor {

void init_XYZ_transform(cxMtx* pRGB2XYZ, cxMtx* pXYZ2RGB, cxVec* pPrims = nullptr, cxVec* pWhite = nullptr);
void init_XYZ_transform_xy_w(cxMtx* pRGB2XYZ, cxMtx* pXYZ2RGB, float wx = 0.31271f, float wy = 0.32902f);
void init_XYZ_transform_xy(cxMtx* pRGB2XYZ, cxMtx* pXYZ2RGB, float rx = 0.64f, float ry = 0.33f, float gx = 0.3f, float gy = 0.6f, float bx = 0.15f, float by = 0.06f, float wx = 0.31271f, float wy = 0.32902f);
cxVec XYZ_to_xyY(const cxVec& xyz);
cxVec xyY_to_XYZ(const cxVec& xyY);
cxVec XYZ_to_Lab(const cxVec& xyz, cxMtx* pRGB2XYZ = nullptr);
cxVec Lab_to_XYZ(const cxVec& lab, cxMtx* pRGB2XYZ = nullptr);
cxVec Lab_to_Lch(const cxVec& lab);
cxVec Lch_to_Lab(const cxVec& lch);
float Lch_perceived_lightness(const cxVec& lch);

float approx_CMF_x31(float w);
float approx_CMF_y31(float w);
float approx_CMF_z31(float w);
float approx_CMF_x64(float w);
float approx_CMF_y64(float w);
float approx_CMF_z64(float w);

inline cxColor lerp(const cxColor& cA, const cxColor& cB, float t) {
	cxColor c;
	for (int i = 0; i < 4; ++i) {
		c.ch[i] = nxCalc::lerp(cA.ch[i], cB.ch[i], t);
	}
	return c;
}

inline cxColor lerp_rgb(const cxColor& cA, const cxColor& cB, float t) {
	cxColor c;
	for (int i = 0; i < 3; ++i) {
		c.ch[i] = nxCalc::lerp(cA.ch[i], cB.ch[i], t);
	}
	return c;
}

inline cxColor scl_rgb(const cxColor& c, const float s) {
	cxColor sc = c;
	sc.scl_rgb(s);
	return sc;
}

inline cxColor scl_rgb(const cxColor& c, const float sr, const float sg, const float sb) {
	cxColor sc = c;
	sc.scl_rgb(sr, sg, sb);
	return sc;
}

inline cxColor scl_rgb(const cxColor& c, const cxColor& s) {
	cxColor sc = c;
	sc.scl_rgb(s);
	return sc;
}

} // nxColor


struct sxToneMap {
	xt_float3 mLinWhite;
	xt_float3 mLinGain;
	xt_float3 mLinBias;

	void set_linear_white(const float val) { mLinWhite.fill(val); }
	void set_linear_white_rgb(const float r, const float g, const float b) { mLinWhite.set(r, g, b); }
	void set_linear_gain(const float val) { mLinGain.fill(val); }
	void set_linear_gain_rgb(const float r, const float g, const float b) { mLinGain.set(r, g, b); }
	void set_linear_bias(const float val) { mLinBias.fill(val); }
	void set_linear_bias_rgb(const float r, const float g, const float b) { mLinBias.set(r, g, b); }

	xt_float3 get_inv_white() const;
	cxColor apply(const cxColor& c) const;

	void reset() {
		set_linear_white(1.0f);
		set_linear_gain(1.0f);
		set_linear_bias(0.0f);
	}
};

struct sxView {
	enum class Mode {
		STANDARD = 0,
		ROT_L90 = 1,
		ROT_R90 = 2,
		ROT_180 = 3
	};

	cxFrustum mFrustum;
	cxMtx mViewMtx;
	cxMtx mProjMtx;
	cxMtx mViewProjMtx;
	cxMtx mInvViewMtx;
	cxMtx mInvProjMtx;
	cxMtx mInvViewProjMtx;
	cxVec mPos;
	cxVec mTgt;
	cxVec mUp;
	float mDegFOVY;
	int mWidth;
	int mHeight;
	float mNear;
	float mFar;
	Mode mMode;

	void set_window(const int width, const int height) {
		mWidth = width;
		mHeight = height;
	}

	void set_frame(const cxVec& pos, const cxVec& tgt, const cxVec& up = cxVec(0.0f, 1.0f, 0.0f)) {
		mPos = pos;
		mTgt = tgt;
		mUp = up;
	}

	void set_range(const float znear, const float zfar) {
		mNear = znear;
		mFar = zfar;
	}

	void set_deg_fovy(const float fovy) { mDegFOVY = fovy; }

	float get_native_aspect() const { return nxCalc::div0(float(mWidth), float(mHeight)); }
	float get_aspect() const;

	cxVec get_dir() const { return (mTgt - mPos).get_normalized(); }

	cxVec get_uv_dir(const float u, const float v) const;

	float calc_fovx() const;
	float calc_deg_fovx() const { return XD_RAD2DEG(calc_fovx()); }

	cxMtx get_mode_mtx() const;

	void init(const int width = 800, const int height = 600);
	void reset() { init(); }
	void update();

	bool ck_sphere_visibility(const cxSphere& sph, const bool exact = true) const;
	bool ck_box_visibility(const cxAABB& box, const bool exact = true) const;
};

struct sxHemisphereLight {
	xt_float3 mUpper;
	xt_float3 mLower;
	xt_float3 mUp;
	float mExp;
	float mGain;

	void set_upvec(const cxVec& v) {
		cxVec n = v.get_normalized();
		mUp.set(n.x, n.y, n.z);
	}

	void reset() {
		mUpper.set(1.1f, 1.09f, 1.12f);
		mLower.set(0.12f, 0.08f, 0.06f);
		mUp.set(0.0f, 1.0f, 0.0f);
		mExp = 1.0f;
		mGain = 1.0f;
	}

	cxColor eval(const cxVec& v) const;
};

struct sxFog {
	xt_float4 mColor;
	xt_float4 mParam;

	void set_rgb(const float r, const float g, const float b) {
		mColor.x = r;
		mColor.y = g;
		mColor.z = b;
	}

	void set_density(const float a) {
		mColor.w = nxCalc::max(a, 0.0f);
	}

	void set_range(const float start, const float end) {
		mParam.x = start;
		mParam.y = nxCalc::rcp0(end - start);
	}

	void set_curve(const float cp1, const float cp2) {
		mParam.z = cp1;
		mParam.w = cp2;
	}

	void set_linear() {
		set_curve(1.0f / 3.0f, 2.0f / 3.0f);
	}

	void reset() {
		set_rgb(1.0f, 1.0f, 1.0f);
		set_density(0.0f);
		set_range(10.0f, 1000.0f);
		set_linear();
	}
};

struct sxPrimVtx {
	xt_float4 pos;
	xt_float4 clr;
	xt_float4 tex;
	xt_float4 prm;

	void encode_normal(const cxVec& nrm);
};


struct sxCompiledExpression;

struct sxStrList {
	uint32_t mSize;
	uint32_t mNum;
	uint32_t mOffs[1];

	bool ck_idx(int idx) const { return (uint32_t)idx < mNum; }
	uint16_t* get_hash_top() const { return (uint16_t*)&mOffs[mNum]; }
	const char* get_str(int idx) const { return ck_idx(idx) ? reinterpret_cast<const char*>(this) + mOffs[idx] : nullptr; }
	bool is_sorted() const;
	int find_str(const char* pStr) const;
	int find_str_any(const char** ppStrs, int n) const;
};

struct sxVecList {
	struct Entry {
		uint32_t mOrg : 30;
		uint32_t mLen : 2;
	};

	uint32_t mSize;
	uint32_t mNum;
	Entry mTbl[1];

	bool ck_idx(int idx) const { return (uint32_t)idx < mNum; }
	const float* get_ptr(int idx) const { return ck_idx(idx) ? reinterpret_cast<const float*>(&mTbl[mNum]) + mTbl[idx].mOrg : nullptr; }
	int get_elems(float* pDst, int idx, int num) const;
	xt_float2 get_f2(int idx);
	xt_float3 get_f3(int idx);
	xt_float4 get_f4(int idx);
};

struct sxData {
	uint32_t mKind;
	uint32_t mFlags;
	uint32_t mFileSize;
	uint32_t mHeadSize;
	uint32_t mOffsStr;
	int16_t mNameId;
	int16_t mPathId;
	uint32_t mFilePathLen;
	uint32_t mOffsExt;

	struct Status {
		uint32_t fmt : 1;
		uint32_t bige : 1;
		uint32_t native : 1;
		uint32_t fixed : 1;
	};

	struct ExtExtry {
		uint32_t kind;
		uint32_t offs;
	};

	struct ExtList {
		uint32_t num;
		uint32_t reserved;
		ExtExtry lst[1];
	};

	sxStrList* get_str_list() const { return mOffsStr ? (sxStrList*)XD_INCR_PTR(this, mOffsStr) : nullptr; }
	const char* get_str(int id) const { sxStrList* pStrLst = get_str_list(); return pStrLst ? pStrLst->get_str(id) : nullptr; }
	int find_str(const char* pStr) const { return pStr && mOffsStr ? get_str_list()->find_str(pStr) : -1; }
	bool has_file_path() const { return !!mFilePathLen; }
	const char* get_file_path() const { return has_file_path() ? (const char*)XD_INCR_PTR(this, mFileSize) : nullptr; }
	const char* get_name() const { return get_str(mNameId); }
	const char* get_base_path() const { return get_str(mPathId); }
	const ExtList* get_ext_list() const { return mOffsExt ? (const ExtList*)XD_INCR_PTR(this, mOffsExt) : nullptr; }
	uint32_t find_ext_offs(const uint32_t kind) const;
	Status get_status() const;

	template<typename T> bool is() const { return mKind == T::KIND; }
	template<typename T> T* as() const { return is<T>() ? (T*)this : nullptr; }
};


struct sxValuesData : public sxData {
	uint32_t mOffsVec;
	uint32_t mOffsGrp;

	enum class eValType {
		UNKNOWN = 0,
		FLOAT = 1,
		VEC2 = 2,
		VEC3 = 3,
		VEC4 = 4,
		INT = 5,
		STRING = 6
	};

	struct ValInfo {
		int16_t mNameId;
		int8_t mType; /* ValType */
		int8_t mReserved;
		union {
			float f;
			int32_t i;
		} mValId;

		eValType get_type() const { return (eValType)mType; }
	};

	struct GrpInfo {
		int16_t mNameId;
		int16_t mPathId;
		int16_t mTypeId;
		int16_t mReserved;
		int32_t mValNum;
		ValInfo mVals[1];
	};

	struct GrpList {
		uint32_t mNum;
		uint32_t mOffs[1];
	};

	class Group {
	private:
		const sxValuesData* mpVals;
		int32_t mGrpId;

		Group() {}

		friend struct sxValuesData;

	public:
		bool is_valid() const { return mpVals && mpVals->ck_grp_idx(mGrpId); }
		bool ck_val_idx(int idx) const { return (uint32_t)idx < (uint32_t)get_val_num(); }
		const char* get_name() const { return is_valid() ? mpVals->get_str(get_info()->mNameId) : nullptr; }
		const char* get_path() const { return is_valid() ? mpVals->get_str(get_info()->mPathId) : nullptr; }
		const char* get_type() const { return is_valid() ? mpVals->get_str(get_info()->mTypeId) : nullptr; }
		const GrpInfo* get_info() const { return is_valid() ? mpVals->get_grp_info(mGrpId) : nullptr; }
		int get_val_num() const { return is_valid() ? get_info()->mValNum : 0; }
		int find_val_idx(const char* pName) const;
		int find_val_idx_any(const char** ppNames, int n) const;
		const char* get_val_name(int idx) const { return ck_val_idx(idx) ? mpVals->get_str(get_info()->mVals[idx].mNameId) : nullptr; }
		eValType get_val_type(int idx) const { return ck_val_idx(idx) ? get_info()->mVals[idx].get_type() : eValType::UNKNOWN; }
		const ValInfo* get_val_info(int idx) const { return ck_val_idx(idx) ? &get_info()->mVals[idx] : nullptr; }
		int get_val_i(int idx) const;
		float get_val_f(int idx) const;
		xt_float2 get_val_f2(int idx) const;
		xt_float3 get_val_f3(int idx) const;
		xt_float4 get_val_f4(int idx) const;
		const char* get_val_s(int idx) const;
		float get_float(const char* pName, const float defVal = 0.0f) const;
		float get_float_any(const char** ppNames, int n, const float defVal = 0.0f) const;
		int get_int(const char* pName, const int defVal = 0) const;
		int get_int_any(const char** ppNames, int n, const int defVal = 0) const;
		cxVec get_vec(const char* pName, const cxVec& defVal = cxVec(0.0f)) const;
		cxVec get_vec_any(const char** ppNames, int n, const cxVec& defVal = cxVec(0.0f)) const;
		cxColor get_rgb(const char* pName, const cxColor& defVal = cxColor(0.0f)) const;
		cxColor get_rgb_any(const char** ppNames, int n, const cxColor& defVal = cxColor(0.0f)) const;
		const char* get_str(const char* pName, const char* pDefVal = "") const;
		const char* get_str_any(const char** ppNames, int n, const char* pDefVal = "") const;
	};

	bool ck_grp_idx(int idx) const { return (uint32_t)idx < (uint32_t)get_grp_num(); }
	int find_grp_idx(const char* pName, const char* pPath = nullptr, int startIdx = 0) const;
	sxVecList* get_vec_list() const { return mOffsVec ? reinterpret_cast<sxVecList*>(XD_INCR_PTR(this, mOffsVec)) : nullptr; }
	GrpList* get_grp_list() const { return mOffsGrp ? reinterpret_cast<GrpList*>(XD_INCR_PTR(this, mOffsGrp)) : nullptr; }
	int get_grp_num() const { GrpList* pGrpLst = get_grp_list(); return pGrpLst ? pGrpLst->mNum : 0; };
	GrpInfo* get_grp_info(int idx) const { return ck_grp_idx(idx) ? reinterpret_cast<GrpInfo*>(XD_INCR_PTR(this, get_grp_list()->mOffs[idx])) : nullptr; }
	Group get_grp(int idx) const;
	Group find_grp(const char* pName, const char* pPath = nullptr) const { return get_grp(find_grp_idx(pName, pPath)); }

	static const char* get_val_type_str(eValType typ);

	static const uint32_t KIND;
};

struct sxRigData : public sxData {
	uint32_t mNodeNum;
	uint32_t mLvlNum;
	uint32_t mOffsNode;
	uint32_t mOffsWMtx;
	uint32_t mOffsIMtx;
	uint32_t mOffsLMtx;
	uint32_t mOffsLPos;
	uint32_t mOffsLRot;
	uint32_t mOffsLScl;
	uint32_t mOffsInfo;

	enum class eInfoKind {
		LIST  = XD_FOURCC('L', 'I', 'S', 'T'),
		LIMBS = XD_FOURCC('L', 'I', 'M', 'B'),
		EXPRS = XD_FOURCC('E', 'X', 'P', 'R')
	};

	enum class eLimbType {
		LEG_L,
		LEG_R,
		ARM_L,
		ARM_R
	};

	struct Node {
		int16_t mSelfIdx;
		int16_t mParentIdx;
		int16_t mNameId;
		int16_t mPathId;
		int16_t mTypeId;
		int16_t mLvl;
		uint16_t mAttr;
		uint8_t mRotOrd;
		uint8_t mXfmOrd;

		exRotOrd get_rot_order() const { return (exRotOrd)mRotOrd; }
		exTransformOrd get_xform_order() const { return (exTransformOrd)mXfmOrd; }
		bool is_hrc_top() const { return mParentIdx < 0; }
	};

	struct Info {
		uint32_t mKind;
		uint32_t mOffs;
		uint32_t mNum;
		uint32_t mParam;

		eInfoKind get_kind() const { return (eInfoKind)mKind; }
	};

	struct ExprInfo {
		int16_t mNodeId;
		int8_t mChanId;
		uint8_t mReserved;

		exAnimChan get_chan_id() const { return (exAnimChan)mChanId; }
		sxCompiledExpression* get_code() { return reinterpret_cast<sxCompiledExpression*>(this + 1); }
	};

	struct LimbInfo {
		int32_t mTopCtrl;
		int32_t mEndCtrl;
		int32_t mExtCtrl;
		int32_t mTop;
		int32_t mRot;
		int32_t mEnd;
		int32_t mExt;
		uint8_t mLimbId; /* type:4, idx:4 */
		uint8_t mAxis;
		uint8_t mUp;
		uint8_t mFlags;

		eLimbType get_type() const { return (eLimbType)(mLimbId & 0xF); }
		int get_idx() const { return (mLimbId >> 4) & 0xF; }
		exAxis get_axis() const { return (exAxis)mAxis; }
		exAxis get_up_axis() const { return (exAxis)mUp; }
		bool get_ext_comp_flg() const { return !!(mFlags & 1); }
	};

	struct LimbChain {
		class AdjustFunc {
		public:
			AdjustFunc() {}
			virtual ~AdjustFunc() {}
			virtual cxVec operator()(const sxRigData& rig, const LimbChain& chain, const cxVec& pos) { return pos; }
		};

		struct Solution {
			cxMtx mTop;
			cxMtx mRot;
			cxMtx mEnd;
			cxMtx mExt;
		};

		int mTopCtrl;
		int mEndCtrl;
		int mExtCtrl;
		int mTop;
		int mRot;
		int mEnd;
		int mExt;
		exAxis mAxis;
		exAxis mUp;
		bool mExtCompensate;

		void set(LimbInfo* pInfo);
	};

	bool ck_node_idx(int idx) const { return (uint32_t)idx < mNodeNum; }
	int get_nodes_num() const { return mNodeNum; }
	int get_levels_num() const { return mLvlNum; }
	Node* get_node_top() const { return mOffsNode ? reinterpret_cast<Node*>(XD_INCR_PTR(this, mOffsNode)) : nullptr; }
	Node* get_node_ptr(int idx) const;
	const char* get_node_name(int idx) const;
	const char* get_node_path(int idx) const;
	const char* get_node_type(int idx) const;
	int find_node(const char* pName, const char* pPath = nullptr) const;
	exRotOrd get_rot_order(int idx) const { return ck_node_idx(idx) ? get_node_ptr(idx)->get_rot_order() : exRotOrd::XYZ; }
	exTransformOrd get_xform_order(int idx) const { return ck_node_idx(idx) ? get_node_ptr(idx)->get_xform_order() : exTransformOrd::SRT; }
	int get_parent_idx(int idx) const { return ck_node_idx(idx) ? get_node_ptr(idx)->mParentIdx : -1; }
	cxMtx get_wmtx(int idx) const;
	cxMtx* get_wmtx_ptr(int idx) const;
	cxMtx get_imtx(int idx) const;
	cxMtx* get_imtx_ptr(int idx) const;
	cxMtx get_lmtx(int idx) const;
	cxMtx* get_lmtx_ptr(int idx) const;
	cxMtx calc_lmtx(int idx) const;
	cxVec calc_parent_offs(int idx) const;
	float calc_parent_dist(int idx) const { return calc_parent_offs(idx).mag(); }
	cxVec get_lpos(int idx) const;
	cxVec get_lscl(int idx) const;
	cxVec get_lrot(int idx, bool inRadians = false) const;
	cxQuat calc_lquat(int idx) const;
	cxMtx calc_wmtx(int idx, const cxMtx* pMtxLocal, cxMtx* pParentWMtx = nullptr) const;

	void calc_limb_local(LimbChain::Solution* pSolution, const LimbChain& chain, cxMtx* pMtx, LimbChain::AdjustFunc* pAdjFunc = nullptr) const;
	void copy_limb_solution(cxMtx* pDstMtx, const LimbChain& chain, const LimbChain::Solution& solution);

	bool has_info_list() const;
	Info* find_info(eInfoKind kind) const;
	LimbInfo* find_limb(eLimbType type, int idx = 0) const;

	int get_expr_num() const;
	int get_expr_stack_size() const;
	ExprInfo* get_expr_info(int idx) const;
	sxCompiledExpression* get_expr_code(int idx) const;

	static const uint32_t KIND;
};

struct sxGeometryData : public sxData {
	cxAABB mBBox;
	uint32_t mPntNum;
	uint32_t mPolNum;
	uint32_t mMtlNum;
	uint32_t mGlbAttrNum;
	uint32_t mPntAttrNum;
	uint32_t mPolAttrNum;
	uint32_t mPntGrpNum;
	uint32_t mPolGrpNum;
	uint32_t mSkinNodeNum;
	uint16_t mMaxSkinWgtNum;
	uint16_t mMaxVtxPerPol;

	uint32_t mPntOffs;
	uint32_t mPolOffs;
	uint32_t mMtlOffs;
	uint32_t mGlbAttrOffs;
	uint32_t mPntAttrOffs;
	uint32_t mPolAttrOffs;
	uint32_t mPntGrpOffs;
	uint32_t mPolGrpOffs;
	uint32_t mSkinNodesOffs;
	uint32_t mSkinOffs;
	uint32_t mBVHOffs;

	enum class eAttrClass {
		GLOBAL,
		POINT,
		POLYGON
	};

	struct AttrInfo {
		uint32_t mDataOffs;
		int32_t mNameId;
		uint16_t mElemNum;
		uint8_t mType;
		uint8_t mReserved;

		bool is_int() const { return mType == 0; }
		bool is_float() const { return mType == 1; }
		bool is_string() const { return mType == 2; }
		bool is_float2() const { return is_float() && mElemNum == 2; }
		bool is_float3() const { return is_float() && mElemNum == 3; }
	};

	class Polygon {
	private:
		const sxGeometryData* mpGeom;
		int32_t mPolId;

		Polygon() {}

		friend struct sxGeometryData;

		uint8_t* get_vtx_lst() const;
		int get_mtl_id_size() const { return mpGeom->get_mtl_num() < (1 << 7) ? 1 : 2; }

	public:
		const sxGeometryData* get_geo() const { return mpGeom; }
		int get_id() const { return mPolId; }
		bool is_valid() const { return (mpGeom && mpGeom->ck_pol_idx(mPolId)); }
		bool ck_vtx_idx(int idx) const { return (uint32_t)idx < (uint32_t)get_vtx_num(); }
		int get_vtx_pnt_id(int vtxIdx) const;
		int get_vtx_num() const;
		int get_mtl_id() const;
		cxVec get_vtx_pos(int vtxIdx) const { return is_valid() ? mpGeom->get_pnt(get_vtx_pnt_id(vtxIdx)) : cxVec(0.0f); };
		cxVec calc_centroid() const;
		cxAABB calc_bbox() const;
		cxVec calc_normal_cw() const;
		cxVec calc_normal_ccw() const;
		cxVec calc_normal() const { return calc_normal_cw(); }
		bool is_tri() const { return get_vtx_num() == 3; }
		bool is_quad() const { return get_vtx_num() == 4; }
		bool is_planar(float eps = 1.0e-6f) const;
		bool intersect(const cxLineSeg& seg, cxVec* pHitPos = nullptr, cxVec* pHitNrm = nullptr) const;
		bool contains_xz(const cxVec& pos) const;
		int triangulate(int* pVtxLst, void* pWk = nullptr) const;
		int tri_count() const;
	};

	struct GrpInfo {
		int16_t mNameId;
		int16_t mPathId;
		cxAABB mBBox;
		int32_t mMinIdx;
		int32_t mMaxIdx;
		uint16_t mMaxWgtNum;
		uint16_t mSkinNodeNum;
		uint32_t mIdxNum;
	};

	class Group {
	private:
		const sxGeometryData* mpGeom;
		GrpInfo* mpInfo;

		Group() {}

		friend struct sxGeometryData;

		void* get_idx_top() const;
		int get_idx_elem_size() const;

	public:
		bool is_valid() const { return (mpGeom && mpInfo); }
		const char* get_name() const { return is_valid() ? mpGeom->get_str(mpInfo->mNameId) : nullptr; }
		const char* get_path() const { return is_valid() ? mpGeom->get_str(mpInfo->mPathId) : nullptr; }
		GrpInfo* get_info() const { return mpInfo; }
		cxAABB get_bbox() const {
			if (is_valid()) {
				return mpInfo->mBBox;
			} else {
				cxAABB bbox;
				bbox.init();
				return bbox;
			}
		}
		int get_max_wgt_num() const { return is_valid() ? mpInfo->mMaxWgtNum : 0; }
		bool ck_idx(int at) const { return is_valid() ? (uint32_t)at < mpInfo->mIdxNum : false; }
		uint32_t get_idx_num() const { return is_valid() ? mpInfo->mIdxNum : 0; }
		int get_min_idx() const { return is_valid() ? mpInfo->mMinIdx : -1; }
		int get_max_idx() const { return is_valid() ? mpInfo->mMaxIdx : -1; }
		int get_rel_idx(int at) const;
		int get_idx(int at) const;
		bool contains(int idx) const;
		int get_skin_nodes_num() const { return is_valid() ? mpInfo->mSkinNodeNum : 0; }
		cxSphere* get_skin_spheres() const;
		uint16_t* get_skin_ids() const;
	};

	class HitFunc {
	public:
		HitFunc() {}
		virtual ~HitFunc() {}
		virtual bool operator()(const Polygon& pol, const cxVec& hitPos, const cxVec& hitNrm, float hitDist) { return false; }
	};

	class RangeFunc {
	public:
		RangeFunc() {}
		virtual ~RangeFunc() {}
		virtual bool operator()(const Polygon& pol) { return false; }
	};

	struct BVH {
		struct Node {
			cxAABB mBBox;
			int32_t mLeft;
			int32_t mRight;

			bool is_leaf() const { return mRight < 0; }
			int get_pol_id() const { return mLeft; }
		};

		uint32_t mNodesNum;
		uint32_t mReserved0;
		uint32_t mReserved1;
		uint32_t mReserved2;
	};

	class LeafFunc {
	public:
		LeafFunc() {}
		virtual ~LeafFunc() {}
		virtual bool operator()(const Polygon& pol, const BVH::Node* pNode) { return false; }
	};

	class DisplayList {
	public:
		struct Batch {
			int32_t mMtlId;
			int32_t mGrpId; /* -1 for material groups */
			int32_t mMinIdx;
			int32_t mMaxIdx;
			int32_t mIdxOrg;
			int32_t mTriNum;
			int32_t mMaxWgtNum;
			int32_t mSkinNodesNum;

			bool is_idx16() const { return (mMaxIdx - mMinIdx) < (1 << 16); }
		};

	private:
		struct Data {
			uint32_t mOffsBat;
			uint32_t mOffsIdx32;
			uint32_t mOffsIdx16;
			uint32_t mMtlNum;
			uint32_t mBatNum;
			uint32_t mTriNum;
			uint32_t mIdx16Num;
			uint32_t mIdx32Num;

			Batch* get_bat_top() { return mOffsBat ? reinterpret_cast<Batch*>(XD_INCR_PTR(this, mOffsBat)) : nullptr; }
			uint16_t* get_idx16() { return mOffsIdx16 ? reinterpret_cast<uint16_t*>(XD_INCR_PTR(this, mOffsIdx16)) : nullptr; }
			uint32_t* get_idx32() { return mOffsIdx32 ? reinterpret_cast<uint32_t*>(XD_INCR_PTR(this, mOffsIdx32)) : nullptr; }
		};

		const sxGeometryData* mpGeo;
		Data* mpData;

		DisplayList() : mpGeo(nullptr), mpData(nullptr) {}

		void create(const sxGeometryData& geo, const char* pBatGrpPrefix = nullptr, bool triangulate = true);

		friend struct sxGeometryData;

	public:
		~DisplayList() { destroy(); }

		void destroy();

		bool is_valid() const { return mpGeo != nullptr && mpData != nullptr; }

		int get_mtl_num() const { return mpData ? mpData->mMtlNum : 0; }
		int get_bat_num() const { return mpData ? mpData->mBatNum : 0; }
		int get_tri_num() const { return mpData ? mpData->mTriNum : 0; }

		int get_idx16_num() const { return mpData ? mpData->mIdx16Num : 0; }
		uint16_t* get_idx16_buf() { return mpData ? mpData->get_idx16() : nullptr; }
		int get_idx32_num() const { return mpData ? mpData->mIdx32Num : 0; }
		uint32_t* get_idx32_buf() { return mpData ? mpData->get_idx32() : nullptr; }

		bool bat_idx_ck(int idx) const { return mpData ? (uint32_t)idx < mpData->mBatNum : false; }
		Batch* get_bat(int idx) const { return bat_idx_ck(idx) ? mpData->get_bat_top() + idx : nullptr; }
		const char* get_bat_mtl_name(int batIdx) const;
	};

	int get_pnt_num() const { return mPntNum; }
	int get_pol_num() const { return mPolNum; }
	int get_mtl_num() const { return mMtlNum; }
	int get_pnt_grp_num() const { return mPntGrpNum; }
	int get_pol_grp_num() const { return mPolGrpNum; }
	bool is_same_pol_size() const { return !!(mFlags & 1); }
	bool is_same_pol_mtl() const { return !!(mFlags & 2); }
	bool has_skin_spheres() const { return !!(mFlags & 4); }
	bool all_quads_planar_convex() const { return !!(mFlags & 8); }
	bool is_all_tris() const { return is_same_pol_size() && mMaxVtxPerPol == 3; }
	bool has_skin() const { return mSkinOffs != 0; }
	bool has_skin_nodes() const { return mSkinNodesOffs != 0; }
	bool has_BVH() const { return mBVHOffs != 0; }
	bool ck_BVH_node_idx(int idx) const { return has_BVH() ? (uint32_t)idx < get_BVH()->mNodesNum : false; }
	bool ck_pnt_idx(int idx) const { return (uint32_t)idx < mPntNum; }
	bool ck_pol_idx(int idx) const { return (uint32_t)idx < mPolNum; }
	bool ck_mtl_idx(int idx) const { return (uint32_t)idx < mMtlNum; }
	bool ck_pnt_grp_idx(int idx) const { return (uint32_t)idx < mPntGrpNum; }
	bool ck_pol_grp_idx(int idx) const { return (uint32_t)idx < mPolGrpNum; }
	bool ck_skin_idx(int idx) const { return (uint32_t)idx < mSkinNodeNum; }
	int get_vtx_idx_size() const;
	cxVec* get_pnt_top() const { return mPntOffs ? reinterpret_cast<cxVec*>(XD_INCR_PTR(this, mPntOffs)) : nullptr; }
	cxVec get_pnt(int idx) const { return ck_pnt_idx(idx) ? get_pnt_top()[idx] : cxVec(0.0f); }
	Polygon get_pol(int idx) const;
	cxSphere* get_skin_sph_top() const { return mSkinNodesOffs ? reinterpret_cast<cxSphere*>(XD_INCR_PTR(this, mSkinNodesOffs)) : nullptr; }
	int get_skin_nodes_num() const { return mSkinNodeNum; }
	int32_t* get_skin_node_name_ids() const;
	const char* get_skin_node_name(int idx) const;
	int find_skin_node(const char* pName) const;
	uint32_t get_attr_info_offs(eAttrClass cls) const;
	uint32_t get_attr_info_num(eAttrClass cls) const;
	uint32_t get_attr_item_num(eAttrClass cls) const;
	int find_attr(const char* pName, eAttrClass cls) const;
	int find_glb_attr(const char* pName) const { return find_attr(pName, eAttrClass::GLOBAL); }
	int find_pnt_attr(const char* pName) const { return find_attr(pName, eAttrClass::POINT); }
	int find_pol_attr(const char* pName) const { return find_attr(pName, eAttrClass::POLYGON); }
	bool has_glb_attr(const char* pName) const { return find_glb_attr(pName) >= 0; }
	bool has_pnt_attr(const char* pName) const { return find_pnt_attr(pName) >= 0; }
	bool has_pol_attr(const char* pName) const { return find_pol_attr(pName) >= 0; }
	AttrInfo* get_attr_info(int attrIdx, eAttrClass cls) const;
	AttrInfo* get_glb_attr_info(int attrIdx) const { return get_attr_info(attrIdx, eAttrClass::GLOBAL); }
	AttrInfo* get_pnt_attr_info(int attrIdx) const { return get_attr_info(attrIdx, eAttrClass::POINT); }
	AttrInfo* get_pol_attr_info(int attrIdx) const { return get_attr_info(attrIdx, eAttrClass::POLYGON); }
	const char* get_attr_val_s(int attrIdx, eAttrClass cls, int itemIdx) const;
	float* get_attr_data_f(int attrIdx, eAttrClass cls, int itemIdx, int minElem = 1) const;
	float* get_glb_attr_data_f(int attrIdx, int minElem = 1) const { return get_attr_data_f(attrIdx, eAttrClass::GLOBAL, 0, minElem); }
	float* get_pnt_attr_data_f(int attrIdx, int itemIdx, int minElem = 1) const { return get_attr_data_f(attrIdx, eAttrClass::POINT, itemIdx, minElem); }
	float* get_pol_attr_data_f(int attrIdx, int itemIdx, int minElem = 1) const { return get_attr_data_f(attrIdx, eAttrClass::POLYGON, itemIdx, minElem); }
	float get_pnt_attr_val_f(int attrIdx, int pntIdx) const;
	xt_float3 get_pnt_attr_val_f3(int attrIdx, int pntIdx) const;
	cxVec get_pnt_normal(int pntIdx) const;
	cxVec get_pnt_tangent(int pntIdx) const;
	cxVec get_pnt_bitangent(int pntIdx) const;
	cxVec calc_pnt_bitangent(int pntIdx) const;
	cxColor get_pnt_color(int pntIdx, bool useAlpha = true) const;
	xt_texcoord get_pnt_texcoord(int pntIdx) const;
	xt_texcoord get_pnt_texcoord2(int pntIdx) const;
	int get_pnt_wgt_num(int pntIdx) const;
	int get_pnt_skin_jnt(int pntIdx, int wgtIdx) const;
	float get_pnt_skin_wgt(int pntIdx, int wgtIdx) const;
	int find_pnt_grp_idx(const char* pName, const char* pPath = nullptr) const;
	int find_pol_grp_idx(const char* pName, const char* pPath = nullptr) const;
	int find_mtl_grp_idx(const char* pName, const char* pPath = nullptr) const;
	Group find_pnt_grp(const char* pName, const char* pPath = nullptr) const { return get_pnt_grp(find_pnt_grp_idx(pName, pPath)); }
	Group find_pol_grp(const char* pName, const char* pPath = nullptr) const { return get_pol_grp(find_pol_grp_idx(pName, pPath)); }
	Group find_mtl_grp(const char* pName, const char* pPath = nullptr) const { return get_mtl_grp(find_mtl_grp_idx(pName, pPath)); }
	GrpInfo* get_mtl_info(int idx) const;
	Group get_mtl_grp(int idx) const;
	const char* get_mtl_name(int idx) const;
	const char* get_mtl_path(int idx) const;
	GrpInfo* get_pnt_grp_info(int idx) const;
	Group get_pnt_grp(int idx) const;
	GrpInfo* get_pol_grp_info(int idx) const;
	Group get_pol_grp(int idx) const;
	void hit_query_nobvh(const cxLineSeg& seg, HitFunc& fun) const;
	void hit_query(const cxLineSeg& seg, HitFunc& fun) const;
	void range_query_nobvh(const cxAABB& box, RangeFunc& fun) const;
	void range_query(const cxAABB& box, RangeFunc& fun) const;
	void leaf_hit_query(const cxLineSeg& seg, LeafFunc& fun) const;
	xt_int2 find_leaf_level_range() const;
	int find_min_leaf_level() const;
	int find_max_leaf_level() const;
	BVH* get_BVH() const { return has_BVH() ? reinterpret_cast<BVH*>(XD_INCR_PTR(this, mBVHOffs)) : nullptr; }
	BVH::Node* get_BVH_node(int nodeId) const { return ck_BVH_node_idx(nodeId) ? &reinterpret_cast<BVH::Node*>(get_BVH() + 1)[nodeId] : nullptr; }
	cxAABB calc_world_bbox(cxMtx* pMtxW, int* pIdxMap = nullptr) const;
	void calc_tangents(cxVec* pTng, bool flip = false, const char* pAttrName = nullptr) const;
	cxVec* calc_tangents(bool flip = false, const char* pAttrName = nullptr) const;
	void* alloc_triangulation_wk() const;

	static const uint32_t KIND;
};

struct sxDDSHead {
	uint32_t mMagic;
	uint32_t mSize;
	uint32_t mFlags;
	uint32_t mHeight;
	uint32_t mWidth;
	uint32_t mPitchLin;
	uint32_t mDepth;
	uint32_t mMipMapCount;
	uint32_t mReserved1[11];
	struct PixelFormat {
		uint32_t mSize;
		uint32_t mFlags;
		uint32_t mFourCC;
		uint32_t mBitCount;
		uint32_t mMaskR;
		uint32_t mMaskG;
		uint32_t mMaskB;
		uint32_t mMaskA;
	} mFormat;
	uint32_t mCaps1;
	uint32_t mCaps2;
	uint32_t mReserved2[3];

	bool is_dds128() const { return mFormat.mFourCC == 0x74; }
	bool is_dds64() const { return mFormat.mFourCC == 0x71; }
	bool is_dds32() const { return mFormat.mFourCC == 0 && mFormat.mBitCount == 32; }

	void set_encoding(uint32_t code) { mReserved1[1] = code; }
	uint32_t get_encoding() const { return mReserved1[1]; }

	void set_gamma(float gamma) {
		float* pGamma = reinterpret_cast<float*>(&mReserved1[2]);
		*pGamma = gamma;
	}

	float get_gamma() const {
		const float* pGamma = reinterpret_cast<const float*>(&mReserved1[2]);
		return *pGamma;
	}

	void init(uint32_t w, uint32_t h);
	void init_dds128(uint32_t w, uint32_t h);
	void init_dds64(uint32_t w, uint32_t h);
	void init_dds32(uint32_t w, uint32_t h, bool encRGB = true);

	static const uint32_t ENC_RGBE;
	static const uint32_t ENC_BGRE;
	static const uint32_t ENC_RGBI;
	static const uint32_t ENC_BGRI;
};

namespace nxImage {

sxDDSHead* alloc_dds128(uint32_t w, uint32_t h, uint32_t* pSize = nullptr);
void save_dds128(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h);
void save_dds64(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h);
void save_dds32_rgbe(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h);
void save_dds32_bgre(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h);
void save_dds32_rgbi(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h);
void save_dds32_bgri(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h);
void save_dds32_rgba8(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h, float gamma = 2.2f);
void save_dds32_bgra8(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h, float gamma = 2.2f);

cxColor* decode_dds(sxDDSHead* pDDS, uint32_t* pWidth, uint32_t* pHeight, float gamma = 2.2f);

void save_sgi(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h, float gamma = 2.2f);
void save_hdr(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h, float gamma = 1.0f, float exposure = 1.0f);

void calc_resample_wgts(int oldRes, int newRes, xt_float4* pWgt, int16_t* pOrg);
cxColor* upscale(const cxColor* pSrc, int srcW, int srcH, int xscl, int yscl, bool filt = true, cxColor* pDstBuff = nullptr, void* pWrkMem = nullptr, bool cneg = true);
size_t calc_upscale_work_size(int srcW, int srcH, int xscl, int yscl);

} // nxImage

struct sxImageData : public sxData {
	uint32_t mWidth;
	uint32_t mHeight;
	uint32_t mPlaneNum;
	uint32_t mPlaneOffs;

	struct PlaneInfo {
		uint32_t mDataOffs;
		int16_t mNameId;
		uint8_t mTrailingZeroes;
		int8_t mFormat;
		float mMinVal;
		float mMaxVal;
		float mValOffs;
		uint32_t mBitCount;
		uint32_t mReserved0;
		uint32_t mReserved1;

		bool is_const() const { return mFormat == 0; }
		bool is_compressed() const { return mFormat == 1; }
	};

	class Plane {
	private:
		const sxImageData* mpImg;
		int32_t mPlaneId;

		Plane() {}

		void expand(float* pDst, int pixelStride) const;

		friend struct sxImageData;
	public:
		bool is_valid() const { return mPlaneId >= 0; }
		void get_data(float* pDst, int pixelStride = 1) const;
		float* get_data() const;
	};

	class DDS {
	private:
		sxDDSHead* mpHead;
		uint32_t mSize;

		friend struct sxImageData;

	public:
		bool is_valid() const { return mpHead && mSize; }
		void free();
		sxDDSHead* get_data_ptr() const { return mpHead; }
		uint32_t get_data_size() const { return mSize; }
		int get_width() const { return is_valid() ? mpHead->mWidth : 0; }
		int get_height() const { return is_valid() ? mpHead->mHeight : 0; }
		void save(const char* pOutPath) const;
	};

	struct Pyramid {
		int32_t mBaseWidth;
		int32_t mBaseHeight;
		int32_t mLvlNum;
		int32_t mLvlOffs[1];

		bool ck_lvl_idx(int idx) const { return (uint32_t)idx < (uint32_t)mLvlNum; }
		cxColor* get_lvl(int idx) const { return ck_lvl_idx(idx) ? reinterpret_cast<cxColor*>(XD_INCR_PTR(this, mLvlOffs[idx])) : nullptr; }
		void get_lvl_dims(int idx, int* pWidth, int* pHeight) const;
		DDS get_lvl_dds(int idx) const;
	};

	bool ck_plane_idx(int idx) const { return (uint32_t)idx < mPlaneNum; }
	int get_width() const { return mWidth; }
	int get_height() const { return mHeight; }
	int calc_mip_num() const;
	int find_plane_idx(const char* pName) const;
	Plane find_plane(const char* pName) const;
	Plane get_plane(int idx) const;
	PlaneInfo* get_plane_info(int idx) const { return ck_plane_idx(idx) ? reinterpret_cast<PlaneInfo*>(XD_INCR_PTR(this, mPlaneOffs)) + idx : nullptr; }
	PlaneInfo* find_plane_info(const char* pName) const { return get_plane_info(find_plane_idx(pName)); }
	bool is_plane_hdr(int idx) const;
	bool is_hdr() const;
	void get_rgba(float* pDst) const;
	cxColor* get_rgba() const;
	DDS get_dds() const;
	Pyramid* get_pyramid() const;

	static const uint32_t KIND;
};


struct sxKeyframesData : public sxData {
	float mFPS;
	int32_t mMinFrame;
	int32_t mMaxFrame;
	uint32_t mFCurveNum;
	uint32_t mFCurveOffs;
	uint32_t mNodeInfoNum;
	uint32_t mNodeInfoOffs;

	enum class eFunc {
		CONSTANT = 0,
		LINEAR = 1,
		CUBIC = 2
	};

	struct NodeInfo {
		int16_t mPathId;
		int16_t mNameId;
		int16_t mTypeId;
		uint8_t mRotOrd;
		uint8_t mXfmOrd;

		exRotOrd get_rot_order() const { return (exRotOrd)mRotOrd; }
		exTransformOrd get_xform_order() const { return (exTransformOrd)mXfmOrd; }
	};

	struct FCurveInfo {
		int16_t mNodePathId;
		int16_t mNodeNameId;
		int16_t mChanNameId;
		uint16_t mKeyNum;
		float mMinVal;
		float mMaxVal;
		uint32_t mValOffs;
		uint32_t mLSlopeOffs;
		uint32_t mRSlopeOffs;
		uint32_t mFnoOffs;
		uint32_t mFuncOffs;
		int8_t mCmnFunc;
		uint8_t mReserved8;
		uint16_t mReserved16;
		uint32_t mReserved32[2];

		bool is_const() const { return mKeyNum == 0; }
		bool has_fno_lst() const { return mFnoOffs != 0; }
		eFunc get_common_func() const { return mCmnFunc < 0 ? eFunc::LINEAR : (eFunc)mCmnFunc; }
	};

	class FCurve {
	private:
		const sxKeyframesData* mpKfr;
		int32_t mFcvId;

		FCurve() {}

		friend struct sxKeyframesData;

	public:
		bool is_valid() const { return mpKfr && mpKfr->ck_fcv_idx(mFcvId); }
		FCurveInfo* get_info() const { return is_valid() ? mpKfr->get_fcv_info(mFcvId) : nullptr; }
		const char* get_node_name() const { return is_valid() ? mpKfr->get_fcv_node_name(mFcvId) : nullptr; }
		const char* get_node_path() const { return is_valid() ? mpKfr->get_fcv_node_path(mFcvId) : nullptr; }
		const char* get_chan_name() const { return is_valid() ? mpKfr->get_fcv_chan_name(mFcvId) : nullptr; }
		bool is_const() const { return is_valid() ? get_info()->is_const() : true; }
		int get_key_num() const { return is_valid() ? get_info()->mKeyNum : 0; }
		bool ck_key_idx(int fno) const { return is_valid() ? (uint32_t)fno < (uint32_t)get_key_num() : false; }
		int find_key_idx(int fno) const;
		int get_fno(int idx) const;
		float eval(float frm, bool extrapolate = false) const;
	};

	struct RigLink {
		struct Val {
			xt_float3 f3;
			int32_t fcvId[3];

			cxVec get_vec() const { return cxVec(f3.x, f3.y, f3.z); }
			void set_vec(const cxVec& v) { f3.set(v.x, v.y, v.z); }
		};

		struct Node {
			int16_t mRigNodeId;
			int16_t mKfrNodeId;
			uint32_t mPosValOffs;
			uint32_t mRotValOffs;
			uint32_t mSclValOffs;
			exRotOrd mRotOrd;
			exTransformOrd mXformOrd;
			bool mUseSlerp;

			Val* get_pos_val() { return mPosValOffs ? (Val*)XD_INCR_PTR(this, mPosValOffs) : nullptr; }
			Val* get_rot_val() { return mRotValOffs ? (Val*)XD_INCR_PTR(this, mRotValOffs) : nullptr; }
			Val* get_scl_val() { return mSclValOffs ? (Val*)XD_INCR_PTR(this, mSclValOffs) : nullptr; }

			float get_pos_chan(int idx);
			float get_rot_chan(int idx);
			float get_scl_chan(int idx);
			float get_anim_chan(exAnimChan chan);
			bool ck_pos_chan(int idx);
			bool ck_rot_chan(int idx);
			bool ck_scl_chan(int idx);
			bool ck_anim_chan(exAnimChan chan);
		};

		int32_t mNodeNum;
		int32_t mRigNodeNum;
		uint32_t mRigMapOffs;
		Node mNodes[1];

		int16_t* get_rig_map() const { return mRigMapOffs ? (int16_t*)XD_INCR_PTR(this, mRigMapOffs) : nullptr; }
		bool ck_node_idx(int idx) const { return (uint32_t)idx < (uint32_t)mNodeNum; }
	};

	bool has_node_info() const;
	bool ck_node_info_idx(int idx) const { return has_node_info() && ((uint32_t)idx < mNodeInfoNum); }
	int get_node_info_num() const { return has_node_info() ? mNodeInfoNum : 0; }
	int find_node_info_idx(const char* pName, const char* pPath = nullptr, int startIdx = 0) const;
	NodeInfo* get_node_info_ptr(int idx) const { return ck_node_info_idx(idx) ? reinterpret_cast<NodeInfo*>(XD_INCR_PTR(this, mNodeInfoOffs)) + idx : nullptr; }
	const char* get_node_name(int idx) const;

	float get_frame_rate() const { return mFPS; }
	int get_frame_count() const { return get_max_fno() + 1; }
	int get_max_fno() const { return mMaxFrame - mMinFrame; }
	int get_rel_fno(int fno) const { return (fno - mMinFrame) % get_frame_count(); }
	bool ck_fno(int fno) const { return (uint32_t)fno <= (uint32_t)get_max_fno(); }
	bool ck_fcv_idx(int idx) const { return (uint32_t)idx < mFCurveNum; }
	int find_fcv_idx(const char* pNodeName, const char* pChanName, const char* pNodePath = nullptr) const;
	int get_fcv_num() const { return mFCurveNum; }
	FCurveInfo* get_fcv_top() const { return mFCurveOffs ? reinterpret_cast<FCurveInfo*>XD_INCR_PTR(this, mFCurveOffs) : nullptr; }
	FCurveInfo* get_fcv_info(int idx) const { return mFCurveOffs && (ck_fcv_idx(idx)) ? get_fcv_top() + idx : nullptr; }
	FCurve get_fcv(int idx) const;
	FCurve find_fcv(const char* pNodeName, const char* pChanName, const char* pNodePath = nullptr) const { return get_fcv(find_fcv_idx(pNodeName, pChanName, pNodePath)); }
	const char* get_fcv_node_name(int idx) const { return ck_fcv_idx(idx) ? get_str(get_fcv_info(idx)->mNodeNameId) : nullptr; }
	const char* get_fcv_node_path(int idx) const { return ck_fcv_idx(idx) ? get_str(get_fcv_info(idx)->mNodePathId) : nullptr; }
	const char* get_fcv_chan_name(int idx) const { return ck_fcv_idx(idx) ? get_str(get_fcv_info(idx)->mChanNameId) : nullptr; }
	RigLink* make_rig_link(const sxRigData& rig) const;
	void eval_rig_link_node(RigLink* pLink, int nodeIdx, float frm, const sxRigData* pRig = nullptr, cxMtx* pRigLocalMtx = nullptr) const;
	void eval_rig_link(RigLink* pLink, float frm, const sxRigData* pRig = nullptr, cxMtx* pRigLocalMtx = nullptr) const;

	void dump_clip(FILE* pOut) const;
	void dump_clip(const char* pOutPath = nullptr) const;

	static const uint32_t KIND;
};

struct sxCompiledExpression {
	uint32_t mSig;
	uint32_t mLen;
	uint32_t mValsNum;
	uint32_t mCodeNum;
	uint32_t mStrsNum;

	enum class eOp : uint8_t {
		NOP = 0,
		END = 1,
		NUM = 2,
		STR = 3,
		VAR = 4,
		CMP = 5,
		ADD = 6,
		SUB = 7,
		MUL = 8,
		DIV = 9,
		MOD = 10,
		NEG = 11,
		FUN = 12,
		XOR = 13,
		AND = 14,
		OR = 15
	};

	enum class eCmp : uint8_t {
		EQ = 0,
		NE = 1,
		LT = 2,
		LE = 3,
		GT = 4,
		GE = 5
	};

	struct Code {
		uint8_t mOp;
		int8_t mPrio;
		int16_t mInfo;

		eOp get_op() const { return (eOp)mOp; }
	};

	struct StrInfo {
		uint32_t mHash;
		uint16_t mOffs;
		uint16_t mLen;
	};

	struct String {
		const char* mpChars;
		uint32_t mHash;
		uint32_t mLen;

		bool is_valid() const { return mpChars != nullptr; }
		bool eq(const char* pStr) const;
		bool starts_with(const char* pStr) const;
	};

	template<typename T> struct TagsT {
		T* mpBitAry;
		int mNum;

		TagsT() : mpBitAry(nullptr), mNum(0) {}

		void init(void* pBits, int n) {
			mpBitAry = (T*)pBits;
			mNum = n;
		}

		void reset() {
			mpBitAry = nullptr;
			mNum = 0;
		}

		bool ck_idx(int idx) const { return (uint32_t)idx < mNum; }
		void set_num(int idx) { XD_BIT_ARY_CL(T, mpBitAry, idx); }
		bool is_num(int idx) const { return !XD_BIT_ARY_CK(T, mpBitAry, idx); }
		void set_str(int idx) { XD_BIT_ARY_ST(T, mpBitAry, idx); }
		bool is_str(int idx) const { return XD_BIT_ARY_CK(T, mpBitAry, idx); }

		static int calc_mem_size(int n) { return XD_BIT_ARY_SIZE(T, n); }
	};

	typedef TagsT<uint8_t> Tags;

	class Stack {
	public:
		union Val {
			float num;
			int32_t sptr;
		};
	protected:
		void* mpMem;
		Val* mpVals;
		Tags mTags;
		int mPtr;

	public:
		Stack() : mpMem(nullptr), mpVals(nullptr), mPtr(0) {}

		void alloc(int n);
		void free();

		int size() const { return mTags.mNum; }
		void clear() { mPtr = 0; }
		bool is_empty() const { return mPtr == 0; }
		bool is_full() const { return mPtr >= size(); }
		void push_num(float num);
		float pop_num();
		void push_str(int ptr);
		int pop_str();
	};

	class ExecIfc {
	public:
		virtual Stack* get_stack() { return nullptr; }
		virtual void set_result(const float val) {}
		virtual float ch(const String& path) { return 0.0f; }
		virtual float detail(const String& path, const String& attrName, int idx) { return 0.0f; }
		virtual float var(const String& name) { return 0.0f; }
	};

	bool is_valid() const { return mSig == XD_FOURCC('C', 'E', 'X', 'P') && mLen > 0; }
	const float* get_vals_top() const { return reinterpret_cast<const float*>(this + 1); }
	bool ck_val_idx(int idx) const { return (uint32_t)idx < mValsNum; }
	float get_val(int idx) const { return ck_val_idx(idx) ? get_vals_top()[idx] : 0.0f; }
	const Code* get_code_top() const { return reinterpret_cast<const Code*>(&get_vals_top()[mValsNum]); }
	bool ck_str_idx(int idx) const { return (uint32_t)idx < mStrsNum; }
	const StrInfo* get_str_info_top() const { return reinterpret_cast<const StrInfo*>(&get_code_top()[mCodeNum]); }
	const StrInfo* get_str_info(int idx) const { return ck_str_idx(idx) ? get_str_info_top() + idx : nullptr; }
	void get_str(int idx, String* pStr) const;
	String get_str(int idx) const;
	void exec(ExecIfc& ifc) const;
	void disasm(FILE* pFile = stdout) const;
};

struct sxExprLibData : public sxData {
	uint32_t mExprNum;
	uint32_t mListOffs;

	struct ExprInfo {
		int16_t mNodeNameId;
		int16_t mNodePathId;
		int16_t mChanNameId;
		int16_t mReserved;

		const sxCompiledExpression* get_compiled_expr() const { return reinterpret_cast<const sxCompiledExpression*>(this + 1); }
	};

	class Entry {
	private:
		const sxExprLibData* mpLib;
		int32_t mExprId;

		Entry() {}

		friend struct sxExprLibData;

	public:
		bool is_valid() const { return mpLib && mpLib->ck_expr_idx(mExprId); }
		const ExprInfo* get_info() const { return is_valid() ? mpLib->get_info(mExprId) : nullptr; }
		const sxCompiledExpression* get_expr() const { return is_valid() ? mpLib->get_info(mExprId)->get_compiled_expr() : nullptr; }
		const char* get_node_name() const { return is_valid() ? mpLib->get_str(get_info()->mNodeNameId) : nullptr; }
		const char* get_node_path() const { return is_valid() ? mpLib->get_str(get_info()->mNodePathId) : nullptr; }
		const char* get_chan_name() const { return is_valid() ? mpLib->get_str(get_info()->mChanNameId) : nullptr; }
	};

	bool ck_expr_idx(int idx) const { return (uint32_t)idx < mExprNum; }
	int get_expr_num() const { return mExprNum; }
	int find_expr_idx(const char* pNodeName, const char* pChanName, const char* pNodePath = nullptr, int startIdx = 0) const;
	const ExprInfo* get_info(int idx) const { return ck_expr_idx(idx) ? (const ExprInfo*)XD_INCR_PTR(this, ((uint32_t*)XD_INCR_PTR(this, mListOffs))[idx]) : nullptr; }
	Entry get_entry(int idx) const;
	int count_rig_entries(const sxRigData& rig) const;

	static const uint32_t KIND;
};

struct sxModelData : public sxData {
	cxAABB mBBox;
	uint32_t mPntNum;
	uint32_t mTriNum;
	uint32_t mMtlNum;
	uint32_t mTexNum;
	uint32_t mBatNum;
	uint32_t mSknNum;
	uint32_t mSklNum;
	uint32_t mIdx16Num;
	uint32_t mIdx32Num;
	uint32_t mMaxJntsPerBat;

	uint32_t mPntOffs;
	uint32_t mMtlOffs;
	uint32_t mTexOffs;
	uint32_t mBatOffs;
	uint32_t mSknOffs;
	uint32_t mSklOffs;
	uint32_t mIdx16Offs;
	uint32_t mIdx32Offs;
	uint8_t mGPUWk[32];

	struct VtxRigidHalf {
		xt_float3 pos;
		xt_half2 oct;
		xt_half2 tex;
		xt_half4 clr;

		cxVec get_normal() const {
			return nxVec::decode_octa(oct);
		}
	};

	struct VtxSkinHalf {
		xt_float3 pos;
		xt_half2 oct;
		xt_half2 tex;
		xt_half4 clr;
		uint16_t wgt[4];
		uint8_t jnt[4];

		cxVec get_normal() const {
			return nxVec::decode_octa(oct);
		}
	};

	struct VtxRigidShort {
		xt_float3 pos;
		int16_t oct[2];
		uint16_t clr[4];
		xt_float2 tex;

		cxVec get_normal() const {
			return nxVec::decode_octa(oct);
		}
	};

	struct VtxSkinShort {
		uint16_t qpos[3];
		uint16_t w0;
		uint16_t oct[2];
		uint16_t w1;
		uint16_t w2;
		uint16_t clr[4];
		int16_t tex[2];
		uint8_t jnt[4];

		cxVec get_normal() const {
			return nxVec::decode_octa(oct);
		}
	};

	struct PntSkin {
		float wgt[4];
		int idx[4];
		int num;
	};

	struct TexInfo {
		int32_t mNameId;
		int32_t mPathId;
		int32_t mWk[2];

		template<typename T> T* get_wk() { return reinterpret_cast<T*>(mWk); }
		template<typename T> const T* get_wk() const { return reinterpret_cast<const T*>(mWk); }
	};

	struct Material {
		enum SortMode {
			SORT_NEAR = 0,
			SORT_CENTER = 1,
			SORT_FAR = 2
		};

		struct Flags {
			uint32_t alpha : 1;
			uint32_t dither : 1;
			uint32_t forceBlend : 1;
			uint32_t dblSided : 1;
			uint32_t flipTangent : 1;
			uint32_t flipBitangent : 1;
			uint32_t shadowCast : 1;
			uint32_t shadowRecv : 1;
			uint32_t baseMapSpecAlpha : 1;
			uint32_t sortTris : 1;
			uint32_t diffRoughness : 1;
		};

		struct BasePatternExt {
			xt_float2 offs;
			xt_float2 scl;
			xt_float3 clr;
			int32_t texId;
		};

		struct SpecPatternExt {
			xt_float2 offs;
			xt_float2 scl;
			xt_float3 clr;
			int32_t texId;
		};

		struct NormPatternExt {
			xt_float2 offs;
			xt_float2 scl;
			xt_float2 factor;
			int32_t texId;
		};

		int32_t mNameId;
		int32_t mPathId;
		int32_t mBaseTexId;
		int32_t mSpecTexId;
		int32_t mBumpTexId;
		int32_t mSurfTexId;
		int32_t mExtTexId;
		Flags mFlags;
		xt_float3 mBaseColor;
		xt_float3 mSpecColor;
		float mRoughness;
		float mFresnel;
		float mMetallic;
		float mBumpScale;
		float mAlphaLim;
		float mShadowOffs;
		float mShadowWght;
		float mShadowDensity;
		float mShadowAlphaLim;
		float mMask;
		uint32_t mExtOffs;
		uint32_t mSwapOffs;
		xt_half mSortBias;
		uint8_t mSortMode;
		int8_t mSortLayer;
		uint32_t mReserved;
		uint32_t mWk[4];

		bool is_alpha() const { return mFlags.alpha != 0; }
		bool is_cutout() const { return mAlphaLim > 0.0f; }

		SortMode get_sort_mode() const {
			SortMode sm = SORT_NEAR;
			switch (mSortMode) {
				case 0:
				case 1:
				case 2:
					sm = (SortMode)mSortMode;
					break;
			}
			return sm;
		}

		float get_sort_bias() const { return mSortBias.get(); }
	};

	struct Batch {
		int32_t mNameId;
		int32_t mMtlId;
		int32_t mMinIdx;
		int32_t mMaxIdx;
		int32_t mIdxOrg;
		int32_t mTriNum;
		int32_t mJntNum;
		int32_t mJntInfoOrg;

		bool is_idx16() const { return (mMaxIdx - mMinIdx) < (1 << 16); }
	};

	struct SkinInfo {
		int32_t mMdlSpheresOffs;
		int32_t mBatSpheresOffs;
		int32_t mNamesOffs;
		int32_t mSkelMapOffs;
		int32_t mBatJntListsOffs;
		int32_t mBatItemsNum;
	};

	bool is_static() const { return (mFlags & 1) != 0; }
	bool half_encoding() const { return (mFlags & 2) != 0; }
	bool has_skin() const { return mSknNum > 0; }
	bool has_skel() const { return mSklNum > 0; }
	size_t get_vtx_size() const;
	bool ck_pid(int pid) const { return uint32_t(pid) < mPntNum; }
	bool ck_mtl_id(const int imtl) const { return uint32_t(imtl) < mMtlNum; }
	bool ck_tex_id(const int tid) const { return uint32_t(tid) < mTexNum; }
	bool ck_batch_id(const int ibat) const { return uint32_t(ibat) < mBatNum; }
	bool ck_skin_id(const int iskn) const { return uint32_t(iskn) < mSknNum; }
	bool ck_skel_id(const int iskl) const { return uint32_t(iskl) < mSklNum; }
	const void* get_pnt_data_top() const { return mPntOffs ? XD_INCR_PTR(this, mPntOffs) : nullptr; }
	const uint16_t* get_idx16_top() const { return mIdx16Offs ? reinterpret_cast<const uint16_t*>(XD_INCR_PTR(this, mIdx16Offs)) : nullptr; }
	const uint32_t* get_idx32_top() const { return mIdx32Offs ? reinterpret_cast<const uint32_t*>(XD_INCR_PTR(this, mIdx32Offs)) : nullptr; }
	const SkinInfo* get_skin_info() const { return mSknOffs ? reinterpret_cast<const SkinInfo*>(XD_INCR_PTR(this, mSknOffs)) : nullptr; }
	cxVec get_pnt_pos(const int pid) const;
	cxVec get_pnt_nrm(const int pid) const;
	cxColor get_pnt_clr(const int pid) const;
	xt_texcoord get_pnt_tex(const int pid) const;
	PntSkin get_pnt_skin(const int pid) const;
	const cxSphere* get_mdl_spheres() const;
	const Material* get_material(const int imtl) const;
	int find_material_id(const char* pName) const;
	const char* get_material_name(const int imtl) const;
	const char* get_material_path(const int imtl) const;
	bool mtl_has_swaps(const int imtl) const;
	int get_mtl_num_swaps(const int imtl) const;
	const Material* get_swap_material(const int imtl, const int iswp) const;
	bool mtl_has_exts(const int imtl) const;
	const ExtList* get_mtl_ext_list(const int imtl) const;
	int get_mtl_num_exts(const int imtl) const;
	uint32_t find_mtl_ext_offs(const int imtl, const uint32_t kind) const;
	const Material::BasePatternExt* find_mtl_base_pattern_ext(const int imtl) const;
	const Material::SpecPatternExt* find_mtl_spec_pattern_ext(const int imtl) const;
	const Material::NormPatternExt* find_mtl_norm_pattern_ext(const int imtl) const;
	cxAABB get_batch_bbox(const int ibat) const;
	const Batch* get_batch_ptr(const int ibat) const;
	const char* get_batch_name(const int ibat) const;
	xt_int3 get_batch_tri_indices(const int ibat, const int itri) const;
	int get_batch_jnt_num(const int ibat) const;
	const int32_t* get_batch_jnt_list(const int ibat) const;
	const cxSphere* get_batch_spheres(const int ibat) const;
	const Material* get_batch_material(const int ibat) const;
	int count_alpha_batches() const;
	const TexInfo* get_tex_info(const int tid) const;
	TexInfo* get_tex_info(const int tid);
	void clear_tex_wk();
	const char* get_tex_name(const int tid) const;
	const char* get_tex_path(const int tid) const;
	const char* get_skin_name(const int iskn) const;
	const int32_t* get_skin_to_skel_map() const;
	int find_skin_id(const char* pName) const;
	const xt_xmtx* get_skel_xforms_ptr() const;
	xt_xmtx get_skel_local_xform(const int inode) const;
	xt_xmtx get_skel_inv_world_xform(const int inode) const;
	xt_xmtx calc_skel_node_world_xform(const int inode, const xt_xmtx* pLocXforms, xt_xmtx* pParentXform = nullptr) const;
	xt_xmtx calc_skel_node_chain_xform(const int inode, const int itop, const xt_xmtx* pLocXforms) const;
	cxVec get_skel_local_offs(const int inode) const;
	const int32_t* get_skel_names_ptr() const;
	const int32_t* get_skel_parents_ptr() const;
	const char* get_skel_name(const int iskl) const;
	int find_skel_node_id(const char* pName) const;

	template<typename T> T* get_gpu_wk() { return reinterpret_cast<T*>(mGPUWk); }
	template<typename T> const T* get_gpu_wk() const { return reinterpret_cast<const T*>(mGPUWk); }

	void dump_geo(FILE* pOut) const;
	void dump_geo(const char* pOutPath) const;
	void dump_ocapt(FILE* pOut, const char* pSkelPath = nullptr) const;
	void dump_ocapt(const char* pOutPath, const char* pSkelPath = nullptr) const;
	void dump_skel(FILE* pOut) const;
	void dump_skel(const char* pOutPath) const;
	void dump_mdl_spheres(FILE* pOut) const;
	void dump_mdl_spheres(const char* pOutPath) const;
	void dump_bat_spheres(FILE* pOut) const;
	void dump_bat_spheres(const char* pOutPath) const;

	static const uint32_t KIND;
};

struct sxTextureData : public sxData {
	enum class Format {
		RGBA_BYTE_LINEAR = 0,
		RGBA_BYTE_GAMMA2 = 1
	};

	uint32_t mWidth;
	uint32_t mHeight;
	uint32_t mFormat;
	uint32_t mReserved;
	uint32_t mDataOffs;
	uint32_t mCmprOffs;
	uint32_t mReserved1;
	uint32_t mReserved2;
	uint32_t mGPUWk[4];

	Format get_format() const { return Format(mFormat & 0xFF); }
	bool mipmap_disabled() const { return (mFlags & 1) != 0; }
	bool mipmap_enabled() const { return !mipmap_disabled(); }
	bool lod_bias_disabled() const { return (mFlags & 2) != 0; }
	bool lod_bias_enabled() const { return !lod_bias_disabled(); }
	bool is_gamma2() const;
	const void* get_data_ptr() const { return mDataOffs ? XD_INCR_PTR(this, mDataOffs) : nullptr; }

	template<typename T> T* get_gpu_wk() { return reinterpret_cast<T*>(mGPUWk); }
	template<typename T> const T* get_gpu_wk() const { return reinterpret_cast<const T*>(mGPUWk); }

	static const uint32_t KIND;
};

struct sxMotionData : public sxData {
	float mFPS;
	uint32_t mFrameNum;
	int32_t mStartFrame;
	uint32_t mNodeNum;
	uint32_t mNodeOffs;
	uint32_t mReserved0;
	uint32_t mReserved1;
	uint32_t mReserved2;

	struct Node {
		int16_t mNameId;
		uint16_t mAttr;
		uint32_t mTrkOffsQ;
		uint32_t mTrkOffsT;
		uint32_t mTrkOffsS;

		exRotOrd get_rot_ord() const { return (exRotOrd)(mAttr & 7); }
	};

	struct Track {
		cxAABB mBBox;
		uint32_t mAttr;
		uint16_t mData[1];

		int get_mask() const { return mAttr & 7; }

		int get_stride() const {
			int s = 0;
			int m = get_mask();
			for (int i = 0; i < 3; ++i) {
				if (m & (1 << i)) ++s;
			}
			return s;
		}
	};

	bool ck_node_id(const int inode) const { return uint32_t(inode) < mNodeNum; }
	const Node* get_nodes_top() const { return mNodeOffs ? reinterpret_cast<const Node*>(XD_INCR_PTR(this, mNodeOffs)) : nullptr; }
	const Node* get_node(const int inode) const;
	const char* get_node_name(const int inode) const;
	int find_node_id(const char* pName) const;
	const Node* find_node(const char* pName) const;
	const Track* get_q_track(const int inode) const;
	const Track* get_t_track(const int inode) const;
	cxQuat eval_quat(const int inode, const float frm) const;
	cxVec eval_pos(const int inode, const float frm) const;

	void dump_clip(FILE* pOut, const float fstep = 1.0f) const;
	void dump_clip(const char* pOutPath, const float fstep = 1.0f) const;

	static const uint32_t KIND;
};

struct sxCollisionData : public sxData {
	cxAABB mBBox;
	uint32_t mPntNum;
	uint32_t mPolNum;
	uint32_t mGrpNum;
	uint32_t mTriNum;
	uint32_t mMaxVtxPerPol;
	uint32_t mReserved;
	uint32_t mPntOffs;
	uint32_t mPolIdxOrgOffs;
	uint32_t mPolVtxNumOffs;
	uint32_t mPolTriOrgOffs;
	uint32_t mPolIdxOffs;
	uint32_t mPolGrpOffs;
	uint32_t mPolTriIdxOffs;
	uint32_t mPolBBoxOffs;
	uint32_t mPolNrmOffs;
	uint32_t mGrpInfoOffs;
	uint32_t mBVHBBoxOffs;
	uint32_t mBVHInfoOffs;

	struct GrpInfo {
		int16_t mNameId;
		int16_t mPathId;
	};

	struct BVHNodeInfo {
		int32_t mLeft;
		int32_t mRight;

		bool is_leaf() const { return mRight < 0; }
		int get_pol_id() const { return is_leaf() ? mLeft : -1; }
	};

	struct Tri {
		cxVec vtx[3];
		cxVec nrm;
		int ipol;
		int itri;
	};

	struct NearestHit {
		cxVec pos;
		cxVec nrm;
		float dist;
		int count;
	};

	typedef bool (*TriFunc)(const sxCollisionData& col, const Tri& tri, void* pWk);
	typedef bool (*HitFunc)(const sxCollisionData& col, const Tri& tri, const cxVec& pos, const float dist, void* pWk);

	bool ck_pid(const int pid) const { return uint32_t(pid) < mPntNum; }
	bool ck_pnt_id(const int ipnt) const { return ck_pid(ipnt); }
	bool ck_pol_id(const int ipol) const { return uint32_t(ipol) < mPolNum; }
	bool ck_grp_id(const int igrp) const { return uint32_t(igrp) < mGrpNum; }
	bool all_pols_same_size() const { return bool(mFlags & 1); }
	bool all_tris() const { return all_pols_same_size() && mMaxVtxPerPol == 3; }

	cxVec get_pnt(const int ipnt) const;

	cxAABB get_pol_bbox(const int ipol) const;
	cxVec get_pol_normal(const int ipol) const;
	int get_pol_grp_id(const int ipol) const;
	int get_pol_num_vtx(const int ipol) const;
	int get_pol_num_tris(const int ipol) const;
	int get_pol_pnt_idx(const int ipol, const int ivtx) const;
	int get_pol_tri_pnt_idx(const int ipol, const int itri, const int ivtx) const;

	const char* get_grp_name(const int igrp) const;
	const char* get_grp_path(const int igrp) const;

	int for_all_tris(const TriFunc func, void* pWk, const bool calcNormals = true) const;
	int for_tris_in_range(const TriFunc func, const cxAABB& bbox, void* pWk, const bool calcNormals = true) const;

	int hit_check(const HitFunc func, const cxLineSeg& seg, void* pWk);
	NearestHit nearest_hit(const cxLineSeg& seg);

	void dump_pol_geo(FILE* pOut) const;
	void dump_pol_geo(const char* pOutPath) const;
	void dump_tri_geo(FILE* pOut) const;
	void dump_tri_geo(const char* pOutPath) const;

	static const uint32_t KIND;
};

struct sxFileCatalogue : public sxData {
	uint32_t mFilesNum;
	uint32_t mListOffs;

	struct FileInfo {
		int32_t mNameId;
		int32_t mFileNameId;
	};

	bool ck_file_idx(int idx) const { return (uint32_t)idx < mFilesNum; }
	const FileInfo* get_info(int idx) const { return ck_file_idx(idx) ? &((const FileInfo*)XD_INCR_PTR(this, mListOffs))[idx] : nullptr; }
	const char* get_item_name(int idx) const { const FileInfo* pInfo = get_info(idx); return pInfo ? get_str(pInfo->mNameId) : nullptr; }
	const char* get_file_name(int idx) const { const FileInfo* pInfo = get_info(idx); return pInfo ? get_str(pInfo->mFileNameId) : nullptr; }
	int find_item_name_idx(const char* pName) const;

	static const uint32_t KIND;
};


#define XD_PLEXLST_TAG "xPlexLst"

template <typename T, int PLEX_SIZE = 16>
class cxPlexList {
protected:
	struct Link {
		Link* mpPrev;
		Link* mpNext;
		T* mpItem;

		void init(T* pItem) {
			mpPrev = nullptr;
			mpNext = nullptr;
			mpItem = pItem;
		}

		void reset() {
			mpPrev = nullptr;
			mpNext = nullptr;
			mpItem = nullptr;
		}
	};

	class Plex {
	public:
		cxPlexList* mpList;
		Plex* mpPrev;
		Plex* mpNext;
		Link mNode[PLEX_SIZE];
		int32_t mCount;
		XD_BIT_ARY_DECL(uint32_t, mMask, PLEX_SIZE);

	protected:
		Plex() { reset(); }

		void ctor(cxPlexList* pList) {
			mpList = pList;
			mpPrev = nullptr;
			mpNext = nullptr;
			reset();
		}

		friend class cxPlexList;

	public:
		class Itr {
		protected:
			Plex* mpPlex;
			int32_t mCount;

		public:
			int32_t mIdx;

			Itr(Plex* pPlex) : mpPlex(pPlex) {
				reset();
				mIdx = mpPlex->find_first();
				if (mIdx >= 0) {
					++mCount;
				}
			}

			void reset() {
				mCount = 0;
				mIdx = -1;
			}

			void next() {
				if (mCount >= mpPlex->get_count()) {
					mIdx = -1;
				} else {
					mIdx = mpPlex->find_next(mIdx);
					if (mIdx >= 0) {
						++mCount;
					}
				}
			}

			bool end() const { return mIdx < 0; }

			T* get_item() const {
				T* pItm = nullptr;
				if (mIdx >= 0) {
					pItm = (*mpPlex)[mIdx];
				}
				return pItm;
			}
		};

		void reset() {
			::memset(mMask, 0, sizeof(mMask));
			mCount = 0;
		}

		int get_count() const { return mCount; }
		bool is_full() const { return get_count() >= PLEX_SIZE; }
		T* get_top() { return reinterpret_cast<T*>(this + 1); }
		T* get_end() { return get_top() + PLEX_SIZE; }

		int get_idx(T* pItm) {
			T* pTop = get_top();
			if (pItm >= pTop && pItm < get_end()) {
				return (int)(pItm - pTop);
			}
			return -1;
		}

		T* operator[](int i) { return &get_top()[i]; }

		int find_next(int last = -1) const {
			int idx = -1;
			if (mCount > 0) {
				for (int i = last + 1; i < PLEX_SIZE; ++i) {
					if (XD_BIT_ARY_CK(uint32_t, mMask, i)) {
						idx = i;
						break;
					}
				}
			}
			return idx;
		}

		int find_first() const { return find_next(); }

		int get_slot() {
			int idx = -1;
			if (!is_full()) {
				for (int i = 0; i < PLEX_SIZE; ++i) {
					if (!XD_BIT_ARY_CK(uint32_t, mMask, i)) {
						idx = i;
						break;
					}
				}
			}
			return idx;
		}

		T* alloc_item() {
			T* pItm = nullptr;
			int idx = get_slot();
			if (idx >= 0) {
				XD_BIT_ARY_ST(uint32_t, mMask, idx);
				pItm = operator[](idx);
				++mCount;
			}
			if (pItm && mpList && mpList->mItmCtor) {
				mpList->mItmCtor(pItm);
			}
			return pItm;
		}

		void free_item(T* pItm) {
			int idx = get_idx(pItm);
			if (idx >= 0) {
				if (pItm && mpList && mpList->mItmDtor) {
					mpList->mItmDtor(pItm);
				}
				XD_BIT_ARY_CL(uint32_t, mMask, idx);
				--mCount;
			}
		}
	};

	Plex* mpHead;
	Link* mpLinkHead;
	Link* mpLinkTail;
	void (*mItmCtor)(T*);
	void (*mItmDtor)(T*);
	int32_t mCount;
	bool mAutoDelete;

	Plex* alloc_plex() {
		Plex* pPlex = nullptr;
		size_t size = sizeof(Plex) + PLEX_SIZE*sizeof(T);
		void* pMem = nxCore::mem_alloc(size, "xPlex");
		if (pMem) {
			pPlex = reinterpret_cast<Plex*>(pMem);
			pPlex->ctor(this);
		}
		return pPlex;
	}

	void free_plex(Plex* pPlex) {
		if (!pPlex) return;
		nxCore::mem_free(pPlex);
	}

	void clear_plex(Plex* pPlex) {
		if (!pPlex) return;
		if (pPlex->get_count()) {
			if (mItmDtor) {
				for (typename Plex::Itr it(pPlex); !it.end(); it.next()) {
					mItmDtor(it.get_item());
				}
			}
		}
		pPlex->reset();
	}

	void destroy_plex(Plex* pPlex) {
		clear_plex(pPlex);
		free_plex(pPlex);
	}

	Plex* find_plex(T* pItem) const {
		Plex* pRes = nullptr;
		Plex* pPlex = mpHead;
		while (pPlex) {
			if (pPlex->get_idx(pItem) >= 0) {
				pRes = pPlex;
				break;
			}
			pPlex = pPlex->mpNext;
		}
		return pRes;
	}

public:
	class Itr {
	protected:
		Link* mpLink;
	public:
		Itr(Link* pLink) : mpLink(pLink) {}
		T* item() const { return mpLink ? mpLink->mpItem : nullptr; }
		bool end() const { return mpLink == nullptr; }
		void next() {
			if (mpLink) {
				mpLink = mpLink->mpNext;
			}
		}
	};

protected:
	void ctor(bool preAlloc, bool autoDelete) {
		mpHead = nullptr;
		mpLinkHead = nullptr;
		mpLinkTail = nullptr;
		set_item_handlers(nullptr, nullptr);
		mCount = 0;
		mAutoDelete = autoDelete;
		if (preAlloc) {
			mpHead = alloc_plex();
		}
	}

	void dtor() {
		purge();
	}

public:
	cxPlexList(bool preAlloc = false, bool autoDelete = true) {
		ctor(preAlloc, autoDelete);
	}

	~cxPlexList() {
		dtor();
	}

	void set_item_handlers(void (*ctor)(T*), void (*dtor)(T*)) {
		mItmCtor = ctor;
		mItmDtor = dtor;
	}

	int get_count() const { return mCount; }

	void purge() {
		Plex* pPlex = mpHead;
		while (pPlex) {
			Plex* pNext = pPlex->mpNext;
			destroy_plex(pPlex);
			pPlex = pNext;
		}
		mCount = 0;
		mpLinkHead = nullptr;
		mpLinkTail = nullptr;
		mpHead = nullptr;
	}

	void clear() {
		Plex* pPlex = mpHead;
		while (pPlex) {
			Plex* pNext = pPlex->mpNext;
			clear_plex(pPlex);
			if (mAutoDelete) {
				free_plex(pPlex);
			}
			pPlex = pNext;
		}
		mCount = 0;
		mpLinkHead = nullptr;
		mpLinkTail = nullptr;
		if (mAutoDelete) {
			mpHead = nullptr;
		}
	}

	T* new_item() {
		T* pItem = 0;
		if (!mpHead) {
			mpHead = alloc_plex();
			mpLinkHead = 0;
			mpLinkHead = 0;
			mCount = 0;
		}
		Plex* pPlex = mpHead;
		while (true) {
			if (!pPlex->is_full()) {
				break;
			}
			if (pPlex->mpNext) {
				pPlex = pPlex->mpNext;
			} else {
				Plex* pNew = alloc_plex();
				pNew->mpPrev = pPlex;
				pPlex->mpNext = pNew;
				pPlex = pNew;
				break;
			}
		}
		if (pPlex) {
			pItem = pPlex->alloc_item();
			if (pItem) {
				int idx = pPlex->get_idx(pItem);
				if (idx >= 0) {
					Link* pLink = &pPlex->mNode[idx];
					pLink->init(pItem);
					if (mpLinkHead) {
						pLink->mpPrev = mpLinkTail;
						mpLinkTail->mpNext = pLink;
						mpLinkTail = pLink;
					} else {
						mpLinkHead = pLink;
						mpLinkTail = pLink;
					}
				}
			}
		}
		if (pItem) {
			++mCount;
		}
		return pItem;
	}

	void remove(T* pItem) {
		if (!pItem) return;
		if (mCount <= 0) return;
		Plex* pPlex = find_plex(pItem);
		if (pPlex) {
			int idx = pPlex->get_idx(pItem);
			if (idx >= 0) {
				Link* pLink = &pPlex->mNode[idx];
				Link* pNext = pLink->mpNext;
				if (pLink->mpPrev) {
					pLink->mpPrev->mpNext = pNext;
					if (pNext) {
						pNext->mpPrev = pLink->mpPrev;
					} else {
						mpLinkTail = pLink->mpPrev;
					}
				} else {
					mpLinkHead = pNext;
					if (mpLinkHead) {
						mpLinkHead->mpPrev = nullptr;
					} else {
						mpLinkTail = nullptr;
					}
				}
				pPlex->free_item(pItem);
				pLink->reset();
				--mCount;
				if (pPlex->get_count() <= 0) {
					if (mAutoDelete) {
						Plex* pPrevPlex = pPlex->mpPrev;
						Plex* pNextPlex = pPlex->mpNext;
						destroy_plex(pPlex);
						if (pPlex == mpHead) {
							if (pNextPlex) {
								mpHead = pNextPlex;
								mpHead->mpPrev = nullptr;
							} else {
								mpHead = nullptr;
							}
						} else {
							if (pPrevPlex) {
								pPrevPlex->mpNext = pNextPlex;
							}
							if (pNextPlex) {
								pNextPlex->mpPrev = pPrevPlex;
							}
						}
					}
				}
			}
		}
	}

	Itr get_itr() { return Itr(mpLinkHead); }

	T* get_item(int idx) {
		T* pItm = nullptr;
		if (uint32_t(idx) < uint32_t(get_count())) {
			int cnt = 0;
			for (Itr it = get_itr(); !it.end(); it.next()) {
				if (cnt == idx) {
					pItm = it.item();
					break;
				}
				++cnt;
			}
		}
		return pItm;
	}

	static cxPlexList* create(const char* pTag = XD_PLEXLST_TAG, bool preAlloc = false, bool autoDelete = true) {
		cxPlexList* pLst = (cxPlexList*)nxCore::mem_alloc(sizeof(cxPlexList), pTag ? pTag : XD_PLEXLST_TAG);
		if (pLst) {
			pLst->ctor(preAlloc, autoDelete);
		}
		return pLst;
	}

	static void destroy(cxPlexList* pLst) {
		if (pLst) {
			pLst->dtor();
			nxCore::mem_free(pLst);
		}
	}
};

#define XD_STRSTORE_TAG "xStrStore"

class cxStrStore {
private:
	cxStrStore* mpNext;
	size_t mSize;
	size_t mPtr;

	cxStrStore() {}

public:
	char* add(const char* pStr);

	static cxStrStore* create(const char* pTag = XD_STRSTORE_TAG);
	static void destroy(cxStrStore* pStore);
};

#define XD_STRMAP_TAG "xStrMap"

template<typename T> class cxStrMap {
protected:
	struct Slot {
		const char* pKey;
		T val;
		uint32_t hash;

		void chain() {
			hash |= 1U << 31;
		}

		bool is_chained() const {
			return (hash & (1U << 31)) != 0;
		}

		uint32_t get_hash() {
			return hash & (~(1U << 31));
		}

		bool hash_cmp(uint32_t h) {
			return get_hash() == h;
		}

		void clear_hash() {
			hash &= 1U << 31;
		}
	};

	int mInUse;
	float mLoadFactor;
	int mThreshold;
	int mSize;
	Slot* mpSlots;

	void set_new_tbl(Slot* pSlots) {
		if (mpSlots) {
			nxCore::mem_free(mpSlots);
		}
		mpSlots = pSlots;
		mThreshold = nxCalc::min(int(float(mSize) * mLoadFactor), mSize - 1);
	}

	Slot* alloc_tbl(int newSize = 0) {
		size_t size = (newSize <= 0 ? mSize : newSize) * sizeof(Slot);
		Slot* pSlots = reinterpret_cast<Slot*>(nxCore::mem_alloc(size, "xStrMap:Slots"));
		if (pSlots) {
			::memset(pSlots, 0, size);
		}
		return pSlots;
	}

	uint32_t calc_step(uint32_t hash, const int size = 0) {
		uint32_t s = uint32_t(size <= 1 ? mSize : size);
		if (s <= 1) return 1;
		return (((hash >> 5) + 1U) % (s-1)) + 1;
	}

	void rehash() {
		uint32_t newSize = uint32_t(nxCalc::prime((mSize << 1) | 1));
		Slot* pNewSlots = alloc_tbl(newSize);
		for (int i = 0; i < mSize; ++i) {
			if (mpSlots[i].pKey) {
				uint32_t h = mpSlots[i].get_hash();
				uint32_t spot = h;
				uint32_t step = calc_step(h, newSize);
				for (uint32_t j = spot % newSize; ; spot += step, j = spot % newSize) {
					if (pNewSlots[j].pKey) {
						pNewSlots[j].chain();
					} else {
						pNewSlots[j].pKey = mpSlots[i].pKey;
						pNewSlots[j].val = mpSlots[i].val;
						pNewSlots[j].hash |= h;
						break;
					}
				}
			}
		}
		mSize = int(newSize);
		set_new_tbl(pNewSlots);
	}

	const char* get_removed_marker() const {
		return reinterpret_cast<const char*>(this);
	}

	const char* put_impl(const char* pKey, T val, bool overwrite) {
		if (mInUse >= mThreshold) {
			rehash();
		}
		const char* pRemoved = get_removed_marker();
		uint32_t h = nxCore::str_hash32(pKey) & (~(1U << 31));
		uint32_t spot = h;
		uint32_t step = calc_step(h);
		int freeIdx = -1;
		for (int i = 0; i < mSize; ++i) {
			int idx = int(spot % uint32_t(mSize));
			Slot* pSlot = &mpSlots[idx];
			if (freeIdx < 0 && pSlot->pKey == pRemoved && pSlot->is_chained()) {
				freeIdx = idx;
			}
			if (!pSlot->pKey || (pSlot->pKey == pRemoved && !pSlot->is_chained())) {
				if (freeIdx < 0) {
					freeIdx = idx;
				}
				break;
			}
			if (pSlot->hash_cmp(h) && nxCore::str_eq(pSlot->pKey, pKey)) {
				if (overwrite) {
					mpSlots[idx].val = val;
					return mpSlots[idx].pKey;
				} else {
					return nullptr;
				}
			}
			if (freeIdx < 0) {
				mpSlots[idx].chain();
			}
			spot += step;
		}
		if (freeIdx >= 0) {
			mpSlots[freeIdx].pKey = pKey;
			mpSlots[freeIdx].val = val;
			mpSlots[freeIdx].hash |= h;
			++mInUse;
			return mpSlots[freeIdx].pKey;
		}
		return nullptr;
	}

	int find(const char* pKey) {
		uint32_t h = nxCore::str_hash32(pKey) & (~(1U << 31));
		uint32_t spot = h;
		uint32_t step = calc_step(h);
		for (int i = 0; i < mSize; ++i) {
			int idx = int(spot % uint32_t(mSize));
			Slot* pSlot = &mpSlots[idx];
			if (!pSlot->pKey) return -1;
			if (pSlot->hash_cmp(h) && nxCore::str_eq(pSlot->pKey, pKey)) {
				return idx;
			}
			if (!pSlot->is_chained()) {
				return -1;
			}
			spot += step;
		}
		return -1;
	}

	void ctor(int capacity, float loadScl) {
		mInUse = 0;
		mpSlots = nullptr;
		capacity = nxCalc::max(capacity, 16);
		mLoadFactor = 0.75f * nxCalc::max(loadScl, 0.1f);
		mSize = nxCalc::prime(int(float(capacity) / mLoadFactor));
		set_new_tbl(alloc_tbl());
	}

	void dtor() {
		if (mpSlots) {
			nxCore::mem_free(mpSlots);
		}
	}

public:
	cxStrMap(int capacity = 0, float loadScl = 1.0f) {
		ctor(capacity, loadScl);
	}

	~cxStrMap() {
		dtor();
	}

	int get_item_count() const {
		return mInUse;
	}

	const char* put(const char* pKey, T val) {
		if (pKey == nullptr || pKey == get_removed_marker()) {
			return nullptr;
		}
		return put_impl(pKey, val, true);
	}

	const char* add(const char* pKey, T val) {
		if (pKey == nullptr || pKey == get_removed_marker()) {
			return nullptr;
		}
		return put_impl(pKey, val, false);
	}

	bool get(const char* pKey, T* pVal) {
		int idx = find(pKey);
		if (idx < 0) return false;
		if (pVal) {
			*pVal = mpSlots[idx].val;
		}
		return true;
	}

	void remove(const char* pKey) {
		int idx = find(pKey);
		if (idx >= 0) {
			mpSlots[idx].clear_hash();
			if (mpSlots[idx].is_chained()) {
				mpSlots[idx].pKey = get_removed_marker();
			} else {
				mpSlots[idx].pKey = nullptr;
			}
			::memset(&mpSlots[idx].val, 0, sizeof(T));
			--mInUse;
		}
	}

	const char* find_key_for_val(T val) {
		const char* pKey = nullptr;
		for (int i = 0; i < mSize; ++i) {
			Slot* pSlot = &mpSlots[i];
			if (pSlot->pKey && pSlot->pKey != get_removed_marker()) {
				if (val == pSlot->val) {
					pKey = pSlot->pKey;
					break;
				}
			}
		}
		return pKey;
	}

	static cxStrMap* create(const char* pTag = XD_STRMAP_TAG, int capacity = 0, float loadScl = 1.0f) {
		cxStrMap* pMap = (cxStrMap*)nxCore::mem_alloc(sizeof(cxStrMap), pTag ? pTag : XD_STRMAP_TAG);
		if (pMap) {
			pMap->ctor(capacity, loadScl);
		}
		return pMap;
	}

	static void destroy(cxStrMap* pMap) {
		if (pMap) {
			pMap->dtor();
			nxCore::mem_free(pMap);
		}
	}
};

class cxCmdLine {
protected:
	typedef cxStrMap<char*> MapT;
	typedef cxPlexList<char*> ListT;

	char* mpProgPath;
	cxStrStore* mpStore;
	ListT* mpArgLst;
	MapT* mpOptMap;

	void ctor(int argc, char* argv[]);
	void dtor();

public:
	cxCmdLine(int argc, char* argv[]) {
		ctor(argc, argv);
	}

	~cxCmdLine() {
		dtor();
	}

	const char* get_full_prog_path() const {
		return mpProgPath;
	}

	int get_args_count() const {
		return mpArgLst ? mpArgLst->get_count() : 0;
	}

	const char* get_arg(const int i) const;

	int get_opts_count() const {
		return mpOptMap ? mpOptMap->get_item_count() : 0;
	}

	const char* get_opt(const char* pName) const;

	int get_int_opt(const char* pName, const int defVal = 0) const;
	float get_float_opt(const char* pName, const float defVal = 0.0f) const;
	bool get_bool_opt(const char* pName, const bool defVal = false) const;

	static cxCmdLine* create(int argc, char* argv[]);
	static void destroy(cxCmdLine* pCmdLine);
};

class cxStopWatch {
protected:
	double* mpSmps;
	double mT;
	int mSmpsNum;
	int mSmpIdx;

public:
	cxStopWatch() : mpSmps(nullptr), mSmpsNum(0), mSmpIdx(0) {}
	~cxStopWatch() { free(); }

	void alloc(int nsmps);
	void free();

	void begin();
	bool end();
	void reset();
	double median();
};


namespace nxSH {

inline int calc_coefs_num(int order) { return order < 1 ? 0 : nxCalc::sq(order); }
inline constexpr int calc_consts_num(int order) { return order < 1 ? 0 : order*order - (order - 1); }
inline int calc_ary_idx(int l, int m) { return l*(l+1) + m; }
inline int band_idx_from_ary_idx(int idx) { return (int)::mth_sqrtf((float)idx); }
inline int func_idx_from_ary_band(int idx, int l) { return idx - l*(l + 1); }

void calc_weights(float* pWgt, int order, float s, float scl = 1.0f);
void get_diff_weights(float* pWgt, int order, float scl = 1.0f);
void apply_weights(float* pDst, int order, const float* pSrc, const float* pWgt, int nelems = 1);
void calc_consts(int order, float* pConsts);
void calc_consts(int order, double* pConsts);
void eval(int order, float* pCoefs, float x, float y, float z, const float* pConsts);
void eval_ary4(int order, float* pCoefs, float x[4], float y[4], float z[4], const float* pConsts);
void eval_ary8(int order, float* pCoefs, float x[8], float y[8], float z[8], const float* pConsts);
void eval_sh3(float* pCoefs, float x, float y, float z, const float* pConsts);
void eval_sh3_ary4(float* pCoefs, float x[4], float y[4], float z[4], const float* pConsts);
void eval_sh3_ary8(float* pCoefs, float x[8], float y[8], float z[8], const float* pConsts);

} // nxSH

namespace nxDataUtil {

exAnimChan anim_chan_from_str(const char* pStr);
const char* anim_chan_to_str(exAnimChan chan);
exRotOrd rot_ord_from_str(const char* pStr);
const char* rot_ord_to_str(exRotOrd rord);
exTransformOrd xform_ord_from_str(const char* pStr);
const char* xform_ord_to_str(exTransformOrd xord);

} // nxDataUtil

struct sxPackedData {
	uint32_t mSig;
	uint32_t mAttr;
	uint32_t mPackSize;
	uint32_t mRawSize;

	int get_mode() const { return (int)(mAttr & 0xFF); }

	static const uint32_t SIG;
};

namespace nxData {

sxData* load(const char* pPath);
void unload(sxData* pData);

sxPackedData* pack(const uint8_t* pSrc, const uint32_t srcSize, const uint32_t mode = 0);
uint8_t* unpack(sxPackedData* pPkd, const char* pTemTag = "xTmpMem", uint8_t* pDstMem = nullptr, const uint32_t dstMemSize = 0, size_t* pSize = nullptr, const bool recursive = true);

template<typename T> T* load_as(const char* pPath) {
	sxData* pData = nxData::load(pPath);
	if (pData) {
		if (pData->is<T>()) {
			return pData->as<T>();
		}
		nxData::unload(pData);
	}
	return nullptr;
}

} // nxData


#define XD_RSRC_ADDR_KEY_SIZE 24

class cxResourceManager {
protected:
	cxResourceManager() {}

public:
	typedef cxStrMap<sxGeometryData*> GeoDataMap;
	typedef cxStrMap<sxImageData*> ImgDataMap;
	typedef cxStrMap<sxRigData*> RigDataMap;
	typedef cxStrMap<sxKeyframesData*> KfrDataMap;
	typedef cxStrMap<sxValuesData*> ValDataMap;
	typedef cxStrMap<sxExprLibData*> ExpDataMap;
	typedef cxStrMap<sxModelData*> MdlDataMap;
	typedef cxStrMap<sxTextureData*> TexDataMap;
	typedef cxStrMap<sxMotionData*> MotDataMap;
	typedef cxStrMap<sxCollisionData*> ColDataMap;

	struct GfxIfc {
		void (*prepareModel)(sxModelData* pMdl);
		void (*releaseModel)(sxModelData* pMdl);
		void (*prepareTexture)(sxTextureData* pTex);
		void (*releaseTexture)(sxTextureData* pTex);
		void (*prepareModelWork)(cxModelWork* pMdlWk);
		void (*releaseModelWork)(cxModelWork* pMdlWk);

		void reset() {
			prepareModel = nullptr;
			releaseModel = nullptr;
			prepareTexture = nullptr;
			releaseTexture = nullptr;
			prepareModelWork = nullptr;
			releaseModelWork = nullptr;
		}
	};

	class Pkg {
	protected:
		Pkg() {}

	public:
		struct Entry {
			sxData* mpData;
			const char* mpName;
			const char* mpFileName;
			char mAddrKey[XD_RSRC_ADDR_KEY_SIZE];

			void set_data(sxData* pData);
		};

		typedef cxPlexList<Entry> EntryList;

	protected:
		char* mpName;
		cxResourceManager* mpMgr;
		sxFileCatalogue* mpCat;
		EntryList* mpEntries;
		GeoDataMap* mpGeoDataMap;
		ImgDataMap* mpImgDataMap;
		RigDataMap* mpRigDataMap;
		KfrDataMap* mpKfrDataMap;
		ValDataMap* mpValDataMap;
		ExpDataMap* mpExpDataMap;
		MdlDataMap* mpMdlDataMap;
		TexDataMap* mpTexDataMap;
		MotDataMap* mpMotDataMap;
		ColDataMap* mpColDataMap;
		sxModelData* mpDefMdl;
		sxGeometryData* mpDefGeo;
		sxRigData* mpDefRig;
		sxValuesData* mpDefVal;
		sxExprLibData* mpDefExp;

		friend class cxResourceManager;

	public:
		int mGeoNum;
		int mImgNum;
		int mRigNum;
		int mKfrNum;
		int mValNum;
		int mExpNum;
		int mMdlNum;
		int mTexNum;
		int mMotNum;
		int mColNum;

		const char* get_name() const { return mpName; }
		const EntryList* get_list() const { return mpEntries; }
		EntryList::Itr get_iterator() { return mpEntries->get_itr(); }
		sxGeometryData* find_geometry(const char* pName) const;
		sxImageData* find_image(const char* pName) const;
		sxRigData* find_rig(const char* pName) const;
		sxKeyframesData* find_keyframes(const char* pName) const;
		sxValuesData* find_values(const char* pName) const;
		sxExprLibData* find_expressions(const char* pName) const;
		sxModelData* find_model(const char* pName) const;
		sxTextureData* find_texture(const char* pName) const;
		sxMotionData* find_motion(const char* pName) const;
		sxCollisionData* find_collision(const char* pName) const;
		void prepare_gfx();
		void release_gfx();
		sxModelData* get_default_model() const { return mpDefMdl; }
		sxGeometryData* get_default_geometry() const { return mpDefGeo; }
		sxRigData* get_default_rig() const { return mpDefRig; }
		sxValuesData* get_default_values() const { return mpDefVal; }
		sxExprLibData* get_default_expressions() const { return mpDefExp; }
	};

protected:
	typedef cxPlexList<Pkg> PkgList;
	typedef cxStrMap<Pkg*> PkgMap;
	typedef cxStrMap<Pkg*> DataToPkgMap;

	char* mpAppPath;
	char* mpRelDataDir;
	char* mpDataPath;
	PkgList* mpPkgList;
	PkgMap* mpPkgMap;
	DataToPkgMap* mpDataToPkgMap;

	GfxIfc mGfxIfc;

	static void pkg_ctor(Pkg* pPkg);
	static void pkg_dtor(Pkg* pPkg);

public:
	const char* get_data_path() const { return mpDataPath; }
	bool contains_pkg(const Pkg* pPkg) const { return pPkg && pPkg->mpMgr == this; }
	Pkg* load_pkg(const char* pName);
	void unload_pkg(Pkg* pPkg);
	void unload_all();
	Pkg* find_pkg(const char* pName) const;
	Pkg* find_pkg_for_data(sxData* pData) const;
	sxGeometryData* find_geometry_in_pkg(Pkg* pPkg, const char* pGeoName) const;
	sxImageData* find_image_in_pkg(Pkg* pPkg, const char* pImgName) const;
	sxRigData* find_rig_in_pkg(Pkg* pPkg, const char* pRigName) const;
	sxKeyframesData* find_keyframes_in_pkg(Pkg* pPkg, const char* pKfrName) const;
	sxValuesData* find_values_in_pkg(Pkg* pPkg, const char* pValName) const;
	sxExprLibData* find_expressions_in_pkg(Pkg* pPkg, const char* pExpName) const;
	sxModelData* find_model_in_pkg(Pkg* pPkg, const char* pMdlName) const;
	sxTextureData* find_texture_in_pkg(Pkg* pPkg, const char* pTexName) const;
	sxMotionData* find_motion_in_pkg(Pkg* pPkg, const char* pMotName) const;
	sxCollisionData* find_collision_in_pkg(Pkg* pPkg, const char* pColName) const;

	sxTextureData* find_texture_for_model(sxModelData* pMdl, const char* pTexName) const { return find_texture_in_pkg(find_pkg_for_data(pMdl), pTexName); }
	sxMotionData* find_motion_for_model(sxModelData* pMdl, const char* pMotName) const { return find_motion_in_pkg(find_pkg_for_data(pMdl), pMotName); }
	sxValuesData* find_values_for_model(sxModelData* pMdl, const char* pValName) const { return find_values_in_pkg(find_pkg_for_data(pMdl), pValName); }

	sxModelData* get_pkg_default_model(Pkg* pPkg) const { return contains_pkg(pPkg) ? pPkg->get_default_model() : nullptr; }
	int get_num_geometries_in_pkg(Pkg* pPkg) const { return contains_pkg(pPkg) ? pPkg->mGeoNum : 0; }
	int get_num_images_in_pkg(Pkg* pPkg) const { return contains_pkg(pPkg) ? pPkg->mImgNum : 0; }
	int get_num_rigs_in_pkg(Pkg* pPkg) const { return contains_pkg(pPkg) ? pPkg->mRigNum : 0; }
	int get_num_keyframes_in_pkg(Pkg* pPkg) const { return contains_pkg(pPkg) ? pPkg->mKfrNum : 0; }
	int get_num_values_in_pkg(Pkg* pPkg) const { return contains_pkg(pPkg) ? pPkg->mValNum : 0; }
	int get_num_expressions_in_pkg(Pkg* pPkg) const { return contains_pkg(pPkg) ? pPkg->mExpNum : 0; }
	int get_num_models_in_pkg(Pkg* pPkg) const { return contains_pkg(pPkg) ? pPkg->mMdlNum : 0; }
	int get_num_textures_in_pkg(Pkg* pPkg) const { return contains_pkg(pPkg) ? pPkg->mTexNum : 0; }
	int get_num_motions_in_pkg(Pkg* pPkg) const { return contains_pkg(pPkg) ? pPkg->mMotNum : 0; }
	int get_num_collisions_in_pkg(Pkg* pPkg) const { return contains_pkg(pPkg) ? pPkg->mColNum : 0; }

	void set_gfx_ifc(const GfxIfc& ifc);
	void prepare_pkg_gfx(Pkg* pPkg);
	void release_pkg_gfx(Pkg* pPkg);
	void prepare_all_gfx();
	void release_all_gfx();
	GfxIfc get_gfx_ifc() const { return mGfxIfc; }

	static cxResourceManager* create(const char* pAppPath, const char* pRelDataDir);
	static void destroy(cxResourceManager* pMgr);
};


class cxMotionWork {
private:
	cxMotionWork() {}

public:
	sxModelData* mpMdlData;
	const sxMotionData* mpCurrentMotData;

	xt_xmtx* mpXformsL;
	xt_xmtx* mpXformsW;
	xt_xmtx* mpPrevXformsW;
	xt_xmtx* mpBlendXformsL;
	uint8_t* mpBlendDisableBits;
	cxVec mMoveRelPos;
	cxQuat mMoveRelQuat;
	float mEvalFrame;
	float mFrame;
	float mBlendDuration;
	float mBlendCount;
	float mUniformScale;
	float mHeightOffs;
	int mRootId;
	int mMoveId;
	int mCenterId;
	bool mPlayLastFrame;

	bool ck_node_id(const int inode) const { return mpMdlData ? mpMdlData->ck_skel_id(inode) : false; }
	int find_node_id(const char* pName) const { return mpMdlData ? mpMdlData->find_skel_node_id(pName) : -1; }

	void apply_motion(const sxMotionData* pMotData, const float frameAdd, float* pLoopFlg = nullptr);

	xt_xmtx eval_skel_node_chain_xform(const sxMotionData* pMotData, const int inode, const int itop, const float frame);

	void adjust_leg(const cxVec& effPos, const int inodeTop, const int inodeRot, const int inodeEnd, const int inodeExt);

	void copy_prev_world();
	void calc_world();
	void calc_root_world();

	xt_xmtx get_node_local_xform(const int inode) const;
	xt_xmtx get_node_prev_world_xform(const int inode) const;
	xt_xmtx calc_node_world_xform(const int inode, xt_xmtx* pParentXform = nullptr) const;
	cxMtx calc_node_world_mtx(const int inode, cxMtx* pParentMtx = nullptr) const;

	void reset_node_local_xform(const int inode);
	void set_node_local_xform(const int inode, const xt_xmtx& lm);
	void set_node_local_tx(const int inode, const float x);
	void set_node_local_ty(const int inode, const float y);
	void set_node_local_tz(const int inode, const float z);
	void set_root_local_tx(const float x) { set_node_local_tx(mRootId, x); }
	void set_root_local_ty(const float y) { set_node_local_ty(mRootId, y); }
	void set_root_local_tz(const float z) { set_node_local_tz(mRootId, z); }

	void disable_node_blending(const int inode, const float disable = true);
	void blend_init(const int duration);
	void blend_exec();

	void set_base_node_ids(const char* pRootName = "root", const char* pMoveName = "n_Move", const char* pCenterName = "n_Center");

	static cxMotionWork* create(sxModelData* pMdlData);
	static void destroy(cxMotionWork* pWk);
};


class cxModelWork {
private:
	cxModelWork() {}

public:
	cxAABB mWorldBBox;
	cxAABB mPrevWorldBBox;
	sxModelData* mpData;
	xt_xmtx* mpWorldXform;
	xt_xmtx* mpSkinXforms;
	cxAABB* mpBatBBoxes;
	uint32_t* mpCullBits;
	uint32_t* mpHideBits;
	void* mpParamMem;
	void* mpExtMem;
	void* mpGPUWk;
	float mRenderMask;
	int mVariation;
	bool mBoundsValid;
	cxResourceManager::Pkg* mpTexPkg;

	bool has_skel() const { return mpData && mpData->has_skel(); }
	bool ck_skel_id(const int iskl) const { return mpData ? mpData->ck_skel_id(iskl) : false; }
	int find_skel_node_id(const char* pName) const { return mpData ? mpData->find_skel_node_id(pName) : -1; }

	xt_xmtx calc_skel_node_world_rest_xform(const int iskl) const { return mpData ? mpData->calc_skel_node_world_xform(iskl, nullptr) : nxMtx::xmtx_identity(); }
	cxMtx calc_skel_node_world_rest_mtx(const int iskl) const { return nxMtx::mtx_from_xmtx(calc_skel_node_world_rest_xform(iskl)); }

	int get_batches_num() const { return mpData ? mpData->mBatNum : 0; }
	bool ck_batch_id(const int ibat) const { return mpData ? mpData->ck_batch_id(ibat) : false; }

	int get_mtls_num() const { return mpData ? mpData->mMtlNum : 0; }
	bool ck_mtl_id(const int imtl) const { return mpData ? mpData->ck_mtl_id(imtl) : false; }
	int find_mtl_id(const char* pMtlName) { return mpData ? mpData->find_material_id(pMtlName) : -1; }
	void hide_mtl(const int imtl, const bool hide = true);
	void hide_mtl(const char* pMtlName, const bool hide = true) { hide_mtl(find_mtl_id(pMtlName), hide); }
	bool is_mtl_hidden(const int imtl) const;
	bool is_bat_mtl_hidden(const int ibat) const;

	void copy_prev_world_bbox() { mPrevWorldBBox = mWorldBBox; }
	void copy_prev_world_xform();
	cxMtx get_prev_world_xform() const;

	void set_pose(const cxMotionWork* pMot);
	void update_bounds();
	void frustum_cull(const cxFrustum* pFst, const bool precise = true);
	bool calc_batch_visibility(const cxFrustum* pFst, const int ibat, const bool precise = true);

	sxTextureData* find_texture(cxResourceManager* pRsrcMgr, const char* pTexName) const;

	static cxModelWork* create(sxModelData* pMdl, const size_t paramMemSize = 0, const size_t extMemSize = 0);
	static void destroy(cxModelWork* pWk);
};


namespace nxApp {

void init_params(int argc, char* argv[]);
void reset();
cxCmdLine* get_cmd_line();
int get_opts_count();
const char* get_opt(const char* pName);
int get_int_opt(const char* pName, const int defVal = 0);
float get_float_opt(const char* pName, const float defVal = 0.0f);
bool get_bool_opt(const char* pName, const bool defVal = false);
int get_args_count();
const char* get_arg(const int idx);

} // nxApp

