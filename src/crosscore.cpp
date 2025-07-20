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

#include "crosscore.hpp"

#ifdef XD_NOSTDLIB
#	undef XD_FILEFUNCS_ENABLED
#	define XD_FILEFUNCS_ENABLED 0
#	undef XD_THREADFUNCS_ENABLED
#	define XD_THREADFUNCS_ENABLED 0
#	undef XD_TIMEFUNCS_ENABLED
#	define XD_TIMEFUNCS_ENABLED 0
#	undef XD_MALLOC_ENABLED
#	define XD_MALLOC_ENABLED 0
#	undef XD_CXXATOMIC_ENABLED
#	define XD_CXXATOMIC_ENABLED 0
#	undef XD_STRFUNCS_INTERNAL
#	define XD_STRFUNCS_INTERNAL 1
#	undef XD_MEMFUNCS_INTERNAL
#	define XD_MEMFUNCS_INTERNAL 1
#	undef XD_NUMPARSE_INTERNAL
#	define XD_NUMPARSE_INTERNAL 1

#else /* XD_NOSTDLIB */

#	ifndef XD_FILEFUNCS_ENABLED
#		define XD_FILEFUNCS_ENABLED 1
#	endif

#	ifndef XD_THREADFUNCS_ENABLED
#		define XD_THREADFUNCS_ENABLED 1
#	endif

#	ifndef XD_TIMEFUNCS_ENABLED
#		define XD_TIMEFUNCS_ENABLED 1
#	endif

#	ifndef XD_MALLOC_ENABLED
#		define XD_MALLOC_ENABLED 1
#	endif

#	ifndef XD_CXXATOMIC_ENABLED
#		define XD_CXXATOMIC_ENABLED 1
#	endif

#	ifndef XD_STRFUNCS_INTERNAL
#		define XD_STRFUNCS_INTERNAL 0
#	endif

#	ifndef XD_MEMFUNCS_INTERNAL
#		define XD_MEMFUNCS_INTERNAL 0
#	endif

#	ifndef XD_NUMPARSE_INTERNAL
#		define XD_NUMPARSE_INTERNAL 0
#	endif

#endif /* XD_NOSTDLIB */

#ifndef XD_TSK_NATIVE
#	define XD_TSK_NATIVE 1
#endif

#if !XD_THREADFUNCS_ENABLED
#	undef XD_TSK_NATIVE
#	define XD_TSK_NATIVE 0
#endif

#ifndef XD_SYNC_MULTI
#	if XD_TSK_NATIVE
#		define XD_SYNC_MULTI 1
#	else
#		define XD_SYNC_MULTI 0
#	endif
#endif

#ifndef XD_CLOCK_MODE
#	define XD_CLOCK_MODE 0
#endif

#if defined(XD_SYS_WINDOWS)
#	undef _WIN32_WINNT
#	define _WIN32_WINNT 0x0500
#	define WIN32_LEAN_AND_MEAN 1
#	undef NOMINMAX
#	define NOMINMAX
#	include <Windows.h>
#	include <tchar.h>
#	if XD_TSK_NATIVE
#		define XD_TSK_NATIVE_WINDOWS
#	endif
#elif defined(XD_SYS_LINUX) || defined(XD_SYS_BSD)
#	if XD_TSK_NATIVE
#		define XD_TSK_NATIVE_PTHREAD
#	endif
#	include <unistd.h>
#	include <time.h>
#elif defined(XD_SYS_ILLUMOS)
#	if XD_TSK_NATIVE
#		define XD_TSK_NATIVE_PTHREAD
#	endif
#	include <unistd.h>
#	include <time.h>
#elif defined(XD_SYS_HAIKU)
#	include <kernel/OS.h>
#	include <time.h>
#	undef XD_TSK_NATIVE
#	define XD_TSK_NATIVE 0
#elif !defined(XD_SYS_NONE)
#	undef XD_TSK_NATIVE
#	define XD_TSK_NATIVE 0
#	include <time.h>
#endif

#ifndef XD_NOSTDLIB
#	if !XD_TSK_NATIVE || defined(XD_SYS_APPLE) || defined(XD_FORCE_CHRONO)
#		include <chrono>
#	endif
#endif

#if XD_TSK_NATIVE
#	if !(defined(XD_TSK_NATIVE_WINDOWS) || defined(XD_TSK_NATIVE_PTHREAD))
#		undef XD_THREADFUNCS_ENABLED
#		define XD_THREADFUNCS_ENABLED 0
#	endif
#else
#	if XD_THREADFUNCS_ENABLED
#		include <thread>
#		include <mutex>
#		include <condition_variable>
#	endif
#endif

#if XD_CXXATOMIC_ENABLED
#	include <atomic>
#endif

#ifdef XD_TSK_NATIVE_PTHREAD
#	include <pthread.h>
#endif

#ifdef XD_SYS_OPENBSD
#	include <sys/types.h>
#	include <sys/sysctl.h>
#endif

#ifdef XD_SYS_SUNOS
#	include <sys/types.h>
#	include <sys/processor.h>
#	include <unistd.h>
#endif


const uint32_t sxValuesData::KIND = XD_FOURCC('X', 'V', 'A', 'L');
const uint32_t sxRigData::KIND = XD_FOURCC('X', 'R', 'I', 'G');
const uint32_t sxGeometryData::KIND = XD_FOURCC('X', 'G', 'E', 'O');
const uint32_t sxImageData::KIND = XD_FOURCC('X', 'I', 'M', 'G');
const uint32_t sxKeyframesData::KIND = XD_FOURCC('X', 'K', 'F', 'R');
const uint32_t sxExprLibData::KIND = XD_FOURCC('X', 'C', 'E', 'L');
const uint32_t sxModelData::KIND = XD_FOURCC('X', 'M', 'D', 'L');
const uint32_t sxTextureData::KIND = XD_FOURCC('X', 'T', 'E', 'X');
const uint32_t sxMotionData::KIND = XD_FOURCC('X', 'M', 'O', 'T');
const uint32_t sxCollisionData::KIND = XD_FOURCC('X', 'C', 'O', 'L');
const uint32_t sxFileCatalogue::KIND = XD_FOURCC('F', 'C', 'A', 'T');

const uint32_t sxDDSHead::ENC_RGBE = XD_FOURCC('R', 'G', 'B', 'E');
const uint32_t sxDDSHead::ENC_BGRE = XD_FOURCC('B', 'G', 'R', 'E');
const uint32_t sxDDSHead::ENC_RGBI = XD_FOURCC('R', 'G', 'B', 'I');
const uint32_t sxDDSHead::ENC_BGRI = XD_FOURCC('B', 'G', 'R', 'I');

const uint32_t sxPackedData::SIG = XD_FOURCC('x', 'p', 'k', 'd');

static const char* s_pXDataMemTag = "xData";

#if XD_THREADFUNCS_ENABLED
static const char* s_pXWorkerTag = "xWorker";
#endif

#if defined(XD_TSK_NATIVE_WINDOWS)
struct sxLock {
	CRITICAL_SECTION mCS;
};

struct sxSignal {
	HANDLE mhEvt;
};

struct sxWorker {
	HANDLE mhThread;
	sxSignal* mpSigExec;
	sxSignal* mpSigDone;
	xt_worker_func mFunc;
	void* mpData;
	DWORD mTID;
	bool mEndFlg;
};
#elif defined(XD_TSK_NATIVE_PTHREAD)
struct sxLock {
	pthread_mutex_t mMutex;
};

struct sxSignal {
	pthread_mutex_t mMutex;
	pthread_cond_t mCndVar;
	bool mState;
};

struct sxWorker {
	sxSignal* mpSigExec;
	sxSignal* mpSigDone;
	xt_worker_func mFunc;
	void* mpData;
	pthread_t mThread;
	bool mEndFlg;
};
#elif XD_THREADFUNCS_ENABLED
struct sxLock {
	std::mutex mMutex;
};

struct sxSignal {
	std::mutex mMutex;
	std::condition_variable mCndVar;
	bool mState;
};

struct sxWorker {
	sxSignal* mpSigExec;
	sxSignal* mpSigDone;
	xt_worker_func mFunc;
	void* mpData;
	std::thread mThread;
	bool mEndFlg;
};
#else
struct sxLock { void* p; };
struct sxSignal { void* p; };
struct sxWorker { void* p; };
#endif


namespace nxSys {

FILE* x_fopen(const char* fpath, const char* mode) {
#if XD_FILEFUNCS_ENABLED
#if defined(_MSC_VER)
	FILE* f;
	::fopen_s(&f, fpath, mode);
	return f;
#else
	return ::fopen(fpath, mode);
#endif
#else
	return nullptr;
#endif
}

void x_strcpy(char* pDst, const size_t dstSize, const char* pSrc) {
#if XD_STRFUNCS_INTERNAL
	if (pDst && pSrc && dstSize > 0) {
		size_t srcSize = nxCore::str_len(pSrc) + 1;
		if (dstSize >= srcSize) {
			for (size_t i = 0; i < srcSize; ++i) {
				pDst[i] = pSrc[i];
			}
		}
	}
#else
#if defined(_MSC_VER)
	::strcpy_s(pDst, dstSize, pSrc);
#else
	::strcpy(pDst, pSrc);
#endif
#endif
}


void* def_malloc(size_t size) {
#if XD_MALLOC_ENABLED
	return ::malloc(size);
#else
	return nullptr;
#endif
}

void def_free(void* p) {
#if XD_MALLOC_ENABLED
	if (p) {
		::free(p);
	}
#endif
}

xt_fhandle def_fopen(const char* fpath) {
	return (xt_fhandle)x_fopen(fpath, "rb");
}

void def_fclose(xt_fhandle fh) {
#if XD_FILEFUNCS_ENABLED
	::fclose((FILE*)fh);
#endif
}

size_t def_fsize(xt_fhandle fh) {
	long len = 0;
#if XD_FILEFUNCS_ENABLED
	FILE* f = (FILE*)fh;
	long old = ::ftell(f);
	if (0 == ::fseek(f, 0, SEEK_END)) {
		len = ::ftell(f);
	}
	::fseek(f, old, SEEK_SET);
#endif
	return (size_t)len;
}

size_t def_fread(xt_fhandle fh, void* pDst, size_t size) {
	size_t nread = 0;
#if XD_FILEFUNCS_ENABLED
	if (fh && pDst && size) {
		FILE* f = (FILE*)fh;
		nread = ::fread(pDst, 1, size, f);
	}
#endif
	return nread;
}

void def_dbgmsg(const char* pMsg) {
}

static sxSysIfc s_ifc {
	def_malloc,
	def_free,
	def_fopen,
	def_fclose,
	def_fsize,
	def_fread,
	def_dbgmsg,
	nullptr, // micros
	nullptr // sleep
};

void init(sxSysIfc* pIfc) {
	if (pIfc) {
		nxCore::mem_copy(&s_ifc, pIfc, sizeof(sxSysIfc));
	}
}

void* malloc(size_t size) {
	void* p = nullptr;
	if (s_ifc.fn_malloc) {
		p = s_ifc.fn_malloc(size);
	} else {
		p = def_malloc(size);
	}
	return p;
}

void free(void* p) {
	if (s_ifc.fn_free) {
		s_ifc.fn_free(p);
	} else {
		def_free(p);
	}
}

xt_fhandle fopen(const char* fpath) {
	xt_fhandle fh = nullptr;
	if (s_ifc.fn_fopen) {
		fh = s_ifc.fn_fopen(fpath);
	} else {
		fh = def_fopen(fpath);
	}
	return fh;
}

void fclose(xt_fhandle fh) {
	if (s_ifc.fn_fclose) {
		s_ifc.fn_fclose(fh);
	} else {
		def_fclose(fh);
	}
}

size_t fsize(xt_fhandle fh) {
	size_t size = 0;
	if (s_ifc.fn_fsize) {
		size = s_ifc.fn_fsize(fh);
	} else {
		size = def_fsize(fh);
	}
	return size;
}

size_t fread(xt_fhandle fh, void* pDst, size_t size) {
	size_t nread = 0;
	if (s_ifc.fn_fread) {
		nread = s_ifc.fn_fread(fh, pDst, size);
	} else {
		nread = def_fread(fh, pDst, size);
	}
	return nread;
}

void dbgmsg(const char* pMsg) {
	if (s_ifc.fn_dbgmsg) {
		s_ifc.fn_dbgmsg(pMsg);
	} else {
		def_dbgmsg(pMsg);
	}
}

static const char* s_pDecDigits = "0123456789";
static const char* s_pHexDigits = "0123456789ABCDEF";

XD_NOINLINE void dbgmsg_u32(const uint32_t x) {
	char buf[16];
	uint32_t val = x;
	char* p = buf + (sizeof(buf) - 1);
	*p-- = 0;
	while (true) {
		char c = s_pDecDigits[val % 10];
		*p-- = c;
		val /= 10;
		if (0 == val) break;
	}
	dbgmsg(p + 1);
}

XD_NOINLINE void dbgmsg_i32(const int32_t x) {
	char buf[16];
	int32_t val = x;
	bool sgn = val < 0;
	if (sgn) val = -val;
	char* p = buf + (sizeof(buf) - 1);
	*p-- = 0;
	while (true) {
		char c = s_pDecDigits[val % 10];
		*p-- = c;
		val /= 10;
		if (0 == val) break;
	}
	if (sgn) {
		*p-- = '-';
	}
	dbgmsg(p + 1);
}

template<typename T> inline void t_dbgmsg_hex(const T x) {
	char buf[48];
	char* p = buf;
	for (int i = int(sizeof(T))*2; --i >= 0;) {
		*p++ = s_pHexDigits[(x >> (i << 2)) & 0xF];
	}
	*p = 0;
	dbgmsg(buf);
}

XD_NOINLINE void dbgmsg_u32_hex(const uint32_t x) { t_dbgmsg_hex(x); }
XD_NOINLINE void dbgmsg_u64_hex(const uint64_t x) { t_dbgmsg_hex(x); }

void dbgmsg_ptr(const void* p) {
	t_dbgmsg_hex((uintptr_t)p);
}

XD_NOINLINE void dbgmsg_f32(const float x, const int nfrc) {
	char buf[64];
	float val = x;
	bool sgn = (val < 0.0f);
	if (sgn) {
		val = -val;
	}
	float s = 1000000.0f;
	if (nfrc > 0 && nfrc < 10) {
		s = nxCalc::ipow(10.0f, nfrc);
	}
	uint32_t i = (uint32_t)val;
	uint32_t f = (uint32_t)((val - float(i))*s + s);
	char* p = buf + (sizeof(buf) - 1);
	*p-- = 0;
	while (true) {
		char c = s_pDecDigits[f % 10];
		*p-- = c;
		f /= 10;
		if (0 == f) break;
	}
	p[1] = '.';
	while (true) {
		char c = s_pDecDigits[i % 10];
		*p-- = c;
		i /= 10;
		if (0 == i) break;
	}
	if (sgn) {
		*p-- = '-';
	}
	dbgmsg(p + 1);
}

void dbgmsg_eol() { nxSys::dbgmsg("\n"); }

FILE* fopen_w_txt(const char* fpath) {
	return x_fopen(fpath, "w");
}

FILE* fopen_w_bin(const char* fpath) {
	return x_fopen(fpath, "wb");
}

XD_NOINLINE double time_micros() {
	double ms = 0.0f;
	if (s_ifc.fn_micros) {
		ms = s_ifc.fn_micros();
	} else {
#if XD_TIMEFUNCS_ENABLED
#	if defined(XD_SYS_WINDOWS)
		LARGE_INTEGER frq;
		if (QueryPerformanceFrequency(&frq)) {
			LARGE_INTEGER ctr;
			QueryPerformanceCounter(&ctr);
			ms = ((double)ctr.QuadPart / (double)frq.QuadPart) * 1.0e6;
		}
#	elif defined(XD_SYS_APPLE) || defined(XD_FORCE_CHRONO)
		using namespace std::chrono;
		auto t = high_resolution_clock::now();
		ms = (double)duration_cast<nanoseconds>(t.time_since_epoch()).count() * 1.0e-3;
#	else
#		if XD_CLOCK_MODE == 1
		timespec t;
		clock_gettime(CLOCK_MONOTONIC_RAW, &t);
		ms = (double)t.tv_nsec*1.0e-3 + (double)t.tv_sec*1.0e6;
#		else
		timespec t;
		if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) {
			clock_gettime(CLOCK_REALTIME, &t);
		}
		ms = (double)t.tv_nsec*1.0e-3 + (double)t.tv_sec*1.0e6;
#		endif
#	endif
#endif
	}
	return ms;
}

XD_NOINLINE void sleep_millis(uint32_t millis) {
	if (s_ifc.fn_sleep) {
		s_ifc.fn_sleep(millis);
	} else {
#if XD_TIMEFUNCS_ENABLED
#	if defined(XD_TSK_NATIVE_WINDOWS)
		::Sleep(millis);
#	elif defined(XD_TSK_NATIVE_PTHREAD)
		if (millis < 1000) {
			timespec req;
			timespec rem;
			req.tv_sec = 0;
			req.tv_nsec = millis * 1000000;
			::nanosleep(&req, &rem);
		} else {
			::usleep(millis * 1000);
		}
#	elif XD_THREADFUNCS_ENABLED
		using namespace std;
		this_thread::sleep_for(chrono::milliseconds(millis));
#	endif
#endif
	}
}

XD_NOINLINE int num_active_cpus() {
	int ncpu = 1;
#if defined(XD_SYS_LINUX) || defined(XD_SYS_FREEBSD) || defined(XD_SYS_SUNOS)
	ncpu = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(XD_SYS_OPENBSD)
	int cmd[] = { CTL_HW, HW_NCPUONLINE };
	size_t clen = sizeof(ncpu);
	sysctl(cmd, XD_ARY_LEN(cmd), &ncpu, &clen, nullptr, 0);
#elif defined(XD_SYS_WINDOWS)
	SYSTEM_INFO si;
	nxCore::mem_zero(&si, sizeof(SYSTEM_INFO));
	GetSystemInfo(&si);
	ncpu = (int)si.dwNumberOfProcessors;
#elif defined(XD_SYS_HAIKU)
	system_info si;
	get_system_info(&si);
	ncpu = si.cpu_count;
#endif
	return ncpu;
}

#if defined(XD_TSK_NATIVE_WINDOWS)
sxLock* lock_create() {
	sxLock* pLock = (sxLock*)nxCore::mem_alloc(sizeof(sxLock), "xLock");
	if (pLock) {
		::InitializeCriticalSection(&pLock->mCS);
	}
	return pLock;
}

void lock_destroy(sxLock* pLock) {
	if (pLock) {
		::DeleteCriticalSection(&pLock->mCS);
		nxCore::mem_free(pLock);
	}
}

bool lock_acquire(sxLock* pLock) {
	bool res = false;
	if (pLock) {
		::EnterCriticalSection(&pLock->mCS);
		res = true;
	}
	return res;
}

bool lock_release(sxLock* pLock) {
	bool res = false;
	if (pLock) {
		::LeaveCriticalSection(&pLock->mCS);
		res = true;
	}
	return res;
}


sxSignal* signal_create() {
	sxSignal* pSig = (sxSignal*)nxCore::mem_alloc(sizeof(sxSignal), "xSignal");
	if (pSig) {
		pSig->mhEvt = ::CreateEventA(NULL, FALSE, FALSE, NULL);
	}
	return pSig;
}

void signal_destroy(sxSignal* pSig) {
	if (pSig) {
		::CloseHandle(pSig->mhEvt);
		nxCore::mem_free(pSig);
	}
}

bool signal_wait(sxSignal* pSig) {
	bool res = false;
	if (pSig) {
		res = ::WaitForSingleObject(pSig->mhEvt, INFINITE) == WAIT_OBJECT_0;
	}
	return res;
}

bool signal_set(sxSignal* pSig) {
	return pSig ? !!::SetEvent(pSig->mhEvt) : false;
}

bool signal_reset(sxSignal* pSig) {
	return pSig ? !!::ResetEvent(pSig->mhEvt) : false;
}


static DWORD APIENTRY wnd_wrk_entry(void* pSelf) {
	sxWorker* pWrk = (sxWorker*)pSelf;
	if (!pWrk) return 1;
	signal_set(pWrk->mpSigDone);
	while (!pWrk->mEndFlg) {
		if (signal_wait(pWrk->mpSigExec)) {
			signal_reset(pWrk->mpSigExec);
			if (!pWrk->mEndFlg && pWrk->mFunc) {
				pWrk->mFunc(pWrk->mpData);
			}
			signal_set(pWrk->mpSigDone);
		}
	}
	return 0;
}

sxWorker* worker_create(xt_worker_func func, void* pData) {
	sxWorker* pWrk = (sxWorker*)nxCore::mem_alloc(sizeof(sxWorker), s_pXWorkerTag);
	if (pWrk) {
		pWrk->mFunc = func;
		pWrk->mpData = pData;
		pWrk->mpSigExec = signal_create();
		pWrk->mpSigDone = signal_create();
		pWrk->mEndFlg = false;
		pWrk->mhThread = ::CreateThread(NULL, 0, wnd_wrk_entry, pWrk, CREATE_SUSPENDED, &pWrk->mTID);
		if (pWrk->mhThread) {
			::ResumeThread(pWrk->mhThread);
		}
	}
	return pWrk;
}

void worker_destroy(sxWorker* pWrk) {
	if (pWrk) {
		worker_stop(pWrk);
		signal_destroy(pWrk->mpSigDone);
		signal_destroy(pWrk->mpSigExec);
		nxCore::mem_free(pWrk);
	}
}

void worker_exec(sxWorker* pWrk) {
	if (pWrk) {
		signal_reset(pWrk->mpSigDone);
		signal_set(pWrk->mpSigExec);
	}
}

void worker_wait(sxWorker* pWrk) {
	if (pWrk) {
		signal_wait(pWrk->mpSigDone);
	}
}

void worker_stop(sxWorker* pWrk) {
	if (pWrk && !pWrk->mEndFlg) {
		pWrk->mEndFlg = true;
		worker_exec(pWrk);
		worker_wait(pWrk);
		::WaitForSingleObject(pWrk->mhThread, INFINITE);
		::CloseHandle(pWrk->mhThread);
	}
}

#elif defined(XD_TSK_NATIVE_PTHREAD)

sxLock* lock_create() {
	sxLock* pLock = (sxLock*)nxCore::mem_alloc(sizeof(sxLock), "xLock");
	if (pLock) {
		pthread_mutex_init(&pLock->mMutex, nullptr);
	}
	return pLock;
}

void lock_destroy(sxLock* pLock) {
	if (pLock) {
		pthread_mutex_destroy(&pLock->mMutex);
		nxCore::mem_free(pLock);
	}
}

bool lock_acquire(sxLock* pLock) {
	bool res = false;
	if (pLock) {
		if (pthread_mutex_lock(&pLock->mMutex) == 0) {
			res = true;
		}
	}
	return res;
}

bool lock_release(sxLock* pLock) {
	bool res = false;
	if (pLock) {
		if (pthread_mutex_unlock(&pLock->mMutex) == 0) {
			res = true;
		}
	}
	return res;
}


sxSignal* signal_create() {
	sxSignal* pSig = (sxSignal*)nxCore::mem_alloc(sizeof(sxSignal), "xSignal");
	if (pSig) {
		pthread_mutex_init(&pSig->mMutex, nullptr);
		pthread_cond_init(&pSig->mCndVar, nullptr);
		pSig->mState = false;
	}
	return pSig;
}

void signal_destroy(sxSignal* pSig) {
	if (pSig) {
		pthread_cond_destroy(&pSig->mCndVar);
		pthread_mutex_destroy(&pSig->mMutex);
		nxCore::mem_free(pSig);
	}
}

bool signal_wait(sxSignal* pSig) {
	bool res = false;
	if (pSig) {
		if (pthread_mutex_lock(&pSig->mMutex) != 0) return false;
		if (!pSig->mState) {
			while (true) {
				res = pthread_cond_wait(&pSig->mCndVar, &pSig->mMutex) == 0;
				if (!res || pSig->mState) break;
			}
			if (res) {
				pSig->mState = false;
			}
		} else {
			res = true;
			pSig->mState = false;
		}
		pthread_mutex_unlock(&pSig->mMutex);
	}
	return res;
}

bool signal_set(sxSignal* pSig) {
	if (!pSig) return false;
	if (pthread_mutex_lock(&pSig->mMutex) != 0) return false;
	pSig->mState = true;
	if (pSig->mState) {
		pthread_mutex_unlock(&pSig->mMutex);
		pthread_cond_signal(&pSig->mCndVar);
	}
	return true;
}

bool signal_reset(sxSignal* pSig) {
	if (!pSig) return false;
	if (pthread_mutex_lock(&pSig->mMutex) != 0) return false;
	pSig->mState = false;
	if (pthread_mutex_unlock(&pSig->mMutex) != 0) return false;
	return true;
}


static void* pthread_wrk_func(void* pSelf) {
	sxWorker* pWrk = (sxWorker*)pSelf;
	if (!pWrk) return (void*)1;
	signal_set(pWrk->mpSigDone);
	while (!pWrk->mEndFlg) {
		if (signal_wait(pWrk->mpSigExec)) {
			if (!pWrk->mEndFlg && pWrk->mFunc) {
				pWrk->mFunc(pWrk->mpData);
			}
			signal_set(pWrk->mpSigDone);
		}
	}
	return (void*)0;
}

sxWorker* worker_create(xt_worker_func func, void* pData) {
	sxWorker* pWrk = (sxWorker*)nxCore::mem_alloc(sizeof(sxWorker), s_pXWorkerTag);
	if (pWrk) {
		pWrk->mFunc = func;
		pWrk->mpData = pData;
		pWrk->mpSigExec = signal_create();
		pWrk->mpSigDone = signal_create();
		pWrk->mEndFlg = false;
		pthread_create(&pWrk->mThread, nullptr, pthread_wrk_func, pWrk);
#if defined(XD_SYS_LINUX)
		pthread_setname_np(pWrk->mThread, s_pXWorkerTag);
#endif
	}
	return pWrk;
}

void worker_destroy(sxWorker* pWrk) {
	if (pWrk) {
		worker_stop(pWrk);
		signal_destroy(pWrk->mpSigDone);
		signal_destroy(pWrk->mpSigExec);
		nxCore::mem_free(pWrk);
	}
}

void worker_exec(sxWorker* pWrk) {
	if (pWrk) {
		signal_reset(pWrk->mpSigDone);
		signal_set(pWrk->mpSigExec);
	}
}

void worker_wait(sxWorker* pWrk) {
	if (pWrk) {
		signal_wait(pWrk->mpSigDone);
	}
}

void worker_stop(sxWorker* pWrk) {
	if (pWrk && !pWrk->mEndFlg) {
		void* exitRes = (void*)-1;
		pWrk->mEndFlg = true;
		worker_exec(pWrk);
		worker_wait(pWrk);
		pthread_join(pWrk->mThread, &exitRes);
	}
}

#elif XD_THREADFUNCS_ENABLED

sxLock* lock_create() {
	sxLock* pLock = (sxLock*)nxCore::mem_alloc(sizeof(sxLock), "xLock");
	if (pLock) {
		::new ((void*)pLock) sxLock;
	}
	return pLock;
}

void lock_destroy(sxLock* pLock) {
	if (pLock) {
		pLock->~sxLock();
		nxCore::mem_free(pLock);
	}
}

bool lock_acquire(sxLock* pLock) {
	bool res = false;
	if (pLock) {
		try {
			pLock->mMutex.lock();
			res = true;
		} catch (...) {
			res = false;
		}
	}
	return res;
}

bool lock_release(sxLock* pLock) {
	bool res = false;
	if (pLock) {
		pLock->mMutex.unlock();
		res = true;
	}
	return res;
}


sxSignal* signal_create() {
	sxSignal* pSig = (sxSignal*)nxCore::mem_alloc(sizeof(sxSignal), "xSignal");
	if (pSig) {
		::new ((void*)pSig) sxSignal;
		pSig->mState = false;
	}
	return pSig;
}

void signal_destroy(sxSignal* pSig) {
	if (pSig) {
		pSig->~sxSignal();
		nxCore::mem_free(pSig);
	}
}

bool signal_wait(sxSignal* pSig) {
	bool res = false;
	if (pSig) {
		std::unique_lock<std::mutex> ulk(pSig->mMutex);
		if (!pSig->mState) {
			while (true) {
				pSig->mCndVar.wait(ulk);
				res = true;
				if (pSig->mState) break;
			}
			if (res) {
				pSig->mState = false;
			}
		} else {
			res = true;
			pSig->mState = false;
		}
	}
	return res;
}

bool signal_set(sxSignal* pSig) {
	if (!pSig) return false;
	pSig->mMutex.lock();
	pSig->mState = true;
	if (pSig->mState) {
		pSig->mMutex.unlock();
		pSig->mCndVar.notify_one();
	}
	return true;
}

bool signal_reset(sxSignal* pSig) {
	if (!pSig) return false;
	pSig->mMutex.lock();
	pSig->mState = false;
	pSig->mMutex.unlock();
	return true;
}


static void std_wrk_func(sxWorker* pWrk) {
	if (!pWrk) return;
	signal_set(pWrk->mpSigDone);
	while (!pWrk->mEndFlg) {
		if (signal_wait(pWrk->mpSigExec)) {
			if (!pWrk->mEndFlg && pWrk->mFunc) {
				pWrk->mFunc(pWrk->mpData);
			}
			signal_set(pWrk->mpSigDone);
		}
	}
}

sxWorker* worker_create(xt_worker_func func, void* pData) {
	sxWorker* pWrk = (sxWorker*)nxCore::mem_alloc(sizeof(sxWorker), s_pXWorkerTag);
	if (pWrk) {
		::new ((void*)pWrk) sxWorker;
		pWrk->mFunc = func;
		pWrk->mpData = pData;
		pWrk->mpSigExec = signal_create();
		pWrk->mpSigDone = signal_create();
		pWrk->mEndFlg = false;
		::new ((void*)&pWrk->mThread) std::thread(std_wrk_func, pWrk);
	}
	return pWrk;
}

void worker_destroy(sxWorker* pWrk) {
	if (pWrk) {
		worker_stop(pWrk);
		signal_destroy(pWrk->mpSigDone);
		signal_destroy(pWrk->mpSigExec);
		pWrk->~sxWorker();
		nxCore::mem_free(pWrk);
	}
}

void worker_exec(sxWorker* pWrk) {
	if (pWrk) {
		signal_reset(pWrk->mpSigDone);
		signal_set(pWrk->mpSigExec);
	}
}

void worker_wait(sxWorker* pWrk) {
	if (pWrk) {
		signal_wait(pWrk->mpSigDone);
	}
}

void worker_stop(sxWorker* pWrk) {
	if (pWrk && !pWrk->mEndFlg) {
		pWrk->mEndFlg = true;
		worker_exec(pWrk);
		worker_wait(pWrk);
		pWrk->mThread.join();
	}
}

#else

sxLock* lock_create() { return nullptr; }
void lock_destroy(sxLock* pLock) { }
bool lock_acquire(sxLock* pLock) { return true; }
bool lock_release(sxLock* pLock) { return true; }
sxSignal* signal_create() { return nullptr; }
void signal_destroy(sxSignal* pSig) { }
bool signal_wait(sxSignal* pSig) { return true; }
bool signal_set(sxSignal* pSig) { return true; }
bool signal_reset(sxSignal* pSig) { return true; }
sxWorker* worker_create(xt_worker_func func, void* pData) { return nullptr; }
void worker_destroy(sxWorker* pWrk) { }
void worker_exec(sxWorker* pWrk) { }
void worker_wait(sxWorker* pWrk) { }
void worker_stop(sxWorker* pWrk) { }

#endif // XD_TSK_NATIVE_*


#if !defined(XD_MSC_ATOMIC)
#if XD_CXXATOMIC_ENABLED
int32_t atomic_inc(int32_t* p) {
	auto pA = (std::atomic<int32_t>*)p;
	return pA->fetch_add(1) + 1;
}
int32_t atomic_dec(int32_t* p) {
	auto pA = (std::atomic<int32_t>*)p;
	return pA->fetch_sub(1) - 1;
}
int32_t atomic_add(int32_t* p, const int32_t val) {
	auto pA = (std::atomic<int32_t>*)p;
	return pA->fetch_add(val) + val;
}
#elif defined(__GNUC__) && !defined(XD_NOSTDLIB)
int32_t atomic_inc(int32_t* p) {
	return __atomic_fetch_add(p, 1, __ATOMIC_SEQ_CST) + 1;
}
int32_t atomic_dec(int32_t* p) {
	return __atomic_fetch_sub(p, 1, __ATOMIC_SEQ_CST) - 1;
}
int32_t atomic_add(int32_t* p, const int32_t val) {
	return __atomic_fetch_add(p, val, __ATOMIC_SEQ_CST) + val;
}
#else
int32_t atomic_inc(int32_t* p) {
	int32_t res = *p;
	*p += 1;
	return res + 1;
}
int32_t atomic_dec(int32_t* p) {
	int32_t res = *p;
	*p -= 1;
	return res - 1;
}
int32_t atomic_add(int32_t* p, const int32_t val) {
	int32_t res = *p;
	*p += val;
	return res + val;
}
#endif
#endif


bool is_LE() {
	static const char* pRef = "LECK";
	uxVal32 ccLE;
	for (int i = 0; i < 4; ++i) {
		ccLE.b[i] = pRef[i];
	}
	uint32_t fcc = XD_FOURCC(pRef[0], pRef[1], pRef[2], pRef[3]);
	return ccLE.u == fcc;
}

} // nxSys


namespace nxCore {

struct sxMemInfo {
	sxMemInfo* mpPrev;
	sxMemInfo* mpNext;
	size_t mSize;
	size_t mRawSize;
	uint32_t mOffs;
	uint32_t mAlgn;
};

static sxMemInfo* s_pMemHead = nullptr;
static sxMemInfo* s_pMemTail = nullptr;
static uint32_t s_allocCount = 0;
static uint64_t s_allocBytes = 0;
static uint64_t s_allocPeakBytes = 0;
static bool s_memInfoCkEnabled = false;

sxMemInfo* mem_info_from_addr(void* pMem) {
	sxMemInfo* pMemInfo = nullptr;
	if (pMem) {
		uint8_t* pOffs = (uint8_t*)pMem - sizeof(uint32_t);
		uint32_t offs = 0;
		for (int i = 0; i < 4; ++i) {
			offs |= pOffs[i] << (i * 8);
		}
		pMemInfo = (sxMemInfo*)((uint8_t*)pMem - offs);
	}
	return pMemInfo;
}

void* mem_addr_from_info(sxMemInfo* pInfo) {
	void* p = nullptr;
	if (pInfo) {
		p = (uint8_t*)pInfo + pInfo->mOffs;
	}
	return p;
}

sxMemInfo* find_mem_info(void* pMem) {
	sxMemInfo* pMemInfo = nullptr;
	sxMemInfo* pCkInfo = mem_info_from_addr(pMem);
	if (s_memInfoCkEnabled) {
		sxMemInfo* pWkInfo = s_pMemHead;
		while (pWkInfo) {
			if (pWkInfo == pCkInfo) {
				pMemInfo = pWkInfo;
				pWkInfo = nullptr;
			} else {
				pWkInfo = pWkInfo->mpNext;
			}
		}
	} else {
		pMemInfo = pCkInfo;
	}
	return pMemInfo;
}

void* mem_alloc(const size_t size, const char* pTag, int alignment) {
	void* p = nullptr;
	if (alignment < 1) alignment = 0x10;
	if (size > 0) {
		if (!pTag) pTag = "xMem";
		size_t tagLen = nxCore::str_len(pTag) + 1;
		size_t asize = nxCore::align_pad(sizeof(sxMemInfo) + tagLen + size + alignment + sizeof(uint32_t), alignment);
		void* p0 = nxSys::malloc(asize);
		if (p0) {
			p = (void*)nxCore::align_pad((uintptr_t)((uint8_t*)p0 + sizeof(sxMemInfo) + tagLen + sizeof(uint32_t)), alignment);
			uint32_t offs = (uint32_t)((uint8_t*)p - (uint8_t*)p0);
			uint8_t* pOffs = (uint8_t*)p - sizeof(uint32_t);
			pOffs[0] = offs & 0xFF;
			pOffs[1] = (offs >> 8) & 0xFF;
			pOffs[2] = (offs >> 16) & 0xFF;
			pOffs[3] = (offs >> 24) & 0xFF;
			char* pTagDst = (char*)XD_INCR_PTR(p0, sizeof(sxMemInfo));
			for (size_t i = 0; i < tagLen; ++i) {
				pTagDst[i] = pTag[i];
			}
			sxMemInfo* pInfo = mem_info_from_addr(p);
			pInfo->mRawSize = asize;
			pInfo->mSize = size;
			pInfo->mOffs = offs;
			pInfo->mAlgn = alignment;
			pInfo->mpPrev = nullptr;
			pInfo->mpNext = nullptr;
			if (!s_pMemHead) {
				s_pMemHead = pInfo;
			} else {
				pInfo->mpPrev = s_pMemTail;
			}
			if (s_pMemTail) {
				s_pMemTail->mpNext = pInfo;
			}
			s_pMemTail = pInfo;
			s_allocBytes += pInfo->mSize;
			s_allocPeakBytes = nxCalc::max(s_allocPeakBytes, s_allocBytes);
			++s_allocCount;
		}
	}
	return p;
}

void* mem_realloc(void* pMem, size_t newSize, int alignment) {
	void* pNewMem = pMem;
	if (pMem && newSize) {
		sxMemInfo* pMemInfo = find_mem_info(pMem);
		if (pMemInfo) {
			const char* pTag = mem_tag(pMem);
			size_t oldSize = pMemInfo->mSize;
			size_t cpySize = nxCalc::min(oldSize, newSize);
			if (alignment < 1) alignment = pMemInfo->mAlgn;
			void* p = mem_alloc(newSize, pTag, alignment);
			if (p) {
				pNewMem = p;
				mem_copy(pNewMem, pMem, cpySize);
				mem_free(pMem);
			}
		} else {
			dbg_msg("invalid realloc request: %p\n", pMem);
		}
	}
	return pNewMem;
}

void* mem_resize(void* pMem, float factor, int alignment) {
	void* pNewMem = pMem;
	if (pMem && factor > 0.0f) {
		sxMemInfo* pMemInfo = find_mem_info(pMem);
		if (pMemInfo) {
			const char* pTag = mem_tag(pMem);
			size_t oldSize = pMemInfo->mSize;
			size_t newSize = (size_t)::mth_ceilf((float)oldSize * factor);
			size_t cpySize = nxCalc::min(oldSize, newSize);
			if (alignment < 1) alignment = pMemInfo->mAlgn;
			void* p = mem_alloc(newSize, pTag, alignment);
			if (p) {
				pNewMem = p;
				mem_copy(pNewMem, pMem, cpySize);
				mem_free(pMem);
			}
		} else {
			dbg_msg("memory @ %p cannot be resized\n", pMem);
		}
	}
	return pNewMem;
}

void mem_free(void* pMem) {
	if (!pMem) return;
	sxMemInfo* pInfo = find_mem_info(pMem);
	if (!pInfo) {
		dbg_msg("cannot free memory @ %p\n", pMem);
		return;
	}
	sxMemInfo* pNext = pInfo->mpNext;
	if (pInfo->mpPrev) {
		pInfo->mpPrev->mpNext = pNext;
		if (pNext) {
			pNext->mpPrev = pInfo->mpPrev;
		} else {
			s_pMemTail = pInfo->mpPrev;
		}
	} else {
		s_pMemHead = pNext;
		if (pNext) {
			pNext->mpPrev = nullptr;
		} else {
			s_pMemTail = nullptr;
		}
	}
	s_allocBytes -= pInfo->mSize;
	void* p0 = (void*)((intptr_t)pMem - pInfo->mOffs);
	nxSys::free(p0);
	--s_allocCount;
}

size_t mem_size(void* pMem) {
	size_t size = 0;
	sxMemInfo* pMemInfo = find_mem_info(pMem);
	if (pMemInfo) {
		size = pMemInfo->mSize;
	}
	return size;
}

const char* mem_tag(void* pMem) {
	const char* pTag = nullptr;
	sxMemInfo* pMemInfo = find_mem_info(pMem);
	if (pMemInfo) {
		pTag = (const char*)XD_INCR_PTR(pMemInfo, sizeof(sxMemInfo));
	}
	return pTag;
}

void mem_info_check_enable(const bool flg) {
	s_memInfoCkEnabled = flg;
}

void mem_dbg() {
	dbg_msg("%d allocs\n", s_allocCount);
	char xdataKind[5];
	xdataKind[4] = 0;
	sxMemInfo* pInfo = s_pMemHead;
	while (pInfo) {
		void* pMem = mem_addr_from_info(pInfo);
		const char* pTag = mem_tag(pMem);
		dbg_msg("%p: %s, size=0x%X (0x%X, 0x%X)\n", pMem, pTag, pInfo->mSize, pInfo->mRawSize, pInfo->mAlgn);
		if (nxCore::str_eq(pTag, s_pXDataMemTag)) {
			sxData* pData = (sxData*)pMem;
			mem_copy(xdataKind, &pData->mKind, 4);
			dbg_msg("  %s: %s\n", xdataKind, pData->get_file_path());
		}
		pInfo = pInfo->mpNext;
	}
}

uint64_t mem_allocated_bytes() {
	return s_allocBytes;
}

uint64_t mem_peak_bytes() {
	return s_allocPeakBytes;
}

void mem_zero(void* pDst, size_t dstSize) {
#if XD_MEMFUNCS_INTERNAL
	if (pDst) {
		uint8_t* pd = (uint8_t*)pDst;
		for (size_t i = 0; i < dstSize; ++i) {
			*pd++ = 0;
		}
	}
#else
	if (pDst && dstSize > 0) {
		::memset(pDst, 0, dstSize);
	}
#endif
}

void mem_fill(void* pDst, uint8_t fillVal, size_t dstSize) {
#if XD_MEMFUNCS_INTERNAL
	if (pDst) {
		uint8_t* pd = (uint8_t*)pDst;
		for (size_t i = 0; i < dstSize; ++i) {
			*pd++ = fillVal;
		}
	}
#else
	if (pDst && dstSize > 0) {
		::memset(pDst, (int)fillVal, dstSize);
	}
#endif
}

#if XD_MEMFUNCS_INTERNAL
XD_FORCEINLINE static void x_memcopy_sub(uint8_t* pDst, const uint8_t* pSrc, const size_t len) {
	for (size_t i = 0; i < len; ++i) {
		*pDst++ = *pSrc++;
	}
}

XD_FORCEINLINE static int x_memcompare_sub(const void* pSrc1, const void* pSrc2, const size_t len) {
	int res = 0;
	const uint8_t* p1 = (const uint8_t*)pSrc1;
	const uint8_t* p2 = (const uint8_t*)pSrc2;
	size_t i;
	for (i = 0; i < len; ++i) {
		if (*p1 != *p2) {
			res = *p1 < *p2 ? -1 : 1;
			break;
		}
		++p1;
		++p2;
	}
	return res;
}
#endif

void mem_copy(void* pDst, const void* pSrc, size_t cpySize) {
	if (pDst && pSrc && cpySize > 0) {
#if XD_MEMFUNCS_INTERNAL
		x_memcopy_sub((uint8_t*)pDst, (const uint8_t*)pSrc, cpySize);
#else
		::memcpy(pDst, pSrc, cpySize);
#endif
	}
}

bool mem_eq(const void* pSrc1, const void* pSrc2, size_t memSize) {
	bool res = false;
	if (pSrc1 && pSrc2 && memSize > 0) {
#if XD_MEMFUNCS_INTERNAL
		res = x_memcompare_sub(pSrc1, pSrc2, memSize) == 0;
#else
		res = ::memcmp(pSrc1, pSrc2, memSize) == 0;
#endif
	}
	return res;
}

void dbg_break(const char* pMsg) {
}

void dbg_msg(const char* pFmt, ...) {
	char msg[1024 * 2];
	va_list argLst;
	va_start(argLst, pFmt);
	int n = 0;
#ifndef XD_NOSTDLIB
#if defined(_MSC_VER)
	n = ::vsprintf_s(msg, sizeof(msg), pFmt, argLst);
#else
	n = ::vsprintf(msg, pFmt, argLst);
#endif
#endif
	va_end(argLst);
	if (n > 0) {
		if (msg[0] == '[') {
			if (nxCore::str_starts_with(msg, "[BREAK]")) {
				dbg_break(msg);
			}
		}
		nxSys::dbgmsg(msg);
	}
}

static void* bin_load_impl(const char* pPath, size_t* pSize, bool appendPath, bool unpack, bool recursive, const char* pTag) {
	void* pData = nullptr;
	size_t size = 0;
	xt_fhandle fh = nxSys::fopen(pPath);
	if (fh) {
		size_t pathLen = nxCore::str_len(pPath);
		size_t fsize = nxSys::fsize(fh);
		size_t memsize = fsize;
		if (appendPath) {
			memsize += pathLen + 1;
		}
		pData = mem_alloc(memsize, pTag);
		if (pData) {
			size = nxSys::fread(fh, pData, fsize);
		}
		nxSys::fclose(fh);
		if (pData && unpack && fsize > sizeof(sxPackedData)) {
			sxPackedData* pPkd = (sxPackedData*)pData;
			if (pPkd->mSig == sxPackedData::SIG && pPkd->mPackSize == fsize) {
				size_t memsize0 = memsize;
				if (recursive) {
					uint8_t* pUnpkd = nxData::unpack(pPkd, pTag, nullptr, 0, &memsize, true);
					if (pUnpkd) {
						size = memsize;
						fsize = size;
						mem_free(pData);
						pData = pUnpkd;
						if (appendPath) {
							memsize += pathLen + 1;
							pData = mem_alloc(memsize, pTag);
							mem_copy(pData, pUnpkd, size);
							mem_free(pUnpkd);
						}
					} else {
						memsize = memsize0;
					}
				} else {
					memsize = pPkd->mRawSize;
					if (appendPath) {
						memsize += pathLen + 1;
					}
					uint8_t* pUnpkd = (uint8_t*)mem_alloc(memsize, pTag);
					if (nxData::unpack(pPkd, pTag, pUnpkd, (uint32_t)memsize)) {
						size = pPkd->mRawSize;
						fsize = size;
						mem_free(pData);
						pData = pUnpkd;
					} else {
						mem_free(pUnpkd);
						memsize = memsize0;
					}
				}
			}
		}
		if (pData && appendPath) {
			nxSys::x_strcpy(&((char*)pData)[fsize], pathLen + 1, pPath);
		}
	}
	if (pSize) {
		*pSize = size;
	}
	return pData;
}

void* bin_load(const char* pPath, size_t* pSize, bool appendPath, bool unpack) {
	return bin_load_impl(pPath, pSize, appendPath, unpack, true, "xBin");
}

void bin_unload(void* pMem) {
	mem_free(pMem);
}

void bin_save(const char* pPath, const void* pMem, size_t size) {
#if XD_FILEFUNCS_ENABLED
	if (!pPath || !pMem || !size) return;
	FILE* f = nxSys::fopen_w_bin(pPath);
	if (f) {
		::fwrite(pMem, 1, size, f);
		::fclose(f);
	}
#endif
}

void* raw_bin_load(const char* pPath, size_t* pSize) {
	void* pData = nullptr;
	size_t size = 0;
#if XD_FILEFUNCS_ENABLED
	if (pPath) {
		xt_fhandle fh = nxSys::def_fopen(pPath);
		if (fh) {
			size_t fsize = nxSys::def_fsize(fh);
			if (fsize > 0) {
				pData = mem_alloc(fsize, "xRawBin");
				if (pData) {
					size = nxSys::def_fread(fh, pData, fsize);
				}
			}
			nxSys::def_fclose(fh);
		}
	}
#endif
	if (pSize) {
		*pSize = size;
	}
	return pData;
}

// http://www.isthe.com/chongo/tech/comp/fnv/index.html
uint32_t str_hash32(const char* pStr) {
	if (!pStr) return 0;
	uint32_t h = 2166136261;
	while (true) {
		uint8_t c = *pStr++;
		if (!c) break;
		h *= 16777619;
		h ^= c;
	}
	return h;
}

uint16_t str_hash16(const char* pStr) {
	uint32_t h = str_hash32(pStr);
	return (uint16_t)((h >> 16) ^ (h & 0xFFFF));
}

XD_FORCEINLINE static int x_strcompare_sub(const char* pStr1, const char* pStr2) {
	int res = 0;
	if (pStr1 && pStr2) {
		const char* p1 = pStr1;
		const char* p2 = pStr2;
		while (true) {
			char c = *p1;
			if (c != *p2) {
				res = c < *p2 ? -1 : 1;
				break;
			}
			if (!c) {
				break;
			}
			++p1;
			++p2;
		}
	}
	return res;
}

int str_cmp(const char* pStrA, const char* pStrB) {
	return x_strcompare_sub(pStrA, pStrB);
}

bool str_eq(const char* pStrA, const char* pStrB) {
	bool res = false;
	if (pStrA && pStrB) {
#if XD_STRFUNCS_INTERNAL
		res = x_strcompare_sub(pStrA, pStrB) == 0;
#else
		res = ::strcmp(pStrA, pStrB) == 0;
#endif
	}
	return res;
}

bool str_eq_x(const char* pStrA, const char* pStrB) {
	bool res = false;
	if (pStrA && pStrB) {
		if (pStrA == pStrB) {
			res = true;
		} else {
			res = x_strcompare_sub(pStrA, pStrB) == 0;
		}
	}
	return res;
}

bool str_starts_with(const char* pStr, const char* pPrefix) {
	if (pStr && pPrefix) {
		size_t len = nxCore::str_len(pPrefix);
		for (size_t i = 0; i < len; ++i) {
			char ch = pStr[i];
			if (0 == ch || ch != pPrefix[i]) return false;
		}
		return true;
	}
	return false;
}

bool str_ends_with(const char* pStr, const char* pPostfix) {
	if (pStr && pPostfix) {
		size_t lenStr = nxCore::str_len(pStr);
		size_t lenPost = nxCore::str_len(pPostfix);
		if (lenStr < lenPost) return false;
		for (size_t i = 0; i < lenPost; ++i) {
			if (pStr[lenStr - lenPost + i] != pPostfix[i]) return false;
		}
		return true;
	}
	return false;
}

char* str_dup(const char* pSrc, const char* pTag) {
	char* pDst = nullptr;
	if (pSrc) {
		size_t len = nxCore::str_len(pSrc) + 1;
		pDst = reinterpret_cast<char*>(mem_alloc(len, pTag ? pTag : "xStr"));
		mem_copy(pDst, pSrc, len);
	}
	return pDst;
}

size_t str_len(const char* pStr) {
	size_t len = 0;
	if (pStr) {
#if XD_STRFUNCS_INTERNAL
		const char* p = pStr;
		while (true) {
			if (!*p) break;
			++p;
		}
		len = (size_t)(p - pStr);
#else
		len = ::strlen(pStr);
#endif
	}
	return len;
}

#if XD_NUMPARSE_INTERNAL
static int64_t x_parse_i64_sub(const char* pChrs, const size_t nchrs) {
	int64_t val = 0;
	int64_t mul = 1;
	for (int i = int(nchrs); --i >= 0;) {
		int64_t d = pChrs[i] - '0';
		val += d * mul;
		mul *= 10;
	}
	return val;
}

static int64_t x_parse_i64_impl(const char* pStr) {
	int64_t res = 0;
	size_t len = str_len(pStr);
	if (len > 0 && len < 20) {
		const char* p = pStr;
		bool sgn = false;
		while ((*p == ' ' || *p == '\t') && len > 0) {
			++p;
			--len;
		}
		if (len > 0) {
			if (*p == '-') {
				sgn = true;
				++p;
				--len;
			} else  if (*p == '+') {
				++p;
				--len;
			}
			if (len > 0) {
				bool ck = true;
				for (size_t i = 0; i < len; ++i) {
					if (p[i] < '0' || p[i] > '9') {
						ck = false;
						break;
					}
				}
				if (ck) {
					res = x_parse_i64_sub(p, len);
					if (sgn) {
						res = -res;
					}
				}
			}
		}
	}
	return res;
}

static double x_parse_f64_impl(const char* pStr) {
	double res = 0.0;
	size_t len = str_len(pStr);
	if (len > 0 && len < 0x100) {
		const char* p = pStr;
		bool sgn = false;
		while ((*p == ' ' || *p == '\t') && len > 0) {
			++p;
			--len;
		}
		if (len > 0) {
			if (*p == '-') {
				sgn = true;
				++p;
				--len;
			} else  if (*p == '+') {
				++p;
				--len;
			}
			if (len > 0) {
				bool ck = true;
				int dpos = -1;
				for (size_t i = 0; i < len; ++i) {
					if (p[i] < '0' || p[i] > '9') {
						if (p[i] == '.' && dpos < 0) {
							dpos = int(i);
						} else {
							ck = false;
							break;
						}
					}
				}
				if (ck) {
					if (dpos < 0) {
						res = double(x_parse_i64_sub(p, len));
					} else {
						size_t ilen = dpos;
						size_t flen = len - dpos - 1;
						double wpart = double(x_parse_i64_sub(p, ilen));
						double fpart = double(x_parse_i64_sub(p + dpos + 1, flen));
						fpart *= nxCalc::ipow(10.0, -int(flen));
						res = wpart + fpart;
					}
					if (sgn) {
						res = -res;
					}
				}
			}
		}
	}
	return res;
}
#endif

XD_NOINLINE int64_t parse_i64(const char* pStr) {
	int64_t res = 0;
	if (pStr) {
#if XD_NUMPARSE_INTERNAL
		res = x_parse_i64_impl(pStr);
#else
#if defined(_MSC_VER)
		res = ::_atoi64(pStr);
#else
		res = ::atoll(pStr);
#endif
#endif
	}
	return res;
}

XD_NOINLINE double parse_f64(const char* pStr) {
	double res = 0.0;
	if (pStr) {
#if XD_NUMPARSE_INTERNAL
		res = x_parse_f64_impl(pStr);
#else
		res = ::atof(pStr);
#endif
	}
	return res;
}

XD_NOINLINE uint64_t parse_u64_hex(const char* pStr) {
	uint64_t val = 0;
	if (pStr) {
		size_t len = str_len(pStr);
		if (len <= 16) {
			const char* p0 = pStr;
			const char* p = pStr + len - 1;
			int s = 0;
			while (p >= p0) {
				char c = *p;
				int64_t d = 0;
				if (c >= '0' && c <= '9') {
					d = c - '0';
				} else if (c >= 'a' && c <= 'f') {
					d = c - 'a' + 10;
				} else if (c >= 'A' && c <= 'F') {
					d = c - 'A' + 10;
				} else {
					return 0;
				}
				val |= d << s;
				s += 4;
				--p;
			}
		}
	}
	return val;
}

XD_NOINLINE int str_fmt(char* pBuf, size_t bufSize, const char* pFmt, ...) {
	if (!pBuf || bufSize < 1) return -1;
	va_list argLst;
	va_start(argLst, pFmt);
	int n = 0;
#ifndef XD_NOSTDLIB
#if defined(_MSC_VER)
	n = ::vsprintf_s(pBuf, bufSize, pFmt, argLst);
#else
	n = ::vsprintf(pBuf, pFmt, argLst);
#endif
#endif
	va_end(argLst);
	return n;
}


XD_FORCEINLINE void sort_swap_bytes(uint8_t* pA, uint8_t* pB, size_t n) {
	uint8_t t;
	do {
		t = *pA;
		*pA++ = *pB;
		*pB++ = t;
		--n;
	} while (n != 0);
}

struct sxSortPrivateWk {
	void* pCtx;
	xt_sortcmp_func cmpfn;
	size_t elemSize;

	uint8_t* pivot(uint8_t* p, size_t n) {
		if (n > 10) {
			size_t j = elemSize * (n / 6);
			uint8_t* pI = p + j;
			uint8_t* pJ = pI + j * 2;
			uint8_t* pK = pI + j * 4;
			if (cmpfn(pI, pJ, pCtx) < 0) {
				if (cmpfn(pI, pK, pCtx) < 0) {
					if (cmpfn(pJ, pK, pCtx) < 0) {
						return pJ;
					}
					return pK;
				}
				return pI;
			}
			if (cmpfn(pJ, pK, pCtx) < 0) {
				if (cmpfn(pI, pK, pCtx) < 0) {
					return pI;
				}
				return pK;
			}
			return pJ;
		}
		return p + (elemSize * (n / 2));
	}

	void sort(uint8_t* p, size_t n) {
		size_t s = elemSize;
		while (n > 1) {
			size_t j;
			uint8_t* pJ;
			uint8_t* pN;
			uint8_t* pI = pivot(p, n);
			sort_swap_bytes(p, pI, s);
			pI = p;
			pN = p + (n * s);
			pJ = pN;
			while (true) {
				do {
					pI += s;
				} while (pI < pN && cmpfn(pI, p, pCtx) < 0);
				do {
					pJ -= s;
				} while (pJ > p && cmpfn(pJ, p, pCtx) > 0);
				if (pJ < pI) {
					break;
				}
				sort_swap_bytes(pI, pJ, s);
			}
			sort_swap_bytes(p, pJ, s);
			j = (size_t)(pJ - p) / s;
			n -= j + 1;
			if (j >= n) {
				sort(p, j);
				p += (j + 1) * s;
			} else {
				sort(p + (j + 1) * s, n);
				n = j;
			}
		}
	}
};

void sort(void* pEntries, size_t numEntries, size_t elemSize, xt_sortcmp_func cmpfn, void* pCtx) {
	if (!pEntries) return;
	if (!cmpfn) return;
	if (numEntries < 2) return;
	if (elemSize < 1) return;
	sxSortPrivateWk wk;
	wk.pCtx = pCtx;
	wk.cmpfn = cmpfn;
	wk.elemSize = elemSize;
	wk.sort((uint8_t*)pEntries, numEntries);
}

static int x_sort_cmpfunc_f32(const void* pA, const void* pB, void* pCtx) {
	float* pVal1 = (float*)pA;
	float* pVal2 = (float*)pB;
	float v1 = *pVal1;
	float v2 = *pVal2;
	if (v1 > v2) return 1;
	if (v1 < v2) return -1;
	return 0;
}

void sort_f32(float* pVals, size_t numVals) {
	sort(pVals, numVals, sizeof(float), x_sort_cmpfunc_f32);
}

static int x_sort_cmpfunc_f64(const void* pA, const void* pB, void* pCtx) {
	double* pVal1 = (double*)pA;
	double* pVal2 = (double*)pB;
	double v1 = *pVal1;
	double v2 = *pVal2;
	if (v1 > v2) return 1;
	if (v1 < v2) return -1;
	return 0;
}

void sort_f64(double* pVals, size_t numVals) {
	sort(pVals, numVals, sizeof(double), x_sort_cmpfunc_f64);
}


uint16_t float_to_half(const float x) {
#if XD_HAS_F16
	union FTOH { _Float16 h; uint16_t u; } ftoh;
	ftoh.h = (_Float16)x;
	return ftoh.u;
#else
	uint32_t b = f32_get_bits(x);
	uint16_t s = (uint16_t)((b >> 16) & (1 << 15));
	b &= ~(1U << 31);
	if (b > 0x477FE000U) {
		/* infinity */
		b = 0x7C00;
	} else {
		if (b < 0x38800000U) {
			uint32_t r = 0x70 + 1 - (b >> 23);
			b &= (1U << 23) - 1;
			b |= (1U << 23);
			b >>= r;
		} else {
			b += 0xC8000000U;
		}
		uint32_t a = (b >> 13) & 1;
		b += (1U << 12) - 1;
		b += a;
		b >>= 13;
		b &= (1U << 15) - 1;
	}
	return (uint16_t)(b | s);
#endif
}

float half_to_float(const uint16_t h) {
#if XD_HAS_F16
	union HTOF { uint16_t u; _Float16 h; } htof;
	htof.u = h;
	return (float)htof.h;
#else
	float f = 0.0f;
	if (h & ((1 << 16) - 1)) {
		int32_t e = (((h >> 10) & 0x1F) + 0x70) << 23;
		uint32_t m = (h & ((1 << 10) - 1)) << (23 - 10);
		uint32_t s = (uint32_t)(h >> 15) << 31;
		f = f32_set_bits(e | m | s);
	}
	return f;
#endif
}

float f32_set_bits(const uint32_t x) {
	uxVal32 v;
	v.u = x;
	return v.f;
}

uint32_t f32_get_bits(const float x) {
	uxVal32 v;
	v.f = x;
	return v.u;
}

float f32_get_mantissa(const float x) {
	uxVal32 v;
	v.f = 1.0f;
	v.u |= f32_get_mantissa_bits(x);
	return v.f;
};

bool f32_almost_eq(const float x, const float y, const float tol) {
	bool eq = false;
	float adif = ::mth_fabsf(x - y);
	if (x == 0.0f || y == 0.0f) {
		eq = adif <= tol;
	} else {
		float ax = ::mth_fabsf(x);
		float ay = ::mth_fabsf(y);
		eq = (adif / ax) <= tol && (adif / ay) <= tol;
	}
	return eq;
}

uint32_t f32_encode(const float x, const int nexp, const int nmts, const bool sgn/*=false*/) {
	uint32_t enc = 0;
	bool minFlg = false;
	if (x != 0.0f) {
		if (nexp <= 8) {
			int e = f32_get_exp(x);
			int bias = (1 << (nexp - 1)) - 1;
			if (e < -(bias - 1)) {
				minFlg = true;
			} else if (e > bias) {
				enc = (1U << (nexp + nmts)) - 1;
			} else {
				enc |= (e + bias) << nmts;
			}
		}
		if (minFlg) {
			enc = 1;
		} else if (nmts <= 23) {
			enc |= f32_get_mantissa_bits(x) >> (23 - nmts);
		}
	}
	if (sgn) {
		if (x < 0.0f) {
			enc |= 1U << (nexp + nmts);
		}
	}
	return enc;
}

float f32_decode(const uint32_t enc, const int nexp, const int nmts) {
#if 0
	uxVal32 mv;
	if (enc == 0) return 0.0f;
	int bias = (1 << (nexp - 1)) - 1;
	int e = int((enc >> nmts) & ((1U << nexp) - 1)) - bias;
	mv.f = 1.0f;
	mv.u |= (enc & ((1U << nmts) - 1)) << (23 - nmts);
	mv.u |= (enc & (1U << (nexp + nmts))) << ((23 + 8) - (nexp + nmts));
	return nxCalc::ipow(2.0f, e) * mv.f;
#else
	uxVal32 mv;
	mv.u = (enc & (1U << (nexp + nmts))) << ((23 + 8) - (nexp + nmts));
	mv.u |= (enc & ((1U << nmts) - 1)) << (23 - nmts);
	int bias = (1 << (nexp - 1)) - 1;
	int e = int((enc >> nmts) & ((1U << nexp) - 1)) - bias;
	mv.u |= (e + 0x7F) << 23;
	return mv.f;
#endif
}

uint32_t fetch_bits32(const uint8_t* pTop, const uint32_t org, const uint32_t len) {
	uint64_t res = 0;
	uint32_t idx = org >> 3;
	mem_copy(&res, &pTop[idx], 5);
	res >>= org & 7;
	res &= (1ULL << len) - 1;
	return (uint32_t)res;
}

static sxRNG s_rng = { /* seed(1) -> */ 0x910A2DEC89025CC1ULL, 0xBEEB8DA1658EEC67ULL };

void rng_seed(sxRNG* pState, uint64_t seed) {
	if (!pState) {
		pState = &s_rng;
	}
	if (seed == 0) {
		seed = 1;
	}
	uint64_t st = seed;
	for (int i = 0; i < 2; ++i) {
		/* splitmix64 */
		st += 0x9E3779B97F4A7C15ULL;
		uint64_t z = st;
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
		z ^= (z >> 31);
		pState->s[i] = z;
	}
}

/* xoroshiro128+ */
uint64_t rng_next(sxRNG* pState) {
	if (!pState) {
		pState = &s_rng;
	}
	uint64_t s0 = pState->s[0];
	uint64_t s1 = pState->s[1];
	uint64_t r = s0 + s1;
	s1 ^= s0;
	s0 = (s0 << 55) | (s0 >> 9);
	s0 ^= s1 ^ (s1 << 14);
	s1 = (s1 << 36) | (s1 >> 28);
	pState->s[0] = s0;
	pState->s[1] = s1;
	return r;
}

float rng_f01(sxRNG* pState) {
	uint32_t bits = (uint32_t)rng_next(pState);
	bits &= 0x7FFFFF;
	bits |= f32_get_bits(1.0f);
	float f = f32_set_bits(bits);
	return f - 1.0f;
}

} // nxCore


#define XD_HEAP_BLK_TAG XD_FOURCC('B', 'L', 'K', 0)
#define XD_HEAP_BLK_ALLOC XD_FOURCC(0, 0, 0, 0x80)

void cxHeap::init(Block* pBlk) {
	mpBlkTop = pBlk;
	mpBlkFree = pBlk;
	mpBlkLast = nullptr;
	mBlkNum = 1;
	mBlkUsed = 0;
	mBlkFree = 1;

	pBlk->blkTag = XD_HEAP_BLK_TAG;
	pBlk->ownerTag = 0;
	pBlk->pPrev = nullptr;
	pBlk->pNext = nullptr;
	pBlk->pPrevSub = nullptr;
	pBlk->pNextSub = nullptr;
	pBlk->size = (size_t)((char*)this + mSize - (char*)(pBlk + 1));
}

void* cxHeap::alloc(size_t size, const uint32_t tag) {
	Block* pBlkUse = nullptr;
	Block** ppBlkList = &mpBlkFree;
	Block* pBlkFound = *ppBlkList;
	while (pBlkFound) {
		if (pBlkFound->size >= size) {
			break;
		}
		ppBlkList = &pBlkFound->pNext;
		pBlkFound = *ppBlkList;
	}
	if (!pBlkFound) {
		return nullptr;
	}
	if (pBlkFound->pNext) {
		pBlkFound->pNext->pPrev = pBlkFound->pPrev;
	}
	*ppBlkList = pBlkFound->pNext;
	pBlkUse = (Block*)XD_INCR_PTR(pBlkFound + 1, pBlkFound->size - size);
	size_t alignMod = (size_t)((uintptr_t)pBlkUse % alignment());
	if (alignMod) {
		pBlkUse = (Block*)((char*)pBlkUse - alignMod);
	}
	--pBlkUse;
	if (pBlkFound + 2 >= pBlkUse) {
		pBlkUse = pBlkFound;
		pBlkUse->ownerTag = tag;
		pBlkUse->blkTag |= XD_HEAP_BLK_ALLOC;
		pBlkUse->pNext = mpBlkLast;
		if (pBlkUse->pNext) {
			pBlkUse->pNext->pPrev = pBlkUse;
		}
		mpBlkLast = pBlkUse;
		pBlkUse->pPrev = nullptr;
		++mBlkUsed;
		--mBlkFree;
	} else {
		pBlkUse->ownerTag = tag;
		pBlkUse->blkTag = XD_HEAP_BLK_TAG | XD_HEAP_BLK_ALLOC;
		pBlkUse->size = (size_t)((uintptr_t)XD_INCR_PTR(pBlkFound + 1, pBlkFound->size) - (uintptr_t)(pBlkUse + 1));
		pBlkUse->pNextSub = pBlkFound->pNextSub;
		if (pBlkUse->pNextSub) {
			pBlkUse->pNextSub->pPrevSub = pBlkUse;
		}
		pBlkUse->pPrevSub = pBlkFound;
		pBlkFound->pNextSub = pBlkUse;
		pBlkFound->size = (size_t)((char*)pBlkUse - (char*)(pBlkFound + 1));
		pBlkUse->pNext = mpBlkLast;
		if (pBlkUse->pNext) {
			pBlkUse->pNext->pPrev = pBlkUse;
		}
		mpBlkLast = pBlkUse;
		pBlkUse->pPrev = nullptr;
		pBlkFound->pPrev = nullptr;

		ppBlkList = &mpBlkFree;
		while (*ppBlkList) {
			if ((*ppBlkList)->size >= pBlkFound->size) break;
			pBlkFound->pPrev = *ppBlkList;
			ppBlkList = &(*ppBlkList)->pNext;
		}
		pBlkFound->pNext = *ppBlkList;
		if (pBlkFound->pNext) {
			pBlkFound->pNext->pPrev = pBlkFound;
		}
		*ppBlkList = pBlkFound;
		++mBlkNum;
		++mBlkUsed;
	}
	return (pBlkUse + 1);
}

void cxHeap::free(void* pMem) {
	if (!pMem) return;
	bool inRange = (char*)pMem > (char*)this && (char*)pMem < (char*)XD_INCR_PTR(this, mSize);
	if (!inRange) return;
	Block* pBlkWk = ((Block*)pMem) - 1;
	if (pBlkWk->blkTag != (XD_HEAP_BLK_TAG | XD_HEAP_BLK_ALLOC)) {
		return;
	}

	pBlkWk->blkTag &= ~XD_HEAP_BLK_ALLOC;
	if (pBlkWk->pNext) {
		pBlkWk->pNext->pPrev = pBlkWk->pPrev;
	}
	if (pBlkWk->pPrev) {
		pBlkWk->pPrev->pNext = pBlkWk->pNext;
	} else {
		mpBlkLast = pBlkWk->pNext;
	}

	Block* pBlkMerge = pBlkWk->pNextSub;
	if (pBlkMerge && !(pBlkMerge->blkTag & XD_HEAP_BLK_ALLOC)) {
		if (pBlkMerge->pNext) {
			pBlkMerge->pNext->pPrev = pBlkMerge->pPrev;
		}
		if (pBlkMerge->pPrev) {
			pBlkMerge->pPrev->pNext = pBlkMerge->pNext;
		} else {
			mpBlkFree = pBlkMerge->pNext;
		}
		pBlkWk->pNextSub = pBlkMerge->pNextSub;
		if (pBlkWk->pNextSub) {
			pBlkWk->pNextSub->pPrevSub = pBlkWk;
		}
		pBlkWk->size = (size_t)((char*)(pBlkMerge + 1) + pBlkMerge->size - (char*)(pBlkWk + 1));
		pBlkMerge->blkTag = 0;
		--mBlkFree;
		--mBlkNum;
	}

	pBlkMerge = pBlkWk->pPrevSub;
	if (pBlkMerge && !(pBlkMerge->blkTag & XD_HEAP_BLK_ALLOC)) {
		if (pBlkMerge->pNext) {
			pBlkMerge->pNext->pPrev = pBlkMerge->pPrev;
		}
		if (pBlkMerge->pPrev) {
			pBlkMerge->pPrev->pNext = pBlkMerge->pNext;
		} else {
			mpBlkFree = pBlkMerge->pNext;
		}
		pBlkMerge->pNextSub = pBlkWk->pNextSub;
		if (pBlkMerge->pNextSub) {
			pBlkMerge->pNextSub->pPrevSub = pBlkMerge;
		}
		pBlkMerge->size = (size_t)((char*)(pBlkWk + 1) + pBlkWk->size - (char*)(pBlkMerge + 1));
		pBlkWk->blkTag = 0;
		pBlkWk = pBlkMerge;
		--mBlkFree;
		--mBlkNum;
	}

	--mBlkUsed;
	++mBlkFree;
	pBlkWk->pPrev = nullptr;
	Block** ppBlkList = &mpBlkFree;
	while (*ppBlkList) {
		if ((*ppBlkList)->size >= pBlkWk->size) break;
		pBlkWk->pPrev = *ppBlkList;
		ppBlkList = &(*ppBlkList)->pNext;
	}
	pBlkWk->pNext = *ppBlkList;
	if (pBlkWk->pNext) {
		pBlkWk->pNext->pPrev = pBlkWk;
	}
	*ppBlkList = pBlkWk;
}

XD_NOINLINE void cxHeap::purge() {
	if (mpBlkTop) {
		init(mpBlkTop);
	}
}

uint32_t cxHeap::alignment() const {
	int32_t algn = (int32_t)mAlign;
	if (algn < 0) {
		algn = -algn;
	}
	return (uint32_t)algn;
}

XD_NOINLINE bool cxHeap::contains(const void* p) const {
	bool res = false;
	if (p) {
		res = (char*)p >(char*)XD_INCR_PTR(this, sizeof(cxHeap) + sizeof(Block)) && (char*)p < (char*)XD_INCR_PTR(this, mSize);
	}
	return res;
}

XD_NOINLINE cxHeap* cxHeap::create(const size_t size, const char* pName, const uint32_t align) {
	cxHeap* pHeap = nullptr;
	void* pMem = nxCore::mem_alloc(size, pName);
	if (pMem) {
		size_t headSize = sizeof(cxHeap) + sizeof(Block);
		size_t alignVal = (int32_t)align > 0 ? align : 0x10;
		size_t alignMod = ((size_t)(uintptr_t)XD_INCR_PTR(pMem, headSize)) % alignVal;
		if (alignMod) {
			headSize += align - alignMod;
		}
		if (headSize >= size) {
			nxCore::mem_free(pMem);
		} else {
			pHeap = (cxHeap*)pMem;
			Block* pBlk = (Block*)XD_INCR_PTR(pMem, headSize - sizeof(Block));
			pHeap->mSize = size;
			pHeap->mAlign = (uint32_t)alignVal;
			pHeap->init(pBlk);
		}
	}
	return pHeap;
}

XD_NOINLINE cxHeap* cxHeap::create(void* pMem, const size_t size, const char* pName, const uint32_t align) {
	cxHeap* pHeap = nullptr;
	if (pMem) {
		size_t headSize = sizeof(cxHeap) + sizeof(Block);
		size_t alignVal = (int32_t)align > 0 ? align : 0x10;
		size_t alignMod = ((size_t)(uintptr_t)XD_INCR_PTR(pMem, headSize)) % alignVal;
		if (alignMod) {
			headSize += align - alignMod;
		}
		if (headSize < size) {
			pHeap = (cxHeap*)pMem;
			Block* pBlk = (Block*)XD_INCR_PTR(pMem, headSize - sizeof(Block));
			pHeap->mSize = size - headSize;
			pHeap->mAlign = (uint32_t)(-(int32_t)alignVal);
			pHeap->init(pBlk);
		}
	}
	return pHeap;
}

XD_NOINLINE void cxHeap::destroy(cxHeap* pHeap) {
	if (pHeap) {
		if ((int32_t)pHeap->mAlign > 0) {
			nxCore::mem_free(pHeap);
		}
	}
}

struct sxJobQueue {
	int mSlotsNum;
	int mPutIdx;
#if XD_CXXATOMIC_ENABLED
	std::atomic<int> mAccessIdx;
#else
	int mAccessIdx;
#endif
	sxJob* mpJobs[1];

	sxJob* get_next_job() {
		sxJob* pJob = nullptr;
		int count = mPutIdx;
		if (count > 0) {
#if XD_CXXATOMIC_ENABLED
			int idx = mAccessIdx.fetch_add(1);
#elif defined(__GNUC__) && !defined(XD_NOSTDLIB)
			int idx = __atomic_fetch_add(&mAccessIdx, 1, __ATOMIC_SEQ_CST);
#else
			int idx = mAccessIdx;
			++mAccessIdx;
#endif
			if (idx < count) {
				pJob = mpJobs[idx];
			}
		}
		return pJob;
	}

	void reset_cursor() {
#if XD_CXXATOMIC_ENABLED
		mAccessIdx.store(0);
#else
		mAccessIdx = 0;
#endif
	}

	int get_count() const {
		return mPutIdx;
	}
};

#if XD_THREADFUNCS_ENABLED
static void brigade_wrk_func(void* pMem) {
	sxJobContext* pCtx = (sxJobContext*)pMem;
	if (!pCtx) return;
	cxBrigade* pBgd = pCtx->mpBrigade;
	if (!pBgd) return;
	sxJobQueue* pQue = pBgd->get_queue();
	if (!pQue) return;
	if (pBgd->is_dynamic_scheduling()) {
		while (true) {
			sxJob* pJob = pQue->get_next_job();
			if (!pJob) break;
			pCtx->mpJob = pJob;
			if (pJob->mFunc) {
				pJob->mFunc(pCtx);
			}
			++pCtx->mJobsDone;
		}
	} else {
		for (int i = pCtx->mJobOrg; i <= pCtx->mJobEnd; ++i) {
			sxJob* pJob = pQue->mpJobs[i];
			if (!pJob) break;
			pCtx->mpJob = pJob;
			if (pJob->mFunc) {
				pJob->mFunc(pCtx);
			}
			++pCtx->mJobsDone;
		}
	}
}
#endif

void cxBrigade::exec(sxJobQueue* pQue) {
	if (!pQue) return;
	pQue->reset_cursor();
	mpQue = pQue;
	int wrkNum = mActiveWrkNum;
	for (int i = 0; i < wrkNum; ++i) {
		mpJobCtx[i].mJobsDone = 0;
	}
	int njobs = pQue->get_count();
	if (njobs < 1) {
		return;
	}
	if (is_dynamic_scheduling()) {
		for (int i = 0; i < wrkNum; ++i) {
			nxSys::worker_exec(mppWrk[i]);
		}
	} else {
		int jobOrg = 0;
		int jobAdd = njobs / wrkNum;
		int jobExt = njobs % wrkNum;
		for (int i = 0; i < wrkNum; ++i) {
			mpJobCtx[i].mJobOrg = jobOrg;
			jobOrg += jobAdd;
			if (i == 0) {
				jobOrg += jobExt;
			}
			mpJobCtx[i].mJobEnd = jobOrg - 1;
		}
		for (int i = 0; i < wrkNum; ++i) {
			nxSys::worker_exec(mppWrk[i]);
		}
	}
}

void cxBrigade::wait() {
	if (!mpQue) return;
	if (mpQue->get_count() > 0) {
		int wrkNum = mActiveWrkNum;
		if (mpDoneHandles) {
#if defined(XD_TSK_NATIVE_WINDOWS)
			::WaitForMultipleObjects(wrkNum, (const HANDLE*)mpDoneHandles, TRUE, INFINITE);
#endif
		} else {
			for (int i = 0; i < wrkNum; ++i) {
				nxSys::worker_wait(mppWrk[i]);
			}
		}
	}
	mpQue = nullptr;
}

void cxBrigade::set_active_workers_num(const int num) {
	mActiveWrkNum = nxCalc::clamp(num, 1, mWrkNum);
}

void cxBrigade::reset_active_workers() {
	mActiveWrkNum = mWrkNum;
	for (int i = 0; i < mWrkNum; ++i) {
		mpJobCtx[i].mJobsDone = 0;
	}
}

int cxBrigade::get_jobs_done_count(const int wrkId) const {
	int n = 0;
	if (ck_worker_id(wrkId)) {
		n = mpJobCtx[wrkId].mJobsDone;
	}
	return n;
}

sxJobContext* cxBrigade::get_job_context(const int wrkId) {
	sxJobContext* pCtx = nullptr;
	if (ck_worker_id(wrkId)) {
		pCtx = &mpJobCtx[wrkId];
	}
	return pCtx;
}

void cxBrigade::auto_affinity() {
#if defined(XD_TSK_NATIVE_PTHREAD) && defined(XD_SYS_LINUX) && !defined(__ANDROID__)
	if (mWrkNum < 2) return;
	if (!mppWrk) return;
	int ncpu = nxSys::num_active_cpus();
	if (ncpu < 2) return;
	pthread_t thrMain = pthread_self();
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	pthread_setaffinity_np(thrMain, sizeof(mask), &mask);
	int cpuMin = 1;
	if (ncpu < 3) {
		cpuMin = 0;
	}
	int cpuNo = cpuMin;
	for (int i = 0; i < mWrkNum; ++i) {
		sxWorker* pWrk = mppWrk[i];
		if (pWrk) {
			CPU_ZERO(&mask);
			CPU_SET(cpuNo, &mask);
			pthread_setaffinity_np(pWrk->mThread, sizeof(mask), &mask);
			++cpuNo;
		}
		if (cpuNo >= ncpu) {
			cpuNo = cpuMin;
		}
	}
#elif defined(XD_TSK_NATIVE_PTHREAD) && defined(XD_SYS_SUNOS)
	if (mWrkNum < 2) return;
	if (!mppWrk) return;
	int cpuLst[256];
	int ncpu = nxSys::num_active_cpus();
	if (ncpu > XD_ARY_LEN(cpuLst)) {
		nxCore::dbg_msg("auto_affinity: too many active cpus: %d\n", ncpu);
		return;
	}
	for (int i = 0; i < XD_ARY_LEN(cpuLst); ++i) { cpuLst[i] = -1; }
	int cidMax = sysconf(_SC_CPUID_MAX);
	nxCore::dbg_msg("auto_affinity: ncpu = %d, max cpuId = %d\n", ncpu, cidMax);
	int icpu = 0;
	for (int cid = 0; cid <= cidMax; ++cid) {
		if (p_online(cid, P_STATUS) != -1) {
			nxCore::dbg_msg("auto_affinity: cpu %d is online\n", cid);
			cpuLst[icpu] = cid;
			++icpu;
		}
	}
	nxCore::dbg_msg("auto_affinity: %d cpus online\n", icpu);
	int cpuMin = 0;
	int cpuNo = cpuMin;
	for (int i = 0; i < mWrkNum; ++i) {
		sxWorker* pWrk = mppWrk[i];
		if (pWrk) {
			processorid_t cid = cpuLst[cpuNo];
			processorid_t oid = -1;
			id_t lwpid = (id_t)pWrk->mThread;
			int res = processor_bind(P_LWPID, lwpid, cid, &oid);
			if (res == 0) {
				nxCore::dbg_msg("auto_affinity: lwp %d, %d -> %d\n", lwpid, oid, cid);
			} else {
				nxCore::dbg_msg("auto_affinity: failed for lwp %d, cid %d\n", lwpid, cid);
			}
			++cpuNo;
		}
		if (cpuNo >= ncpu) {
			cpuNo = cpuMin;
		}
	}
#endif
}

#if XD_THREADFUNCS_ENABLED
cxBrigade* cxBrigade::create(int wrkNum) {
	cxBrigade* pBgd = nullptr;
	if (wrkNum < 1) wrkNum = 1;
	size_t memSize = XD_ALIGN(sizeof(cxBrigade), 0x10);
	size_t wrkOffs = memSize;
	size_t wrkSize = wrkNum*sizeof(sxWorker*) + wrkNum*sizeof(sxJobContext);
#if defined(XD_TSK_NATIVE_WINDOWS)
	if (XD_SYNC_MULTI) {
		wrkSize += wrkNum*sizeof(HANDLE);
	}
#endif
	memSize += wrkSize;
	pBgd = (cxBrigade*)nxCore::mem_alloc(memSize, "xBrigade");
	if (pBgd) {
		pBgd->mpQue = nullptr;
		pBgd->mppWrk = (sxWorker**)XD_INCR_PTR(pBgd, wrkOffs);
		pBgd->mpJobCtx = (sxJobContext*)(pBgd->mppWrk + wrkNum);
		pBgd->mpDoneHandles = nullptr;
		pBgd->mWrkNum = wrkNum;
		pBgd->mActiveWrkNum = wrkNum;
		pBgd->set_dynamic_scheduling();
		nxCore::mem_zero(pBgd->mppWrk, wrkSize);
		for (int i = 0; i < wrkNum; ++i) {
			pBgd->mpJobCtx[i].mpBrigade = pBgd;
			pBgd->mpJobCtx[i].mWrkId = i;
		}
		for (int i = 0; i < wrkNum; ++i) {
			pBgd->mppWrk[i] = nxSys::worker_create(brigade_wrk_func, &pBgd->mpJobCtx[i]);
		}
#if defined(XD_TSK_NATIVE_WINDOWS)
		if (XD_SYNC_MULTI) {
			pBgd->mpDoneHandles = pBgd->mpJobCtx + wrkNum;
			HANDLE* pDoneHnd = (HANDLE*)pBgd->mpDoneHandles;
			for (int i = 0; i < wrkNum; ++i) {
				pDoneHnd[i] = pBgd->mppWrk[i]->mpSigDone->mhEvt;
			}
		}
#endif
	}
	return pBgd;
}
#else
cxBrigade* cxBrigade::create(int wrkNum) {
	return nullptr;
}
#endif

void cxBrigade::destroy(cxBrigade* pBgd) {
	if (!pBgd) return;
	int wrkNum = pBgd->mWrkNum;
	for (int i = 0; i < wrkNum; ++i) {
		nxSys::worker_stop(pBgd->mppWrk[i]);
	}
	for (int i = 0; i < wrkNum; ++i) {
		nxSys::worker_destroy(pBgd->mppWrk[i]);
	}
	nxCore::mem_free(pBgd);
}


namespace nxTask {

sxJobQueue* queue_create(int slotsNum) {
	sxJobQueue* pQue = nullptr;
	if (slotsNum > 0) {
		size_t memSize = sizeof(sxJobQueue) + (slotsNum - 1)*sizeof(sxJob*);
		pQue = (sxJobQueue*)nxCore::mem_alloc(memSize, "xJobQueue");
		if (pQue) {
			pQue->mSlotsNum = slotsNum;
			pQue->mPutIdx = 0;
			pQue->reset_cursor();
			for (int i = 0; i < slotsNum; ++i) {
				pQue->mpJobs[i] = nullptr;
			}
		}
	}
	return pQue;
}

void queue_destroy(sxJobQueue* pQue) {
	if (pQue) {
		nxCore::mem_free(pQue);
	}
}

void queue_add(sxJobQueue* pQue, sxJob* pJob) {
	if (!pQue) return;
	if (!pJob) return;
	if (pQue->mPutIdx < pQue->mSlotsNum) {
		pQue->mpJobs[pQue->mPutIdx] = pJob;
		pJob->mId = pQue->mPutIdx;
		++pQue->mPutIdx;
	}
}

void queue_count_adjust(sxJobQueue* pQue, int count) {
	if (pQue && count <= pQue->mSlotsNum) {
		pQue->mPutIdx = count;
	}
}

void queue_purge(sxJobQueue* pQue) {
	if (pQue) {
		pQue->mPutIdx = 0;
	}
}

int queue_get_max_job_num(sxJobQueue* pQue) {
	return pQue ? pQue->mSlotsNum : 0;
}

int queue_get_job_count(sxJobQueue* pQue) {
	return pQue ? pQue->mPutIdx : 0;
}

void queue_exec(sxJobQueue* pQue, cxBrigade* pBgd) {
	if (!pQue) return;
	if (pBgd) {
		pBgd->exec(pQue);
		pBgd->wait();
	} else {
#ifdef XD_USE_OMP
		int n = queue_get_job_count(pQue);
#		pragma omp parallel for
		for (int i = 0; i < n; ++i) {
			sxJob* pJob = pQue->mpJobs[i];
			if (pJob) {
				if (pJob->mFunc) {
					sxJobContext ctx;
					ctx.mWrkId = -1;
					ctx.mpBrigade = nullptr;
					ctx.mJobsDone = 0;
					ctx.mpJob = pJob;
					pJob->mFunc(&ctx);
				}
			}
		}
#else
		sxJobContext ctx;
		ctx.mWrkId = -1;
		ctx.mpBrigade = nullptr;
		ctx.mJobsDone = 0;
		int n = queue_get_job_count(pQue);
		for (int i = 0; i < n; ++i) {
			sxJob* pJob = pQue->mpJobs[i];
			if (pJob) {
				if (pJob->mFunc) {
					ctx.mpJob = pJob;
					pJob->mFunc(&ctx);
				}
			}
		}
#endif
	}
}


} // nxTask


namespace nxCalc {

static float s_identityMtx[4][4] = {
	{ 1.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f },
};

float ease_crv(const float p1, const float p2, const float t) {
	float tt = t*t;
	float ttt = tt*t;
	float ttt3 = ttt*3.0f;
	float b1 = ttt3 - tt*6.0f + t*3.0f;
	float b2 = tt*3.0f - ttt3;
	return b1*p1 + b2*p2 + ttt;
}

float hermite(const float p0, const float m0, const float p1, const float m1, const float t) {
	float tt = t*t;
	float ttt = tt*t;
	float tt3 = tt*3.0f;
	float tt2 = tt + tt;
	float ttt2 = tt2 * t;
	float h00 = ttt2 - tt3 + 1.0f;
	float h10 = ttt - tt2 + t;
	float h01 = -ttt2 + tt3;
	float h11 = ttt - tt;
	return (h00*p0 + h10*m0 + h01*p1 + h11*m1);
}

float fit(const float val, const float oldMin, const float oldMax, const float newMin, const float newMax) {
	float rel = div0(val - oldMin, oldMax - oldMin);
	rel = saturate(rel);
	return lerp(newMin, newMax, rel);
}

float calc_fovy(const float focal, const float aperture, const float aspect) {
	float zoom = ((2.0f * focal) / aperture) * aspect;
	float fovy = 2.0f * ::mth_atan2f(1.0f, zoom);
	return fovy;
}

bool is_prime(const int32_t x) {
	if (x <= 1) return false;
	if (is_even(x)) return x == 2;
	int n = int(::mth_sqrt(x));
	for (int i = 3; i < n; i += 2) {
		if ((x % i) == 0) return false;
	}
	return true;
}

int32_t prime(const int32_t x) {
	if (is_prime(x)) return x;
	int org = x > 1 ? (x & (~1)) - 1 : 0;
	int end = int32_t(uint32_t(-1) >> 1);
	for (int i = org; i < end; i += 2) {
		if (is_prime(i)) return i;
	}
	return x;
}

double median(double* pData, const size_t num) {
	double med = 0.0;
	if (pData && num) {
		nxCore::sort_f64(pData, num);
		med = (num & 1) ? pData[(num - 1) / 2] : (pData[(num / 2) - 1] + pData[num / 2]) * 0.5;
	}
	return med;
}

double harmonic_mean(double* pData, const size_t num) {
	double hm = 0.0;
	if (pData && num) {
		double s = 0.0;
		for (size_t i = 0; i < num; ++i) {
			s += rcp0(pData[i]);
		}
		hm = div0(double(num), s);
	}
	return hm;
}

} // nxCalc

namespace nxLA {

void mul_mm_f(float* pDst, const float* pSrc1, const float* pSrc2, const int M, const int N, const int P) {
	mul_mm<float, float, float>(pDst, pSrc1, pSrc2, M, N, P);
}

void mul_vm_f(float* pDstVec, const float* pSrcVec, const float* pMtx, const int M, const int N) {
	mul_mm_f(pDstVec, pSrcVec, pMtx, 1, M, N);
}

#ifndef XD_LA_MUL_MV_F
#	define XD_LA_MUL_MV_F 0
#endif

void mul_mv_f(float* pDstVec, const float* pMtx, const float* pSrcVec, const int M, const int N) {
#if XD_LA_MUL_MV_F == 0
	int r = 0;
	for (int i = 0; i < M; ++i) {
		float t = 0.0f;
		for (int j = 0; j < N; ++j) {
			t += pMtx[r + j] * pSrcVec[j];
		}
		pDstVec[i] = t;
		r += N;
	}
#else
	mul_mm_f(pDstVec, pMtx, pSrcVec, M, N, 1);
#endif
}

void mul_mv_f_hcalc(float* pDstVec, const float* pMtx, const float* pSrcVec, const int M, const int N) {
#if XD_HAS_F16 && XD_HCALC_LOCBUF_SIZE > 0
	int r = 0;
	if (N <= XD_HCALC_LOCBUF_SIZE) {
		_Float16 hvec[XD_HCALC_LOCBUF_SIZE];
		for (int j = 0; j < N; ++j) {
			hvec[j] = (_Float16)pSrcVec[j];
		}
		for (int i = 0; i < M; ++i) {
			_Float16 t = 0;
			for (int j = 0; j < N; ++j) {
				t += (_Float16)pMtx[r + j] * hvec[j];
			}
			pDstVec[i] = (float)t;
			r += N;
		}
	} else {
		for (int i = 0; i < M; ++i) {
			_Float16 t = 0;
			for (int j = 0; j < N; ++j) {
				t += (_Float16)pMtx[r + j] * (_Float16)pSrcVec[j];
			}
			pDstVec[i] = (float)t;
			r += N;
		}
	}
#else
	nxLA::mul_mv_f(pDstVec, pMtx, pSrcVec, M, N);
#endif
}

void mul_mv_hf(float* pDstVec, const xt_half* pMtx, const float* pSrcVec, const int M, const int N) {
#if XD_HAS_F16 && XD_HCALC_LOCBUF_SIZE > 0
	int r = 0;
	if (N <= XD_HCALC_LOCBUF_SIZE) {
		_Float16 hvec[XD_HCALC_LOCBUF_SIZE];
		for (int j = 0; j < N; ++j) {
			hvec[j] = (_Float16)pSrcVec[j];
		}
		for (int i = 0; i < M; ++i) {
			_Float16 t = 0;
			for (int j = 0; j < N; ++j) {
				t += *(_Float16*)&pMtx[r + j] * hvec[j];
			}
			pDstVec[i] = (float)t;
			r += N;
		}
	} else {
		for (int i = 0; i < M; ++i) {
			float t = 0;
			for (int j = 0; j < N; ++j) {
				t += pMtx[r + j].get() * pSrcVec[j];
			}
			pDstVec[i] = t;
			r += N;
		}
	}
#else
	int r = 0;
	for (int i = 0; i < M; ++i) {
		float t = 0;
		for (int j = 0; j < N; ++j) {
			t += pMtx[r + j].get() * pSrcVec[j];
		}
		pDstVec[i] = t;
		r += N;
	}
#endif
}

void mul_mm_d(double* pDst, const double* pSrc1, const double* pSrc2, const int M, const int N, const int P) {
	mul_mm<double, double, double>(pDst, pSrc1, pSrc2, M, N, P);
}

void mul_vm_d(double* pDstVec, const double* pSrcVec, const double* pMtx, const int M, const int N) {
	mul_mm_d(pDstVec, pSrcVec, pMtx, 1, M, N);
}
void mul_mv_d(double* pDstVec, const double* pMtx, const double* pSrcVec, const int M, const int N) {
	mul_mm_d(pDstVec, pMtx, pSrcVec, M, N, 1);
}

float dot_f(const float* pVec1, const float* pVec2, const int N) {
	float d = 0.0f;
	for (int i = 0; i < N; ++i) {
		d += pVec1[i] * pVec2[i];
	}
	return d;
}

void scl_f(float* pDst, const float* pSrc, const float s, const int N) {
	for (int i = 0; i < N; ++i) {
		pDst[i] = pSrc[i] * s;
	}
}

void sclmul_f(float* pDst, const float* pSrc1, const float* pSrc2, const float s, const int N) {
	for (int i = 0; i < N; ++i) {
		pDst[i] = pSrc1[i] * pSrc2[i] * s;
	}
}

void mul_mm_h(xt_half* pDst, const xt_half* pSrc1, const xt_half* pSrc2, const int M, const int N, const int P) {
#if XD_HAS_F16
	_Float16* pDstH = (_Float16*)pDst;
	const _Float16* pSrc1H = (const _Float16*)pSrc1;
	const _Float16* pSrc2H = (const _Float16*)pSrc2;
	for (int i = 0; i < M; ++i) {
		int ra = i * N;
		int rr = i * P;
		_Float16 s = pSrc1H[ra];
		for (int k = 0; k < P; ++k) {
			pDstH[rr + k] = pSrc2H[k] * s;
		}
	}
	for (int i = 0; i < M; ++i) {
		int ra = i * N;
		int rr = i * P;
		for (int j = 1; j < N; ++j) {
			int rb = j * P;
			_Float16 s = pSrc1H[ra + j];
			for (int k = 0; k < P; ++k) {
				pDstH[rr + k] += pSrc2H[rb + k] * s;
			}
		}
	}
#else
	for (int i = 0; i < M; ++i) {
		int ra = i * N;
		int rr = i * P;
		float s = pSrc1[ra].get();
		for (int k = 0; k < P; ++k) {
			xt_half t;
			t.set(pSrc2[k].get() * s);
			pDst[rr + k] = t;
		}
	}
	for (int i = 0; i < M; ++i) {
		int ra = i * N;
		int rr = i * P;
		for (int j = 1; j < N; ++j) {
			int rb = j * P;
			float s = pSrc1[ra + j].get();
			for (int k = 0; k < P; ++k) {
				xt_half t;
				t.set(pDst[rr + k].get() + pSrc2[rb + k].get()*s);
				pDst[rr + k] = t;
			}
		}
	}
#endif
}

} // nxLA


namespace nxML {

// ref: "Root Mean Square Layer Normalization"
void rms_norm(float* pDst, const float* pSrc, const float* pWgt, const int N, const float eps) {
	float s = nxLA::dot_f(pSrc, pSrc, N);
	s = nxCalc::div0(s, float(N));
	if (s > eps) {
		s = 1.0f / ::mth_sqrtf(s);
	} else {
		s = 0.0f;
	}
	nxLA::sclmul_f(pDst, pSrc, pWgt, s, N);
}

void softmax(float* pVec, const int N) {
	float m = pVec[0];
	for (int i = 1; i < N; ++i) {
		m = nxCalc::max(m, pVec[i]);
	}
	for (int i = 0; i < N; ++i) {
		pVec[i] -= m;
	}
	for (int i = 0; i < N; ++i) {
		pVec[i] = ::mth_expf(pVec[i]);
	}
	float s = 0.0f;
	for (int i = 0; i < N; ++i) {
		s += pVec[i];
	}
	nxLA::scl_f(pVec, pVec, nxCalc::rcp0(s), N);
}


} // nxML


void xt_half::set(const float f) {
	x = nxCore::float_to_half(f);
}

float xt_half::get() const {
	return nxCore::half_to_float(x);
}

uint32_t xt_half::get_exp_bits() const {
	return (x >> 10) & 0x1F;
}

int xt_half::get_exp() const {
	return int(get_exp_bits()) - 0xF;
}

uint32_t xt_half::get_mantissa_bits() const {
	return x & ((1U << 10) - 1);
}

float xt_half::get_mantissa() const {
	uxVal32 v;
	v.f = 1.0f;
	v.u |= get_mantissa_bits() << (23 - 10);
	return v.f;
}

void xt_half::rand01(sxRNG* pRNG) {
	float r = nxCore::rng_f01(pRNG);
	set(r);
}


void xt_half2::set(const float fx, const float fy) {
	x = nxCore::float_to_half(fx);
	y = nxCore::float_to_half(fy);
}

xt_float2 xt_half2::get() const {
	xt_float2 fv;
	fv.x = nxCore::half_to_float(x);
	fv.y = nxCore::half_to_float(y);
	return fv;
}


void xt_half3::set(const float fx, const float fy, const float fz) {
	x = nxCore::float_to_half(fx);
	y = nxCore::float_to_half(fy);
	z = nxCore::float_to_half(fz);
}

xt_float3 xt_half3::get() const {
	xt_float3 fv;
	fv.x = nxCore::half_to_float(x);
	fv.y = nxCore::half_to_float(y);
	fv.z = nxCore::half_to_float(z);
	return fv;
}


void xt_half4::set(const float fx, const float fy, const float fz, const float fw) {
	x = nxCore::float_to_half(fx);
	y = nxCore::float_to_half(fy);
	z = nxCore::float_to_half(fz);
	w = nxCore::float_to_half(fw);
}

xt_float4 xt_half4::get() const {
	xt_float4 fv;
	fv.x = nxCore::half_to_float(x);
	fv.y = nxCore::half_to_float(y);
	fv.z = nxCore::half_to_float(z);
	fv.w = nxCore::half_to_float(w);
	return fv;
}


static inline float tex_scroll(float val, float add) {
	val += add;
	if (val < 0.0f) val += 1.0f;
	if (val > 1.0f) val -= 1.0f;
	return val;
}

void xt_texcoord::scroll(const xt_texcoord& step) {
	u = tex_scroll(u, step.u);
	v = tex_scroll(v, step.v);
}

void xt_texcoord::encode_half(uint16_t* pDst) const {
	if (pDst) {
		pDst[0] = nxCore::float_to_half(u);
		pDst[1] = nxCore::float_to_half(v);
	}
}


float xt_float4::dp(const xt_float4& f4) const {
	const float* p1 = *this;
	const float* p2 = f4;
	float d = 0.0f;
	for (int i = 0; i < 4; ++i) {
		d += p1[i] * p2[i];
	}
	return d;
}


void xt_mtx::identity() {
	nxCore::mem_copy(this, nxCalc::s_identityMtx, sizeof(*this));
}

void xt_mtx::zero() {
	nxCore::mem_zero(this, sizeof(*this));
}


void xt_xmtx::identity() {
	nxCore::mem_copy(this, nxCalc::s_identityMtx, sizeof(*this));
}

void xt_xmtx::zero() {
	nxCore::mem_zero(this, sizeof(*this));
}


namespace nxVec {

float dist2(const cxVec& pos0, const cxVec& pos1) { return (pos1 - pos0).mag2(); }
float dist(const cxVec& pos0, const cxVec& pos1) { return (pos1 - pos0).mag(); }

cxVec get_axis(exAxis axis) {
	int i = (int)axis;
	cxVec v(0.0f);
	float* pV = (float*)&v;
	pV[(i >> 1) & 3] = (i & 1) ? -1.0f : 1.0f;
	return v;
}

cxVec from_polar_uv(float u, float v) {
	float azimuth = u * 2.0f * XD_PI;
	float sinAzi = ::mth_sinf(azimuth);
	float cosAzi = ::mth_cosf(azimuth);
	float elevation = (v - 1.0f) * XD_PI;
	float sinEle = ::mth_sinf(elevation);
	float cosEle = ::mth_cosf(elevation);
	float nx = cosAzi*sinEle;
	float ny = cosEle;
	float nz = sinAzi*sinEle;
	return cxVec(nx, ny, nz);
}

cxVec reflect(const cxVec& vec, const cxVec& nrm) {
	return vec - nrm*vec.dot(nrm)*2.0f;
}

cxVec decode_octa(const xt_float2& oct) {
	cxVec v;
	v.decode_octa(oct);
	return v;
}

cxVec decode_octa(const xt_half2& oct) {
	return decode_octa(oct.get());
}

cxVec decode_octa(const int16_t oct[2]) {
	xt_float2 o;
	o.set(float(oct[0]), float(oct[1]));
	o.scl(1.0f / 0x7FFF);
	return decode_octa(o);
}

cxVec decode_octa(const uint16_t oct[2]) {
	xt_float2 o;
	o.set(float(oct[0]), float(oct[1]));
	o.scl(2.0f / 0xFFFF);
	for (int i = 0; i < 2; ++i) {
		o[i] = nxCalc::clamp(o[i] - 1.0f, -1.0f, 1.0f);
	}
	return decode_octa(o);
}

cxVec rot_sc_x(const cxVec& v, const float s, const float c) {
	float y = v.y;
	float z = v.z;
	return cxVec(v.x, y*c - z*s, y*s + z*c);
}

cxVec rot_sc_y(const cxVec& v, const float s, const float c) {
	float z = v.z;
	float x = v.x;
	return cxVec(z*s + x*c, v.y, z*c - x*s);
}

cxVec rot_sc_z(const cxVec& v, const float s, const float c) {
	float x = v.x;
	float y = v.y;
	return cxVec(x*c - y*s, x*s + y*c, v.z);
}

cxVec rot_rad_x(const cxVec& v, const float rx) {
	return rot_sc_x(v, ::mth_sinf(rx), ::mth_cosf(rx));
}

cxVec rot_rad_y(const cxVec& v, const float ry) {
	return rot_sc_y(v, ::mth_sinf(ry), ::mth_cosf(ry));
}

cxVec rot_rad_z(const cxVec& v, const float rz) {
	return rot_sc_z(v, ::mth_sinf(rz), ::mth_cosf(rz));
}

cxVec rot_deg_x(const cxVec& v, const float dx) {
	return rot_rad_x(v, XD_DEG2RAD(dx));
}

cxVec rot_deg_y(const cxVec& v, const float dy) {
	return rot_rad_y(v, XD_DEG2RAD(dy));
}

cxVec rot_deg_z(const cxVec& v, const float dz) {
	return rot_rad_z(v, XD_DEG2RAD(dz));
}

cxVec rot_rad(const cxVec& v, const float rx, const float ry, const float rz, exRotOrd rord) {
	cxVec r(0.0f);
	switch (rord) {
		case exRotOrd::XYZ:
			r = rot_rad_z(rot_rad_y(rot_rad_x(v, rx), ry), rz);
			break;
		case exRotOrd::XZY:
			r = rot_rad_y(rot_rad_z(rot_rad_x(v, rx), rz), ry);
			break;
		case exRotOrd::YXZ:
			r = rot_rad_z(rot_rad_x(rot_rad_y(v, ry), rx), rz);
			break;
		case exRotOrd::YZX:
			r = rot_rad_x(rot_rad_z(rot_rad_y(v, ry), rz), rx);
			break;
		case exRotOrd::ZXY:
			r = rot_rad_y(rot_rad_x(rot_rad_z(v, rz), rx), ry);
			break;
		case exRotOrd::ZYX:
			r = rot_rad_x(rot_rad_y(rot_rad_z(v, rz), ry), rx);
			break;
	}
	return r;
}

cxVec rot_deg(const cxVec& v, const float dx, const float dy, const float dz, exRotOrd rord) {
	return rot_rad(v, XD_DEG2RAD(dx), XD_DEG2RAD(dy), XD_DEG2RAD(dz), rord);
}

}// nxVec


void cxVec::parse(const char* pStr) {
	float val[3];
	nxCore::mem_zero(val, sizeof(val));
#ifndef XD_NOSTDLIB
#if defined(_MSC_VER)
	::sscanf_s(pStr, "[%f,%f,%f]", &val[0], &val[1], &val[2]);
#else
	::sscanf(pStr, "[%f,%f,%f]", &val[0], &val[1], &val[2]);
#endif
#endif
	set(val[0], val[1], val[2]);
}

float cxVec::mag() const {
	cxVec v = *this;
	float m = v.max_abs_elem();
	float l = 0.0f;
	if (m) {
		v /= m;
		l = v.mag_fast() * m;
	}
	return l;
}

#ifndef XD_UNSAFE_VECNRM
#	define XD_UNSAFE_VECNRM 0
#endif

void cxVec::normalize(const cxVec& v) {
#if XD_UNSAFE_VECNRM
	normalize_fast(v);
#else
	cxVec n = v;
	float m = v.max_abs_elem();
	if (m > 0.0f) {
		n.scl(1.0f / m);
		n.scl(1.0f / n.mag_fast());
	}
	*this = n;
#endif
}

void cxVec::normalize_fast(const cxVec& v) {
	cxVec n = v;
	float m = v.mag_fast();
	if (m > 0.0f) {
		n.scl(1.0f / m);
	}
	*this = n;
}

// Refs:
//   Meyer et al. "On floating-point normal vectors"
//   Cigolle et al. "A Survey of Efficient Representations for Independent Unit Vectors"
//   http://jcgt.org/published/0003/02/01/
xt_float2 cxVec::encode_octa() const {
	xt_float2 oct;
	cxVec av = abs_val();
	float d = nxCalc::rcp0(av.x + av.y + av.z);
	float ox = x * d;
	float oy = y * d;
	if (z < 0.0f) {
		float tx = (1.0f - ::mth_fabsf(oy)) * (ox < 0.0f ? -1.0f : 1.0f);
		float ty = (1.0f - ::mth_fabsf(ox)) * (oy < 0.0f ? -1.0f : 1.0f);
		ox = tx;
		oy = ty;
	}
	oct.x = ox;
	oct.y = oy;
	return oct;
}

void cxVec::decode_octa(const xt_float2& oct) {
	float ox = oct.x;
	float oy = oct.y;
	float ax = ::mth_fabsf(ox);
	float ay = ::mth_fabsf(oy);
	z = 1.0f - ax - ay;
	if (z < 0.0f) {
		x = (1.0f - ay) * (ox < 0.0f ? -1.0f : 1.0f);
		y = (1.0f - ax) * (oy < 0.0f ? -1.0f : 1.0f);
	} else {
		x = ox;
		y = oy;
	}
	normalize();
}


void cxMtx::identity_sr() {
	nxCore::mem_copy(this, nxCalc::s_identityMtx, sizeof(float)*3*4);
}

void cxMtx::from_quat(const cxQuat& q) {
	cxVec ax = q.get_axis_x();
	cxVec ay = q.get_axis_y();
	cxVec az = q.get_axis_z();
	set_rot_frame(ax, ay, az);
}

void cxMtx::from_quat_and_pos(const cxQuat& qrot, const cxVec& vtrans) {
	from_quat(qrot);
	set_translation(vtrans);
}

namespace nxMtx {

void clean_rotations(cxMtx* pMtx, const int n) {
	if (!pMtx) return;
	if (n <= 0) return;
	float rmA[3 * 3];
	float rmU[3 * 3];
	float rmS[3];
	float rmV[3 * 3];
	float rmWk[3];
	for (int i = 0; i < n; ++i) {
		cxMtx m = pMtx[i];
		cxVec t = m.get_translation();
		for (int j = 0; j < 3; ++j) {
			m.get_row_vec(j).to_mem(&rmA[j * 3]);
		}
		bool res = nxLA::sv_decomp(rmU, rmS, rmV, rmWk, rmA, 3, 3);
		if (res) {
			nxLA::transpose(rmV, 3, 3);
			nxLA::mul_mm(rmA, rmU, rmV, 3, 3, 3);
			cxVec av[3];
			for (int j = 0; j < 3; ++j) {
				av[j].from_mem(&rmA[j * 3]);
			}
			m.set_rot_frame(av[0], av[1], av[2]);
			m.set_translation(t);
			pMtx[i] = m;
		}
	}
}

cxMtx mtx_from_xmtx(const xt_xmtx& xm) {
	cxMtx m;
	nxCore::mem_copy((void*)&m, &xm, sizeof(xm));
	m.set_translation(0.0f, 0.0f, 0.0f);
	m.transpose();
	return m;
}

xt_xmtx xmtx_from_mtx(const cxMtx& m) {
	xt_xmtx xm;
	cxMtx tm = m.get_transposed();
	nxCore::mem_copy(&xm, &tm, sizeof(xm));
	return xm;
}

cxVec xmtx_calc_vec(const xt_xmtx& m, const cxVec& v) {
	cxVec res;
	float x = v.x;
	float y = v.y;
	float z = v.z;
	res.x = x*m.m[0][0] + y*m.m[0][1] + z*m.m[0][2];
	res.y = x*m.m[1][0] + y*m.m[1][1] + z*m.m[1][2];
	res.z = x*m.m[2][0] + y*m.m[2][1] + z*m.m[2][2];
	return res;
}

cxVec xmtx_calc_pnt(const xt_xmtx& m, const cxVec& v) {
	cxVec res;
	float x = v.x;
	float y = v.y;
	float z = v.z;
	res.x = x*m.m[0][0] + y*m.m[0][1] + z*m.m[0][2] + m.m[0][3];
	res.y = x*m.m[1][0] + y*m.m[1][1] + z*m.m[1][2] + m.m[1][3];
	res.z = x*m.m[2][0] + y*m.m[2][1] + z*m.m[2][2] + m.m[2][3];
	return res;
}

#if defined(XD_USE_VEMA) && defined(XD_USE_VEMA_MTX)

xt_xmtx xmtx_concat(const xt_xmtx& a, const xt_xmtx& b) {
	xt_xmtx xm;
	VEMA_FN(ConcatMtx3x4F)(*((VemaMtx3x4F*)&xm), *((const VemaMtx3x4F*)&a), *((const VemaMtx3x4F*)&b));
	return xm;
}

#else

#ifndef XD_XMTX_CONCAT_VEC
#if defined(__GNUC__) && !defined(__clang__)
#	define XD_XMTX_CONCAT_VEC 1
#else
#	define XD_XMTX_CONCAT_VEC 0
#endif
#endif

xt_xmtx xmtx_concat(const xt_xmtx& a, const xt_xmtx& b) {
	float a00 = a.m[0][0]; float a01 = a.m[1][0]; float a02 = a.m[2][0];
	float a10 = a.m[0][1]; float a11 = a.m[1][1]; float a12 = a.m[2][1];
	float a20 = a.m[0][2]; float a21 = a.m[1][2]; float a22 = a.m[2][2];
	float a30 = a.m[0][3]; float a31 = a.m[1][3]; float a32 = a.m[2][3];
	float b00 = b.m[0][0]; float b01 = b.m[1][0]; float b02 = b.m[2][0];
	float b10 = b.m[0][1]; float b11 = b.m[1][1]; float b12 = b.m[2][1];
	float b20 = b.m[0][2]; float b21 = b.m[1][2]; float b22 = b.m[2][2];
	float b30 = b.m[0][3]; float b31 = b.m[1][3]; float b32 = b.m[2][3];
	xt_xmtx xm;
#if XD_XMTX_CONCAT_VEC
	float* p = xm;
	float x0[] = { a00, a10, a20, a30,  a00, a10, a20, a30,  a00, a10, a20, a30 };
	float x1[] = { a01, a11, a21, a31,  a01, a11, a21, a31,  a01, a11, a21, a31 };
	float x2[] = { a02, a12, a22, a32,  a02, a12, a22, a32,  a02, a12, a22, a32 };
#	if XD_XMTX_CONCAT_VEC < 2
	for (int i = 0; i < 0x4; ++i) { p[i] = x0[i]*b00 + x1[i]*b10 + x2[i]*b20; }
	for (int i = 4; i < 0x8; ++i) { p[i] = x0[i]*b01 + x1[i]*b11 + x2[i]*b21; }
	for (int i = 8; i < 0xC; ++i) { p[i] = x0[i]*b02 + x1[i]*b12 + x2[i]*b22; }
#	else
	for (int i = 0; i < 0x4; ++i) { x0[i] *= b00; }
	for (int i = 4; i < 0x8; ++i) { x0[i] *= b01; }
	for (int i = 8; i < 0xC; ++i) { x0[i] *= b02; }
	for (int i = 0; i < 0x4; ++i) { x1[i] *= b10; }
	for (int i = 4; i < 0x8; ++i) { x1[i] *= b11; }
	for (int i = 8; i < 0xC; ++i) { x1[i] *= b12; }
	for (int i = 0; i < 0x4; ++i) { x2[i] *= b20; }
	for (int i = 4; i < 0x8; ++i) { x2[i] *= b21; }
	for (int i = 8; i < 0xC; ++i) { x2[i] *= b22; }
	for (int i = 0; i < 12; ++i) {
		p[i] = x0[i] + x1[i] + x2[i];
	}
#	endif
#else
	xm.m[0][0] = a00*b00 + a01*b10 + a02*b20;
	xm.m[0][1] = a10*b00 + a11*b10 + a12*b20;
	xm.m[0][2] = a20*b00 + a21*b10 + a22*b20;
	xm.m[0][3] = a30*b00 + a31*b10 + a32*b20;
	xm.m[1][0] = a00*b01 + a01*b11 + a02*b21;
	xm.m[1][1] = a10*b01 + a11*b11 + a12*b21;
	xm.m[1][2] = a20*b01 + a21*b11 + a22*b21;
	xm.m[1][3] = a30*b01 + a31*b11 + a32*b21;
	xm.m[2][0] = a00*b02 + a01*b12 + a02*b22;
	xm.m[2][1] = a10*b02 + a11*b12 + a12*b22;
	xm.m[2][2] = a20*b02 + a21*b12 + a22*b22;
	xm.m[2][3] = a30*b02 + a31*b12 + a32*b22;
#endif
	xm.m[0][3] += b30;
	xm.m[1][3] += b31;
	xm.m[2][3] += b32;
	return xm;
}

#endif

xt_xmtx xmtx_basis(const cxVec& ax, const cxVec& ay, const cxVec& az, const cxVec& pos) {
	xt_xmtx xm;
	xm.m[0][0] = ax.x;
	xm.m[1][0] = ax.y;
	xm.m[2][0] = ax.z;
	xm.m[0][1] = ay.x;
	xm.m[1][1] = ay.y;
	xm.m[2][1] = ay.z;
	xm.m[0][2] = az.x;
	xm.m[1][2] = az.y;
	xm.m[2][2] = az.z;
	xm.m[0][3] = pos.x;
	xm.m[1][3] = pos.y;
	xm.m[2][3] = pos.z;
	return xm;
}

xt_xmtx xmtx_from_quat_pos(const cxQuat& quat, const cxVec& pos) {
	cxVec ax = quat.get_axis_x();
	cxVec ay = quat.get_axis_y();
	cxVec az = quat.get_axis_z();
	return xmtx_basis(ax, ay, az, pos);
}

xt_xmtx xmtx_from_deg_pos(const float dx, const float dy, const float dz, const cxVec& pos, exRotOrd rord) {
	cxVec ax = nxVec::rot_deg(nxVec::get_axis(exAxis::PLUS_X), dx, dy, dz, rord);
	cxVec ay = nxVec::rot_deg(nxVec::get_axis(exAxis::PLUS_Y), dx, dy, dz, rord);
	cxVec az = nxVec::rot_deg(nxVec::get_axis(exAxis::PLUS_Z), dx, dy, dz, rord);
	return xmtx_basis(ax, ay, az, pos);
}

cxVec xmtx_get_pos(const xt_xmtx& xm) {
	return cxVec(xm.m[0][3], xm.m[1][3], xm.m[2][3]);
}

void xmtx_set_pos(xt_xmtx& xm, const cxVec& pos) {
	xm.m[0][3] = pos.x;
	xm.m[1][3] = pos.y;
	xm.m[2][3] = pos.z;
}

void dump_hgeo(FILE* pOut, const cxMtx* pMtx, const int n, float scl) {
#if XD_FILEFUNCS_ENABLED
	if (!pOut) return;
	if (!pMtx) return;
	if (n <= 0) return;
	if (scl <= 0.0f) return;
	::fprintf(pOut, "PGEOMETRY V5\n");
	::fprintf(pOut, "NPoints %d NPrims %d\n", n * 4, n * 3);
	::fprintf(pOut, "NPointGroups 0 NPrimGroups 0\n");
	::fprintf(pOut, "NPointAttrib 0 NVertexAttrib 0 NPrimAttrib 1 NAttrib 0\n");
	for (int i = 0; i < n; ++i) {
		cxVec pnt[4];
		pnt[0] = pMtx[i].get_translation();
		const exAxis ax[] = { exAxis::PLUS_X, exAxis::PLUS_Y, exAxis::PLUS_Z };
		for (int j = 0; j < 3; ++j) {
			pnt[j + 1] = pMtx[i].calc_pnt(nxVec::get_axis(ax[j]));
		}
		if (scl != 1.0f) {
			for (int j = 0; j < 3; ++j) {
				pnt[j + 1].scl(scl);
			}
		}
		for (int j = 0; j < 4; ++j) {
			::fprintf(pOut, "%.10f %.10f %.10f 1\n", pnt[j].x, pnt[j].y, pnt[j].z);
		}
	}
	::fprintf(pOut, "PrimitiveAttrib\n");
	::fprintf(pOut, "Cd 3 float 1 1 1\n");
	::fprintf(pOut, "Run %d Poly\n", n * 3);
	for (int i = 0; i < n; ++i) {
		int p0 = i * 4;
		::fprintf(pOut, " 2 > %d %d [1 0 0]\n", p0, p0 + 1);
		::fprintf(pOut, " 2 > %d %d [0 1 0]\n", p0, p0 + 2);
		::fprintf(pOut, " 2 > %d %d [0 0 1]\n", p0, p0 + 3);
	}
	::fprintf(pOut, "beginExtra\n");
	::fprintf(pOut, "endExtra\n");
#endif
}

void dump_hgeo(const char* pOutPath, const cxMtx* pMtx, const int n, float scl) {
#if XD_FILEFUNCS_ENABLED
	FILE* pOut = nxSys::fopen_w_txt(pOutPath);
	if (!pOut) {
		return;
	}
	dump_hgeo(pOut, pMtx, n, scl);
	::fclose(pOut);
#endif
}

} // nxMtx

cxQuat cxMtx::to_quat() const {
	cxQuat q;
	float s;
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float w = 1.0f;
	float trace = m[0][0] + m[1][1] + m[2][2];
	if (trace > 0.0f) {
		s = ::mth_sqrtf(trace + 1.0f);
		w = s * 0.5f;
		s = 0.5f / s;
		x = (m[1][2] - m[2][1]) * s;
		y = (m[2][0] - m[0][2]) * s;
		z = (m[0][1] - m[1][0]) * s;
	} else {
		if (m[1][1] > m[0][0]) {
			if (m[2][2] > m[1][1]) {
				s = m[2][2] - m[1][1] - m[0][0];
				s = ::mth_sqrtf(s + 1.0f);
				z = s * 0.5f;
				if (s != 0.0f) {
					s = 0.5f / s;
				}
				w = (m[0][1] - m[1][0]) * s;
				x = (m[2][0] + m[0][2]) * s;
				y = (m[2][1] + m[1][2]) * s;
			} else {
				s = m[1][1] - m[2][2] - m[0][0];
				s = ::mth_sqrtf(s + 1.0f);
				y = s * 0.5f;
				if (s != 0.0f) {
					s = 0.5f / s;
				}
				w = (m[2][0] - m[0][2]) * s;
				z = (m[1][2] + m[2][1]) * s;
				x = (m[1][0] + m[0][1]) * s;
			}
		} else if (m[2][2] > m[0][0]) {
			s = m[2][2] - m[1][1] - m[0][0];
			s = ::mth_sqrtf(s + 1.0f);
			z = s * 0.5f;
			if (s != 0.0f) {
				s = 0.5f / s;
			}
			w = (m[0][1] - m[1][0]) * s;
			x = (m[2][0] + m[0][2]) * s;
			y = (m[2][1] + m[1][2]) * s;
		} else {
			s = m[0][0] - m[1][1] - m[2][2];
			s = ::mth_sqrtf(s + 1.0f);
			x = s * 0.5f;
			if (s != 0.0f) {
				s = 0.5f / s;
			}
			w = (m[1][2] - m[2][1]) * s;
			y = (m[0][1] + m[1][0]) * s;
			z = (m[0][2] + m[2][0]) * s;
		}
	}
	q.set(x, y, z, w);
	return q;
}

void cxMtx::transpose() {
	float t;
	t = m[0][1]; m[0][1] = m[1][0]; m[1][0] = t;
	t = m[0][2]; m[0][2] = m[2][0]; m[2][0] = t;
	t = m[0][3]; m[0][3] = m[3][0]; m[3][0] = t;
	t = m[1][2]; m[1][2] = m[2][1]; m[2][1] = t;
	t = m[1][3]; m[1][3] = m[3][1]; m[3][1] = t;
	t = m[2][3]; m[2][3] = m[3][2]; m[3][2] = t;
}

void cxMtx::transpose(const cxMtx& mtx) {
	float t;
	m[0][0] = mtx.m[0][0];
	m[1][1] = mtx.m[1][1];
	m[2][2] = mtx.m[2][2];
	m[3][3] = mtx.m[3][3];
	t = mtx.m[0][1]; m[0][1] = mtx.m[1][0]; m[1][0] = t;
	t = mtx.m[0][2]; m[0][2] = mtx.m[2][0]; m[2][0] = t;
	t = mtx.m[0][3]; m[0][3] = mtx.m[3][0]; m[3][0] = t;
	t = mtx.m[1][2]; m[1][2] = mtx.m[2][1]; m[2][1] = t;
	t = mtx.m[1][3]; m[1][3] = mtx.m[3][1]; m[3][1] = t;
	t = mtx.m[2][3]; m[2][3] = mtx.m[3][2]; m[3][2] = t;
}

void cxMtx::transpose_sr() {
	float t;
	t = m[0][1]; m[0][1] = m[1][0]; m[1][0] = t;
	t = m[0][2]; m[0][2] = m[2][0]; m[2][0] = t;
	t = m[1][2]; m[1][2] = m[2][1]; m[2][1] = t;
}

void cxMtx::transpose_sr(const cxMtx& mtx) {
	nxCore::mem_copy(this, mtx, sizeof(cxMtx));
	transpose_sr();
}

void cxMtx::invert() {
#if XD_USE_LA
	int idx[4 * 3];
	bool ok = nxLA::inv_gj((float*)this, 4, idx);
	if (!ok) {
		nxCore::mem_zero(*this, sizeof(cxMtx));
	}
#else
	invert_fast();
#endif
}

void cxMtx::invert(const cxMtx& mtx) {
	nxCore::mem_copy(this, mtx, sizeof(cxMtx));
	invert();
}

void cxMtx::invert_fast() {
	float det;
	float a0, a1, a2, a3, a4, a5;
	float b0, b1, b2, b3, b4, b5;

	a0 = m[0][0]*m[1][1] - m[0][1]*m[1][0];
	a1 = m[0][0]*m[1][2] - m[0][2]*m[1][0];
	a2 = m[0][0]*m[1][3] - m[0][3]*m[1][0];
	a3 = m[0][1]*m[1][2] - m[0][2]*m[1][1];
	a4 = m[0][1]*m[1][3] - m[0][3]*m[1][1];
	a5 = m[0][2]*m[1][3] - m[0][3]*m[1][2];

	b0 = m[2][0]*m[3][1] - m[2][1]*m[3][0];
	b1 = m[2][0]*m[3][2] - m[2][2]*m[3][0];
	b2 = m[2][0]*m[3][3] - m[2][3]*m[3][0];
	b3 = m[2][1]*m[3][2] - m[2][2]*m[3][1];
	b4 = m[2][1]*m[3][3] - m[2][3]*m[3][1];
	b5 = m[2][2]*m[3][3] - m[2][3]*m[3][2];

	det = a0*b5 - a1*b4 + a2*b3 + a3*b2 - a4*b1 + a5*b0;

	if (det == 0.0f) {
		nxCore::mem_zero(*this, sizeof(cxMtx));
	} else {
		cxMtx im;

		im.m[0][0] =  m[1][1]*b5 - m[1][2]*b4 + m[1][3]*b3;
		im.m[1][0] = -m[1][0]*b5 + m[1][2]*b2 - m[1][3]*b1;
		im.m[2][0] =  m[1][0]*b4 - m[1][1]*b2 + m[1][3]*b0;
		im.m[3][0] = -m[1][0]*b3 + m[1][1]*b1 - m[1][2]*b0;

		im.m[0][1] = -m[0][1]*b5 + m[0][2]*b4 - m[0][3]*b3;
		im.m[1][1] =  m[0][0]*b5 - m[0][2]*b2 + m[0][3]*b1;
		im.m[2][1] = -m[0][0]*b4 + m[0][1]*b2 - m[0][3]*b0;
		im.m[3][1] =  m[0][0]*b3 - m[0][1]*b1 + m[0][2]*b0;

		im.m[0][2] =  m[3][1]*a5 - m[3][2]*a4 + m[3][3]*a3;
		im.m[1][2] = -m[3][0]*a5 + m[3][2]*a2 - m[3][3]*a1;
		im.m[2][2] =  m[3][0]*a4 - m[3][1]*a2 + m[3][3]*a0;
		im.m[3][2] = -m[3][0]*a3 + m[3][1]*a1 - m[3][2]*a0;

		im.m[0][3] = -m[2][1]*a5 + m[2][2]*a4 - m[2][3]*a3;
		im.m[1][3] =  m[2][0]*a5 - m[2][2]*a2 + m[2][3]*a1;
		im.m[2][3] = -m[2][0]*a4 + m[2][1]*a2 - m[2][3]*a0;
		im.m[3][3] =  m[2][0]*a3 - m[2][1]*a1 + m[2][2]*a0;

		float idet = 1.0f / det;
		float* pDst = &m[0][0];
		float* pSrc = &im.m[0][0];
		for (int i = 0; i < 4 * 4; ++i) {
			pDst[i] = pSrc[i] * idet;
		}
	}
}

void cxMtx::invert_lu() {
	float lu[4 * 4];
	nxLA::mtx_cpy(lu, (float*)this, 4, 4);
	float wk[4];
	int idx[4];
	bool ok = nxLA::lu_decomp(lu, 4, wk, idx);
	if (!ok) {
		nxCore::mem_zero(*this, sizeof(cxMtx));
		return;
	}
	for (int i = 0; i < 4; ++i) {
		nxLA::lu_solve(m[i], lu, nxCalc::s_identityMtx[i], idx, 4);
	}
	transpose();
}

void cxMtx::invert_lu_hi() {
	double lu[4 * 4];
	nxLA::mtx_cpy(lu, (float*)this, 4, 4);
	double wk[4];
	int idx[4];
	bool ok = nxLA::lu_decomp(lu, 4, wk, idx);
	if (!ok) {
		nxCore::mem_zero(*this, sizeof(cxMtx));
		return;
	}
	double inv[4][4];
	double b[4][4];
	nxLA::mtx_cpy(&b[0][0], &nxCalc::s_identityMtx[0][0], 4, 4);
	for (int i = 0; i < 4; ++i) {
		nxLA::lu_solve(inv[i], lu, b[i], idx, 4);
	}
	nxLA::mtx_cpy((float*)this, &inv[0][0], 4, 4);
	transpose();
}

void cxMtx::mul(const cxMtx& mtx) {
	mul(*this, mtx);
}

#ifndef XD_CLANG_EXPERIMENTAL_
void cxMtx::mul(const cxMtx& m1, const cxMtx& m2) {
#if defined(XD_USE_VEMA) && defined(XD_USE_VEMA_MTX)
	VEMA_FN(MulMtx4x4F)(*((VemaMtx4x4F*)this), *((const VemaMtx4x4F*)&m1), *((const VemaMtx4x4F*)&m2));
#elif XD_USE_LA
	float res[4 * 4];
	nxLA::mul_mm(res, (const float*)m1, (const float*)m2, 4, 4, 4);
	from_mem(res);
#else
	cxMtx m0;
	m0.m[0][0] = m1.m[0][0]*m2.m[0][0] + m1.m[0][1]*m2.m[1][0] + m1.m[0][2]*m2.m[2][0] + m1.m[0][3]*m2.m[3][0];
	m0.m[0][1] = m1.m[0][0]*m2.m[0][1] + m1.m[0][1]*m2.m[1][1] + m1.m[0][2]*m2.m[2][1] + m1.m[0][3]*m2.m[3][1];
	m0.m[0][2] = m1.m[0][0]*m2.m[0][2] + m1.m[0][1]*m2.m[1][2] + m1.m[0][2]*m2.m[2][2] + m1.m[0][3]*m2.m[3][2];
	m0.m[0][3] = m1.m[0][0]*m2.m[0][3] + m1.m[0][1]*m2.m[1][3] + m1.m[0][2]*m2.m[2][3] + m1.m[0][3]*m2.m[3][3];

	m0.m[1][0] = m1.m[1][0]*m2.m[0][0] + m1.m[1][1]*m2.m[1][0] + m1.m[1][2]*m2.m[2][0] + m1.m[1][3]*m2.m[3][0];
	m0.m[1][1] = m1.m[1][0]*m2.m[0][1] + m1.m[1][1]*m2.m[1][1] + m1.m[1][2]*m2.m[2][1] + m1.m[1][3]*m2.m[3][1];
	m0.m[1][2] = m1.m[1][0]*m2.m[0][2] + m1.m[1][1]*m2.m[1][2] + m1.m[1][2]*m2.m[2][2] + m1.m[1][3]*m2.m[3][2];
	m0.m[1][3] = m1.m[1][0]*m2.m[0][3] + m1.m[1][1]*m2.m[1][3] + m1.m[1][2]*m2.m[2][3] + m1.m[1][3]*m2.m[3][3];

	m0.m[2][0] = m1.m[2][0]*m2.m[0][0] + m1.m[2][1]*m2.m[1][0] + m1.m[2][2]*m2.m[2][0] + m1.m[2][3]*m2.m[3][0];
	m0.m[2][1] = m1.m[2][0]*m2.m[0][1] + m1.m[2][1]*m2.m[1][1] + m1.m[2][2]*m2.m[2][1] + m1.m[2][3]*m2.m[3][1];
	m0.m[2][2] = m1.m[2][0]*m2.m[0][2] + m1.m[2][1]*m2.m[1][2] + m1.m[2][2]*m2.m[2][2] + m1.m[2][3]*m2.m[3][2];
	m0.m[2][3] = m1.m[2][0]*m2.m[0][3] + m1.m[2][1]*m2.m[1][3] + m1.m[2][2]*m2.m[2][3] + m1.m[2][3]*m2.m[3][3];

	m0.m[3][0] = m1.m[3][0]*m2.m[0][0] + m1.m[3][1]*m2.m[1][0] + m1.m[3][2]*m2.m[2][0] + m1.m[3][3]*m2.m[3][0];
	m0.m[3][1] = m1.m[3][0]*m2.m[0][1] + m1.m[3][1]*m2.m[1][1] + m1.m[3][2]*m2.m[2][1] + m1.m[3][3]*m2.m[3][1];
	m0.m[3][2] = m1.m[3][0]*m2.m[0][2] + m1.m[3][1]*m2.m[1][2] + m1.m[3][2]*m2.m[2][2] + m1.m[3][3]*m2.m[3][2];
	m0.m[3][3] = m1.m[3][0]*m2.m[0][3] + m1.m[3][1]*m2.m[1][3] + m1.m[3][2]*m2.m[2][3] + m1.m[3][3]*m2.m[3][3];

	*this = m0;
#endif
}

#else /* XD_CLANG_EXPERIMENTAL_ */

typedef float xt_fmtxmul __attribute__((matrix_type(4, 4)));

void cxMtx::mul(const cxMtx& m1, const cxMtx& m2) {
	*(xt_fmtxmul*)this = *(xt_fmtxmul*)&m2 * *(xt_fmtxmul*)&m1;
}
#endif /* XD_CLANG_EXPERIMENTAL_ */

void cxMtx::add(const cxMtx& mtx) {
	add(*this, mtx);
}

void cxMtx::add(const cxMtx& m1, const cxMtx& m2) {
	nxLA::add_mm((float*)this, (const float*)m1, (const float*)m2, 4, 4);
}

void cxMtx::sub(const cxMtx& mtx) {
	sub(*this, mtx);
}

void cxMtx::sub(const cxMtx& m1, const cxMtx& m2) {
	nxLA::sub_mm((float*)this, (const float*)m1, (const float*)m2, 4, 4);
}

void cxMtx::rev_mul(const cxMtx& mtx) {
	mul(mtx, *this);
}

// Ref: http://graphics.pixar.com/library/OrthonormalB/paper.pdf
void cxMtx::from_upvec(const cxVec& n) {
	float s = n.z < 0 ? -1.0f : 1.0f;
	float a = -nxCalc::rcp0(s - n.z);
	float b = n.x * n.y * a;
	cxVec nx(1.0f + s*n.x*n.x*a, s*b, s*n.x);
	cxVec nz(b, s + n.y*n.y*a, n.y);
	set_rot_frame(nx, n, nz);
}

void cxMtx::orient_zy(const cxVec& axisZ, const cxVec& axisY, bool normalizeInput) {
	cxVec az = normalizeInput ? axisZ.get_normalized() : axisZ;
	cxVec ay = normalizeInput ? axisY.get_normalized() : axisY;
	cxVec ax = nxVec::cross(ay, az).get_normalized();
	ay = nxVec::cross(az, ax).get_normalized();
	set_rot_frame(ax, ay, az);
}

void cxMtx::orient_zx(const cxVec& axisZ, const cxVec& axisX, bool normalizeInput) {
	cxVec az = normalizeInput ? axisZ.get_normalized() : axisZ;
	cxVec ax = normalizeInput ? axisX.get_normalized() : axisX;
	cxVec ay = nxVec::cross(az, ax).get_normalized();
	ax = nxVec::cross(ay, az).get_normalized();
	set_rot_frame(ax, ay, az);
}

void cxMtx::set_rot_frame(const cxVec& axisX, const cxVec& axisY, const cxVec& axisZ) {
	m[0][0] = axisX.x;
	m[0][1] = axisX.y;
	m[0][2] = axisX.z;
	m[0][3] = 0.0f;

	m[1][0] = axisY.x;
	m[1][1] = axisY.y;
	m[1][2] = axisY.z;
	m[1][3] = 0.0f;

	m[2][0] = axisZ.x;
	m[2][1] = axisZ.y;
	m[2][2] = axisZ.z;
	m[2][3] = 0.0f;

	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
}

void cxMtx::set_rot_n(const cxVec& axis, float ang) {
	cxVec axis2 = axis * axis;
	float s = ::mth_sinf(ang);
	float c = ::mth_cosf(ang);
	float t = 1.0f - c;
	float x = axis.x;
	float y = axis.y;
	float z = axis.z;
	float xy = x*y;
	float xz = x*z;
	float yz = y*z;
	m[0][0] = t*axis2.x + c;
	m[0][1] = t*xy + s*z;
	m[0][2] = t*xz - s*y;
	m[0][3] = 0.0f;

	m[1][0] = t*xy - s*z;
	m[1][1] = t*axis2.y + c;
	m[1][2] = t*yz + s*x;
	m[1][3] = 0.0f;

	m[2][0] = t*xz + s*y;
	m[2][1] = t*yz - s*x;
	m[2][2] = t*axis2.z + c;
	m[2][3] = 0.0f;

	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
}

void cxMtx::set_rot(const cxVec& axis, float ang) {
	set_rot_n(axis.get_normalized(), ang);
}

void cxMtx::set_rot_x(float rx) {
	set_rot_n(nxVec::get_axis(exAxis::PLUS_X), rx);
}

void cxMtx::set_rot_y(float ry) {
	set_rot_n(nxVec::get_axis(exAxis::PLUS_Y), ry);
}

void cxMtx::set_rot_z(float rz) {
	set_rot_n(nxVec::get_axis(exAxis::PLUS_Z), rz);
}

void cxMtx::set_rot(float rx, float ry, float rz, exRotOrd ord) {
	static struct { uint8_t m0, m1, m2; } tbl[] = {
		/* XYZ */ {0, 1, 2},
		/* XZY */ {0, 2, 1},
		/* YXZ */ {1, 0, 2},
		/* YZX */ {1, 2, 0},
		/* ZXY */ {2, 0, 1},
		/* ZYX */ {2, 1, 0}
	};
	uint32_t idx = (uint32_t)ord;
	if (idx >= XD_ARY_LEN(tbl)) {
		identity();
	} else {
		cxMtx rm[3];
		rm[0].set_rot_x(rx);
		rm[1].set_rot_y(ry);
		rm[2].set_rot_z(rz);
		int m0 = tbl[idx].m0;
		int m1 = tbl[idx].m1;
		int m2 = tbl[idx].m2;
		*this = rm[m0];
		mul(rm[m1]);
		mul(rm[m2]);
	}
}

void cxMtx::set_rot_degrees(const cxVec& r, exRotOrd ord) {
	cxVec rr = r * XD_DEG2RAD(1.0f);
	set_rot(rr.x, rr.y, rr.z, ord);
}

bool cxMtx::is_valid_rot(float tol) const {
	cxVec r0 = get_row_vec(0);
	cxVec r1 = get_row_vec(1);
	cxVec r2 = get_row_vec(2);
	float v = ::mth_fabsf(nxVec::scalar_triple(r0, r1, r2));
	if (::mth_fabsf(v - 1.0f) > tol) return false;

	v = r0.dot(r0);
	if (::mth_fabsf(v - 1.0f) > tol) return false;
	v = r1.dot(r1);
	if (::mth_fabsf(v - 1.0f) > tol) return false;
	v = r2.dot(r2);
	if (::mth_fabsf(v - 1.0f) > tol) return false;

	v = r0.dot(r1);
	if (::mth_fabsf(v) > tol) return false;
	v = r1.dot(r2);
	if (::mth_fabsf(v) > tol) return false;
	v = r0.dot(r2);
	if (::mth_fabsf(v) > tol) return false;

	return true;
}

static inline float limit_pi(float rad) {
	rad = ::mth_fmodf(rad, XD_PI*2);
	if (::mth_fabsf(rad) > XD_PI) {
		if (rad < 0.0f) {
			rad = XD_PI*2 + rad;
		} else {
			rad = rad - XD_PI*2;
		}
	}
	return rad;
}

static struct { uint8_t i0, i1, i2, s; } s_getRotTbl[] = {
	/* XYZ */ { 0, 1, 2, 1 },
	/* XZY */ { 0, 2, 1, 0 },
	/* YXZ */ { 1, 0, 2, 0 },
	/* YZX */ { 1, 2, 0, 1 },
	/* ZXY */ { 2, 0, 1, 1 },
	/* ZYX */ { 2, 1, 0, 0 }
};

cxVec cxMtx::get_rot(exRotOrd ord) const {
	cxVec rv(0.0f);
	cxQuat q = to_quat();
	const float eps = 1.0e-6f;
	int axisMask = 0;
	for (int i = 0; i < 4; ++i) {
		if (::mth_fabsf(q[i]) < eps) {
			axisMask |= 1 << i;
		}
	}
	bool singleAxis = false;
	float qw = nxCalc::clamp(q.w, -1.0f, 1.0f);
	switch (axisMask) {
		case 6: /* 0110 -> X */
			rv.x = ::mth_acosf(qw) * 2.0f;
			if (q.x < 0.0f) rv.x = -rv.x;
			rv.x = limit_pi(rv.x);
			singleAxis = true;
			break;
		case 5: /* 0101 -> Y */
			rv.y = ::mth_acosf(qw) * 2.0f;
			if (q.y < 0.0f) rv.y = -rv.y;
			rv.y = limit_pi(rv.y);
			singleAxis = true;
			break;
		case 3: /* 0011 -> Z */
			rv.z = ::mth_acosf(qw) * 2.0f;
			if (q.z < 0.0f) rv.z = -rv.z;
			rv.z = limit_pi(rv.z);
			singleAxis = true;
			break;
		case 7: /* 0111 -> identity */
			singleAxis = true;
			break;
	}
	if (singleAxis) {
		return rv;
	}
	if ((uint32_t)ord >= XD_ARY_LEN(s_getRotTbl)) {
		ord = exRotOrd::XYZ;
	}
	int i0 = s_getRotTbl[(int)ord].i0;
	int i1 = s_getRotTbl[(int)ord].i1;
	int i2 = s_getRotTbl[(int)ord].i2;
	float sgn = s_getRotTbl[(int)ord].s ? 1.0f : -1.0f;
	cxVec rm[3];
	rm[0].set(m[i0][i0], m[i0][i1], m[i0][i2]);
	rm[1].set(m[i1][i0], m[i1][i1], m[i1][i2]);
	rm[2].set(m[i2][i0], m[i2][i1], m[i2][i2]);
	float r[3] = { 0, 0, 0 };
	r[i0] = ::mth_atan2f(rm[1][2], rm[2][2]);
	r[i1] = ::mth_atan2f(-rm[0][2], ::mth_sqrtf(nxCalc::sq(rm[0][0]) + nxCalc::sq(rm[0][1])));
	float s = ::mth_sinf(r[i0]);
	float c = ::mth_cosf(r[i0]);
	r[i2] = ::mth_atan2f(s*rm[2][0] - c*rm[1][0], c*rm[1][1] - s*rm[2][1]);
	for (int i = 0; i < 3; ++i) {
		r[i] *= sgn;
	}
	for (int i = 0; i < 3; ++i) {
		r[i] = limit_pi(r[i]);
	}
	rv.from_mem(r);
	return rv;
}

void cxMtx::mk_scl(float sx, float sy, float sz) {
	identity();
	m[0][0] = sx;
	m[1][1] = sy;
	m[2][2] = sz;
}

cxVec cxMtx::get_scl(cxMtx* pMtxRT, exTransformOrd xord) const {
	cxVec scl(0.0f);
	cxVec iscl;
	switch (xord) {
		case exTransformOrd::SRT:
		case exTransformOrd::STR:
		case exTransformOrd::TSR:
			scl.x = get_row_vec(0).mag();
			scl.y = get_row_vec(1).mag();
			scl.z = get_row_vec(2).mag();
			break;
		case exTransformOrd::RST:
		case exTransformOrd::RTS:
		case exTransformOrd::TRS:
			scl.x = cxVec(m[0][0], m[1][0], m[2][0]).mag();
			scl.y = cxVec(m[0][1], m[1][1], m[2][1]).mag();
			scl.z = cxVec(m[0][2], m[1][2], m[2][2]).mag();
			break;
	}
	if (pMtxRT) {
		iscl = scl.inv_val();
		switch (xord) {
			case exTransformOrd::SRT:
			case exTransformOrd::STR:
				pMtxRT->mk_scl(iscl);
				pMtxRT->mul(*this);
				break;
			case exTransformOrd::RST:
				*pMtxRT = get_sr();
				pMtxRT->mul(nxMtx::mk_scl(iscl));
				pMtxRT->set_translation(get_translation());
				break;
			case exTransformOrd::RTS:
			case exTransformOrd::TRS:
				*pMtxRT = *this;
				pMtxRT->mul(nxMtx::mk_scl(iscl));
				break;
			case exTransformOrd::TSR:
				pMtxRT->mk_scl(iscl);
				pMtxRT->mul(get_sr());
				pMtxRT->set_translation((get_inverted() * *pMtxRT).get_translation().neg_val());
				break;
		}
	}
	return scl;
}

void cxMtx::calc_xform(const cxMtx& mtxT, const cxMtx& mtxR, const cxMtx& mtxS, exTransformOrd ord) {
	const uint8_t S = 0;
	const uint8_t R = 1;
	const uint8_t T = 2;
	static struct { uint8_t m0, m1, m2; } tbl[] = {
		/* SRT */ { S, R, T },
		/* STR */ { S, T, R },
		/* RST */ { R, S, T },
		/* RTS */ { R, T, S },
		/* TSR */ { T, S, R },
		/* TRS */ { T, R, S }
	};
	const cxMtx* lst[3];
	lst[S] = &mtxS;
	lst[R] = &mtxR;
	lst[T] = &mtxT;

	if ((uint32_t)ord >= XD_ARY_LEN(tbl)) {
		ord = exTransformOrd::SRT;
	}
	int m0 = tbl[(int)ord].m0;
	int m1 = tbl[(int)ord].m1;
	int m2 = tbl[(int)ord].m2;
	*this = *lst[m0];
	mul(*lst[m1]);
	mul(*lst[m2]);
}

void cxMtx::mk_view(const cxVec& pos, const cxVec& tgt, const cxVec& upvec, cxMtx* pInv) {
	cxVec up = upvec;
	cxVec dir = (tgt - pos).get_normalized();
	cxVec side = nxVec::cross(up, dir).get_normalized();
	up = nxVec::cross(side, dir);
	set_rot_frame(side.neg_val(), up.neg_val(), dir.neg_val());
	if (pInv) {
		*pInv = *this;
		pInv->set_translation(pos);
	}
	transpose_sr();
	cxVec org = calc_vec(pos.neg_val());
	set_translation(org);
}

void cxMtx::mk_proj(float fovy, float aspect, float znear, float zfar) {
	float h = fovy*0.5f;
	float s = ::mth_sinf(h);
	float c = ::mth_cosf(h);
	float cot = c / s;
	float q = zfar / (zfar - znear);
	nxCore::mem_zero(this, sizeof(cxMtx));
	m[2][3] = -1.0f;
	m[0][0] = cot / aspect;
	m[1][1] = cot;
	m[2][2] = -q;
	m[3][2] = -q * znear;
}

cxVec cxMtx::calc_vec(const cxVec& v) const {
#if XD_USE_LA
	float res[4];
	nxLA::mul_vm(res, (float*)&v, (float*)m, 3, 4);
	return nxVec::from_mem(res);
#else
	float x = v.x;
	float y = v.y;
	float z = v.z;
	float tx = x*m[0][0] + y*m[1][0] + z*m[2][0];
	float ty = x*m[0][1] + y*m[1][1] + z*m[2][1];
	float tz = x*m[0][2] + y*m[1][2] + z*m[2][2];
	return cxVec(tx, ty, tz);
#endif
}

cxVec cxMtx::calc_pnt(const cxVec& v) const {
#if XD_USE_LA
	float vec[4] = { v.x, v.y, v.z, 1.0f };
	float res[4];
	nxLA::mul_vm(res, vec, (float*)m, 4, 4);
	return nxVec::from_mem(res);
#else
	float x = v.x;
	float y = v.y;
	float z = v.z;
	float tx = x*m[0][0] + y*m[1][0] + z*m[2][0] + m[3][0];
	float ty = x*m[0][1] + y*m[1][1] + z*m[2][1] + m[3][1];
	float tz = x*m[0][2] + y*m[1][2] + z*m[2][2] + m[3][2];
	return cxVec(tx, ty, tz);
#endif
}

xt_float4 cxMtx::apply(const xt_float4& qv) const {
	float x = qv.x;
	float y = qv.y;
	float z = qv.z;
	float w = qv.w;
	xt_float4 res;
	res.x = x*m[0][0] + y*m[1][0] + z*m[2][0] + w*m[3][0];
	res.y = x*m[0][1] + y*m[1][1] + z*m[2][1] + w*m[3][1];
	res.z = x*m[0][2] + y*m[1][2] + z*m[2][2] + w*m[3][2];
	res.w = x*m[0][3] + y*m[1][3] + z*m[2][3] + w*m[3][3];
	return res;
}

template<typename T> T mtx4x4_norm(const cxMtx& m) {
	T t[4 * 4];
	T s[4];
	T w[4];
	T nrm = T(0);
	nxLA::mtx_cpy(t, &m.m[0][0], 4, 4);
	if (nxLA::sv_decomp(t, s, (T*)nullptr, w, t, 4, 4, false, false)) {
		nrm = nxCalc::max(s[0], s[1], s[2], s[3]);
	}
	return nrm;
}

XD_NOINLINE float cxMtx::norm(bool hprec) const {
	return hprec ? float(mtx4x4_norm<double>(*this)) : mtx4x4_norm<float>(*this);
}

template<typename T> T mtx4x4_det(const cxMtx& m) {
	T t[4 * 4];
	T wk[4];
	int idx[4];
	int dsgn = 0;
	T det = T(0);
	nxLA::mtx_cpy(t, &m.m[0][0], 4, 4);
	if (nxLA::lu_decomp(t, 4, wk, idx, &dsgn)) {
		det = nxLA::lu_det(t, 4, dsgn);
	}
	return det;
}

XD_NOINLINE float cxMtx::det(bool hprec) const {
	return hprec ? float(mtx4x4_det<double>(*this)) : mtx4x4_det<float>(*this);
}

template<typename T> T mtx4x4_det_sr(const cxMtx& m) {
	T t[3 * 3];
	T wk[3];
	int idx[3];
	T det = T(0);
	int dsgn = 0;
	int ti = 0;
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			t[ti] = m.m[i][j];
			++ti;
		}
	}
	if (nxLA::lu_decomp(t, 3, wk, idx, &dsgn)) {
		det = nxLA::lu_det(t, 3, dsgn);
	}
	return det;
}

XD_NOINLINE float cxMtx::det_sr(bool hprec) const {
	return hprec ? float(mtx4x4_det_sr<double>(*this)) : mtx4x4_det_sr<float>(*this);
}


void cxQuat::from_mtx(const cxMtx& mtx) {
	*this = mtx.to_quat();
}

cxMtx cxQuat::to_mtx() const {
	cxMtx m;
	m.from_quat(*this);
	return m;
}

float cxQuat::get_axis_ang(cxVec* pAxis) const {
	float ang = ::mth_acosf(w) * 2.0f;
	if (pAxis) {
		float s = nxCalc::rcp0(::mth_sqrtf(1.0f - w*w));
		pAxis->set(x, y, z);
		pAxis->scl(s);
		pAxis->normalize();
	}
	return ang;
}

cxVec cxQuat::get_log_vec() const {
	float cosh = w;
	float hang = ::mth_acosf(cosh);
	cxVec axis(x, y, z);
	axis.normalize();
	axis.scl(hang);
	return axis;
}

void cxQuat::from_log_vec(const cxVec& lvec, bool nrmFlg) {
	float hang = lvec.mag();
	float cosh = ::mth_cosf(hang);
	cxVec axis = lvec * nxCalc::sinc(hang);
	x = axis.x;
	y = axis.y;
	z = axis.z;
	w = cosh;
	if (nrmFlg) {
		normalize();
	}
}

void cxQuat::from_vecs(const cxVec& vfrom, const cxVec& vto) {
	cxVec axis = nxVec::cross(vfrom, vto);
	float sqm = axis.mag2();
	if (sqm == 0.0f) {
		identity();
	} else {
		float d = vfrom.dot(vto);
		if (d == 1.0f) {
			identity();
		} else {
			d = nxCalc::clamp(d, -1.0f, 1.0f);
			float c = ::mth_sqrtf((1.0f + d) * 0.5f);
			float s = ::mth_sqrtf((1.0f - d) * 0.5f);
			axis.scl(nxCalc::rcp0(::mth_sqrtf(sqm)) * s);
			x = axis.x;
			y = axis.y;
			z = axis.z;
			w = c;
			normalize();
		}
	}
}

cxMtx cxQuat::get_col_mtx() const {
	cxMtx m;
	float t[4 * 4] = {
		 w, -z,  y, x,
		 z,  w, -x, y,
		-y,  x,  w, z,
		-x, -y, -z, w
	};
	m.from_mem(t);
	return m;
}
cxMtx cxQuat::get_row_mtx() const {
	cxMtx m;
	float t[4 * 4] = {
		 w,  z, -y, -x,
		-z,  w,  x, -y,
		 y, -x,  w, -z,
		 x,  y,  z,  w
	};
	m.from_mem(t);
	return m;
}

void cxQuat::set_rot(const cxVec& axis, float ang) {
	float hang = ang * 0.5f;
	float s = ::mth_sinf(hang);
	float c = ::mth_cosf(hang);
	cxVec v = axis.get_normalized() * s;
	set(v.x, v.y, v.z, c);
}

void cxQuat::set_rot_x(float rx) {
	float h = rx * 0.5f;
	float s = ::mth_sinf(h);
	float c = ::mth_cosf(h);
	set(s, 0.0f, 0.0f, c);
}

void cxQuat::set_rot_y(float ry) {
	float h = ry * 0.5f;
	float s = ::mth_sinf(h);
	float c = ::mth_cosf(h);
	set(0.0f, s, 0.0f, c);
}

void cxQuat::set_rot_z(float rz) {
	float h = rz * 0.5f;
	float s = ::mth_sinf(h);
	float c = ::mth_cosf(h);
	set(0.0f, 0.0f, s, c);
}

void cxQuat::set_rot(float rx, float ry, float rz, exRotOrd ord) {
	static struct { uint8_t q2, q1, q0; } tbl[] = {
		/* XYZ */ {0, 1, 2},
		/* XZY */ {0, 2, 1},
		/* YXZ */ {1, 0, 2},
		/* YZX */ {1, 2, 0},
		/* ZXY */ {2, 0, 1},
		/* ZYX */ {2, 1, 0}
	};
	uint32_t idx = (uint32_t)ord;
	if (idx >= XD_ARY_LEN(tbl)) {
		identity();
	} else {
		cxQuat rq[3];
		rq[0].set_rot_x(rx);
		rq[1].set_rot_y(ry);
		rq[2].set_rot_z(rz);
		int q0 = tbl[idx].q0;
		int q1 = tbl[idx].q1;
		int q2 = tbl[idx].q2;
		*this = rq[q0];
		mul(rq[q1]);
		mul(rq[q2]);
	}
}

void cxQuat::set_rot_degrees(float dx, float dy, float dz, exRotOrd ord) {
	set_rot_degrees(cxVec(dx, dy, dz), ord);
}

void cxQuat::set_rot_degrees(const cxVec& r, exRotOrd ord) {
	cxVec rr = r * XD_DEG2RAD(1.0f);
	set_rot(rr.x, rr.y, rr.z, ord);
}

cxVec cxQuat::get_rot(exRotOrd ord) const {
	cxVec rv(0.0f);
	int axisMask = 0;
	const float eps = 1.0e-6f;
	if (::mth_fabsf(x) < eps) axisMask |= 1;
	if (::mth_fabsf(y) < eps) axisMask |= 2;
	if (::mth_fabsf(z) < eps) axisMask |= 4;
	if (::mth_fabsf(w) < eps) axisMask |= 8;
	bool singleAxis = false;
	float qw = nxCalc::clamp(w, -1.0f, 1.0f);
	switch (axisMask) {
		case 6: /* 0110 -> X */
			rv.x = ::mth_acosf(qw) * 2.0f;
			if (x < 0.0f) rv.x = -rv.x;
			rv.x = limit_pi(rv.x);
			singleAxis = true;
			break;
		case 5: /* 0101 -> Y */
			rv.y = ::mth_acosf(qw) * 2.0f;
			if (y < 0.0f) rv.y = -rv.y;
			rv.y = limit_pi(rv.y);
			singleAxis = true;
			break;
		case 3: /* 0011 -> Z */
			rv.z = ::mth_acosf(qw) * 2.0f;
			if (z < 0.0f) rv.z = -rv.z;
			rv.z = limit_pi(rv.z);
			singleAxis = true;
			break;
		case 7: /* 0111 -> identity */
			singleAxis = true;
			break;
	}
	if (singleAxis) {
		return rv;
	}
	if ((uint32_t)ord >= XD_ARY_LEN(s_getRotTbl)) {
		ord = exRotOrd::XYZ;
	}
	int i0 = s_getRotTbl[(int)ord].i0;
	int i1 = s_getRotTbl[(int)ord].i1;
	int i2 = s_getRotTbl[(int)ord].i2;
	float sgn = s_getRotTbl[(int)ord].s ? 1.0f : -1.0f;
	cxVec m[3];
	m[0] = get_axis_x();
	m[1] = get_axis_y();
	m[2] = get_axis_z();
	cxVec rm[3];
	rm[0].set(m[i0][i0], m[i0][i1], m[i0][i2]);
	rm[1].set(m[i1][i0], m[i1][i1], m[i1][i2]);
	rm[2].set(m[i2][i0], m[i2][i1], m[i2][i2]);
	float r[3] = { 0, 0, 0 };
	r[i0] = ::mth_atan2f(rm[1][2], rm[2][2]);
	r[i1] = ::mth_atan2f(-rm[0][2], ::mth_sqrtf(nxCalc::sq(rm[0][0]) + nxCalc::sq(rm[0][1])));
	float s = ::mth_sinf(r[i0]);
	float c = ::mth_cosf(r[i0]);
	r[i2] = ::mth_atan2f(s*rm[2][0] - c*rm[1][0], c*rm[1][1] - s*rm[2][1]);
	for (int i = 0; i < 3; ++i) {
		r[i] *= sgn;
	}
	for (int i = 0; i < 3; ++i) {
		r[i] = limit_pi(r[i]);
	}
	rv.from_mem(r);
	return rv;
}

void cxQuat::slerp(const cxQuat& q1, const cxQuat& q2, float t) {
	xt_float4 v1 = q1;
	xt_float4 v2 = q2;
	if (q1.dot(q2) < 0.0f) {
		v2.scl(-1.0f);
	}
	float u = 0.0f;
	float v = 0.0f;
	for (int i = 0; i < 4; ++i) {
		u += nxCalc::sq(v1[i] - v2[i]);
		v += nxCalc::sq(v1[i] + v2[i]);
	}
	float ang = 2.0f * ::mth_atan2f(::mth_sqrtf(u), ::mth_sqrtf(v));
	float s = 1.0f - t;
	float d = 1.0f / nxCalc::sinc(ang);
	s = nxCalc::sinc(ang*s) * d * s;
	t = nxCalc::sinc(ang*t) * d * t;
	for (int i = 0; i < 4; ++i) {
		(*this)[i] = v1[i]*s + v2[i]*t;
	}
	normalize();
}

cxVec cxQuat::apply(const cxVec& v) const {
	cxVec qv = cxVec(x, y, z);
	float d = qv.dot(v);
	float ww = w*w;
	return ((qv*d + v*ww) - nxVec::cross(v, qv)*w)*2.0f - v;
}

// http://www.geometrictools.com/Documentation/ConstrainedQuaternions.pdf

static inline cxQuat closest_quat_by_axis(const cxQuat& qsrc, int axis) {
	cxQuat qres;
	qres.identity();
	float e = qsrc.get_at(axis);
	float w = qsrc.w;
	float sqmag = nxCalc::sq(e) + nxCalc::sq(w);
	if (sqmag > 0.0f) {
		float s = nxCalc::rcp0(::mth_sqrtf(sqmag));
		qres.set_at(axis, e*s);
		qres.w = w*s;
	}
	return qres;
}

cxQuat cxQuat::get_closest_x() const { return closest_quat_by_axis(*this, 0); }

cxQuat cxQuat::get_closest_y() const { return closest_quat_by_axis(*this, 1); }

cxQuat cxQuat::get_closest_z() const { return closest_quat_by_axis(*this, 2); }

cxQuat cxQuat::get_closest_xy() const {
	cxQuat q;
	float det = ::mth_fabsf(-x*y - z*w);
	float s;
	if (det < 0.5f) {
		float d = ::mth_sqrtf(::mth_fabsf(1.0f - 4.0f*nxCalc::sq(det)));
		float a = x*w - y*z;
		float b = nxCalc::sq(w) - nxCalc::sq(x) + nxCalc::sq(y) - nxCalc::sq(z);
		float s0, c0;
		if (b >= 0.0f) {
			s0 = a;
			c0 = (d + b)*0.5f;
		} else {
			s0 = (d - b)*0.5f;
			c0 = a;
		}
		s = nxCalc::rcp0(nxCalc::hypot(s0, c0));
		s0 *= s;
		c0 *= s;

		float s1 = y*c0 - z*s0;
		float c1 = w*c0 + x*s0;
		s = nxCalc::rcp0(nxCalc::hypot(s1, c1));
		s1 *= s;
		c1 *= s;

		q.set(s0*c1, c0*s1, -s0*s1, c0*c1);
	} else {
		s = nxCalc::rcp0(::mth_sqrtf(det));
		q.set(x*s, 0.0f, 0.0f, w*s);
	}
	return q;
}

cxQuat cxQuat::get_closest_yx() const {
	cxQuat q = cxQuat(x, y, -z, w).get_closest_xy();
	q.z = -q.z;
	return q;
}

cxQuat cxQuat::get_closest_xz() const {
	cxQuat q = cxQuat(x, z, y, w).get_closest_yx();
	float t = q.y;
	q.y = q.z;
	q.z = t;
	return q;
}

cxQuat cxQuat::get_closest_zx() const {
	cxQuat q = cxQuat(x, z, y, w).get_closest_xy();
	float t = q.y;
	q.y = q.z;
	q.z = t;
	return q;
}

cxQuat cxQuat::get_closest_yz() const {
	cxQuat q = cxQuat(y, z, x, w).get_closest_xy();
	return cxQuat(q.z, q.x, q.y, q.w);
}

cxQuat cxQuat::get_closest_zy() const {
	cxQuat q = cxQuat(y, z, x, w).get_closest_yx();
	return cxQuat(q.z, q.x, q.y, q.w);
}


namespace nxQuat {

cxQuat log(const cxQuat& q) {
	float m = q.mag();
	float w = ::mth_logf(m);
	cxVec v = q.get_imag_part();
	float s = v.mag();
	s = s < 1.0e-5f ? 1.0f : ::mth_atan2f(s, q.get_real_part()) / s;
	v.scl(s);
	return cxQuat(v.x, v.y, v.z, w);
}

cxQuat exp(const cxQuat& q) {
	cxVec v = q.get_imag_part();
	float e = ::mth_expf(q.get_real_part());
	float h = v.mag();
	float c = ::mth_cosf(h);
	float s = ::mth_sinf(h);
	v.normalize();
	v.scl(s);
	return cxQuat(v.x, v.y, v.z, c).get_scaled(e);
}

cxQuat pow(const cxQuat& q, float x) {
	if (q.mag() < 1.0e-5f) return nxQuat::zero();
	return exp(log(q).get_scaled(x));
}

float arc_dist(const cxQuat& a, const cxQuat& b) {
	double ax = a.x;
	double ay = a.y;
	double az = a.z;
	double aw = a.w;
	double bx = b.x;
	double by = b.y;
	double bz = b.z;
	double bw = b.w;
	double d = ::mth_fabs(ax*bx + ay*by + az*bz + aw*bw);
	if (d > 1.0) d = 1.0;
	d = ::mth_acos(d) / (XD_PI / 2.0);
	return float(d);
}

} // nxQuat


XD_NOINLINE void sxQuatTrackball::init() {
	mSpin.identity();
	mQuat.identity();
}

static cxVec tball_proj(float x, float y, float r) {
	float d = nxCalc::hypot(x, y);
	float t = r / ::mth_sqrtf(2.0f);
	float z;
	if (d < t) {
		z = ::mth_sqrtf(r*r - d*d);
	} else {
		z = (t*t) / d;
	}
	return cxVec(x, y, z);
}

XD_NOINLINE void sxQuatTrackball::update(const float x0, const float y0, const float x1, const float y1, const float radius) {
	cxVec tp0 = tball_proj(x0, y0, radius);
	cxVec tp1 = tball_proj(x1, y1, radius);
	cxVec dir = tp0 - tp1;
	cxVec axis = nxVec::cross(tp1, tp0);
	float t = nxCalc::clamp(dir.mag() / (2.0f*radius), -1.0f, 1.0f);
	float ang = 2.0f * ::mth_asinf(t);
	mSpin.set_rot(axis, ang);
	mQuat.mul(mSpin);
	mQuat.normalize();
}

XD_NOINLINE cxVec sxQuatTrackball::calc_dir(const float dist) const {
	return mQuat.apply(nxVec::get_axis(exAxis::PLUS_Z) * dist);
}

XD_NOINLINE cxVec sxQuatTrackball::calc_up() const {
	return mQuat.apply(nxVec::get_axis(exAxis::PLUS_Y));
}


void cxDualQuat::mul(const cxDualQuat& dq1, const cxDualQuat& dq2) {
	cxQuat r0 = dq1.mReal;
	cxQuat d0 = dq1.mDual;
	cxQuat r1 = dq2.mReal;
	cxQuat d1 = dq2.mDual;
	mReal = r0 * r1;
	mDual = r0*d1 + d0*r1;
}

void cxDualQuat::mul(const cxDualQuat& dq) {
	mul(*this, dq);
}

void cxDualQuat::normalize(bool fast) {
	cxQuat r = mReal;
	float irm = nxCalc::rcp0(r.mag());
	if (fast) {
		mReal.scl(irm);
		mDual.scl(irm);
	} else {
		cxQuat d = mDual;
		cxQuat rn = r * irm;
		cxQuat dn = d * irm;
		float rd = rn.dot(dn);
		dn -= rn * rd;
		mReal = rn;
		mDual = dn;
	}
}

cxVec cxDualQuat::calc_pnt(const cxVec& pos) const {
	cxQuat t(pos.x, pos.y, pos.z, 0.0f);
	t.scl(0.5f);
	cxQuat d = mDual;
	d.add(mReal * t);
	d.scl(2.0f);
	d.mul(mReal.get_conjugate());
	return d.get_vector_part();
}

void cxDualQuat::interpolate(const cxDualQuat& dqA, const cxDualQuat& dqB, float t) {
	cxDualQuat b = dqB;
	float ab = dqA.mReal.dot(dqB.mReal);
	if (ab < 0.0f) {
		b.mReal.negate();
		b.mDual.negate();
		ab = -ab;
	}

	if (1.0f - ab < 1.0e-6f) {
		mReal.lerp(dqA.mReal, dqB.mReal, t);
		mDual.lerp(dqA.mDual, dqB.mDual, t);
		normalize();
		return;
	}

	cxDualQuat d;
	d.mReal = dqA.mReal.get_conjugate();
	d.mDual = dqA.mDual.get_conjugate();
	d.mul(b, d);
	d.normalize();

	cxVec vr = d.mReal.get_vector_part();
	cxVec vd = d.mDual.get_vector_part();
	float ir = nxCalc::rcp0(vr.mag());
	float ang = 2.0f * ::mth_acosf(d.mReal.w);
	float tns = -2.0f * d.mDual.w * ir;
	cxVec dir = vr * ir;
	cxVec mom = (vd - dir*tns*d.mReal.w*0.5f) * ir;

	ang *= t;
	tns *= t;
	float ha = ang * 0.5f;
	float sh = ::mth_sinf(ha);
	float ch = ::mth_cosf(ha);

	cxDualQuat r;
	r.mReal.from_parts(dir * sh, ch);
	r.mDual.from_parts(mom*sh + dir*ch*tns*0.5f, -tns*0.5f*sh);

	mul(r, dqA);
	normalize();
}


namespace nxGeom {

bool seg_seg_overlap_2d(const float s0x0, const float s0y0, const float s0x1, const float s0y1, const float s1x0, const float s1y0, const float s1x1, const float s1y1) {
	bool res = false;
	float a1 = signed_tri_area_2d(s0x0, s0y0, s0x1, s0y1, s1x1, s1y1);
	float a2 = signed_tri_area_2d(s0x0, s0y0, s0x1, s0y1, s1x0, s1y0);
	if (a1*a2 < 0.0f) {
		float a3 = signed_tri_area_2d(s1x0, s1y0, s1x1, s1y1, s0x0, s0y0);
		float a4 = a3 + a2 - a1;
		if (a3*a4 < 0.0f) {
			res = true;
		}
	}
	return res;
}

bool seg_plane_intersect(const cxVec& p0, const cxVec& p1, const cxPlane& pln, float* pT, cxVec* pHitPos) {
	cxVec d = p1 - p0;
	cxVec n = pln.get_normal();
	float dn = d.dot(n);
	float t = (pln.get_D() - n.dot(p0)) / dn;
	bool res = t >= 0.0f && t <= 1.0f;
	if (res) {
		if (pT) {
			*pT = t;
		}
		if (pHitPos) {
			*pHitPos = p0 + d*t;
		}
	}
	return res;
}

bool seg_sph_intersect(const cxVec& p0, const cxVec& p1, const cxSphere& sph, float* pT, cxVec* pHitPos) {
	return seg_sph_intersect(p0, p1, sph.get_center(), sph.get_radius(), pT, pHitPos);
}

bool seg_sph_intersect(const cxVec& p0, const cxVec& p1, const cxVec& c, const float r, float* pT, cxVec* pHitPos) {
	cxVec d = p1 - p0;
	float len = d.length();
	cxVec hitPos = p0;
	float t = -1.0f;
	bool res = false;
	if (len > 0.0f) {
		cxVec v = p0 - c;
		float b = v.dot(d * (1.0f / len));
		float c = v.mag2() - r*r;
		res = !(b > 0.0f && c > 0.0f);
		if (res) {
			float ds = b*b - c;
			res = ds >= 0.0f;
			if (res) {
				t = (-b - ::mth_sqrtf(ds)) / len;
				if (t < 0.0f) {
					t = 0.0f;
					hitPos = p0;
				} else {
					res = t <= 1.0f;
					if (res) {
						hitPos = p0 + d*t;
					}
				}
			}
		}
	} else {
		res = pnt_in_sph(p0, c, r);
	}
	if (res) {
		if (pT) {
			*pT = t;
		}
		if (pHitPos) {
			*pHitPos = hitPos;
		}
	}
	return res;
}

bool seg_quad_intersect_cw(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& v3, cxVec* pHitPos, cxVec* pHitNrm) {
	return seg_quad_intersect_ccw(p0, p1, v3, v2, v1, v0, pHitPos, pHitNrm);
}

bool seg_quad_intersect_cw_n(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& v3, const cxVec& nrm, cxVec* pHitPos) {
	return seg_quad_intersect_ccw_n(p0, p1, v3, v2, v1, v0, nrm, pHitPos);
}

#ifndef XD_SEG_QUAD_VEC
#	define XD_SEG_QUAD_VEC 0
#endif

bool seg_quad_intersect_ccw(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& v3, cxVec* pHitPos, cxVec* pHitNrm) {
	cxVec vec[4];
	cxVec edge[4];
	edge[0] = v1 - v0;
	cxVec n = nxVec::cross(edge[0], v2 - v0).get_normalized();
	vec[0] = p0 - v0;
	float d0 = vec[0].dot(n);
	float d1 = (p1 - v0).dot(n);
	if (d0*d1 > 0.0f || (d0 == 0.0f && d1 == 0.0f)) return false;
	edge[1] = v2 - v1;
	edge[2] = v3 - v2;
	edge[3] = v0 - v3;
	vec[1] = p0 - v1;
	vec[2] = p0 - v2;
	vec[3] = p0 - v3;
	cxVec dir = p1 - p0;
#if XD_SEG_QUAD_VEC
	float tp[4];
	for (int i = 0; i < 4; ++i) {
		tp[i] = nxVec::scalar_triple(edge[i], dir, vec[i]);
	}
	for (int i = 0; i < 4; ++i) {
		if (tp[i] < 0.0f) return false;
	}
#else
	if (nxVec::scalar_triple(edge[0], dir, vec[0]) < 0.0f) return false;
	if (nxVec::scalar_triple(edge[1], dir, vec[1]) < 0.0f) return false;
	if (nxVec::scalar_triple(edge[2], dir, vec[2]) < 0.0f) return false;
	if (nxVec::scalar_triple(edge[3], dir, vec[3]) < 0.0f) return false;
#endif
	float d = dir.dot(n);
	float t;
	if (d == 0.0f || d0 == 0.0f) {
		t = 0.0f;
	} else {
		t = -d0 / d;
	}
	if (t > 1.0f || t < 0.0f) return false;
	if (pHitPos) {
		*pHitPos = p0 + dir*t;
	}
	if (pHitNrm) {
		*pHitNrm = n;
	}
	return true;
}

bool seg_quad_intersect_ccw_n(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& v3, const cxVec& nrm, cxVec* pHitPos) {
	cxVec vec[4];
	cxVec edge[4];
	vec[0] = p0 - v0;
	float d0 = vec[0].dot(nrm);
	float d1 = (p1 - v0).dot(nrm);
	if (d0*d1 > 0.0f || (d0 == 0.0f && d1 == 0.0f)) return false;
	edge[0] = v1 - v0;
	edge[1] = v2 - v1;
	edge[2] = v3 - v2;
	edge[3] = v0 - v3;
	vec[1] = p0 - v1;
	vec[2] = p0 - v2;
	vec[3] = p0 - v3;
	cxVec dir = p1 - p0;
	if (nxVec::scalar_triple(edge[0], dir, vec[0]) < 0.0f) return false;
	if (nxVec::scalar_triple(edge[1], dir, vec[1]) < 0.0f) return false;
	if (nxVec::scalar_triple(edge[2], dir, vec[2]) < 0.0f) return false;
	if (nxVec::scalar_triple(edge[3], dir, vec[3]) < 0.0f) return false;
	float d = dir.dot(nrm);
	float t;
	if (d == 0.0f || d0 == 0.0f) {
		t = 0.0f;
	} else {
		t = -d0 / d;
	}
	if (t > 1.0f || t < 0.0f) return false;
	if (pHitPos) {
		*pHitPos = p0 + dir*t;
	}
	return true;
}

bool seg_polyhedron_intersect(const cxVec& p0, const cxVec& p1, const cxPlane* pPln, int plnNum, float* pFirst, float* pLast) {
	float first = 0.0f;
	float last = 1.0f;
	bool res = false;
	cxVec d = p1 - p0;
	for (int i = 0; i < plnNum; ++i) {
		cxVec nrm = pPln[i].get_normal();
		float dnm = d.dot(nrm);
		float dist = pPln[i].get_D() - nrm.dot(p0);
		if (dnm == 0.0f) {
			if (dist < 0.0f) goto _exit;
		} else {
			float t = dist / dnm;
			if (dnm < 0.0f) {
				if (t > first) first = t;
			} else {
				if (t < last) last = t;
			}
			if (first > last) goto _exit;
		}
	}
	res = true;
_exit:
	if (pFirst) {
		*pFirst = first;
	}
	if (pLast) {
		*pLast = last;
	}
	return res;
}

bool seg_tri_intersect_cw_n(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& nrm, cxVec* pHitPos) {
	return seg_quad_intersect_cw_n(p0, p1, v0, v1, v2, v0, nrm, pHitPos);
}

bool seg_tri_intersect_cw(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, cxVec* pHitPos, cxVec* pHitNrm) {
	return seg_quad_intersect_cw(p0, p1, v0, v1, v2, v0, pHitPos, pHitNrm);
}


bool seg_tri_intersect_ccw_n(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& nrm, cxVec* pHitPos) {
	return seg_quad_intersect_ccw_n(p0, p1, v0, v1, v2, v0, nrm, pHitPos);
}

bool seg_tri_intersect_ccw(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2, cxVec* pHitPos, cxVec* pHitNrm) {
	return seg_quad_intersect_ccw(p0, p1, v0, v1, v2, v0, pHitPos, pHitNrm);
}

xt_float4 seg_tri_intersect_bc_cw(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2) {
	xt_float4 uvwt;
	uvwt.fill(-1.0f);
	cxVec e1 = v1 - v0;
	cxVec e2 = v2 - v0;
	cxVec sv = p0 - p1;
	cxVec n = nxVec::cross(e2, e1);
	float d = sv.dot(n);
	if (d > 0.0f) {
		cxVec sv0 = p0 - v0;
		float t = sv0.dot(n);
		if (t >= 0.0f && t <= d) {
			cxVec e = nxVec::cross(sv, sv0);
			float w = e.dot(e1);
			if (w >= 0.0f && w <= d) {
				float v = -e.dot(e2);
				if (v >= 0.0f && v + w <= d) {
					float s = 1.0f / d;
					uvwt.set(1.0f, v, w, t);
					uvwt.scl(s);
					uvwt.x = 1.0f - uvwt.y - uvwt.z;
				}
			}
		}
	}
	return uvwt;
}

xt_float4 seg_tri_intersect_bc_ccw(const cxVec& p0, const cxVec& p1, const cxVec& v0, const cxVec& v1, const cxVec& v2) {
	xt_float4 uvwt;
	uvwt.fill(-1.0f);
	cxVec e1 = v1 - v0;
	cxVec e2 = v2 - v0;
	cxVec sv = p0 - p1;
	cxVec n = nxVec::cross(e1, e2);
	float d = sv.dot(n);
	if (d > 0.0f) {
		cxVec sv0 = p0 - v0;
		float t = sv0.dot(n);
		if (t >= 0.0f && t <= d) {
			cxVec e = nxVec::cross(sv, sv0);
			float v = e.dot(e2);
			if (v >= 0.0f && v <= d) {
				float w = -e.dot(e1);
				if (w >= 0.0f && v + w <= d) {
					float s = 1.0f / d;
					uvwt.set(1.0f, v, w, t);
					uvwt.scl(s);
					uvwt.x = 1.0f - uvwt.y - uvwt.z;
				}
			}
		}
	}
	return uvwt;
}

cxVec barycentric(const cxVec& pos, const cxVec& v0, const cxVec& v1, const cxVec& v2) {
	cxVec dv0 = pos - v0;
	cxVec dv1 = v1 - v0;
	cxVec dv2 = v2 - v0;
	float d01 = dv0.dot(dv1);
	float d02 = dv0.dot(dv2);
	float d12 = dv1.dot(dv2);
	float d11 = dv1.dot(dv1);
	float d22 = dv2.dot(dv2);
	float d = d11*d22 - nxCalc::sq(d12);
	float ood = 1.0f / d;
	float v = (d01*d22 - d02*d12) * ood;
	float w = (d02*d11 - d01*d12) * ood;
	float u = (1.0f - v - w);
	return cxVec(u, v, w);
}

cxVec barycentric_uv(const cxVec& pos, const cxVec& v0, const cxVec& v1, const cxVec& v2) {
	cxVec dv0 = pos - v0;
	cxVec dv1 = v1 - v0;
	cxVec dv2 = v2 - v0;
	float d01 = dv0.dot(dv1);
	float d02 = dv0.dot(dv2);
	float d12 = dv1.dot(dv2);
	float d11 = dv1.dot(dv1);
	float d22 = dv2.dot(dv2);
	float d = d11*d22 - nxCalc::sq(d12);
	float ood = 1.0f / d;
	float u = (d01*d22 - d02*d12) * ood;
	float v = (d02*d11 - d01*d12) * ood;
	float w = (1.0f - u - v);
	return cxVec(u, v, w);
}

float quad_dist2(const cxVec& pos, const cxVec vtx[4]) {
	int i;
	cxVec v[4];
	cxVec edge[4];
	for (i = 0; i < 4; ++i) {
		v[i] = pos - vtx[i];
	}
	for (i = 0; i < 4; ++i) {
		edge[i] = vtx[i < 3 ? i + 1 : 0] - vtx[i];
	}
	cxVec n = nxVec::cross(edge[0], vtx[2] - vtx[0]).get_normalized();
	float d[4];
	for (i = 0; i < 4; ++i) {
		d[i] = nxVec::scalar_triple(edge[i], v[i], n);
	}
	int mask = 0;
	for (i = 0; i < 4; ++i) {
		if (d[i] >= 0) mask |= 1 << i;
	}
	if (mask == 0xF) {
		return nxCalc::sq(v[0].dot(n));
	}
	if (::mth_fabsf(d[3]) < 1e-6f) {
		edge[3] = edge[2];
	}
	for (i = 0; i < 4; ++i) {
		d[i] = edge[i].mag2();
	}
	float len[4];
	for (i = 0; i < 4; ++i) {
		len[i] = ::mth_sqrtf(d[i]);
	}
	for (i = 0; i < 4; ++i) {
		edge[i].scl(nxCalc::rcp0(len[i]));
	}
	for (i = 0; i < 4; ++i) {
		d[i] = v[i].dot(edge[i]);
	}
	for (i = 0; i < 4; ++i) {
		d[i] = nxCalc::clamp(d[i], 0.0f, len[i]);
	}
	for (i = 0; i < 4; ++i) {
		edge[i].scl(d[i]);
	}
	for (i = 0; i < 4; ++i) {
		v[i] = vtx[i] + edge[i];
	}
	for (i = 0; i < 4; ++i) {
		d[i] = nxVec::dist2(pos, v[i]);
	}
	return nxCalc::min(d[0], d[1], d[2], d[3]);
}

int quad_convex_ck(const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& v3) {
	int res = 0;
	cxVec e0 = v1 - v0;
	cxVec e1 = v2 - v1;
	cxVec e2 = v3 - v2;
	cxVec e3 = v0 - v3;
	cxVec n01 = nxVec::cross(e0, e1);
	cxVec n23 = nxVec::cross(e2, e3);
	if (n01.dot(n23) > 0.0f) res |= 1;
	cxVec n12 = nxVec::cross(e1, e2);
	cxVec n30 = nxVec::cross(e3, e0);
	if (n12.dot(n30) > 0.0f) res |= 2;
	return res;
}

void update_nrm_newell(cxVec* pNrm, cxVec* pVtxI, cxVec* pVtxJ) {
	cxVec dv = *pVtxI - *pVtxJ;
	cxVec sv = *pVtxI + *pVtxJ;
	*pNrm += cxVec(dv[1], dv[2], dv[0]) * cxVec(sv[2], sv[0], sv[1]);
}

cxVec poly_normal_cw(cxVec* pVtx, const int vtxNum) {
	cxVec nrm;
	nrm.zero();
	for (int i = 0; i < vtxNum; ++i) {
		int j = i - 1;
		if (j < 0) j = vtxNum - 1;
		update_nrm_newell(&nrm, &pVtx[i], &pVtx[j]);
	}
	nrm.normalize();
	return nrm;
}

cxVec poly_normal_ccw(cxVec* pVtx, const int vtxNum) {
	cxVec nrm;
	nrm.zero();
	for (int i = 0; i < vtxNum; ++i) {
		int j = i + 1;
		if (j >= vtxNum) j = 0;
		update_nrm_newell(&nrm, &pVtx[i], &pVtx[j]);
	}
	nrm.normalize();
	return nrm;
}

cxVec tri_pnt_closest(const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& pnt) {
	cxVec cpnt(FLT_MAX);
	cxVec e1 = v1 - v0;
	cxVec e2 = v2 - v0;
	cxVec v = pnt - v0;
	float d1 = v.dot(e1);
	float d2 = v.dot(e2);
	if (d1 <= 0.0f && d2 <= 0.0f) {
		cpnt = v0;
	} else {
		v = pnt - v1;
		float d3 = v.dot(e1);
		float d4 = v.dot(e2);
		if (d3 >= 0.0f && d3 >= d4) {
			cpnt = v1;
		} else {
			float t2 = d1*d4 - d3*d2;
			if (t2 <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
				cpnt = v0 + e1*nxCalc::div0(d1, d1 - d3);
			} else {
				v = pnt - v2;
				float d5 = v.dot(e1);
				float d6 = v.dot(e2);
				if (d6 >= 0.0f && d6 >= d5) {
					cpnt = v2;
				} else {
					float t1 = d5*d2 - d1*d6;
					if (t1 <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
						cpnt = v0 + e2*nxCalc::div0(d2, d2 - d6);
					} else {
						float t0 = d3*d6 - d5*d4;
						float d43 = d4 - d3;
						float d56 = d5 - d6;
						if (t0 <= 0.0f && d43 >= 0.0f && d56 >= 0.0f) {
							cpnt = v1 + (v2 - v1)*nxCalc::div0(d43, d43 + d56);
						} else {
							float s = nxCalc::rcp0(t0 + t1 + t2);
							cpnt = v0 + e1*t1*s + e2*t2*s;
						}
					}
				}
			}
		}
	}
	return cpnt;
}

float seg_seg_dist2(const cxVec& s0p0, const cxVec& s0p1, const cxVec& s1p0, const cxVec& s1p1, cxVec* pBridgeP0, cxVec* pBridgeP1) {
	static float eps = 1e-6f;
	float t0 = 0.0f;
	float t1 = 0.0f;
	cxVec dir0 = s0p1 - s0p0;
	cxVec dir1 = s1p1 - s1p0;
	cxVec vec = s0p0 - s1p0;
	float len0 = dir0.mag2();
	float len1 = dir1.mag2();
	float vd1 = vec.dot(dir1);
	if (len0 <= eps) {
		if (len1 > eps) {
			t1 = nxCalc::saturate(vd1 / len1);
		}
	} else {
		float vd0 = vec.dot(dir0);
		if (len1 <= eps) {
			t0 = nxCalc::saturate(-vd0 / len0);
		} else {
			float dd = dir0.dot(dir1);
			float dn = len0*len1 - nxCalc::sq(dd);
			if (dn != 0.0f) {
				t0 = nxCalc::saturate((dd*vd1 - vd0*len1) / dn);
			}
			t1 = (dd*t0 + vd1) / len1;
			if (t1 < 0.0f) {
				t0 = nxCalc::saturate(-vd0 / len0);
				t1 = 0.0f;
			} else if (t1 > 1.0f) {
				t0 = nxCalc::saturate((dd - vd0) / len0);
				t1 = 1.0f;
			}
		}
	}
	cxVec bp0 = s0p0 + dir0*t0;
	cxVec bp1 = s1p0 + dir1*t1;
	if (pBridgeP0) {
		*pBridgeP0 = bp0;
	}
	if (pBridgeP1) {
		*pBridgeP1 = bp1;
	}
	return (bp1 - bp0).mag2();
}

bool cap_aabb_overlap(const cxVec& cp0, const cxVec& cp1, const float cr, const cxVec& bmin, const cxVec& bmax) {
	cxVec brad = (bmax - bmin) * 0.5f;
	cxVec bcen = (bmin + bmax) * 0.5f;
	cxVec ccen = (cp0 + cp1) * 0.5f;
	cxVec h = (cp1 - cp0) * 0.5f;
	cxVec v = ccen - bcen;
	for (int i = 0; i < 3; ++i) {
		float d0 = ::mth_fabsf(v[i]);
		float d1 = ::mth_fabsf(h[i]) + brad[i] + cr;
		if (d0 > d1) return false;
	}
	cxVec tpos;
	line_pnt_closest(cp0, cp1, bcen, &tpos);
	cxVec a = (tpos - bcen).get_normalized();
	if (::mth_fabsf(v.dot(a)) > ::mth_fabsf(brad.dot(a)) + cr) return false;
	return true;
}

bool tri_aabb_overlap(const cxVec& v0, const cxVec& v1, const cxVec& v2, const cxVec& bmin, const cxVec& bmax) {
	cxVec vtx[3];
	vtx[0] = v0;
	vtx[1] = v1;
	vtx[2] = v2;
	return tri_aabb_overlap(vtx, bmin, bmax);
}

bool tri_aabb_overlap(const cxVec vtx[3], const cxVec& bmin, const cxVec& bmax) {
	cxVec c = (bmin + bmax) * 0.5f;
	cxVec e = (bmax - bmin) * 0.5f;

	cxVec v[3];
	for (int i = 0; i < 3; ++i) {
		v[i] = vtx[i] - c;
	}

	cxVec f[3];
	for (int i = 0; i < 3; ++i) {
		int i1 = (i + 1) % 3;
		f[i] = v[i1] - v[i];
	}

	cxVec u[3];
	nxLA::identity((float*)&u[0], 3);
	for (int i = 0; i < 9; ++i) {
		int aidx = i / 3;
		int fidx = (i % 3) % 3;
		cxVec a = nxVec::cross(u[aidx], f[fidx]);
		float r = 0.0f;
		for (int j = 0; j < 3; ++j) {
			r += ::mth_fabsf(a.dot(u[j])) * e[j];
		}
		cxVec p;
		for (int j = 0; j < 3; ++j) {
			p.set_at(j, v[j].dot(a));
		}
		float pmin = p.min_elem();
		float pmax = p.max_elem();
		if (pmin > r || pmax < -r) return false;
	}

	for (int i = 0; i < 3; ++i) {
		cxVec b;
		for (int j = 0; j < 3; ++j) {
			b.set_at(j, v[j][i]);
		}
		float bmin = b.min_elem();
		float bmax = b.max_elem();
		float r = e[i];
		if (bmin > r || bmax < -r) return false;
	}

	cxVec pn = nxVec::cross(f[1], f[0]);
	float pd = pn.dot(vtx[0]);
	float s = pn.dot(c) - pd;
	pn.abs();
	float r = e.dot(pn);
	return ::mth_fabsf(s) <= r;
}

bool obb_obb_overlap(const cxMtx& m1, const cxMtx& m2) {
	cxVec vx1 = m1.get_row_vec(0);
	cxVec vy1 = m1.get_row_vec(1);
	cxVec vz1 = m1.get_row_vec(2);
	cxVec t1 = m1.get_translation();
	cxVec r1(vx1.mag(), vy1.mag(), vz1.mag());
	r1.scl(0.5f);

	cxVec vx2 = m2.get_row_vec(0);
	cxVec vy2 = m2.get_row_vec(1);
	cxVec vz2 = m2.get_row_vec(2);
	cxVec t2 = m2.get_translation();
	cxVec r2(vx2.mag(), vy2.mag(), vz2.mag());
	r2.scl(0.5f);

	cxVec v = t2 - t1;
	if (v.mag2() <= nxCalc::sq(r1.min_elem() + r2.min_elem())) {
		return true;
	}

	vx1.normalize();
	vy1.normalize();
	vz1.normalize();
	vx2.normalize();
	vy2.normalize();
	vz2.normalize();

	cxVec dv[3];
	dv[0].set(vx1.dot(vx2), vx1.dot(vy2), vx1.dot(vz2));
	dv[0].abs();
	dv[1].set(vy1.dot(vx2), vy1.dot(vy2), vy1.dot(vz2));
	dv[1].abs();
	dv[2].set(vz1.dot(vx2), vz1.dot(vy2), vz1.dot(vz2));
	dv[2].abs();

	cxVec tv;
	cxVec cv;

	tv.set(v.dot(vx1), v.dot(vy1), v.dot(vz1));
	tv.abs();
	cv.set(r2.dot(dv[0]), r2.dot(dv[1]), r2.dot(dv[2]));
	cv.add(r1);
	if (tv.x > cv.x) return false;
	if (tv.y > cv.y) return false;
	if (tv.z > cv.z) return false;

	float t;
	t = dv[0].y; dv[0].y = dv[1].x; dv[1].x = t;
	t = dv[0].z; dv[0].z = dv[2].x; dv[2].x = t;
	t = dv[1].z; dv[1].z = dv[2].y; dv[2].y = t;

	tv.set(v.dot(vx2), v.dot(vy2), v.dot(vz2));
	tv.abs();
	cv.set(r1.dot(dv[0]), r1.dot(dv[1]), r1.dot(dv[2]));
	cv.add(r2);
	if (tv.x > cv.x) return false;
	if (tv.y > cv.y) return false;
	if (tv.z > cv.z) return false;

	dv[0].cross(vx1, vx2);
	dv[1].cross(vx1, vy2);
	dv[2].cross(vx1, vz2);
	tv.set(dv[0].dot(vx1), dv[0].dot(vy1), dv[0].dot(vz1));
	tv.abs();
	float x = r1.dot(tv);
	tv.set(dv[0].dot(vx2), dv[0].dot(vy2), dv[0].dot(vz2));
	tv.abs();
	x += r2.dot(tv);
	tv.set(dv[1].dot(vx1), dv[1].dot(vy1), dv[1].dot(vz1));
	tv.abs();
	float y = r1.dot(tv);
	tv.set(dv[1].dot(vx2), dv[1].dot(vy2), dv[1].dot(vz2));
	tv.abs();
	y += r2.dot(tv);
	tv.set(dv[2].dot(vx1), dv[2].dot(vy1), dv[2].dot(vz1));
	tv.abs();
	float z = r1.dot(tv);
	tv.set(dv[2].dot(vx2), dv[2].dot(vy2), dv[2].dot(vz2));
	tv.abs();
	z += r2.dot(tv);
	cv.set(x, y, z);
	tv.set(v.dot(dv[0]), v.dot(dv[1]), v.dot(dv[2]));
	tv.abs();
	if (tv.x > cv.x) return false;
	if (tv.y > cv.y) return false;
	if (tv.z > cv.z) return false;

	dv[0].cross(vy1, vx2);
	dv[1].cross(vy1, vy2);
	dv[2].cross(vy1, vz2);
	tv.set(dv[0].dot(vx1), dv[0].dot(vy1), dv[0].dot(vz1));
	tv.abs();
	x = r1.dot(tv);
	tv.set(dv[0].dot(vx2), dv[0].dot(vy2), dv[0].dot(vz2));
	tv.abs();
	x += r2.dot(tv);
	tv.set(dv[1].dot(vx1), dv[1].dot(vy1), dv[1].dot(vz1));
	tv.abs();
	y = r1.dot(tv);
	tv.set(dv[1].dot(vx2), dv[1].dot(vy2), dv[1].dot(vz2));
	tv.abs();
	y += r2.dot(tv);
	tv.set(dv[2].dot(vx1), dv[2].dot(vy1), dv[2].dot(vz1));
	tv.abs();
	z = r1.dot(tv);
	tv.set(dv[2].dot(vx2), dv[2].dot(vy2), dv[2].dot(vz2));
	tv.abs();
	z += r2.dot(tv);
	cv.set(x, y, z);
	tv.set(v.dot(dv[0]), v.dot(dv[1]), v.dot(dv[2]));
	tv.abs();
	if (tv.x > cv.x) return false;
	if (tv.y > cv.y) return false;
	if (tv.z > cv.z) return false;

	dv[0].cross(vz1, vx2);
	dv[1].cross(vz1, vy2);
	dv[2].cross(vz1, vz2);
	tv.set(dv[0].dot(vx1), dv[0].dot(vy1), dv[0].dot(vz1));
	tv.abs();
	x = r1.dot(tv);
	tv.set(dv[0].dot(vx2), dv[0].dot(vy2), dv[0].dot(vz2));
	tv.abs();
	x += r2.dot(tv);
	tv.set(dv[1].dot(vx1), dv[1].dot(vy1), dv[1].dot(vz1));
	tv.abs();
	y = r1.dot(tv);
	tv.set(dv[1].dot(vx2), dv[1].dot(vy2), dv[1].dot(vz2));
	tv.abs();
	y += r2.dot(tv);
	tv.set(dv[2].dot(vx1), dv[2].dot(vy1), dv[2].dot(vz1));
	tv.abs();
	z = r1.dot(tv);
	tv.set(dv[2].dot(vx2), dv[2].dot(vy2), dv[2].dot(vz2));
	tv.abs();
	z += r2.dot(tv);
	cv.set(x, y, z);
	tv.set(v.dot(dv[0]), v.dot(dv[1]), v.dot(dv[2]));
	tv.abs();
	if (tv.x > cv.x) return false;
	if (tv.y > cv.y) return false;
	if (tv.z > cv.z) return false;

	return true;
}

float sph_region_weight(const cxVec& pos, const cxVec& center, const float attnStart, const float attnEnd) {
	float wght = 0.0f;
	float dist = nxVec::dist(pos, center);
	if (dist <= attnEnd) {
		if (dist <= attnStart) {
			wght = 1.0f;
		} else {
			float falloff = nxCalc::rcp0(attnEnd - attnStart);
			wght = nxCalc::saturate(1.0f - falloff);
		}
	}
	return wght;
}

float cap_region_weight(const cxVec& pos, const cxVec& capPos0, const cxVec& capPos1, const float attnStart, const float attnEnd) {
	float wght = 0.0f;
	float dist = nxVec::dist(pos, seg_pnt_closest(capPos0, capPos1, pos));
	if (dist <= attnEnd) {
		if (dist <= attnStart) {
			wght = 1.0f;
		} else {
			float falloff = nxCalc::rcp0(attnEnd - attnStart);
			wght = nxCalc::saturate(1.0f - falloff);
		}
	}
	return wght;
}

float obb_region_weight(const cxVec& pos, const cxMtx& invMtx, const cxVec& attnStart, const cxVec& attnEnd) {
	cxVec loc = invMtx.calc_pnt(pos);
	loc.abs();
	loc.min(cxVec(1.0f));
	cxVec d = loc - attnStart;
	d.saturate();
	cxVec falloff = nxVec::rcp0(attnEnd - attnStart);
	cxVec attn = cxVec(1.0f) - (d * falloff);
	attn.saturate();
	float wght = attn.x * attn.y * attn.z;
	return wght;
}

XD_NOINLINE static void cvx_inv(cxMtx* pMtx, const int mode) {
	switch (mode) {
		case 0:
		default:
			pMtx->invert();
			break;
		case 1:
			pMtx->invert_fast();
			break;
		case 2:
			pMtx->invert_lu();
			break;
		case 3:
			pMtx->invert_lu_hi();
			break;
	}
}

float sph_convex_dist(
	const cxMtx& xformSph, const float radius,
	const cxMtx& xformCvx, const cxVec* pPts, const int npts,
	const uint16_t* pTriPids, const cxVec* pTriNrms, const int ntri,
	cxVec* pSphPnt, cxVec* pCvxPnt, cxVec* pSepVec,
	const int xformMode, const bool cw
) {
	cxVec vtx[3];
	cxVec sepVec;
	float dist = FLT_MAX;

	cxMtx invCvx = xformCvx;
	cvx_inv(&invCvx, xformMode);

	cxMtx xformStoC = xformSph * invCvx;
	cxVec ckPos = xformStoC.get_translation();

	cxMtx xformCtoS = xformStoC;
	cvx_inv(&xformCtoS, xformMode);

	cxVec minPos(0.0f);
	cxVec minNrm(0.0f);
	float minSqDist = -1.0f;
	for (int i = 0; i < ntri; ++i) {
		int triBase = i * 3;
		if (cw) {
			for (int j = 3; --j >= 0;) {
				vtx[2 - j] = pPts[pTriPids[triBase + j]];
			}
		} else {
			for (int j = 0; j < 3; ++j) {
				vtx[j] = pPts[pTriPids[triBase + j]];
			}
		}
		cxVec nrm = pTriNrms[i];
		cxVec v0 = ckPos - vtx[0];
		float d = nrm.dot(v0);
		if (d > 0.0f) {
			cxVec closest(0.0f);
#if 0
			float tc = -1.0f;
			cxVec rel = ckPos - nrm*d;
			cxVec t0 = rel - vtx[0];
			cxVec t1 = rel - vtx[1];
			cxVec e0 = vtx[1] - vtx[0];
			cxVec tn = nxVec::cross(e0, nrm);
			cxVec ck0(t0.dot(tn), t0.dot(e0), -t1.dot(e0));
			if (ck0.all_gt0()) {
				tc = dir_line_pnt_closest(vtx[0], e0, rel, &closest);
			} else {
				cxVec t2 = rel - vtx[2];
				cxVec e1 = vtx[2] - vtx[1];
				tn = nxVec::cross(e1, nrm);
				cxVec ck1(t1.dot(tn), t1.dot(e1), -t2.dot(e1));
				if (ck1.all_gt0()) {
					tc = dir_line_pnt_closest(vtx[1], e1, rel, &closest);
				} else {
					cxVec e2 = vtx[0] - vtx[2];
					tn = nxVec::cross(e2, nrm);
					cxVec ck2(t2.dot(tn), t2.dot(e2), -t0.dot(e2));
					if (ck2.all_gt0()) {
						tc = dir_line_pnt_closest(vtx[2], e2, rel, &closest);
					} else {
						if (ck0.x <= 0.0f && ck1.x <= 0.0f && ck2.x <= 0.0f) {
							closest = rel;
							tc = 0.0f;
						} else {
							if (ck0.y <= 0.0f && ck2.z <= 0.0f) {
								closest = vtx[0];
							} else if (ck0.z <= 0.0f && ck1.y <= 0.0f) {
								closest = vtx[1];
							} else {
								closest = vtx[2];
							}
						}
					}
				}
			}
#else
			closest = tri_pnt_closest(vtx[0], vtx[1], vtx[2], ckPos);
#endif
			float sqDist = nxVec::dist2(ckPos, closest);
			if (minSqDist < 0.0f || sqDist < minSqDist) {
				minSqDist = sqDist;
				minPos = closest;
				minNrm = nrm;
			}
		}
	}
	if (minSqDist >= 0.0f) {
		if (minSqDist > 1e-5f) {
			dist = ::mth_sqrtf(minSqDist) - radius;
			sepVec = minPos - ckPos;
			sepVec.normalize();
		} else {
			dist = -radius;
			sepVec = minNrm.neg_val();
		}
		if (pCvxPnt) {
			*pCvxPnt = minPos;
		}
		if (pSphPnt) {
			*pSphPnt = xformCtoS.calc_pnt(ckPos + sepVec*radius);
		}
		if (pSepVec) {
			*pSepVec = xformCvx.calc_vec(sepVec);
		}
	}

	return dist;
}


struct sxAABBTreeBuildWk {
	const cxAABB* pSrcBoxes;
	cxAABB* pNodeBoxes;
	int32_t* pNodeInfos;
	int32_t* pIdxWk;
	int32_t nsrc;
	int nodeCnt;
};

static void build_aabb_tree_leaf(sxAABBTreeBuildWk* pWk, int inode, int idx) {
	int32_t* pInfo = &pWk->pNodeInfos[inode * 2];
	int32_t isrc = pWk->pIdxWk[idx];
	pInfo[0] = isrc;
	pInfo[1] = -1;
	pWk->pNodeBoxes[inode] = pWk->pSrcBoxes[isrc];
}

static void build_aabb_tree_node_bbox(sxAABBTreeBuildWk* pWk, int inode, int idx, int cnt) {
	int32_t isrc = pWk->pIdxWk[idx];
	cxAABB bb = pWk->pSrcBoxes[isrc];
	for (int i = 1; i < cnt; ++i) {
		isrc = pWk->pIdxWk[idx + i];
		bb.merge(pWk->pSrcBoxes[isrc]);
	}
	pWk->pNodeBoxes[inode] = bb;
}

static int build_aabb_tree_split(sxAABBTreeBuildWk* pWk, int idx, int cnt, float pivot, int axis) {
	int mid = 0;
	int32_t* pIdx = pWk->pIdxWk;
	const cxAABB* pBoxes = pWk->pSrcBoxes;
	for (int i = 0; i < cnt; ++i) {
		int ibox = pIdx[idx + i];
		cxAABB bb = pBoxes[ibox];
		float c = bb.get_center()[axis];
		if (c < pivot) {
			pIdx[idx + i] = pIdx[idx + mid];
			pIdx[idx + mid] = ibox;
			++mid;
		}
	}
	if (mid == 0 || mid == cnt) {
		mid = cnt >> 1;
	}
	return mid;
}

static void build_aabb_tree_node(sxAABBTreeBuildWk* pWk, int inode, int idx, int cnt, int axis) {
	if (cnt == 1) {
		build_aabb_tree_leaf(pWk, inode, idx);
	} else if (cnt == 2) {
		int32_t* pInfo = &pWk->pNodeInfos[inode * 2];
		pInfo[0] = pWk->nodeCnt;
		pInfo[1] = pWk->nodeCnt + 1;
		pWk->nodeCnt += 2;
		build_aabb_tree_leaf(pWk, pInfo[0], idx);
		build_aabb_tree_leaf(pWk, pInfo[1], idx + 1);
		pWk->pNodeBoxes[inode] = pWk->pNodeBoxes[pInfo[0]];
		pWk->pNodeBoxes[inode].merge(pWk->pNodeBoxes[pInfo[1]]);
	} else {
		build_aabb_tree_node_bbox(pWk, inode, idx, cnt);
		float piv = pWk->pNodeBoxes[inode].get_center()[axis];
		int mid = build_aabb_tree_split(pWk, idx, cnt, piv, axis);
		int nextAxis = (axis + 1) % 3;
		int32_t* pInfo = &pWk->pNodeInfos[inode * 2];
		pInfo[0] = pWk->nodeCnt;
		pInfo[1] = pWk->nodeCnt + 1;
		pWk->nodeCnt += 2;
		build_aabb_tree_node(pWk, pInfo[0], idx, mid, nextAxis);
		build_aabb_tree_node(pWk, pInfo[1], idx + mid, cnt - mid, nextAxis);
	}
}

void build_aabb_tree(const cxAABB* pSrcBoxes, const int32_t nsrc, cxAABB* pNodeBoxes, int32_t* pNodeInfos, int32_t* pIdxWk) {
	sxAABBTreeBuildWk wk;
	wk.pSrcBoxes = pSrcBoxes;
	wk.nsrc = nsrc;
	wk.pNodeBoxes = pNodeBoxes;
	wk.pNodeInfos = pNodeInfos;
	wk.pIdxWk = pIdxWk;
	wk.nodeCnt = 1;
	for (int32_t i = 0; i < nsrc; ++i) {
		pIdxWk[i] = i;
	}
	cxAABB rootBB = pSrcBoxes[0];
	for (int32_t i = 1; i < nsrc; ++i) {
		rootBB.merge(pSrcBoxes[i]);
	}
	pNodeBoxes[0] = rootBB;
	cxVec vsize = rootBB.get_size_vec();
	float maxSize = vsize.max_elem();
	int axis = 0;
	if (maxSize == vsize.y) {
		axis = 1;
	} else if (maxSize == vsize.z) {
		axis = 2;
	}
	build_aabb_tree_node(&wk, 0, 0, nsrc, axis);
}

} // nxGeom


void cxPlane::calc(const cxVec& pos, const cxVec& nrm) {
	mABC = nrm;
	mD = pos.dot(nrm);
}


bool cxSphere::overlaps(const cxAABB& box) const {
	return nxGeom::sph_aabb_overlap(get_center(), get_radius(), box.get_min_pos(), box.get_max_pos());
}

bool cxSphere::overlaps(const cxCapsule& cap, cxVec* pCapAxisPos) const {
	return nxGeom::sph_cap_overlap(get_center(), get_radius(), cap.get_pos0(), cap.get_pos1(), cap.get_radius(), pCapAxisPos);
}


void cxAABB::transform(const cxAABB& box, const cxMtx& mtx) {
	cxVec omin = box.mMin;
	cxVec omax = box.mMax;
	cxVec nmin = mtx.get_translation();
	cxVec nmax = nmin;
	cxVec va, ve, vf;

	va = mtx.get_row_vec(0);
	ve = va * omin.x;
	vf = va * omax.x;
	nmin += nxVec::min(ve, vf);
	nmax += nxVec::max(ve, vf);

	va = mtx.get_row_vec(1);
	ve = va * omin.y;
	vf = va * omax.y;
	nmin += nxVec::min(ve, vf);
	nmax += nxVec::max(ve, vf);

	va = mtx.get_row_vec(2);
	ve = va * omin.z;
	vf = va * omax.z;
	nmin += nxVec::min(ve, vf);
	nmax += nxVec::max(ve, vf);

	mMin = nmin;
	mMax = nmax;
}

void cxAABB::from_sph(const cxSphere& sph) {
	cxVec c = sph.get_center();
	cxVec r = sph.get_radius();
	mMin = c - r;
	mMax = c + r;
}

cxVec cxAABB::closest_pnt(const cxVec& pos, bool onFace) const {
	cxVec res = nxVec::min(nxVec::max(pos, mMin), mMax);
	if (onFace && contains(res)) {
		cxVec relMin = res - mMin;
		cxVec relMax = mMax - res;
		cxVec dmin = nxVec::min(relMin, relMax);
		int elemIdx = (dmin.x < dmin.y && dmin.x < dmin.z) ? 0 : dmin.y < dmin.z ? 1 : 2;
		res.set_at(elemIdx, (relMin[elemIdx] < relMax[elemIdx] ? mMin : mMax)[elemIdx]);
	}
	return res;
}

bool cxAABB::seg_ck(const cxVec& p0, const cxVec& p1) const {
	cxVec dir = p1 - p0;
	float len = dir.mag();
	if (len < FLT_EPSILON) {
		return contains(p0);
	}
	float tmin = 0.0f;
	float tmax = len;

	if (dir.x != 0) {
		float dx = len / dir.x;
		float x1 = (mMin.x - p0.x) * dx;
		float x2 = (mMax.x - p0.x) * dx;
		float xmin = nxCalc::min(x1, x2);
		float xmax = nxCalc::max(x1, x2);
		tmin = nxCalc::max(tmin, xmin);
		tmax = nxCalc::min(tmax, xmax);
		if (tmin > tmax) {
			return false;
		}
	} else {
		if (p0.x < mMin.x || p0.x > mMax.x) return false;
	}

	if (dir.y != 0) {
		float dy = len / dir.y;
		float y1 = (mMin.y - p0.y) * dy;
		float y2 = (mMax.y - p0.y) * dy;
		float ymin = nxCalc::min(y1, y2);
		float ymax = nxCalc::max(y1, y2);
		tmin = nxCalc::max(tmin, ymin);
		tmax = nxCalc::min(tmax, ymax);
		if (tmin > tmax) {
			return false;
		}
	} else {
		if (p0.y < mMin.y || p0.y > mMax.y) return false;
	}

	if (dir.z != 0) {
		float dz = len / dir.z;
		float z1 = (mMin.z - p0.z) * dz;
		float z2 = (mMax.z - p0.z) * dz;
		float zmin = nxCalc::min(z1, z2);
		float zmax = nxCalc::max(z1, z2);
		tmin = nxCalc::max(tmin, zmin);
		tmax = nxCalc::min(tmax, zmax);
		if (tmin > tmax) {
			return false;
		}
	} else {
		if (p0.z < mMin.z || p0.z > mMax.z) return false;
	}

	if (tmax > len) return false;
	return true;
}


bool cxCapsule::overlaps(const cxAABB& box) const {
	return nxGeom::cap_aabb_overlap(mPos0, mPos1, mRadius, box.get_min_pos(), box.get_max_pos());
}

bool cxCapsule::overlaps(const cxCapsule& cap) const {
	return nxGeom::cap_cap_overlap(mPos0, mPos1, mRadius, cap.mPos0, cap.mPos1, cap.mRadius);
}


void cxFrustum::calc_normals() {
	mNrm[0] = nxGeom::tri_normal_cw(mPnt[0], mPnt[1], mPnt[3]); /* near */
	mNrm[1] = nxGeom::tri_normal_cw(mPnt[0], mPnt[3], mPnt[4]); /* left */
	mNrm[2] = nxGeom::tri_normal_cw(mPnt[0], mPnt[4], mPnt[5]); /* top */
	mNrm[3] = nxGeom::tri_normal_cw(mPnt[6], mPnt[2], mPnt[1]); /* right */
	mNrm[4] = nxGeom::tri_normal_cw(mPnt[6], mPnt[7], mPnt[3]); /* bottom */
	mNrm[5] = nxGeom::tri_normal_cw(mPnt[6], mPnt[5], mPnt[4]); /* far */
}

void cxFrustum::calc_planes() {
	for (int i = 0; i < 6; ++i) {
		static int vtxNo[] = { 0, 0, 0, 6, 6, 6 };
		mPlane[i].calc(mPnt[vtxNo[i]], mNrm[i]);
	}
}

void cxFrustum::init(const cxMtx& mtx, float fovy, float aspect, float znear, float zfar) {
	int i;
	float x, y, z;
	float t = ::mth_tanf(fovy * 0.5f);
	z = znear;
	y = t * z;
	x = y * aspect;
	mPnt[0].set(-x, y, -z);
	mPnt[1].set(x, y, -z);
	mPnt[2].set(x, -y, -z);
	mPnt[3].set(-x, -y, -z);
	z = zfar;
	y = t * z;
	x = y * aspect;
	mPnt[4].set(-x, y, -z);
	mPnt[5].set(x, y, -z);
	mPnt[6].set(x, -y, -z);
	mPnt[7].set(-x, -y, -z);
	calc_normals();
	for (i = 0; i < 8; ++i) {
		mPnt[i] = mtx.calc_pnt(mPnt[i]);
	}
	for (i = 0; i < 6; ++i) {
		mNrm[i] = mtx.calc_vec(mNrm[i]);
	}
	calc_planes();
}

cxVec cxFrustum::get_center() const {
	cxVec c = mPnt[0];
	for (int i = 1; i < 8; ++i) {
		c += mPnt[i];
	}
	c /= 8.0f;
	return c;
}


cxVec cxFrustum::get_vertex(const int idx) const {
	cxVec v(0.0f);
	if (unsigned(idx) < 8) {
		v = mPnt[idx];
	}
	return v;
}

bool cxFrustum::cull(const cxSphere& sph) const {
	cxVec c = sph.get_center();
	float r = sph.get_radius();
	cxVec v = c - mPnt[0];
	for (int i = 0; i < 6; ++i) {
		if (i == 3) v = c - mPnt[6];
		float d = v.dot(mNrm[i]);
		if (r < d) return true;
	}
	return false;
}

bool cxFrustum::overlaps(const cxSphere& sph) const {
	float sd = FLT_MAX;
	bool flg = false;
	cxVec c = sph.get_center();
	for (int i = 0; i < 6; ++i) {
		if (mPlane[i].signed_dist(c) > 0.0f) {
			static uint16_t vtxTbl[6] = {
				0x0123,
				0x0374,
				0x1045,
				0x5621,
				0x6732,
				0x4765
			};
			uint32_t vidx = vtxTbl[i];
			cxVec vtx[4];
			for (int j = 0; j < 4; ++j) {
				vtx[j] = mPnt[vidx & 0xF];
				vidx >>= 4;
			}
			float dist = nxGeom::quad_dist2(c, vtx);
			sd = nxCalc::min(sd, dist);
			flg = true;
		}
	}
	if (!flg) return true;
	if (sd <= nxCalc::sq(sph.get_radius())) return true;
	return false;
}

bool cxFrustum::cull(const cxAABB& box) const {
	cxVec c = box.get_center();
	cxVec r = box.get_max_pos() - c;
	cxVec v = c - mPnt[0];
	for (int i = 0; i < 6; ++i) {
		cxVec n = mNrm[i];
		if (i == 3) v = c - mPnt[6];
		if (r.dot(n.abs_val()) < v.dot(n)) return true;
	}
	return false;
}

static struct {
	uint8_t i0, i1;
} s_frustumEdgeTbl[] = {
	{ 0, 1 },{ 1, 2 },{ 2, 3 },{ 3, 0 },
	{ 4, 5 },{ 5, 6 },{ 6, 7 },{ 7, 4 },
	{ 0, 4 },{ 1, 5 },{ 2, 6 },{ 3, 7 }
};

bool cxFrustum::overlaps(const cxAABB& box) const {
	for (int i = 0; i < 12; ++i) {
		cxVec p0 = mPnt[s_frustumEdgeTbl[i].i0];
		cxVec p1 = mPnt[s_frustumEdgeTbl[i].i1];
		if (box.seg_ck(p0, p1)) return true;
	}
	const int imin = 0;
	const int imax = 1;
	cxVec bb[2];
	bb[imin] = box.get_min_pos();
	bb[imax] = box.get_max_pos();
	static struct {
		uint8_t i0x, i0y, i0z, i1x, i1y, i1z;
	} boxEdgeTbl[] = {
		{ imin, imin, imin, imax, imin, imin },
		{ imax, imin, imin, imax, imin, imax },
		{ imax, imin, imax, imin, imin, imax },
		{ imin, imin, imax, imin, imin, imin },

		{ imin, imax, imin, imax, imax, imin },
		{ imax, imax, imin, imax, imax, imax },
		{ imax, imax, imax, imin, imax, imax },
		{ imin, imax, imax, imin, imax, imin },

		{ imin, imin, imin, imin, imax, imin },
		{ imax, imin, imin, imax, imax, imin },
		{ imax, imin, imax, imax, imax, imax },
		{ imin, imin, imax, imin, imax, imax }
	};
	for (int i = 0; i < 12; ++i) {
		float p0x = bb[boxEdgeTbl[i].i0x].x;
		float p0y = bb[boxEdgeTbl[i].i0y].y;
		float p0z = bb[boxEdgeTbl[i].i0z].z;
		float p1x = bb[boxEdgeTbl[i].i1x].x;
		float p1y = bb[boxEdgeTbl[i].i1y].y;
		float p1z = bb[boxEdgeTbl[i].i1z].z;
		cxVec p0(p0x, p0y, p0z);
		cxVec p1(p1x, p1y, p1z);
		if (nxGeom::seg_polyhedron_intersect(p0, p1, mPlane, 6)) return true;
	}
	return false;
}

void cxFrustum::dump_geo(FILE* pOut) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOut) return;
	const int nedges = XD_ARY_LEN(s_frustumEdgeTbl);
	::fprintf(pOut, "PGEOMETRY V5\n");
	::fprintf(pOut, "NPoints %d NPrims %d\n", 8, nedges);
	::fprintf(pOut, "NPointGroups 0 NPrimGroups 0\n");
	::fprintf(pOut, "NPointAttrib 0 NVertexAttrib 0 NPrimAttrib 0 NAttrib 0\n");
	for (int i = 0; i < 8; ++i) {
		cxVec v = get_vertex(i);
		::fprintf(pOut, "%f %f %f 1\n", v.x, v.y, v.z);
	}
	::fprintf(pOut, "Run %d Poly\n", nedges);
	for (int i = 0; i < nedges; ++i) {
		::fprintf(pOut, " 2 < %d %d\n", s_frustumEdgeTbl[i].i0, s_frustumEdgeTbl[i].i1);
	}
	::fprintf(pOut, "beginExtra\n");
	::fprintf(pOut, "endExtra\n");
#endif
}

void cxFrustum::dump_geo(const char* pOutPath) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOutPath) return;
	FILE* pOut = nxSys::fopen_w_txt(pOutPath);
	if (!pOut) {
		return;
	}
	dump_geo(pOut);
	::fclose(pOut);
#endif
}


namespace nxColor {

void init_XYZ_transform(cxMtx* pRGB2XYZ, cxMtx* pXYZ2RGB, cxVec* pPrims, cxVec* pWhite) {
	cxVec rx, ry, rz;
	if (pPrims) {
		rx = pPrims[0];
		ry = pPrims[1];
		rz = pPrims[2];
	} else {
		/* 709 primaries */
		rx.set(0.640f, 0.330f, 0.030f);
		ry.set(0.300f, 0.600f, 0.100f);
		rz.set(0.150f, 0.060f, 0.790f);
	}

	cxVec w;
	if (pWhite) {
		w = *pWhite;
	} else {
		/* D65 1931 */
		w.set(0.3127f, 0.3290f, 0.3582f);
	}
	w.scl(nxCalc::rcp0(w.y));

	cxMtx cm;
	cm.set_rot_frame(rx, ry, rz);

	cxMtx im = cm.get_inverted();
	cxVec j = im.calc_vec(w);
	cxMtx tm;
	tm.mk_scl(j);
	tm.mul(cm);
	if (pRGB2XYZ) {
		*pRGB2XYZ = tm;
	}
	if (pXYZ2RGB) {
		*pXYZ2RGB = tm.get_inverted();
	}
}

void init_XYZ_transform_xy_w(cxMtx* pRGB2XYZ, cxMtx* pXYZ2RGB, float wx, float wy) {
	cxVec w(wx, wy, 1.0f - (wx + wy));
	init_XYZ_transform(pRGB2XYZ, pXYZ2RGB, nullptr, &w);
}

void init_XYZ_transform_xy(cxMtx* pRGB2XYZ, cxMtx* pXYZ2RGB, float rx, float ry, float gx, float gy, float bx, float by, float wx, float wy) {
	cxVec prims[3];
	prims[0].set(rx, ry, 1.0f - (rx + ry));
	prims[1].set(gx, gy, 1.0f - (gx + gy));
	prims[2].set(bx, by, 1.0f - (bx + by));
	cxVec white(wx, wy, 1.0f - (wx + wy));
	init_XYZ_transform(pRGB2XYZ, pXYZ2RGB, prims, &white);
}

cxVec XYZ_to_xyY(const cxVec& xyz) {
	float s = xyz.x + xyz.y + xyz.z;
	cxVec sv = xyz * nxCalc::rcp0(s);
	return cxVec(sv.x, sv.y, xyz.y);
}

cxVec xyY_to_XYZ(const cxVec& xyY) {
	float x = xyY.x;
	float y = xyY.y;
	float z = 1.0f - (x + y);
	float Y = xyY.z;
	float s = nxCalc::div0(Y, y);
	return cxVec(x * s, Y, z * s);
}

cxVec XYZ_to_Lab(const cxVec& xyz, cxMtx* pRGB2XYZ) {
	cxVec white = cxColor(1.0f).XYZ(pRGB2XYZ);
	cxVec l = xyz * nxVec::rcp0(white);
	for (int i = 0; i < 3; ++i) {
		float e = l.get_at(i);
		if (e > 0.008856f) {
			e = nxCalc::cb_root(e);
		} else {
			e = e*7.787f + (16.0f / 116);
		}
		l.set_at(i, e);
	}
	float lx = l.x;
	float ly = l.y;
	float lz = l.z;
	return cxVec(116.0f*ly - 16.0f, 500.0f*(lx - ly), 200.0f*(ly - lz));
}

cxVec Lab_to_XYZ(const cxVec& lab, cxMtx* pRGB2XYZ) {
	cxVec white = cxColor(1.0f).XYZ(pRGB2XYZ);
	float L = lab.x;
	float a = lab.y;
	float b = lab.z;
	float ly = (L + 16.0f) / 116.0f;
	cxVec t(a/500.0f + ly, ly, -b/200.0f + ly);
	for (int i = 0; i < 3; ++i) {
		float e = t.get_at(i);
		if (e > 0.206893f) {
			e = nxCalc::cb(e);
		} else {
			e = (e - (16.0f/116)) / 7.787f;
		}
		t.set_at(i, e);
	}
	t.mul(white);
	return t;
}

cxVec Lab_to_Lch(const cxVec& lab) {
	float L = lab.x;
	float a = lab.y;
	float b = lab.z;
	float c = nxCalc::hypot(a, b);
	float h = ::mth_atan2f(b, a);
	return cxVec(L, c, h);
}

cxVec Lch_to_Lab(const cxVec& lch) {
	float L = lch.x;
	float c = lch.y;
	float h = lch.z;
	float hs = ::mth_sinf(h);
	float hc = ::mth_cosf(h);
	return cxVec(L, c*hc, c*hs);
}

float Lch_perceived_lightness(const cxVec& lch) {
	float L = lch.x;
	float c = lch.y;
	float h = lch.z;
	float fh = ::mth_fabsf(::mth_sinf((h - (XD_PI*0.5f)) * 0.5f))*0.116f + 0.085f;
	float fL = 2.5f - (2.5f/100)*L; // 1 at 60, 0 at 100 (white)
	return L + fh*fL*c;
}


// Single-Lobe CMF fits from Wyman et al. "Simple Analytic Approximations to the CIE XYZ Color Matching Functions"

float approx_CMF_x31(float w) {
	return 1.065f*::mth_expf(-0.5f * nxCalc::sq((w - 595.8f) / 33.33f)) + 0.366f*::mth_expf(-0.5f * nxCalc::sq((w - 446.8f) / 19.44f));
}

float approx_CMF_y31(float w) {
	return 1.014f*::mth_expf(-0.5f * nxCalc::sq((::mth_logf(w) - 6.32130766f) / 0.075f));
}

float approx_CMF_z31(float w) {
	return 1.839f*::mth_expf(-0.5f * nxCalc::sq((::mth_logf(w) - 6.1088028f) / 0.051f));
}

float approx_CMF_x64(float w) {
	return 0.398f*::mth_expf(-1250.0f * nxCalc::sq(::mth_logf((w + 570.1f) / 1014.0f))) + 1.132f*::mth_expf(-234.0f * nxCalc::sq(::mth_logf((1338.0f - w) / 743.5f)));
}

float approx_CMF_y64(float w) {
	return 1.011f*::mth_expf(-0.5f * nxCalc::sq((w - 556.1f) / 46.14f));
}

float approx_CMF_z64(float w) {
	return 2.06f*::mth_expf(-32.0f * nxCalc::sq(::mth_logf((w - 265.8f) / 180.4f)));
}

} // nxColor

cxVec cxColor::YCgCo() const {
	cxVec rgb = rgb_vec();
	return cxVec(
		rgb.dot(cxVec(0.25f, 0.5f, 0.25f)),
		rgb.dot(cxVec(-0.25f, 0.5f, -0.25f)),
		rgb.dot(cxVec(0.5f, 0.0f, -0.5f))
	);
}

void cxColor::from_YCgCo(const cxVec& ygo) {
	float Y = ygo[0];
	float G = ygo[1];
	float O = ygo[2];
	float t = Y - G;
	r = t + O;
	g = Y + G;
	b = t - O;
}

cxVec cxColor::TMI() const {
	float T = b - r;
	float M = (r - g*2.0f + b) * 0.5f;
	float I = average();
	return cxVec(T, M, I);
}

void cxColor::from_TMI(const cxVec& tmi) {
	float T = tmi[0];
	float M = tmi[1];
	float I = tmi[2];
	r = I - T*0.5f + M/3.0f;
	g = I - M*2.0f/3.0f;
	b = I + T*0.5f + M/3.0f;
	a = 1.0f;
}

static inline cxMtx* mtx_RGB2XYZ(cxMtx* pRGB2XYZ) {
	static float tm709[] = {
		0.412453f, 0.212671f, 0.019334f, 0.0f,
		0.357580f, 0.715160f, 0.119193f, 0.0f,
		0.180423f, 0.072169f, 0.950227f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	return pRGB2XYZ ? pRGB2XYZ : (cxMtx*)tm709;
}

static inline cxMtx* mtx_XYZ2RGB(cxMtx* pXYZ2RGB) {
	static float im709[] = {
		 3.240479f, -0.969256f,  0.055648f, 0.0f,
		-1.537150f,  1.875992f, -0.204043f, 0.0f,
		-0.498535f,  0.041556f,  1.057311f, 0.0f,
		 0.0f, 0.0f, 0.0f, 1.0f
	};
	return pXYZ2RGB ? pXYZ2RGB : (cxMtx*)im709;
}

cxVec cxColor::XYZ(cxMtx* pRGB2XYZ) const {
	return mtx_RGB2XYZ(pRGB2XYZ)->calc_vec(rgb_vec());
}

void cxColor::from_XYZ(const cxVec& xyz, cxMtx* pXYZ2RGB) {
	set(mtx_XYZ2RGB(pXYZ2RGB)->calc_vec(xyz));
}

cxVec cxColor::xyY(cxMtx* pRGB2XYZ) const {
	return nxColor::XYZ_to_xyY(XYZ(pRGB2XYZ));
}

void cxColor::from_xyY(const cxVec& xyY, cxMtx* pXYZ2RGB) {
	from_XYZ(nxColor::xyY_to_XYZ(xyY), pXYZ2RGB);
}

cxVec cxColor::Lab(cxMtx* pRGB2XYZ) const {
	return nxColor::XYZ_to_Lab(XYZ(pRGB2XYZ), pRGB2XYZ);
}

void cxColor::from_Lab(const cxVec& lab, cxMtx* pRGB2XYZ, cxMtx* pXYZ2RGB) {
	from_XYZ(nxColor::Lab_to_XYZ(lab, pRGB2XYZ), pXYZ2RGB);
}

cxVec cxColor::Lch(cxMtx* pRGB2XYZ) const {
	return nxColor::Lab_to_Lch(Lab(pRGB2XYZ));
}

void cxColor::from_Lch(const cxVec& lch, cxMtx* pRGB2XYZ, cxMtx* pXYZ2RGB) {
	from_Lab(nxColor::Lch_to_Lab(lch), pRGB2XYZ, pXYZ2RGB);
}

uint32_t cxColor::encode_rgbe() const {
	float m = max();
	if (m < 1.0e-32f) return 0;
	float t[3];
	int e;
	float s = nxCalc::div0(::mth_frexpf(m, &e) * 256.0f, m);
	for (int i = 0; i < 3; ++i) {
		t[i] = (*this)[i] * s;
	}
	uxVal32 res;
	for (int i = 0; i < 3; ++i) {
		res.b[i] = (uint8_t)t[i];
	}
	res.b[3] = e + 128;
	return res.u;
}

void cxColor::decode_rgbe(uint32_t rgbe) {
	if (rgbe) {
		uxVal32 v;
		v.u = rgbe;
		int e = v.b[3];
		float s = ::mth_ldexpf(1.0f, e - (128 + 8));
		float t[3];
		for (int i = 0; i < 3; ++i) {
			t[i] = (float)v.b[i];
		}
		for (int i = 0; i < 3; ++i) {
			(*this)[i] = t[i] * s;
		}
		a = 1.0f;
	} else {
		zero_rgb();
	}
}

static inline uint32_t swap_enc_clr_rb(uint32_t enc) {
	uxVal32 v;
	v.u = enc;
	uint8_t t = v.b[0];
	v.b[0] = v.b[2];
	v.b[2] = t;
	return v.u;
}

uint32_t cxColor::encode_bgre() const {
	return swap_enc_clr_rb(encode_rgbe());
}

void cxColor::decode_bgre(uint32_t bgre) {
	decode_rgbe(swap_enc_clr_rb(bgre));
}

uint32_t cxColor::encode_rgbi() const {
	float e[4];
	for (int i = 0; i < 3; ++i) {
		e[i] = nxCalc::clamp((*this)[i], 0.0f, 100.0f);
	}
	e[3] = 1.0f;
	for (int i = 0; i < 4; ++i) {
		e[i] = ::mth_sqrtf(e[i]); /* gamma 2.0 encoding */
	}
	float m = nxCalc::max(e[0], e[1], e[2]);
	if (m > 1.0f) {
		float s = 1.0f / m;
		for (int i = 0; i < 4; ++i) {
			e[i] *= s;
		}
	}
	for (int i = 0; i < 4; ++i) {
		e[i] *= 255.0f;
	}
	uxVal32 res;
	for (int i = 0; i < 4; ++i) {
		res.b[i] = (uint8_t)e[i];
	}
	return res.u;
}

void cxColor::decode_rgbi(uint32_t rgbi) {
	uxVal32 v;
	v.u = rgbi;
	for (int i = 0; i < 4; ++i) {
		(*this)[i] = (float)v.b[i];
	}
	scl(nxCalc::rcp0(a));
	for (int i = 0; i < 4; ++i) {
		(*this)[i] = nxCalc::sq((*this)[i]); /* gamma 2.0 decoding */
	}
}

uint32_t cxColor::encode_bgri() const {
	return swap_enc_clr_rb(encode_rgbi());
}

void cxColor::decode_bgri(uint32_t bgri) {
	decode_rgbi(swap_enc_clr_rb(bgri));
}

uint32_t cxColor::encode_rgba8() const {
	float f[4];
	for (int i = 0; i < 4; ++i) {
		f[i] = nxCalc::saturate((*this)[i]);
	}
	for (int i = 0; i < 4; ++i) {
		f[i] *= 255.0f;
	}
	uxVal32 v;
	for (int i = 0; i < 4; ++i) {
		v.b[i] = (uint8_t)f[i];
	}
	return v.u;
}

void cxColor::decode_rgba8(uint32_t rgba) {
	uxVal32 v;
	v.u = rgba;
	for (int i = 0; i < 4; ++i) {
		(*this)[i] = (float)v.b[i];
	}
	scl(1.0f / 255.0f);
}

uint32_t cxColor::encode_bgra8() const {
	return swap_enc_clr_rb(encode_rgba8());
}

void cxColor::decode_bgra8(uint32_t bgra) {
	decode_rgba8(swap_enc_clr_rb(bgra));
}

uint16_t cxColor::encode_bgr565() const {
	uint16_t ir = (uint16_t)::mth_roundf(nxCalc::saturate(r) * float(0x1F));
	uint16_t ig = (uint16_t)::mth_roundf(nxCalc::saturate(g) * float(0x3F));
	uint16_t ib = (uint16_t)::mth_roundf(nxCalc::saturate(b) * float(0x1F));
	uint16_t ic = ib;
	ic |= ig << 5;
	ic |= ir << (5 + 6);
	return ic;
}

void cxColor::decode_bgr565(uint16_t bgr) {
	b = float(bgr & 0x1F);
	g = float((bgr >> 5) & 0x3F);
	r = float((bgr >> (5+6)) & 0x1F);
	scl_rgb(1.0f / float(0x1F), 1.0f / float(0x3F), 1.0f / float(0x1F));
	a = 1.0f;
}


xt_float3 sxToneMap::get_inv_white() const {
	xt_float3 iw;
	for (int i = 0; i < 3; ++i) {
		iw[i] = nxCalc::rcp0(mLinWhite[i]);
	}
	return iw;
}

cxColor sxToneMap::apply(const cxColor& c) const {
	cxColor res;
	xt_float3 iw = get_inv_white();
	for (int i = 0; i < 3; ++i) {
		float cc = c[i];
		res[i] = nxCalc::div0((cc * (1.0f + cc*iw[i])), 1.0f + cc);
	}
	for (int i = 0; i < 3; ++i) {
		res[i] *= mLinGain[i];
	}
	for (int i = 0; i < 3; ++i) {
		res[i] += mLinBias[i];
	}
	for (int i = 0; i < 3; ++i) {
		res[i] = nxCalc::max(res[i], 0.0f);
	}
	res.a = c.a;
	return res;
}


XD_NOINLINE float sxView::get_aspect() const {
	float aspect = get_native_aspect();
	if (mMode == Mode::ROT_L90 || mMode == Mode::ROT_R90) {
		aspect = nxCalc::rcp0(aspect);
	}
	return aspect;
}

XD_NOINLINE float sxView::calc_fovx() const {
	return 2.0f * ::mth_atanf(::mth_tanf(XD_DEG2RAD(mDegFOVY) * 0.5f) * get_aspect());
}

XD_NOINLINE cxVec sxView::get_uv_dir(const float u, const float v) const {
	float nu = nxCalc::fit(u, 0.0f, 1.0f, -1.0f, 1.0f);
	float nv = nxCalc::fit(v, 0.0f, 1.0f, -1.0f, 1.0f);
	cxVec dir(nu, nv, 1.0f);
	dir = mInvProjMtx.calc_pnt(dir);
	dir.normalize();
	dir = mInvViewMtx.calc_vec(dir);
	return dir;
}

XD_NOINLINE cxMtx sxView::get_mode_mtx() const {
	cxMtx m;
	switch (mMode) {
		case Mode::ROT_L90:
			m = nxMtx::mk_rot_z(XD_DEG2RAD(90));
			break;
		case Mode::ROT_R90:
			m = nxMtx::mk_rot_z(XD_DEG2RAD(-90));
			break;
		case Mode::ROT_180:
			m = nxMtx::mk_rot_z(XD_DEG2RAD(-180));
			break;
		default:
			m.identity();
			break;
	}
	return m;
}

XD_NOINLINE void sxView::init(const int width, const int height) {
	mMode = Mode::STANDARD;
	set_window(width, height);
	set_frame(cxVec(0.75f, 1.3f, 3.5f), cxVec(0.0f, 0.95f, 0.0f));
	set_range(0.1f, 1000.0f);
	set_deg_fovy(30.0f);
	update();
}

XD_NOINLINE void sxView::update() {
	mViewMtx.mk_view(mPos, mTgt, mUp, &mInvViewMtx);
	float fovy = XD_DEG2RAD(mDegFOVY);
	float aspect = get_aspect();
	mProjMtx.mk_proj(fovy, aspect, mNear, mFar);
	switch (mMode) {
		case Mode::ROT_L90:
		case Mode::ROT_R90:
		case Mode::ROT_180:
			mProjMtx.mul(get_mode_mtx());
			break;
		default:
			break;
	}
	mViewProjMtx = mViewMtx * mProjMtx;
	mInvProjMtx = mProjMtx.get_inverted();
	mInvViewProjMtx = mViewProjMtx.get_inverted();
	mFrustum.init(mInvViewMtx, fovy, aspect, mNear, mFar);
}

XD_NOINLINE bool sxView::ck_sphere_visibility(const cxSphere& sph, const bool exact) const {
	bool vis = !mFrustum.cull(sph);
	if (vis && exact) {
		vis = mFrustum.overlaps(sph);
	}
	return vis;
}

XD_NOINLINE bool sxView::ck_box_visibility(const cxAABB& box, const bool exact) const {
	bool vis = !mFrustum.cull(box);
	if (vis && exact) {
		vis = mFrustum.overlaps(box);
	}
	return vis;
}

XD_NOINLINE cxColor sxHemisphereLight::eval(const cxVec& v) const {
	float val = (v.dot(cxVec(mUp.x, mUp.y, mUp.z)) + 1.0f) * 0.5f;
	val = nxCalc::saturate(::mth_powf(val, mExp) * mGain);
	cxColor clr(
		nxCalc::lerp(mLower.x, mUpper.x, val),
		nxCalc::lerp(mLower.y, mUpper.y, val),
		nxCalc::lerp(mLower.z, mUpper.z, val)
		);
	return clr;
}


void sxPrimVtx::encode_normal(const cxVec& nrm) {
	xt_float2 oct = nrm.encode_octa();
	prm.x = oct.x;
	prm.y = oct.y;
}


namespace nxSH {

void calc_weights(float* pWgt, int order, float s, float scl) {
	if (!pWgt) return;
	for (int i = 0; i < order; ++i) {
		pWgt[i] = ::mth_expf(float(-i*i) / (2.0f*s)) * scl;
	}
}

void get_diff_weights(float* pWgt, int order, float scl) {
	for (int i = 0; i < order; ++i) {
		pWgt[i] = 0.0f;
	}
	if (order >= 1) pWgt[0] = scl;
	if (order >= 2) pWgt[1] = scl / 1.5f;
	if (order >= 3) pWgt[2] = scl / 4.0f;
}

void apply_weights(float* pDst, int order, const float* pSrc, const float* pWgt, int nelems) {
	for (int l = 0; l < order; ++l) {
		int i0 = calc_coefs_num(l);
		int i1 = calc_coefs_num(l + 1);
		float w = pWgt[l];
		for (int i = i0; i < i1; ++i) {
			int idx = i * nelems;
			for (int j = 0; j < nelems; ++j) {
				pDst[idx + j] = pSrc[idx + j] * w;
			}
		}
	}
}

double calc_K(int l, int m) {
	int am = m < 0 ? -m : m;
	double v = 1.0;
	for (int k = l + am; k > l - am; --k) {
		v *= k;
	}
	return ::mth_sqrt((2.0*l + 1.0) / (4.0*M_PI*v));
}

double calc_Pmm(int m) {
	double v = 1.0;
	for (int k = 0; k <= m; ++k) {
		v *= 1.0 - 2.0*k;
	}
	return v;
}

double calc_constA(int m) {
	return calc_Pmm(m) * calc_K(m, m) * ::mth_sqrt(2.0);
}

double calc_constB(int m) {
	return double(2*m + 1) * calc_Pmm(m) * calc_K(m+1, m);
}

double calc_constC1(int l, int m) {
	double c = calc_K(l, m) / calc_K(l-1, m) * double(2*l-1) / double(l-m);
	if (l > 80) {
		if (mth_isnan(c)) c = 0.0;
	}
	return c;
}

double calc_constC2(int l, int m) {
	double c = -calc_K(l, m) / calc_K(l-2, m) * double(l+m-1) / double(l-m);
	if (l > 80) {
		if (mth_isnan(c)) c = 0.0;
	}
	return c;
}

double calc_constD1(int m) {
	return double((2*m+3)*(2*m+1)) * calc_Pmm(m) / 2.0 * calc_K(m+2, m);
}

double calc_constD2(int m) {
	return double(-(2*m + 1)) * calc_Pmm(m) / 2.0 * calc_K(m+2, m);
}

double calc_constE1(int m) {
	return double((2*m+5)*(2*m+3)*(2*m+1)) * calc_Pmm(m) / 6.0 * calc_K(m+3,m);
}

double calc_constE2(int m) {
	double pmm = calc_Pmm(m);
	return (double((2*m+5)*(2*m+1))*pmm/6.0 + double((2*m+2)*(2*m+1))*pmm/3.0) * -calc_K(m+3, m);
}

template<typename CONST_T>
void calc_consts_t(int order, CONST_T* pConsts) {
	if (order < 1 || !pConsts) return;
	int idx = 0;
	pConsts[idx++] = (CONST_T)calc_K(0, 0);
	if (order > 1) {
		// 1, 0
		pConsts[idx++] = (CONST_T)(calc_Pmm(0) * calc_K(1, 0));
	}
	if (order > 2) {
		// 2, 0
		pConsts[idx++] = (CONST_T)calc_constD1(0);
		pConsts[idx++] = (CONST_T)calc_constD2(0);
	}
	if (order > 3) {
		// 2, 0
		pConsts[idx++] = (CONST_T)calc_constE1(0);
		pConsts[idx++] = (CONST_T)calc_constE2(0);
	}
	for (int l = 4; l < order; ++l) {
		pConsts[idx++] = (CONST_T)calc_constC1(l, 0);
		pConsts[idx++] = (CONST_T)calc_constC2(l, 0);
	}
	const double scl = ::mth_sqrt(2.0);
	for (int m = 1; m < order-1; ++m) {
		pConsts[idx++] = (CONST_T)calc_constA(m);
		if (m + 1 < order) {
			pConsts[idx++] = (CONST_T)(calc_constB(m) * scl);
		}
		if (m + 2 < order) {
			pConsts[idx++] = (CONST_T)(calc_constD1(m) * scl);
			pConsts[idx++] = (CONST_T)(calc_constD2(m) * scl);
		}
		if (m + 3 < order) {
			pConsts[idx++] = (CONST_T)(calc_constE1(m) * scl);
			pConsts[idx++] = (CONST_T)(calc_constE2(m) * scl);
		}
		for (int l = m+4; l < order; ++l) {
			pConsts[idx++] = (CONST_T)calc_constC1(l, m);
			pConsts[idx++] = (CONST_T)calc_constC2(l, m);
		}
	}
	if (order > 1) {
		pConsts[idx++] = (CONST_T)calc_constA(order - 1);
	}
}

template<typename EVAL_T, typename CONST_T, int N>
XD_FORCEINLINE
void eval_t(int order, EVAL_T* pCoefs, const EVAL_T x[], const EVAL_T y[], const EVAL_T z[], const CONST_T* pConsts) {
	EVAL_T zz[N];
	for (int i = 0; i < N; ++i) {
		zz[i] = z[i] * z[i];
	}
	EVAL_T cval = (EVAL_T)pConsts[0];
	for (int i = 0; i < N; ++i) {
		pCoefs[i] = cval;
	}
	int idx = 1;
	int idst;
	EVAL_T* pDst;
	EVAL_T cval2;
	if (order > 1) {
		idst = calc_ary_idx(1, 0);
		pDst = &pCoefs[idst * N];
		cval = (EVAL_T)pConsts[idx++];
		for (int i = 0; i < N; ++i) {
			pDst[i] = z[i] * cval;
		}
	}
	if (order > 2) {
		idst = calc_ary_idx(2, 0);
		pDst = &pCoefs[idst * N];
		cval = (EVAL_T)pConsts[idx++];
		cval2 = (EVAL_T)pConsts[idx++];
		for (int i = 0; i < N; ++i) {
			pDst[i] = zz[i]*cval;
		}
		for (int i = 0; i < N; ++i) {
			pDst[i] += cval2;
		}
	}
	if (order > 3) {
		idst = calc_ary_idx(3, 0);
		pDst = &pCoefs[idst * N];
		cval = (EVAL_T)pConsts[idx++];
		cval2 = (EVAL_T)pConsts[idx++];
		for (int i = 0; i < N; ++i) {
			pDst[i] = zz[i]*cval;
		}
		for (int i = 0; i < N; ++i) {
			pDst[i] += cval2;
		}
		for (int i = 0; i < N; ++i) {
			pDst[i] *= z[i];
		}
	}
	for (int l = 4; l < order; ++l) {
		int isrc1 = calc_ary_idx(l-1, 0);
		int isrc2 = calc_ary_idx(l-2, 0);
		EVAL_T* pSrc1 = &pCoefs[isrc1 * N];
		EVAL_T* pSrc2 = &pCoefs[isrc2 * N];
		idst = calc_ary_idx(l, 0);
		pDst = &pCoefs[idst * N];
		cval = (EVAL_T)pConsts[idx++];
		cval2 = (EVAL_T)pConsts[idx++];
		for (int i = 0; i < N; ++i) {
			pDst[i] = z[i]*cval;
		}
		for (int i = 0; i < N; ++i) {
			pDst[i] *= pSrc1[i];
		}
		for (int i = 0; i < N; ++i) {
			pDst[i] += pSrc2[i] * cval2;
		}
	}
	EVAL_T prev[3*N];
	EVAL_T* pPrev;
	EVAL_T s[2*N];
	EVAL_T sin[N];
	for (int i = 0; i < N; ++i) {
		s[i] = y[i];
	}
	EVAL_T c[2*N];
	EVAL_T cos[N];
	for (int i = 0; i < N; ++i) {
		c[i] = x[i];
	}
	int scIdx = 0;
	for (int m = 1; m < order - 1; ++m) {
		int l = m;
		idst = l*l + l - m;
		pDst = &pCoefs[idst * N];
		cval = (EVAL_T)pConsts[idx++];
		for (int i = 0; i < N; ++i) {
			sin[i] = s[scIdx*N + i];
		}
		for (int i = 0; i < N; ++i) {
			pDst[i] = sin[i] * cval;
		}
		idst = l*l + l + m;
		pDst = &pCoefs[idst * N];
		for (int i = 0; i < N; ++i) {
			cos[i] = c[scIdx*N + i];
		}
		for (int i = 0; i < N; ++i) {
			pDst[i] = cos[i] * cval;
		}

		if (m + 1 < order) {
			// (m+1, -m), (m+1, m)
			l = m + 1;
			cval = (EVAL_T)pConsts[idx++];
			pPrev = &prev[1*N];
			for (int i = 0; i < N; ++i) {
				pPrev[i] = z[i] * cval;
			}
			idst = l*l + l - m;
			pDst = &pCoefs[idst * N];
			for (int i = 0; i < N; ++i) {
				pDst[i] = pPrev[i] * sin[i];
			}
			idst = l*l + l + m;
			pDst = &pCoefs[idst * N];
			for (int i = 0; i < N; ++i) {
				pDst[i] = pPrev[i] * cos[i];
			}
		}

		if (m + 2 < order) {
			// (m+2, -m), (m+2, m)
			l = m + 2;
			cval = (EVAL_T)pConsts[idx++];
			cval2 = (EVAL_T)pConsts[idx++];
			pPrev = &prev[2*N];
			for (int i = 0; i < N; ++i) {
				pPrev[i] = zz[i]*cval + cval2;
			}
			idst = l*l + l - m;
			pDst = &pCoefs[idst * N];
			for (int i = 0; i < N; ++i) {
				pDst[i] = pPrev[i] * sin[i];
			}
			idst = l*l + l + m;
			pDst = &pCoefs[idst * N];
			for (int i = 0; i < N; ++i) {
				pDst[i] = pPrev[i] * cos[i];
			}
		}

		if (m + 3 < order) {
			// (m+3, -m), (m+3, m)
			l = m + 3;
			cval = (EVAL_T)pConsts[idx++];
			cval2 = (EVAL_T)pConsts[idx++];
			pPrev = &prev[0*N];
			for (int i = 0; i < N; ++i) {
				pPrev[i] = zz[i]*cval;
			}
			for (int i = 0; i < N; ++i) {
				pPrev[i] += cval2;
			}
			for (int i = 0; i < N; ++i) {
				pPrev[i] *= z[i];
			}
			idst = l*l + l - m;
			pDst = &pCoefs[idst * N];
			for (int i = 0; i < N; ++i) {
				pDst[i] = pPrev[i] * sin[i];
			}
			idst = l*l + l + m;
			pDst = &pCoefs[idst * N];
			for (int i = 0; i < N; ++i) {
				pDst[i] = pPrev[i] * cos[i];
			}
		}

		const unsigned prevMask = 1 | (0 << 2) | (2 << 4) | (1 << 6) | (0 << 8) | (2 << 10) | (1 << 12);
		unsigned mask = prevMask | ((prevMask >> 2) << 14);
		unsigned maskCnt = 0;
		for (l = m + 4; l < order; ++l) {
			unsigned prevIdx = mask & 3;
			pPrev = &prev[prevIdx * N];
			EVAL_T* pPrev1 = &prev[((mask >> 2) & 3) * N];
			EVAL_T* pPrev2 = &prev[((mask >> 4) & 3) * N];
			cval = (EVAL_T)pConsts[idx++];
			cval2 = (EVAL_T)pConsts[idx++];
			for (int i = 0; i < N; ++i) {
				pPrev[i] = z[i] * cval;
			}
			for (int i = 0; i < N; ++i) {
				pPrev[i] *= pPrev1[i];
			}
			for (int i = 0; i < N; ++i) {
				pPrev[i] += pPrev2[i] * cval2;
			}
			idst = l*l + l - m;
			pDst = &pCoefs[idst * N];
			for (int i = 0; i < N; ++i) {
				pDst[i] = pPrev[i] * sin[i];
			}
			idst = l*l + l + m;
			pDst = &pCoefs[idst * N];
			for (int i = 0; i < N; ++i) {
				pDst[i] = pPrev[i] * cos[i];
			}
			if (order < 11) {
				mask >>= 4;
			} else {
				++maskCnt;
				if (maskCnt < 3) {
					mask >>= 4;
				} else {
					mask = prevMask;
					maskCnt = 0;
				}
			}
		}

		EVAL_T* pSinSrc = &s[scIdx * N];
		EVAL_T* pCosSrc = &c[scIdx * N];
		EVAL_T* pSinDst = &s[(scIdx^1) * N];
		EVAL_T* pCosDst = &c[(scIdx^1) * N];
		for (int i = 0; i < N; ++i) {
			pSinDst[i] = x[i]*pSinSrc[i] + y[i]*pCosSrc[i];
		}
		for (int i = 0; i < N; ++i) {
			pCosDst[i] = x[i]*pCosSrc[i] - y[i]*pSinSrc[i];
		}
		scIdx ^= 1;
	}

	if (order > 1) {
		cval = (EVAL_T)pConsts[idx];
		idst = calc_ary_idx(order - 1, -(order - 1));
		pDst = &pCoefs[idst * N];
		for (int i = 0; i < N; ++i) {
			sin[i] = s[scIdx*N + i];
		}
		for (int i = 0; i < N; ++i) {
			pDst[i] = sin[i] * cval;
		}
		idst = calc_ary_idx(order - 1, order - 1);
		pDst = &pCoefs[idst * N];
		for (int i = 0; i < N; ++i) {
			cos[i] = c[scIdx*N + i];
		}
		for (int i = 0; i < N; ++i) {
			pDst[i] = cos[i] * cval;
		}
	}
}

void calc_consts(int order, float* pConsts) {
	calc_consts_t(order, pConsts);
}

void calc_consts(int order, double* pConsts) {
	calc_consts_t(order, pConsts);
}

void eval(int order, float* pCoefs, float x, float y, float z, const float* pConsts) {
	eval_t<float, float, 1>(order, pCoefs, &x, &y, &z, pConsts);
}

void eval_ary4(int order, float* pCoefs, float x[4], float y[4], float z[4], const float* pConsts) {
	eval_t<float, float, 4>(order, pCoefs, x, y, z, pConsts);
}

void eval_ary8(int order, float* pCoefs, float x[8], float y[8], float z[8], const float* pConsts) {
	eval_t<float, float, 8>(order, pCoefs, x, y, z, pConsts);
}

void eval_sh3(float* pCoefs, float x, float y, float z, const float* pConsts) {
	float tc[calc_consts_num(3)];
	if (!pConsts) {
		calc_consts(3, tc);
		pConsts = tc;
	}
	eval_t<float, float, 1>(3, pCoefs, &x, &y, &z, pConsts);
}

void eval_sh3_ary4(float* pCoefs, float x[4], float y[4], float z[4], const float* pConsts) {
	float tc[calc_consts_num(3)];
	if (!pConsts) {
		calc_consts(3, tc);
		pConsts = tc;
	}
	eval_t<float, float, 4>(3, pCoefs, x, y, z, pConsts);
}

void eval_sh3_ary8(float* pCoefs, float x[8], float y[8], float z[8], const float* pConsts) {
	float tc[calc_consts_num(3)];
	if (!pConsts) {
		calc_consts(3, tc);
		pConsts = tc;
	}
	eval_t<float, float, 8>(3, pCoefs, x, y, z, pConsts);
}

void eval_sh6(float* pCoefs, float x, float y, float z, const float* pConsts) {
	eval_t<float, float, 1>(6, pCoefs, &x, &y, &z, pConsts);
}

void eval_sh6_ary4(float* pCoefs, float x[4], float y[4], float z[4], const float* pConsts) {
	eval_t<float, float, 4>(6, pCoefs, x, y, z, pConsts);
}

void eval_sh6_ary8(float* pCoefs, float x[8], float y[8], float z[8], const float* pConsts) {
	eval_t<float, float, 8>(6, pCoefs, x, y, z, pConsts);
}

cxVec extract_dominant_dir_rgb(const float* pCoefsR, const float* pCoefsG, const float* pCoefsB) {
	cxVec dir;
	if (!pCoefsR || !pCoefsG || !pCoefsB) {
		dir.zero();
		return dir;
	}
	int idx = calc_ary_idx(1, 1);
	cxColor cx(pCoefsR[idx], pCoefsG[idx], pCoefsB[idx]);
	idx = calc_ary_idx(1, -1);
	cxColor cy(pCoefsR[idx], pCoefsG[idx], pCoefsB[idx]);
	idx = calc_ary_idx(1, 0);
	cxColor cz(pCoefsR[idx], pCoefsG[idx], pCoefsB[idx]);
	float lx = cx.luminance();
	float ly = cy.luminance();
	float lz = cz.luminance();
	dir.set(-lx, -ly, lz);
	dir.normalize();
	return dir;
}

} // nxSH


cxDiffuseSH::cxDiffuseSH() {
	nxSH::calc_consts(XD_DIFFSH_ORDER, mConsts);
	clear_coefs();
	mpImg = nullptr;
	mImgW = 0;
	mImgH = 0;
	mpHemi = nullptr;
	mpFunc = nullptr;
}

void cxDiffuseSH::clear_coefs() {
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) { mCoefsR[i] = 0.0f; }
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) { mCoefsG[i] = 0.0f; }
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) { mCoefsB[i] = 0.0f; }
}

void cxDiffuseSH::operator()(const int x, const int y, const float dx, const float dy, const float dz, const float dw) {
	cxColor c(0.0f);
	if (mpImg) {
		c = mpImg[y*mImgW + x];
	} else if (mpHemi) {
		c = mpHemi->eval(cxVec(dx, dy, dz));
	} else if (mpFunc) {
		c = (*mpFunc)(x, y, cxVec(dx, dy, dz));
	}
	float ctmp[XD_DIFFSH_NUMCOEFFS];
#if XD_DIFFSH_ORDER == 3
	nxSH::eval_sh3(ctmp, dx, dy, dz, mConsts);
#else
	nxSH::eval(XD_DIFFSH_ORDER, ctmp, dx, dy, dz, mConsts);
#endif
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) {
		ctmp[i] *= dw;
	}
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) {
		mCoefsR[i] += c.r * ctmp[i];
	}
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) {
		mCoefsG[i] += c.g * ctmp[i];
	}
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) {
		mCoefsB[i] += c.b * ctmp[i];
	}
}

void cxDiffuseSH::exec_pano_scan(const int w, const int h) {
	float scl = nxCalc::panorama_scan(*this, w, h);
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) {
		mCoefsR[i] *= scl;
	}
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) {
		mCoefsG[i] *= scl;
	}
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) {
		mCoefsB[i] *= scl;
	}
}

XD_NOINLINE void cxDiffuseSH::from_img(const cxColor* pImg, const int width, const int height) {
	clear_coefs();
	if (!pImg) return;
	if (width <= 0) return;
	if (height <= 0) return;
	mpImg = pImg;
	mImgW = width;
	mImgH = height;
	exec_pano_scan(mImgW, mImgH);
	mpImg = nullptr;
	mImgW = 0;
	mImgH = 0;
}

XD_NOINLINE void cxDiffuseSH::from_hemi(const sxHemisphereLight* pHemi, const int width, const int height) {
	clear_coefs();
	if (!pHemi) return;
	int w = nxCalc::max(width, 8);
	int h = nxCalc::max(height, 4);
	mpHemi = pHemi;
	exec_pano_scan(w, h);
	mpHemi = nullptr;
}

XD_NOINLINE void cxDiffuseSH::from_func(EnvFunc& func, const int width, const int height) {
	clear_coefs();
	int w = nxCalc::max(width, 8);
	int h = nxCalc::max(height, 4);
	mpFunc = &func;
	exec_pano_scan(w, h);
	mpFunc = nullptr;
}

XD_NOINLINE cxColor cxDiffuseSH::eval(const cxVec& v, const float scale) const {
	float wgt[XD_DIFFSH_ORDER];
#if XD_DIFFSH_ORDER > 3
	nxSH::calc_weights(wgt, XD_DIFFSH_ORDER, 3.0f, scale);
#else
	nxSH::get_diff_weights(wgt, XD_DIFFSH_ORDER, scale);
#endif
	float vcoefs[XD_DIFFSH_NUMCOEFFS];
#if XD_DIFFSH_ORDER == 3
	nxSH::eval_sh3(vcoefs, v.x, v.y, v.z, mConsts);
#else
	nxSH::eval(XD_DIFFSH_ORDER, vcoefs, v.x, v.y, v.z, mConsts);
#endif
	float shc[XD_DIFFSH_NUMCOEFFS];
	float r = 0.0f;
	nxSH::apply_weights(shc, XD_DIFFSH_ORDER, mCoefsR, wgt);
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) {
		r += vcoefs[i] * shc[i];
	}
	float g = 0.0f;
	nxSH::apply_weights(shc, XD_DIFFSH_ORDER, mCoefsG, wgt);
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) {
		g += vcoefs[i] * shc[i];
	}
	float b = 0.0f;
	nxSH::apply_weights(shc, XD_DIFFSH_ORDER, mCoefsB, wgt);
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) {
		b += vcoefs[i] * shc[i];
	}
	return cxColor(r, g, b);
}

void cxDiffuseSH::combine(const cxDiffuseSH& other, const float wghtThis, const float wghtOther) {
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) {
		mCoefsR[i] = mCoefsR[i]*wghtThis + other.mCoefsR[i]*wghtOther;
	}
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) {
		mCoefsG[i] = mCoefsG[i]*wghtThis + other.mCoefsG[i]*wghtOther;
	}
	for (int i = 0; i < XD_DIFFSH_NUMCOEFFS; ++i) {
		mCoefsB[i] = mCoefsB[i]*wghtThis + other.mCoefsB[i]*wghtOther;
	}
}

cxVec cxDiffuseSH::extract_dominant_dir() const {
	return nxSH::extract_dominant_dir_rgb(mCoefsR, mCoefsG, mCoefsB);
}

void cxDiffuseSH::apply_to_hemi(sxHemisphereLight* pHemi, const float scale) const {
	if (!pHemi) return;
	cxVec up = extract_dominant_dir();
	cxColor upper = eval(up, scale);
	cxColor lower = eval(up.neg_val(), scale);
	pHemi->reset();
	pHemi->set_upvec(up);
	pHemi->mUpper.set(upper.r, upper.g, upper.b);
	pHemi->mLower.set(lower.r, lower.g, lower.b);
}


namespace nxDataUtil {

exAnimChan anim_chan_from_str(const char* pStr) {
	exAnimChan res = exAnimChan::UNKNOWN;
	static struct {
		const char* pName;
		exAnimChan id;
	} tbl[] = {
		{ "tx", exAnimChan::TX },
		{ "ty", exAnimChan::TY },
		{ "tz", exAnimChan::TZ },
		{ "rx", exAnimChan::RX },
		{ "ry", exAnimChan::RY },
		{ "rz", exAnimChan::RZ },
		{ "sx", exAnimChan::SX },
		{ "sy", exAnimChan::SY },
		{ "sz", exAnimChan::SZ },
	};
	if (pStr) {
		for (size_t i = 0; i < XD_ARY_LEN(tbl); ++i) {
			if (nxCore::str_eq(pStr, tbl[i].pName)) {
				res = tbl[i].id;
				break;
			}
		}
	}
	return res;

}
const char* anim_chan_to_str(exAnimChan chan) {
	const char* pName = "<unknown>";
	switch (chan) {
		case exAnimChan::TX: pName = "tx"; break;
		case exAnimChan::TY: pName = "ty"; break;
		case exAnimChan::TZ: pName = "tz"; break;
		case exAnimChan::RX: pName = "rx"; break;
		case exAnimChan::RY: pName = "ry"; break;
		case exAnimChan::RZ: pName = "rz"; break;
		case exAnimChan::SX: pName = "sx"; break;
		case exAnimChan::SY: pName = "sy"; break;
		case exAnimChan::SZ: pName = "sz"; break;
		default: break;
	}
	return pName;
}

exRotOrd rot_ord_from_str(const char* pStr) {
	exRotOrd rord = exRotOrd::XYZ;
	static struct {
		const char* pName;
		exRotOrd ord;
	} tbl[] = {
		{ "xyz", exRotOrd::XYZ },
		{ "xzy", exRotOrd::XZY },
		{ "yxz", exRotOrd::YXZ },
		{ "yzx", exRotOrd::YZX },
		{ "zxy", exRotOrd::ZXY },
		{ "zyx", exRotOrd::ZYX }
	};
	if (pStr) {
		for (size_t i = 0; i < XD_ARY_LEN(tbl); ++i) {
			if (nxCore::str_eq(pStr, tbl[i].pName)) {
				rord = tbl[i].ord;
				break;
			}
		}
	}
	return rord;
}

const char* rot_ord_to_str(exRotOrd rord) {
	const char* pStr;
	switch (rord) {
		case exRotOrd::XYZ: pStr = "xyz"; break;
		case exRotOrd::XZY: pStr = "xzy"; break;
		case exRotOrd::YXZ: pStr = "yxz"; break;
		case exRotOrd::YZX: pStr = "yzx"; break;
		case exRotOrd::ZXY: pStr = "zxy"; break;
		case exRotOrd::ZYX: pStr = "zyx"; break;
		default: pStr = "xyz"; break;
	}
	return pStr;
}

exTransformOrd xform_ord_from_str(const char* pStr) {
	exTransformOrd xord = exTransformOrd::SRT;
	static struct {
		const char* pName;
		exTransformOrd ord;
	} tbl[] = {
		{ "srt", exTransformOrd::SRT },
		{ "str", exTransformOrd::STR },
		{ "rst", exTransformOrd::RST },
		{ "rts", exTransformOrd::RTS },
		{ "tsr", exTransformOrd::TSR },
		{ "trs", exTransformOrd::TRS }
	};
	if (pStr) {
		for (size_t i = 0; i < XD_ARY_LEN(tbl); ++i) {
			if (nxCore::str_eq(pStr, tbl[i].pName)) {
				xord = tbl[i].ord;
				break;
			}
		}
	}
	return xord;
}

const char* xform_ord_to_str(exTransformOrd xord) {
	const char* pStr;
	switch (xord) {
		case exTransformOrd::SRT: pStr = "srt"; break;
		case exTransformOrd::STR: pStr = "str"; break;
		case exTransformOrd::RST: pStr = "rst"; break;
		case exTransformOrd::RTS: pStr = "rts"; break;
		case exTransformOrd::TSR: pStr = "tsr"; break;
		case exTransformOrd::TRS: pStr = "trs"; break;
		default: pStr = "srt"; break;
	}
	return pStr;
}

} // nxDataUtil

namespace nxData {

XD_NOINLINE sxData* load(const char* pPath) {
	size_t size = 0;
	sxData* pData = reinterpret_cast<sxData*>(nxCore::bin_load_impl(pPath, &size, true, true, true, s_pXDataMemTag));
	if (pData) {
		if (pData->mFileSize == size) {
			pData->mFilePathLen = (uint32_t)nxCore::str_len(pPath);
		} else {
			unload(pData);
			pData = nullptr;
		}
	}
	return pData;
}

XD_NOINLINE void unload(sxData* pData) {
	nxCore::bin_unload(pData);
}

static int pk_find_dict_idx(const uint32_t* pCnts, int n, uint32_t cnt) {
	const uint32_t* p = pCnts;
	uint32_t c = (uint32_t)n;
	while (c > 1) {
		uint32_t mid = c / 2;
		const uint32_t* pm = &p[mid];
		uint32_t ck = *pm;
		p = (cnt > ck) ? p : pm;
		c -= mid;
	}
	return (int)(p - pCnts) + ((p != pCnts) & (*p < cnt));
}

static uint8_t pk_get_bit_cnt(uint8_t x) {
	for (uint8_t i = 8; --i > 0;) {
		if (x & (1 << i)) return i + 1;
	}
	return 1;
}

static uint32_t pk_bit_cnt_to_bytes(uint32_t n) {
	return (n >> 3) + ((n & 7) != 0);
}

static uint32_t pk_mk_dict(uint8_t* pDict, uint8_t* pXlat, const uint8_t* pSrc, uint32_t srcSize) {
	uint32_t cnt[0x100];
	nxCore::mem_zero(cnt, sizeof(cnt));
	for (uint32_t i = 0; i < srcSize; ++i) {
		++cnt[pSrc[i]];
	}
	uint8_t idx[0x100];
	uint32_t idict = 0;
	for (uint32_t i = 0; i < 0x100; ++i) {
		if (cnt[i]) {
			idx[idict] = i;
			++idict;
		}
	}
	nxCore::mem_zero(pDict, 0x100);
	uint32_t dictSize = idict;
	uint32_t dictCnt[0x100];
	for (uint32_t i = 0; i < dictSize; ++i) {
		uint8_t id = idx[i];
		uint32_t c = cnt[id];
		if (i == 0) {
			pDict[i] = id;
			dictCnt[i] = c;
		} else {
			int idx = pk_find_dict_idx(dictCnt, i, c);
			if (idx == int(i - 1)) {
				if (c > dictCnt[idx]) {
					pDict[i] = pDict[i - 1];
					dictCnt[i] = dictCnt[i - 1];
					pDict[i - 1] = id;
					dictCnt[i - 1] = c;
				} else {
					pDict[i] = id;
					dictCnt[i] = c;
				}
			} else {
				if (c > dictCnt[idx]) --idx;
				for (int j = i; --j >= idx + 1;) {
					pDict[j + 1] = pDict[j];
					dictCnt[j + 1] = dictCnt[j];
				}
				pDict[idx + 1] = id;
				dictCnt[idx + 1] = c;
			}
		}
	}
	nxCore::mem_zero(pXlat, 0x100);
	for (uint32_t i = 0; i < dictSize; ++i) {
		pXlat[pDict[i]] = i;
	}
	return dictSize;
}

static uint32_t pk_encode(uint8_t* pBitCnt, uint8_t* pBitCode, const uint8_t* pXlat, const uint8_t* pSrc, uint32_t srcSize) {
	uint32_t bitDst = 0;
	for (uint32_t i = 0; i < srcSize; ++i) {
		uint8_t code = pXlat[pSrc[i]];
		uint8_t nbits = pk_get_bit_cnt(code);
		uint8_t bitCode = nbits - 1;
		uint32_t cntIdx = i * 3;
		uint32_t byteIdx = cntIdx >> 3;
		uint32_t bitIdx = cntIdx & 7;
		pBitCnt[byteIdx] |= bitCode << bitIdx;
		pBitCnt[byteIdx + 1] |= bitCode >> (8 - bitIdx);

		uint8_t dstBits = nbits;
		if (nbits > 1) {
			code -= 1 << (nbits - 1);
			--dstBits;
		}

		byteIdx = bitDst >> 3;
		bitIdx = bitDst & 7;
		pBitCode[byteIdx] |= code << bitIdx;
		pBitCode[byteIdx + 1] |= code >> (8 - bitIdx);
		bitDst += dstBits;
	}
	return bitDst;
}

static uint32_t backref_encode(
	uint8_t* pDst, const uint32_t dstSize,
	const uint8_t* pSrc, const uint32_t srcSize,
	uint32_t* pTbl, const uint32_t tblBits
) {
	if (!pTbl || tblBits > 23) return 0;
	uint32_t tblSize = 1 << tblBits;
	uint32_t tblMask = tblSize - 1;
	int sh = nxCalc::max((24 - int(tblBits)), 1);
	for (uint32_t i = 0; i < tblSize; ++i) {
		pTbl[i] = 0;
	}
	const uint32_t storLim = 1 << 5;
	uint32_t nstor = 0;
	uint32_t isrc = 0;
	uint32_t idst = 0;
	uint32_t h = pSrc[0] << 8;
	h |= pSrc[1];
	pDst[idst++] = 0;
	while (true) {
		if (isrc >= srcSize - 2) break;
		h <<= 8;
		h |= pSrc[isrc + 2];
		uint32_t idx = h >> sh;
		if (sh >= 8) {
			idx -= h;
		} else {
			idx += h;
		}
		idx &= tblMask;
		uint32_t istor = pTbl[idx];
		pTbl[idx] = isrc;
		bool flg = (istor > 0) && (istor < isrc) && (isrc < srcSize - 4);
		uint32_t offs = 0;
		if (flg) {
			offs = isrc - (istor + 1);
			flg = offs < (1 << 13);
		}
		if (flg) {
			flg = nxCore::mem_eq(&pSrc[isrc], &pSrc[istor], 3);
		}
		if (flg) {
			if (nstor > 0) {
				pDst[idst - (nstor + 1)] = nstor - 1;
			} else {
				--idst;
			}
			uint32_t iorg = 3;
			uint32_t iend = nxCalc::min(srcSize - isrc - 2, 0x108U);
			uint32_t rlen = 1;
			for (uint32_t i = iorg; i < iend; ++i) {
				if (pSrc[isrc + i] != pSrc[istor + i]) break;
				++rlen;
			}
			if (rlen < 7) {
				pDst[idst++] = uint8_t(((offs >> 8) & 0x1F) | (rlen << 5));
				pDst[idst++] = uint8_t(offs & 0xFF);
			} else {
				pDst[idst++] = uint8_t(((offs >> 8) & 0x1F) | (7 << 5));
				pDst[idst++] = uint8_t(offs & 0xFF);
				pDst[idst++] = uint8_t((rlen - 7) & 0xFF);
			}
			nstor = 0;
			++idst;
			isrc += rlen + 1;
			if (isrc >= srcSize - 3) {
				++isrc;
				break;
			}
			h = pSrc[isrc];
			h <<= 8;
			h |= pSrc[isrc + 1];
			h <<= 8;
			h |= pSrc[isrc + 2];
			idx = h >> sh;
			if (sh >= 8) {
				idx -= h;
			} else {
				idx += h;
			}
			idx &= tblMask;
			pTbl[idx] = isrc;
			++isrc;
		} else {
			if (idst < dstSize && idst < srcSize) {
				pDst[idst++] = pSrc[isrc++];
				++nstor;
				if (nstor >= storLim) {
					pDst[idst - (nstor + 1)] = nstor - 1;
					nstor = 0;
					++idst;
				}
			} else {
				return 0;
			}
		}
	}
	if (idst > dstSize - 3) {
		return 0;
	}
	while (isrc < srcSize) {
		pDst[idst++] = pSrc[isrc++];
		++nstor;
		if (nstor >= storLim) {
			pDst[idst - (nstor + 1)] = nstor - 1;
			nstor = 0;
			++idst;
		}
	}
	if (nstor > 0) {
		pDst[idst - (nstor + 1)] = nstor - 1;
	} else {
		--idst;
	}
	return idst;
}

static uint32_t backref_decode(
	uint8_t* pDst, const uint32_t dstSize,
	const uint8_t* pSrc, const uint32_t srcSize
) {
	uint32_t isrc = 0;
	uint32_t idst = 0;
	while (true) {
		if (isrc >= srcSize) break;
		uint32_t dat = pSrc[isrc++];
		uint32_t len = dat & 0x1F;
		if (dat == len) {
			++len;
			if (idst + len > dstSize) return 0;
			for (int32_t i = int32_t(len); --i >= 0;) {
				pDst[idst++] = pSrc[isrc++];
			}
		} else {
			uint32_t offs = (len << 8);
			offs += pSrc[isrc++];
			len = dat >> 5;
			if (len == 7) {
				len += pSrc[isrc++];
			}
			len += 2;
			if (idst + len >= dstSize) return 0;
			uint32_t iref = idst - offs - 1;
			for (int32_t i = int32_t(len); --i >= 0;) {
				pDst[idst++] = pDst[iref++];
			}
		}
	}
	return idst;
}

struct sxPkdWork {
	uint8_t mDict[0x100];
	uint8_t mXlat[0x100];
	uint32_t mSrcSize;
	uint32_t mDictSize;
	uint32_t mBitCntBytes;
	uint8_t* mpBitCnt;
	uint8_t* mpBitCode;
	uint32_t mBitCodeBits;
	uint32_t mBitCodeBytes;

	sxPkdWork() : mpBitCnt(nullptr), mpBitCode(nullptr) {}
	~sxPkdWork() { reset(); }

	void reset() {
		if (mpBitCnt) {
			nxCore::mem_free(mpBitCnt);
			mpBitCnt = nullptr;
		}
		if (mpBitCode) {
			nxCore::mem_free(mpBitCode);
			mpBitCode = nullptr;
		}
	}

	bool is_valid() const { return (mpBitCnt && mpBitCode); }

	bool encode(const uint8_t* pSrc, uint32_t srcSize) {
		bool res = false;
		mSrcSize = srcSize;
		mDictSize = pk_mk_dict(mDict, mXlat, pSrc, srcSize);
		mBitCntBytes = pk_bit_cnt_to_bytes(srcSize * 3);
		mpBitCnt = (uint8_t*)nxCore::mem_alloc(mBitCntBytes + 1, "xPkd:EncCnt");
		if (mpBitCnt) {
			nxCore::mem_zero(mpBitCnt, mBitCntBytes);
			mpBitCode = (uint8_t*)nxCore::mem_alloc(srcSize, "xPkd:EncCode");
			if (mpBitCode) {
				nxCore::mem_zero(mpBitCode, srcSize);
				mBitCodeBits = pk_encode(mpBitCnt, mpBitCode, mXlat, pSrc, srcSize);
				mBitCodeBytes = pk_bit_cnt_to_bytes(mBitCodeBits);
				res = true;
			} else {
				nxCore::mem_free(mpBitCnt);
				mpBitCnt = nullptr;
			}
		}
		return res;
	}

	uint32_t get_pkd_size() const {
		return is_valid() ? sizeof(sxPackedData) + mDictSize + mBitCntBytes + mBitCodeBytes : 0;
	}
};

static sxPackedData* pkd0(const sxPkdWork& wk) {
	sxPackedData* pPkd = nullptr;
	if (wk.is_valid()) {
		uint32_t size = wk.get_pkd_size();
		pPkd = (sxPackedData*)nxCore::mem_alloc(size + 1, "xPkd0");
		if (pPkd) {
			pPkd->mSig = sxPackedData::SIG;
			pPkd->mAttr = 0 | ((wk.mDictSize - 1) << 8);
			pPkd->mPackSize = size;
			pPkd->mRawSize = wk.mSrcSize;
			uint8_t* pDict = (uint8_t*)(pPkd + 1);
			uint8_t* pCnt = pDict + wk.mDictSize;
			uint8_t* pCode = pCnt + wk.mBitCntBytes;
			nxCore::mem_copy(pDict, wk.mDict, wk.mDictSize);
			nxCore::mem_copy(pCnt, wk.mpBitCnt, wk.mBitCntBytes);
			nxCore::mem_copy(pCode, wk.mpBitCode, wk.mBitCodeBytes);
		}
	}
	return pPkd;
}

static sxPackedData* pkd1(const sxPkdWork& wk, const sxPkdWork& wk2) {
	sxPackedData* pPkd = nullptr;
	if (wk.is_valid() && wk2.is_valid()) {
		uint32_t size = sizeof(sxPackedData) + 4 + wk.mDictSize + wk2.mDictSize + wk2.mBitCntBytes + wk2.mBitCodeBytes + wk.mBitCodeBytes;
		pPkd = (sxPackedData*)nxCore::mem_alloc(size + 1, "xPkd1");
		if (pPkd) {
			pPkd->mSig = sxPackedData::SIG;
			pPkd->mAttr = 1 | ((wk.mDictSize - 1) << 8) | ((wk2.mDictSize - 1) << 16);
			pPkd->mPackSize = size;
			pPkd->mRawSize = wk.mSrcSize;
			uint32_t* pCntSize = (uint32_t*)(pPkd + 1);
			uint8_t* pDict = (uint8_t*)(pPkd + 1) + 4;
			uint8_t* pDict2 = pDict + wk.mDictSize;
			uint8_t* pCnt2 = pDict2 + wk2.mDictSize;
			uint8_t* pCode2 = pCnt2 + wk2.mBitCntBytes;
			uint8_t* pCode = pCode2 + wk2.mBitCodeBytes;
			*pCntSize = wk2.mBitCodeBytes;
			nxCore::mem_copy(pDict, wk.mDict, wk.mDictSize);
			nxCore::mem_copy(pDict2, wk2.mDict, wk2.mDictSize);
			nxCore::mem_copy(pCnt2, wk2.mpBitCnt, wk2.mBitCntBytes);
			nxCore::mem_copy(pCode2, wk2.mpBitCode, wk2.mBitCodeBytes);
			nxCore::mem_copy(pCode, wk.mpBitCode, wk.mBitCodeBytes);
		}
	}
	return pPkd;
}

sxPackedData* pack(const uint8_t* pSrc, const uint32_t srcSize, const uint32_t mode) {
	sxPackedData* pPkd = nullptr;
	if (pSrc && srcSize > 0x10) {
		if (mode < 2) {
			sxPkdWork wk;
			wk.encode(pSrc, srcSize);
			uint32_t pkdSize = wk.get_pkd_size();
			if (pkdSize && pkdSize < srcSize) {
				if (mode == 1) {
					sxPkdWork wk2;
					wk2.encode(wk.mpBitCnt, wk.mBitCntBytes);
					uint32_t pkdSize2 = wk2.is_valid() ? wk2.get_pkd_size() + 4 + wk.mDictSize + wk.mBitCodeBytes : 0;
					if (pkdSize2 && pkdSize2 < pkdSize) {
						pPkd = pkd1(wk, wk2);
					} else {
						wk2.reset();
						pPkd = pkd0(wk);
					}
				} else {
					pPkd = pkd0(wk);
				}
			}
		} else if (mode == 2) {
			uint32_t tblBits = 18;
			uint32_t tblSize = uint32_t(size_t(1 << tblBits) * sizeof(uint32_t));
			size_t tmemSize = size_t(double(srcSize) * 2.25);
			uint8_t* pBref = (uint8_t*)nxCore::mem_alloc(tblSize + tmemSize, "xPkd:BackRef");
			if (pBref) {
				uint32_t* pTbl = (uint32_t*)pBref;
				uint8_t* pDst = pBref + tblSize;
				uint32_t brefSize = backref_encode(pDst, (uint32_t)tmemSize, pSrc, srcSize, pTbl, tblBits);
				if (brefSize != 0 && brefSize < srcSize) {
					sxPackedData* pTmpPkd = pack(pDst, brefSize, 0);
					if (pTmpPkd) {
						pPkd = (sxPackedData*)nxCore::mem_alloc(pTmpPkd->mPackSize + sizeof(uint32_t), "xPkd:BackPack");
						if (pPkd) {
							nxCore::mem_copy(pPkd, pTmpPkd, sizeof(sxPackedData));
							*((uint32_t*)(pPkd + 1)) = brefSize;
							nxCore::mem_copy(XD_INCR_PTR((pPkd + 1), sizeof(uint32_t)), pTmpPkd + 1, size_t(pTmpPkd->mPackSize) - sizeof(sxPackedData));
							pPkd->mAttr &= 0xFFFFFF00;
							pPkd->mAttr |= 2;
							pPkd->mPackSize += uint32_t(sizeof(uint32_t));
							pPkd->mRawSize = srcSize;
						}
						nxCore::mem_free(pTmpPkd);
					}
				}
				nxCore::mem_free(pBref);
			}
		}
	}
	return pPkd;
}

static void pk_decode(uint8_t* pDst, const uint32_t size, uint8_t* pDict, uint8_t* pBitCnt, uint8_t* pBitCodes) {
	uint32_t codeBitIdx = 0;
	for (uint32_t i = 0; i < size; ++i) {
		uint32_t cntBitIdx = i * 3;
		uint32_t byteIdx = cntBitIdx >> 3;
		uint32_t bitIdx = cntBitIdx & 7;
		uint8_t nbits = pBitCnt[byteIdx] >> bitIdx;
		nbits |= pBitCnt[byteIdx + 1] << (8 - bitIdx);
		nbits &= 7;
		nbits += 1;

		uint8_t codeBits = nbits;
		if (nbits > 1) {
			--codeBits;
		}
		byteIdx = codeBitIdx >> 3;
		bitIdx = codeBitIdx & 7;
		uint8_t code = pBitCodes[byteIdx] >> bitIdx;
		code |= pBitCodes[byteIdx + (bitIdx != 0 ? 1 : 0)] << (8 - bitIdx);
		code &= (uint8_t)(((uint32_t)1 << codeBits) - 1);
		codeBitIdx += codeBits;
		if (nbits > 1) {
			code += 1 << (nbits - 1);
		}
		pDst[i] = pDict[code];
	}
}

uint8_t* unpack(sxPackedData* pPkd, const char* pMemTag, uint8_t* pDstMem, const uint32_t dstMemSize, size_t* pSize, const bool recursive) {
	uint8_t* pDst = nullptr;
	if (pPkd && pPkd->mSig == sxPackedData::SIG) {
		if (pDstMem && dstMemSize >= pPkd->mRawSize) {
			pDst = pDstMem;
		} else {
			pDst = (uint8_t*)nxCore::mem_alloc(pPkd->mRawSize, pMemTag);
		}
		if (pDst) {
			int mode = pPkd->get_mode();
			if (mode == 0) {
				uint32_t dictSize = ((pPkd->mAttr >> 8) & 0xFF) + 1;
				uint8_t* pDict = (uint8_t*)(pPkd + 1);
				uint8_t* pBitCnt = pDict + dictSize;
				uint8_t* pBitCodes = pBitCnt + pk_bit_cnt_to_bytes(pPkd->mRawSize * 3);
				pk_decode(pDst, pPkd->mRawSize, pDict, pBitCnt, pBitCodes);
			} else if (mode == 1) {
				uint32_t dictSize = ((pPkd->mAttr >> 8) & 0xFF) + 1;
				uint32_t dictSize2 = ((pPkd->mAttr >> 16) & 0xFF) + 1;
				uint32_t* pCode2Size = (uint32_t*)(pPkd + 1);
				uint8_t* pDict = (uint8_t*)(pCode2Size + 1);
				uint8_t* pDict2 = pDict + dictSize;
				uint8_t* pCnt2 = pDict2 + dictSize2;
				uint32_t cntSize = pk_bit_cnt_to_bytes(pPkd->mRawSize * 3);
				uint8_t* pCode2 = pCnt2 + pk_bit_cnt_to_bytes(cntSize * 3);
				uint8_t* pCode = pCode2 + *pCode2Size;
				uint8_t* pCnt = (uint8_t*)nxCore::mem_alloc(cntSize, "xPkd:Cnt");
				if (pCnt) {
					pk_decode(pCnt, cntSize, pDict2, pCnt2, pCode2);
					pk_decode(pDst, pPkd->mRawSize, pDict, pCnt, pCode);
					nxCore::mem_free(pCnt);
				} else {
					if (pDst != pDstMem) {
						nxCore::mem_free(pDst);
					}
					pDst = nullptr;
				}
			} else if (mode == 2) {
				uint32_t dictSize = ((pPkd->mAttr >> 8) & 0xFF) + 1;
				uint8_t* pDict = (uint8_t*)(pPkd + 1) + sizeof(uint32_t);
				uint8_t* pBitCnt = pDict + dictSize;
				uint32_t brefSize = *((uint32_t*)(pPkd + 1));
				uint8_t* pBitCodes = pBitCnt + pk_bit_cnt_to_bytes(brefSize * 3);
				uint8_t* pBref = (uint8_t*)nxCore::mem_alloc(brefSize, "xPkd:BackRef");
				if (pBref) {
					pk_decode(pBref, brefSize, pDict, pBitCnt, pBitCodes);
					uint32_t decodedSize = backref_decode(pDst, pPkd->mRawSize, pBref, brefSize);
					if (decodedSize != pPkd->mRawSize) {
						if (pDst != pDstMem) {
							nxCore::mem_free(pDst);
						}
						pDst = nullptr;
					}
					nxCore::mem_free(pBref);
				} else {
					if (pDst != pDstMem) {
						nxCore::mem_free(pDst);
					}
					pDst = nullptr;
				}
			}
			if (pSize) {
				*pSize = pDst ? pPkd->mRawSize : 0;
			}
		}
		if (pDst && !pDstMem && recursive) {
			sxPackedData* pRecPkd = (sxPackedData*)pDst;
			if (pRecPkd->mSig == sxPackedData::SIG) {
				size_t recSize = 0;
				uint8_t* pRecDst = unpack(pRecPkd, pMemTag, nullptr, 0, &recSize, true);
				if (pRecDst) {
					nxCore::mem_free(pDst);
					pDst = pRecDst;
				}
				if (pSize) {
					*pSize = recSize;
				}
			}
		}
	}
	return pDst;
}

} // nxData


static int find_str_hash_idx(const uint16_t* pHash, int n, uint16_t h) {
	const uint16_t* p = pHash;
	uint32_t cnt = (uint32_t)n;
	while (cnt > 1) {
		uint32_t mid = cnt / 2;
		const uint16_t* pm = &p[mid];
		uint16_t ck = *pm;
		p = (h < ck) ? p : pm;
		cnt -= mid;
	}
	return (int)(p - pHash) + ((p != pHash) & (*p > h));
}

bool sxStrList::is_sorted() const {
	uint8_t flags = ((uint8_t*)this + mSize)[-1];
	return (flags != 0) && ((flags & 1) != 0);
}

XD_NOINLINE int sxStrList::find_str(const char* pStr) const {
	if (!pStr) return -1;
	uint16_t h = nxCore::str_hash16(pStr);
	uint16_t* pHash = get_hash_top();
	if (is_sorted()) {
		int idx = find_str_hash_idx(pHash, mNum, h);
		if (h == pHash[idx]) {
			int nck = 1;
			for (int i = idx; --i >= 0;) {
				if (pHash[i] != h) break;
				--idx;
				++nck;
			}
			for (int i = 0; i < nck; ++i) {
				int strIdx = idx + i;
				if (nxCore::str_eq(get_str(strIdx), pStr)) {
					return strIdx;
				}
			}
		}
	} else {
		for (uint32_t i = 0; i < mNum; ++i) {
			if (h == pHash[i]) {
				if (nxCore::str_eq(get_str(i), pStr)) {
					return i;
				}
			}
		}
	}
	return -1;
}

XD_NOINLINE int sxStrList::find_str_any(const char** ppStrs, int n) const {
	if (!ppStrs || n < 1) return -1;
	int org;
	uint16_t hblk[8];
	int bsize = XD_ARY_LEN(hblk);
	int nblk = n / bsize;
	uint16_t* pHash = get_hash_top();
	for (int iblk = 0; iblk < nblk; ++iblk) {
		org = iblk * bsize;
		for (int j = 0; j < bsize; ++j) {
			hblk[j] = nxCore::str_hash16(ppStrs[org + j]);
		}
		for (uint32_t i = 0; i < mNum; ++i) {
			uint16_t htst = pHash[i];
			const char* pStr = get_str(i);
			for (int j = 0; j < bsize; ++j) {
				if (htst == hblk[j]) {
					if (nxCore::str_eq(pStr, ppStrs[org + j])) {
						return i;
					}
				}
			}
		}
	}
	org = nblk * bsize;
	for (int j = org; j < n; ++j) {
		hblk[j - org] = nxCore::str_hash16(ppStrs[j]);
	}
	for (uint32_t i = 0; i < mNum; ++i) {
		uint16_t htst = pHash[i];
		const char* pStr = get_str(i);
		for (int j = org; j < n; ++j) {
			if (htst == hblk[j - org]) {
				if (nxCore::str_eq(pStr, ppStrs[j])) {
					return i;
				}
			}
		}
	}
	return -1;
}


int sxVecList::get_elems(float* pDst, int idx, int num) const {
	int n = 0;
	if (ck_idx(idx)) {
		const float* pVal = get_ptr(idx);
		const float* pLim = reinterpret_cast<const float*>(XD_INCR_PTR(this, mSize));
		for (int i = 0; i < num; ++i) {
			if (pVal >= pLim) break;
			if (pDst) {
				pDst[i] = *pVal;
			}
			++pVal;
			++n;
		}
	}
	return n;
}

xt_float2 sxVecList::get_f2(int idx) {
	xt_float2 v;
	v.fill(0.0f);
	get_elems(v, idx, 2);
	return v;
}

xt_float3 sxVecList::get_f3(int idx) {
	xt_float3 v;
	v.fill(0.0f);
	get_elems(v, idx, 3);
	return v;
}

xt_float4 sxVecList::get_f4(int idx) {
	xt_float4 v;
	v.fill(0.0f);
	get_elems(v, idx, 4);
	return v;
}


uint32_t sxData::find_ext_offs(const uint32_t kind) const {
	uint32_t offs = 0;
	const ExtList* pLst = get_ext_list();
	if (pLst) {
		for (uint32_t i = 0; i < pLst->num; ++i) {
			if (kind == pLst->lst[i].kind) {
				offs = pLst->lst[i].offs;
				break;
			}
		}
	}
	return offs;
}

sxData::Status sxData::get_status() const {
	Status s;
	struct {
		uint32_t ccSys;
		uint32_t ccLE;
		uint32_t ccBE;
	} flst[] = {
		{ sxValuesData::KIND, 0, 0 },
		{ sxRigData::KIND, 0, 0 },
		{ sxGeometryData::KIND, 0, 0 },
		{ sxImageData::KIND, 0, 0 },
		{ sxKeyframesData::KIND, 0, 0 },
		{ sxModelData::KIND, 0, 0 },
		{ sxTextureData::KIND, 0, 0 },
		{ sxMotionData::KIND, 0, 0 },
		{ sxCollisionData::KIND, 0, 0 },
		{ sxFileCatalogue::KIND, 0, 0}
	};
	s.fmt = 0;
	s.native = 0;
	s.bige = 0;
	bool sysLE = nxSys::is_LE();
	int nfmt = XD_ARY_LEN(flst);
	for (int i = 0; i < nfmt; ++i) {
		if (sysLE) {
			flst[i].ccLE = flst[i].ccSys;
			flst[i].ccBE = nxSys::bswap_u32(flst[i].ccLE);
		} else {
			flst[i].ccBE = flst[i].ccSys;
			flst[i].ccLE = nxSys::bswap_u32(flst[i].ccBE);
		}
	}
	for (int i = 0; i < nfmt; ++i) {
		if (nxCore::mem_eq(&mKind, &flst[i].ccLE, sizeof(uint32_t))) {
			s.fmt = 1;
			s.native = sysLE ? 1 : 0;
			s.bige = 0;
			break;
		} else if (nxCore::mem_eq(&mKind, &flst[i].ccBE, sizeof(uint32_t))) {
			s.fmt = 1;
			s.native = sysLE ? 0 : 1;
			s.bige = 1;
			break;
		}
	}
	s.fixed = mFilePathLen != 0;
	return s;
}


int sxValuesData::Group::find_val_idx(const char* pName) const {
	int idx = -1;
	if (pName && is_valid()) {
		sxStrList* pStrLst = mpVals->get_str_list();
		if (pStrLst) {
			int nameId = pStrLst->find_str(pName);
			if (nameId >= 0) {
				const GrpInfo* pInfo = get_info();
				const ValInfo* pVal = pInfo->mVals;
				int nval = pInfo->mValNum;
				for (int i = 0; i < nval; ++i) {
					if (pVal[i].mNameId == nameId) {
						idx = i;
						break;
					}
				}
			}
		}
	}
	return idx;
}

int sxValuesData::Group::find_val_idx_any(const char** ppNames, int n) const {
	int idx = -1;
	if (ppNames && n > 0 && is_valid()) {
		sxStrList* pStrLst = mpVals->get_str_list();
		if (pStrLst) {
			for (int i = 0; i < n; ++i) {
				const char* pName = ppNames[i];
				int tst = find_val_idx(pName);
				if (ck_val_idx(tst)) {
					idx = tst;
					break;
				}
			}
		}
	}
	return idx;
}

int sxValuesData::Group::get_val_i(int idx) const {
	int res = 0;
	const ValInfo* pVal = get_val_info(idx);
	if (pVal) {
		float ftmp[1] = { 0.0f };
		eValType typ = pVal->get_type();
		switch (typ) {
			case eValType::INT:
				res = pVal->mValId.i;
				break;
			case eValType::FLOAT:
				res = (int)pVal->mValId.f;
				break;
			case eValType::VEC2:
			case eValType::VEC3:
			case eValType::VEC4:
				mpVals->get_vec_list()->get_elems(ftmp, pVal->mValId.i, 1);
				res = (int)ftmp[0];
				break;
			default:
				break;
		}
	}
	return res;
}

float sxValuesData::Group::get_val_f(int idx) const {
	float res = 0.0f;
	const ValInfo* pVal = get_val_info(idx);
	if (pVal) {
		eValType typ = pVal->get_type();
		switch (typ) {
			case eValType::INT:
				res = (float)pVal->mValId.i;
				break;
			case eValType::FLOAT:
				res = pVal->mValId.f;
				break;
			case eValType::VEC2:
			case eValType::VEC3:
			case eValType::VEC4:
				mpVals->get_vec_list()->get_elems(&res, pVal->mValId.i, 1);
				break;
			default:
				break;
		}
	}
	return res;
}

xt_float2 sxValuesData::Group::get_val_f2(int idx) const {
	xt_float2 res;
	res.fill(0.0f);
	const ValInfo* pVal = get_val_info(idx);
	if (pVal) {
		eValType typ = pVal->get_type();
		switch (typ) {
			case eValType::INT:
				res.fill((float)pVal->mValId.i);
				break;
			case eValType::FLOAT:
				res.fill(pVal->mValId.f);
				break;
			case eValType::VEC2:
			case eValType::VEC3:
			case eValType::VEC4:
				res = mpVals->get_vec_list()->get_f2(pVal->mValId.i);
				break;
			default:
				break;
		}
	}
	return res;
}

xt_float3 sxValuesData::Group::get_val_f3(int idx) const {
	xt_float3 res;
	res.fill(0.0f);
	const ValInfo* pVal = get_val_info(idx);
	if (pVal) {
		eValType typ = pVal->get_type();
		switch (typ) {
			case eValType::INT:
				res.fill((float)pVal->mValId.i);
				break;
			case eValType::FLOAT:
				res.fill(pVal->mValId.f);
				break;
			case eValType::VEC2:
			case eValType::VEC3:
			case eValType::VEC4:
				res = mpVals->get_vec_list()->get_f3(pVal->mValId.i);
				break;
			default:
				break;
		}
	}
	return res;
}

xt_float4 sxValuesData::Group::get_val_f4(int idx) const {
	xt_float4 res;
	res.fill(0.0f);
	const ValInfo* pVal = get_val_info(idx);
	if (pVal) {
		eValType typ = pVal->get_type();
		switch (typ) {
			case eValType::INT:
				res.fill((float)pVal->mValId.i);
				break;
			case eValType::FLOAT:
				res.fill(pVal->mValId.f);
				break;
			case eValType::VEC2:
			case eValType::VEC3:
			case eValType::VEC4:
				res = mpVals->get_vec_list()->get_f4(pVal->mValId.i);
				break;
			default:
				break;
		}
	}
	return res;
}

const char* sxValuesData::Group::get_val_s(int idx) const {
	const char* pStr = nullptr;
	const ValInfo* pVal = get_val_info(idx);
	if (pVal) {
		sxStrList* pStrLst = mpVals->get_str_list();
		if (pStrLst) {
			if (pVal->get_type() == eValType::STRING) {
				pStr = pStrLst->get_str(pVal->mValId.i);
			}
		}
	}
	return pStr;
}

float sxValuesData::Group::get_float(const char* pName, const float defVal) const {
	float f = defVal;
	int idx = find_val_idx(pName);
	if (idx >= 0) {
		f = get_val_f(idx);
	}
	return f;
}

float sxValuesData::Group::get_float_any(const char** ppNames, int n, const float defVal) const {
	float f = defVal;
	int idx = find_val_idx_any(ppNames, n);
	if (idx >= 0) {
		f = get_val_f(idx);
	}
	return f;
}

int sxValuesData::Group::get_int(const char* pName, const int defVal) const {
	int i = defVal;
	int idx = find_val_idx(pName);
	if (idx >= 0) {
		i = get_val_i(idx);
	}
	return i;
}

int sxValuesData::Group::get_int_any(const char** ppNames, int n, const int defVal) const {
	int i = defVal;
	int idx = find_val_idx_any(ppNames, n);
	if (idx >= 0) {
		i = get_val_i(idx);
	}
	return i;
}

cxVec sxValuesData::Group::get_vec(const char* pName, const cxVec& defVal) const {
	cxVec v = defVal;
	int idx = find_val_idx(pName);
	if (idx >= 0) {
		xt_float3 f3 = get_val_f3(idx);
		v.set(f3.x, f3.y, f3.z);
	}
	return v;
}

cxVec sxValuesData::Group::get_vec_any(const char** ppNames, int n, const cxVec& defVal) const {
	cxVec v = defVal;
	int idx = find_val_idx_any(ppNames, n);
	if (idx >= 0) {
		xt_float3 f3 = get_val_f3(idx);
		v.set(f3.x, f3.y, f3.z);
	}
	return v;
}

cxColor sxValuesData::Group::get_rgb(const char* pName, const cxColor& defVal) const {
	cxColor c = defVal;
	int idx = find_val_idx(pName);
	if (idx >= 0) {
		xt_float3 f3 = get_val_f3(idx);
		c.set(f3.x, f3.y, f3.z);
	}
	return c;
}

cxColor sxValuesData::Group::get_rgb_any(const char** ppNames, int n, const cxColor& defVal) const {
	cxColor c = defVal;
	int idx = find_val_idx_any(ppNames, n);
	if (idx >= 0) {
		xt_float3 f3 = get_val_f3(idx);
		c.set(f3.x, f3.y, f3.z);
	}
	return c;
}

const char* sxValuesData::Group::get_str(const char* pName, const char* pDefVal) const {
	const char* pStr = pDefVal;
	int idx = find_val_idx(pName);
	if (idx >= 0) {
		pStr = get_val_s(idx);
	}
	return pStr;
}

const char* sxValuesData::Group::get_str_any(const char** ppNames, int n, const char* pDefVal) const {
	const char* pStr = pDefVal;
	int idx = find_val_idx_any(ppNames, n);
	if (idx >= 0) {
		pStr = get_val_s(idx);
	}
	return pStr;
}


int sxValuesData::find_grp_idx(const char* pName, const char* pPath, int startIdx) const {
	int idx = -1;
	sxStrList* pStrLst = get_str_list();
	if (ck_grp_idx(startIdx) && pStrLst) {
		int ngrp = get_grp_num();
		int nameId = pName ? pStrLst->find_str(pName) : -1;
		if (nameId >= 0) {
			if (pPath) {
				int pathId = pStrLst->find_str(pPath);
				if (pathId >= 0) {
					for (int i = startIdx; i < ngrp; ++i) {
						GrpInfo* pGrp = get_grp_info(i);
						if (nameId == pGrp->mNameId && pathId == pGrp->mPathId) {
							idx = i;
							break;
						}
					}
				}
			} else {
				for (int i = startIdx; i < ngrp; ++i) {
					GrpInfo* pGrp = get_grp_info(i);
					if (nameId == pGrp->mNameId) {
						idx = i;
						break;
					}
				}
			}
		} else if (pPath) {
			int pathId = pStrLst->find_str(pPath);
			if (pathId >= 0) {
				int ngrp = get_grp_num();
				for (int i = startIdx; i < ngrp; ++i) {
					GrpInfo* pGrp = get_grp_info(i);
					if (pathId == pGrp->mPathId) {
						idx = i;
						break;
					}
				}
			}
		}
	}
	return idx;
}

sxValuesData::Group sxValuesData::get_grp(int idx) const {
	Group grp;
	if (ck_grp_idx(idx)) {
		grp.mpVals = this;
		grp.mGrpId = idx;
	} else {
		grp.mpVals = nullptr;
		grp.mGrpId = -1;
	}
	return grp;
}

/*static*/ const char* sxValuesData::get_val_type_str(eValType typ) {
	const char* pStr = "UNKNOWN";
	switch (typ) {
		case eValType::FLOAT:
			pStr = "FLOAT";
			break;
		case eValType::VEC2:
			pStr = "VEC2";
			break;
		case eValType::VEC3:
			pStr = "VEC3";
			break;
		case eValType::VEC4:
			pStr = "VEC4";
			break;
		case eValType::INT:
			pStr = "INT";
			break;
		case eValType::STRING:
			pStr = "STRING";
			break;
		default:
			break;
	}
	return pStr;
}


sxRigData::Node* sxRigData::get_node_ptr(int idx) const {
	Node* pNode = nullptr;
	Node* pTop = get_node_top();
	if (pTop) {
		if (ck_node_idx(idx)) {
			pNode = pTop + idx;
		}
	}
	return pNode;
}

const char* sxRigData::get_node_name(int idx) const {
	const char* pName = nullptr;
	if (ck_node_idx(idx)) {
		sxStrList* pStrLst = get_str_list();
		if (pStrLst) {
			pName = pStrLst->get_str(get_node_ptr(idx)->mNameId);
		}
	}
	return pName;
}

const char* sxRigData::get_node_path(int idx) const {
	const char* pPath = nullptr;
	if (ck_node_idx(idx)) {
		sxStrList* pStrLst = get_str_list();
		if (pStrLst) {
			pPath = pStrLst->get_str(get_node_ptr(idx)->mPathId);
		}
	}
	return pPath;
}

const char* sxRigData::get_node_type(int idx) const {
	const char* pType = nullptr;
	if (ck_node_idx(idx)) {
		sxStrList* pStrLst = get_str_list();
		if (pStrLst) {
			pType = pStrLst->get_str(get_node_ptr(idx)->mTypeId);
		}
	}
	return pType;
}

int sxRigData::find_node(const char* pName, const char* pPath) const {
	int idx = -1;
	sxStrList* pStrLst = get_str_list();
	if (pStrLst) {
		int n = mNodeNum;
		int nameId = pStrLst->find_str(pName);
		if (nameId >= 0) {
			if (pPath) {
				int pathId = pStrLst->find_str(pPath);
				if (pathId >= 0) {
					for (int i = 0; i < n; ++i) {
						Node* pNode = get_node_ptr(i);
						if (pNode->mNameId == nameId && pNode->mPathId == pathId) {
							idx = i;
							break;
						}
					}
				}
			} else {
				for (int i = 0; i < n; ++i) {
					Node* pNode = get_node_ptr(i);
					if (pNode->mNameId == nameId) {
						idx = i;
						break;
					}
				}
			}
		}
	}
	return idx;
}

cxMtx sxRigData::get_wmtx(int idx) const {
	cxMtx mtx;
	cxMtx* pMtx = get_wmtx_ptr(idx);
	if (pMtx) {
		mtx = *pMtx;
	} else {
		mtx.identity();
	}
	return mtx;
}

cxMtx* sxRigData::get_wmtx_ptr(int idx) const {
	cxMtx* pMtx = nullptr;
	if (ck_node_idx(idx) && mOffsWMtx) {
		cxMtx* pTop = reinterpret_cast<cxMtx*>(XD_INCR_PTR(this, mOffsWMtx));
		pMtx = pTop + idx;
	}
	return pMtx;
}

cxMtx sxRigData::get_imtx(int idx) const {
	cxMtx mtx;
	cxMtx* pMtx = get_imtx_ptr(idx);
	if (pMtx) {
		mtx = *pMtx;
	} else {
		pMtx = get_wmtx_ptr(idx);
		if (pMtx) {
			mtx = pMtx->get_inverted();
		} else {
			mtx.identity();
		}
	}
	return mtx;
}

cxMtx* sxRigData::get_imtx_ptr(int idx) const {
	cxMtx* pMtx = nullptr;
	if (ck_node_idx(idx) && mOffsIMtx) {
		cxMtx* pTop = reinterpret_cast<cxMtx*>(XD_INCR_PTR(this, mOffsIMtx));
		pMtx = pTop + idx;
	}
	return pMtx;
}

cxMtx sxRigData::get_lmtx(int idx) const {
	cxMtx mtx;
	cxMtx* pMtx = get_lmtx_ptr(idx);
	if (pMtx) {
		mtx = *pMtx;
	} else {
		mtx = calc_lmtx(idx);
	}
	return mtx;
}

cxMtx* sxRigData::get_lmtx_ptr(int idx) const {
	cxMtx* pMtx = nullptr;
	if (ck_node_idx(idx) && mOffsLMtx) {
		cxMtx* pTop = reinterpret_cast<cxMtx*>(XD_INCR_PTR(this, mOffsLMtx));
		pMtx = pTop + idx;
	}
	return pMtx;
}

cxMtx sxRigData::calc_lmtx(int idx) const {
	Node* pNode = get_node_ptr(idx);
	if (pNode->mParentIdx < 0) {
		return get_wmtx(idx);
	}
	cxMtx m = get_wmtx(idx);
	m.mul(get_imtx(pNode->mParentIdx));
	return m;
}

cxVec sxRigData::calc_parent_offs(int idx) const {
	Node* pNode = get_node_ptr(idx);
	if (pNode->mParentIdx < 0) {
		return get_wmtx_ptr(idx)->get_translation();
	}
	return get_wmtx_ptr(idx)->get_translation() - get_wmtx_ptr(pNode->mParentIdx)->get_translation();
}

cxVec sxRigData::get_lpos(int idx) const {
	cxVec pos(0.0f);
	if (ck_node_idx(idx) && mOffsLPos) {
		cxVec* pTop = reinterpret_cast<cxVec*>(XD_INCR_PTR(this, mOffsLPos));
		pos = pTop[idx];
	}
	return pos;
}

cxVec sxRigData::get_lscl(int idx) const {
	cxVec scl(1.0f);
	if (ck_node_idx(idx) && mOffsLScl) {
		cxVec* pTop = reinterpret_cast<cxVec*>(XD_INCR_PTR(this, mOffsLScl));
		scl = pTop[idx];
	}
	return scl;
}

cxVec sxRigData::get_lrot(int idx, bool inRadians) const {
	cxVec rot(0.0f);
	if (ck_node_idx(idx) && mOffsLRot) {
		cxVec* pTop = reinterpret_cast<cxVec*>(XD_INCR_PTR(this, mOffsLRot));
		rot = pTop[idx];
		if (inRadians) {
			rot.scl(XD_DEG2RAD(1.0f));
		}
	}
	return rot;
}

cxQuat sxRigData::calc_lquat(int idx) const {
	cxQuat q;
	cxVec r = get_lrot(idx, true);
	q.set_rot(r.x, r.y, r.z, get_rot_order(idx));
	return q;
}

cxMtx sxRigData::calc_wmtx(int idx, const cxMtx* pMtxLocal, cxMtx* pParentWMtx) const {
	cxMtx mtx;
	mtx.identity();
	cxMtx parentMtx;
	parentMtx.identity();
	if (pMtxLocal && ck_node_idx(idx)) {
		Node* pNode = get_node_ptr(idx);
		mtx = pMtxLocal[pNode->mSelfIdx];
		pNode = get_node_ptr(pNode->mParentIdx);
		while (pNode && !pNode->is_hrc_top()) {
			parentMtx.mul(pMtxLocal[pNode->mSelfIdx]);
			pNode = get_node_ptr(pNode->mParentIdx);
		}
		if (pNode) {
			parentMtx.mul(pMtxLocal[pNode->mSelfIdx]);
		}
		mtx.mul(parentMtx);
	}
	if (pParentWMtx) {
		*pParentWMtx = parentMtx;
	}
	return mtx;
}

void sxRigData::LimbChain::set(LimbInfo* pInfo) {
	if (pInfo) {
		mTopCtrl = pInfo->mTopCtrl;
		mEndCtrl = pInfo->mEndCtrl;
		mExtCtrl = pInfo->mExtCtrl;
		mTop = pInfo->mTop;
		mRot = pInfo->mRot;
		mEnd = pInfo->mEnd;
		mExt = pInfo->mExt;
		mAxis = (exAxis)pInfo->mAxis;
		mUp = (exAxis)pInfo->mUp;
		mExtCompensate = pInfo->get_ext_comp_flg();
	} else {
		mTopCtrl = -1;
		mEndCtrl = -1;
		mExtCtrl = -1;
		mTop = -1;
		mRot = -1;
		mEnd = -1;
		mExt = -1;
		mAxis = exAxis::MINUS_Y;
		mUp = exAxis::PLUS_Z;
		mExtCompensate = false;
	}
}

static xt_float2 ik_cos_law(float a, float b, float c) {
	xt_float2 ang;
	if (c < a + b) {
		float aa = nxCalc::sq(a);
		float bb = nxCalc::sq(b);
		float cc = nxCalc::sq(c);
		float c0 = (aa - bb + cc) / (2.0f*a*c);
		float c1 = (aa + bb - cc) / (2.0f*a*b);
		c0 = nxCalc::clamp(c0, -1.0f, 1.0f);
		c1 = nxCalc::clamp(c1, -1.0f, 1.0f);
		ang[0] = -::mth_acosf(c0);
		ang[1] = XD_PI - ::mth_acosf(c1);
	} else {
		ang.fill(0.0f);
	}
	return ang;
}

struct sxLimbIKWork {
	cxMtx mRootW;
	cxMtx mParentW;
	cxMtx mTopW;
	cxMtx mRotW;
	cxMtx mEndW;
	cxMtx mExtW;
	cxMtx mTopL;
	cxMtx mRotL;
	cxMtx mEndL;
	cxMtx mExtL;
	cxVec mRotOffs;
	const sxRigData* mpRig;
	const sxRigData::LimbChain* mpChain;
	float mDistTopRot;
	float mDistRotEnd;
	float mDistTopEnd;
	exAxis mAxis;
	exAxis mUp;

	void set_axis_leg() {
		mAxis = exAxis::MINUS_Y;
		mUp = exAxis::PLUS_Z;
	}

	void set_axis_arm_l() {
		mAxis = exAxis::PLUS_X;
		mUp = exAxis::MINUS_Z;
	}

	void set_axis_arm_r() {
		mAxis = exAxis::MINUS_X;
		mUp = exAxis::MINUS_Z;
	}

	void calc_world();
	void calc_local(bool fixPos = true);
};

void sxLimbIKWork::calc_world() {
	xt_float2 ang = ik_cos_law(mDistTopRot, mDistRotEnd, mDistTopEnd);
	cxVec axis = nxVec::get_axis(mAxis);
	cxVec up = nxVec::get_axis(mUp);
	cxMtx mtxZY = nxMtx::orient_zy(axis, up, false);
	cxVec side = mtxZY.calc_vec(nxVec::get_axis(exAxis::PLUS_X));
	cxVec axisX = mTopW.calc_vec(side);
	cxVec axisZ = (mEndW.get_translation() - mTopW.get_translation()).get_normalized();
	cxMtx mtxZX = nxMtx::orient_zx(axisZ, axisX, false);
	cxMtx mtxIK = mtxZY.get_transposed() * mtxZX;
	cxMtx rotMtx = nxMtx::from_axis_angle(side, ang[0]);
	cxVec topPos = mTopW.get_translation();
	mTopW = rotMtx * mtxIK;
	mTopW.set_translation(topPos);
	cxVec rotPos = mTopW.calc_pnt(mRotOffs);
	rotMtx = nxMtx::from_axis_angle(side, ang[1]);
	mRotW = rotMtx * mTopW;
	mRotW.set_translation(rotPos);
}

void sxLimbIKWork::calc_local(bool fixPos) {
	mTopL = mTopW * mParentW.get_inverted();
	mRotL = mRotW * mTopW.get_inverted();
	mEndL = mEndW * mRotW.get_inverted();
	if (fixPos && mpRig && mpChain) {
		mTopL.set_translation(mpRig->get_lpos(mpChain->mTop));
		mRotL.set_translation(mpRig->get_lpos(mpChain->mRot));
		mEndL.set_translation(mpRig->get_lpos(mpChain->mEnd));
	}
}

void sxRigData::calc_limb_local(LimbChain::Solution* pSolution, const LimbChain& chain, cxMtx* pMtx, LimbChain::AdjustFunc* pAdjFunc) const {
	if (!pMtx) return;
	if (!pSolution) return;
	if (!ck_node_idx(chain.mTopCtrl)) return;
	if (!ck_node_idx(chain.mEndCtrl)) return;
	if (!ck_node_idx(chain.mTop)) return;
	if (!ck_node_idx(chain.mRot)) return;
	if (!ck_node_idx(chain.mEnd)) return;
	int parentIdx = get_parent_idx(chain.mTopCtrl);
	if (!ck_node_idx(parentIdx)) return;
	bool isExt = ck_node_idx(chain.mExtCtrl);
	int rootIdx = isExt ? get_parent_idx(chain.mExtCtrl) : get_parent_idx(chain.mEndCtrl);
	if (!ck_node_idx(rootIdx)) return;

	sxLimbIKWork ik;
	ik.mpRig = this;
	ik.mpChain = &chain;
	ik.mTopW = calc_wmtx(chain.mTopCtrl, pMtx, &ik.mParentW);
	ik.mRootW = calc_wmtx(rootIdx, pMtx);
	if (isExt) {
		ik.mExtW = pMtx[chain.mExtCtrl] * ik.mRootW;
		ik.mEndW = pMtx[chain.mExtCtrl] * pMtx[chain.mEndCtrl].get_sr() * ik.mRootW;
	} else {
		ik.mExtW.identity();
		ik.mEndW = pMtx[chain.mEndCtrl] * ik.mRootW;
	}

	cxVec effPos = isExt ? ik.mExtW.get_translation() : ik.mEndW.get_translation();
	if (pAdjFunc) {
		effPos = (*pAdjFunc)(*this, chain, effPos);
	}

	cxVec endPos;
	if (isExt) {
		ik.mExtW.set_translation(effPos);
		cxVec extOffs = ck_node_idx(chain.mExt) ? get_lpos(chain.mExt).neg_val() : get_lpos(chain.mExtCtrl);
		extOffs = pMtx[chain.mExtCtrl].calc_vec(extOffs);
		endPos = (pMtx[chain.mEndCtrl] * ik.mRootW).calc_vec(extOffs) + ik.mExtW.get_translation();
	} else {
		endPos = effPos;
	}
	ik.mEndW.set_translation(endPos);

	ik.mRotOffs = get_lpos(chain.mRot);
	ik.mDistTopRot = calc_parent_dist(chain.mRot);
	ik.mDistRotEnd = calc_parent_dist(chain.mEnd);
	ik.mDistTopEnd = nxVec::dist(endPos, ik.mTopW.get_translation());
	ik.mAxis = chain.mAxis;
	ik.mUp = chain.mUp;
	ik.calc_world();
	ik.calc_local();

	if (isExt) {
		if (chain.mExtCompensate) {
			ik.mExtL = ik.mExtW * ik.mEndW.get_inverted();
		} else {
			ik.mExtL = pMtx[chain.mExtCtrl] * ik.mRootW * ik.mEndW.get_inverted();
		}
		ik.mExtL.set_translation(get_lpos(chain.mExt));
	} else {
		ik.mExtL.identity();
	}

	pSolution->mTop = ik.mTopL;
	pSolution->mRot = ik.mRotL;
	pSolution->mEnd = ik.mEndL;
	pSolution->mExt = ik.mExtL;
}

void sxRigData::copy_limb_solution(cxMtx* pDstMtx, const LimbChain& chain, const LimbChain::Solution& solution) {
	if (!pDstMtx) return;
	if (ck_node_idx(chain.mTop)) {
		pDstMtx[chain.mTop] = solution.mTop;
	}
	if (ck_node_idx(chain.mRot)) {
		pDstMtx[chain.mRot] = solution.mRot;
	}
	if (ck_node_idx(chain.mEnd)) {
		pDstMtx[chain.mEnd] = solution.mEnd;
	}
	if (ck_node_idx(chain.mExt)) {
		pDstMtx[chain.mExt] = solution.mExt;
	}
}

bool sxRigData::has_info_list() const {
	bool res = false;
	ptrdiff_t offs = (uint8_t*)&mOffsInfo - (uint8_t*)this;
	if (mHeadSize > (uint32_t)offs) {
		if (mOffsInfo != 0) {
			Info* pInfo = reinterpret_cast<Info*>(XD_INCR_PTR(this, mOffsInfo));
			if (pInfo->get_kind() == eInfoKind::LIST) {
				res = true;
			}
		}
	}
	return res;
}

sxRigData::Info* sxRigData::find_info(eInfoKind kind) const {
	Info* pInfo = nullptr;
	if (has_info_list()) {
		Info* pList = reinterpret_cast<Info*>(XD_INCR_PTR(this, mOffsInfo));
		Info* pEntry = reinterpret_cast<Info*>(XD_INCR_PTR(this, pList->mOffs));
		int n = pList->mNum;
		for (int i = 0; i < n; ++i) {
			if (pEntry->get_kind() == kind) {
				pInfo = pEntry;
				break;
			}
			++pEntry;
		}
	}
	return pInfo;
}

sxRigData::LimbInfo* sxRigData::find_limb(sxRigData::eLimbType type, int idx) const {
	sxRigData::LimbInfo* pLimb = nullptr;
	Info* pInfo = find_info(eInfoKind::LIMBS);
	if (pInfo && pInfo->mOffs) {
		int n = pInfo->mNum;
		LimbInfo* pCk = (LimbInfo*)XD_INCR_PTR(this, pInfo->mOffs);
		for (int i = 0; i < n; ++i) {
			if (pCk[i].get_type() == type && pCk[i].get_idx() == idx) {
				pLimb = &pCk[i];
				break;
			}
		}
	}
	return pLimb;
}

int sxRigData::get_expr_num() const {
	int n = 0;
	Info* pInfo = find_info(eInfoKind::EXPRS);
	if (pInfo) {
		n = pInfo->mNum;
	}
	return n;
}

int sxRigData::get_expr_stack_size() const {
	int stksize = 0;
	Info* pInfo = find_info(eInfoKind::EXPRS);
	if (pInfo) {
		stksize = (int)(pInfo->mParam & 0xFF);
		if (0 == stksize) {
			stksize = 16;
		}
	}
	return stksize;
}

sxRigData::ExprInfo* sxRigData::get_expr_info(int idx) const {
	ExprInfo* pExprInfo = nullptr;
	Info* pInfo = find_info(eInfoKind::EXPRS);
	if (pInfo && pInfo->mOffs) {
		if ((uint32_t)idx < pInfo->mNum) {
			uint32_t* pOffs = reinterpret_cast<uint32_t*>(XD_INCR_PTR(this, pInfo->mOffs));
			uint32_t offs = pOffs[idx];
			if (offs) {
				pExprInfo = reinterpret_cast<sxRigData::ExprInfo*>(XD_INCR_PTR(this, offs));
			}
		}
	}
	return pExprInfo;
}

sxCompiledExpression* sxRigData::get_expr_code(int idx) const {
	sxCompiledExpression* pCode = nullptr;
	ExprInfo* pExprInfo = get_expr_info(idx);
	if (pExprInfo) {
		pCode = pExprInfo->get_code();
	}
	return pCode;
}


int sxGeometryData::get_vtx_idx_size() const {
	int size = 0;
	if (mPntNum <= (1 << 8)) {
		size = 1;
	} else if (mPntNum <= (1 << 16)) {
		size = 2;
	} else {
		size = 3;
	}
	return size;
}

sxGeometryData::Polygon sxGeometryData::get_pol(int idx) const {
	Polygon pol;
	if (ck_pol_idx(idx)) {
		pol.mPolId = idx;
		pol.mpGeom = this;
	} else {
		pol.mPolId = -1;
		pol.mpGeom = nullptr;
	}
	return pol;
}

int32_t* sxGeometryData::get_skin_node_name_ids() const {
	int32_t* pIds = nullptr;
	cxSphere* pSphTop = get_skin_sph_top();
	if (pSphTop) {
		pIds = reinterpret_cast<int32_t*>(&pSphTop[mSkinNodeNum]);
	}
	return pIds;
}

const char* sxGeometryData::get_skin_node_name(int idx) const {
	const char* pName = nullptr;
	if (ck_skin_idx(idx)) {
		sxStrList* pStrLst = get_str_list();
		cxSphere* pSphTop = get_skin_sph_top();
		if (pStrLst && pSphTop) {
			int32_t* pNameIds = reinterpret_cast<int32_t*>(&pSphTop[mSkinNodeNum]);
			pName = pStrLst->get_str(pNameIds[idx]);
		}
	}
	return pName;
}

int sxGeometryData::find_skin_node(const char* pName) const {
	int idx = -1;
	sxStrList* pStrLst = get_str_list();
	if (has_skin_nodes() && pStrLst) {
		int nameId = pStrLst->find_str(pName);
		if (nameId >= 0) {
			cxSphere* pSphTop = get_skin_sph_top();
			int32_t* pNameIds = reinterpret_cast<int32_t*>(&pSphTop[mSkinNodeNum]);
			int n = get_skin_nodes_num();
			for (int i = 0; i < n; ++i) {
				if (pNameIds[i] == nameId) {
					idx = i;
					break;
				}
			}
		}
	}
	return idx;
}

uint32_t sxGeometryData::get_attr_info_offs(eAttrClass cls) const {
	uint32_t offs = 0;
	switch (cls) {
		case eAttrClass::GLOBAL:
			offs = mGlbAttrOffs;
			break;
		case eAttrClass::POINT:
			offs = mPntAttrOffs;
			break;
		case eAttrClass::POLYGON:
			offs = mPolAttrOffs;
			break;
	}
	return offs;
}

uint32_t sxGeometryData::get_attr_info_num(eAttrClass cls) const {
	uint32_t num = 0;
	switch (cls) {
		case eAttrClass::GLOBAL:
			num = mGlbAttrNum;
			break;
		case eAttrClass::POINT:
			num = mPntAttrNum;
			break;
		case eAttrClass::POLYGON:
			num = mPolAttrNum;
			break;
	}
	return num;
}

uint32_t sxGeometryData::get_attr_item_num(eAttrClass cls) const {
	uint32_t num = 0;
	switch (cls) {
		case eAttrClass::GLOBAL:
			num = 1;
			break;
		case eAttrClass::POINT:
			num = mPntNum;
			break;
		case eAttrClass::POLYGON:
			num = mPolNum;
			break;
	}
	return num;
}

int sxGeometryData::find_attr(const char* pName, eAttrClass cls) const {
	int attrId = -1;
	uint32_t offs = get_attr_info_offs(cls);
	uint32_t num = get_attr_info_num(cls);
	if (offs && num) {
		sxStrList* pStrLst = get_str_list();
		if (pStrLst) {
			int nameId = pStrLst->find_str(pName);
			if (nameId >= 0) {
				AttrInfo* pAttr = reinterpret_cast<AttrInfo*>(XD_INCR_PTR(this, offs));
				for (int i = 0; i < (int)num; ++i) {
					if (pAttr[i].mNameId == nameId) {
						attrId = i;
						break;
					}
				}
			}
		}
	}
	return attrId;
}

sxGeometryData::AttrInfo* sxGeometryData::get_attr_info(int attrIdx, eAttrClass cls) const {
	AttrInfo* pAttr = nullptr;
	uint32_t offs = get_attr_info_offs(cls);
	uint32_t num = get_attr_info_num(cls);
	if (offs && num && (uint32_t)attrIdx < num) {
		pAttr = &reinterpret_cast<AttrInfo*>(XD_INCR_PTR(this, offs))[attrIdx];
	}
	return pAttr;
}

const char* sxGeometryData::get_attr_val_s(int attrIdx, eAttrClass cls, int itemIdx) const {
	const char* pStr = nullptr;
	uint32_t itemNum = get_attr_item_num(cls);
	if ((uint32_t)itemIdx < itemNum) {
		AttrInfo* pInfo = get_attr_info(attrIdx, cls);
		if (pInfo && pInfo->is_string()) {
			uint32_t* pTop = reinterpret_cast<uint32_t*>(XD_INCR_PTR(this, pInfo->mDataOffs));
			int strId = pTop[itemIdx*pInfo->mElemNum];
			pStr = get_str(strId);
		}
	}
	return pStr;
}

float* sxGeometryData::get_attr_data_f(int attrIdx, eAttrClass cls, int itemIdx, int minElem) const {
	float* pData = nullptr;
	uint32_t itemNum = get_attr_item_num(cls);
	if ((uint32_t)itemIdx < itemNum) {
		AttrInfo* pInfo = get_attr_info(attrIdx, cls);
		if (pInfo && pInfo->is_float() && pInfo->mElemNum >= minElem) {
			float* pTop = reinterpret_cast<float*>(XD_INCR_PTR(this, pInfo->mDataOffs));
			pData = &pTop[itemIdx*pInfo->mElemNum];
		}
	}
	return pData;
}

float sxGeometryData::get_pnt_attr_val_f(int attrIdx, int pntIdx) const {
	float res = 0.0f;
	float* pData = get_attr_data_f(attrIdx, eAttrClass::POINT, pntIdx);
	if (pData) {
		res = *pData;
	}
	return res;
}

xt_float3 sxGeometryData::get_pnt_attr_val_f3(int attrIdx, int pntIdx) const {
	xt_float3 res;
	float* pData = get_attr_data_f(attrIdx, eAttrClass::POINT, pntIdx, 3);
	if (pData) {
		res.set(pData[0], pData[1], pData[2]);
	} else {
		res.fill(0.0f);
	}
	return res;
}

cxVec sxGeometryData::get_pnt_normal(int pntIdx) const {
	cxVec nrm(0.0f, 1.0f, 0.0f);
	int attrIdx = find_pnt_attr("N");
	if (attrIdx >= 0) {
		float* pData = get_attr_data_f(attrIdx, eAttrClass::POINT, pntIdx, 3);
		if (pData) {
			nrm.from_mem(pData);
		}
	}
	return nrm;
}

cxVec sxGeometryData::get_pnt_tangent(int pntIdx) const {
	cxVec tng(1.0f, 0.0f, 0.0f);
	int attrIdx = find_pnt_attr("tangentu");
	if (attrIdx >= 0) {
		float* pData = get_attr_data_f(attrIdx, eAttrClass::POINT, pntIdx, 3);
		if (pData) {
			tng.from_mem(pData);
		}
	}
	return tng;
}

cxVec sxGeometryData::get_pnt_bitangent(int pntIdx) const {
	cxVec btg(0.0f, 0.0f, 1.0f);
	int attrIdx = find_pnt_attr("tangentv");
	if (attrIdx >= 0) {
		float* pData = get_attr_data_f(attrIdx, eAttrClass::POINT, pntIdx, 3);
		if (pData) {
			btg.from_mem(pData);
		} else {
			btg = calc_pnt_bitangent(pntIdx);
		}
	} else {
		btg = calc_pnt_bitangent(pntIdx);
	}
	return btg;
}

cxVec sxGeometryData::calc_pnt_bitangent(int pntIdx) const {
	cxVec nrm = get_pnt_normal(pntIdx);
	cxVec tng = get_pnt_tangent(pntIdx);
	cxVec btg = nxVec::cross(tng, nrm);
	btg.normalize();
	return btg;
}

cxColor sxGeometryData::get_pnt_color(int pntIdx, bool useAlpha) const {
	cxColor clr(1.0f, 1.0f, 1.0f);
	int attrIdx = find_pnt_attr("Cd");
	if (attrIdx >= 0) {
		float* pData = get_attr_data_f(attrIdx, eAttrClass::POINT, pntIdx, 3);
		if (pData) {
			clr.set(pData[0], pData[1], pData[2]);
		}
	}
	if (useAlpha) {
		attrIdx = find_pnt_attr("Alpha");
		if (attrIdx >= 0) {
			float* pData = get_attr_data_f(attrIdx, eAttrClass::POINT, pntIdx, 1);
			if (pData) {
				clr.a = *pData;
			}
		}
	}
	return clr;
}

xt_texcoord sxGeometryData::get_pnt_texcoord(int pntIdx) const {
	xt_texcoord tex;
	tex.set(0.0f, 0.0f);
	int attrIdx = find_pnt_attr("uv");
	if (attrIdx >= 0) {
		float* pData = get_attr_data_f(attrIdx, eAttrClass::POINT, pntIdx, 2);
		if (pData) {
			tex.set(pData[0], pData[1]);
		}
	}
	return tex;
}

xt_texcoord sxGeometryData::get_pnt_texcoord2(int pntIdx) const {
	xt_texcoord tex;
	tex.set(0.0f, 0.0f);
	int attrIdx = find_pnt_attr("uv2");
	if (attrIdx < 0) {
		attrIdx = find_pnt_attr("uv");
	}
	if (attrIdx >= 0) {
		float* pData = get_attr_data_f(attrIdx, eAttrClass::POINT, pntIdx, 2);
		if (pData) {
			tex.set(pData[0], pData[1]);
		}
	}
	return tex;
}

int sxGeometryData::get_pnt_wgt_num(int pntIdx) const {
	int n = 0;
	if (has_skin() && ck_pnt_idx(pntIdx)) {
		uint32_t* pSkinTbl = reinterpret_cast<uint32_t*>(XD_INCR_PTR(this, mSkinOffs));
		uint8_t* pWgtNum = reinterpret_cast<uint8_t*>(&pSkinTbl[mPntNum]);
		n = pWgtNum[pntIdx];
	}
	return n;
}

int sxGeometryData::get_pnt_skin_jnt(int pntIdx, int wgtIdx) const {
	int idx = -1;
	if (has_skin() && ck_pnt_idx(pntIdx)) {
		uint32_t* pSkinTbl = reinterpret_cast<uint32_t*>(XD_INCR_PTR(this, mSkinOffs));
		uint8_t* pWgtNum = reinterpret_cast<uint8_t*>(&pSkinTbl[mPntNum]);
		int nwgt = pWgtNum[pntIdx];
		if ((uint32_t)wgtIdx < (uint32_t)nwgt) {
			float* pWgt = reinterpret_cast<float*>(XD_INCR_PTR(this, pSkinTbl[pntIdx]));
			uint8_t* pIdx = reinterpret_cast<uint8_t*>(&pWgt[nwgt]);
			int nnodes = get_skin_nodes_num();
			if (nnodes <= (1 << 8)) {
				idx = pIdx[wgtIdx];
			} else {
				idx = pIdx[wgtIdx*2];
				idx |= pIdx[wgtIdx*2 + 1] << 8;
			}
		}
	}
	return idx;
}

float sxGeometryData::get_pnt_skin_wgt(int pntIdx, int wgtIdx) const {
	float w = 0.0f;
	if (has_skin() && ck_pnt_idx(pntIdx)) {
		uint32_t* pSkinTbl = reinterpret_cast<uint32_t*>(XD_INCR_PTR(this, mSkinOffs));
		uint8_t* pWgtNum = reinterpret_cast<uint8_t*>(&pSkinTbl[mPntNum]);
		int nwgt = pWgtNum[pntIdx];
		if ((uint32_t)wgtIdx < (uint32_t)nwgt) {
			float* pWgt = reinterpret_cast<float*>(XD_INCR_PTR(this, pSkinTbl[pntIdx]));
			w = pWgt[wgtIdx];
		}
	}
	return w;
}

int sxGeometryData::find_pnt_grp_idx(const char* pName, const char* pPath) const {
	int idx = -1;
	sxStrList* pStrLst = get_str_list();
	int ngrp = (int)mPntGrpNum;
	if (ngrp && mPntGrpOffs && pStrLst) {
		int nameId = pStrLst->find_str(pName);
		if (nameId >= 0) {
			uint32_t* pOffs = reinterpret_cast<uint32_t*>(XD_INCR_PTR(this, mPntGrpOffs));
			if (pPath) {
				int pathId = pStrLst->find_str(pPath);
				if (pathId >= 0) {
					for (int i = 0; i < ngrp; ++i) {
						if (pOffs[i]) {
							GrpInfo* pInfo = reinterpret_cast<GrpInfo*>(XD_INCR_PTR(this, pOffs[i]));
							if (pInfo->mNameId == nameId && pInfo->mPathId == pathId) {
								idx = i;
								break;
							}
						}
					}
				}
			} else {
				for (int i = 0; i < ngrp; ++i) {
					if (pOffs[i]) {
						GrpInfo* pInfo = reinterpret_cast<GrpInfo*>(XD_INCR_PTR(this, pOffs[i]));
						if (pInfo->mNameId == nameId) {
							idx = i;
							break;
						}
					}
				}
			}
		}
	}
	return idx;
}

int sxGeometryData::find_pol_grp_idx(const char* pName, const char* pPath) const {
	int idx = -1;
	sxStrList* pStrLst = get_str_list();
	int ngrp = (int)mPolGrpNum;
	if (ngrp && mPolGrpOffs && pStrLst) {
		int nameId = pStrLst->find_str(pName);
		if (nameId >= 0) {
			uint32_t* pOffs = reinterpret_cast<uint32_t*>(XD_INCR_PTR(this, mPolGrpOffs));
			if (pPath) {
				int pathId = pStrLst->find_str(pPath);
				if (pathId >= 0) {
					for (int i = 0; i < ngrp; ++i) {
						if (pOffs[i]) {
							GrpInfo* pInfo = reinterpret_cast<GrpInfo*>(XD_INCR_PTR(this, pOffs[i]));
							if (pInfo->mNameId == nameId && pInfo->mPathId == pathId) {
								idx = i;
								break;
							}
						}
					}
				}
			} else {
				for (int i = 0; i < ngrp; ++i) {
					if (pOffs[i]) {
						GrpInfo* pInfo = reinterpret_cast<GrpInfo*>(XD_INCR_PTR(this, pOffs[i]));
						if (pInfo->mNameId == nameId) {
							idx = i;
							break;
						}
					}
				}
			}
		}
	}
	return idx;
}

int sxGeometryData::find_mtl_grp_idx(const char* pName, const char* pPath) const {
	int idx = -1;
	sxStrList* pStrLst = get_str_list();
	int nmtl = (int)mMtlNum;
	if (nmtl && mMtlOffs && pStrLst) {
		int nameId = pStrLst->find_str(pName);
		if (nameId >= 0) {
			uint32_t* pOffs = reinterpret_cast<uint32_t*>(XD_INCR_PTR(this, mMtlOffs));
			if (pPath) {
				int pathId = pStrLst->find_str(pPath);
				if (pathId >= 0) {
					for (int i = 0; i < nmtl; ++i) {
						if (pOffs[i]) {
							GrpInfo* pInfo = reinterpret_cast<GrpInfo*>(XD_INCR_PTR(this, pOffs[i]));
							if (pInfo->mNameId == nameId && pInfo->mPathId == pathId) {
								idx = i;
								break;
							}
						}
					}
				}
			} else {
				for (int i = 0; i < nmtl; ++i) {
					if (pOffs[i]) {
						GrpInfo* pInfo = reinterpret_cast<GrpInfo*>(XD_INCR_PTR(this, pOffs[i]));
						if (pInfo->mNameId == nameId) {
							idx = i;
							break;
						}
					}
				}
			}
		}
	}
	return idx;
}

sxGeometryData::GrpInfo* sxGeometryData::get_mtl_info(int idx) const {
	GrpInfo* pInfo = nullptr;
	if (ck_mtl_idx(idx) && mMtlOffs) {
		uint32_t offs = reinterpret_cast<uint32_t*>(XD_INCR_PTR(this, mMtlOffs))[idx];
		if (offs) {
			pInfo = reinterpret_cast<GrpInfo*>(XD_INCR_PTR(this, offs));
		}
	}
	return pInfo;
}

sxGeometryData::Group sxGeometryData::get_mtl_grp(int idx) const {
	Group grp;
	GrpInfo* pInfo = get_mtl_info(idx);
	if (pInfo) {
		grp.mpGeom = this;
		grp.mpInfo = pInfo;
	} else {
		grp.mpGeom = nullptr;
		grp.mpInfo = nullptr;
	}
	return grp;
}

const char* sxGeometryData::get_mtl_name(int idx) const {
	const char* pName = nullptr;
	GrpInfo* pInfo = get_mtl_info(idx);
	if (pInfo) {
		pName = get_str(pInfo->mNameId);
	}
	return pName;
}

const char* sxGeometryData::get_mtl_path(int idx) const {
	const char* pPath = nullptr;
	GrpInfo* pInfo = get_mtl_info(idx);
	if (pInfo) {
		pPath = get_str(pInfo->mPathId);
	}
	return pPath;
}

sxGeometryData::GrpInfo* sxGeometryData::get_pnt_grp_info(int idx) const {
	GrpInfo* pInfo = nullptr;
	if (ck_pnt_grp_idx(idx) && mPntGrpOffs) {
		uint32_t offs = reinterpret_cast<uint32_t*>(XD_INCR_PTR(this, mPntGrpOffs))[idx];
		if (offs) {
			pInfo = reinterpret_cast<GrpInfo*>(XD_INCR_PTR(this, offs));
		}
	}
	return pInfo;
}

sxGeometryData::Group sxGeometryData::get_pnt_grp(int idx) const {
	Group grp;
	GrpInfo* pInfo = get_pnt_grp_info(idx);
	if (pInfo) {
		grp.mpGeom = this;
		grp.mpInfo = pInfo;
	} else {
		grp.mpGeom = nullptr;
		grp.mpInfo = nullptr;
	}
	return grp;
}

sxGeometryData::GrpInfo* sxGeometryData::get_pol_grp_info(int idx) const {
	GrpInfo* pInfo = nullptr;
	if (ck_pol_grp_idx(idx) && mPolGrpOffs) {
		uint32_t offs = reinterpret_cast<uint32_t*>(XD_INCR_PTR(this, mPolGrpOffs))[idx];
		if (offs) {
			pInfo = reinterpret_cast<GrpInfo*>(XD_INCR_PTR(this, offs));
		}
	}
	return pInfo;
}

sxGeometryData::Group sxGeometryData::get_pol_grp(int idx) const {
	Group grp;
	GrpInfo* pInfo = get_pol_grp_info(idx);
	if (pInfo) {
		grp.mpGeom = this;
		grp.mpInfo = pInfo;
	} else {
		grp.mpGeom = nullptr;
		grp.mpInfo = nullptr;
	}
	return grp;
}

void sxGeometryData::hit_query_nobvh(const cxLineSeg& seg, HitFunc& fun) const {
	cxVec hitPos;
	cxVec hitNrm;
	int npol = get_pol_num();
	for (int i = 0; i < npol; ++i) {
		Polygon pol = get_pol(i);
		bool hitFlg = pol.intersect(seg, &hitPos, &hitNrm);
		if (hitFlg) {
			float hitDist = nxVec::dist(seg.get_pos0(), hitPos);
			bool contFlg = fun(pol, hitPos, hitNrm, hitDist);
			if (!contFlg) {
				break;
			}
		}
	}
}

struct sxBVHWork {
	const sxGeometryData* mpGeo;
	cxAABB mQryBBox;
	cxLineSeg mQrySeg;
	sxGeometryData::HitFunc* mpHitFunc;
	sxGeometryData::RangeFunc* mpRangeFunc;
	sxGeometryData::LeafFunc* mpLeafFunc;
	bool mStopFlg;

	sxBVHWork() : mpGeo(nullptr), mpHitFunc(nullptr), mpRangeFunc(nullptr), mStopFlg(false) {}
};

static void BVH_hit_sub(sxBVHWork& wk, int nodeId) {
	if (wk.mStopFlg) return;
	sxGeometryData::BVH::Node* pNode = wk.mpGeo->get_BVH_node(nodeId);
	if (pNode->mBBox.overlaps(wk.mQryBBox) && pNode->mBBox.seg_ck(wk.mQrySeg)) {
		if (pNode->is_leaf()) {
			cxVec hitPos;
			cxVec hitNrm;
			sxGeometryData::Polygon pol = wk.mpGeo->get_pol(pNode->get_pol_id());
			bool hitFlg = pol.intersect(wk.mQrySeg, &hitPos, &hitNrm);
			if (hitFlg) {
				float hitDist = nxVec::dist(wk.mQrySeg.get_pos0(), hitPos);
				bool contFlg = (*wk.mpHitFunc)(pol, hitPos, hitNrm, hitDist);
				if (!contFlg) {
					wk.mStopFlg = true;
				}
			}
		} else {
			BVH_hit_sub(wk, pNode->mLeft);
			BVH_hit_sub(wk, pNode->mRight);
		}
	}
}

void sxGeometryData::hit_query(const cxLineSeg& seg, HitFunc& fun) const {
	BVH* pBVH = get_BVH();
	if (pBVH) {
		sxBVHWork wk;
		wk.mpGeo = this;
		wk.mQrySeg = seg;
		wk.mQryBBox.from_seg(seg);
		wk.mpHitFunc = &fun;
		BVH_hit_sub(wk, 0);
	} else {
		hit_query_nobvh(seg, fun);
	}
}

void sxGeometryData::range_query_nobvh(const cxAABB& box, RangeFunc& fun) const {
	int npol = get_pol_num();
	for (int i = 0; i < npol; ++i) {
		Polygon pol = get_pol(i);
		bool rangeFlg = pol.calc_bbox().overlaps(box);
		if (rangeFlg) {
			bool contFlg = fun(pol);
			if (!contFlg) {
				break;
			}
		}
	}
}

static void BVH_range_sub(sxBVHWork& wk, int nodeId) {
	if (wk.mStopFlg) return;
	sxGeometryData::BVH::Node* pNode = wk.mpGeo->get_BVH_node(nodeId);
	if (wk.mQryBBox.overlaps(pNode->mBBox)) {
		if (pNode->is_leaf()) {
			sxGeometryData::Polygon pol = wk.mpGeo->get_pol(pNode->get_pol_id());
			bool contFlg = (*wk.mpRangeFunc)(pol);
			if (!contFlg) {
				wk.mStopFlg = true;
			}
		} else {
			BVH_range_sub(wk, pNode->mLeft);
			BVH_range_sub(wk, pNode->mRight);
		}
	}
}

void sxGeometryData::range_query(const cxAABB& box, RangeFunc& fun) const {
	BVH* pBVH = get_BVH();
	if (pBVH) {
		sxBVHWork wk;
		wk.mpGeo = this;
		wk.mQryBBox = box;
		wk.mpRangeFunc = &fun;
		BVH_range_sub(wk, 0);
	} else {
		range_query_nobvh(box, fun);
	}
}

static void BVH_leaf_hit_sub(sxBVHWork& wk, int nodeId) {
	if (wk.mStopFlg) return;
	sxGeometryData::BVH::Node* pNode = wk.mpGeo->get_BVH_node(nodeId);
	if (pNode->mBBox.overlaps(wk.mQryBBox) && pNode->mBBox.seg_ck(wk.mQrySeg)) {
		if (pNode->is_leaf()) {
			sxGeometryData::Polygon pol = wk.mpGeo->get_pol(pNode->get_pol_id());
			bool contFlg = (*wk.mpLeafFunc)(pol, pNode);
			if (!contFlg) {
				wk.mStopFlg = true;
			}
		} else {
			BVH_leaf_hit_sub(wk, pNode->mLeft);
			BVH_leaf_hit_sub(wk, pNode->mRight);
		}
	}
}

void sxGeometryData::leaf_hit_query(const cxLineSeg& seg, LeafFunc& fun) const {
	if (!has_BVH()) return;
	sxBVHWork wk;
	wk.mpGeo = this;
	wk.mQrySeg = seg;
	wk.mQryBBox.from_seg(seg);
	wk.mpLeafFunc = &fun;
	BVH_leaf_hit_sub(wk, 0);
}

struct sxBVHLvlWork {
	const sxGeometryData* mpGeo;
	xt_int2 mLvlRange;
	sxBVHLvlWork() : mpGeo(nullptr) {
		mLvlRange.set(-1, -1);
	}
};

static void BVH_leaf_lvl_range_sub(sxBVHLvlWork& wk, int nodeId, int lvl) {
	sxGeometryData::BVH::Node* pNode = wk.mpGeo->get_BVH_node(nodeId);
	if (pNode->is_leaf()) {
		if (wk.mLvlRange[0] < 0 || lvl < wk.mLvlRange[0]) {
			wk.mLvlRange[0] = lvl;
		}
		wk.mLvlRange[1] = nxCalc::max(wk.mLvlRange[1], lvl);
	} else {
		BVH_leaf_lvl_range_sub(wk, pNode->mLeft, lvl + 1);
		BVH_leaf_lvl_range_sub(wk, pNode->mRight, lvl + 1);
	}
}

xt_int2 sxGeometryData::find_leaf_level_range() const {
	sxBVHLvlWork wk;
	if (has_BVH()) {
		wk.mpGeo = this;
		BVH_leaf_lvl_range_sub(wk, 0, 0);
	}
	return wk.mLvlRange;
}

int sxGeometryData::find_min_leaf_level() const {
	return find_leaf_level_range()[0];
}

int sxGeometryData::find_max_leaf_level() const {
	return find_leaf_level_range()[1];
}

cxAABB sxGeometryData::calc_world_bbox(cxMtx* pMtxW, int* pIdxMap) const {
	cxAABB bbox = mBBox;
	if (pMtxW) {
		if (has_skin()) {
			cxSphere* pSph = get_skin_sph_top();
			if (pSph) {
				int n = mSkinNodeNum;
				for (int i = 0; i < n; ++i) {
					int idx = i;
					if (pIdxMap) {
						idx = pIdxMap[i];
					}
					cxSphere sph = pSph[i];
					cxVec spos = pMtxW[idx].calc_pnt(sph.get_center());
					cxVec rvec(sph.get_radius());
					cxAABB sbb(spos - rvec, spos + rvec);
					if (i == 0) {
						bbox = sbb;
					} else {
						bbox.merge(sbb);
					}
				}
			}
		} else {
			bbox.transform(*pMtxW);
		}
	}
	return bbox;
}

void sxGeometryData::calc_tangents(cxVec* pTng, bool flip, const char* pAttrName) const {
	if (!pTng) return;
	int npnt = get_pnt_num();
	nxCore::mem_zero((void*)pTng, npnt * sizeof(cxVec));
	if (!is_all_tris()) return;
	int nrmAttrIdx = find_pnt_attr("N");
	if (nrmAttrIdx < 0) return;
	int texAttrIdx = find_pnt_attr(pAttrName ? pAttrName : "uv");
	if (texAttrIdx < 0) return;
	int ntri = get_pol_num();
	cxVec triPts[3];
	xt_texcoord triUVs[3];
	for (int i = 0; i < ntri; ++i) {
		Polygon tri = get_pol(i);
		for (int j = 0; j < 3; ++j) {
			int pid = tri.get_vtx_pnt_id(j);
			triPts[j] = get_pnt(pid);
			float* pUVData = get_attr_data_f(texAttrIdx, eAttrClass::POINT, pid, 2);
			if (pUVData) {
				triUVs[j].set(pUVData[0], pUVData[1]);
			} else {
				triUVs[j].set(0.0f, 0.0f);
			}
		}
		cxVec dp1 = triPts[1] - triPts[0];
		cxVec dp2 = triPts[2] - triPts[0];
		xt_texcoord dt1;
		dt1.set(triUVs[1].u - triUVs[0].u, triUVs[1].v - triUVs[0].v);
		xt_texcoord dt2;
		dt2.set(triUVs[2].u - triUVs[0].u, triUVs[2].v - triUVs[0].v);
		float d = nxCalc::rcp0(dt1.u*dt2.v - dt1.v*dt2.u);
		cxVec tu = (dp1*dt2.v - dp2*dt1.v) * d;
		for (int j = 0; j < 3; ++j) {
			int pid = tri.get_vtx_pnt_id(j);
			pTng[pid] = tu;
		}
	}
	for (int i = 0; i < npnt; ++i) {
		cxVec nrm;
		float* pData = get_attr_data_f(nrmAttrIdx, eAttrClass::POINT, i, 3);
		if (pData) {
			nrm.from_mem(pData);
		} else {
			nrm.zero();
		}
		float d = nrm.dot(pTng[i]);
		nrm.scl(d);
		if (flip) {
			pTng[i] = nrm - pTng[i];
		} else {
			pTng[i].sub(nrm);
		}
		pTng[i].normalize();
	}
}

cxVec* sxGeometryData::calc_tangents(bool flip, const char* pAttrName) const {
	int npnt = get_pnt_num();
	cxVec* pTng = (cxVec*)nxCore::mem_alloc(npnt * sizeof(cxVec), "xGeo:Tng");
	calc_tangents(pTng, flip, pAttrName);
	return pTng;
}

void* sxGeometryData::alloc_triangulation_wk() const {
	void* pWk = nullptr;
	int n = mMaxVtxPerPol;
	if (n > 4) {
		pWk = nxCore::mem_alloc(n * sizeof(int) * 2, "xGeo:TriangWk");
	}
	return pWk;
}

uint8_t* sxGeometryData::Polygon::get_vtx_lst() const {
	uint8_t* pLst = nullptr;
	if (is_valid()) {
		if (mpGeom->is_same_pol_size()) {
			uint32_t offs = mpGeom->mPolOffs;
			if (!mpGeom->is_same_pol_mtl()) {
				offs += mpGeom->get_pol_num() * get_mtl_id_size();
			}
			int vlstSize = mpGeom->get_vtx_idx_size() * mpGeom->mMaxVtxPerPol;
			offs += vlstSize*mPolId;
			pLst = reinterpret_cast<uint8_t*>(XD_INCR_PTR(mpGeom, offs));
		} else {
			uint32_t offs = reinterpret_cast<uint32_t*>(XD_INCR_PTR(mpGeom, mpGeom->mPolOffs))[mPolId];
			pLst = reinterpret_cast<uint8_t*>(XD_INCR_PTR(mpGeom, offs));
		}
	}
	return pLst;
}

int sxGeometryData::Polygon::get_vtx_pnt_id(int vtxIdx) const {
	int pntIdx = -1;
	if (ck_vtx_idx(vtxIdx)) {
		uint8_t* pIdx = get_vtx_lst();
		if (pIdx) {
			int idxSize = mpGeom->get_vtx_idx_size();
			pIdx += idxSize * vtxIdx;
			switch (idxSize) {
				case 1:
					pntIdx = *pIdx;
					break;
				case 2:
					pntIdx = pIdx[0] | (pIdx[1] << 8);
					break;
				case 3:
					pntIdx = pIdx[0] | (pIdx[1] << 8) | (pIdx[2] << 16);
					break;
			}
		}
	}
	return pntIdx;
}

int sxGeometryData::Polygon::get_vtx_num() const {
	int nvtx = 0;
	if (mpGeom && is_valid()) {
		if (mpGeom->is_same_pol_size()) {
			nvtx = mpGeom->mMaxVtxPerPol;
		} else {
			int npol = mpGeom->get_pol_num();
			uint8_t* pNum = reinterpret_cast<uint8_t*>(XD_INCR_PTR(mpGeom, mpGeom->mPolOffs));
			pNum += npol * 4;
			if (!mpGeom->is_same_pol_mtl()) {
				pNum += npol * get_mtl_id_size();
			}
			if (mpGeom->mMaxVtxPerPol < (1 << 8)) {
				nvtx = pNum[mPolId];
			} else {
				pNum += mPolId * 2;
				nvtx = pNum[0] | (pNum[1] << 8);
			}
		}
	}
	return nvtx;
}

int sxGeometryData::Polygon::get_mtl_id() const {
	int mtlId = -1;
	if (mpGeom->get_mtl_num() > 0 && is_valid()) {
		if (mpGeom->is_same_pol_mtl()) {
			mtlId = 0;
		} else {
			uint8_t* pId = reinterpret_cast<uint8_t*>(XD_INCR_PTR(mpGeom, mpGeom->mPolOffs));
			if (!mpGeom->is_same_pol_size()) {
				pId += mpGeom->get_pol_num() * 4;
			}
			switch (get_mtl_id_size()) {
				case 1:
					mtlId = reinterpret_cast<int8_t*>(pId)[mPolId];
					break;
				case 2:
				default:
					mtlId = reinterpret_cast<int16_t*>(pId)[mPolId];
					break;
			}
		}
	}
	return mtlId;
}

cxVec sxGeometryData::Polygon::calc_centroid() const {
	int nvtx = get_vtx_num();
	cxVec c(0.0f);
	for (int i = 0; i < nvtx; ++i) {
		c += get_vtx_pos(i);
	}
	c.scl(nxCalc::rcp0((float)nvtx));
	return c;
}

cxAABB sxGeometryData::Polygon::calc_bbox() const {
	cxAABB bbox;
	bbox.init();
	int nvtx = get_vtx_num();
	for (int i = 0; i < nvtx; ++i) {
		bbox.add_pnt(get_vtx_pos(i));
	}
	return bbox;
}

cxVec sxGeometryData::Polygon::calc_normal_cw() const {
	cxVec nrm;
	nrm.zero();
	cxVec* pPnt = mpGeom->get_pnt_top();
	int nvtx = get_vtx_num();
	for (int i = 0; i < nvtx; ++i) {
		int j = i - 1;
		if (j < 0) j = nvtx - 1;
		int pntI = get_vtx_pnt_id(i);
		int pntJ = get_vtx_pnt_id(j);
		nxGeom::update_nrm_newell(&nrm, &pPnt[pntI], &pPnt[pntJ]);
	}
	nrm.normalize();
	return nrm;
}

cxVec sxGeometryData::Polygon::calc_normal_ccw() const {
	cxVec nrm;
	nrm.zero();
	cxVec* pPnt = mpGeom->get_pnt_top();
	int nvtx = get_vtx_num();
	for (int i = 0; i < nvtx; ++i) {
		int j = i + 1;
		if (j >= nvtx) j = 0;
		int pntI = get_vtx_pnt_id(i);
		int pntJ = get_vtx_pnt_id(j);
		nxGeom::update_nrm_newell(&nrm, &pPnt[pntI], &pPnt[pntJ]);
	}
	nrm.normalize();
	return nrm;
}

bool sxGeometryData::Polygon::is_planar(float eps) const {
	int nvtx = get_vtx_num();
	if (nvtx < 4) return true;
	cxVec c = calc_centroid();
	cxVec n = calc_normal_cw();
	cxPlane pln;
	pln.calc(c, n);
	for (int i = 0; i < nvtx; ++i) {
		if (pln.dist(get_vtx_pos(i)) > eps) return false;
	}
	return true;
}

bool sxGeometryData::Polygon::intersect(const cxLineSeg& seg, cxVec* pHitPos, cxVec* pHitNrm) const {
	int i;
	bool res = false;
	int nvtx = get_vtx_num();
	bool planarFlg = false;
	int cvxMsk = 0;
	cxVec qvtx[4];
	cxVec sp0 = seg.get_pos0();
	cxVec sp1 = seg.get_pos1();
	switch (nvtx) {
		case 3:
			res = nxGeom::seg_tri_intersect_cw(sp0, sp1, get_vtx_pos(0), get_vtx_pos(1), get_vtx_pos(2), pHitPos, pHitNrm);
			break;
		case 4:
			for (i = 0; i < 4; ++i) {
				qvtx[i] = get_vtx_pos(i);
			}
			if (mpGeom->all_quads_planar_convex()) {
				planarFlg = true;
				cvxMsk = 3;
			} else {
				planarFlg = is_planar();
				cvxMsk = nxGeom::quad_convex_ck(qvtx[0], qvtx[1], qvtx[2], qvtx[3]);
			}
			if (planarFlg && cvxMsk == 3) {
				res = nxGeom::seg_quad_intersect_cw(sp0, sp1, qvtx[0], qvtx[1], qvtx[2], qvtx[3], pHitPos, pHitNrm);
			}
			break;
	}
	return res;
}

bool sxGeometryData::Polygon::contains_xz(const cxVec& pos) const {
	if (!is_valid()) return false;
	int cnt = 0;
	float rx0 = pos.x;
	float rz0 = pos.z;
	float rx1 = mpGeom->mBBox.get_max_pos().x + 1.0e-5f;
	float rz1 = rz0;
	int nvtx = get_vtx_num();
	for (int i = 0; i < nvtx; ++i) {
		int j = i + 1;
		if (j >= nvtx) j = 0;
		cxVec p0 = get_vtx_pos(i);
		cxVec p1 = get_vtx_pos(j);
		float ex0 = p0.x;
		float ez0 = p0.z;
		float ex1 = p1.x;
		float ez1 = p1.z;
		if (nxGeom::seg_seg_overlap_2d(rx0, rz0, rx1, rz1, ex0, ez0, ex1, ez1)) {
			++cnt;
		}
	}
	return !!(cnt & 1);
}

int sxGeometryData::Polygon::triangulate(int* pTris, void* pWk) const {
	if (!is_valid()) return 0;
	if (!pTris) return 0;
	int nvtx = get_vtx_num();
	if (nvtx < 3) return 0;
	if (nvtx == 3) {
		for (int i = 0; i < 3; ++i) {
			pTris[i] = get_vtx_pnt_id(i);
		}
		return 1;
	}
	struct Lnk {
		int prev;
		int next;
	} lnk[4];
	Lnk* pLnk = lnk;
	if (nvtx > (int)XD_ARY_LEN(lnk)) {
		if (pWk) {
			pLnk = (Lnk*)pWk;
		} else {
			pLnk = (Lnk*)nxCore::mem_alloc(nvtx * sizeof(Lnk), "xGeo:TriangLnk");
		}
	}
	for (int i = 0; i < nvtx; ++i) {
		pLnk[i].prev = i > 0 ? i - 1 : nvtx - 1;
		pLnk[i].next = i < nvtx - 1 ? i + 1 : 0;
	}
	cxVec nrm = calc_normal_cw();
	int vcnt = nvtx;
	int tptr = 0;
	int idx = 0;
	while (vcnt > 3) {
		int prev = pLnk[idx].prev;
		int next = pLnk[idx].next;
		cxVec v0 = get_vtx_pos(prev);
		cxVec v1 = get_vtx_pos(idx);
		cxVec v2 = get_vtx_pos(next);
		cxVec tnrm = nxGeom::tri_normal_cw(v0, v1, v2);
		bool flg = tnrm.dot(nrm) >= 0.0f;
		if (flg) {
			next = pLnk[next].next;
			do {
				cxVec v = get_vtx_pos(next);
				if (nxGeom::pnt_in_tri(v, v0, v1, v2)) {
					flg = false;
					break;
				}
				next = pLnk[next].next;
			} while (next != prev);
		}
		if (flg) {
			pTris[tptr++] = pLnk[idx].prev;
			pTris[tptr++] = idx;
			pTris[tptr++] = pLnk[idx].next;
			pLnk[pLnk[idx].prev].next = pLnk[idx].next;
			pLnk[pLnk[idx].next].prev = pLnk[idx].prev;
			--vcnt;
			idx = pLnk[idx].prev;
		} else {
			idx = pLnk[idx].next;
		}
	}
	pTris[tptr++] = pLnk[idx].prev;
	pTris[tptr++] = idx;
	pTris[tptr++] = pLnk[idx].next;
	if (pLnk != lnk && !pWk) {
		nxCore::mem_free(pLnk);
		pLnk = nullptr;
	}
	return nvtx - 2;
}

int sxGeometryData::Polygon::tri_count() const {
	if (!is_valid()) return 0;
	int nvtx = get_vtx_num();
	if (nvtx < 3) return 0;
	return nvtx - 2;
}


void* sxGeometryData::Group::get_idx_top() const {
	void* pIdxTop = is_valid() ? mpInfo + 1 : nullptr;
	if (pIdxTop) {
		int nskn = mpInfo->mSkinNodeNum;
		if (nskn) {
			if (mpGeom->has_skin_spheres()) {
				pIdxTop = XD_INCR_PTR(pIdxTop, sizeof(cxSphere)*nskn); /* skip spheres */
			}
			pIdxTop = XD_INCR_PTR(pIdxTop, sizeof(uint16_t)*nskn); /* skip node list */
		}
	}
	return pIdxTop;
}

int sxGeometryData::Group::get_idx_elem_size() const {
	int idxSpan = mpInfo->mMaxIdx - mpInfo->mMinIdx;
	int res = 0;
	if (idxSpan < (1 << 8)) {
		res = 1;
	} else if (idxSpan < (1 << 16)) {
		res = 2;
	} else {
		res = 3;
	}
	return res;
}

int sxGeometryData::Group::get_rel_idx(int at) const {
	int idx = -1;
	if (ck_idx(at)) {
		void* pIdxTop = get_idx_top();
		int elemSize = get_idx_elem_size();
		if (elemSize == 1) {
			idx = reinterpret_cast<uint8_t*>(pIdxTop)[at];
		} else if (elemSize == 2) {
			idx = reinterpret_cast<uint16_t*>(pIdxTop)[at];
		} else {
			uint8_t* pIdx = reinterpret_cast<uint8_t*>(XD_INCR_PTR(pIdxTop, at * 3));
			idx = pIdx[0] | (pIdx[1] << 8) | (pIdx[2] << 16);
		}
	}
	return idx;
}

int sxGeometryData::Group::get_idx(int at) const {
	int idx = -1;
	if (ck_idx(at)) {
		idx = get_rel_idx(at) + mpInfo->mMinIdx;
	}
	return idx;
}

bool sxGeometryData::Group::contains(int idx) const {
	bool res = false;
	if (is_valid() && idx >= mpInfo->mMinIdx && idx <= mpInfo->mMaxIdx) {
		int bit = idx - mpInfo->mMinIdx;
		void* pIdxTop = get_idx_top();
		int idxElemSize = get_idx_elem_size();
		uint8_t* pBits = reinterpret_cast<uint8_t*>(pIdxTop) + mpInfo->mIdxNum*idxElemSize;
		res = !!(pBits[bit >> 3] & (1 << (bit & 7)));
	}
	return res;
}

cxSphere* sxGeometryData::Group::get_skin_spheres() const {
	cxSphere* pSph = nullptr;
	if (is_valid()) {
		int nskn = mpInfo->mSkinNodeNum;
		if (nskn) {
			if (mpGeom->has_skin_spheres()) {
				pSph = (cxSphere*)(mpInfo + 1);
			}
		}
	}
	return pSph;
}

uint16_t* sxGeometryData::Group::get_skin_ids() const {
	uint16_t* pLst = nullptr;
	if (is_valid()) {
		int nskn = mpInfo->mSkinNodeNum;
		if (nskn) {
			if (mpGeom->has_skin_spheres()) {
				pLst = (uint16_t*)XD_INCR_PTR(mpInfo + 1, sizeof(cxSphere)*nskn);
			} else {
				pLst = (uint16_t*)(mpInfo + 1);
			}
		}
	}
	return pLst;
}

void sxGeometryData::DisplayList::create(const sxGeometryData& geo, const char* pBatGrpPrefix, bool triangulate) {
	int nmtl = geo.get_mtl_num();
	int nbat = nmtl > 0 ? nmtl : 1;
	int batGrpCount = 0;
	int* pBatGrpIdx = nullptr;
	int* pPolTriLst = nullptr;
	void* pPolTriWk = nullptr;
	if (geo.mMaxVtxPerPol > 3) {
		pPolTriLst = (int*)nxCore::mem_alloc((geo.mMaxVtxPerPol - 2) * 3 * sizeof(int), "xGeo:DispTrisTmp");
		pPolTriWk = geo.alloc_triangulation_wk();
	}
	if (pBatGrpPrefix) {
		for (int i = 0; i < geo.get_pol_grp_num(); ++i) {
			Group polGrp = geo.get_pol_grp(i);
			if (polGrp.is_valid()) {
				const char* pPolGrpName = polGrp.get_name();
				if (nxCore::str_starts_with(pPolGrpName, pBatGrpPrefix)) {
					++batGrpCount;
				}
			}
		}
		if (batGrpCount > 0) {
			nbat = batGrpCount;
			pBatGrpIdx = reinterpret_cast<int*>(nxCore::mem_alloc(batGrpCount * sizeof(int), "xGeo:DispBatsTmp"));
			batGrpCount = 0;
			if (pBatGrpIdx) {
				for (int i = 0; i < geo.get_pol_grp_num(); ++i) {
					Group polGrp = geo.get_pol_grp(i);
					if (polGrp.is_valid()) {
						const char* pPolGrpName = polGrp.get_name();
						if (nxCore::str_starts_with(pPolGrpName, pBatGrpPrefix)) {
							pBatGrpIdx[batGrpCount] = i;
							++batGrpCount;
						}
					}
				}
			}
		}
	}
	int ntri = 0;
	int nidx16 = 0;
	int nidx32 = 0;
	for (int i = 0; i < nbat; ++i) {
		int npol = 0;
		if (batGrpCount > 0) {
			int batGrpIdx = pBatGrpIdx[i];
			Group batGrp = geo.get_pol_grp(batGrpIdx);
			npol = batGrp.get_idx_num();
		} else {
			if (nmtl > 0) {
				Group mtlGrp = geo.get_mtl_grp(i);
				npol = mtlGrp.get_idx_num();
			} else {
				npol = geo.get_pol_num();
			}
		}
		int32_t minIdx = 0;
		int32_t maxIdx = 0;
		int batIdxCnt = 0;
		for (int j = 0; j < npol; ++j) {
			int polIdx = 0;
			if (batGrpCount > 0) {
				int batGrpIdx = pBatGrpIdx[i];
				Group batGrp = geo.get_pol_grp(batGrpIdx);
				polIdx = batGrp.get_idx(j);
			} else {
				if (nmtl > 0) {
					polIdx = geo.get_mtl_grp(i).get_idx(j);
				} else {
					polIdx = j;
				}
			}
			Polygon pol = geo.get_pol(polIdx);
			if (pol.is_tri() || (triangulate && pol.tri_count() > 0)) {
				for (int k = 0; k < pol.get_vtx_num(); ++k) {
					int32_t vtxId = pol.get_vtx_pnt_id(k);
					if (batIdxCnt > 0) {
						minIdx = nxCalc::min(minIdx, vtxId);
						maxIdx = nxCalc::max(maxIdx, vtxId);
					} else {
						minIdx = vtxId;
						maxIdx = vtxId;
					}
				}
				batIdxCnt += pol.tri_count() * 3;
				ntri += pol.tri_count();
			}
		}
		if ((maxIdx - minIdx) < (1 << 16)) {
			nidx16 += batIdxCnt;
		} else {
			nidx32 += batIdxCnt;
		}
	}
	size_t size = XD_ALIGN(sizeof(Data), 0x10);
	uint32_t offsBat = (uint32_t)size;
	size += XD_ALIGN(nbat * sizeof(Batch), 0x10);
	uint32_t offsIdx32 = nidx32 > 0 ? (uint32_t)size : 0;
	size += XD_ALIGN(nidx32 * sizeof(uint32_t), 0x10);
	uint32_t offsIdx16 = nidx16 > 0 ? (uint32_t)size : 0;
	size += XD_ALIGN(nidx16 * sizeof(uint16_t), 0x10);
	Data* pData = (Data*)nxCore::mem_alloc(size, "xGeo:DispLstData");
	if (!pData) {
		return;
	}
	nxCore::mem_zero(pData, size);
	pData->mOffsBat = offsBat;
	pData->mOffsIdx32 = offsIdx32;
	pData->mOffsIdx16 = offsIdx16;
	pData->mIdx16Num = nidx16;
	pData->mIdx32Num = nidx32;
	pData->mMtlNum = nmtl;
	pData->mBatNum = nbat;
	pData->mTriNum = ntri;
	mpGeo = &geo;
	mpData = pData;
	nidx16 = 0;
	nidx32 = 0;
	for (int i = 0; i < nbat; ++i) {
		Batch* pBat = get_bat(i);
		int npol = 0;
		if (pBat) {
			if (batGrpCount > 0) {
				int batGrpIdx = pBatGrpIdx[i];
				Group batGrp = geo.get_pol_grp(batGrpIdx);
				pBat->mMtlId = geo.get_pol(batGrp.get_idx(0)).get_mtl_id();
				if (geo.has_skin()) {
					pBat->mMaxWgtNum = batGrp.get_max_wgt_num();
				}
				npol = batGrp.get_idx_num();
			} else {
				pBat->mGrpId = -1;
				if (nmtl > 0) {
					Group mtlGrp = geo.get_mtl_grp(i);
					pBat->mMtlId = i;
					if (geo.has_skin()) {
						pBat->mMaxWgtNum = mtlGrp.get_max_wgt_num();
					}
					npol = mtlGrp.get_idx_num();
				} else {
					pBat->mMtlId = -1;
					if (geo.has_skin()) {
						pBat->mMaxWgtNum = geo.mMaxSkinWgtNum;
					}
					npol = geo.get_pol_num();
				}
			}
			int batIdxCnt = 0;
			pBat->mTriNum = 0;
			for (int j = 0; j < npol; ++j) {
				int polIdx = 0;
				if (batGrpCount > 0) {
					int batGrpIdx = pBatGrpIdx[i];
					Group batGrp = geo.get_pol_grp(batGrpIdx);
					polIdx = batGrp.get_idx(j);
				} else {
					if (nmtl > 0) {
						polIdx = geo.get_mtl_grp(i).get_idx(j);
					} else {
						polIdx = j;
					}
				}
				Polygon pol = geo.get_pol(polIdx);
				if (pol.is_tri() || (triangulate && pol.tri_count() > 0)) {
					for (int k = 0; k < pol.get_vtx_num(); ++k) {
						int32_t vtxId = pol.get_vtx_pnt_id(k);
						if (batIdxCnt > 0) {
							pBat->mMinIdx = nxCalc::min(pBat->mMinIdx, vtxId);
							pBat->mMaxIdx = nxCalc::max(pBat->mMaxIdx, vtxId);
						} else {
							pBat->mMinIdx = vtxId;
							pBat->mMaxIdx = vtxId;
						}
					}
					batIdxCnt += pol.tri_count() * 3;
					pBat->mTriNum += pol.tri_count();
				}
			}
			if (pBat->is_idx16()) {
				pBat->mIdxOrg = nidx16;
				nidx16 += batIdxCnt;
			} else {
				pBat->mIdxOrg = nidx32;
				nidx32 += batIdxCnt;
			}
		}
	}
	nidx16 = 0;
	nidx32 = 0;
	for (int i = 0; i < nbat; ++i) {
		Batch* pBat = get_bat(i);
		int npol = 0;
		if (pBat) {
			if (batGrpCount > 0) {
				int batGrpIdx = pBatGrpIdx[i];
				Group batGrp = geo.get_pol_grp(batGrpIdx);
				npol = batGrp.get_idx_num();
			} else {
				pBat->mGrpId = -1;
				if (nmtl > 0) {
					Group mtlGrp = geo.get_mtl_grp(i);
					npol = mtlGrp.get_idx_num();
				} else {
					npol = geo.get_pol_num();
				}
			}
			for (int j = 0; j < npol; ++j) {
				int polIdx = 0;
				if (batGrpCount > 0) {
					int batGrpIdx = pBatGrpIdx[i];
					Group batGrp = geo.get_pol_grp(batGrpIdx);
					polIdx = batGrp.get_idx(j);
				} else {
					if (nmtl > 0) {
						polIdx = geo.get_mtl_grp(i).get_idx(j);
					} else {
						polIdx = j;
					}
				}
				Polygon pol = geo.get_pol(polIdx);
				if (pol.is_tri() || (triangulate && pol.tri_count() > 0)) {
					if (pBat->is_idx16()) {
						uint16_t* pIdx16 = pData->get_idx16();
						if (pIdx16) {
							if (pol.is_tri()) {
								for (int k = 0; k < 3; ++k) {
									pIdx16[nidx16 + k] = (uint16_t)(pol.get_vtx_pnt_id(k) - pBat->mMinIdx);
								}
								nidx16 += 3;
							} else {
								int ntris = pol.triangulate(pPolTriLst, pPolTriWk);
								for (int t = 0; t < ntris; ++t) {
									for (int k = 0; k < 3; ++k) {
										pIdx16[nidx16 + k] = (uint16_t)(pol.get_vtx_pnt_id(pPolTriLst[t*3 + k]) - pBat->mMinIdx);
									}
									nidx16 += 3;
								}
							}
						}
					} else {
						uint32_t* pIdx32 = pData->get_idx32();
						if (pIdx32) {
							if (pol.is_tri()) {
								for (int k = 0; k < 3; ++k) {
									pIdx32[nidx32 + k] = (uint32_t)(pol.get_vtx_pnt_id(k) - pBat->mMinIdx);
								}
								nidx32 += 3;
							} else {
								int ntris = pol.triangulate(pPolTriLst, pPolTriWk);
								for (int t = 0; t < ntris; ++t) {
									for (int k = 0; k < 3; ++k) {
										pIdx32[nidx32 + k] = (uint32_t)(pol.get_vtx_pnt_id(pPolTriLst[t*3 + k]) - pBat->mMinIdx);
									}
									nidx32 += 3;
								}
							}
						}
					}
				}
			}
		}
	}

	if (pBatGrpIdx) {
		nxCore::mem_free(pBatGrpIdx);
		pBatGrpIdx = nullptr;
	}
	if (pPolTriLst) {
		nxCore::mem_free(pPolTriLst);
		pPolTriLst = nullptr;
	}
	if (pPolTriWk) {
		nxCore::mem_free(pPolTriWk);
		pPolTriWk = nullptr;
	}
}

void sxGeometryData::DisplayList::destroy() {
	if (mpData) {
		nxCore::mem_free(mpData);
	}
	mpData = nullptr;
	mpGeo = nullptr;
}

const char* sxGeometryData::DisplayList::get_bat_mtl_name(int batIdx) const {
	const char* pMtlName = nullptr;
	if (is_valid()) {
		Batch* pBat = get_bat(batIdx);
		if (pBat) {
			pMtlName = mpGeo->get_mtl_name(pBat->mMtlId);
		}
	}
	return pMtlName;
}


void sxDDSHead::init(uint32_t w, uint32_t h) {
	nxCore::mem_zero(this, sizeof(sxDDSHead));
	mMagic = XD_FOURCC('D', 'D', 'S', ' ');
	mSize = sizeof(sxDDSHead) - 4;
	mHeight = h;
	mWidth = w;
}

void sxDDSHead::init_dds128(uint32_t w, uint32_t h) {
	init(w, h);
	mFlags = 0x081007;
	mPitchLin = w * h * (sizeof(float) * 4);
	mFormat.mSize = 0x20;
	mFormat.mFlags = 4;
	mFormat.mFourCC = 0x74;
	mCaps1 = 0x1000;
}

void sxDDSHead::init_dds64(uint32_t w, uint32_t h) {
	init(w, h);
	mFlags = 0x081007;
	mPitchLin = w * h * (sizeof(xt_half) * 4);
	mFormat.mSize = 0x20;
	mFormat.mFlags = 4;
	mFormat.mFourCC = 0x71;
	mCaps1 = 0x1000;
}

void sxDDSHead::init_dds32(uint32_t w, uint32_t h, bool encRGB) {
	init(w, h);
	mFlags = 0x081007;
	mPitchLin = w * h * (sizeof(uint8_t) * 4);
	mFormat.mSize = 0x20;
	mFormat.mFlags = 0x41;
	mFormat.mBitCount = 0x20;
	if (encRGB) {
		mFormat.mMaskR = 0xFF << 0;
		mFormat.mMaskG = 0xFF << 8;
		mFormat.mMaskB = 0xFF << 16;
		mFormat.mMaskA = 0xFF << 24;
	} else {
		mFormat.mMaskR = 0xFF << 16;
		mFormat.mMaskG = 0xFF << 8;
		mFormat.mMaskB = 0xFF << 0;
		mFormat.mMaskA = 0xFF << 24;
	}
	mCaps1 = 0x1000;
}


namespace nxImage {

sxDDSHead* alloc_dds128(uint32_t w, uint32_t h, uint32_t* pSize) {
	sxDDSHead* pDDS = nullptr;
	uint32_t npix = w * h;
	if ((int)npix > 0) {
		uint32_t dataSize = npix * sizeof(cxColor);
		uint32_t size = sizeof(sxDDSHead) + dataSize;
		pDDS = reinterpret_cast<sxDDSHead*>(nxCore::mem_alloc(size, "xDDS"));
		if (pDDS) {
			pDDS->init_dds128(w, h);
			if (pSize) {
				*pSize = size;
			}
		}
	}
	return pDDS;
}

void save_dds128(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h) {
#if XD_FILEFUNCS_ENABLED
	uint32_t npix = w*h;
	if ((int)npix > 0) {
		FILE* pOut = nxSys::fopen_w_bin(pPath);
		if (pOut) {
			sxDDSHead dds = {};
			dds.init_dds128(w, h);
			::fwrite(&dds, sizeof(dds), 1, pOut);
			::fwrite(pClr, sizeof(cxColor), npix, pOut);
			::fclose(pOut);
		}
	}
#endif
}

void save_dds64(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h) {
#if XD_FILEFUNCS_ENABLED
	uint32_t npix = w*h;
	if ((int)npix > 0) {
		FILE* pOut = nxSys::fopen_w_bin(pPath);
		if (pOut) {
			sxDDSHead dds = {};
			dds.init_dds64(w, h);
			::fwrite(&dds, sizeof(dds), 1, pOut);
			xt_half4 h;
			for (uint32_t i = 0; i < npix; ++i) {
				h.set(pClr[i].r, pClr[i].g, pClr[i].b, pClr[i].a);
				::fwrite(&h, sizeof(h), 1, pOut);
			}
			::fclose(pOut);
		}
	}
#endif
}

void save_dds32_rgbe(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h) {
#if XD_FILEFUNCS_ENABLED
	uint32_t npix = w*h;
	if ((int)npix > 0) {
		FILE* pOut = nxSys::fopen_w_bin(pPath);
		if (pOut) {
			sxDDSHead dds = {};
			dds.init_dds32(w, h);
			dds.set_encoding(sxDDSHead::ENC_RGBE);
			::fwrite(&dds, sizeof(dds), 1, pOut);
			for (uint32_t i = 0; i < npix; ++i) {
				uint32_t e = pClr[i].encode_rgbe();
				::fwrite(&e, sizeof(e), 1, pOut);
			}
			::fclose(pOut);
		}
	}
#endif
}

void save_dds32_bgre(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h) {
#if XD_FILEFUNCS_ENABLED
	uint32_t npix = w*h;
	if ((int)npix > 0) {
		FILE* pOut = nxSys::fopen_w_bin(pPath);
		if (pOut) {
			sxDDSHead dds = {};
			dds.init_dds32(w, h, false);
			dds.set_encoding(sxDDSHead::ENC_BGRE);
			::fwrite(&dds, sizeof(dds), 1, pOut);
			for (uint32_t i = 0; i < npix; ++i) {
				uint32_t e = pClr[i].encode_bgre();
				::fwrite(&e, sizeof(e), 1, pOut);
			}
			::fclose(pOut);
		}
	}
#endif
}

void save_dds32_rgbi(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h) {
#if XD_FILEFUNCS_ENABLED
	uint32_t npix = w*h;
	if ((int)npix > 0) {
		FILE* pOut = nxSys::fopen_w_bin(pPath);
		if (pOut) {
			sxDDSHead dds = {};
			dds.init_dds32(w, h);
			dds.set_encoding(sxDDSHead::ENC_RGBI);
			::fwrite(&dds, sizeof(dds), 1, pOut);
			for (uint32_t i = 0; i < npix; ++i) {
				uint32_t e = pClr[i].encode_rgbi();
				::fwrite(&e, sizeof(e), 1, pOut);
			}
			::fclose(pOut);
		}
	}
#endif
}

void save_dds32_bgri(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h) {
#if XD_FILEFUNCS_ENABLED
	uint32_t npix = w*h;
	if ((int)npix > 0) {
		FILE* pOut = nxSys::fopen_w_bin(pPath);
		if (pOut) {
			sxDDSHead dds = {};
			dds.init_dds32(w, h, false);
			dds.set_encoding(sxDDSHead::ENC_BGRI);
			::fwrite(&dds, sizeof(dds), 1, pOut);
			for (uint32_t i = 0; i < npix; ++i) {
				uint32_t e = pClr[i].encode_bgri();
				::fwrite(&e, sizeof(e), 1, pOut);
			}
			::fclose(pOut);
		}
	}
#endif
}

void save_dds32_rgba8(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h, float gamma) {
#if XD_FILEFUNCS_ENABLED
	uint32_t npix = w*h;
	if ((int)npix > 0) {
		FILE* pOut = nxSys::fopen_w_bin(pPath);
		if (pOut) {
			sxDDSHead dds = {};
			dds.init_dds32(w, h);
			if (gamma > 0.0f) {
				dds.set_gamma(gamma);
			} else {
				dds.set_gamma(1.0f);
			}
			::fwrite(&dds, sizeof(dds), 1, pOut);
			if (gamma > 0.0f && gamma != 1.0f) {
				float invg = 1.0f / gamma;
				for (uint32_t i = 0; i < npix; ++i) {
					cxColor cc = pClr[i];
					for (int j = 0; j < 3; ++j) {
						if (cc[j] > 0.0f) {
							cc[j] = ::mth_powf(cc[j], invg);
						} else {
							cc[j] = 0.0f;
						}
					}
					uint32_t e = cc.encode_rgba8();
					::fwrite(&e, sizeof(e), 1, pOut);
				}
			} else {
				for (uint32_t i = 0; i < npix; ++i) {
					uint32_t e = pClr[i].encode_rgba8();
					::fwrite(&e, sizeof(e), 1, pOut);
				}
			}
			::fclose(pOut);
		}
	}
#endif
}

void save_dds32_bgra8(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h, float gamma) {
#if XD_FILEFUNCS_ENABLED
	uint32_t npix = w*h;
	if ((int)npix > 0) {
		FILE* pOut = nxSys::fopen_w_bin(pPath);
		if (pOut) {
			sxDDSHead dds = {};
			dds.init_dds32(w, h, false);
			if (gamma > 0.0f) {
				dds.set_gamma(gamma);
			} else {
				dds.set_gamma(1.0f);
			}
			::fwrite(&dds, sizeof(dds), 1, pOut);
			if (gamma > 0.0f && gamma != 1.0f) {
				float invg = 1.0f / gamma;
				for (uint32_t i = 0; i < npix; ++i) {
					cxColor cc = pClr[i];
					for (int j = 0; j < 3; ++j) {
						if (cc[j] > 0.0f) {
							cc[j] = ::mth_powf(cc[j], invg);
						} else {
							cc[j] = 0.0f;
						}
					}
					uint32_t e = cc.encode_bgra8();
					::fwrite(&e, sizeof(e), 1, pOut);
				}
			} else {
				for (uint32_t i = 0; i < npix; ++i) {
					uint32_t e = pClr[i].encode_bgra8();
					::fwrite(&e, sizeof(e), 1, pOut);
				}
			}
			::fclose(pOut);
		}
	}
#endif
}

cxColor* decode_dds(sxDDSHead* pDDS, uint32_t* pWidth, uint32_t* pHeight, float gamma) {
	if (!pDDS) return nullptr;
	int w = pDDS->mWidth;
	int h = pDDS->mHeight;
	int n = w * h;
	size_t size = n * sizeof(cxColor);
	if (pWidth) {
		*pWidth = w;
	}
	if (pHeight) {
		*pHeight = h;
	}
	cxColor* pClr = (cxColor*)nxCore::mem_alloc(size, "xDDS:Clr");
	if (pClr) {
		if (pDDS->is_dds128()) {
			nxCore::mem_copy((void*)pClr, pDDS + 1, size);
		} else if (pDDS->is_dds64()) {
			xt_half4* pHalf = (xt_half4*)(pDDS + 1);
			xt_float4* pFloat = (xt_float4*)pClr;
			for (int i = 0; i < n; ++i) {
				pFloat[i] = pHalf[i].get();
			}
		} else if (pDDS->is_dds32()) {
			if (pDDS->get_encoding() == sxDDSHead::ENC_RGBE) {
				uint32_t* pRGBE = (uint32_t*)(pDDS + 1);
				for (int i = 0; i < n; ++i) {
					pClr[i].decode_rgbe(pRGBE[i]);
				}
			} else if (pDDS->get_encoding() == sxDDSHead::ENC_BGRE) {
				uint32_t* pBGRE = (uint32_t*)(pDDS + 1);
				for (int i = 0; i < n; ++i) {
					pClr[i].decode_bgre(pBGRE[i]);
				}
			} else if (pDDS->get_encoding() == sxDDSHead::ENC_RGBI) {
				uint32_t* pRGBI = (uint32_t*)(pDDS + 1);
				for (int i = 0; i < n; ++i) {
					pClr[i].decode_rgbi(pRGBI[i]);
				}
			} else if (pDDS->get_encoding() == sxDDSHead::ENC_BGRI) {
				uint32_t* pBGRI = (uint32_t*)(pDDS + 1);
				for (int i = 0; i < n; ++i) {
					pClr[i].decode_bgri(pBGRI[i]);
				}
			} else {
				float wg = pDDS->get_gamma();
				if (wg <= 0.0f) wg = gamma;
				uint32_t maskR = pDDS->mFormat.mMaskR;
				uint32_t maskG = pDDS->mFormat.mMaskG;
				uint32_t maskB = pDDS->mFormat.mMaskB;
				uint32_t maskA = pDDS->mFormat.mMaskA;
				uint32_t shiftR = nxCore::ctz32(maskR) & 0x1F;
				uint32_t shiftG = nxCore::ctz32(maskG) & 0x1F;
				uint32_t shiftB = nxCore::ctz32(maskB) & 0x1F;
				uint32_t shiftA = nxCore::ctz32(maskA) & 0x1F;
				uint32_t* pEnc32 = (uint32_t*)(pDDS + 1);
				for (int i = 0; i < n; ++i) {
					uint32_t enc = pEnc32[i];
					uint32_t ir = (enc & maskR) >> shiftR;
					uint32_t ig = (enc & maskG) >> shiftG;
					uint32_t ib = (enc & maskB) >> shiftB;
					uint32_t ia = (enc & maskA) >> shiftA;
					pClr[i].set((float)(int)ir, (float)(int)ig, (float)(int)ib, (float)(int)ia);
				}
				float scl[4];
				scl[0] = nxCalc::rcp0((float)(int)(maskR >> shiftR));
				scl[1] = nxCalc::rcp0((float)(int)(maskG >> shiftG));
				scl[2] = nxCalc::rcp0((float)(int)(maskB >> shiftB));
				scl[3] = nxCalc::rcp0((float)(int)(maskA >> shiftA));
				for (int i = 0; i < n; ++i) {
					float* pDst = (float*)(pClr + i);
					for (int j = 0; j < 4; ++j) {
						pDst[j] *= scl[j];
					}
				}
				if (wg > 0.0f) {
					for (int i = 0; i < n; ++i) {
						float* pDst = (float*)(pClr + i);
						for (int j = 0; j < 4; ++j) {
							pDst[j] = ::mth_powf(pDst[j], wg);
						}
					}
				}
			}
		} else {
			nxSys::dbgmsg("unsupported DDS format\n");
			for (int i = 0; i < n; ++i) {
				pClr[i].zero_rgb();
			}
		}
	}
	return pClr;
}

void save_sgi(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h, float gamma) {
#if XD_FILEFUNCS_ENABLED
	if (!pPath || !pClr || (w*h == 0)) return;
	FILE* pOut = nxSys::fopen_w_bin(pPath);
	if (!pOut) return;
	size_t size = w * h * 4;
	if (size < 0x200) size = 0x200;
	void* pMem = nxCore::mem_alloc(size, "xSGI:Tmp");
	if (pMem) {
		nxCore::mem_zero(pMem, size);
		uint8_t* pHead = (uint8_t*)pMem;
		pHead[0] = 1;
		pHead[1] = 0xDA;
		pHead[3] = 1;
		pHead[5] = 3;
		pHead[0xB] = 4;
		pHead[0x13] = 0xFF;
		pHead[6] = (uint8_t)((w >> 8) & 0xFF);
		pHead[7] = (uint8_t)(w & 0xFF);
		pHead[8] = (uint8_t)((h >> 8) & 0xFF);
		pHead[9] = (uint8_t)(h & 0xFF);
		::fwrite(pHead, 0x200, 1, pOut);
		uint8_t* pDst = (uint8_t*)pMem;
		bool gflg = gamma > 0.0f && gamma != 1.0f;
		float invg = gflg ? 1.0f / gamma : 1.0f;
		int offs = int(w*h);
		int idst = 0;
		for (int y = int(h); --y >= 0;) {
			for (int x = 0; x < int(w); ++x) {
				uint32_t idx = y*w + x;
				cxColor c = pClr[idx];
				if (gflg) {
					for (int i = 0; i < 3; ++i) {
						if (c[i] > 0.0f) {
							c[i] = ::mth_powf(c[i], invg);
						} else {
							c[i] = 0.0f;
						}
					}
				}
				uint32_t ic = c.encode_rgba8();
				pDst[idst] = uint8_t(ic & 0xFF);
				pDst[idst + offs] = uint8_t((ic >> 8) & 0xFF);
				pDst[idst + offs*2] = uint8_t((ic >> 16) & 0xFF);
				pDst[idst + offs*3] = uint8_t((ic >> 24) & 0xFF);
				++idst;
			}
		}
		::fwrite(pMem, w*h*4, 1, pOut);
		nxCore::mem_free(pMem);
	}
	::fclose(pOut);
#endif
}

void save_hdr(const char* pPath, const cxColor* pClr, uint32_t w, uint32_t h, float gamma, float exposure) {
#if XD_FILEFUNCS_ENABLED
	size_t n = w * h;
	if (!pPath || !pClr || !n) return;
	FILE* pOut = nxSys::fopen_w_bin(pPath);
	if (!pOut) return;
	::fprintf(pOut, "#?RADIANCE\n# crosscore\nFORMAT=%s\nGAMMA=%f\nEXPOSURE=%f\n",
		"32-bit_rle_rgbe", gamma, exposure);
	::fprintf(pOut, "\n-Y %d +X %d\n", h, w);
	bool gflg = gamma > 0.0f && gamma != 1.0f;
	float invg = 1.0f;
	if (gflg) invg = 1.0f / gamma;
	for (size_t i = 0; i < n; ++i) {
		cxColor c = pClr[i];
		if (gflg) {
			for (int j = 0; j < 3; ++j) {
				if (c[j] > 0.0f) {
					c[j] = ::mth_powf(c[j], invg);
				} else {
					c[j] = 0.0f;
				}
			}
		} else {
			c.clip_neg();
		}
		uint32_t rgbe = c.encode_rgbe();
		::fwrite(&rgbe, sizeof(rgbe), 1, pOut);
	}
	::fclose(pOut);
#endif
}

/* see PBRT book for details */
void calc_resample_wgts(int oldRes, int newRes, xt_float4* pWgt, int16_t* pOrg) {
	float rt = float(oldRes) / float(newRes);
	float fw = 2.0f;
	for (int i = 0; i < newRes; ++i) {
		float c = (float(i) + 0.5f) * rt;
		float org = ::mth_floorf((c - fw) + 0.5f);
		pOrg[i] = (int16_t)org;
		float* pW = pWgt[i];
		float s = 0.0f;
		for (int j = 0; j < 4; ++j) {
			float pos = org + float(j) + 0.5f;
			float x = ::mth_fabsf((pos - c) / fw);
			float w;
			if (x < 1.0e-5f) {
				w = 1.0f;
			} else if (x > 1.0f) {
				w = 1.0f;
			} else {
				x *= XD_PI;
				w = nxCalc::sinc(x*2.0f) * nxCalc::sinc(x);
			}
			pW[j] = w;
			s += w;
		}
		s = nxCalc::rcp0(s);
		pWgt[i].scl(s);
	}
}

/* VC: must be noinline to work around AVX/x64 optimizer bug */
static XD_NOINLINE cxColor* clr_dup(cxColor* pDst, const cxColor c, int num) {
	for (int i = 0; i < num; ++i) {
		*pDst++ = c;
	}
	return pDst;
}

cxColor* upscale(const cxColor* pSrc, int srcW, int srcH, int xscl, int yscl, bool filt, cxColor* pDstBuff, void* pWrkMem, bool cneg) {
	if (!pSrc) return nullptr;
	int w = srcW * xscl;
	int h = srcH * yscl;
	if (w <= 0 || h <= 0) return nullptr;
	cxColor* pDst = pDstBuff;
	if (!pDstBuff) {
		pDst = reinterpret_cast<cxColor*>(nxCore::mem_alloc(w * h * sizeof(cxColor), "xImg:UpClrs"));
	}
	if (!pDst) return nullptr;
	if (filt) {
		int wgtNum = nxCalc::max(w, h);
		xt_float4* pWgt;
		cxColor* pTmp;
		int16_t* pOrg;
		if (pWrkMem) {
			pWgt = reinterpret_cast<xt_float4*>(pWrkMem);
			pTmp = reinterpret_cast<cxColor*>(XD_INCR_PTR(pWgt, wgtNum * sizeof(xt_float4)));
			pOrg = reinterpret_cast<int16_t*>(XD_INCR_PTR(pTmp, wgtNum * sizeof(cxColor)));
		} else {
			pWgt = reinterpret_cast<xt_float4*>(nxCore::mem_alloc(wgtNum * sizeof(xt_float4), "xImg:UpTmpW"));
			pTmp = reinterpret_cast<cxColor*>(nxCore::mem_alloc(wgtNum * sizeof(cxColor), "xImg:UpTmp"));
			pOrg = reinterpret_cast<int16_t*>(nxCore::mem_alloc(wgtNum * sizeof(int16_t), "xImg:UpTmpO"));
		}
		nxImage::calc_resample_wgts(srcW, w, pWgt, pOrg);
		for (int y = 0; y < srcH; ++y) {
			for (int x = 0; x < w; ++x) {
				cxColor clr;
				clr.zero();
				xt_float4 wgt = pWgt[x];
				for (int i = 0; i < 4; ++i) {
					int x0 = nxCalc::clamp(pOrg[x] + i, 0, srcW - 1);
					cxColor csrc = pSrc[y*srcW + x0];
					csrc.scl(wgt[i]);
					clr.add(csrc);
				}
				pDst[y*w + x] = clr;
			}
		}
		nxImage::calc_resample_wgts(srcH, h, pWgt, pOrg);
		for (int x = 0; x < w; ++x) {
			for (int y = 0; y < h; ++y) {
				cxColor clr;
				clr.zero();
				xt_float4 wgt = pWgt[y];
				for (int i = 0; i < 4; ++i) {
					int y0 = nxCalc::clamp(pOrg[y] + i, 0, srcH - 1);
					cxColor csrc = pDst[y0*w + x];
					csrc.scl(wgt[i]);
					clr.add(csrc);
				}
				pTmp[y] = clr;
			}
			for (int y = 0; y < h; ++y) {
				pDst[y*w + x] = pTmp[y];
			}
		}
		if (!pWrkMem) {
			nxCore::mem_free(pOrg);
			nxCore::mem_free(pTmp);
			nxCore::mem_free(pWgt);
		}
	} else {
		const cxColor* pClr = pSrc;
		cxColor* pDstX = pDst;
		for (int y = 0; y < srcH; ++y) {
			for (int x = 0; x < srcW; ++x) {
				cxColor c = *pClr++;
				pDstX = clr_dup(pDstX, c, xscl);
			}
			cxColor* pDstY = pDstX;
			for (int i = 1; i < yscl; ++i) {
				for (int j = 0; j < w; ++j) {
					*pDstY++ = pDstX[j - w];
				}
			}
			pDstX = pDstY;
		}
	}
	if (cneg) {
		for (int i = 0; i < w * h; ++i) {
			pDst[i].clip_neg();
		}
	}
	return pDst;
}

size_t calc_upscale_work_size(int srcW, int srcH, int xscl, int yscl) {
	int w = srcW * xscl;
	int h = srcH * yscl;
	if (w <= 0 || h <= 0) return 0;
	int wgtNum = nxCalc::max(w, h);
	size_t size = wgtNum * (sizeof(xt_float4) + sizeof(cxColor) + sizeof(int16_t));
	return XD_ALIGN(size, 0x10);
}

} // nxImage


void sxImageData::Plane::expand(float* pDst, int pixelStride) const {
	PlaneInfo* pInfo = mpImg->get_plane_info(mPlaneId);
	uint8_t* pBits = reinterpret_cast<uint8_t*>(XD_INCR_PTR(mpImg, pInfo->mDataOffs));
	const int tblSize = 1 << 8;
	int32_t tbl[tblSize];
	nxCore::mem_zero(tbl, sizeof(tbl));
	int bitCnt = pInfo->mBitCount;
	uxVal32 uval;
	int bitPtr = 0;
	int pred = 0;
	int hash = 0;
	while (bitCnt) {
		int bitLen = nxCore::fetch_bits32(pBits, bitPtr, 5);
		bitPtr += 5;
		int bxor = 0;
		if (bitLen) {
			bxor = nxCore::fetch_bits32(pBits, bitPtr, bitLen);
			bitPtr += bitLen;
		}
		bitCnt -= 5 + bitLen;
		int ival = bxor ^ pred;
		tbl[hash] = ival;
		hash = (ival >> 21) & (tblSize - 1);
		pred = tbl[hash];
		uval.i = ival << pInfo->mTrailingZeroes;
		float fval = uval.f;
		fval += pInfo->mValOffs;
		*pDst = fval;
		pDst += pixelStride;
	}
}

void sxImageData::Plane::get_data(float* pDst, int pixelStride) const {
	int w = mpImg->get_width();
	int h = mpImg->get_height();
	int npix = w * h;
	PlaneInfo* pInfo = mpImg->get_plane_info(mPlaneId);
	if (pInfo) {
		void* pDataTop = XD_INCR_PTR(mpImg, pInfo->mDataOffs);
		if (pInfo->is_const()) {
			float cval = *reinterpret_cast<float*>(pDataTop);
			for (int i = 0; i < npix; ++i) {
				*pDst = cval;
				pDst += pixelStride;
			}
		} else if (pInfo->is_compressed()) {
			expand(pDst, pixelStride);
		} else {
			float* pSrc = reinterpret_cast<float*>(pDataTop);
			for (int i = 0; i < npix; ++i) {
				*pDst = *pSrc;
				pDst += pixelStride;
				++pSrc;
			}
		}
	}
}

float* sxImageData::Plane::get_data() const {
	int w = mpImg->get_width();
	int h = mpImg->get_height();
	int npix = w * h;
	int memsize = npix * sizeof(float);
	float* pDst = reinterpret_cast<float*>(nxCore::mem_alloc(memsize, "xImg:PlaneData"));
	get_data(pDst);
	return pDst;
}

int sxImageData::calc_mip_num() const {
	int n = 1;
	uint32_t w = get_width();
	uint32_t h = get_height();
	while (w > 1 && h > 1) {
		if (w > 1) w >>= 1;
		if (h > 1) h >>= 1;
		++n;
	}
	return n;
}

int sxImageData::find_plane_idx(const char* pName) const {
	int idx = -1;
	sxStrList* pStrLst = get_str_list();
	int npln = mPlaneNum;
	if (npln && pStrLst) {
		int nameId = pStrLst->find_str(pName);
		if (nameId >= 0) {
			for (int i = 0; i < npln; ++i) {
				PlaneInfo* pInfo = get_plane_info(i);
				if (pInfo && pInfo->mNameId == nameId) {
					idx = i;
					break;
				}
			}
		}
	}
	return idx;
}

sxImageData::Plane sxImageData::find_plane(const char* pName) const {
	Plane plane;
	int idx = find_plane_idx(pName);
	if (idx < 0) {
		plane.mpImg = nullptr;
		plane.mPlaneId = -1;
	} else {
		plane.mpImg = this;
		plane.mPlaneId = idx;
	}
	return plane;
}

sxImageData::Plane sxImageData::get_plane(int idx) const {
	Plane plane;
	if (ck_plane_idx(idx)) {
		plane.mpImg = this;
		plane.mPlaneId = idx;
	} else {
		plane.mpImg = nullptr;
		plane.mPlaneId = -1;
	}
	return plane;
}

bool sxImageData::is_plane_hdr(int idx) const {
	bool res = false;
	PlaneInfo* pInfo = get_plane_info(idx);
	if (pInfo) {
		res = pInfo->mMinVal < 0.0f || pInfo->mMaxVal > 1.0f;
	}
	return res;
}

bool sxImageData::is_hdr() const {
	int n = mPlaneNum;
	for (int i = 0; i < n; ++i) {
		if (is_plane_hdr(i)) return true;
	}
	return false;
}

void sxImageData::get_rgba(float* pDst) const {
	if (!pDst) return;
	int w = get_width();
	int h = get_height();
	int npix = w * h;
	int memsize = npix * sizeof(cxColor);
	nxCore::mem_zero(pDst, memsize);
	static const char* plnName[] = { "r", "g", "b", "a" };
	for (int i = 0; i < 4; ++i) {
		Plane pln = find_plane(plnName[i]);
		if (pln.is_valid()) {
			pln.get_data(pDst + i, 4);
		}
	}
}

cxColor* sxImageData::get_rgba() const {
	int w = get_width();
	int h = get_height();
	int npix = w * h;
	int memsize = npix * sizeof(cxColor);
	float* pDst = reinterpret_cast<float*>(nxCore::mem_alloc(memsize, "xImg:RGBA"));
	get_rgba(pDst);
	return reinterpret_cast<cxColor*>(pDst);
}

sxImageData::DDS sxImageData::get_dds() const {
	DDS dds;
	int w = get_width();
	int h = get_height();
	dds.mpHead = nxImage::alloc_dds128(w, h, &dds.mSize);
	if (dds.mpHead) {
		float* pDst = reinterpret_cast<float*>(dds.mpHead + 1);
		get_rgba(pDst);
	}
	return dds;
}

void sxImageData::DDS::free() {
	if (mpHead) {
		nxCore::mem_free(mpHead);
	}
	mpHead = nullptr;
	mSize = 0;
}

void sxImageData::DDS::save(const char* pOutPath) const {
	if (is_valid()) {
		nxCore::bin_save(pOutPath, mpHead, mSize);
	}
}

static float pmd_log2f(float x) {
#if 1
	return ::mth_logf(x) / ::mth_logf(2.0f);
#else
	return ::log2f(x);
#endif
}

sxImageData::Pyramid* sxImageData::get_pyramid() const {
	Pyramid* pPmd = nullptr;
	int w0 = get_width();
	int h0 = get_height();
	int w = w0;
	int h = h0;
	bool flgW = nxCore::is_pow2(w);
	bool flgH = nxCore::is_pow2(h);
	int baseW = flgW ? w : (1 << (1 + (int)pmd_log2f(float(w))));
	int baseH = flgH ? h : (1 << (1 + (int)pmd_log2f(float(h))));
	w = baseW;
	h = baseH;
	int nlvl = 1;
	int npix = 1;
	while (w > 1 || h > 1) {
		npix += w*h;
		if (w > 1) w >>= 1;
		if (h > 1) h >>= 1;
		++nlvl;
	}
	size_t headSize = sizeof(Pyramid) + (nlvl - 1)*sizeof(uint32_t);
	uint32_t topOffs = (uint32_t)XD_ALIGN(headSize, 0x10);
	size_t memSize = topOffs + npix*sizeof(cxColor);
	pPmd = reinterpret_cast<sxImageData::Pyramid*>(nxCore::mem_alloc(memSize, "xImg:Pyramid"));
	pPmd->mBaseWidth = baseW;
	pPmd->mBaseHeight = baseH;
	pPmd->mLvlNum = nlvl;
	w = baseW;
	h = baseH;
	uint32_t offs = topOffs;
	for (int i = 0; i < nlvl; ++i) {
		pPmd->mLvlOffs[i] = offs;
		offs += w*h*sizeof(cxColor);
		if (w > 1) w >>= 1;
		if (h > 1) h >>= 1;
	}
	int wgtNum = nxCalc::max(baseW, baseH);
	cxColor* pBase = get_rgba();
	cxColor* pLvl = pPmd->get_lvl(0);
	if (flgW && flgH) {
		nxCore::mem_copy(pLvl, pBase, baseW*baseH*sizeof(cxColor));
	} else {
		xt_float4* pWgt = reinterpret_cast<xt_float4*>(nxCore::mem_alloc(wgtNum*sizeof(xt_float4), "xImg:PmdTmpW"));
		int16_t* pOrg = reinterpret_cast<int16_t*>(nxCore::mem_alloc(wgtNum*sizeof(int16_t), "xImg:PmdTmpO"));
		cxColor* pTmp = reinterpret_cast<cxColor*>(nxCore::mem_alloc(wgtNum*sizeof(cxColor), "xImg:PmdTmp"));
		nxImage::calc_resample_wgts(w0, baseW, pWgt, pOrg);
		for (int y = 0; y < h0; ++y) {
			for (int x = 0; x < baseW; ++x) {
				cxColor clr;
				clr.zero();
				xt_float4 wgt = pWgt[x];
				for (int i = 0; i < 4; ++i) {
					int x0 = nxCalc::clamp(pOrg[x] + i, 0, w0-1);
					cxColor csrc = pBase[y*w0 + x0];
					csrc.scl(wgt[i]);
					clr.add(csrc);
				}
				pLvl[y*baseW + x] = clr;
			}
		}
		nxImage::calc_resample_wgts(h0, baseH, pWgt, pOrg);
		for (int x = 0; x < baseW; ++x) {
			for (int y = 0; y < baseH; ++y) {
				cxColor clr;
				clr.zero();
				xt_float4 wgt = pWgt[y];
				for (int i = 0; i < 4; ++i) {
					int y0 = nxCalc::clamp(pOrg[y] + i, 0, h0-1);
					cxColor csrc = pLvl[y0*baseW + x];
					csrc.scl(wgt[i]);
					clr.add(csrc);
				}
				pTmp[y] = clr;
			}
			for (int y = 0; y < baseH; ++y) {
				pLvl[y*baseW + x] = pTmp[y];
			}
		}
		for (int i = 0; i < baseW * baseH; ++i) {
			pLvl[i].clip_neg();
		}
		nxCore::mem_free(pTmp);
		nxCore::mem_free(pOrg);
		nxCore::mem_free(pWgt);
	}
	nxCore::mem_free(pBase);
	w = baseW/2;
	h = baseH/2;
	for (int i = 1; i < nlvl; ++i) {
		cxColor* pLvlSrc = pPmd->get_lvl(i-1);
		cxColor* pLvlDst = pPmd->get_lvl(i);
		int wsrc = w*2;
		int hsrc = h*2;
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				int sx0 = nxCalc::min(x*2, wsrc - 1);
				int sy0 = nxCalc::min(y*2, hsrc - 1);
				int sx1 = nxCalc::min(sx0 + 1, wsrc - 1);
				int sy1 = nxCalc::min(sy0 + 1, hsrc - 1);
				cxColor clr = pLvlSrc[sy0*wsrc + sx0];
				clr.add(pLvlSrc[sy0*wsrc + sx1]);
				clr.add(pLvlSrc[sy1*wsrc + sx0]);
				clr.add(pLvlSrc[sy1*wsrc + sx1]);
				clr.scl(0.25f);
				pLvlDst[y*w + x] = clr;
			}
		}
		if (w > 1) w >>= 1;
		if (h > 1) h >>= 1;
	}
	return pPmd;
}

void sxImageData::Pyramid::get_lvl_dims(int idx, int* pWidth, int* pHeight) const {
	int w = 0;
	int h = 0;
	if (ck_lvl_idx(idx)) {
		w = mBaseWidth;
		h = mBaseHeight;
		for (int i = 0; i < idx; ++i) {
			if (w > 1) w >>= 1;
			if (h > 1) h >>= 1;
		}
	}
	if (pWidth) {
		*pWidth = w;
	}
	if (pHeight) {
		*pHeight = h;
	}
}

sxImageData::DDS sxImageData::Pyramid::get_lvl_dds(int idx) const {
	DDS dds;
	dds.mpHead = nullptr;
	dds.mSize = 0;
	if (ck_lvl_idx(idx)) {
		int w, h;
		get_lvl_dims(idx, &w, &h);
		dds.mpHead = nxImage::alloc_dds128(w, h, &dds.mSize);
		cxColor* pLvl = get_lvl(idx);
		if (pLvl) {
			cxColor* pDst = reinterpret_cast<cxColor*>(dds.mpHead + 1);
			nxCore::mem_copy(pDst, pLvl, w*h*sizeof(cxColor));
		}
	}
	return dds;
}



float sxKeyframesData::RigLink::Node::get_pos_chan(int idx) {
	float val = 0.0f;
	if ((uint32_t)idx < 3) {
		Val* pVal = get_pos_val();
		if (pVal && pVal->fcvId[idx] >= 0) {
			val = pVal->f3[idx];
		}
	}
	return val;
}

float sxKeyframesData::RigLink::Node::get_rot_chan(int idx) {
	float val = 0.0f;
	if ((uint32_t)idx < 3) {
		Val* pVal = get_rot_val();
		if (pVal && pVal->fcvId[idx] >= 0) {
			val = pVal->f3[idx];
		}
	}
	return val;
}

float sxKeyframesData::RigLink::Node::get_scl_chan(int idx) {
	float val = 1.0f;
	if ((uint32_t)idx < 3) {
		Val* pVal = get_scl_val();
		if (pVal && pVal->fcvId[idx] >= 0) {
			val = pVal->f3[idx];
		}
	}
	return val;
}

float sxKeyframesData::RigLink::Node::get_anim_chan(exAnimChan chan) {
	float val = 0.0f;
	switch (chan) {
		case exAnimChan::TX: val = get_pos_chan(0); break;
		case exAnimChan::TY: val = get_pos_chan(1); break;
		case exAnimChan::TZ: val = get_pos_chan(2); break;
		case exAnimChan::RX: val = get_rot_chan(0); break;
		case exAnimChan::RY: val = get_rot_chan(1); break;
		case exAnimChan::RZ: val = get_rot_chan(2); break;
		case exAnimChan::SX: val = get_scl_chan(0); break;
		case exAnimChan::SY: val = get_scl_chan(1); break;
		case exAnimChan::SZ: val = get_scl_chan(2); break;
		default: break;
	}
	return val;
}

bool sxKeyframesData::RigLink::Node::ck_pos_chan(int idx) {
	bool res = false;
	if ((uint32_t)idx < 3) {
		Val* pVal = get_pos_val();
		res = (pVal && pVal->fcvId[idx] >= 0);
	}
	return res;
}

bool sxKeyframesData::RigLink::Node::ck_rot_chan(int idx) {
	bool res = false;
	if ((uint32_t)idx < 3) {
		Val* pVal = get_rot_val();
		res = (pVal && pVal->fcvId[idx] >= 0);
	}
	return res;
}

bool sxKeyframesData::RigLink::Node::ck_scl_chan(int idx) {
	bool res = false;
	if ((uint32_t)idx < 3) {
		Val* pVal = get_scl_val();
		res = (pVal && pVal->fcvId[idx] >= 0);
	}
	return res;
}

bool sxKeyframesData::RigLink::Node::ck_anim_chan(exAnimChan chan) {
	bool res = false;
	switch (chan) {
		case exAnimChan::TX: res = ck_pos_chan(0); break;
		case exAnimChan::TY: res = ck_pos_chan(1); break;
		case exAnimChan::TZ: res = ck_pos_chan(2); break;
		case exAnimChan::RX: res = ck_rot_chan(0); break;
		case exAnimChan::RY: res = ck_rot_chan(1); break;
		case exAnimChan::RZ: res = ck_rot_chan(2); break;
		case exAnimChan::SX: res = ck_scl_chan(0); break;
		case exAnimChan::SY: res = ck_scl_chan(1); break;
		case exAnimChan::SZ: res = ck_scl_chan(2); break;
		default: break;
	}
	return res;
}

bool sxKeyframesData::has_node_info() const {
	bool res = false;
	ptrdiff_t offs = (uint8_t*)&mNodeInfoNum - (uint8_t*)this;
	if (mHeadSize > (uint32_t)offs) {
		res = mNodeInfoNum != 0 && mNodeInfoOffs != 0;
	}
	return res;
}

int sxKeyframesData::find_node_info_idx(const char* pName, const char* pPath, int startIdx) const {
	int idx = -1;
	if (has_node_info()) {
		sxStrList* pStrLst = get_str_list();
		if (pStrLst && ck_node_info_idx(startIdx)) {
			NodeInfo* pInfo = reinterpret_cast<NodeInfo*>(XD_INCR_PTR(this, mNodeInfoOffs));
			int ninfo = get_node_info_num();
			int nameId = pName ? pStrLst->find_str(pName) : -1;
			if (nameId >= 0) {
				if (pPath) {
					int pathId = pStrLst->find_str(pPath);
					if (pathId >= 0) {
						for (int i = startIdx; i < ninfo; ++i) {
							if (pInfo[i].mNameId == nameId && pInfo[i].mPathId == pathId) {
								idx = i;
								break;
							}
						}
					}
				} else {
					for (int i = startIdx; i < ninfo; ++i) {
						if (pInfo[i].mNameId == nameId) {
							idx = i;
							break;
						}
					}
				}
			} else if (pPath) {
				int pathId = pStrLst->find_str(pPath);
				if (pathId >= 0) {
					for (int i = startIdx; i < ninfo; ++i) {
						if (pInfo[i].mPathId == pathId) {
							idx = i;
							break;
						}
					}
				}
			}
		}
	}
	return idx;
}

template<int IDX_SIZE> int fno_get(uint8_t* pTop, int at) {
	uint8_t* pFno = pTop + at*IDX_SIZE;
	int fno = -1;
	switch (IDX_SIZE) {
		case 1:
			fno = *pFno;
			break;
		case 2:
			fno = pFno[0] | (pFno[1] << 8);
			break;
		case 3:
			fno = pFno[0] | (pFno[1] << 8) | (pFno[2] << 16);
			break;
	}
	return fno;
}

template<int IDX_SIZE> int find_fno_idx_lin(uint8_t* pTop, int num, int fno) {
	for (int i = 1; i < num; ++i) {
		int ck = fno_get<IDX_SIZE>(pTop, i);
		if (fno < ck) {
			return i - 1;
		}
	}
	return num - 1;
}

template<int IDX_SIZE> int find_fno_idx_bin(uint8_t* pTop, int num, int fno) {
	uint32_t istart = 0;
	uint32_t iend = num;
	do {
		uint32_t imid = (istart + iend) / 2;
		int ck = fno_get<IDX_SIZE>(pTop, imid);
		if (fno < ck) {
			iend = imid;
		} else {
			istart = imid;
		}
	} while (iend - istart >= 2);
	return istart;
}

template<int IDX_SIZE> int find_fno_idx(uint8_t* pTop, int num, int fno) {
	if (num > 10) {
		return find_fno_idx_bin<IDX_SIZE>(pTop, num, fno);
	}
	return find_fno_idx_lin<IDX_SIZE>(pTop, num, fno);
}

int sxKeyframesData::FCurve::find_key_idx(int fno) const {
	int idx = -1;
	if (is_valid() && mpKfr->ck_fno(fno)) {
		FCurveInfo* pInfo = get_info();
		if (!pInfo->is_const()) {
			if (pInfo->mFnoOffs) {
				uint8_t* pFno = reinterpret_cast<uint8_t*>(XD_INCR_PTR(mpKfr, pInfo->mFnoOffs));
				int nfno = pInfo->mKeyNum;
				int maxFno = mpKfr->get_max_fno();
				if (maxFno < (1 << 8)) {
					idx = find_fno_idx<1>(pFno, nfno, fno);
				} else if (maxFno < (1 << 16)) {
					idx = find_fno_idx<2>(pFno, nfno, fno);
				} else {
					idx = find_fno_idx<3>(pFno, nfno, fno);
				}
			} else {
				idx = fno;
			}
		}
	}
	return idx;
}

int sxKeyframesData::FCurve::get_fno(int idx) const {
	int fno = 0;
	FCurveInfo* pInfo = get_info();
	if (pInfo) {
		if (!pInfo->is_const() && (uint32_t)idx < (uint32_t)get_key_num()) {
			if (pInfo->has_fno_lst()) {
				uint8_t* pFno = reinterpret_cast<uint8_t*>(XD_INCR_PTR(mpKfr, pInfo->mFnoOffs));
				int maxFno = mpKfr->get_max_fno();
				if (maxFno < (1 << 8)) {
					fno = fno_get<1>(pFno, idx);
				} else if (maxFno < (1 << 16)) {
					fno = fno_get<2>(pFno, idx);
				} else {
					fno = fno_get<3>(pFno, idx);
				}
			} else {
				fno = idx;
			}
		}
	}
	return fno;
}

float sxKeyframesData::FCurve::eval(float frm, bool extrapolate) const {
	float val = 0.0f;
	int fno = (int)frm;
	if (is_valid()) {
		FCurveInfo* pInfo = get_info();
		if (pInfo->is_const()) {
			val = pInfo->mMinVal;
		} else {
			bool loopFlg = false;
			if (pInfo->has_fno_lst()) {
				float lastFno = (float)get_fno(pInfo->mKeyNum - 1);
				if (frm > lastFno && frm < lastFno + 1.0f) {
					loopFlg = true;
				}
			}
			if (loopFlg) {
				int lastIdx = pInfo->mKeyNum - 1;
				eFunc func = pInfo->get_common_func();
				if (pInfo->mFuncOffs) {
					func = (eFunc)reinterpret_cast<uint8_t*>(XD_INCR_PTR(mpKfr, pInfo->mFuncOffs))[lastIdx];
				}
				float* pVals = reinterpret_cast<float*>(XD_INCR_PTR(mpKfr, pInfo->mValOffs));
				if (func == eFunc::CONSTANT) {
					val = pVals[lastIdx];
				} else {
					float t = frm - (float)fno;
					if (extrapolate) {
						val = nxCalc::lerp(pVals[lastIdx], pVals[lastIdx] + pVals[0], t);
					} else {
						val = nxCalc::lerp(pVals[lastIdx], pVals[0], t);
					}
				}
			} else if (mpKfr->ck_fno(fno)) {
				float* pVals = reinterpret_cast<float*>(XD_INCR_PTR(mpKfr, pInfo->mValOffs));
				int i0 = find_key_idx(fno);
				int i1 = i0 + 1;
				float f0 = (float)get_fno(i0);
				float f1 = (float)get_fno(i1);
				if (frm != f0) {
					eFunc func = pInfo->get_common_func();
					if (pInfo->mFuncOffs) {
						func = (eFunc)reinterpret_cast<uint8_t*>(XD_INCR_PTR(mpKfr, pInfo->mFuncOffs))[i0];
					}
					if (func == eFunc::CONSTANT) {
						val = pVals[i0];
					} else {
						float t = (frm - f0) / (f1 - f0);
						float v0 = pVals[i0];
						float v1 = pVals[i1];
						if (func == eFunc::LINEAR) {
							val = nxCalc::lerp(v0, v1, t);
						} else if (func == eFunc::CUBIC) {
							float* pSlopeL = pInfo->mLSlopeOffs ? reinterpret_cast<float*>(XD_INCR_PTR(mpKfr, pInfo->mLSlopeOffs)) : nullptr;
							float* pSlopeR = pInfo->mRSlopeOffs ? reinterpret_cast<float*>(XD_INCR_PTR(mpKfr, pInfo->mRSlopeOffs)) : nullptr;
							float outgoing = 0.0f;
							float incoming = 0.0f;
							if (pSlopeR) {
								outgoing = pSlopeR[i0];
							}
							if (pSlopeL) {
								incoming = pSlopeL[i1];
							}
							float fscl = nxCalc::rcp0(mpKfr->mFPS);
							float seg = (f1 - f0) * fscl;
							outgoing *= seg;
							incoming *= seg;
							val = nxCalc::hermite(v0, outgoing, v1, incoming, t);
						}
					}
				} else {
					val = pVals[i0];
				}
			}
		}
	}
	return val;
}

const char* sxKeyframesData::get_node_name(int idx) const {
	const char* pName = nullptr;
	NodeInfo* pInfo = get_node_info_ptr(idx);
	if (pInfo) {
		pName = get_str(pInfo->mNameId);
	}
	return pName;
}

int sxKeyframesData::find_fcv_idx(const char* pNodeName, const char* pChanName, const char* pNodePath) const {
	int idx = -1;
	sxStrList* pStrLst = get_str_list();
	if (pStrLst) {
		int chanId = pStrLst->find_str(pChanName);
		int nameId = chanId > 0 ? pStrLst->find_str(pNodeName) : -1;
		if (chanId >= 0 && nameId >= 0) {
			int i;
			FCurveInfo* pInfo;
			int nfcv = get_fcv_num();
			if (pNodePath) {
				int pathId = pStrLst->find_str(pNodePath);
				if (pathId >= 0) {
					for (i = 0; i < nfcv; ++i) {
						pInfo = get_fcv_info(i);
						if (pInfo && pInfo->mChanNameId == chanId && pInfo->mNodeNameId == nameId && pInfo->mNodePathId == pathId) {
							idx = i;
							break;
						}
					}
				}
			} else {
				for (i = 0; i < nfcv; ++i) {
					pInfo = get_fcv_info(i);
					if (pInfo && pInfo->mChanNameId == chanId && pInfo->mNodeNameId == nameId) {
						idx = i;
						break;
					}
				}
			}
		}
	}
	return idx;
}

sxKeyframesData::FCurve sxKeyframesData::get_fcv(int idx) const {
	FCurve fcv;
	if (ck_fcv_idx(idx)) {
		fcv.mpKfr = this;
		fcv.mFcvId = idx;
	} else {
		fcv.mpKfr = nullptr;
		fcv.mFcvId = -1;
	}
	return fcv;
}

sxKeyframesData::RigLink* sxKeyframesData::make_rig_link(const sxRigData& rig) const {
	RigLink* pLink = nullptr;
	if (has_node_info()) {
		static const char* posChans[] = { "tx", "ty", "tz" };
		static const char* rotChans[] = { "rx", "ry", "rz" };
		static const char* sclChans[] = { "sx", "sy", "sz" };
		int n = get_node_info_num();
		int nodeNum = 0;
		int posNum = 0;
		int rotNum = 0;
		int sclNum = 0;
		for (int i = 0; i < n; ++i) {
			NodeInfo* pNodeInfo = get_node_info_ptr(i);
			if (pNodeInfo) {
				const char* pNodeName = get_str(pNodeInfo->mNameId);
				int rigNodeId = rig.find_node(pNodeName);
				if (rigNodeId >= 0) {
					++nodeNum;
					for (int j = 0; j < 3; ++j) {
						if (find_fcv_idx(pNodeName, posChans[j]) >= 0) {
							++posNum;
							break;
						}
					}
					for (int j = 0; j < 3; ++j) {
						if (find_fcv_idx(pNodeName, rotChans[j]) >= 0) {
							++rotNum;
							break;
						}
					}
					for (int j = 0; j < 3; ++j) {
						if (find_fcv_idx(pNodeName, sclChans[j]) >= 0) {
							++sclNum;
							break;
						}
					}
				}
			}
		}
		if (nodeNum > 0) {
			int valNum = posNum + rotNum + sclNum;
			size_t memsize = sizeof(RigLink) + (nodeNum - 1)*sizeof(RigLink::Node);
			size_t valTop = memsize;
			memsize += valNum*sizeof(RigLink::Val);
			size_t mapTop = memsize;
			memsize += rig.get_nodes_num()*sizeof(int16_t);
			pLink = reinterpret_cast<RigLink*>(nxCore::mem_alloc(memsize, "xKfr:RigLink"));
			if (pLink) {
				nxCore::mem_zero(pLink, memsize);
				pLink->mNodeNum = nodeNum;
				pLink->mRigNodeNum = rig.get_nodes_num();
				pLink->mRigMapOffs = (uint32_t)mapTop;
				int16_t* pRigMap = pLink->get_rig_map();
				if (pRigMap) {
					for (int i = 0; i < pLink->mRigNodeNum; ++i) {
						pRigMap[i] = -1;
					}
				}
				RigLink::Val* pVal = (RigLink::Val*)XD_INCR_PTR(pLink, valTop);
				RigLink::Node* pLinkNode = pLink->mNodes;
				int inode = 0;
				for (int i = 0; i < n; ++i) {
					NodeInfo* pNodeInfo = get_node_info_ptr(i);
					if (pNodeInfo) {
						const char* pNodeName = get_str(pNodeInfo->mNameId);
						int rigNodeId = rig.find_node(pNodeName);
						if (rigNodeId >= 0) {
							int32_t posFcvId[3];
							bool posFlg = false;
							for (int j = 0; j < 3; ++j) {
								posFcvId[j] = find_fcv_idx(pNodeName, posChans[j]);
								posFlg |= posFcvId[j] >= 0;
							}
							int32_t rotFcvId[3];
							bool rotFlg = false;
							for (int j = 0; j < 3; ++j) {
								rotFcvId[j] = find_fcv_idx(pNodeName, rotChans[j]);
								rotFlg |= rotFcvId[j] >= 0;
							}
							int32_t sclFcvId[3];
							bool sclFlg = false;
							for (int j = 0; j < 3; ++j) {
								sclFcvId[j] = find_fcv_idx(pNodeName, sclChans[j]);
								sclFlg |= sclFcvId[j] >= 0;
							}
							if (posFlg || rotFlg || sclFlg) {
								pLinkNode->mKfrNodeId = i;
								pLinkNode->mRigNodeId = rigNodeId;
								pLinkNode->mRotOrd = pNodeInfo->get_rot_order();
								pLinkNode->mXformOrd = pNodeInfo->get_xform_order();
								pLinkNode->mUseSlerp = false;
								if (posFlg) {
									pVal->set_vec(rig.get_lpos(rigNodeId));
									for (int j = 0; j < 3; ++j) {
										pVal->fcvId[j] = posFcvId[j];
									}
									pLinkNode->mPosValOffs = (uint32_t)((uint8_t*)pVal - (uint8_t*)pLinkNode);
									++pVal;
								}
								if (rotFlg) {
									pVal->set_vec(rig.get_lrot(rigNodeId));
									for (int j = 0; j < 3; ++j) {
										pVal->fcvId[j] = rotFcvId[j];
									}
									pLinkNode->mRotValOffs = (uint32_t)((uint8_t*)pVal - (uint8_t*)pLinkNode);
									++pVal;
								}
								if (sclFlg) {
									pVal->set_vec(rig.get_lscl(rigNodeId));
									for (int j = 0; j < 3; ++j) {
										pVal->fcvId[j] = sclFcvId[j];
									}
									pLinkNode->mSclValOffs = (uint32_t)((uint8_t*)pVal - (uint8_t*)pLinkNode);
									++pVal;
								}
								pRigMap[rigNodeId] = inode;
								++inode;
								++pLinkNode;
							}
						}
					}
				}
			}
		}
	}
	return pLink;
}

void sxKeyframesData::eval_rig_link_node(RigLink* pLink, int nodeIdx, float frm, const sxRigData* pRig, cxMtx* pRigLocalMtx) const {
	if (!pLink) return;
	if (!pLink->ck_node_idx(nodeIdx)) return;
	const int POS_MASK = 1 << 0;
	const int ROT_MASK = 1 << 1;
	const int SCL_MASK = 1 << 2;
	int xformMask = 0;
	RigLink::Node* pNode = &pLink->mNodes[nodeIdx];
	RigLink::Val* pPosVal = pNode->get_pos_val();
	if (pPosVal) {
		for (int j = 0; j < 3; ++j) {
			int fcvId = pPosVal->fcvId[j];
			if (ck_fcv_idx(fcvId)) {
				xformMask |= POS_MASK;
				FCurve fcv = get_fcv(fcvId);
				if (fcv.is_valid()) {
					pPosVal->f3[j] = fcv.eval(frm);
				}
			}
		}
	}
	RigLink::Val* pRotVal = pNode->get_rot_val();
	if (pRotVal) {
		int ifrm = (int)frm;
		if (pNode->mUseSlerp && frm != (float)ifrm) {
			cxVec rot0(0.0f);
			cxVec rot1(0.0f);
			for (int j = 0; j < 3; ++j) {
				int fcvId = pRotVal->fcvId[j];
				if (ck_fcv_idx(fcvId)) {
					xformMask |= ROT_MASK;
					FCurve fcv = get_fcv(fcvId);
					if (fcv.is_valid()) {
						rot0.set_at(j, fcv.eval((float)ifrm));
						rot1.set_at(j, fcv.eval((float)(ifrm + 1)));
					}
				}
			}
			cxQuat q0;
			q0.set_rot_degrees(rot0, pNode->mRotOrd);
			cxQuat q1;
			q1.set_rot_degrees(rot1, pNode->mRotOrd);
			cxQuat qr = nxQuat::slerp(q0, q1, frm - (float)ifrm);
			cxVec rv = qr.get_rot_degrees(pNode->mRotOrd);
			pRotVal->set_vec(rv);
		} else {
			for (int j = 0; j < 3; ++j) {
				int fcvId = pRotVal->fcvId[j];
				if (ck_fcv_idx(fcvId)) {
					xformMask |= ROT_MASK;
					FCurve fcv = get_fcv(fcvId);
					if (fcv.is_valid()) {
						pRotVal->f3[j] = fcv.eval(frm);
					}
				}
			}
		}
	}
	RigLink::Val* pSclVal = pNode->get_scl_val();
	if (pSclVal) {
		for (int j = 0; j < 3; ++j) {
			int fcvId = pSclVal->fcvId[j];
			if (ck_fcv_idx(fcvId)) {
				xformMask |= SCL_MASK;
				FCurve fcv = get_fcv(fcvId);
				if (fcv.is_valid()) {
					pSclVal->f3[j] = fcv.eval(frm);
				}
			}
		}
	}
	if (xformMask && pRig && pRigLocalMtx) {
		cxMtx sm;
		cxMtx rm;
		cxMtx tm;
		exRotOrd rord = pNode->mRotOrd;
		exTransformOrd xord = pNode->mXformOrd;
		if (xformMask & SCL_MASK) {
			sm.mk_scl(pSclVal->get_vec());
		} else {
			sm.mk_scl(pRig->get_lscl(pNode->mRigNodeId));
		}
		if (xformMask & ROT_MASK) {
			rm.set_rot_degrees(pRotVal->get_vec(), rord);
		} else {
			rm.set_rot_degrees(pRig->get_lrot(pNode->mRigNodeId));
		}
		if (xformMask & POS_MASK) {
			tm.mk_translation(pPosVal->get_vec());
		} else {
			tm.mk_translation(pRig->get_lpos(pNode->mRigNodeId));
		}
		pRigLocalMtx[pNode->mRigNodeId].calc_xform(tm, rm, sm, xord);
	}
}

void sxKeyframesData::eval_rig_link(RigLink* pLink, float frm, const sxRigData* pRig, cxMtx* pRigLocalMtx) const {
	if (!pLink) return;
	int n = pLink->mNodeNum;
	for (int i = 0; i < n; ++i) {
		eval_rig_link_node(pLink, i, frm, pRig, pRigLocalMtx);
	}
}

void sxKeyframesData::dump_clip(FILE* pOut) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOut) return;
	int nfcv = get_fcv_num();
	int nfrm = get_frame_count();
	::fprintf(pOut, "{\n");
	::fprintf(pOut, "	rate = %d\n", (int)get_frame_rate());
	::fprintf(pOut, "	start = -1\n");
	::fprintf(pOut, "	tracklength = %d\n", nfrm);
	::fprintf(pOut, "	tracks = %d\n", nfcv);
	for (int i = 0; i < nfcv; ++i) {
		::fprintf(pOut, "   {\n");
		FCurve fcv = get_fcv(i);
		const char* pNodePath = fcv.get_node_path();
		const char* pNodeName = fcv.get_node_name();
		const char* pChanName = fcv.get_chan_name();
		::fprintf(pOut, "      name = %s/%s:%s\n", pNodePath, pNodeName, pChanName);
		::fprintf(pOut, "      data =");
		for (int j = 0; j < nfrm; ++j) {
			float frm = (float)j;
			float val = fcv.eval(frm);
			::fprintf(pOut, " %f", val);
		}
		::fprintf(pOut, "\n");
		::fprintf(pOut, "   }\n");
	}
	::fprintf(pOut, "}\n");
#endif
}

void sxKeyframesData::dump_clip(const char* pOutPath) const {
#if XD_FILEFUNCS_ENABLED
	char outPath[XD_MAX_PATH];
	if (!pOutPath) {
		const char* pSrcPath = get_file_path();
		if (!pSrcPath) {
			return;
		}
#if defined(_MSC_VER)
		::sprintf_s(outPath, sizeof(outPath), "%s.clip", pSrcPath);
#else
		::sprintf(outPath, "%s.clip", pSrcPath);
#endif
		pOutPath = outPath;
	}
	FILE* pOut = nxSys::fopen_w_txt(pOutPath);
	if (!pOut) {
		return;
	}
	dump_clip(pOut);
	::fclose(pOut);
#endif
}


enum class exExprFunc {
	_abs, _acos, _asin, _atan, _atan2, _ceil,
	_ch, _clamp, _cos, _deg, _detail, _distance, _exp,
	_fit, _fit01, _fit10, _fit11,
	_floor, _frac, _if, _int, _length,
	_log, _log10, _max, _min, _pow, _rad,
	_rint, _round, _sign, _sin, _sqrt, _tan
};


bool sxCompiledExpression::String::eq(const char* pStr) const {
	bool res = false;
	if (pStr) {
		size_t len = nxCore::str_len(pStr);
		if (len == mLen) {
			res = nxCore::mem_eq(mpChars, pStr, len);
		}
	}
	return res;
}

bool sxCompiledExpression::String::starts_with(const char* pStr) const {
	bool res = false;
	if (pStr) {
		size_t len = nxCore::str_len(pStr);
		if (len <= mLen) {
			res = nxCore::mem_eq(mpChars, pStr, len);
		}
	}
	return res;
}

void sxCompiledExpression::Stack::alloc(int n) {
	free();
	if (n < 1) return;
	size_t memsize = n * sizeof(Val) + Tags::calc_mem_size(n);
	mpMem = nxCore::mem_alloc(memsize, "xExpr:Stack");
	if (mpMem) {
		nxCore::mem_zero(mpMem, memsize);
		mpVals = reinterpret_cast<Val*>(mpMem);
		mTags.init(mpVals + n, n);
	}
}

void sxCompiledExpression::Stack::free() {
	if (mpMem) {
		nxCore::mem_free(mpMem);
	}
	mpMem = nullptr;
	mpVals = nullptr;
	mPtr = 0;
	mTags.reset();
}

XD_NOINLINE void sxCompiledExpression::Stack::push_num(float num) {
	if (is_full()) {
		return;
	}
	mpVals[mPtr].num = num;
	mTags.set_num(mPtr);
	++mPtr;
}

XD_NOINLINE float sxCompiledExpression::Stack::pop_num() {
	if (is_empty()) {
		return 0.0f;
	}
	--mPtr;
	if (mTags.is_num(mPtr)) {
		return mpVals[mPtr].num;
	}
	return 0.0f;
}

XD_NOINLINE void sxCompiledExpression::Stack::push_str(int ptr) {
	if (is_full()) {
		return;
	}
	mpVals[mPtr].sptr = ptr;
	mTags.set_str(mPtr);
	++mPtr;
}

XD_NOINLINE int sxCompiledExpression::Stack::pop_str() {
	if (is_empty()) {
		return -1;
	}
	--mPtr;
	if (mTags.is_str(mPtr)) {
		return mpVals[mPtr].sptr;
	}
	return -1;
}

void sxCompiledExpression::get_str(int idx, String* pStr) const {
	if (!pStr) return;
	const StrInfo* pInfo = get_str_info(idx);
	if (pInfo) {
		pStr->mpChars = reinterpret_cast<const char*>(XD_INCR_PTR(&get_str_info_top()[mStrsNum], pInfo->mOffs));
		pStr->mHash = pInfo->mHash;
		pStr->mLen = pInfo->mLen;
	} else {
		pStr->mpChars = nullptr;
		pStr->mHash = 0;
		pStr->mLen = 0;
	}
}

sxCompiledExpression::String sxCompiledExpression::get_str(int idx) const {
	String str;
	get_str(idx, &str);
	return str;
}

static inline float expr_acos(float val) {
	return XD_RAD2DEG(::mth_acosf(nxCalc::clamp(val, -1.0f, 1.0f)));
}

static inline float expr_asin(float val) {
	return XD_RAD2DEG(::mth_asinf(nxCalc::clamp(val, -1.0f, 1.0f)));
}

static inline float expr_atan(float val) {
	return XD_RAD2DEG(::mth_atanf(val));
}

static inline float expr_atan2(float y, float x) {
	return XD_RAD2DEG(::mth_atan2f(y, x));
}

static inline float expr_dist(sxCompiledExpression::Stack* pStk) {
	cxVec p1;
	p1.z = pStk->pop_num();
	p1.y = pStk->pop_num();
	p1.x = pStk->pop_num();
	cxVec p0;
	p0.z = pStk->pop_num();
	p0.y = pStk->pop_num();
	p0.x = pStk->pop_num();
	return nxVec::dist(p0, p1);
}

static inline float expr_fit(sxCompiledExpression::Stack* pStk) {
	float newmax = pStk->pop_num();
	float newmin = pStk->pop_num();
	float oldmax = pStk->pop_num();
	float oldmin = pStk->pop_num();
	float num = pStk->pop_num();
	return nxCalc::fit(num, oldmin, oldmax, newmin, newmax);
}

static inline float expr_fit01(sxCompiledExpression::Stack* pStk) {
	float newmax = pStk->pop_num();
	float newmin = pStk->pop_num();
	float num = nxCalc::saturate(pStk->pop_num());
	return nxCalc::fit(num, 0.0f, 1.0f, newmin, newmax);
}

static inline float expr_fit10(sxCompiledExpression::Stack* pStk) {
	float newmax = pStk->pop_num();
	float newmin = pStk->pop_num();
	float num = nxCalc::saturate(pStk->pop_num());
	return nxCalc::fit(num, 1.0f, 0.0f, newmin, newmax);
}

static inline float expr_fit11(sxCompiledExpression::Stack* pStk) {
	float newmax = pStk->pop_num();
	float newmin = pStk->pop_num();
	float num = nxCalc::clamp(pStk->pop_num(), -1.0f, 1.0f);
	return nxCalc::fit(num, -1.0f, 1.0f, newmin, newmax);
}

static inline float expr_len(sxCompiledExpression::Stack* pStk) {
	cxVec v;
	v.z = pStk->pop_num();
	v.y = pStk->pop_num();
	v.x = pStk->pop_num();
	return v.mag();
}

void sxCompiledExpression::exec(ExecIfc& ifc) const {
	Stack* pStk = ifc.get_stack();
	if (!pStk) {
		return;
	}

	float valA;
	float valB;
	float valC;
	float cmpRes = 0.0f;
	int funcId;
	String str1;
	String str2;
	int n = mCodeNum;
	const Code* pCode = get_code_top();
	for (int i = 0; i < n; ++i) {
		eOp op = pCode->get_op();
		if (op == eOp::END) break;
		switch (op) {
		case eOp::NUM:
			pStk->push_num(get_val(pCode->mInfo));
			break;
		case eOp::STR:
			pStk->push_str(pCode->mInfo);
			break;
		case eOp::VAR:
			get_str(pCode->mInfo, &str1);
			pStk->push_num(ifc.var(str1));
			break;
		case eOp::CMP:
			valB = pStk->pop_num();
			valA = pStk->pop_num();
			switch ((eCmp)pCode->mInfo) {
			case eCmp::EQ:
				cmpRes = valA == valB ? 1.0f : 0.0f;
				break;
			case eCmp::NE:
				cmpRes = valA != valB ? 1.0f : 0.0f;
				break;
			case eCmp::LT:
				cmpRes = valA < valB ? 1.0f : 0.0f;
				break;
			case eCmp::LE:
				cmpRes = valA <= valB ? 1.0f : 0.0f;
				break;
			case eCmp::GT:
				cmpRes = valA > valB ? 1.0f : 0.0f;
				break;
			case eCmp::GE:
				cmpRes = valA >= valB ? 1.0f : 0.0f;
				break;
			default:
				break;
			}
			pStk->push_num(cmpRes);
			break;
		case eOp::ADD:
			valB = pStk->pop_num();
			valA = pStk->pop_num();
			pStk->push_num(valA + valB);
			break;
		case eOp::SUB:
			valB = pStk->pop_num();
			valA = pStk->pop_num();
			pStk->push_num(valA - valB);
			break;
		case eOp::MUL:
			valB = pStk->pop_num();
			valA = pStk->pop_num();
			pStk->push_num(valA * valB);
			break;
		case eOp::DIV:
			valB = pStk->pop_num();
			valA = pStk->pop_num();
			pStk->push_num(nxCalc::div0(valA, valB));
			break;
		case eOp::MOD:
			valB = pStk->pop_num();
			valA = pStk->pop_num();
			pStk->push_num(valB != 0.0f ? ::mth_fmodf(valA, valB) : 0.0f);
			break;
		case eOp::NEG:
			valA = pStk->pop_num();
			pStk->push_num(-valA);
			break;
		case eOp::FUN:
			funcId = pCode->mInfo;
			switch ((exExprFunc)funcId) {
			case exExprFunc::_abs:
				valA = pStk->pop_num();
				pStk->push_num(::mth_fabsf(valA));
				break;
			case exExprFunc::_acos:
				valA = pStk->pop_num();
				pStk->push_num(expr_acos(valA));
				break;
			case exExprFunc::_asin:
				valA = pStk->pop_num();
				pStk->push_num(expr_asin(valA));
				break;
			case exExprFunc::_atan:
				valA = pStk->pop_num();
				pStk->push_num(expr_atan(valA));
				break;
			case exExprFunc::_atan2:
				valB = pStk->pop_num();
				valA = pStk->pop_num();
				pStk->push_num(expr_atan2(valA, valB));
				break;
			case exExprFunc::_ceil:
				valA = pStk->pop_num();
				pStk->push_num(::mth_ceilf(valA));
				break;
			case exExprFunc::_ch:
				get_str(pStk->pop_str(), &str1);
				pStk->push_num(ifc.ch(str1));
				break;
			case exExprFunc::_clamp:
				valC = pStk->pop_num();
				valB = pStk->pop_num();
				valA = pStk->pop_num();
				pStk->push_num(nxCalc::clamp(valA, valB, valC));
				break;
			case exExprFunc::_cos:
				valA = pStk->pop_num();
				pStk->push_num(::mth_cosf(XD_DEG2RAD(valA)));
				break;
			case exExprFunc::_deg:
				valA = pStk->pop_num();
				pStk->push_num(XD_RAD2DEG(valA));
				break;
			case exExprFunc::_detail:
				valA = pStk->pop_num();
				get_str(pStk->pop_str(), &str2);
				get_str(pStk->pop_str(), &str1);
				pStk->push_num(ifc.detail(str1, str2, (int)valA));
				break;
			case exExprFunc::_distance:
				pStk->push_num(expr_dist(pStk));
				break;
			case exExprFunc::_exp:
				valA = pStk->pop_num();
				pStk->push_num(::mth_expf(valA));
				break;
			case exExprFunc::_fit:
				pStk->push_num(expr_fit(pStk));
				break;
			case exExprFunc::_fit01:
				pStk->push_num(expr_fit01(pStk));
				break;
			case exExprFunc::_fit10:
				pStk->push_num(expr_fit10(pStk));
				break;
			case exExprFunc::_fit11:
				pStk->push_num(expr_fit11(pStk));
				break;
			case exExprFunc::_floor:
				valA = pStk->pop_num();
				pStk->push_num(::mth_floorf(valA));
				break;
			case exExprFunc::_frac:
				valA = pStk->pop_num();
				pStk->push_num(valA - ::mth_floorf(valA)); // Houdini-compatible: frac(-2.7) = 0.3
				break;
			case exExprFunc::_if:
				valC = pStk->pop_num();
				valB = pStk->pop_num();
				valA = pStk->pop_num();
				pStk->push_num(valA != 0.0f ? valB : valC);
				break;
			case exExprFunc::_int:
				valA = pStk->pop_num();
				pStk->push_num(::mth_truncf(valA));
				break;
			case exExprFunc::_length:
				pStk->push_num(expr_len(pStk));
				break;
			case exExprFunc::_log:
				valA = pStk->pop_num();
				pStk->push_num(::mth_logf(valA));
				break;
			case exExprFunc::_log10:
				valA = pStk->pop_num();
				pStk->push_num(::mth_log10f(valA));
				break;
			case exExprFunc::_max:
				valB = pStk->pop_num();
				valA = pStk->pop_num();
				pStk->push_num(nxCalc::max(valA, valB));
				break;
			case exExprFunc::_min:
				valB = pStk->pop_num();
				valA = pStk->pop_num();
				pStk->push_num(nxCalc::min(valA, valB));
				break;
			case exExprFunc::_pow:
				valB = pStk->pop_num();
				valA = pStk->pop_num();
				pStk->push_num(::mth_powf(valA, valB));
				break;
			case exExprFunc::_rad:
				valA = pStk->pop_num();
				pStk->push_num(XD_DEG2RAD(valA));
				break;
			case exExprFunc::_rint:
			case exExprFunc::_round:
				valA = pStk->pop_num();
				pStk->push_num(::mth_roundf(valA));
				break;
			case exExprFunc::_sign:
				valA = pStk->pop_num();
				pStk->push_num(valA == 0.0f ? 0 : valA < 0.0f ? -1.0f : 1.0f);
				break;
			case exExprFunc::_sin:
				valA = pStk->pop_num();
				pStk->push_num(::mth_sinf(XD_DEG2RAD(valA)));
				break;
			case exExprFunc::_sqrt:
				valA = pStk->pop_num();
				pStk->push_num(::mth_sqrtf(valA));
				break;
			case exExprFunc::_tan:
				valA = pStk->pop_num();
				pStk->push_num(::mth_tanf(XD_DEG2RAD(valA)));
				break;
			default:
				break;
			}
			break;
		case eOp::XOR:
			valB = pStk->pop_num();
			valA = pStk->pop_num();
			pStk->push_num((float)((int)valA ^ (int)valB));
			break;
		case eOp::AND:
			valB = pStk->pop_num();
			valA = pStk->pop_num();
			pStk->push_num((float)((int)valA & (int)valB));
			break;
		case eOp::OR:
			valB = pStk->pop_num();
			valA = pStk->pop_num();
			pStk->push_num((float)((int)valA | (int)valB));
			break;
		default:
			break;
		}

		++pCode;
	}
	float res = pStk->pop_num();
	ifc.set_result(res);
}

#if XD_FILEFUNCS_ENABLED
static const char* s_exprOpNames[] = {
	"NOP", "END", "NUM", "STR", "VAR", "CMP", "ADD", "SUB", "MUL", "DIV", "MOD", "NEG", "FUN", "XOR", "AND", "OR"
};

static const char* s_exprCmpNames[] = {
	"EQ", "NE", "LT", "LE", "GT", "GE"
};

static const char* s_exprFuncNames[] = {
	"abs", "acos", "asin", "atan", "atan2", "ceil",
	"ch", "clamp", "cos", "deg", "detail", "distance",
	"exp", "fit", "fit01", "fit10", "fit11",
	"floor", "frac", "if", "int", "length",
	"log", "log10", "max", "min", "pow", "rad",
	"rint", "round", "sign", "sin", "sqrt", "tan"
};
#endif

void sxCompiledExpression::disasm(FILE* pFile) const {
#if XD_FILEFUNCS_ENABLED
	int n = mCodeNum;
	const Code* pCode = get_code_top();
	char abuf[128];
	for (int i = 0; i < n; ++i) {
		int addr = i;
		eOp op = pCode->get_op();
		const char* pOpName = "<bad>";
		if ((uint32_t)op < (uint32_t)XD_ARY_LEN(s_exprOpNames)) {
			pOpName = s_exprOpNames[(int)op];
		}
		const char* pArg = "";
		if (op == eOp::CMP) {
			eCmp cmp = (eCmp)pCode->mInfo;
			pArg = "<bad>";
			if ((uint32_t)cmp < (uint32_t)XD_ARY_LEN(s_exprCmpNames)) {
				pArg = s_exprCmpNames[(int)cmp];
			}
		} else if (op == eOp::NUM) {
			float num = get_val(pCode->mInfo);
#if defined(_MSC_VER)
			::sprintf_s(abuf, sizeof(abuf), "%f", num);
#else
			::sprintf(abuf, "%f", num);
#endif
			pArg = abuf;
		} else if (op == eOp::STR) {
			String str = get_str(pCode->mInfo);
			if (str.is_valid()) {
#if defined(_MSC_VER)
				::sprintf_s(abuf, sizeof(abuf), "%s", str.mpChars);
#else
				::sprintf(abuf, "%s", str.mpChars);
#endif
				pArg = abuf;
			}
		} else if (op == eOp::FUN) {
			int func = pCode->mInfo;
			pArg = "<bad>";
			if ((uint32_t)func < (uint32_t)XD_ARY_LEN(s_exprFuncNames)) {
				pArg = s_exprFuncNames[func];
			}
		}
		::fprintf(pFile, "0x%02X: %s %s\n", addr, pOpName, pArg);

		++pCode;
	}
#endif /* XD_FILEFUNCS_ENABLED */
}

int sxExprLibData::find_expr_idx(const char* pNodeName, const char* pChanName, const char* pNodePath, int startIdx) const {
	int idx = -1;
	sxStrList* pStrLst = get_str_list();
	if (ck_expr_idx(startIdx) && pStrLst) {
		int nameId = pStrLst->find_str(pNodeName);
		int chanId = pStrLst->find_str(pChanName);
		int pathId = pStrLst->find_str(pNodePath);
		bool doneFlg = (pNodeName && (nameId < 0)) || (pChanName && (chanId < 0)) || (pNodePath && pathId < 0);
		if (!doneFlg) {
			int n = get_expr_num();
			for (int i = startIdx; i < n; ++i) {
				const ExprInfo* pInfo = get_info(i);
				bool nameFlg = pNodeName ? pInfo->mNodeNameId == nameId : true;
				bool chanFlg = pChanName ? pInfo->mChanNameId == chanId : true;
				bool pathFlg = pNodePath ? pInfo->mNodePathId == pathId : true;
				if (nameFlg && chanFlg && pathFlg) {
					idx = i;
					break;
				}
			}
		}
	}
	return idx;
}

sxExprLibData::Entry sxExprLibData::get_entry(int idx) const {
	Entry ent;
	if (ck_expr_idx(idx)) {
		ent.mpLib = this;
		ent.mExprId = idx;
	} else {
		ent.mpLib = nullptr;
		ent.mExprId = -1;
	}
	return ent;
}

int sxExprLibData::count_rig_entries(const sxRigData& rig) const {
	int n = 0;
	sxStrList* pStrLst = get_str_list();
	for (int i = 0; i < rig.get_nodes_num(); ++i) {
		const char* pNodeName = rig.get_node_name(i);
		const char* pNodePath = rig.get_node_path(i);
		int nameId = pStrLst->find_str(pNodeName);
		int pathId = pStrLst->find_str(pNodePath);
		if (nameId >= 0) {
			for (int j = 0; j < get_expr_num(); ++j) {
				const ExprInfo* pInfo = get_info(j);
				bool nameFlg = pInfo->mNodeNameId == nameId;
				bool pathFlg = pathId < 0 ? true : pInfo->mNodePathId == pathId;
				if (nameFlg && pathFlg) {
					++n;
				}
			}
		}
	}
	return n;
}


size_t sxModelData::get_vtx_size() const {
	size_t vsize = 0;
	if (half_encoding()) {
		vsize = has_skin() ? sizeof(VtxSkinHalf) : sizeof(VtxRigidHalf);
	} else {
		vsize = has_skin() ? sizeof(VtxSkinShort) : sizeof(VtxRigidShort);
	}
	return vsize;
}

cxVec sxModelData::get_pnt_pos(const int pid) const {
	cxVec pos(0.0f);
	if (mPntOffs && ck_pid(pid)) {
		if (half_encoding()) {
			if (has_skin()) {
				const VtxSkinHalf* pVtx = reinterpret_cast<const VtxSkinHalf*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				pos.from_mem(pVtx->pos);
			} else {
				const VtxRigidHalf* pVtx = reinterpret_cast<const VtxRigidHalf*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				pos.from_mem(pVtx->pos);
			}
		} else {
			if (has_skin()) {
				const VtxSkinShort* pVtx = reinterpret_cast<const VtxSkinShort*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				pos.set(float(pVtx->qpos[0]), float(pVtx->qpos[1]), float(pVtx->qpos[2]));
				pos.scl(1.0f / float(0xFFFF));
				pos.mul(mBBox.get_size_vec());
				pos.add(mBBox.get_min_pos());
			} else {
				const VtxRigidShort* pVtx = reinterpret_cast<const VtxRigidShort*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				pos.from_mem(pVtx->pos);
			}
		}
	}
	return pos;
}

cxVec sxModelData::get_pnt_nrm(const int pid) const {
	cxVec nrm = nxVec::get_axis(exAxis::PLUS_Z);
	if (mPntOffs && ck_pid(pid)) {
		if (half_encoding()) {
			if (has_skin()) {
				const VtxSkinHalf* pVtx = reinterpret_cast<const VtxSkinHalf*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				nrm = pVtx->get_normal();
			} else {
				const VtxRigidHalf* pVtx = reinterpret_cast<const VtxRigidHalf*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				nrm = pVtx->get_normal();
			}
		} else {
			if (has_skin()) {
				const VtxSkinShort* pVtx = reinterpret_cast<const VtxSkinShort*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				nrm = pVtx->get_normal();
			} else {
				const VtxRigidShort* pVtx = reinterpret_cast<const VtxRigidShort*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				nrm = pVtx->get_normal();
			}
		}
	}
	return nrm;
}

cxColor sxModelData::get_pnt_clr(const int pid) const {
	cxColor clr(1.0f);
	if (mPntOffs && ck_pid(pid)) {
		if (half_encoding()) {
			if (has_skin()) {
				const VtxSkinHalf* pVtx = reinterpret_cast<const VtxSkinHalf*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				clr.set(pVtx->clr);
			} else {
				const VtxRigidHalf* pVtx = reinterpret_cast<const VtxRigidHalf*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				clr.set(pVtx->clr);
			}
		} else {
			if (has_skin()) {
				const VtxSkinShort* pVtx = reinterpret_cast<const VtxSkinShort*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				for (int i = 0; i < 4; ++i) {
					clr[i] = float(pVtx->clr[i]);
				}
			} else {
				const VtxRigidShort* pVtx = reinterpret_cast<const VtxRigidShort*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				for (int i = 0; i < 4; ++i) {
					clr[i] = float(pVtx->clr[i]);
				}
			}
			clr.scl(1.0f / float(0x7FF));
		}
	}
	return clr;
}

xt_texcoord sxModelData::get_pnt_tex(const int pid) const {
	xt_texcoord tex;
	tex.fill(0.0f);
	if (mPntOffs && ck_pid(pid)) {
		if (half_encoding()) {
			if (has_skin()) {
				const VtxSkinHalf* pVtx = reinterpret_cast<const VtxSkinHalf*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				tex.set(pVtx->tex.get());
			} else {
				const VtxRigidHalf* pVtx = reinterpret_cast<const VtxRigidHalf*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				tex.set(pVtx->tex.get());
			}
		} else {
			if (has_skin()) {
				const VtxSkinShort* pVtx = reinterpret_cast<const VtxSkinShort*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				for (int i = 0; i < 2; ++i) {
					tex[i] = float(pVtx->tex[i]);
				}
				tex.scl(1.0f / float(0x7FF));
			} else {
				const VtxRigidShort* pVtx = reinterpret_cast<const VtxRigidShort*>(XD_INCR_PTR(this, mPntOffs)) + pid;
				tex.set(pVtx->tex);
			}
		}
	}
	return tex;
}

sxModelData::PntSkin sxModelData::get_pnt_skin(const int pid) const {
	PntSkin skn;
	nxCore::mem_zero(&skn, sizeof(skn));
	if (has_skin() && mPntOffs && ck_pid(pid)) {
		float wgt[4];
		uint8_t idx[4];
		const float s = 1.0f / 0xFFFF;
		if (half_encoding()) {
			const VtxSkinHalf* pVtx = reinterpret_cast<const VtxSkinHalf*>(XD_INCR_PTR(this, mPntOffs)) + pid;
			for (int i = 0; i < 4; ++i) {
				wgt[i] = float(pVtx->wgt[i]) * s;
				idx[i] = pVtx->jnt[i];
			}
		} else {
			const VtxSkinShort* pVtx = reinterpret_cast<const VtxSkinShort*>(XD_INCR_PTR(this, mPntOffs)) + pid;
			wgt[0] = float(pVtx->w0) * s;
			wgt[1] = float(pVtx->w1) * s;
			wgt[2] = float(pVtx->w2) * s;
			wgt[3] = 1.0f - (wgt[0] + wgt[1] + wgt[2]);
			for (int i = 0; i < 4; ++i) {
				idx[i] = pVtx->jnt[i];
			}
		}
		for (int i = 0; i < 4; ++i) {
			if (wgt[i] == 0.0f || (i > 0 && idx[i] == idx[i - 1])) break;
			++skn.num;
		}
		for (int i = 0; i < skn.num; ++i) {
			skn.idx[i] = idx[i];
			skn.wgt[i] = wgt[i];
		}
	}
	return skn;
}

const cxSphere* sxModelData::get_mdl_spheres() const {
	const cxSphere* pSph = nullptr;
	const SkinInfo* pInfo = get_skin_info();
	if (pInfo) {
		pSph = reinterpret_cast<const cxSphere*>(XD_INCR_PTR(this, pInfo->mMdlSpheresOffs));
	}
	return pSph;
}

const sxModelData::Material* sxModelData::get_material(const int imtl) const {
	const Material* pMtl = nullptr;
	if (mMtlOffs && ck_mtl_id(imtl)) {
		pMtl = reinterpret_cast<const Material*>(XD_INCR_PTR(this, mMtlOffs)) + imtl;
	}
	return pMtl;
}

int sxModelData::find_material_id(const char* pName) const {
	int imtl = -1;
	int iname = find_str(pName);
	if (iname >= 0) {
		for (uint32_t i = 0; i < mMtlNum; ++i) {
			const Material* pMtl = get_material(i);
			if (pMtl && pMtl->mNameId == iname) {
				imtl = int(i);
				break;
			}
		}
	}
	return imtl;
}

const char* sxModelData::get_material_name(const int imtl) const {
	const char* pMtlName = nullptr;
	const Material* pMtl = get_material(imtl);
	if (pMtl) {
		pMtlName = get_str(pMtl->mNameId);
	}
	return pMtlName;
}

const char* sxModelData::get_material_path(const int imtl) const {
	const char* pMtlPath = nullptr;
	const Material* pMtl = get_material(imtl);
	if (pMtl) {
		pMtlPath = get_str(pMtl->mPathId);
	}
	return pMtlPath;
}

bool sxModelData::mtl_has_swaps(const int imtl) const {
	const Material* pMtl = get_material(imtl);
	if (!pMtl) return false;
	return pMtl->mSwapOffs > 0;
}

int sxModelData::get_mtl_num_swaps(const int imtl) const {
	int n = 0;
	if (mtl_has_swaps(imtl)) {
		const Material* pMtl = get_material(imtl);
		if (pMtl) {
			const int32_t* pSwps = reinterpret_cast<const int32_t*>(XD_INCR_PTR(this, pMtl->mSwapOffs));
			n = pSwps[0];
		}
	}
	return n;
}

const sxModelData::Material* sxModelData::get_swap_material(const int imtl, const int iswp) const {
	const Material* pSwpMtl = nullptr;
	int nswp = get_mtl_num_swaps(imtl);
	if (iswp > 0 && iswp <= nswp) { /* [1..nswp] */
		const Material* pMtl = get_material(imtl);
		if (pMtl) {
			const int32_t* pSwps = reinterpret_cast<const int32_t*>(XD_INCR_PTR(this, pMtl->mSwapOffs));
			int id = pSwps[iswp];
			if (id >= 0) {
				pSwpMtl = reinterpret_cast<const Material*>(XD_INCR_PTR(this, mMtlOffs)) + mMtlNum + id;
			}
		}
	}
	return pSwpMtl;
}

bool sxModelData::mtl_has_exts(const int imtl) const {
	const Material* pMtl = get_material(imtl);
	if (!pMtl) return false;
	return pMtl->mExtOffs > 0;
}

const sxData::ExtList* sxModelData::get_mtl_ext_list(const int imtl) const {
	const ExtList* pLst = nullptr;
	const Material* pMtl = get_material(imtl);
	if (pMtl && pMtl->mExtOffs > 0) {
		pLst = reinterpret_cast<const ExtList*>(XD_INCR_PTR(this, pMtl->mExtOffs));
	}
	return pLst;
}

int sxModelData::get_mtl_num_exts(const int imtl) const {
	int n = 0;
	const ExtList* pLst = get_mtl_ext_list(imtl);
	if (pLst) {
		n = pLst->num;
	}
	return n;
}

uint32_t sxModelData::find_mtl_ext_offs(const int imtl, const uint32_t kind) const {
	uint32_t offs = 0;
	const ExtList* pLst = get_mtl_ext_list(imtl);
	if (pLst) {
		for (uint32_t i = 0; i < pLst->num; ++i) {
			if (kind == pLst->lst[i].kind) {
				offs = pLst->lst[i].offs;
				break;
			}
		}
	}
	return offs;
}

const sxModelData::Material::BasePatternExt* sxModelData::find_mtl_base_pattern_ext(const int imtl) const {
	const Material::BasePatternExt* pPat = nullptr;
	uint32_t offs = find_mtl_ext_offs(imtl, XD_FOURCC('P', 'a', 't', 'B'));
	if (offs > 0) {
		pPat = reinterpret_cast<const Material::BasePatternExt*>(XD_INCR_PTR(this, offs));
	}
	return pPat;
}

const sxModelData::Material::SpecPatternExt* sxModelData::find_mtl_spec_pattern_ext(const int imtl) const {
	const Material::SpecPatternExt* pPat = nullptr;
	uint32_t offs = find_mtl_ext_offs(imtl, XD_FOURCC('P', 'a', 't', 'S'));
	if (offs > 0) {
		pPat = reinterpret_cast<const Material::SpecPatternExt*>(XD_INCR_PTR(this, offs));
	}
	return pPat;
}

const sxModelData::Material::NormPatternExt* sxModelData::find_mtl_norm_pattern_ext(const int imtl) const {
	const Material::NormPatternExt* pPat = nullptr;
	uint32_t offs = find_mtl_ext_offs(imtl, XD_FOURCC('P', 'a', 't', 'N'));
	if (offs > 0) {
		pPat = reinterpret_cast<const Material::NormPatternExt*>(XD_INCR_PTR(this, offs));
	}
	return pPat;
}

cxAABB sxModelData::get_batch_bbox(const int ibat) const {
	cxAABB bbox;
	if (mBatOffs && ck_batch_id(ibat)) {
		const cxAABB* pBBox = reinterpret_cast<const cxAABB*>(XD_INCR_PTR(this, mBatOffs)) + ibat;
		bbox = *pBBox;
	} else {
		bbox.init();
	}
	return bbox;
}

const sxModelData::Batch* sxModelData::get_batch_ptr(const int ibat) const {
	const Batch* pBat = nullptr;
	if (mBatOffs && ck_batch_id(ibat)) {
		pBat = reinterpret_cast<const Batch*>(XD_INCR_PTR(this, mBatOffs + (mBatNum * sizeof(cxAABB)))) + ibat;
	}
	return pBat;
}

const char* sxModelData::get_batch_name(const int ibat) const {
	const char* pName = nullptr;
	const Batch* pBat = get_batch_ptr(ibat);
	if (pBat) {
		pName = get_str(pBat->mNameId);
	}
	return pName;
}

xt_int3 sxModelData::get_batch_tri_indices(const int ibat, const int itri) const {
	xt_int3 vidx;
	vidx.fill(-1);
	const Batch* pBat = get_batch_ptr(ibat);
	if (pBat) {
		if (itri >= 0 && itri < pBat->mTriNum) {
			if (pBat->is_idx16()) {
				const uint16_t* pIdx = get_idx16_top();
				if (pIdx) {
					for (int i = 0; i < 3; ++i) {
						vidx[i] = pBat->mMinIdx + pIdx[pBat->mIdxOrg + (itri * 3) + i];
					}
				}
			} else {
				const uint32_t* pIdx = get_idx32_top();
				for (int i = 0; i < 3; ++i) {
					vidx[i] = int32_t(pBat->mMinIdx + pIdx[pBat->mIdxOrg + (itri * 3) + i]);
				}
			}
		}
	}
	return vidx;
}

int sxModelData::get_batch_jnt_num(const int ibat) const {
	int n = 0;
	if (has_skin()) {
		const Batch* pBat = get_batch_ptr(ibat);
		if (pBat) {
			n = pBat->mJntNum;
		}
	}
	return n;
}

const int32_t* sxModelData::get_batch_jnt_list(const int ibat) const {
	const int32_t* pLst = nullptr;
	if (has_skin()) {
		const Batch* pBat = get_batch_ptr(ibat);
		const SkinInfo* pInfo = get_skin_info();
		if (pBat && pInfo && pInfo->mBatJntListsOffs) {
			pLst = reinterpret_cast<const int32_t*>(XD_INCR_PTR(this, pInfo->mBatJntListsOffs)) + pBat->mJntInfoOrg;
		}
	}
	return pLst;
}

const cxSphere* sxModelData::get_batch_spheres(const int ibat) const {
	const cxSphere* pSph = nullptr;
	if (has_skin()) {
		const Batch* pBat = get_batch_ptr(ibat);
		const SkinInfo* pInfo = get_skin_info();
		if (pBat && pInfo && pInfo->mBatSpheresOffs) {
			pSph = reinterpret_cast<const cxSphere*>(XD_INCR_PTR(this, pInfo->mBatSpheresOffs)) + pBat->mJntInfoOrg;
		}
	}
	return pSph;
}

const sxModelData::Material* sxModelData::get_batch_material(const int ibat) const {
	const Material* pMtl = nullptr;
	const Batch* pBat = get_batch_ptr(ibat);
	if (pBat) {
		pMtl = get_material(pBat->mMtlId);
	}
	return pMtl;
}

int sxModelData::count_alpha_batches() const {
	int n = 0;
	for (uint32_t i = 0; i < mBatNum; ++i) {
		const Material* pMtl = get_batch_material(i);
		if (pMtl) {
			if (pMtl->is_alpha()) {
				++n;
			}
		}
	}
	return n;
}

const sxModelData::TexInfo* sxModelData::get_tex_info(const int tid) const {
	const TexInfo* pTex = nullptr;
	if (mTexOffs && ck_tex_id(tid)) {
		pTex = reinterpret_cast<const TexInfo*>(XD_INCR_PTR(this, mTexOffs)) + tid;
	}
	return pTex;
}

sxModelData::TexInfo* sxModelData::get_tex_info(const int tid) {
	TexInfo* pTex = nullptr;
	if (mTexOffs && ck_tex_id(tid)) {
		pTex = reinterpret_cast<TexInfo*>(XD_INCR_PTR(this, mTexOffs)) + tid;
	}
	return pTex;
}

void sxModelData::clear_tex_wk() {
	for (uint32_t i = 0; i < mTexNum; ++i) {
		TexInfo* pTexInfo = get_tex_info(i);
		if (pTexInfo) {
			nxCore::mem_zero(pTexInfo->mWk, sizeof(pTexInfo->mWk));
		}
	}
}

const char* sxModelData::get_tex_name(const int tid) const {
	const char* pTexName = nullptr;
	const TexInfo* pTexInfo = get_tex_info(tid);
	if (pTexInfo) {
		pTexName = get_str(pTexInfo->mNameId);
	}
	return pTexName;
}

const char* sxModelData::get_tex_path(const int tid) const {
	const char* pTexPath = nullptr;
	const TexInfo* pTexInfo = get_tex_info(tid);
	if (pTexInfo) {
		pTexPath = get_str(pTexInfo->mPathId);
	}
	return pTexPath;
}

const char* sxModelData::get_skin_name(const int iskn) const {
	const char* pName = nullptr;
	if (has_skin() && ck_skin_id(iskn)) {
		const SkinInfo* pInfo = get_skin_info();
		if (pInfo && pInfo->mNamesOffs) {
			int nameId = reinterpret_cast<const int32_t*>(XD_INCR_PTR(this, pInfo->mNamesOffs))[iskn];
			pName = get_str(nameId);
		}
	}
	return pName;
}

const int32_t* sxModelData::get_skin_to_skel_map() const {
	const int32_t* pMap = nullptr;
	if (has_skin()) {
		const SkinInfo* pInfo = get_skin_info();
		if (pInfo && pInfo->mSkelMapOffs) {
			pMap = reinterpret_cast<const int32_t*>(XD_INCR_PTR(this, pInfo->mSkelMapOffs));
		}
	}
	return pMap;
}

int sxModelData::find_skin_id(const char* pName) const {
	int iskn = -1;
	int iname = find_str(pName);
	if (iname >= 0) {
		const SkinInfo* pInfo = get_skin_info();
		if (pInfo && pInfo->mNamesOffs) {
			const int32_t* pNameIds = reinterpret_cast<const int32_t*>(XD_INCR_PTR(this, pInfo->mNamesOffs));
			for (uint32_t i = 0; i < mSknNum; ++i) {
				int id = pNameIds[i];
				if (id == iname) {
					iskn = int(i);
					break;
				}
			}
		}
	}
	return iskn;
}

const xt_xmtx* sxModelData::get_skel_xforms_ptr() const {
	const xt_xmtx* pXforms = nullptr;
	if (mSklOffs > 0) {
		pXforms = reinterpret_cast<const xt_xmtx*>(XD_INCR_PTR(this, mSklOffs));
	}
	return pXforms;
}

xt_xmtx sxModelData::get_skel_local_xform(const int inode) const {
	xt_xmtx xm;
	xm.identity();
	if (ck_skel_id(inode)) {
		const xt_xmtx* pXforms = get_skel_xforms_ptr();
		if (pXforms) {
			xm = pXforms[inode];
		}
	}
	return xm;
}

xt_xmtx sxModelData::get_skel_inv_world_xform(const int inode) const {
	xt_xmtx xm;
	xm.identity();
	if (ck_skel_id(inode)) {
		const xt_xmtx* pXforms = get_skel_xforms_ptr();
		if (pXforms) {
			xm = pXforms[mSklNum + inode];
		}
	}
	return xm;
}

XD_NOINLINE xt_xmtx sxModelData::calc_skel_node_world_xform(const int inode, const xt_xmtx* pLocXforms, xt_xmtx* pParentXform) const {
	xt_xmtx xm;
	xm.identity();
	xt_xmtx xmp;
	xmp.identity();
	if (has_skel() && ck_skel_id(inode)) {
		const xt_xmtx* pL = pLocXforms ? pLocXforms : get_skel_xforms_ptr();
		if (pL) {
			const int32_t* pParents = get_skel_parents_ptr();
			int idx = pParents[inode];
			while (ck_skel_id(idx)) {
				xm = nxMtx::xmtx_concat(xm, pL[idx]);
				idx = pParents[idx];
			}
			xmp = xm;
			xm = nxMtx::xmtx_concat(pL[inode], xmp);
		}
	}
	if (pParentXform) {
		*pParentXform = xmp;
	}
	return xm;
}

XD_NOINLINE xt_xmtx sxModelData::calc_skel_node_chain_xform(const int inode, const int itop, const xt_xmtx* pLocXforms) const {
	xt_xmtx xm;
	xm.identity();
	if (has_skel() && ck_skel_id(inode)) {
		const xt_xmtx* pL = pLocXforms ? pLocXforms : get_skel_xforms_ptr();
		if (pL) {
			if (inode != itop) {
				const int32_t* pParents = get_skel_parents_ptr();
				int idx = pParents[inode];
				while (ck_skel_id(idx)) {
					xm = nxMtx::xmtx_concat(xm, pL[idx]);
					if (idx == itop) {
						break;
					}
					idx = pParents[idx];
				}
			}
			xm = nxMtx::xmtx_concat(pL[inode], xm);
		}
	}
	return xm;
}

cxVec sxModelData::get_skel_local_offs(const int inode) const {
	xt_xmtx lx = get_skel_local_xform(inode);
	return nxMtx::xmtx_get_pos(lx);
}

const int32_t* sxModelData::get_skel_names_ptr() const {
	const int32_t* p = nullptr;
	const xt_xmtx* pXforms = get_skel_xforms_ptr();
	if (pXforms) {
		p = reinterpret_cast<const int32_t*>(pXforms + mSklNum*2);
	}
	return p;
}

const int32_t* sxModelData::get_skel_parents_ptr() const {
	const int32_t* p = nullptr;
	const int32_t* pNames = get_skel_names_ptr();
	if (pNames) {
		p = pNames + mSklNum;
	}
	return p;
}

const char* sxModelData::get_skel_name(const int iskl) const {
	const char* pName = nullptr;
	if (ck_skel_id(iskl)) {
		const int32_t* pIdTbl = get_skel_names_ptr();
		if (pIdTbl) {
			pName = get_str(pIdTbl[iskl]);
		}
	}
	return pName;
}

int sxModelData::find_skel_node_id(const char* pName) const {
	int inode = -1;
	const int32_t* pNames = get_skel_names_ptr();
	if (pNames) {
		int iname = find_str(pName);
		if (iname >= 0) {
			for (uint32_t i = 0; i < mSklNum; ++i) {
				if (pNames[i] == iname) {
					inode = int(i);
					break;
				}
			}
		}
	}
	return inode;
}

void sxModelData::dump_geo(FILE* pOut) const {
#if XD_FILEFUNCS_ENABLED
	::fprintf(pOut, "PGEOMETRY V5\n");
	::fprintf(pOut, "NPoints %d NPrims %d\n", int(mPntNum), int(mTriNum));
	::fprintf(pOut, "NPointGroups 0 NPrimGroups %d\n", int(mBatNum));
	::fprintf(pOut, "NPointAttrib 4 NVertexAttrib 0 NPrimAttrib 0 NAttrib 0\n");
	::fprintf(pOut, "PointAttrib\n");
	::fprintf(pOut, "N 3 vector 0 0 0\n");
	::fprintf(pOut, "Cd 3 float 1 1 1\n");
	::fprintf(pOut, "Alpha 1 float 1\n");
	::fprintf(pOut, "uv 3 float 0 0 1\n");
	for (uint32_t i = 0; i < mPntNum; ++i) {
		cxVec pos = get_pnt_pos(i);
		cxVec nrm = get_pnt_nrm(i);
		cxColor clr = get_pnt_clr(i);
		xt_texcoord tex = get_pnt_tex(i);
		::fprintf(pOut, "%f %f %f 1 (%f %f %f  %f %f %f  %f  %f %f 1)\n",
		          pos.x, pos.y, pos.z,
		          nrm.x, nrm.y, nrm.z,
		          clr.r, clr.g, clr.b,
		          clr.a,
		          tex.u, 1.0f - tex.v
		);
	}
	::fprintf(pOut, "Run %d Poly\n", int(mTriNum));
	for (uint32_t i = 0; i < mBatNum; ++i) {
		const Batch* pBat = get_batch_ptr(i);
		if (pBat) {
			for (int j = 0; j < pBat->mTriNum; ++j) {
				xt_int3 vidx = get_batch_tri_indices(i, j);
				::fprintf(pOut, " 3 < %d %d %d\n", int(vidx[0]), int(vidx[1]), int(vidx[2]));
			}
		}
	}
	uint32_t triOrg = 0;
	for (uint32_t i = 0; i < mBatNum; ++i) {
		const Batch* pBat = get_batch_ptr(i);
		const char* pBatName = get_batch_name(i);
		::fprintf(pOut, "%s unordered\n", pBatName);
		::fprintf(pOut, "%d ", int(mTriNum));
		int cnt = 0;
		for (uint32_t j = 0; j < mTriNum; ++j) {
			::fprintf(pOut, "%c", j >= triOrg && j < triOrg + pBat->mTriNum ? '1' : '0');
			++cnt;
			if (cnt > 64) {
				::fprintf(pOut, "\n");
				cnt = 0;
			}
		}
		::fprintf(pOut, "\n");
		triOrg += pBat->mTriNum;
	}
	::fprintf(pOut, "beginExtra\n");
	::fprintf(pOut, "endExtra\n");
#endif
}

void sxModelData::dump_geo(const char* pOutPath) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOutPath) return;
	FILE* pOut = nxSys::fopen_w_txt(pOutPath);
	if (!pOut) {
		return;
	}
	dump_geo(pOut);
	::fclose(pOut);
#endif
}

void sxModelData::dump_ocapt(FILE* pOut, const char* pSkelPath) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOut) {
		return;
	}
	if (!has_skin()) {
		return;
	}
	if (pSkelPath == nullptr) {
		pSkelPath = "/obj/ANIM";
	}
	::fprintf(pOut, "1\n\n");
	for (uint32_t i = 0; i < mPntNum; ++i) {
		PntSkin skn = get_pnt_skin(i);
		for (int j = 0; j < skn.num; ++j) {
			const char* pJntName = get_skin_name(skn.idx[j]);
			::fprintf(pOut, "%d %s/%s/cregion 0 %f\n", int(i), pSkelPath, pJntName, skn.wgt[j]);
		}
	}
#endif
}

void sxModelData::dump_ocapt(const char* pOutPath, const char* pSkelPath) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOutPath) {
		return;
	}
	if (!has_skin()) {
		return;
	}
	FILE* pOut = nxSys::fopen_w_txt(pOutPath);
	if (!pOut) {
		return;
	}
	dump_ocapt(pOut);
	::fclose(pOut);
#endif
}

void sxModelData::dump_skel(FILE* pOut) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOut) {
		return;
	}
	if (!has_skel()) {
		return;
	}
	int nskel = mSklNum;
	::fprintf(pOut, "base = hou.node('obj')\n");
	::fprintf(pOut, "basePath = base.path() + '/'\n");
	sxRNG rng;
	nxCore::rng_seed(&rng, 1);
	for (int i = 0; i < nskel; ++i) {
		const char* pNodeName = get_skel_name(i);
		if (!pNodeName) pNodeName = "__none__";
		::fprintf(pOut, "# %d: %s\n", i, pNodeName);
		::fprintf(pOut, "nd = base.createNode('null', '%s')\n", pNodeName);
		int ctrlTyp = 1; // Circles
		int ctrlOri = 0; // All planes
		float ctrlScl = 0.005f;
		cxColor ctrlClr = cxColor(0.0f, 0.75f, 0.0f);
		const char* pCtrlShape = "rect";
		if (nxCore::str_eq(pNodeName, "root")) {
			ctrlOri = 2; // ZX plane
			ctrlScl = 0.1f;
		} else if (nxCore::str_eq(pNodeName, "n_Move")) {
			ctrlOri = 2; // ZX plane
			ctrlScl = 0.5f;
		} else if (nxCore::str_eq(pNodeName, "n_Center")) {
			ctrlTyp = 3; // Planes
			ctrlOri = 2; // ZX plane
			ctrlScl = 0.25f;
		}
		::fprintf(pOut, "nd.setParms({ 'controltype':%d,'orientation':%d,'geoscale':%f })\n", ctrlTyp, ctrlOri, ctrlScl);
		xt_xmtx lm = get_skel_local_xform(i);
		::fprintf(pOut, "nd.setParmTransform(hou.Matrix4([[%f, %f, %f, 0], [%f, %f, %f, 0], [%f, %f, %f, 0], [%f, %f, %f, 1]]))\n",
			lm.m[0][0], lm.m[1][0], lm.m[2][0],
			lm.m[0][1], lm.m[1][1], lm.m[2][1],
			lm.m[0][2], lm.m[1][2], lm.m[2][2],
			lm.m[0][3], lm.m[1][3], lm.m[2][3]
		); 
		::fprintf(pOut, "nd.setUserData('nodeshape', '%s')\n", pCtrlShape);
		if (find_skin_id(pNodeName) >= 0) {
			for (int j = 0; j < 3; ++j) {
				ctrlClr[j] = nxCalc::fit(nxCore::rng_f01(&rng), 0.0f, 1.0f, 0.25f, 0.5f);
			}
			::fprintf(pOut, "cr = nd.createNode('cregion', 'cregion')\n");
			::fprintf(pOut, "cr.setParms({ 'squashx':0.0001,'squashy' : 0.0001,'squashz' : 0.0001 })\n");
		}
		::fprintf(pOut, "nd.setParms({ 'dcolorr':%.4f,'dcolorg':%.4f,'dcolorb':%.4f })\n",
		          ctrlClr[0], ctrlClr[1], ctrlClr[2]);
	}
	const int32_t* pParents = get_skel_parents_ptr();
	for (int i = 0; i < nskel; ++i) {
		int iparent = pParents[i];
		if (iparent >= 0) {
			const char* pNodeName = get_skel_name(i);
			const char* pParentName = get_skel_name(iparent);
			if (pNodeName && pParentName) {
				::fprintf(pOut, "# %s -> %s\n", pNodeName, pParentName);
				::fprintf(pOut, "nd = hou.node(basePath + '%s')\n", pNodeName);
				::fprintf(pOut, "nd.setFirstInput(hou.node(basePath + '%s'))\n", pParentName);
			}
		}
	}
#endif
}

void sxModelData::dump_skel(const char* pOutPath) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOutPath) {
		return;
	}
	if (!has_skel()) {
		return;
	}
	FILE* pOut = nxSys::fopen_w_txt(pOutPath);
	if (!pOut) {
		return;
	}
	dump_skel(pOut);
	::fclose(pOut);
#endif
}

void sxModelData::dump_mdl_spheres(FILE* pOut) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOut) {
		return;
	}
	if (!has_skin()) {
		return;
	}
	const cxSphere* pSph = get_mdl_spheres();
	if (!pSph) {
		return;
	}
	int nskn = int(mSknNum);
	::fprintf(pOut, "PGEOMETRY V5\n");
	::fprintf(pOut, "NPoints %d NPrims %d\n", nskn, nskn);
	::fprintf(pOut, "NPointGroups 0 NPrimGroups %d\n", nskn);
	::fprintf(pOut, "NPointAttrib 0 NVertexAttrib 0 NPrimAttrib 0 NAttrib 0\n");
	for (int i = 0; i < nskn; ++i) {
		cxSphere sph = pSph[i];
		cxVec c = sph.get_center();
		::fprintf(pOut, "%f %f %f 1\n", c.x, c.y, c.z);
	}
	for (int i = 0; i < nskn; ++i) {
		cxSphere sph = pSph[i];
		float r = sph.get_radius();
		::fprintf(pOut, "Sphere %d  %f 0 0  0 %f 0  0 0 %f\n", i, r, r, r);
	}
	for (int i = 0; i < nskn; ++i) {
		::fprintf(pOut, "%s unordered\n", get_skin_name(i));
		::fprintf(pOut, "%d", nskn);
		for (int j = 0; j < nskn; ++j) {
			::fprintf(pOut, " %c", i == j ? '1' : '0');
		}
		::fprintf(pOut, "\n");
	}
	::fprintf(pOut, "beginExtra\n");
	::fprintf(pOut, "endExtra\n");
#endif
}

void sxModelData::dump_mdl_spheres(const char* pOutPath) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOutPath) {
		return;
	}
	if (!has_skin()) {
		return;
	}
	FILE* pOut = nxSys::fopen_w_txt(pOutPath);
	if (!pOut) {
		return;
	}
	dump_mdl_spheres(pOut);
	::fclose(pOut);
#endif
}

void sxModelData::dump_bat_spheres(FILE* pOut) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOut) {
		return;
	}
	if (!has_skin()) {
		return;
	}
	const SkinInfo* pInfo = get_skin_info();
	if (!pInfo) {
		return;
	}
	int n = pInfo->mBatItemsNum;
	::fprintf(pOut, "PGEOMETRY V5\n");
	::fprintf(pOut, "NPoints %d NPrims %d\n", n, n);
	::fprintf(pOut, "NPointGroups 0 NPrimGroups %d\n", n);
	::fprintf(pOut, "NPointAttrib 0 NVertexAttrib 0 NPrimAttrib 0 NAttrib 0\n");
	for (uint32_t i = 0; i < mBatNum; ++i) {
		const Batch* pBat = get_batch_ptr(i);
		const cxSphere* pSph = get_batch_spheres(i);
		for (int j = 0; j < pBat->mJntNum; ++j) {
			cxSphere sph = pSph[j];
			cxVec c = sph.get_center();
			::fprintf(pOut, "%f %f %f 1\n", c.x, c.y, c.z);
		}
	}
	int isph = 0;
	for (uint32_t i = 0; i < mBatNum; ++i) {
		const Batch* pBat = get_batch_ptr(i);
		const cxSphere* pSph = get_batch_spheres(i);
		for (int j = 0; j < pBat->mJntNum; ++j) {
			cxSphere sph = pSph[j];
			float r = sph.get_radius();
			::fprintf(pOut, "Sphere %d  %f 0 0  0 %f 0  0 0 %f\n", isph, r, r, r);
			++isph;
		}
	}
	isph = 0;
	for (uint32_t i = 0; i < mBatNum; ++i) {
		const Batch* pBat = get_batch_ptr(i);
		const int32_t* pJnts = get_batch_jnt_list(i);
		for (int j = 0; j < pBat->mJntNum; ++j) {
			int ijnt = pJnts[j];
			const char* pJntName = get_skin_name(ijnt);
			::fprintf(pOut, "%s_%s unordered\n", get_batch_name(i), pJntName);
			::fprintf(pOut, "%d", n);
			for (int k = 0; k < n; ++k) {
				::fprintf(pOut, " %c", k == isph ? '1' : '0');
			}
			::fprintf(pOut, "\n");
			++isph;
		}
	}
	::fprintf(pOut, "beginExtra\n");
	::fprintf(pOut, "endExtra\n");
#endif
}

void sxModelData::dump_bat_spheres(const char* pOutPath) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOutPath) {
		return;
	}
	if (!has_skin()) {
		return;
	}
	FILE* pOut = nxSys::fopen_w_txt(pOutPath);
	if (!pOut) {
		return;
	}
	dump_bat_spheres(pOut);
	::fclose(pOut);
#endif
}


bool sxTextureData::is_gamma2() const {
	bool res = false;
	Format fmt = get_format();
	switch (fmt) {
		case Format::RGBA_BYTE_GAMMA2:
			res = true;
			break;
		default:
			break;
	}
	return res;
}


const sxMotionData::Node* sxMotionData::get_node(const int inode) const {
	const Node* pNode = nullptr;
	if (mNodeOffs && ck_node_id(inode)) {
		pNode = reinterpret_cast<const Node*>(XD_INCR_PTR(this, mNodeOffs)) + inode;
	}
	return pNode;
}

const char* sxMotionData::get_node_name(const int inode) const {
	const char* pName = nullptr;
	const Node* pNode = get_node(inode);
	if (pNode) {
		pName = get_str(pNode->mNameId);
	}
	return pName;
}

int sxMotionData::find_node_id(const char* pName) const {
	int id = -1;
	const Node* pNodes = get_nodes_top();
	if (pNodes) {
		int nameId = find_str(pName);
		if (nameId >= 0) {
			for (uint32_t i = 0; i < mNodeNum; ++i) {
				if (pNodes[i].mNameId == nameId) {
					id = int(i);
					break;
				}
			}
		}
	}
	return id;
}

const sxMotionData::Node* sxMotionData::find_node(const char* pName) const {
	const Node* pNode = nullptr;
	int id = find_node_id(pName);
	if (id >= 0) {
		pNode = get_nodes_top() + id;
	}
	return pNode;
}

const sxMotionData::Track* sxMotionData::get_q_track(const int inode) const {
	Track* pTrk = nullptr;
	const Node* pNode = get_node(inode);
	if (pNode) {
		if (pNode->mTrkOffsQ) {
			pTrk = (Track*)XD_INCR_PTR(this, pNode->mTrkOffsQ);
		}
	}
	return pTrk;
}

const sxMotionData::Track* sxMotionData::get_t_track(const int inode) const {
	Track* pTrk = nullptr;
	const Node* pNode = get_node(inode);
	if (pNode) {
		if (pNode->mTrkOffsT) {
			pTrk = (Track*)XD_INCR_PTR(this, pNode->mTrkOffsT);
		}
	}
	return pTrk;
}

struct XMOTFrameInfo {
	float f;
	float t;
	int i0;
	int i1;

	void calc(float frm, int nfrm) {
		f = ::mth_fmodf(::mth_fabsf(frm), float(nfrm));
		i0 = int(::mth_floorf(f));
		t = f - float(i0);
		if (frm < 0.0f) t = 1.0f - t;
		i1 = i0 < nfrm - 1 ? i0 + 1 : 0;
	}

	bool need_interp() const {
		return t != 0.0f && i0 != i1;
	}
};

static cxVec xmot_get_trk_vec(const sxMotionData::Track* pTrk, int idx) {
	cxVec v;
	int mask = pTrk->get_mask();
	if (mask) {
		v.zero();
		const uint16_t* pSrc = &pTrk->mData[idx * pTrk->get_stride()];
		for (int i = 0; i < 3; ++i) {
			if (mask & (1 << i)) {
				v.set_at(i, float(*pSrc++));
			}
		}
		v.scl(1.0f / 0xFFFF);
		v.mul(pTrk->mBBox.get_size_vec());
		v.add(pTrk->mBBox.get_min_pos());
	} else {
		v = pTrk->mBBox.get_min_pos();
	}
	return v;
}

cxQuat sxMotionData::eval_quat(const int inode, const float frm) const {
	cxQuat q;
	const Track* pTrk = get_q_track(inode);
	if (pTrk) {
		XMOTFrameInfo fi;
		fi.calc(frm, mFrameNum);
		if (fi.need_interp()) {
			cxVec v0 = xmot_get_trk_vec(pTrk, fi.i0);
			cxVec v1 = xmot_get_trk_vec(pTrk, fi.i1);
			cxQuat q0 = nxQuat::from_log_vec(v0);
			cxQuat q1 = nxQuat::from_log_vec(v1);
			q.slerp(q0, q1, fi.t);
		} else {
			q.from_log_vec(xmot_get_trk_vec(pTrk, fi.i0));
		}
	} else {
		q.identity();
	}
	return q;
}

cxVec sxMotionData::eval_pos(const int inode, const float frm) const {
	cxVec pos;
	const Track* pTrk = get_t_track(inode);
	if (pTrk) {
		XMOTFrameInfo fi;
		fi.calc(frm, mFrameNum);
		if (fi.need_interp()) {
			cxVec t0 = xmot_get_trk_vec(pTrk, fi.i0);
			cxVec t1 = xmot_get_trk_vec(pTrk, fi.i1);
			pos = nxVec::lerp(t0, t1, fi.t);
		} else {
			pos = xmot_get_trk_vec(pTrk, fi.i0);
		}
	} else {
		pos.zero();
	}
	return pos;
}

void sxMotionData::dump_clip(FILE* pOut, const float fstep) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOut) return;
	const Node* pNodes = get_nodes_top();
	if (!pNodes) return;

	int nfrm = mFrameNum;
	int len = 0;
	float frm = 0.0f;
	float add = ::mth_fabsf(fstep);
	while (int(frm) < nfrm) {
		frm += add;
		++len;
	}
	int nch = 0;
	for (uint32_t i = 0; i < mNodeNum; ++i) {
		if (pNodes[i].mTrkOffsQ) {
			nch += 3;
		}
		if (pNodes[i].mTrkOffsT) {
			nch += 3;
		}
	}
	cxVec* pBuf = (cxVec*)nxCore::mem_alloc(len * sizeof(cxVec));
	::fprintf(pOut, "{\n");
	::fprintf(pOut, "	rate = %d\n", int(mFPS));
	::fprintf(pOut, "	start = -1\n");
	::fprintf(pOut, "	tracklength = %d\n", len);
	::fprintf(pOut, "	tracks = %d\n", nch);
	static const char* pXYZ = "xyz";
	for (uint32_t i = 0; i < mNodeNum; ++i) {
		const char* pNodeName = get_node_name(i);
		if (pNodes[i].mTrkOffsQ) {
			int ifrm = 0;
			frm = 0.0f;
			while (int(frm) < nfrm) {
				cxQuat q = eval_quat(i, frm);
				pBuf[ifrm++] = q.get_rot_degrees(pNodes[i].get_rot_ord());
				frm += add;
			}
			for (int j = 0; j < 3; ++j) {
				::fprintf(pOut, "   {\n");
				::fprintf(pOut, "      name = %s:r%c\n", pNodeName, pXYZ[j]);
				::fprintf(pOut, "      data =");
				for (int k = 0; k < len; ++k) {
					float val = pBuf[k].get_at(j);
					::fprintf(pOut, " %f", val);
				}
				::fprintf(pOut, "\n");
				::fprintf(pOut, "   }\n");
			}
		}
		if (pNodes[i].mTrkOffsT) {
			int ifrm = 0;
			frm = 0.0f;
			while (int(frm) < nfrm) {
				pBuf[ifrm++] = eval_pos(i, frm);
				frm += add;
			}
			for (int j = 0; j < 3; ++j) {
				::fprintf(pOut, "   {\n");
				::fprintf(pOut, "      name = %s:t%c\n", pNodeName, pXYZ[j]);
				::fprintf(pOut, "      data =");
				for (int k = 0; k < len; ++k) {
					float val = pBuf[k].get_at(j);
					::fprintf(pOut, " %f", val);
				}
				::fprintf(pOut, "\n");
				::fprintf(pOut, "   }\n");
			}
		}
	}
	::fprintf(pOut, "}\n");
#endif
}

void sxMotionData::dump_clip(const char* pOutPath, const float fstep) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOutPath) return;
	FILE* pOut = nxSys::fopen_w_txt(pOutPath);
	if (!pOut) {
		return;
	}
	dump_clip(pOut, fstep);
	::fclose(pOut);
#endif
}


cxVec sxCollisionData::get_pnt(const int ipnt) const {
	cxVec pnt;
	if (ck_pnt_id(ipnt) && mPntOffs) {
		const cxVec* pPnts = reinterpret_cast<const cxVec*>(XD_INCR_PTR(this, mPntOffs));
		pnt = pPnts[ipnt];
	} else {
		pnt.zero();
	}
	return pnt;
}

cxAABB sxCollisionData::get_pol_bbox(const int ipol) const {
	cxAABB bbox;
	if (ck_pol_id(ipol) && mPolBBoxOffs) {
		const cxAABB* pBB = reinterpret_cast<const cxAABB*>(XD_INCR_PTR(this, mPolBBoxOffs)) + ipol;
		bbox = *pBB;
	} else {
		bbox.init();
	}
	return bbox;
}

cxVec sxCollisionData::get_pol_normal(const int ipol) const {
	cxVec nrm;
	if (ck_pol_id(ipol) && mPolNrmOffs) {
		const uint16_t* pOct = reinterpret_cast<const uint16_t*>(XD_INCR_PTR(this, mPolNrmOffs)) + ipol;
		nrm = nxVec::decode_octa(pOct);
	} else {
		nrm = nxVec::get_axis(exAxis::PLUS_Y);
	}
	return nrm;
}

int sxCollisionData::get_pol_grp_id(const int ipol) const {
	int igrp = 0;
	if (ck_pol_id(ipol) && mPolGrpOffs) {
		const uint8_t* pGrp = reinterpret_cast<const uint8_t*>(XD_INCR_PTR(this, mPolGrpOffs)) + ipol;
		igrp = *pGrp;
	}
	return igrp;
}

int sxCollisionData::get_pol_num_vtx(const int ipol) const {
	int nvtx = 0;
	if (all_pols_same_size()) {
		nvtx = mMaxVtxPerPol;
	} else if (mPolVtxNumOffs) {
		const uint8_t* pNum = reinterpret_cast<const uint8_t*>(XD_INCR_PTR(this, mPolVtxNumOffs)) + ipol;
		nvtx = *pNum;
	}
	return nvtx;
}

int sxCollisionData::get_pol_num_tris(const int ipol) const {
	int ntri = 0;
	if (ck_pol_id(ipol)) {
		if (all_pols_same_size()) {
			ntri = mMaxVtxPerPol - 2;
		} else if (mPolVtxNumOffs) {
			const uint8_t* pNum = reinterpret_cast<const uint8_t*>(XD_INCR_PTR(this, mPolVtxNumOffs)) + ipol;
			ntri = *pNum - 2;
		}
	}
	return ntri;
}

int sxCollisionData::get_pol_pnt_idx(const int ipol, const int ivtx) const {
	int ipnt = -1;
	if (ck_pol_id(ipol) && mPolIdxOrgOffs && mPolIdxOffs) {
		if (ivtx >= 0) {
			int nvtx = get_pol_num_vtx(ipol);
			if (ivtx < nvtx) {
				const int32_t* pOrg = reinterpret_cast<const int32_t*>(XD_INCR_PTR(this, mPolIdxOrgOffs)) + ipol;
				int org = *pOrg;
				const int32_t* pIdx = reinterpret_cast<const int32_t*>(XD_INCR_PTR(this, mPolIdxOffs));
				ipnt = pIdx[org + ivtx];
			}
		}
	}
	return ipnt;
}

int sxCollisionData::get_pol_tri_pnt_idx(const int ipol, const int itri, const int ivtx) const {
	int ipnt = -1;
	if (ck_pol_id(ipol) && itri >= 0 && itri < get_pol_num_tris(ipol) && ivtx >= 0 && ivtx < 3) {
		if (all_tris() || get_pol_num_vtx(ipol) == 3) {
			ipnt = get_pol_pnt_idx(ipol, ivtx);
		} else {
			if (mPolTriOrgOffs > 0 && mPolTriIdxOffs > 0) {
				const int32_t* pTriOrg = reinterpret_cast<const int32_t*>(XD_INCR_PTR(this, mPolTriOrgOffs)) + ipol;
				int org = *pTriOrg + (itri * 3);
				const uint8_t* pTriIdx = reinterpret_cast<const uint8_t*>(XD_INCR_PTR(this, mPolTriIdxOffs));
				ipnt = get_pol_pnt_idx(ipol, pTriIdx[org + ivtx]);
			}
		}
	}
	return ipnt;
}

const char* sxCollisionData::get_grp_name(const int igrp) const {
	const char* pName = nullptr;
	if (ck_grp_id(igrp) && mGrpInfoOffs) {
		const GrpInfo* pInfo = reinterpret_cast<const GrpInfo*>(XD_INCR_PTR(this, mGrpInfoOffs)) + igrp;
		pName = get_str(pInfo->mNameId);
	}
	return pName;
}

const char* sxCollisionData::get_grp_path(const int igrp) const {
	const char* pPath = nullptr;
	if (ck_grp_id(igrp) && mGrpInfoOffs) {
		const GrpInfo* pInfo = reinterpret_cast<const GrpInfo*>(XD_INCR_PTR(this, mGrpInfoOffs)) + igrp;
		pPath = get_str(pInfo->mPathId);
	}
	return pPath;
}

int sxCollisionData::for_all_tris(const TriFunc func, void* pWk, const bool calcNormals) const {
	Tri tri;
	tri.nrm.zero();
	int ipnt[3];
	int triCnt = 0;
	bool contFlg = true;
	if (mPntOffs > 0) {
		const cxVec* pPnts = reinterpret_cast<const cxVec*>(XD_INCR_PTR(this, mPntOffs));
		for (uint32_t i = 0; i < mPolNum; ++i) {
			tri.ipol = i;
			int nptris = get_pol_num_tris(i);
			for (int j = 0; j < nptris; ++j) {
				tri.itri = j;
				for (int k = 0; k < 3; ++k) {
					ipnt[k] = get_pol_tri_pnt_idx(i, j, k);
				}
				for (int k = 0; k < 3; ++k) {
					tri.vtx[k] = pPnts[ipnt[k]];
				}
				if (calcNormals) {
					tri.nrm = nxGeom::tri_normal_cw(tri.vtx[0], tri.vtx[1], tri.vtx[2]);
				}
				contFlg = func(*this, tri, pWk);
				++triCnt;
				if (!contFlg) break;
			}
			if (!contFlg) break;
		}
	}
	return triCnt;
}

struct sxColRangeCtx {
	cxAABB mBBox;
	const sxCollisionData* mpCol;
	const sxCollisionData::TriFunc* mpFunc;
	void* mpFuncWk;
	int mTriCnt;
	bool mContinue;
	bool mCalcNormals;

	void for_pol_tris(const int ipol) {
		cxAABB polBBox = mpCol->get_pol_bbox(ipol);
		if (mBBox.overlaps(polBBox)) {
			sxCollisionData::Tri tri;
			const cxVec* pPnts = reinterpret_cast<const cxVec*>(XD_INCR_PTR(mpCol, mpCol->mPntOffs));
			int ipnt[3];
			tri.ipol = ipol;
			if (!mCalcNormals) {
				tri.nrm = mpCol->get_pol_normal(ipol);
			}
			int ntris = mpCol->get_pol_num_tris(ipol);
			for (int i = 0; i < ntris; ++i) {
				tri.itri = i;
				for (int j = 0; j < 3; ++j) {
					ipnt[j] = mpCol->get_pol_tri_pnt_idx(ipol, i, j);
				}
				for (int j = 0; j < 3; ++j) {
					tri.vtx[j] = pPnts[ipnt[j]];
				}
				if (nxGeom::tri_aabb_overlap(tri.vtx, mBBox.get_min_pos(), mBBox.get_max_pos())) {
					if (mCalcNormals) {
						tri.nrm = nxGeom::tri_normal_cw(tri.vtx[0], tri.vtx[1], tri.vtx[2]);
					}
					mContinue = (*mpFunc)(*mpCol, tri, mpFuncWk);
					++mTriCnt;
				}
				if (!mContinue) break;
			}
		}
	}

	void for_bvh_tris(const int inode) {
		if (!mContinue) return;
		const cxAABB* pNodeBBox = reinterpret_cast<const cxAABB*>(XD_INCR_PTR(mpCol, mpCol->mBVHBBoxOffs)) + inode;
		if (mBBox.overlaps(*pNodeBBox)) {
			const sxCollisionData::BVHNodeInfo* pNodeInfo = reinterpret_cast<const sxCollisionData::BVHNodeInfo*>(XD_INCR_PTR(mpCol, mpCol->mBVHInfoOffs)) + inode;
			if (pNodeInfo->is_leaf()) {
				for_pol_tris(pNodeInfo->get_pol_id());
			} else {
				for_bvh_tris(pNodeInfo->mLeft);
				for_bvh_tris(pNodeInfo->mRight);
			}
		}
	}
};

XD_NOINLINE int sxCollisionData::for_tris_in_range(const TriFunc func, const cxAABB& bbox, void* pWk, const bool calcNormals) const {
	sxColRangeCtx ctx;
	ctx.mpCol = this;
	ctx.mBBox = bbox;
	ctx.mpFunc = &func;
	ctx.mpFuncWk = pWk;
	ctx.mCalcNormals = calcNormals;
	ctx.mContinue = true;
	ctx.mTriCnt = 0;
	if (mBVHBBoxOffs && mBVHInfoOffs) {
		ctx.for_bvh_tris(0);
	} else {
		for (uint32_t i = 0; i < mPolNum; ++i) {
			ctx.for_pol_tris(i);
			if (!ctx.mContinue) break;
		}
	}
	return ctx.mTriCnt;
}

struct sxColHitCtx {
	cxAABB mBBox;
	cxLineSeg mSeg;
	const sxCollisionData* mpCol;
	const sxCollisionData::HitFunc* mpFunc;
	void* mpFuncWk;
	int mHitCnt;
	bool mContinue;

	void for_pol_tris(const int ipol) {
		cxAABB polBBox = mpCol->get_pol_bbox(ipol);
		if (mBBox.overlaps(polBBox) && polBBox.seg_ck(mSeg)) {
			sxCollisionData::Tri tri;
			const cxVec* pPnts = reinterpret_cast<const cxVec*>(XD_INCR_PTR(mpCol, mpCol->mPntOffs));
			int ipnt[3];
			tri.ipol = ipol;
			tri.nrm = mpCol->get_pol_normal(ipol);
			int ntris = mpCol->get_pol_num_tris(ipol);
			for (int i = 0; i < ntris; ++i) {
				tri.itri = i;
				for (int j = 0; j < 3; ++j) {
					ipnt[j] = mpCol->get_pol_tri_pnt_idx(ipol, i, j);
				}
				for (int j = 0; j < 3; ++j) {
					tri.vtx[j] = pPnts[ipnt[j]];
				}
				cxVec hitPos;
				bool hitFlg = nxGeom::seg_tri_intersect_cw(mSeg.get_pos0(), mSeg.get_pos1(), tri.vtx[0], tri.vtx[1], tri.vtx[2], &hitPos, &tri.nrm);
				if (hitFlg) {
					float dist = nxVec::dist(mSeg.get_pos0(), hitPos);
					mContinue = (*mpFunc)(*mpCol, tri, hitPos, dist, mpFuncWk);
					++mHitCnt;
				}
				if (!mContinue) break;
			}
		}
	}

	void for_bvh_tris(const int inode) {
		if (!mContinue) return;
		const cxAABB* pNodeBBox = reinterpret_cast<const cxAABB*>(XD_INCR_PTR(mpCol, mpCol->mBVHBBoxOffs)) + inode;
		if (mBBox.overlaps(*pNodeBBox)) {
			const sxCollisionData::BVHNodeInfo* pNodeInfo = reinterpret_cast<const sxCollisionData::BVHNodeInfo*>(XD_INCR_PTR(mpCol, mpCol->mBVHInfoOffs)) + inode;
			if (pNodeInfo->is_leaf()) {
				for_pol_tris(pNodeInfo->get_pol_id());
			} else {
				for_bvh_tris(pNodeInfo->mLeft);
				for_bvh_tris(pNodeInfo->mRight);
			}
		}
	}
};

XD_NOINLINE int sxCollisionData::hit_check(const HitFunc func, const cxLineSeg& seg, void* pWk) {
	sxColHitCtx ctx;
	ctx.mSeg = seg;
	ctx.mBBox.from_seg(seg);
	ctx.mpCol = this;
	ctx.mpFunc = &func;
	ctx.mpFuncWk = pWk;
	ctx.mContinue = true;
	ctx.mHitCnt = 0;
	if (mBVHBBoxOffs && mBVHInfoOffs) {
		ctx.for_bvh_tris(0);
	} else {
		for (uint32_t i = 0; i < mPolNum; ++i) {
			ctx.for_pol_tris(i);
			if (!ctx.mContinue) break;
		}
	}
	return ctx.mHitCnt;
}

static bool xcol_nearest_hit_func(const sxCollisionData& col, const sxCollisionData::Tri& tri, const cxVec& pos, const float dist, void* pWk) {
	if (!pWk) return false;
	sxCollisionData::NearestHit* pHit = (sxCollisionData::NearestHit*)pWk;
	if (pHit->count > 0) {
		if (dist < pHit->dist) {
			pHit->pos = pos;
			pHit->nrm = tri.nrm;
			pHit->dist = dist;
		}
	} else {
		pHit->pos = pos;
		pHit->nrm = tri.nrm;
		pHit->dist = dist;
	}
	++pHit->count;
	return true;
}

sxCollisionData::NearestHit sxCollisionData::nearest_hit(const cxLineSeg& seg) {
	NearestHit hit;
	hit.pos = seg.get_pos0();
	hit.dist = 0.0f;
	hit.count = 0;
	sxCollisionData::hit_check(xcol_nearest_hit_func, seg, &hit);
	return hit;
}

void sxCollisionData::dump_pol_geo(FILE* pOut) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOut) return;
	if (!mPntOffs) return;
	::fprintf(pOut, "PGEOMETRY V5\n");
	::fprintf(pOut, "NPoints %d NPrims %d\n", int(mPntNum), int(mPolNum));
	::fprintf(pOut, "NPointGroups 0 NPrimGroups 0\n");
	::fprintf(pOut, "NPointAttrib 0 NVertexAttrib 0 NPrimAttrib %d NAttrib 0\n", mGrpNum > 1 ? 2 : 1);
	const cxVec* pPnts = reinterpret_cast<const cxVec*>(XD_INCR_PTR(this, mPntOffs));
	for (uint32_t i = 0; i < mPntNum; ++i) {
		cxVec pos = pPnts[i];
		::fprintf(pOut, "%f %f %f 1\n", pos.x, pos.y, pos.z);
	}
	::fprintf(pOut, "PrimitiveAttrib\n");
	::fprintf(pOut, "N 3 vector 0 0 0\n");
	if (mGrpNum > 1) {
		::fprintf(pOut, "grp_name 1 index %d", int(mGrpNum));
		for (uint32_t i = 0; i < mGrpNum; ++i) {
			::fprintf(pOut, " %s", get_grp_name(i));
		}
		::fprintf(pOut, "\n");
	}
	::fprintf(pOut, "Run %d Poly\n", int(mPolNum));
	for (uint32_t i = 0; i < mPolNum; ++i) {
		int nvtx = get_pol_num_vtx(i);
		::fprintf(pOut, " %d <", nvtx);
		for (int j = 0; j < nvtx; ++j) {
			int ipnt = get_pol_pnt_idx(i, j);
			::fprintf(pOut, " %d", ipnt);
		}
		cxVec nrm = get_pol_normal(i);
		::fprintf(pOut, " [%f %f %f", nrm.x, nrm.y, nrm.z);
		if (mGrpNum > 1) {
			::fprintf(pOut, " %d", get_pol_grp_id(i));
		}
		::fprintf(pOut, "]");
		::fprintf(pOut, "\n");
	}
	::fprintf(pOut, "beginExtra\n");
	::fprintf(pOut, "endExtra\n");
#endif
}

void sxCollisionData::dump_pol_geo(const char* pOutPath) const {
#if XD_FILEFUNCS_ENABLED
	FILE* pOut = nxSys::fopen_w_txt(pOutPath);
	if (!pOut) {
		return;
	}
	dump_pol_geo(pOut);
	::fclose(pOut);
#endif
}

void sxCollisionData::dump_tri_geo(FILE* pOut) const {
#if XD_FILEFUNCS_ENABLED
	if (!pOut) return;
	if (!mPntOffs) return;
	::fprintf(pOut, "PGEOMETRY V5\n");
	::fprintf(pOut, "NPoints %d NPrims %d\n", int(mPntNum), int(mTriNum));
	::fprintf(pOut, "NPointGroups 0 NPrimGroups 0\n");
	::fprintf(pOut, "NPointAttrib 0 NVertexAttrib 0 NPrimAttrib %d NAttrib 0\n", mGrpNum > 1 ? 1 : 0);
	const cxVec* pPnts = reinterpret_cast<const cxVec*>(XD_INCR_PTR(this, mPntOffs));
	for (uint32_t i = 0; i < mPntNum; ++i) {
		cxVec pos = pPnts[i];
		::fprintf(pOut, "%f %f %f 1\n", pos.x, pos.y, pos.z);
	}
	if (mGrpNum > 1) {
		::fprintf(pOut, "PrimitiveAttrib\n");
		::fprintf(pOut, "grp_name 1 index %d", int(mGrpNum));
		for (uint32_t i = 0; i < mGrpNum; ++i) {
			::fprintf(pOut, " %s", get_grp_name(i));
		}
		::fprintf(pOut, "\n");
	}
	::fprintf(pOut, "Run %d Poly\n", int(mTriNum));
	for (uint32_t i = 0; i < mPolNum; ++i) {
		int nptris = get_pol_num_tris(i);
		for (int j = 0; j < nptris; ++j) {
			::fprintf(pOut, " 3 <");
			for (int k = 0; k < 3; ++k) {
				::fprintf(pOut, " %d", get_pol_tri_pnt_idx(i, j, k));
			}
			::fprintf(pOut, " [%d]\n", get_pol_grp_id(i));
		}
	}
	::fprintf(pOut, "beginExtra\n");
	::fprintf(pOut, "endExtra\n");
#endif
}

void sxCollisionData::dump_tri_geo(const char* pOutPath) const {
#if XD_FILEFUNCS_ENABLED
	FILE* pOut = nxSys::fopen_w_txt(pOutPath);
	if (!pOut) {
		return;
	}
	dump_tri_geo(pOut);
	::fclose(pOut);
#endif
}


int sxFileCatalogue::find_item_name_idx(const char* pName) const {
	int idx = -1;
	if (pName) {
		sxStrList* pStrLst = get_str_list();
		int nameId = pStrLst->find_str(pName);
		if (nameId >= 0) {
			const FileInfo* pInfo = get_info(0);
			for (uint32_t i = 0; i < mFilesNum; ++i) {
				if (pInfo->mNameId == nameId) {
					idx = i;
					break;
				}
				++pInfo;
			}
		}
	}
	return idx;
}


void cxMotionWork::apply_motion(const sxMotionData* pMotData, const float frameAdd, float* pLoopFlg) {
	mpCurrentMotData = pMotData;
	mEvalFrame = mFrame;
	if (!pMotData) return;
	if (!mpMdlData) return;
	for (uint32_t i = 0; i < pMotData->mNodeNum; ++i) {
		const char* pMotNodeName = pMotData->get_node_name(i);
		int iskel = mpMdlData->find_skel_node_id(pMotNodeName);
		if (mpMdlData->ck_skel_id(iskel)) {
			const sxMotionData::Node* pMotNode = pMotData->get_node(i);
			xt_xmtx xform = mpXformsL[iskel];
			cxVec pos = nxMtx::xmtx_get_pos(xform);
			if (pMotNode->mTrkOffsT) {
				pos = pMotData->eval_pos(i, mEvalFrame);
				if (iskel == mCenterId) {
					pos.y += mHeightOffs;
				}
				nxMtx::xmtx_set_pos(xform, pos);
			}
			cxQuat quat;
			if (pMotNode->mTrkOffsQ) {
				quat = pMotData->eval_quat(i, mEvalFrame);
				xform = nxMtx::xmtx_from_quat_pos(quat, pos);
			}
			mpXformsL[iskel] = xform;

			if (iskel == mMoveId) {
				if (pMotNode->mTrkOffsT) {
					cxVec vmove = pos;
					if (mEvalFrame > 0.0f) {
						float prevFrame = nxCalc::max(mEvalFrame - frameAdd, 0.0f);
						cxVec prevPos = pMotData->eval_pos(i, prevFrame);
						vmove.sub(prevPos);
					} else {
						vmove.scl(frameAdd);
					}
					mMoveRelPos = vmove * mUniformScale;
				} else {
					mMoveRelPos.zero();
				}
				if (pMotNode->mTrkOffsQ) {
					cxQuat qmove = quat;
					if (mEvalFrame > 0.0f) {
						float prevFrame = nxCalc::max(mEvalFrame - frameAdd, 0.0f);
						cxQuat prevQuat = pMotData->eval_quat(i, prevFrame);
						qmove = prevQuat.get_inverted() * quat;
					} else {
						qmove = pMotData->eval_quat(i, frameAdd);
					}
					mMoveRelQuat = qmove;
				} else {
					mMoveRelQuat.identity();
				}
				if (mpMdlData->ck_skel_id(mRootId)) {
					xt_xmtx moveXform = nxMtx::xmtx_from_quat_pos(mMoveRelQuat, mMoveRelPos);
					mpXformsL[mRootId] = nxMtx::xmtx_concat(moveXform, mpXformsL[mRootId]);
					mpXformsL[mMoveId].identity();
				}
			}
		}
	}
	float maxFrame = float(pMotData->mFrameNum - 1);
	mFrame += frameAdd;
	bool loop = mPlayLastFrame ? mFrame > maxFrame : mFrame >= maxFrame;
	if (loop) {
		mFrame = ::mth_fmodf(mFrame, maxFrame);
	} else if (mFrame < 0.0f) {
		mFrame = maxFrame - ::mth_fmodf(-mFrame, maxFrame);
	}
	if (pLoopFlg) {
		*pLoopFlg = loop;
	}
}

void cxMotionWork::copy_local(const cxMotionWork* pSrcWk, const sxMotionData* pSrcMotData) {
	if (!pSrcWk) return;
	if (!pSrcMotData) return;
	if (!mpMdlData) return;
	if (mpMdlData != pSrcWk->mpMdlData) return;
	for (uint32_t i = 0; i < pSrcMotData->mNodeNum; ++i) {
		const char* pMotNodeName = pSrcMotData->get_node_name(i);
		int iskel = mpMdlData->find_skel_node_id(pMotNodeName);
		if (mpMdlData->ck_skel_id(iskel)) {
			xt_xmtx xform = pSrcWk->mpXformsL[iskel];
			mpXformsL[iskel] = xform;
		}
	}
}

xt_xmtx cxMotionWork::eval_skel_node_chain_xform(const sxMotionData* pMotData, const int inode, const int itop, const float frame) {
	xt_xmtx xm;
	xm.identity();
	if (pMotData && mpMdlData && mpMdlData->ck_skel_id(inode)) {
		xt_xmtx xform;
		float evalFrm = nxCalc::clamp(frame, 0.0f, float(pMotData->mFrameNum - 1));
		if (inode != itop) {
			const int32_t* pParents = mpMdlData->get_skel_parents_ptr();
			int idx = pParents[inode];
			while (mpMdlData->ck_skel_id(idx)) {
				const char* pName = mpMdlData->get_skel_name(idx);
				int imot = pMotData->find_node_id(pName);
				xform = mpXformsL[idx];
				if (pMotData->ck_node_id(imot)) {
					const sxMotionData::Node* pMotNode = pMotData->get_node(imot);
					cxVec pos = nxMtx::xmtx_get_pos(xform);
					if (pMotNode->mTrkOffsT) {
						pos = pMotData->eval_pos(imot, evalFrm);
						nxMtx::xmtx_set_pos(xform, pos);
					}
					if (pMotNode->mTrkOffsQ) {
						cxQuat quat = pMotData->eval_quat(imot, evalFrm);
						xform = nxMtx::xmtx_from_quat_pos(quat, pos);
					}
				}
				xm = nxMtx::xmtx_concat(xm, xform);
				if (idx == itop) {
					break;
				}
				idx = pParents[idx];
			}
		}
		const char* pName = mpMdlData->get_skel_name(inode);
		int imot = pMotData->find_node_id(pName);
		xform = mpXformsL[inode];
		if (pMotData->ck_node_id(imot)) {
			const sxMotionData::Node* pMotNode = pMotData->get_node(imot);
			cxVec pos = nxMtx::xmtx_get_pos(xform);
			if (pMotNode->mTrkOffsT) {
				pos = pMotData->eval_pos(imot, evalFrm);
				nxMtx::xmtx_set_pos(xform, pos);
			}
			if (pMotNode->mTrkOffsQ) {
				cxQuat quat = pMotData->eval_quat(imot, evalFrm);
				xform = nxMtx::xmtx_from_quat_pos(quat, pos);
			}
		}
		xm = nxMtx::xmtx_concat(xform, xm);
	}
	return xm;
}

void cxMotionWork::adjust_leg(const cxVec& effPos, const int inodeTop, const int inodeRot, const int inodeEnd, const int inodeExt) {
	sxModelData* pMdl = mpMdlData;
	if (!pMdl) return;
	if (!ck_node_id(inodeTop)) return;
	if (!ck_node_id(inodeRot)) return;
	if (!ck_node_id(inodeEnd)) return;
	bool isExt = ck_node_id(inodeExt);
	sxLimbIKWork ik;
	ik.set_axis_leg();
	ik.mpRig = nullptr;
	ik.mpChain = nullptr;
	ik.mTopW = calc_node_world_mtx(inodeTop, &ik.mParentW);
	ik.mRotW = calc_node_world_mtx(inodeRot);
	ik.mEndW = calc_node_world_mtx(inodeEnd);
	if (isExt) {
		ik.mExtW = calc_node_world_mtx(inodeExt);
		ik.mExtW.set_translation(effPos);
		cxQuat endRot = nxQuat::from_mtx(ik.mEndW);
		cxVec endOffs = pMdl->get_skel_local_offs(inodeExt);
		ik.mEndW.set_translation(effPos - endRot.apply(endOffs));
	} else {
		ik.mExtW.identity();
		ik.mEndW.set_translation(effPos);
	}
	ik.mRotOffs = pMdl->get_skel_local_offs(inodeRot);
	ik.mDistTopRot = ik.mRotOffs.mag();
	ik.mDistRotEnd = pMdl->get_skel_local_offs(inodeEnd).mag();
	ik.mDistTopEnd = nxVec::dist(ik.mTopW.get_translation(), ik.mEndW.get_translation());
	ik.calc_world();
	ik.calc_local();
	mpXformsL[inodeTop] = nxMtx::xmtx_from_mtx(ik.mTopL);
	mpXformsL[inodeRot] = nxMtx::xmtx_from_mtx(ik.mRotL);
	mpXformsL[inodeEnd] = nxMtx::xmtx_from_mtx(ik.mEndL);
	if (isExt) {
		ik.mExtL = ik.mExtW * ik.mEndW.get_inverted();
		mpXformsL[inodeExt] = nxMtx::xmtx_from_mtx(ik.mExtL);
	}
}

void cxMotionWork::copy_prev_world() {
	if (!mpMdlData) return;
	nxCore::mem_copy(mpPrevXformsW, mpXformsW, mpMdlData->mSklNum * sizeof(xt_xmtx));
}

void cxMotionWork::calc_world() {
	if (!mpMdlData) return;
	int nskel = mpMdlData->mSklNum;
	const int32_t* pParents = mpMdlData->get_skel_parents_ptr();
	for (int i = 0; i < nskel; ++i) {
		int iparent = pParents[i];
		if (iparent >= 0 && iparent < nskel) {
			mpXformsW[i] = nxMtx::xmtx_concat(mpXformsL[i], mpXformsW[iparent]);
		} else {
			mpXformsW[i] = mpXformsL[i];
			if (mUniformScale != 1.0f) {
				cxMtx sm;
				sm.mk_scl(mUniformScale);
				mpXformsW[i] = nxMtx::xmtx_from_mtx(sm * nxMtx::mtx_from_xmtx(mpXformsW[i]));
			}
		}
	}
}

void cxMotionWork::calc_root_world() {
	if (mRootId >= 0) {
		mpXformsW[mRootId] = mpXformsL[mRootId];
		if (mUniformScale != 1.0f) {
			cxMtx sm;
			sm.mk_scl(mUniformScale);
			mpXformsW[mRootId] = nxMtx::xmtx_from_mtx(sm * nxMtx::mtx_from_xmtx(mpXformsW[mRootId]));
		}
	}
}

xt_xmtx cxMotionWork::get_node_local_xform(const int inode) const {
	xt_xmtx lm;
	if (ck_node_id(inode) && mpXformsL) {
		lm = mpXformsL[inode];
	} else {
		lm.identity();
	}
	return lm;
}

xt_xmtx cxMotionWork::get_node_prev_world_xform(const int inode) const {
	xt_xmtx owm;
	if (ck_node_id(inode) && mpPrevXformsW) {
		owm = mpPrevXformsW[inode];
	} else {
		owm.identity();
	}
	return owm;
}

xt_xmtx cxMotionWork::calc_node_world_xform(const int inode, xt_xmtx* pParentXform) const {
	xt_xmtx wm;
	if (mpMdlData) {
		wm = mpMdlData->calc_skel_node_world_xform(inode, mpXformsL, pParentXform);
	} else {
		wm.identity();
	}
	return wm;
}

cxMtx cxMotionWork::calc_node_world_mtx(const int inode, cxMtx* pParentMtx) const {
	xt_xmtx parentXform;
	xt_xmtx worldXform = calc_node_world_xform(inode, pParentMtx ? &parentXform : nullptr);
	if (pParentMtx) {
		*pParentMtx = nxMtx::mtx_from_xmtx(parentXform);
	}
	return nxMtx::mtx_from_xmtx(worldXform);
}

cxMtx cxMotionWork::calc_motion_world_mtx(const int inode) {
	cxMtx wm;
	if (mUniformScale != 1.0f) {
		if (ck_node_id(inode)) {
			update_world_to_node(inode);
			wm = nxMtx::mtx_from_xmtx(mpXformsW[inode]);
		} else {
			wm.identity();
		}
	} else {
		wm = calc_node_world_mtx(inode);
	}
	return wm;
}

void cxMotionWork::update_world_to_node(const int inode) {
	if (!mpMdlData) return;
	int nskel = mpMdlData->mSklNum;
	if (inode < 0 || inode >= nskel) return;
	const int32_t* pParents = mpMdlData->get_skel_parents_ptr();
	for (int i = 0; i < inode; ++i) {
		int iparent = pParents[i];
		if (iparent >= 0 && iparent < nskel) {
			mpXformsW[i] = nxMtx::xmtx_concat(mpXformsL[i], mpXformsW[iparent]);
		} else {
			mpXformsW[i] = mpXformsL[i];
			if (mUniformScale != 1.0f) {
				cxMtx sm;
				sm.mk_scl(mUniformScale);
				mpXformsW[i] = nxMtx::xmtx_from_mtx(sm * nxMtx::mtx_from_xmtx(mpXformsW[i]));
			}
		}
	}
}

void cxMotionWork::reset_node_local_xform(const int inode) {
	if (ck_node_id(inode) && mpXformsL) {
		mpXformsL[inode] = mpMdlData->get_skel_local_xform(inode);
	}
}

void cxMotionWork::set_node_local_xform(const int inode, const xt_xmtx& lm) {
	if (ck_node_id(inode) && mpXformsL) {
		mpXformsL[inode] = lm;
	}
}

void cxMotionWork::set_node_local_tx(const int inode, const float x) {
	if (ck_node_id(inode) && mpXformsL) {
		mpXformsL[inode].m[0][3] = x;
	}
}

void cxMotionWork::set_node_local_ty(const int inode, const float y) {
	if (ck_node_id(inode) && mpXformsL) {
		mpXformsL[inode].m[1][3] = y;
	}
}

void cxMotionWork::set_node_local_tz(const int inode, const float z) {
	if (ck_node_id(inode) && mpXformsL) {
		mpXformsL[inode].m[2][3] = z;
	}
}

void cxMotionWork::disable_node_blending(const int inode, const float disable) {
	if (mpBlendDisableBits && ck_node_id(inode)) {
		if (disable) {
			XD_BIT_ARY_ST(uint8_t, mpBlendDisableBits, inode);
		} else {
			XD_BIT_ARY_CL(uint8_t, mpBlendDisableBits, inode);
		}
	}
}

void cxMotionWork::blend_init(const int duration) {
	if (mpMdlData && mpXformsL && mpBlendXformsL) {
		int nskel = mpMdlData->mSklNum;
		nxCore::mem_copy(mpBlendXformsL, mpXformsL, nskel * sizeof(xt_xmtx));
	}
	mBlendDuration = float(duration);
	mBlendCount = mBlendDuration;
}

void cxMotionWork::blend_exec() {
	if (mBlendCount <= 0.0f) return;
	float t = nxCalc::div0(mBlendDuration - mBlendCount, mBlendDuration);
	if (mpMdlData && mpXformsL && mpBlendXformsL) {
		int nskel = mpMdlData->mSklNum;
		for (int i = 0; i < nskel; ++i) {
			bool blendFlg = !XD_BIT_ARY_CK(uint32_t, mpBlendDisableBits, i);
			if (blendFlg) {
				cxMtx msrc = nxMtx::mtx_from_xmtx(mpBlendXformsL[i]);
				cxVec tsrc = msrc.get_translation();
				cxQuat qsrc;
				qsrc.from_mtx(msrc);

				cxMtx mdst = nxMtx::mtx_from_xmtx(mpXformsL[i]);
				cxVec tdst = mdst.get_translation();
				cxQuat qdst;
				qdst.from_mtx(mdst);

				cxQuat qblend = nxQuat::slerp(qsrc, qdst, t);
				cxVec tblend = nxVec::lerp(tsrc, tdst, t);
				mpXformsL[i] = nxMtx::xmtx_from_quat_pos(qblend, tblend);
			}
		}
	}
	--mBlendCount;
	mBlendCount = nxCalc::max(0.0f, mBlendCount);
}

void cxMotionWork::set_base_node_ids(const char* pRootName, const char* pMoveName, const char* pCenterName) {
	sxModelData* pMdd = mpMdlData;
	mRootId = pMdd ? pMdd->find_skel_node_id(pRootName) : -1;
	mMoveId = pMdd ? pMdd->find_skel_node_id(pMoveName) : -1;
	mCenterId = pMdd ? pMdd->find_skel_node_id(pCenterName) : -1;
}

cxMotionWork* cxMotionWork::create(sxModelData* pMdlData) {
	cxMotionWork* pWk = nullptr;
	if (pMdlData && pMdlData->has_skel()) {
		int nskel = pMdlData->mSklNum;
		size_t size = XD_ALIGN(sizeof(cxMotionWork), 0x10);
		size_t xformOffsL = size;
		size += nskel * sizeof(xt_xmtx);
		size_t xformOffsW = size;
		size += nskel * sizeof(xt_xmtx);
		size_t xformOffsPrevW = size;
		size += nskel * sizeof(xt_xmtx);
		size_t xformOffsBlendL = size;
		size += nskel * sizeof(xt_xmtx);
		size_t blendBitsOffs = size;
		size += XD_BIT_ARY_SIZE(uint8_t, nskel);
		pWk = (cxMotionWork*)nxCore::mem_alloc(size, "xMotWk");
		if (pWk) {
			nxCore::mem_zero((void*)pWk, size);
			pWk->mpMdlData = pMdlData;
			pWk->set_base_node_ids();
			pWk->mpXformsL = (xt_xmtx*)XD_INCR_PTR(pWk, xformOffsL);
			pWk->mpXformsW = (xt_xmtx*)XD_INCR_PTR(pWk, xformOffsW);
			pWk->mpPrevXformsW = (xt_xmtx*)XD_INCR_PTR(pWk, xformOffsPrevW);
			pWk->mpBlendXformsL = (xt_xmtx*)XD_INCR_PTR(pWk, xformOffsBlendL);
			pWk->mpBlendDisableBits = (uint8_t*)XD_INCR_PTR(pWk, blendBitsOffs);
			for (int i = 0; i < nskel; ++i) {
				pWk->mpXformsL[i] = pMdlData->get_skel_local_xform(i);
			}
			for (int i = 0; i < nskel; ++i) {
				pWk->mpBlendXformsL[i] = pWk->mpXformsL[i];
			}
			pWk->mUniformScale = 1.0f;
			pWk->mMoveRelPos.zero();
			pWk->mMoveRelQuat.identity();
			pWk->calc_world();
			pWk->copy_prev_world();
			pWk->mFrame = 0.0f;
			pWk->mPlayLastFrame = true;
		}
	}
	return pWk;
}

void cxMotionWork::destroy(cxMotionWork* pWk) {
	if (pWk) {
		nxCore::mem_free(pWk);
	}
}


void cxModelWork::copy_prev_world_xform() {
	if (mpWorldXform) {
		mpWorldXform[1] = mpWorldXform[0];
	}
}

cxMtx cxModelWork::get_prev_world_xform() const {
	cxMtx wprev;
	if (mpWorldXform) {
		wprev = nxMtx::mtx_from_xmtx(mpWorldXform[1]);
	} else {
		wprev.identity();
	}
	return wprev;
}

void cxModelWork::set_pose(const cxMotionWork* pMot) {
	if (!pMot) return;
	if (!pMot->mpXformsW) return;
	if (!mpData) return;
	if (!mpSkinXforms) return;
	const int32_t* pSkinToSkelMap = mpData->get_skin_to_skel_map();
	if (pSkinToSkelMap) {
		int nskin = mpData->mSknNum;
		for (int i = 0; i < nskin; ++i) {
			int iskel = pSkinToSkelMap[i];
			mpSkinXforms[i] = nxMtx::xmtx_concat(mpData->get_skel_inv_world_xform(iskel), pMot->mpXformsW[iskel]);
		}
	}
}

void cxModelWork::hide_mtl(const int imtl, const bool hide) {
	if (!mpData) return;
	if (mpData->ck_mtl_id(imtl) && mpHideBits) {
		if (hide) {
			XD_BIT_ARY_ST(uint32_t, mpHideBits, imtl);
		} else {
			XD_BIT_ARY_CL(uint32_t, mpHideBits, imtl);
		}
	}
}

bool cxModelWork::is_mtl_hidden(const int imtl) const {
	bool hidden = false;
	if (mpData && mpData->ck_mtl_id(imtl) && mpHideBits) {
		hidden = XD_BIT_ARY_CK(uint32_t, mpHideBits, imtl);
	}
	return hidden;
}

bool cxModelWork::is_bat_mtl_hidden(const int ibat) const {
	bool hidden = false;
	if (mpData) {
		const sxModelData::Batch* pBat = mpData->get_batch_ptr(ibat);
		if (pBat) {
			hidden = is_mtl_hidden(pBat->mMtlId);
		}
	}
	return hidden;
}

void cxModelWork::update_bounds() {
	if (!mpData) return;
	if (mpData->has_skin()) {
		if (mpSkinXforms) {
			const cxSphere* pSph = mpData->get_mdl_spheres();
			int nskin = mpData->mSknNum;
			for (int i = 0; i < nskin; ++i) {
				cxSphere sph = pSph[i];
				cxVec spos = nxMtx::xmtx_calc_pnt(mpSkinXforms[i], sph.get_center());
				cxVec rvec(sph.get_radius());
				cxAABB sbb(spos - rvec, spos + rvec);
				if (i == 0) {
					mWorldBBox = sbb;
				} else {
					mWorldBBox.merge(sbb);
				}
			}
			if (mpBatBBoxes) {
				int nbat = mpData->mBatNum;
				for (int i = 0; i < nbat; ++i) {
					const int32_t* pLst = mpData->get_batch_jnt_list(i);
					int njnt = mpData->get_batch_jnt_num(i);
					pSph = mpData->get_batch_spheres(i);
					for (int j = 0; j < njnt; ++j) {
						cxSphere sph = pSph[j];
						cxVec spos = nxMtx::xmtx_calc_pnt(mpSkinXforms[pLst[j]], sph.get_center());
						cxVec rvec(sph.get_radius());
						cxAABB sbb(spos - rvec, spos + rvec);
						if (j == 0) {
							mpBatBBoxes[i] = sbb;
						} else {
							mpBatBBoxes[i].merge(sbb);
						}
					}
				}
			}
			mBoundsValid = true;
		}
	} else if (mpWorldXform) {
		if (mpData) {
			mWorldBBox = mpData->mBBox;
		} else {
			mWorldBBox.set(cxVec(0.0f));
		}
		cxMtx wm = nxMtx::mtx_from_xmtx(*mpWorldXform);
		mWorldBBox.transform(wm);
		if (mpBatBBoxes) {
			int nbat = mpData->mBatNum;
			for (int i = 0; i < nbat; ++i) {
				mpBatBBoxes[i].transform(mpData->get_batch_bbox(i), wm);
			}
		}
		mBoundsValid = true;
	} else if (mpData->is_static()) {
		mBoundsValid = true;
	}
}

void cxModelWork::frustum_cull(const cxFrustum* pFst, const bool precise) {
	if (!mpCullBits) return;
	int nbat = mpData->mBatNum;
	nxCore::mem_zero(mpCullBits, XD_BIT_ARY_SIZE(uint8_t, nbat));
	if (!pFst) return;
	if (!mBoundsValid) return;
	for (int i = 0; i < nbat; ++i) {
		cxAABB batBB = mpBatBBoxes[i];
		bool cull = pFst->cull(batBB);
		if (precise) {
			if (!cull) {
				bool vis = pFst->overlaps(batBB);
				cull = !vis;
			}
		}
		if (cull) {
			XD_BIT_ARY_ST(uint32_t, mpCullBits, i);
		}
	}
}

bool cxModelWork::calc_batch_visibility(const cxFrustum* pFst, const int ibat, const bool precise) {
	bool visible = false;
	if (pFst && ck_batch_id(ibat)) {
		cxAABB batBB = mpBatBBoxes[ibat];
		visible = !pFst->cull(batBB);
		if (visible && precise) {
			visible = pFst->overlaps(batBB);
		}
		if (mpCullBits) {
			if (visible) {
				XD_BIT_ARY_CL(uint32_t, mpCullBits, ibat);
			} else {
				XD_BIT_ARY_ST(uint32_t, mpCullBits, ibat);
			}
		}
	}
	return visible;
}

sxTextureData* cxModelWork::find_texture(cxResourceManager* pRsrcMgr, const char* pTexName) const {
	sxTextureData* pTex = nullptr;
	if (pTexName && mpData) {
		if (pRsrcMgr) {
			pTex = pRsrcMgr->find_texture_for_model(mpData, pTexName);
		}
		if (!pTex) {
			if (mpTexPkg) {
				pTex = mpTexPkg->find_texture(pTexName);
			}
		}
	}
	return pTex;
}

cxModelWork* cxModelWork::create(sxModelData* pMdl, const size_t paramMemSize, const size_t extMemSize) {
	if (!pMdl) return nullptr;
	cxModelWork* pWk = nullptr;
	size_t size = XD_ALIGN(sizeof(cxModelWork), 0x10);
	size_t offsWM = 0;
	size_t offsJM = 0;
	int nskin = pMdl->mSknNum;
	if (nskin > 0) {
		offsJM = size;
		size += nskin * sizeof(xt_xmtx);
	} else {
		if (!pMdl->is_static()) {
			offsWM = size;
			size += sizeof(xt_xmtx) * 2; /* { current, previous } */
		}
	}
	size_t offsBatBBs = pMdl->is_static() ? 0 : size;
	int nbat = pMdl->mBatNum;
	size += nbat * sizeof(cxAABB);
	size_t offsCull = size;
	size += XD_ALIGN(XD_BIT_ARY_SIZE(uint8_t, nbat), 0x10);
	int nmtl = pMdl->mMtlNum;
	size_t offsHide = size;
	size += XD_ALIGN(XD_BIT_ARY_SIZE(uint8_t, nmtl), 0x10);
	size_t offsParam = 0;
	if (paramMemSize) {
		offsParam = size;
		size += paramMemSize;
	}
	size_t offsExt = 0;
	if (extMemSize) {
		offsExt = size;
		size += extMemSize;
	}
	pWk = (cxModelWork*)nxCore::mem_alloc(size, "xMdlWk");
	if (pWk) {
		nxCore::mem_zero((void*)pWk, size);
		pWk->mpData = pMdl;
		pWk->mpWorldXform = offsWM ? (xt_xmtx*)XD_INCR_PTR(pWk, offsWM) : nullptr;
		if (pWk->mpWorldXform) {
			pWk->mpWorldXform->identity();
			pWk->copy_prev_world_xform();
		}
		pWk->mpSkinXforms = offsJM ? (xt_xmtx*)XD_INCR_PTR(pWk, offsJM) : nullptr;
		if (pWk->mpSkinXforms) {
			for (int i = 0; i < nskin; ++i) {
				pWk->mpSkinXforms[i].identity();
			}
		}
		pWk->mWorldBBox = pMdl->mBBox;
		pWk->copy_prev_world_bbox();
		if (offsBatBBs) {
			pWk->mpBatBBoxes = (cxAABB*)XD_INCR_PTR(pWk, offsBatBBs);
			for (int i = 0; i < nbat; ++i) {
				pWk->mpBatBBoxes[i] = pMdl->get_batch_bbox(i);
			}
			pWk->mBoundsValid = false;
		} else {
			pWk->mpBatBBoxes = pMdl->mBatOffs ? reinterpret_cast<cxAABB*>(XD_INCR_PTR(pMdl, pMdl->mBatOffs)) : nullptr;
			pWk->mBoundsValid = true;
		}
		pWk->mpCullBits = offsCull ? (uint32_t*)XD_INCR_PTR(pWk, offsCull) : nullptr;
		pWk->mpHideBits = offsHide ? (uint32_t*)XD_INCR_PTR(pWk, offsHide) : nullptr;
		pWk->mpParamMem = offsParam ? XD_INCR_PTR(pWk, offsParam) : nullptr;
		pWk->mpExtMem = offsExt ? XD_INCR_PTR(pWk, offsExt) : nullptr;
		if (nskin > 0) {
			pWk->mRenderMask = 1.0f;
		} else {
			pWk->mRenderMask = 0.0f;
		}
	}
	return pWk;
}

void cxModelWork::destroy(cxModelWork* pWk) {
	if (pWk) {
		nxCore::mem_free(pWk);
	}
}


#define XD_STRSTORE_ALLOC_SIZE size_t(4096)

cxStrStore* cxStrStore::create(const char* pTag, sxLock* pMemLock) {
	const size_t defSize = XD_STRSTORE_ALLOC_SIZE;
	size_t size = sizeof(cxStrStore) + defSize;
	if (pMemLock) { nxSys::lock_acquire(pMemLock); }
	cxStrStore* pStore = reinterpret_cast<cxStrStore*>(nxCore::mem_alloc(size, pTag ? pTag : XD_STRSTORE_TAG));
	if (pMemLock) { nxSys::lock_release(pMemLock); }
	if (pStore) {
		pStore->mSize = defSize;
		pStore->mpNext = nullptr;
		pStore->mPtr = 0;
		pStore->mpMemLock = pMemLock;
	}
	return pStore;
}

XD_NOINLINE void cxStrStore::destroy(cxStrStore* pStore, const bool useMemLock) {
	cxStrStore* p = pStore;
	while (p) {
		sxLock* pMemLock = useMemLock ? p->mpMemLock : nullptr;
		cxStrStore* pNext = p->mpNext;
		if (pMemLock) { nxSys::lock_acquire(pMemLock); }
		nxCore::mem_free(p);
		if (pMemLock) { nxSys::lock_release(pMemLock); }
		p = pNext;
	}
}

char* cxStrStore::add(const char* pStr) {
	char* pRes = nullptr;
	if (pStr) {
		cxStrStore* pStore = this;
		size_t len = nxCore::str_len(pStr) + 1;
		while (!pRes) {
			size_t free = pStore->mSize - pStore->mPtr;
			if (free >= len) {
				pRes = reinterpret_cast<char*>(pStore + 1) + pStore->mPtr;
				nxCore::mem_copy(pRes, pStr, len);
				pStore->mPtr += len;
			} else {
				cxStrStore* pNext = pStore->mpNext;
				if (!pNext) {
					size_t size = sizeof(cxStrStore) + nxCalc::max(XD_STRSTORE_ALLOC_SIZE, len);
					sxLock* pMemLock = pStore->mpMemLock;
					if (pMemLock) { nxSys::lock_acquire(pMemLock); }
					pNext = reinterpret_cast<cxStrStore*>(nxCore::mem_alloc(size, XD_STRSTORE_TAG));
					if (pMemLock) { nxSys::lock_release(pMemLock); }
					if (pNext) {
						pNext->mSize = size;
						pNext->mpNext = nullptr;
						pNext->mPtr = 0;
						pNext->mpMemLock = pStore->mpMemLock;
					} else {
						break;
					}
				}
				pStore = pNext;
			}
		}
	}
	return pRes;
}

void cxStrStore::purge() {
	cxStrStore* p = this;
	while (p) {
		p->mPtr = 0;
		p = p->mpNext;
	}
}

void cxCmdLine::ctor(int argc, char* argv[]) {
	mpStore = nullptr;
	mpArgLst = nullptr;
	mpOptMap = nullptr;
	if (argc < 1 || !argv) return;
	mpStore = cxStrStore::create("xCmdLineStrs");
	if (!mpStore) return;
	mpProgPath = mpStore->add(argv[0]);
	if (argc < 2) return;
	mpOptMap = MapT::create("xCmdLineOpts");
	mpArgLst = ListT::create("xCmdLineArgs");
	for (int i = 1; i < argc; ++i) {
		char* pArg = argv[i];
		if (nxCore::str_starts_with(pArg, "-")) {
			int argLen = int(nxCore::str_len(pArg));
			char* pVal = nullptr;
			for (int j = 1; j < argLen; ++j) {
				if (pArg[j] == ':') {
					pVal = &pArg[j + 1];
					break;
				}
			}
			if (pVal && pVal != pArg + argLen) {
				char* pOptStr = mpStore->add(pArg + 1);
				char* pValStr = &pOptStr[pVal - (pArg + 1)];
				pValStr[-1] = 0;
				if (mpOptMap) {
					mpOptMap->put(pOptStr, pValStr);
				}
			}
		} else {
			char* pArgStr = mpStore->add(pArg);
			if (pArgStr && mpArgLst) {
				char** pp = mpArgLst->new_item();
				if (pp) {
					*pp = pArgStr;
				}
			}
		}
	}
}

XD_NOINLINE void cxCmdLine::dtor() {
	ListT::destroy(mpArgLst);
	mpArgLst = nullptr;
	MapT::destroy(mpOptMap);
	mpOptMap = nullptr;
	cxStrStore::destroy(mpStore);
	mpStore = nullptr;
}

XD_NOINLINE const char* cxCmdLine::get_arg(const int i) const {
	const char* pArg = nullptr;
	if (mpArgLst) {
		char** ppArg = mpArgLst->get_item(i);
		if (ppArg) {
			pArg = *ppArg;
		}
	}
	return pArg;
}

XD_NOINLINE const char* cxCmdLine::get_opt(const char* pName) const {
	char* pVal = nullptr;
	if (mpOptMap) {
		mpOptMap->get(pName, &pVal);
	}
	return pVal;
}

XD_NOINLINE int cxCmdLine::get_int_opt(const char* pName, const int defVal) const {
	const char* pValStr = get_opt(pName);
	int res = defVal;
	if (pValStr) {
		if (pValStr[0] && pValStr[1] && (pValStr[0] == '0' && pValStr[1] == 'x')) {
			res = int(nxCore::parse_u64_hex(pValStr + 2));
		} else {
			res = int(nxCore::parse_i64(pValStr));
		}
	}
	return res;
}

XD_NOINLINE float cxCmdLine::get_float_opt(const char* pName, const float defVal) const {
	const char* pValStr = get_opt(pName);
	float res = defVal;
	if (pValStr) {
		res = float(nxCore::parse_f64(pValStr));
	}
	return res;
}

XD_NOINLINE bool cxCmdLine::get_bool_opt(const char* pName, const bool defVal) const {
	return !!get_int_opt(pName, defVal ? 1 : 0);
}

/*static*/ cxCmdLine* cxCmdLine::create(int argc, char* argv[]) {
	cxCmdLine* pCmdLine = (cxCmdLine*)nxCore::mem_alloc(sizeof(cxCmdLine), "xCmdLine");
	if (pCmdLine) {
		pCmdLine->ctor(argc, argv);
	}
	return pCmdLine;
}

/*static*/ void cxCmdLine::destroy(cxCmdLine* pCmdLine) {
	if (pCmdLine) {
		pCmdLine->dtor();
		nxCore::mem_free(pCmdLine);
	}
}


void cxStopWatch::alloc(int nsmps) {
	mpSmps = (double*)nxCore::mem_alloc(nsmps * sizeof(double), "xStopWatch");
	if (mpSmps) {
		mSmpsNum = nsmps;
	}
}

void cxStopWatch::free() {
	if (mpSmps) {
		nxCore::mem_free(mpSmps);
		mpSmps = nullptr;
	}
	mSmpsNum = 0;
	mSmpIdx = 0;
}

void cxStopWatch::begin() {
	mT = nxSys::time_micros();
}

bool cxStopWatch::end() {
	if (mSmpIdx < mSmpsNum) {
		mpSmps[mSmpIdx] = nxSys::time_micros() - mT;
		++mSmpIdx;
	}
	return (mSmpIdx >= mSmpsNum);
}

void cxStopWatch::reset() {
	mSmpIdx = 0;
}

double cxStopWatch::median() {
	double val = 0;
	int n = mSmpIdx;
	if (n > 0 && mpSmps) {
		val = nxCalc::median(mpSmps, n);
	}
	return val;
}

double cxStopWatch::harmonic_mean() {
	double val = 0;
	int n = mSmpIdx;
	if (n > 0 && mpSmps) {
		val = nxCalc::harmonic_mean(mpSmps, n);
	}
	return val;
}


static void make_data_addr_key(char addrStr[XD_RSRC_ADDR_KEY_SIZE], sxData* mpData) {
	static const char* tbl = "0123456789ABCDEF";
	nxCore::mem_zero(addrStr, XD_RSRC_ADDR_KEY_SIZE);
	uintptr_t p = (uintptr_t)mpData;
	for (size_t i = 0; i < sizeof(p) * 2; ++i) {
		addrStr[i] = tbl[(p >> (i * 4)) & 0xF];
	}
}

void cxResourceManager::Pkg::Entry::set_data(sxData* pData) {
	if (pData) {
		mpData = pData;
		make_data_addr_key(mAddrKey, mpData);
	}
}

sxGeometryData* cxResourceManager::Pkg::find_geometry(const char* pName) const {
	sxGeometryData* pGeo = nullptr;
	if (pName && mpGeoDataMap) {
		mpGeoDataMap->get(pName, &pGeo);
	}
	return pGeo;
}

sxImageData* cxResourceManager::Pkg::find_image(const char* pName) const {
	sxImageData* pImg = nullptr;
	if (pName && mpImgDataMap) {
		mpImgDataMap->get(pName, &pImg);
	}
	return pImg;
}

sxRigData* cxResourceManager::Pkg::find_rig(const char* pName) const {
	sxRigData* pRig = nullptr;
	if (pName && mpRigDataMap) {
		mpRigDataMap->get(pName, &pRig);
	}
	return pRig;
}

sxKeyframesData* cxResourceManager::Pkg::find_keyframes(const char* pName) const {
	sxKeyframesData* pKfr = nullptr;
	if (pName && mpKfrDataMap) {
		mpKfrDataMap->get(pName, &pKfr);
	}
	return pKfr;
}

sxValuesData* cxResourceManager::Pkg::find_values(const char* pName) const {
	sxValuesData* pVal = nullptr;
	if (pName && mpValDataMap) {
		mpValDataMap->get(pName, &pVal);
	}
	return pVal;
}

sxExprLibData* cxResourceManager::Pkg::find_expressions(const char* pName) const {
	sxExprLibData* pExp = nullptr;
	if (pName && mpExpDataMap) {
		mpExpDataMap->get(pName, &pExp);
	}
	return pExp;
}

sxModelData* cxResourceManager::Pkg::find_model(const char* pName) const {
	sxModelData* pMdl = nullptr;
	if (pName && mpMdlDataMap) {
		mpMdlDataMap->get(pName, &pMdl);
	}
	return pMdl;
}

sxTextureData* cxResourceManager::Pkg::find_texture(const char* pName) const {
	sxTextureData* pTex = nullptr;
	if (pName && mpTexDataMap) {
		mpTexDataMap->get(pName, &pTex);
	}
	return pTex;
}

sxMotionData* cxResourceManager::Pkg::find_motion(const char* pName) const {
	sxMotionData* pMot = nullptr;
	if (pName && mpMotDataMap) {
		mpMotDataMap->get(pName, &pMot);
	}
	return pMot;
}

sxCollisionData* cxResourceManager::Pkg::find_collision(const char* pName) const {
	sxCollisionData* pCol = nullptr;
	if (pName && mpColDataMap) {
		mpColDataMap->get(pName, &pCol);
	}
	return pCol;
}

void cxResourceManager::Pkg::prepare_gfx() {
	if (!mpMgr) return;
	if (!mpEntries) return;
	for (EntryList::Itr itr = mpEntries->get_itr(); !itr.end(); itr.next()) {
		Entry* pEnt = itr.item();
		if (pEnt->mpData) {
			if (pEnt->mpData->is<sxModelData>()) {
				if (mpMgr->mGfxIfc.prepareModel) {
					mpMgr->mGfxIfc.prepareModel(pEnt->mpData->as<sxModelData>());
				}
			} else if (pEnt->mpData->is<sxTextureData>()) {
				if (mpMgr->mGfxIfc.prepareTexture) {
					mpMgr->mGfxIfc.prepareTexture(pEnt->mpData->as<sxTextureData>());
				}
			}
		}
	}
}

void cxResourceManager::Pkg::release_gfx() {
	if (!mpMgr) return;
	if (!mpEntries) return;
	for (EntryList::Itr itr = mpEntries->get_itr(); !itr.end(); itr.next()) {
		Entry* pEnt = itr.item();
		if (pEnt->mpData) {
			if (pEnt->mpData->is<sxModelData>()) {
				if (mpMgr->mGfxIfc.releaseModel) {
					mpMgr->mGfxIfc.releaseModel(pEnt->mpData->as<sxModelData>());
				}
			} else if (pEnt->mpData->is<sxTextureData>()) {
				if (mpMgr->mGfxIfc.releaseTexture) {
					mpMgr->mGfxIfc.releaseTexture(pEnt->mpData->as<sxTextureData>());
				}
			}
		}
	}
}

void cxResourceManager::pkg_ctor(Pkg* pPkg) {
}

void cxResourceManager::pkg_dtor(Pkg* pPkg) {
	if (!pPkg) return;
	cxResourceManager* pMgr = pPkg->mpMgr;
	if (pPkg->mpEntries) {
		for (Pkg::EntryList::Itr itr = pPkg->mpEntries->get_itr(); !itr.end(); itr.next()) {
			Pkg::Entry* pEnt = itr.item();
			if (pMgr && pMgr->mpDataToPkgMap) {
				pMgr->mpDataToPkgMap->remove(pEnt->mAddrKey);
			}
			if (pEnt->mpData) {
				nxCore::bin_unload(pEnt->mpData);
				pEnt->mpData = nullptr;
			}
		}
		Pkg::EntryList::destroy(pPkg->mpEntries);
		pPkg->mpEntries = nullptr;
	}
	GeoDataMap::destroy(pPkg->mpGeoDataMap);
	pPkg->mpGeoDataMap = nullptr;
	ImgDataMap::destroy(pPkg->mpImgDataMap);
	pPkg->mpImgDataMap = nullptr;
	RigDataMap::destroy(pPkg->mpRigDataMap);
	pPkg->mpRigDataMap = nullptr;
	KfrDataMap::destroy(pPkg->mpKfrDataMap);
	pPkg->mpKfrDataMap = nullptr;
	ValDataMap::destroy(pPkg->mpValDataMap);
	pPkg->mpValDataMap = nullptr;
	ExpDataMap::destroy(pPkg->mpExpDataMap);
	pPkg->mpExpDataMap = nullptr;
	MdlDataMap::destroy(pPkg->mpMdlDataMap);
	pPkg->mpMdlDataMap = nullptr;
	TexDataMap::destroy(pPkg->mpTexDataMap);
	pPkg->mpTexDataMap = nullptr;
	MotDataMap::destroy(pPkg->mpMotDataMap);
	pPkg->mpMotDataMap = nullptr;
	ColDataMap::destroy(pPkg->mpColDataMap);
	pPkg->mpColDataMap = nullptr;
	pPkg->mpDefMdl = nullptr;
	pPkg->mpDefGeo = nullptr;
	pPkg->mpDefRig = nullptr;
	pPkg->mpDefVal = nullptr;
	pPkg->mpDefExp = nullptr;
	pPkg->mGeoNum = 0;
	pPkg->mImgNum = 0;
	pPkg->mRigNum = 0;
	pPkg->mKfrNum = 0;
	pPkg->mValNum = 0;
	pPkg->mExpNum = 0;
	pPkg->mMdlNum = 0;
	pPkg->mTexNum = 0;
	pPkg->mMotNum = 0;
	pPkg->mColNum = 0;
	nxCore::bin_unload(pPkg->mpCat);
	nxCore::mem_free(pPkg->mpName);
	pPkg->mpName = nullptr;
	pPkg->mpCat = nullptr;
}

cxResourceManager::Pkg* cxResourceManager::load_pkg(const char* pName) {
	if (!pName) return nullptr;
	if (!mpPkgList) return nullptr;
	if (!mpPkgMap) return nullptr;
	if (!mpDataToPkgMap) return nullptr;
	char path[1024];
	char* pPath = path;
	size_t pathBufSize = sizeof(path);
	size_t lenDataPath = nxCore::str_len(mpDataPath);
	size_t lenName = nxCore::str_len(pName);
	size_t pathSize = lenDataPath + 1 + (lenName + 1)*2 + 4 + 1;
	if (pathSize > pathBufSize) {
		pPath = (char*)nxCore::mem_alloc(pathSize, "RsrcMgr:path");
		pathBufSize = pathSize;
	}
	nxCore::mem_copy(pPath, mpDataPath, lenDataPath);
	pPath[lenDataPath] = '/';
	nxCore::mem_copy(pPath + lenDataPath + 1, pName, lenName);
	pPath[lenDataPath + 1 + lenName] = '/';
	nxCore::mem_copy(pPath + lenDataPath + 1 + lenName + 1, pName, lenName);
	size_t extIdx = lenDataPath + 1 + lenName + 1 + lenName;
	pPath[extIdx] = '.';
	pPath[extIdx + 1] = 'f';
	pPath[extIdx + 2] = 'c';
	pPath[extIdx + 3] = 'a';
	pPath[extIdx + 4] = 't';
	pPath[extIdx + 5] = 0;
	sxData* pCatData = nxData::load(pPath);
	if (!pCatData) {
		nxCore::dbg_msg("Can't find catalogue for pkg: '%s'\n", pName);
		if (pPath != path) {
			nxCore::mem_free(pPath);
		}
		return nullptr;
	}
	sxFileCatalogue* pCat = pCatData->as<sxFileCatalogue>();
	const char* pPkgName = pCat ? pCat->get_name() : nullptr;
	if (!pPkgName) pPkgName = pName;
	cxResourceManager::Pkg* pPkg = nullptr;
	if (mpPkgMap->get(pPkgName, &pPkg)) {
		nxData::unload(pCatData);
		if (pPath != path) {
			nxCore::mem_free(pPath);
		}
		nxCore::dbg_msg("Pkg \"%s\": already loaded.\n", pPkgName);
		return pPkg;
	}
	if (pCat) {
		pPkg = mpPkgList->new_item();
		if (pPkg) {
			pPkg->mpName = nxCore::str_dup(pPkgName, "xPkg:name");
			pPkg->mpMgr = this;
			pPkg->mpCat = pCat;
			pPkg->mpEntries = Pkg::EntryList::create("xPkg:entries");
			pPkg->mpGeoDataMap = GeoDataMap::create("xPkg:GeoMap");
			pPkg->mpImgDataMap = ImgDataMap::create("xPkg:ImgMap");
			pPkg->mpRigDataMap = RigDataMap::create("xPkg:RigMap");
			pPkg->mpKfrDataMap = KfrDataMap::create("xPkg:KfrMap");
			pPkg->mpValDataMap = ValDataMap::create("xPkg:ValMap");
			pPkg->mpExpDataMap = ExpDataMap::create("xPkg:ExpMap");
			pPkg->mpMdlDataMap = MdlDataMap::create("xPkg:MdlMap");
			pPkg->mpTexDataMap = TexDataMap::create("xPkg:TexMap");
			pPkg->mpMotDataMap = MotDataMap::create("xPkg:MotMap");
			pPkg->mpColDataMap = ColDataMap::create("xPkg:ColMap");
			pPkg->mpDefMdl = nullptr;
			pPkg->mpDefGeo = nullptr;
			pPkg->mpDefRig = nullptr;
			pPkg->mpDefVal = nullptr;
			pPkg->mpDefExp = nullptr;
			pPkg->mGeoNum = 0;
			pPkg->mImgNum = 0;
			pPkg->mRigNum = 0;
			pPkg->mKfrNum = 0;
			pPkg->mValNum = 0;
			pPkg->mExpNum = 0;
			pPkg->mMdlNum = 0;
			pPkg->mTexNum = 0;
			pPkg->mMotNum = 0;
			pPkg->mColNum = 0;
			if (pPkg->mpEntries) {
				for (uint32_t i = 0; i < pCat->mFilesNum; ++i) {
					const char* pItemName = pCat->get_item_name(i);
					const char* pFileName = pCat->get_file_name(i);
					sxData* pData = nullptr;
					if (pFileName) {
						size_t lenFileName = nxCore::str_len(pFileName);
						pathSize = lenDataPath + 1 + lenName + 1 + lenFileName + 1;
						if (pathSize > pathBufSize) {
							if (pPath != path) {
								nxCore::mem_free(pPath);
							}
							pPath = (char*)nxCore::mem_alloc(pathSize, "RsrcMgr:path");
							pathBufSize = pathSize;
						}
						nxCore::mem_copy(pPath, mpDataPath, lenDataPath);
						pPath[lenDataPath] = '/';
						nxCore::mem_copy(pPath + lenDataPath + 1, pName, lenName);
						pPath[lenDataPath + 1 + lenName] = '/';
						size_t fileNameIdx = lenDataPath + 1 + lenName + 1;
						nxCore::mem_copy(pPath + fileNameIdx, pFileName, lenFileName);
						pPath[fileNameIdx + lenFileName] = 0;
						pData = nxData::load(pPath);
					}
					if (pData) {
						Pkg::Entry* pEntry = pPkg->mpEntries->new_item();
						if (pEntry) {
							pEntry->set_data(pData);
							pEntry->mpName = pItemName;
							pEntry->mpFileName = pFileName;
							mpDataToPkgMap->put(pEntry->mAddrKey, pPkg);
						}
						if (pData->is<sxGeometryData>()) {
							sxGeometryData* pGeo = pData->as<sxGeometryData>();
							if (nxCore::str_eq(pItemName, pPkgName)) {
								pPkg->mpDefGeo = pGeo;
							}
							if (pPkg->mpGeoDataMap) {
								pPkg->mpGeoDataMap->put(pItemName, pGeo);
							}
							++pPkg->mGeoNum;
						} else if (pData->is<sxImageData>()) {
							sxImageData* pImg = pData->as<sxImageData>();
							if (pPkg->mpImgDataMap) {
								pPkg->mpImgDataMap->put(pItemName, pImg);
							}
							++pPkg->mImgNum;
						} else if (pData->is<sxRigData>()) {
							sxRigData* pRig = pData->as<sxRigData>();
							if (nxCore::str_eq(pItemName, pPkgName)) {
								pPkg->mpDefRig = pRig;
							}
							if (pPkg->mpRigDataMap) {
								pPkg->mpRigDataMap->put(pItemName, pRig);
							}
							++pPkg->mRigNum;
						} else if (pData->is<sxKeyframesData>()) {
							sxKeyframesData* pKfr = pData->as<sxKeyframesData>();
							if (pPkg->mpKfrDataMap) {
								pPkg->mpKfrDataMap->put(pItemName, pKfr);
							}
							++pPkg->mKfrNum;
						} else if (pData->is<sxValuesData>()) {
							sxValuesData* pVal = pData->as<sxValuesData>();
							if (nxCore::str_eq(pItemName, pPkgName)) {
								pPkg->mpDefVal = pVal;
							}
							if (pPkg->mpValDataMap) {
								pPkg->mpValDataMap->put(pItemName, pVal);
							}
							++pPkg->mValNum;
						} else if (pData->is<sxExprLibData>()) {
							sxExprLibData* pExp = pData->as<sxExprLibData>();
							if (nxCore::str_eq(pItemName, pPkgName)) {
								pPkg->mpDefExp = pExp;
							}
							if (pPkg->mpExpDataMap) {
								pPkg->mpExpDataMap->put(pItemName, pExp);
							}
							++pPkg->mExpNum;
						} else if (pData->is<sxModelData>()) {
							sxModelData* pMdl = pData->as<sxModelData>();
							if (nxCore::str_eq(pItemName, pPkgName)) {
								pPkg->mpDefMdl = pMdl;
							} else if (!pPkg->mpDefMdl && nxCore::str_eq(pItemName, "mdl")) {
								pPkg->mpDefMdl = pMdl;
							}
							if (pPkg->mpMdlDataMap) {
								pPkg->mpMdlDataMap->put(pItemName, pMdl);
							}
							++pPkg->mMdlNum;
						} else if (pData->is<sxTextureData>()) {
							sxTextureData* pTex = pData->as<sxTextureData>();
							if (pPkg->mpTexDataMap) {
								pPkg->mpTexDataMap->put(pItemName, pTex);
							}
							++pPkg->mTexNum;
						} else if (pData->is<sxMotionData>()) {
							sxMotionData* pMot = pData->as<sxMotionData>();
							if (pPkg->mpMotDataMap) {
								pPkg->mpMotDataMap->put(pItemName, pMot);
							}
							++pPkg->mMotNum;
						} else if (pData->is<sxCollisionData>()) {
							sxCollisionData* pCol = pData->as<sxCollisionData>();
							if (pPkg->mpColDataMap) {
								pPkg->mpColDataMap->put(pItemName, pCol);
							}
							++pPkg->mColNum;
						}
					}
				}
			}
			mpPkgMap->put(pPkg->get_name(), pPkg);
		}
	} else {
		nxData::unload(pCatData);
		pCatData = nullptr;
	}
	if (pPath != path) {
		nxCore::mem_free(pPath);
	}
	return pPkg;
}

void cxResourceManager::unload_pkg(Pkg* pPkg) {
	if (this->contains_pkg(pPkg)) {
		if (mpPkgMap) {
			mpPkgMap->remove(pPkg->get_name());
		}
		if (mpPkgList) {
			mpPkgList->remove(pPkg);
		}
	}
}

void cxResourceManager::unload_all() {
	if (mpPkgList) {
		if (mpPkgMap) {
			for (PkgList::Itr itr = mpPkgList->get_itr(); !itr.end(); itr.next()) {
				Pkg* pPkg = itr.item();
				mpPkgMap->remove(pPkg->get_name());
			}
		}
		mpPkgList->purge();
	}
}

cxResourceManager::Pkg* cxResourceManager::find_pkg(const char* pName) const {
	Pkg* pPkg = nullptr;
	if (pName && mpPkgMap) {
		mpPkgMap->get(pName, &pPkg);
	}
	return pPkg;
}

cxResourceManager::Pkg* cxResourceManager::find_pkg_for_data(sxData* pData) const {
	Pkg* pPkg = nullptr;
	if (pData && mpDataToPkgMap) {
		char addrKey[XD_RSRC_ADDR_KEY_SIZE];
		make_data_addr_key(addrKey, pData);
		mpDataToPkgMap->get(addrKey, &pPkg);
	}
	return pPkg;
}

sxGeometryData* cxResourceManager::find_geometry_in_pkg(Pkg* pPkg, const char* pGeoName) const {
	sxGeometryData* pGeo = nullptr;
	if (this->contains_pkg(pPkg)) {
		pGeo = pPkg->find_geometry(pGeoName);
	}
	return pGeo;
}

sxImageData* cxResourceManager::find_image_in_pkg(Pkg* pPkg, const char* pImgName) const {
	sxImageData* pImg = nullptr;
	if (this->contains_pkg(pPkg)) {
		pImg = pPkg->find_image(pImgName);
	}
	return pImg;
}

sxRigData* cxResourceManager::find_rig_in_pkg(Pkg* pPkg, const char* pRigName) const {
	sxRigData* pRig = nullptr;
	if (this->contains_pkg(pPkg)) {
		pRig = pPkg->find_rig(pRigName);
	}
	return pRig;
}

sxKeyframesData* cxResourceManager::find_keyframes_in_pkg(Pkg* pPkg, const char* pKfrName) const {
	sxKeyframesData* pKfr = nullptr;
	if (this->contains_pkg(pPkg)) {
		pKfr = pPkg->find_keyframes(pKfrName);
	}
	return pKfr;
}

sxValuesData* cxResourceManager::find_values_in_pkg(Pkg* pPkg, const char* pValName) const {
	sxValuesData* pVal = nullptr;
	if (this->contains_pkg(pPkg)) {
		pVal = pPkg->find_values(pValName);
	}
	return pVal;
}

sxExprLibData* cxResourceManager::find_expressions_in_pkg(Pkg* pPkg, const char* pExpName) const {
	sxExprLibData* pExp = nullptr;
	if (this->contains_pkg(pPkg)) {
		pExp = pPkg->find_expressions(pExpName);
	}
	return pExp;
}

sxModelData* cxResourceManager::find_model_in_pkg(Pkg* pPkg, const char* pMdlName) const {
	sxModelData* pMdl = nullptr;
	if (this->contains_pkg(pPkg)) {
		pMdl = pPkg->find_model(pMdlName);
	}
	return pMdl;
}

sxTextureData* cxResourceManager::find_texture_in_pkg(Pkg* pPkg, const char* pTexName) const {
	sxTextureData* pTex = nullptr;
	if (this->contains_pkg(pPkg)) {
		pTex = pPkg->find_texture(pTexName);
	}
	return pTex;
}

sxMotionData* cxResourceManager::find_motion_in_pkg(Pkg* pPkg, const char* pMotName) const {
	sxMotionData* pMot = nullptr;
	if (this->contains_pkg(pPkg)) {
		pMot = pPkg->find_motion(pMotName);
	}
	return pMot;
}

sxCollisionData* cxResourceManager::find_collision_in_pkg(Pkg* pPkg, const char* pColName) const {
	sxCollisionData* pCol = nullptr;
	if (this->contains_pkg(pPkg)) {
		pCol = pPkg->find_collision(pColName);
	}
	return pCol;
}

void cxResourceManager::set_gfx_ifc(const GfxIfc& ifc) {
	mGfxIfc = ifc;
}

void cxResourceManager::prepare_pkg_gfx(Pkg* pPkg) {
	if (this->contains_pkg(pPkg)) {
		pPkg->prepare_gfx();
	}
}

void cxResourceManager::release_pkg_gfx(Pkg* pPkg) {
	if (this->contains_pkg(pPkg)) {
		pPkg->release_gfx();
	}
}

void cxResourceManager::prepare_all_gfx() {
	if (!mpPkgList) return;
	for (PkgList::Itr itr = mpPkgList->get_itr(); !itr.end(); itr.next()) {
		Pkg* pPkg = itr.item();
		pPkg->prepare_gfx();
	}
}

void cxResourceManager::release_all_gfx() {
	if (!mpPkgList) return;
	for (PkgList::Itr itr = mpPkgList->get_itr(); !itr.end(); itr.next()) {
		Pkg* pPkg = itr.item();
		pPkg->release_gfx();
	}
}

cxResourceManager* cxResourceManager::create(const char* pAppPath, const char* pRelDataDir) {
	cxResourceManager* pMgr = (cxResourceManager*)nxCore::mem_alloc(sizeof(cxResourceManager), "xRsrcMgr");
	if (pMgr) {
		pMgr->mpAppPath = pAppPath ? nxCore::str_dup(pAppPath, "xRsrcMgr:appPath") : nullptr;
		pMgr->mpRelDataDir = nxCore::str_dup(pRelDataDir ? pRelDataDir : "../data", "xRsrcMgr:relDir");
		size_t pathLen = pMgr->mpAppPath ? nxCore::str_len(pMgr->mpAppPath) : 0;
		if (pathLen > 0) {
			--pathLen;
			while (pathLen > 0) {
				if (pMgr->mpAppPath[pathLen] == '/' || pMgr->mpAppPath[pathLen] == '\\') {
					++pathLen;
					break;
				}
				--pathLen;
			}
		} else {
			pathLen = 0;
		}
		size_t pathSize = pathLen + nxCore::str_len(pMgr->mpRelDataDir) + 1;
		pMgr->mpDataPath = (char*)nxCore::mem_alloc(pathSize, "xRsrcMgr:path");
		if (pMgr->mpDataPath) {
			nxCore::mem_zero(pMgr->mpDataPath, pathSize);
			if (pathLen) {
				nxCore::mem_copy(pMgr->mpDataPath, pMgr->mpAppPath, pathLen);
			}
			nxCore::mem_copy(pMgr->mpDataPath + pathLen, pMgr->mpRelDataDir, nxCore::str_len(pMgr->mpRelDataDir));
		}
		pMgr->mpPkgList = PkgList::create("xRsrcMgr:PkgList");
		pMgr->mpPkgMap = PkgMap::create("xRsrcMgr:PkgMap");
		pMgr->mpDataToPkgMap = DataToPkgMap::create("xRsrcMgr:DataMap");
		if (pMgr->mpPkgList) {
			pMgr->mpPkgList->set_item_handlers(pkg_ctor, pkg_dtor);
		}
		pMgr->mGfxIfc.reset();
	}
	return pMgr;
}

void cxResourceManager::destroy(cxResourceManager* pMgr) {
	if (!pMgr) return;
	pMgr->unload_all();
	PkgList::destroy(pMgr->mpPkgList);
	PkgMap::destroy(pMgr->mpPkgMap);
	DataToPkgMap::destroy(pMgr->mpDataToPkgMap);
	nxCore::mem_free(pMgr->mpAppPath);
	nxCore::mem_free(pMgr->mpRelDataDir);
	nxCore::mem_free(pMgr->mpDataPath);
	nxCore::mem_free(pMgr);
}


static const uint64_t s_xqc_kwcodes[] = {
	0x0000000000006669ULL, /* if */
	0x0000000000006970ULL, /* pi */
	0x0000000000006E69ULL, /* in */
	0x0000000000006F64ULL, /* do */
	0x0000000000726F66ULL, /* for */
	0x0000000000746E69ULL, /* int */
	0x000000000074756FULL, /* out */
	0x0000000063657666ULL, /* fvec */
	0x0000000064696F76ULL, /* void */
	0x0000000065736163ULL, /* case */
	0x0000000065736C65ULL, /* else */
	0x0000000065747962ULL, /* byte */
	0x0000000065757274ULL, /* true */
	0x00000000676E6F6CULL, /* long */
	0x000000006C6F6F62ULL, /* bool */
	0x000000006D756E65ULL, /* enum */
	0x000000006F746F67ULL, /* goto */
	0x000000006F747561ULL, /* auto */
	0x0000000072616863ULL, /* char */
	0x0000000078746D66ULL, /* fmtx */
	0x0000000078746D78ULL, /* xmtx */
	0x000000656C696877ULL, /* while */
	0x00000065736C6166ULL, /* false */
	0x0000006B61657262ULL, /* break */
	0x0000007269617066ULL, /* fpair */
	0x00000074616F6C66ULL, /* float */
	0x0000007461757166ULL, /* fquat */
	0x00000074726F6873ULL, /* short */
	0x00000074736E6F63ULL, /* const */
	0x00000074756F6E69ULL, /* inout */
	0x0000636974617473ULL, /* static */
	0x000064656E676973ULL, /* signed */
	0x0000656C62756F64ULL, /* double */
	0x0000656E696C6E69ULL, /* inline */
	0x0000666F657A6973ULL, /* sizeof */
	0x0000686374697773ULL, /* switch */
	0x00006E7275746572ULL, /* return */
	0x0000746375727473ULL, /* struct */
	0x0066656465707974ULL, /* typedef */
	0x00746C7561666564ULL, /* default */
	0x64656E6769736E75ULL, /* unsigned */
	0x65756E69746E6F63ULL  /* continue */
};

static const int s_xqc_kworg = (int)cxXqcLexer::TokId::TOK_KW_IF;
static const int s_xqc_kwend = (int)cxXqcLexer::TokId::TOK_KW_CONTINUE;
static const int s_xqc_nkw = 42;

static const uint32_t s_xqc_plst[] = {
	0x00000021UL, /* ! */
	0x00000025UL, /* % */
	0x00000026UL, /* & */
	0x00000028UL, /* ( */
	0x00000029UL, /* ) */
	0x0000002AUL, /* * */
	0x0000002BUL, /* + */
	0x0000002CUL, /* , */
	0x0000002DUL, /* - */
	0x0000002EUL, /* . */
	0x0000002FUL, /* / */
	0x0000003AUL, /* : */
	0x0000003BUL, /* ; */
	0x0000003CUL, /* < */
	0x0000003DUL, /* = */
	0x0000003EUL, /* > */
	0x0000005BUL, /* [ */
	0x0000005DUL, /* ] */
	0x0000005EUL, /* ^ */
	0x0000007BUL, /* { */
	0x0000007CUL, /* | */
	0x0000007DUL, /* } */
	0x0000007EUL, /* ~ */
	0x00002626UL, /* && */
	0x00002B2BUL, /* ++ */
	0x00002D2DUL, /* -- */
	0x00003A3AUL, /* :: */
	0x00003C3CUL, /* << */
	0x00003D21UL, /* != */
	0x00003D25UL, /* %= */
	0x00003D26UL, /* &= */
	0x00003D2AUL, /* *= */
	0x00003D2BUL, /* += */
	0x00003D2DUL, /* -= */
	0x00003D2FUL, /* /= */
	0x00003D3CUL, /* <= */
	0x00003D3DUL, /* == */
	0x00003D3EUL, /* >= */
	0x00003D5EUL, /* ^= */
	0x00003D7CUL, /* |= */
	0x00003E3EUL, /* >> */
	0x00007C7CUL, /* || */
	0x003D3C3CUL, /* <<= */
	0x003D3E3EUL  /* >>= */
};

static const int s_xqc_punorg = (int)cxXqcLexer::TokId::TOK_NOT;
static const int s_xqc_punend = (int)cxXqcLexer::TokId::TOK_DSTSHR;
static const int s_xqc_npun = 44;
static const int s_xqc_punmaxlen = 3;

template<typename CODE_T> int xqc_find_code(const CODE_T* pTbl, const CODE_T code, const int n) {
	const CODE_T* p = pTbl;
	uint32_t cnt = (uint32_t)n;
	while (cnt > 1) {
		uint32_t mid = cnt / 2;
		const CODE_T* pm = &p[mid];
		p = (code < *pm) ? p : pm;
		cnt -= mid;
	}
	return *p == code ? (int)(p - pTbl) : -1;
}

template<typename CODE_T> union uxXqcSymCode {
	CODE_T cod;
	char chr[sizeof(CODE_T)];

	uxXqcSymCode(const char* pStr) {
		from_str(pStr);
	}

	void from_str(const char* pStr) {
		size_t idx = 0;
		cod = 0;
		while (pStr[idx] && idx < sizeof(CODE_T)) {
			chr[idx] = pStr[idx];
			++idx;
		}
		if (pStr[idx]) {
			cod = 0; /* too long for a keyword */
		}
	}
};

cxXqcLexer::TokId xqc_find_kwid(const char* pKW) {
	int id = xqc_find_code<uint64_t>(s_xqc_kwcodes, uxXqcSymCode<uint64_t>(pKW).cod, s_xqc_nkw);
	if (id >= 0) {
		id += s_xqc_kworg;
	}
	return (cxXqcLexer::TokId)id;
}

cxXqcLexer::TokId xqc_find_punctid(const char* pPun) {
	int id = xqc_find_code<uint32_t>(s_xqc_plst, uxXqcSymCode<uint32_t>(pPun).cod, s_xqc_npun);
	if (id >= 0) {
		id += s_xqc_punorg;
	}
	return (cxXqcLexer::TokId)id;
}

static bool xqc_dec_digit_ck(int ch) {
	return ch >= '0' && ch <= '9';
}

static bool xqc_hex_digit_ck(int ch) {
	return xqc_dec_digit_ck(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

static int64_t xqc_parse_hex(const char* pStr, const size_t len) {
	int64_t val = 0;
	const char* p0 = pStr;
	bool negFlg = p0[0] == '-';
	if (negFlg) {
		++p0;
	} else if (p0[0] == '+') {
		++p0;
	}
	p0 += 2;
	const char* p = pStr + len - 1;
	int s = 0;
	while (p >= p0) {
		char c = *p;
		int64_t d = 0;
		if (xqc_dec_digit_ck(c)) {
			d = c - '0';
		} else if (c >= 'a' && c <= 'f') {
			d = c - 'a' + 10;
		} else if (c >= 'A' && c <= 'F') {
			d = c - 'A' + 10;
		} else {
			return 0;
		}
		val |= d << s;
		s += 4;
		--p;
	}
	if (negFlg) {
		val = -val;
	}
	return val;
}

struct sxXqcTmpStr {
	static const int BUF_SIZE = 128;

	char mBuf[BUF_SIZE];
	char* mpBuf;
	size_t mSize;
	size_t mCursor;
	sxLock* mpMemLock;

	sxXqcTmpStr() {
		mpMemLock = nullptr;
		mpBuf = mBuf;
		mSize = BUF_SIZE;
		start();
	}

	~sxXqcTmpStr() {
		reset();
	}

	void start() {
		mCursor = 0;
	}

	void set_mem_lock(sxLock* pLock) {
		mpMemLock = pLock;
	}

	sxLock* get_mem_lock() {
		return mpMemLock;
	}

	void mem_lock_acq() {
		if (mpMemLock) {
			nxSys::lock_acquire(mpMemLock);
		}
	}

	void mem_lock_rel() {
		if (mpMemLock) {
			nxSys::lock_release(mpMemLock);
		}
	}

	void reset() {
		if (mpBuf != mBuf) {
			mem_lock_acq();
			nxCore::mem_free(mpBuf);
			mem_lock_rel();
			mpBuf = nullptr;
		}
		mCursor = 0;
	}

	void put(char ch) {
		if (mCursor >= mSize - 1) {
			size_t newSize = (size_t)(mSize * 1.5f);
			mem_lock_acq();
			char* pNewBuf = (char*)nxCore::mem_alloc(newSize, "Xqc::tstr");
			nxCore::mem_copy(pNewBuf, mpBuf, mCursor);
			if (mpBuf != mBuf) {
				nxCore::mem_free(mpBuf);
			}
			mem_lock_rel();
			mpBuf = pNewBuf;
			mSize = newSize;
		}
		mpBuf[mCursor++] = ch;
	}

	char* get_str() {
		mpBuf[mCursor] = 0;
		return mpBuf;
	}

	size_t len() {
		return mCursor;
	}
};

bool cxXqcLexer::Token::is_keyword() const {
	return int(id) >= s_xqc_kworg && int(id) <= s_xqc_kwend;
}

bool cxXqcLexer::Token::is_punctuation() const {
	return int(id) >= s_xqc_punorg && int(id) <= s_xqc_punend;
}

bool cxXqcLexer::Token::is_symbol() const {
	return id == TokId::TOK_SYM;
}

bool cxXqcLexer::Token::is_string() const {
	return id == TokId::TOK_QSTR || id == TokId::TOK_SQSTR;
}

cxXqcLexer::cxXqcLexer() {
	mpText = nullptr;
	mTextSize = 0;
	reset();
	mDisableKwd = false;
	mLineCommentCh = 0;
}

void cxXqcLexer::reset() {
	mCursor = 0;
	mLoc.line = 0;
	mLoc.column = 0;
	mPrevLoc = mLoc;
}

void cxXqcLexer::disable_keywords() {
	mDisableKwd = true;
}

void cxXqcLexer::set_line_comment_char(char ch) {
	mLineCommentCh = ch;
}

void cxXqcLexer::set_text(const char* pText, const size_t textSize) {
	mpText = pText;
	mTextSize = textSize;
}

int cxXqcLexer::read_char() {
	if (mCursor >= mTextSize) return -1;
	int ch = -1;
	mPrevLoc = mLoc;
	while (true) {
		if (mCursor >= mTextSize) break;
		char tst = mpText[mCursor];
		++mCursor;
		++mLoc.column;
		bool eolFlg = tst == '\n';
		if (!eolFlg) {
			eolFlg = tst == '\r';
			if (eolFlg) {
				if (mCursor < mTextSize) {
					if (mpText[mCursor] == '\n') {
						tst = mpText[mCursor];
						++mCursor;
					}
				}
			}
		}
		if (eolFlg) {
			++mLoc.line;
			mLoc.column = 0;
			mPrevLoc = mLoc;
		} else {
			ch = tst & 0xFF;
			break;
		}
	}
	return ch;
}

XD_NOINLINE void cxXqcLexer::scan(TokenFunc& func, sxLock* pMemLock) {
	if (!mpText || mTextSize < 1) {
		nxCore::dbg_msg("Xquic has no text to scan.\n");
		return;
	}
	Token tok;
	sxXqcTmpStr tstr;
	tstr.set_mem_lock(pMemLock);
	int ch = 0;
	bool errFlg = false;
	bool readFlg = true;
	bool contFlg = true;
	while (true) {
		if (!contFlg) break;
		if (readFlg) {
			ch = read_char();
		}
		if (ch <= 0) break;
		if (errFlg) break;
		while (ch == ' ' || ch == '\t') {
			ch = read_char();
		}
		readFlg = true;

		if (mLineCommentCh != 0) {
			if (ch == mLineCommentCh) {
				if (mCursor < mTextSize) {
					int line = mLoc.line;
					int next = read_char();
					while (next >= 0 && line == mLoc.line) {
						next = read_char();
					}
					ch = next;
					readFlg = false;
					if (next < 0) break;
					continue;
				} else {
					break;
				}
			}
		} else if (ch == '/') {
			if (mCursor < mTextSize) {
				int next = mpText[mCursor];
				if (next == '/') {
					int line = mLoc.line;
					while (next >= 0 && line == mLoc.line) {
						next = read_char();
					}
					ch = next;
					readFlg = false;
					if (next < 0) break;
					continue;
				} else if (next == '*') {
					bool done = false;
					read_char();
					while (!done) {
						next = read_char();
						if (next == '*') {
							if (mCursor < mTextSize) {
								if (mpText[mCursor] == '/') {
									done = true;
									read_char();
								}
							} else break;
						} else if (next < 0) break;
					}
					continue;
				}
			}
		}

		tok.loc = mPrevLoc;

		bool numFlg = xqc_dec_digit_ck(ch);
		bool sgnFlg = false;
		bool dotFlg = false;
		bool expFlg = false;
		bool hexFlg = false;
		if (!numFlg) {
			sgnFlg = ch == '-' || ch == '+';
			if (sgnFlg) {
				if (mCursor < mTextSize) {
					int next = mpText[mCursor];
					dotFlg = next == '.';
					numFlg = dotFlg || xqc_dec_digit_ck(next);
				}
			} else {
				if (mCursor < mTextSize) {
					dotFlg = ch == '.';
					int next = mpText[mCursor];
					numFlg = dotFlg && xqc_dec_digit_ck(next);
				}
			}
		}
		if (numFlg) {
			if (ch == '0') {
				if (mCursor < mTextSize) {
					int next = mpText[mCursor];
					hexFlg = next == 'x' || next == 'X';
				}
			} else if (sgnFlg) {
				if (mCursor < mTextSize) {
					int next = mpText[mCursor];
					if (next == '0') {
						if (mCursor + 1 < mTextSize) {
							next = mpText[mCursor + 1];
							hexFlg = next == 'x' || next == 'X';
						}
					}
				}
			}
			tstr.start();
			tstr.put(ch);
			int line = mLoc.line;
			while (true) {
				ch = read_char();
				if (ch == '.') {
					dotFlg = true;
				}
				if (ch == 'e' || ch == 'E') {
					expFlg = true;
				}
				bool putFlg = ch >= 0 && line == mLoc.line;
				if (putFlg) {
					if (hexFlg) {
						putFlg = xqc_hex_digit_ck(ch) || ch == 'x' || ch == 'X';
					} else {
						putFlg = xqc_dec_digit_ck(ch) || ch == '.' || ch == '-' || ch == '+' || ch == 'e' || ch == 'E';
					}
				}
				if (putFlg) {
					tstr.put(ch);
				} else {
					if (hexFlg) {
						const char* pHexStr = tstr.get_str();
						int64_t hexVal = xqc_parse_hex(pHexStr, tstr.len());
						tok.id = TokId::TOK_INT;
						tok.val.i = hexVal;
						contFlg = func(tok);
						readFlg = false;
						break;
					} else if (dotFlg || expFlg) {
						const char* pFltStr = tstr.get_str();
						double fltVal = nxCore::parse_f64(pFltStr);
						tok.id = TokId::TOK_FLOAT;
						tok.val.f = fltVal;
						contFlg = func(tok);
						readFlg = false;
						break;
					} else {
						const char* pIntStr = tstr.get_str();
						int64_t intVal = nxCore::parse_i64(pIntStr);
						tok.id = TokId::TOK_INT;
						tok.val.i = intVal;
						contFlg = func(tok);
						readFlg = false;
						break;
					}
				}
			}
			continue;
		}

		char punct[5];
		punct[0] = ch;
		TokId punctId = TokId::TOK_UNKNOWN;
		size_t maxPunct = s_xqc_punmaxlen;
		if (mCursor - 1 + maxPunct >= mTextSize) {
			maxPunct = mTextSize - mCursor + 1;
		}
		size_t punctLen = maxPunct;
		for (size_t i = 1; i < maxPunct; ++i) {
			punct[i] = mpText[mCursor - 1 + i];
		}
		for (size_t i = maxPunct; i > 0; --i) {
			punct[i] = 0;
			punctId = xqc_find_punctid(punct);
			if (punctId != TokId::TOK_UNKNOWN) {
				punctLen = i;
				break;
			}
		}
		if (punctId != TokId::TOK_UNKNOWN) {
			for (size_t i = 0; i < punctLen - 1; ++i) {
				read_char();
			}
			tok.id = punctId;
			for (size_t i = 0; i < punctLen + 1; ++i) {
				tok.val.c[i] = punct[i];
			}
			contFlg = func(tok);
			continue;
		}

		char qch = '"';
		if (ch != qch) {
			qch = '\'';
			if (ch != qch) {
				qch = 0;
			}
		}
		if (qch) {
			tstr.start();
			while (true) {
				ch = read_char();
				if (ch < 0) {
					tok.id = TokId::TOK_UNKNOWN;
					contFlg = func(tok);
					errFlg = true;
					readFlg = false;
					break;
				}
				if (ch == qch) {
					tok.id = qch == '"' ? TokId::TOK_QSTR : TokId::TOK_SQSTR;
					tok.val.p = tstr.get_str();
					contFlg = func(tok);
					break;
				} else {
					tstr.put(ch);
				}
			}
			continue;
		}

		tstr.start();
		tstr.put(ch);
		int line = mLoc.line;
		while (true) {
			ch = read_char();
			bool putFlg = ch >= 0 && line == mLoc.line;
			if (putFlg) {
				putFlg = !(ch == ' ' || ch == '\t');
				if (putFlg) {
					punct[0] = ch;
					punct[1] = 0;
					TokId punctId = xqc_find_punctid(punct);
					putFlg = punctId == TokId::TOK_UNKNOWN;
				}
			}
			if (putFlg) {
				tstr.put(ch);
			} else {
				char* pSymStr = tstr.get_str();
				TokId kwId = TokId::TOK_UNKNOWN;
				if (!mDisableKwd) {
					kwId = xqc_find_kwid(pSymStr);
				}
				if (kwId != TokId::TOK_UNKNOWN) {
					tok.id = kwId;
				} else {
					tok.id = TokId::TOK_SYM;
				}
				tok.val.p = pSymStr;
				contFlg = func(tok);
				readFlg = false;
				break;
			}
		}
	}
}


namespace nxApp {

cxCmdLine* s_pCmdLine = nullptr;

void init_params(int argc, char* argv[]) {
	if (s_pCmdLine) {
		cxCmdLine::destroy(s_pCmdLine);
		s_pCmdLine = nullptr;
	}
	s_pCmdLine = cxCmdLine::create(argc, argv);
}

void reset() {
	cxCmdLine::destroy(s_pCmdLine);
	s_pCmdLine = nullptr;
}

cxCmdLine* get_cmd_line() {
	return s_pCmdLine;
}

int get_opts_count() {
	return s_pCmdLine ? s_pCmdLine->get_opts_count() : 0;
}

const char* get_opt(const char* pName) {
	return s_pCmdLine ? s_pCmdLine->get_opt(pName) : nullptr;
}

int get_int_opt(const char* pName, const int defVal) {
	return s_pCmdLine ? s_pCmdLine->get_int_opt(pName, defVal) : defVal;
}

float get_float_opt(const char* pName, const float defVal) {
	return s_pCmdLine ? s_pCmdLine->get_float_opt(pName, defVal) : defVal;
}

bool get_bool_opt(const char* pName, const bool defVal) {
	return s_pCmdLine ? s_pCmdLine->get_bool_opt(pName, defVal) : defVal;
}

int get_args_count() {
	return s_pCmdLine ? s_pCmdLine->get_args_count() : 0;
}

const char* get_arg(const int idx) {
	return s_pCmdLine ? s_pCmdLine->get_arg(idx) : nullptr;
}

} // nxApp
