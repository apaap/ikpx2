#pragma once

#include <map>
#include "gollat.hpp"
#include "maths.hpp"
#include <stdio.h>

struct predstruct {

    uint128_t prevrow;
    uint32_t depth; // if zero, then no previous row
    int16_t shiftamt;
    uint16_t exhausted_width;

};

// With 64bit rows, predstruct is 16 bytes. With 128bit rows predstruct
// requires 24 bytes, but compiler actually allocates 32 bytes.
// This suggests 128bit alignment instead of 64bit alignment.
//static_assert(sizeof(predstruct) == 24, "predstruct should be 24 bytes");


typedef std::map<u128seq, predstruct> ikpx_map;


struct ikpxtree {

    int N;
    ikpx_map preds;

    ikpxtree(int tuple_length) : N(tuple_length) { }

    void write_to_file(FILE* fptr) {

        uint64_t header[2];
        header[0] = N;
        header[1] = preds.size();
        fwrite(header, 8, 2, fptr);

        for (auto it = preds.begin(); it != preds.end(); ++it) {
            fwrite(&(it->second), 24, 1, fptr);
            fwrite(it->first.data(), 8, N, fptr);
        }
    }

    void read_from_file(FILE* fptr) {

        uint64_t header[2];
        fread(header, 8, 2, fptr);
        N = header[0];

        for (uint64_t i = 0; i < header[1]; i++) {

            if ((i > 0) && ((i & 0x3fffff) == 0)) {
                std::cout << "# loaded " << i << " of " << header[1] << " nodes." << std::endl;
            }

            u128seq u(N);
            predstruct ps;
            fread(&ps, 24, 1, fptr);
            fread(u.data(), 8, N, fptr);
            preds[u] = ps;
        }
    }

    template<typename T>
    int v2shift(const uint128_t* __restrict__ seq, T &output) const {

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
        v2 += shadow64 ? __builtin_ctzll(shadow64) : 64;

        for (int i = 0; i < N; i++) {
            output[i] = seq[i] >> v2;
        }

        return shadow ? v2 + 1 : 0;
    }

    u128seq inject(const uint128_t *fullseq, uint32_t minheight) {

        u128seq elem_upper(N); int upper = v2shift(fullseq, elem_upper);
        u128seq elem_lower(N); int lower = v2shift(fullseq + 1, elem_lower);

        predstruct ps;
        ps.prevrow = 0; ps.depth = 0; ps.shiftamt = 0; ps.exhausted_width = 0;

        if (lower == 0) {
            elem_lower.clear();
        } else if (preds.find(elem_lower) == preds.end()) {
            if (upper) {
                ps.shiftamt = lower - upper;
                ps.prevrow = elem_upper[0];
                auto it = preds.find(elem_upper);
                if (it != preds.end()) {
                    ps.depth = it->second.depth + 1;
                }
            }
            if (ps.depth >= minheight) {
                preds[elem_lower] = ps;
            } else {
                elem_lower.clear();
            }
        }

        // at this point, elem_lower is either empty or in the std::map
        return elem_lower;
    }

    void inject_base_element() {

        u128seq elem_zero(N);
        predstruct ps;
        ps.prevrow = 0; ps.depth = 0; ps.shiftamt = 0; ps.exhausted_width = 0;

        preds[elem_zero] = ps;
    }

    int16_t inplace_parent(u128seq &elem, const ikpx_map::const_iterator &it) const {

        int16_t shiftamt = it->second.shiftamt;

        for (int i = N-1; i >= 1; i--) {
            uint64_t row = elem[i-1];

            if (shiftamt > 0) { row <<=  shiftamt; }
            if (shiftamt < 0) { row >>= -shiftamt; }

            elem[i] = row;
        }

        elem[0] = it->second.prevrow;

        return shiftamt;
    }

    bool is_subsumed(const ikpx_map::const_iterator &it, int currwidth) const {

        u128seq elem = it->first;

        if (it->second.prevrow) {
            if (it->second.shiftamt > 0) { return false; }
            uint128_t shadow = 0;

            for (int i = 0; i < N; i++) { shadow |= elem[i]; }

            shadow >>= -(it->second.shiftamt);
            if ((shadow == 0) || (floor_log2(shadow) < floor_log2(it->second.prevrow))) {
                return false;
            }
        }

        inplace_parent(elem, it);

        auto it2 = preds.find(elem);

        if ((it2 == preds.end()) || (it2->second.depth == 0)) {
            return false;
        }

        return (it2->second.exhausted_width >= currwidth + 0x4000);
    }

    uint32_t preempt_depth(const u128seq &elements) const {

        u128seq elem(N);
        v2shift(elements.data(), elem);
        auto it = preds.find(elem);
        if (it == preds.end()) { return 0; }
        return it->second.depth;

    }

    apg::pattern materialise(lab32_t *lab, const u128seq &elements) const {

        u128seq elem(N);
        int x = v2shift(elements.data(), elem) - 1;

        std::string rule = apg::get_all_rules()[0];

        apg::pattern res(lab, "", rule);
        apg::pattern cell(lab, "o!", rule);

        for (int j = N-1; j < ((int) elements.size()); j++) {
            for (int i = 0; i < 128; i++) {
                if ((elements[j] >> i) & 1) {
                    res += cell(i, j-N+1);
                }
            }
        }

        int y = 0;

        while (true) {
            auto it = preds.find(elem);
            if ((it == preds.end()) || (it->second.depth == 0)) {
                // reached the end:
                break;
            }

            for (int i = 0; i < 128; i++) {
                if ((elem[N-1] >> i) & 1) {
                    res += cell(x + i, y);
                }
            }

            auto shiftamt = inplace_parent(elem, it);

            x -= shiftamt;
            y -= 1;
        }

        for (int j = N-1; j >= 0; j--) {
            for (int i = 0; i < 128; i++) {
                if ((elem[j] >> i) & 1) {
                    res += cell(x + i, y);
                }
            }
            y -= 1;
        }

        return res;
    }

};
