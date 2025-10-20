#pragma once

#include "vec.hpp"
#include "tigr.h"

inline uint16_t colorTo16b(TPixel c24b) {
    uint16_t r = (c24b.r >> 3) & 0x1F;  // 5 bits for red
    uint16_t g = (c24b.g >> 2) & 0x3F;  // 6 bits for green
    uint16_t b = (c24b.b >> 3) & 0x1F;  // 5 bits for blue
    return (r << 11) | (g << 5) | b;
}

inline TPixel colorTo24b(uint16_t c16b) {
    unsigned char r = (c16b >> 11) & 0x1F; // 5 bits for red
    unsigned char g = (c16b >>  5) & 0x3F; // 6 bits for green
    unsigned char b = c16b & 0x1F; // 5 bits for blue
    r <<= 3;
    g <<= 2;
    b <<= 3;
    return {r,g,b,255};
}