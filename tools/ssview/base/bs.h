#pragma once

// Internal
#include "asserted.h"

// System
#include <climits>
#include <cmath>
#include <cstdint>

// Useful types
typedef int64_t           bsUs_t;
inline constexpr uint32_t InvalidU32 = 0xFFFFFFFF;
inline constexpr uint64_t InvalidU64 = 0xFFFFFFFFFFFFFFFF;

// Macros for likely and unlikely branching
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
#define BS_LIKELY(x)   __builtin_expect(!!(x), 1)
#define BS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define BS_LIKELY(x)   (x)
#define BS_UNLIKELY(x) (x)
#endif

// Utils
template<class T>
T
bsAbs(T a)
{
    return (a >= (T)0) ? a : -a;
}
template<class T>
T
bsRound(T a)
{
    return (a >= 0.) ? (a + 0.5) : -(0.5 - a);
}
template<class T1, class T2>
T1
bsMax(T1 a, T2 b)
{
    return (T1)(a > b ? a : b);
}
template<class T1, class T2>
T1
bsMin(T1 a, T2 b)
{
    return (T1)(a < b ? a : b);
}
template<class T>
T
bsMinMax(T v, T min, T max)
{
    return v <= min ? min : ((v >= max) ? max : v);
}
template<class T>
int
bsSign(T a)
{
    return a >= (T)0 ? 1 : -1;
}
template<class T>
T
bsSquare(T a)
{
    return a * a;
}
template<class T>
void
bsSwap(T& a, T& b)
{
    T tmp = b;
    b     = a;
    a     = tmp;
}
inline int
bsDivCeil(int num, int denum)
{
    return (num + denum - 1) / denum;
}
