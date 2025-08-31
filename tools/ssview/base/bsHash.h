#pragma once

#include "bs.h"

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_umul128)  // For Wyhash
#endif

namespace
{

// ==========================================================================================
// Wyhash https://github.com/wangyi-fudan/wyhash/tree/master (18a25157b modified)
// This is free and unencumbered software released into the public domain under The Unlicense
// (http://unlicense.org/)
// ==========================================================================================

static inline void
wymum(uint64_t* A, uint64_t* B)
{
#if defined(_MSC_VER)
    *A = _umul128(*A, *B, B);
#else
    __uint128_t r = *A;
    r *= *B;
    *A = (uint64_t)r;
    *B = (uint64_t)(r >> 64);
#endif
}

static inline uint64_t
wymix(uint64_t A, uint64_t B)
{
    wymum(&A, &B);
    return A ^ B;
}
static inline uint64_t
wyr8(const uint8_t* p)
{
    uint64_t v;  // NOLINT(cppcoreguidelines-init-variables)
    memcpy(&v, p, 8);
    return v;
}
static inline uint64_t
wyr4(const uint8_t* p)
{
    uint32_t v;  // NOLINT(cppcoreguidelines-init-variables)
    memcpy(&v, p, 4);
    return v;
}
static inline uint64_t
wyr3(const uint8_t* p, size_t k)
{
    return (((uint64_t)p[0]) << 16) | (((uint64_t)p[k >> 1]) << 8) | p[k - 1];
}

};  // namespace

namespace bs
{

static inline uint64_t
wyhash(const void* key, size_t len)
{
    constexpr uint64_t secret0 = 0x2d358dccaa6c78a5ull;
    constexpr uint64_t secret1 = 0x8bb84b93962eacc9ull;
    constexpr uint64_t secret2 = 0x4b33a62ed433d4a3ull;
    constexpr uint64_t secret3 = 0x4d5a2da51de1aa47ull;
    const uint8_t*     p       = (const uint8_t*)key;
    uint64_t           seed    = 0xca813bf4c7abf0a9ull;  // seed ^= wymix(seed ^ secret0, secret1);  with fixed seed = 0
    uint64_t           a = 0, b = 0;

    if (BS_LIKELY(len <= 16)) {
        if (BS_LIKELY(len >= 4)) {
            a = (wyr4(p) << 32) | wyr4(p + ((len >> 3) << 2));
            b = (wyr4(p + len - 4) << 32) | wyr4(p + len - 4 - ((len >> 3) << 2));
        } else if (BS_LIKELY(len > 0)) {
            a = wyr3(p, len);
            b = 0;
        } else {
            a = b = 0;
        }
    } else {
        size_t i = len;
        if (BS_UNLIKELY(i >= 48)) {
            uint64_t see1 = seed, see2 = seed;
            do {
                seed = wymix(wyr8(p) ^ secret1, wyr8(p + 8) ^ seed);
                see1 = wymix(wyr8(p + 16) ^ secret2, wyr8(p + 24) ^ see1);
                see2 = wymix(wyr8(p + 32) ^ secret3, wyr8(p + 40) ^ see2);
                p += 48;
                i -= 48;
            } while (BS_LIKELY(i >= 48));
            seed ^= see1 ^ see2;
        }
        while (BS_UNLIKELY(i > 16)) {
            seed = wymix(wyr8(p) ^ secret1, wyr8(p + 8) ^ seed);
            i -= 16;
            p += 16;
        }
        a = wyr8(p + i - 16);
        b = wyr8(p + i - 8);
    }
    a ^= secret1;
    b ^= seed;
    wymum(&a, &b);
    return wymix(a ^ secret0 ^ len, b ^ secret1);
}

};  // namespace bs
