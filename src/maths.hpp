#pragma once
#include <stdint.h>

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
