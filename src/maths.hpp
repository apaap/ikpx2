#pragma once

//#include "../apgmera/lifelib/hashtrees/numtheory.h"
#include <stdint.h>
#include <bit>
#include <map>

typedef unsigned __int128 uint128_t;

// Hash functor for uint128_t required by hash tables - std::unordered_set, etc.
template <>
struct std::hash<uint128_t> {
    
    std::size_t operator()(const uint128_t v) const {
        return std::hash<uint64_t>{}(v) ^ std::hash<uint64_t>{}(v >> 64);
    }
};

// Cantor's pairing function:
int cantor_pair(int a, int b) {

    return (((a + b) * (a + b + 1)) >> 1) + b;

}

// Reverse each byte in an integer:
uint64_t uint64_reverse(uint64_t x) {
    uint64_t y = (x >> 32) | (x << 32); 
    y = ((y & 0xffff0000ffff0000ull) >> 16) | ((y & 0x0000ffff0000ffffull) << 16);
    y = ((y & 0xff00ff00ff00ff00ull) >> 8)  | ((y & 0x00ff00ff00ff00ffull) << 8);
    y = ((y & 0xf0f0f0f0f0f0f0f0ull) >> 4)  | ((y & 0x0f0f0f0f0f0f0f0full) << 4);
    y = ((y & 0xccccccccccccccccull) >> 2)  | ((y & 0x3333333333333333ull) << 2);
    y = ((y & 0xaaaaaaaaaaaaaaaaull) >> 1)  | ((y & 0x5555555555555555ull) << 1);
    return y;
}

uint128_t uint128_reverse(uint128_t x) {
    uint64_t y1 = x;
    y1 = uint64_reverse(y1);
    uint64_t y2 = (x >> 64);
    y2 = uint64_reverse(y2);
    uint128_t y = ((uint128_t)y1 << 64) | y2; 
    return y;
}

/**
 * Algorithm B, discovered by Gerth Brodal in 1997.
 */
uint64_t floor_log2(uint64_t input) {

    uint64_t lambda = 0;
    uint64_t x = input;

    // B1 [Scale down.]
    if (x >= 0x100000000ull) { lambda += 32; x >>= 32; }
    if (x >= 0x10000ull) { lambda += 16; x >>= 16; }

    // B2 [Replicate.]
    x |= (x << 16);
    x |= (x << 32);

    // B3 [Change leading bits.]
    uint64_t y = x & 0xff00f0f0ccccaaaaull;

    // B4 [Compare all fields.]
    uint64_t h = 0x8000800080008000ull;
    uint64_t t = h & (y | ((y | h) - (x ^ y)));

    // B5 [Compress bits.]
    t += (t << 15);
    t += (t << 30);

    // B6 [Finish.]
    return lambda + (t >> 60);

}

// Can use compiler intrinsics (implentation dependent support) or
// std::bit_width (requires std=c++20)
// May be more or less efficient than floor_log2 algorithm above depending
// on hardware support for LZCNT (or CLZ) and compiler optimisation.
// Alternative: gcc provides __builtin_clzll which compiles to LZCNT when available
uint128_t floor_log2(uint128_t input) {
    
    uint128_t res = 0;
    uint64_t y = (input >> 64);
    if (y) { res = 64; }
    else { y = input; }
    res += std::bit_width(y) - 1;
    
    return res;
}
