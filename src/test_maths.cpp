//g++ -O1 -g -std=c++20 -o test_maths test_maths.cpp

#include <iostream>
#include <inttypes.h>
#include <unordered_set>

#include "maths.hpp"
#include "heap.hpp"

typedef std::vector<uint128_t> u128seq;

struct predstruct {

    uint128_t prevrow;
    uint32_t depth; // if zero, then no previous row
    int16_t shiftamt;
    uint16_t exhausted_width;

};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Please supply a numerical argument." << std::endl;
        return 1;
    }
    
    std::cout << "sizeof(predstruct) = " << sizeof(predstruct) << ", should be == 24." << std::endl;
    std::cout << "sizeof(uint64_t) = " << sizeof(uint64_t) << ", should be == 8." << std::endl;
    std::cout << "sizeof(uint128_t) = " << sizeof(uint128_t) << ", should be == 16." << std::endl;
    
    typedef std::map<u128seq, predstruct> ikpx_map;
    ikpx_map preds;
    triple_heap<ikpx_map::iterator> heap;
    std::unordered_set<uint128_t> already_seen;
    
    uint64_t v1 = std::stoll(argv[1]);
    uint64_t v2 = uint64_reverse(v1);
    
    std::cout << "v1 = " << v1 << ", v2 = " << v2 << std::endl;
    printf("v1 = %016" PRIX64 ", v2 = %016" PRIX64 "\n", v1, v2);
    
    uint128_t vv1 = v1;
    uint128_t vv2 = uint128_reverse(vv1);
    
    already_seen.insert(vv1);
    already_seen.insert(vv2);
    already_seen.count(vv1);

    //std::cout << "vv1 = " << std::format("{}", vv1) << ", vv2 = " << std::format("{}", vv2) << std::endl;
    printf("vv1 = %016" PRIX64 "%016" PRIX64 ", ",(uint64_t)(vv1>>64),(uint64_t)vv1);
    printf("vv2 = %016" PRIX64 "%016" PRIX64 "\n",(uint64_t)(vv2>>64),(uint64_t)vv2);
    
    
    uint64_t flv = floor_log2(v1);
    std::cout << "v = " << v1 << ", floor(log(v)) = " << flv << std::endl;
    
    uint128_t flvv = floor_log2(vv1);
    std::cout << "vv = " << (uint64_t)vv1 << ", floor(log(vv)) = " << (uint64_t)flvv;
    flvv = floor_log2(vv1 << 64);
    std::cout << ", floor(log(vv+2^64)) = " << (uint64_t)flvv << std::endl;
    
    return 0;
}
