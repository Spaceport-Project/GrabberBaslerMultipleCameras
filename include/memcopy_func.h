// #include <time.h>
// #include <math.h>
// #include <stdint.h>
// #include <assert.h>
// #include <string.h>
// #include <stdio.h>
// #include <stdlib.h>

// size_t      iters = 0; // 2^36

// //-----------------------------------------------------------------------------
// // Optimized memcpy
// //

// #if !defined(__GNUC__) && !defined(__attribute__) // no GCC attribute syntax
// #define __attribute__(X)
// #endif

// #if defined(__GNUC__) || defined(_MSC_VER) || defined(__restrict)
// #define restrict __restrict
// #elif !defined(restrict) // restrict or __restrict not supported in C++
// #define restrict
// #endif

// static inline void __attribute__((nonnull))
// copy_small(uint8_t *restrict dst, const uint8_t *restrict src, size_t n)
// {
//     if (n >= 8)
//     {
//         *(uint64_t *restrict)dst = *(const uint64_t *restrict)src;
//         return;
//     }
//     if (n >= 4)
//     {
//         *(uint32_t *restrict)dst = *(const uint32_t *restrict)src;
//         dst += 4;
//         src += 4;
//     }
//     if (n & 2)
//     {
//         *(uint16_t *restrict)dst = *(const uint16_t *restrict)src;
//         dst += 2;
//         src += 2;
//     }
//     if (n & 1)
//         *dst = *src;
// }

// static inline void __attribute__((nonnull))
// copy512(uint64_t *restrict dst, const uint64_t *restrict src, size_t n)
// {
//     size_t chunks;
//     size_t offset;

//     chunks = n >> 3;
//     offset = n - (chunks << 3);
//     while (chunks--)
//     {
//         *dst++ = *src++;
//         *dst++ = *src++;
//         *dst++ = *src++;
//         *dst++ = *src++;
//         *dst++ = *src++;
//         *dst++ = *src++;
//         *dst++ = *src++;
//         *dst++ = *src++;
//     }
//     while (offset--)
//         *dst++ = *src++;
// }

// void __attribute__((nonnull))
// *mem_cpy(void *restrict dst, const void *restrict src, size_t n)
// {
//     uint8_t         *dst8;
//     const uint8_t   *src8;
//     size_t          qwords;
//     size_t          aligned_size;

//     dst8 = (uint8_t*)dst;
//     src8 = (const uint8_t*)src;
//     qwords = n >> 3;
//     if (n > 8)
//     {
//         copy512((uint64_t*)dst, (const uint64_t*)src, qwords);
//         return (dst);
//     }
//     aligned_size = qwords << 3;
//     n -= aligned_size;
//     dst8 += aligned_size;
//     src8 += aligned_size;
//     copy_small(dst8, src8, n);
//     return (dst);
// }

//  void fast_memcpy(void *dst, const void *src, size_t len) {
//     __m128i *dst_ptr = (__m128i *)dst;
//     const __m128i *src_ptr = (const __m128i *)src;

//     for (size_t i = 0; i < len / 16; i++) {
//         _mm_storeu_si128(dst_ptr + i, _mm_loadu_si128(src_ptr + i));
//     }
// }

//  void fast_memcpy_asm(void *dst, const void *src, size_t len) {
//     asm volatile("rep movsb"
//                  : "=D" (dst), "=S" (src), "=c" (len)
//                  : "0" (dst), "1" (src), "2" (len)
//                  : "cc");
// }

//  void X_aligned_memcpy_sse2(void* dest, const void* src, const unsigned long size) {
//     __asm__ __volatile__(
//         "movq %[src], %%rsi;"
//         "movq %[dest], %%rdi;"
//         "movq %[size], %%rbx;"
//         "shrq $7, %%rbx;"  // divide by 128 (8 * 128bit registers)

//         "loop_copy:"
//         "prefetchnta 128(%%rsi);"
//         "prefetchnta 160(%%rsi);"
//         "prefetchnta 192(%%rsi);"
//         "prefetchnta 224(%%rsi);"

//         "movdqa (%%rsi), %%xmm0;"
//         "movdqa 16(%%rsi), %%xmm1;"
//         "movdqa 32(%%rsi), %%xmm2;"
//         "movdqa 48(%%rsi), %%xmm3;"
//         "movdqa 64(%%rsi), %%xmm4;"
//         "movdqa 80(%%rsi), %%xmm5;"
//         "movdqa 96(%%rsi), %%xmm6;"
//         "movdqa 112(%%rsi), %%xmm7;"

//         "movntdq %%xmm0, (%%rdi);"
//         "movntdq %%xmm1, 16(%%rdi);"
//         "movntdq %%xmm2, 32(%%rdi);"
//         "movntdq %%xmm3, 48(%%rdi);"
//         "movntdq %%xmm4, 64(%%rdi);"
//         "movntdq %%xmm5, 80(%%rdi);"
//         "movntdq %%xmm6, 96(%%rdi);"
//         "movntdq %%xmm7, 112(%%rdi);"

//         "addq $128, %%rsi;"
//         "addq $128, %%rdi;"
//         "decq %%rbx;"
//         "jnz loop_copy;"
//         :
//         : [src] "r" (src), [dest] "r" (dest), [size] "r" (size)
//         : "memory", "cc", "rsi", "rdi", "rbx", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
//     );
// }


#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <cstdint>
#include <stdlib.h>
#include <immintrin.h>


#if !defined(__GNUC__) && !defined(__attribute__) // no GCC attribute syntax
#define __attribute__(X)
#endif

#ifdef __cplusplus // C++
extern "C" {

#if defined(__GNUC__) || defined(_MSC_VER) || defined(__restrict)
#define restrict __restrict
#elif !defined(restrict) // restrict or __restrict not supported in C++
#define restrict
#endif

#endif

static inline __attribute__((nonnull))
void copy_small(uint8_t *restrict dst, const uint8_t *restrict src, size_t size);

static inline __attribute__((nonnull))
void copy64(uint64_t *restrict dst, const uint64_t *restrict src, size_t n) ;

static inline __attribute__((nonnull))
void copy512(uint64_t *restrict dst, const uint64_t *restrict src, size_t n) ;

__attribute__((nonnull))
void *mem_cpy(void *restrict dst, const void *restrict src, size_t size) ;


__attribute__((nonnull))
void *mem_cpy2(void *restrict dst, const void *restrict src, size_t size) ;

/* GCC optimizes this to a call to libc memcpy unless compiled with
 * -fno-tree-loop-distribute-patterns
 */
__attribute__((nonnull))
void *mem_cpy_naive(void *restrict dst, const void *restrict src, size_t size);

void fast_memcpy(void *dst, const void *src, size_t len) ;


//  void fast_memcpy(void *dst, const void *src, size_t len) {
//     __m128i *dst_ptr = (__m128i *)dst;
//     const __m128i *src_ptr = (const __m128i *)src;

//     for (size_t i = 0; i < len / 16; i++) {
//         _mm_storeu_si128(dst_ptr + i, _mm_loadu_si128(src_ptr + i));
//     }
// }

//  void fast_memcpy_asm(void *dst, const void *src, size_t len) {
//     asm volatile("rep movsb"
//                  : "=D" (dst), "=S" (src), "=c" (len)
//                  : "0" (dst), "1" (src), "2" (len)
//                  : "cc");
// }

//  void X_aligned_memcpy_sse2(void* dest, const void* src, const unsigned long size) {
//     __asm__ __volatile__(
//         "movq %[src], %%rsi;"
//         "movq %[dest], %%rdi;"
//         "movq %[size], %%rbx;"
//         "shrq $7, %%rbx;"  // divide by 128 (8 * 128bit registers)

//         "loop_copy:"
//         "prefetchnta 128(%%rsi);"
//         "prefetchnta 160(%%rsi);"
//         "prefetchnta 192(%%rsi);"
//         "prefetchnta 224(%%rsi);"

//         "movdqa (%%rsi), %%xmm0;"
//         "movdqa 16(%%rsi), %%xmm1;"
//         "movdqa 32(%%rsi), %%xmm2;"
//         "movdqa 48(%%rsi), %%xmm3;"
//         "movdqa 64(%%rsi), %%xmm4;"
//         "movdqa 80(%%rsi), %%xmm5;"
//         "movdqa 96(%%rsi), %%xmm6;"
//         "movdqa 112(%%rsi), %%xmm7;"

//         "movntdq %%xmm0, (%%rdi);"
//         "movntdq %%xmm1, 16(%%rdi);"
//         "movntdq %%xmm2, 32(%%rdi);"
//         "movntdq %%xmm3, 48(%%rdi);"
//         "movntdq %%xmm4, 64(%%rdi);"
//         "movntdq %%xmm5, 80(%%rdi);"
//         "movntdq %%xmm6, 96(%%rdi);"
//         "movntdq %%xmm7, 112(%%rdi);"

//         "addq $128, %%rsi;"
//         "addq $128, %%rdi;"
//         "decq %%rbx;"
//         "jnz loop_copy;"
//         :
//         : [src] "r" (src), [dest] "r" (dest), [size] "r" (size)
//         : "memory", "cc", "rsi", "rdi", "rbx", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
//     );
// }

#ifdef __cplusplus
}
#endif