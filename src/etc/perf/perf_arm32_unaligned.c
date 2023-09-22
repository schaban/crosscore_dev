/*
	cat /proc/cpu/alignment | grep "faults:"

	gcc perf_arm32_unaligned.c -O3

	results on BeagleBoard-X15:
	 User faults:    2 (fixup)
	 time ./a.out 0 (aligned)
	    real    0m0.512s
	    user    0m0.479s
	    sys     0m0.033s
	 time ./a.out 1 (unaligned)
	    real    0m0.957s
	    user    0m0.526s
	    sys     0m0.429s
*/

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef HANDLE_SIGBUS
#	define HANDLE_SIGBUS 0
#endif

#if HANDLE_SIGBUS
#include <signal.h>

static void sigact_unaligned(int isig, siginfo_t* pInfo, void* pCtxMem) {
	int scode = pInfo->si_code;
	int handled = 0;
	if (isig == SIGBUS && scode == BUS_ADRALN) {
		ucontext_t* pCtx = (ucontext_t*)pCtxMem;
		uintptr_t pc = pCtx->uc_mcontext.arm_pc;
		uint16_t instr = *(uint16_t*)pc;
		if (instr == 0xCA2C) {
			/* ldmia r2, {r2, r3, r5} */
			uint32_t buf[3];
			volatile uint8_t* pSrc = (uint8_t*)pCtx->uc_mcontext.arm_r2;
			uint8_t* pDst = (uint8_t*)buf;
			int i;
			for (i = 0; i < sizeof(uint32_t)*3; ++i) {
				*pDst++ = *pSrc++;
			}
			pCtx->uc_mcontext.arm_r2 = buf[0];
			pCtx->uc_mcontext.arm_r3 = buf[1];
			pCtx->uc_mcontext.arm_r5 = buf[2];
			pCtx->uc_mcontext.arm_pc += 2;
			handled = 1;
		}
	}
	if (!handled) {
		abort();
	}
}

static void set_handler() {
	int res;
	struct sigaction sa;
	sa.sa_sigaction = sigact_unaligned;
	sa.sa_flags = SA_SIGINFO;
	res = sigaction(SIGBUS, &sa, NULL);
	printf("SIGBUS action set: %s\n", res == 0 ? "ok" : "failed");
}
#else
static void set_handler() {}
#endif

uint8_t* gen_rand_f01(uint8_t* pDst) {
	union {
		uint32_t u32;
		float f32;
		uint8_t b[4];
	} bits;
	int i;
	uint32_t r0 = rand() & 0xFF;
	uint32_t r1 = rand() & 0xFF;
	uint32_t r2 = rand() & 0x7F;
	bits.f32 = 1.0f;
	bits.u32 |= r0 | (r1 << 8) | (r2 << 16);
	bits.f32 -= 1.0f;
	for (i = 0; i < 4; ++i) {
		*pDst++ = bits.b[i];
	}
	return pDst;
}

__attribute__((noinline)) void get_f3(float* pDst, const uint8_t* pSrc) {
#if 0
	/* -mno-unaligned-access if slow */
	uint32_t* pS32 = (uint32_t*)pSrc;
	uint32_t* pD32 = (uint32_t*)pDst;
	uint32_t d0 = *pS32++;
	uint32_t d1 = *pS32++;
	uint32_t d2 = *pS32++;
	*pD32++ = d0;
	*pD32++ = d1;
	*pD32++ = d2;
#elif 0
	uint64_t d01 = *(uint64_t*)pSrc;
	uint32_t d2 = ((uint32_t*)pSrc)[2];
	*(uint64_t*)pDst = d01;
	((uint32_t*)pDst)[2] = d2;
#elif 1
	__asm__(
		"mov r2, %0\n"
		"ldmia r2, {r2, r3, r5}\n"
		"strd r2, r3, [%1]\n"
		"str.w r5, [%1, #8]\n"
	:
	: "r" (pSrc), "r" (pDst)
	: "r1", "r2", "r3", "r5", "memory"
	);
#else
	__asm__(
		"vld1.32 {d16}, [%0]\n"
		"ldr r5, [%0, #8]\n"
		"vst1.32 {d16}, [%1]\n"
		"str.w r5, [%1, #8]\n"
	:
	: "r" (pSrc), "r" (pDst)
	: "r5", "d16", "memory"
	);
#endif
}

int main(int argc, char* argv[]) {
	void* pMem;
	int offs = 0;
	int n = 1000000;
	int seed = 1;
	if (argc > 1) {
		offs = atoi(argv[1]);
	}
	if (argc > 2) {
		n = atoi(argv[2]);
	}
	if (argc > 3) {
		seed = atoi(argv[3]);
	}
	printf("offs = %d\n", offs);
	printf("n = %d, %d\n", n, n - (n % 3));
	printf("seed = %d\n", seed);

	set_handler();
	srand(seed);
	pMem = malloc(n*sizeof(float) + offs);
	if (pMem) {
		int i, j;
		int n3 = n / 3;
		float sum = 0.0f;
		float* pFloats = (float*)((uintptr_t)pMem + offs);
		uint8_t* pDst = (uint8_t*)pFloats;
		uint8_t* pSrc = pDst;
		for (i = 0; i < n; ++i) {
			pDst = gen_rand_f01(pDst);
		}
		printf("pFloats = %p\n", pFloats);
		for (i = 0; i < n3; ++i) {
			float ftmp[3];
			get_f3(ftmp, pSrc);
			pSrc += 3 * sizeof(float);
			for (j = 0; j < 3; ++j) {
				sum += ftmp[j];
			}
		}
		printf("sum = %.2f\n", sum);
	}

	return 0;
}

