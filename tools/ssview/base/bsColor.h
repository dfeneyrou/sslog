#pragma once

#include "bs.h"

using bsColor_t = uint32_t;  // 0xAABBGGRR, directly a GL color

// Constants
#define COLOR_TRANSPARENT 0x00000000
#define COLOR_BLACK       0xFF000000
#define COLOR_GREY1       0xFF101010
#define COLOR_GREY2       0xFF202020
#define COLOR_GREY3       0xFF303030
#define COLOR_GREY4       0xFF404040
#define COLOR_GREY5       0xFF505050
#define COLOR_GREY6       0xFF606060
#define COLOR_GREY8       0xFF808080
#define COLOR_GREY10      0xFFA0A0A0
#define COLOR_GREY12      0xFFC0C0C0
#define COLOR_WHITE       0xFFFFFFFF
#define COLOR_ORANGE      0xFF00A0FF
#define COLOR_RED         0xFF0000FF
#define COLOR_GREEN       0xFF00A000
#define COLOR_LEMON       0xFF80FF00
#define COLOR_BLUE        0xFFFF0000
#define COLOR_SHADOW      0x80000000  // Half transparent black

inline bsColor_t
bsLerp(float ratioA, bsColor_t a, bsColor_t b)
{
    uint64_t ratio = (uint64_t)(255. * ratioA);
    return (((ratio * (((uint64_t)a) & (uint64_t)0xFF000000) + (255 - ratio) * (((uint64_t)b) & (uint64_t)0xFF000000)) >> 8) & 0xFF000000) |
           (((ratio * (((uint64_t)a) & (uint64_t)0x00FF0000) + (255 - ratio) * (((uint64_t)b) & (uint64_t)0x00FF0000)) >> 8) & 0x00FF0000) |
           (((ratio * (((uint64_t)a) & (uint64_t)0x0000FF00) + (255 - ratio) * (((uint64_t)b) & (uint64_t)0x0000FF00)) >> 8) & 0x0000FF00) |
           (((ratio * (((uint64_t)a) & (uint64_t)0x000000FF) + (255 - ratio) * (((uint64_t)b) & (uint64_t)0x000000FF)) >> 8) & 0x000000FF);
}

// Hue, Saturation and Value in  [0-256[ . Alpha too.
inline bsColor_t
bsFromHsv(int h, int s, int v, int alpha)
{
    float C = float(v * s) / (float)(255 * 255);
    float X = C * (1. - fabs(fmod(((float)h) / (255. / 6.), 2.) - 1.));
    float m = ((float)v) / 255. - C;
    // clang-format off
    if      (h<42)  { return (alpha<<24) | (((int)(255.*    m))<<16) | (((int)(255.*(X+m)))<<8) | (((int)(255.*(C+m)))<<0); }
    else if (h<85)  { return (alpha<<24) | (((int)(255.*    m))<<16) | (((int)(255.*(C+m)))<<8) | (((int)(255.*(X+m)))<<0); }
    else if (h<127) { return (alpha<<24) | (((int)(255.*(X+m)))<<16) | (((int)(255.*(C+m)))<<8) | (((int)(255.*    m))<<0); }
    else if (h<170) { return (alpha<<24) | (((int)(255.*(C+m)))<<16) | (((int)(255.*(X+m)))<<8) | (((int)(255.*    m))<<0); }
    else if (h<212) { return (alpha<<24) | (((int)(255.*(C+m)))<<16) | (((int)(255.*    m))<<8) | (((int)(255.*(X+m)))<<0); }
    else            { return (alpha<<24) | (((int)(255.*(X+m)))<<16) | (((int)(255.*    m))<<8) | (((int)(255.*(C+m)))<<0); }
    // clang-format on
}

inline bsColor_t
bsForceAlpha(bsColor_t c, uint8_t alpha)
{
    return (c & 0x00FFFFFF) | (((uint32_t)alpha) << 24);
}

inline bsColor_t
bsMultAlpha(bsColor_t c, float alpha)
{
    alpha *= (float)((c >> 24) & 0xFF);
    return (c & 0x00FFFFFF) | (bsMinMax((uint32_t)alpha, (uint32_t)0, (uint32_t)255) << 24);
}

inline bsColor_t
bsLuminosity(bsColor_t c, int increment, int incrementAlpha = 0)
{
    int r = bsMinMax((int)((c >> 0) & 0xFF) + increment, 0, 255);
    int g = bsMinMax((int)((c >> 8) & 0xFF) + increment, 0, 255);
    int b = bsMinMax((int)((c >> 16) & 0xFF) + increment, 0, 255);
    int a = bsMinMax((int)((c >> 24) & 0xFF) + incrementAlpha, 0, 255);
    return (a << 24) | (b << 16) | (g << 8) | r;
}
