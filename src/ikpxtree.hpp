#pragma once

#include <map>
#include "gollat.hpp"
#include <stdio.h>

struct predstruct {

    uint64_t prevrow;
    uint32_t depth; // if zero, then no previous row
    int16_t shiftamt;
    uint16_t exhausted_width;

};

static_assert(sizeof(predstruct) == 16, "predstruct should be 16 bytes");

struct ikpxtree {

    int N;
    std::map<u64seq, predstruct> preds;

    ikpxtree(int tuple_length) : N(tuple_length) { }

    void write_to_file(FILE* fptr) {

        uint64_t header[2];
        header[0] = N;
        header[1] = preds.size();
        fwrite(header, 8, 2, fptr);

        for (auto it = preds.begin(); it != preds.end(); ++it) {
            fwrite(&(it->second), 16, 1, fptr);
            fwrite(it->first.data(), 8, N, fptr);
        }
    }

    void read_from_file(FILE* fptr) {

        uint64_t header[2];
        fread(header, 8, 2, fptr);
        N = header[0];

        for (uint64_t i = 0; i < header[1]; i++) {
            u64seq u(N);
            predstruct ps;
            fread(&ps, 16, 1, fptr);
            fread(u.data(), 8, N, fptr);
            preds[u] = ps;
        }
    }

    template<typename T>
    int v2shift(const uint64_t __restrict__ *seq, T &output) const {

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

    u64seq inject(const uint64_t *fullseq) {

        u64seq elem_upper(N); int upper = v2shift(fullseq, elem_upper);
        u64seq elem_lower(N); int lower = v2shift(fullseq + 1, elem_lower);

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
            preds[elem_lower] = ps;
        }

        // at this point, elem_lower is either empty or in the std::map
        return elem_lower;
    }

    void inject_base_element() {

        u64seq base_element(N+1, 0);

        for (int i = 0; i <= 30; i++) {
            base_element[N] = 1 + (1 << i);
            inject(base_element.data());
        }
    }

    apg::pattern materialise(lab32_t *lab, const uint64_t *lastelem) const {

        u64seq elem(N);
        for (int i = 0; i < N; i++) { elem[i] = lastelem[i]; }

        std::string rule = apg::get_all_rules()[0];

        apg::pattern res(lab, "", rule);
        apg::pattern cell(lab, "o!", rule);

        int y = 0;
        int x = 0;

        while (true) {
            auto it = preds.find(elem);
            if ((it == preds.end()) || (it->second.depth == 0)) {
                // reached the end:
                break;
            }

            int16_t shiftamt = it->second.shiftamt;

            for (int i = 0; i < 64; i++) {
                if ((elem[N-1] >> i) & 1) {
                    res += cell(x + i, y);
                }
            }

            for (int i = N-1; i >= 1; i--) {
                uint64_t row = elem[i-1];

                if (shiftamt > 0) { row <<=  shiftamt; }
                if (shiftamt < 0) { row >>= -shiftamt; }

                elem[i] = row;
            }

            elem[0] = it->second.prevrow;
            x -= shiftamt;
            y -= 1;
        }

        for (int j = N-1; j >= 0; j--) {
            for (int i = 0; i < 64; i++) {
                if ((elem[j] >> i) & 1) {
                    res += cell(x + i, y);
                }
            }
            y -= 1;
        }

        return res;
    }

};
