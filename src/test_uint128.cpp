//g++ -O1 -g -std=c++20 -o test_uint128 test_uint128.cpp

#include <vector>
#include <stdint.h>
#include <iostream>
#include <inttypes.h>

typedef unsigned __int128 uint128_t;

typedef std::vector<uint64_t> u64seq;
typedef std::vector<uint128_t> u128seq;

int N = 4;

template<typename T>
int v2shift(const uint64_t* seq, T &output) {

    uint64_t shadow = 0;
    for (int i = 0; i < N; i++) {
        shadow |= seq[i];
    }

    int v2 = shadow ? __builtin_ctzll(shadow) : 0;

    for (int i = 0; i < N; i++) {
        output[i] = seq[i] >> v2;
    }

    return shadow ? v2 + 1 : 0;
}


template<typename T>
int v2shift(const uint128_t* seq, T &output) {

    uint128_t shadow = 0;
    for (int i = 0; i < N; i++) {
        shadow |= seq[i];
    }
    
    int v2 = 0;
    uint64_t shadow64 = shadow;
    if (shadow64 == 0) {
        v2 = 64;
        shadow64 = shadow >> 64;
    }
    v2 += shadow64 ? __builtin_ctzll(shadow64) : 0;

    for (int i = 0; i < N; i++) {
        output[i] = seq[i] >> v2;
    }

    return shadow ? v2 + 1 : 0;
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Please supply a numerical argument." << std::endl;
        return 1;
    }
    int res = 0;
    
    uint64_t v1 = std::stoll(argv[1]);
    
    u64seq v1seq(N);
    u64seq v2seq(N);
    for(int i = 0; i < N; i++) {
        v1seq[i] = v1 << i;
        v2seq[i] = 0;
    }

    std::cout << "v1 = " << v1 << ", v1seq[0] = " << v1seq[0];
    std::cout << ", v1seq[3] = " << v1seq[3] <<  std::endl;
    printf("v1 = %016" PRIX64 ", v1seq[0] = %016" PRIX64, v1, v1seq[0]);
    printf(", v1seq[3] = %016" PRIX64"\n", v1seq[3]);
    res = v2shift(&(v1seq[0]), v2seq);
    std::cout << "v2shift = " << res << std::endl;
    std::cout << "v2seq[0] = " << v2seq[0] << std::endl;
    std::cout << "v2seq[3] = " << v2seq[3] << std::endl;
    
    uint128_t vv1 = v1;
    u128seq vv1seq(N);
    u128seq vv2seq(N);
    for(int i = 0; i < N; i++) {
        vv1seq[i] = vv1 << i;
        vv2seq[i] = 0;
    }
    
    printf("vv1 = %016" PRIX64 "%016" PRIX64 "\n", (uint64_t)(vv1>>64), (uint64_t)vv1);
    printf("vv1seq[0] = %016" PRIX64 "%016" PRIX64 "\n", (uint64_t)(vv1seq[0]>>64), (uint64_t)vv1seq[0]);
    printf("vv1seq[3] = %016" PRIX64 "%016" PRIX64 "\n", (uint64_t)(vv1seq[3]>>64), (uint64_t)vv1seq[3]);
    res = v2shift(&(vv1seq[0]), vv2seq);
    std::cout << "vv2shift = " << res << std::endl;
    printf("vv2seq[0] = %016" PRIX64 "%016" PRIX64 "\n", (uint64_t)(vv2seq[0]>>64), (uint64_t)vv2seq[0]);
    printf("vv2seq[3] = %016" PRIX64 "%016" PRIX64 "\n", (uint64_t)(vv2seq[3]>>64), (uint64_t)vv2seq[3]);
    
    vv1 = vv1<<64;
    for(int i = 0; i < N; i++) {
        vv1seq[i] = vv1 << i;
        vv2seq[i] = 0;
    }
    
    printf("vv1 = %016" PRIX64 "%016" PRIX64 "\n", (uint64_t)(vv1>>64), (uint64_t)vv1);
    printf("vv1seq[0] = %016" PRIX64 "%016" PRIX64 "\n", (uint64_t)(vv1seq[0]>>64), (uint64_t)vv1seq[0]);
    printf("vv1seq[3] = %016" PRIX64 "%016" PRIX64 "\n", (uint64_t)(vv1seq[3]>>64), (uint64_t)vv1seq[3]);
    res = v2shift(&(vv1seq[0]), vv2seq);
    std::cout << "vv2shift = " << res << std::endl;
    printf("vv2seq[0] = %016" PRIX64 "%016" PRIX64 "\n", (uint64_t)(vv2seq[0]>>64), (uint64_t)vv2seq[0]);
    printf("vv2seq[3] = %016" PRIX64 "%016" PRIX64 "\n", (uint64_t)(vv2seq[3]>>64), (uint64_t)vv2seq[3]);
    
    return 0;
}
