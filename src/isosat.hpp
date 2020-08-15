#pragma once

#include "lattice.hpp"
#include "maths.hpp"

void get_all_implicants(const int* truthtab, int* output, int depth) {

    if (depth == 0) {
        output[0] = (truthtab[0] == 0);
        return;
    }

    int twon = 1;
    int threen = 1;

    for (int i = 1; i < depth; i++) { twon *= 2; threen *= 3; }

    for (int i = 0; i < 2; i++) {
        get_all_implicants(truthtab + twon*i, output + threen*i, depth-1);
    }

    for (int i = 0; i < threen; i++) {
        output[i + threen*2] = output[i] & output[i + threen];
    }

}

void only_prime_implicants(const int* implicants, int* output, int depth) {

    if (depth == 0) {
        output[0] = implicants[0];
        return;
    }

    int threen = 1;
    for (int i = 1; i < depth; i++) { threen *= 3; }

    for (int i = 0; i < 3; i++) {
        only_prime_implicants(implicants + threen*i, output + threen*i, depth-1);
    }

    for (int i = 0; i < threen; i++) {
        if (implicants[i + threen*2]) {
            output[i + threen] = 0;
            output[i] = 0;
        }
    }
}

void extract_prime_implicants(const int* prime_implicants, std::vector<int> &output, int depth, int mask=0) {

    if (depth == 0) {
        if (prime_implicants[0]) {
            output.push_back(mask);
        }
        return;
    }

    int threen = 1;
    for (int i = 1; i < depth; i++) { threen *= 3; }

    for (int i = 0; i < 3; i++) {
        extract_prime_implicants(prime_implicants + threen*i, output, depth-1, mask | ((2-i) << ((depth-1)*2)));
    }

}

std::vector<int> get_prime_implicants(const int* truthtab, int depth) {

    int threen = 1;
    for (int i = 0; i < depth; i++) { threen *= 3; }

    std::vector<int> all_implicants(threen);
    std::vector<int> prime_implicants(threen);

    get_all_implicants(truthtab, all_implicants.data(), depth);
    only_prime_implicants(all_implicants.data(), prime_implicants.data(), depth);

    std::vector<int> result;

    extract_prime_implicants(prime_implicants.data(), result, depth);

    return result;
}

std::vector<int> truth_table_to_prime_implicants(std::vector<int> &truth_table) {

    int truthtab[1024];

    for (int i = 0; i < 512; i++) {
        int islive = truth_table[i];
        truthtab[i] = 1 - islive;
        truthtab[i + 512] = islive;
    }

    return get_prime_implicants(truthtab, 10);
}


struct SubProblem {

    std::vector<int> cnf;
    int fullwidth;
    int fullheight;
    int hdiam;

    SubProblem(int w, int h, int d) : fullwidth(w), fullheight(h), hdiam(d) { }

    int coords2var(int x, int y) const {

        return x + y * fullwidth + 1;

    }

    /**
     * Append clauses to a CNF corresponding to a napkin.
     */
    void include_pi(const std::vector<int> &prime_implicants, const std::vector<int> &coords) {

        for (auto&& x : prime_implicants) {
            for (size_t i = 0; i < coords.size(); i += 2) {
                int v = (x >> i) & 3;
                int var = coords2var(coords[i], coords[i+1]);

                if (v == 1) {
                    cnf.push_back(-var);
                } else if (v == 2) {
                    cnf.push_back(var);
                }
            }
            cnf.push_back(0);
        }
    }

    void identify_vars(int i, int j) {

        if (i == j) { return; }

        cnf.push_back(i);
        cnf.push_back(-j);
        cnf.push_back(0);

        cnf.push_back(-i);
        cnf.push_back(j);
        cnf.push_back(0);

    }

    void symmetrise() {
        for (int j = 0; j < fullheight; j++) {
            for (int i = hdiam; i < fullwidth / 2; i++) {
                identify_vars(coords2var(i, j), coords2var(i, fullwidth - 1 - j));
            }
        }
    }

    void set_state(int x, int y, int s) {
        int i = coords2var(x, y);
        cnf.push_back(s ? i : -i);
        cnf.push_back(0);
    }

    void gutterise() {
        for (int j = 0; j < fullheight; j++) {
            set_state(fullwidth / 2, j, 0);
        }
    }
};


struct MetaProblem {

    u64seq initial_rows;
    Velocity vel;
    int middle_bits;
    bool symmetric;
    bool gutter_symmetric;

    MetaProblem(const u64seq &initial_rows, const Velocity &vel) : initial_rows(initial_rows), vel(vel) {

        uint64_t shadow = 0;
        for (auto&& x : initial_rows) { shadow |= x; }

        middle_bits = floor_log2(1 | (shadow << 1));

        symmetric = (vel.hd == 0);

        if (symmetric && shadow) {
            for (auto&& x : initial_rows) {
                symmetric = symmetric && (x == (uint64_reverse(x) >> (64 - middle_bits)));
            }
        }

        gutter_symmetric = symmetric && (((shadow >> (middle_bits >> 1)) & 1) == 0);
    }

    SubProblem get_instance(const std::vector<int> &prime_implicants, int lpad, int rpad, int lookahead) {

        int hradius = vel.hradius();
        int vradius = vel.vradius();

        int ir = initial_rows.size();
        int fullheight = lookahead + ir;
        int hdiam = hradius * 2;
        int fullwidth = hdiam * 2 + lpad + rpad + middle_bits;

        std::vector<int> vars;

        SubProblem sp(fullwidth, fullheight, hdiam);

        auto& jacobian = vel.jacobian;

        for (int j = 0; j < fullheight; j += 1) {
            for (int i = 0; i < fullwidth; i += 1) {

                if ((i < hdiam) || (i >= fullwidth - hdiam)) {
                    sp.set_state(i, j, 0);
                } else if (j < ir) {
                    int s = 0;
                    if ((i >= hdiam + lpad) && (i < hdiam + lpad + middle_bits)) {
                        s = 1 & (initial_rows[j] >> (i - hdiam - lpad));
                    }
                    sp.set_state(i, j, s);
                }

                if ((i < hradius) || (i >= fullwidth - hradius) || (j < vradius) || (j >= fullheight - vradius)) { continue; }

                vars.clear();

                for (int l = -1; l <= 1; l++) {
                    for (int k = -1; k <= 1; k++) {
                        vars.push_back(i + jacobian[0] * k + jacobian[2] * l);
                        vars.push_back(j + jacobian[1] * k + jacobian[3] * l);
                    }
                }

                vars.push_back(i + jacobian[4]);
                vars.push_back(j + jacobian[5]);
                sp.include_pi(prime_implicants, vars);
            }
        }

        return sp;
    }
};

