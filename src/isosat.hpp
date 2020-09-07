#pragma once

#include "lattice.hpp"
#include "maths.hpp"
#include "solver.hpp"

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
    int vdiam;
    std::vector<int> polarity;
    bool impossible;

    SubProblem(int w, int h, int d, int e) :
        fullwidth(w), fullheight(h), hdiam(d), vdiam(e), polarity(w * h + 1), impossible(false) { }

    int coords2var(int x, int y) const {

        return x + y * fullwidth + 1;

    }

    u64seq sol2res(const std::vector<int> &solution) const {

        u64seq res;

        if (solution[0] != 10) { return res; }

        for (int j = 0; j < fullheight; j++) {
            uint64_t x = 0;
            for (int i = hdiam; i < fullwidth - hdiam; i++) {
                if (solution[coords2var(i, j)] > 0) {
                    x |= (1ull << (i - hdiam));
                }
            }
            res.push_back(x);
        }

        return res;
    }

    u64seq solve() const {

        if (impossible) { u64seq x; return x; }

        auto solution = solve_using_kissat(cnf, fullwidth * fullheight);
        return sol2res(solution);
    }

    template<typename Fn>
    int find_all_solutions(Fn lambda) {

        if (impossible) { return 20; }

        std::vector<int> unique_literals;
        for (int i = 0; i < fullwidth; i++) {
            unique_literals.push_back(coords2var(i, vdiam));
        }

        return multisolve(cnf, unique_literals, fullwidth * fullheight, [&](std::vector<int> &solution) {

            lambda(sol2res(solution));

        });
    }

    /**
     * Append clauses to a CNF corresponding to a napkin.
     */
    void include_pi(const std::vector<int> &prime_implicants, const std::vector<int> &coords) {

        if (impossible) { return; }

        int vars[10];
        int simp_mask = 0;

        for (size_t i = 0; i < 10; i++) {
            int var = coords2var(coords[2*i], coords[2*i+1]);
            vars[i] = var;

            int v = 0;
            if (polarity[var] == -1) { v = 1; }
            if (polarity[var] ==  1) { v = 2; }

            simp_mask |= (v << (i*2));
        }

        int simp_mask2 = ((simp_mask & 0x55555) << 1) | ((simp_mask & 0xaaaaa) >> 1);

        for (auto&& x : prime_implicants) {

            int y = x &~ simp_mask2; // unit propagation
            if (y == 0) { impossible = true; return; }
            if (x & simp_mask) { continue; /* clause already true */ }

            for (size_t i = 0; i < 10; i += 1) {
                int v = (y >> (i*2)) & 3;

                if (v == 1) {
                    cnf.push_back(-vars[i]);
                } else if (v == 2) {
                    cnf.push_back(vars[i]);
                }
            }

            if ((y & (y - 1)) == 0) {
                // implied unit clause
                int var = cnf.back();
                if (var < 0) {
                    polarity[-var] = -1;
                } else {
                    polarity[var] = 1;
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
                identify_vars(coords2var(i, j), coords2var(fullwidth - 1 - i, j));
            }
        }
    }

    void zerolast() {
        for (int j = fullheight - vdiam; j < fullheight; j++) {
            for (int i = hdiam; i < fullwidth - hdiam; i++) {
                set_state(i, j, 0);
            }
        }
    }

    void set_state(int x, int y, int s) {
        int i = coords2var(x, y);
        cnf.push_back(s ? i : -i);
        cnf.push_back(0);
        polarity[i] = (s ? 1 : -1);
    }

    void gutterise() {
        for (int j = 0; j < fullheight; j++) {
            set_state(fullwidth / 2, j, 0);
        }
    }

    void clear_sides() {
        for (int j = 0; j < fullheight; j += 1) {
            for (int i = 0; i < hdiam; i += 1) {
                set_state(i, j, 0);
                set_state(fullwidth - i - 1, j, 0);
            }
        }
    }

    void input_row(uint64_t row, int j, int lpad, int middle_bits) {

        for (int i = hdiam; i < fullwidth - hdiam; i += 1) {

            int s = 0;
            if ((i >= hdiam + lpad) && (i < hdiam + lpad + middle_bits)) {
                s = 1 & (row >> (i - hdiam - lpad));
            }
            set_state(i, j, s);
        }
    }
};


struct MetaProblem {

    // input parameters:
    u64seq initial_rows;
    Velocity vel;
    bool reverse;

    // derived parameters:
    bool symmetric;
    bool gutter_symmetric;
    int middle_bits;

    MetaProblem(const u64seq &initial_rows, const Velocity &vel, bool reverse=false) :
        initial_rows(initial_rows), vel(vel), reverse(reverse) {

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

        SubProblem sp(fullwidth, fullheight, hdiam, vradius * 2);

        int jacobian[6];

        for (int i = 0; i < 6; i += 2) {
            jacobian[i] = vel.jacobian[i];
            jacobian[i+1] = (reverse ? -1 : 1) * vel.jacobian[i+1];
        }

        sp.clear_sides();
        for (int j = 0; j < ir; j++) {
            sp.input_row(initial_rows[j], j, lpad, middle_bits);
        }

        for (int j = vradius; j < fullheight - vradius; j += 1) {
            for (int i = hradius; i < fullwidth - hradius; i += 1) {

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

    template<typename Fn, typename UsatFn>
    void find_multiple_solutions(SubProblem &sp, bool try_completion, Fn lambda, UsatFn lambda2) {

        int solutions = 0;
        uint64_t ext = 0;
        int res = sp.find_all_solutions([&](const u64seq &sol) {
            solutions += 1;
            lambda(sol);
            ext = sol[initial_rows.size()];
        });

        if ((solutions == 0) && (res != 0)) {
            lambda2();
        } else if (try_completion) {
            sp.zerolast();
            if ((solutions == 1) && (res != 0)) {
                // unique extension
                sp.input_row(ext, initial_rows.size(), 0, sp.fullwidth - 2 * sp.hdiam);
            }
            u64seq zres = sp.solve();
            if (zres.size()) { lambda(zres); }
        }

    }

    template<typename Fn>
    int find_all_solutions(int max_width, int exhausted_width, const std::vector<int> &prime_implicants, int lookahead, Fn lambda) {

        uint64_t rproblems = 0;
        int subproblems = 0;

        for (int lpad = 0; lpad <= max_width - middle_bits; lpad++) {
            rproblems |= (1ull << lpad);
        }

        bool try_gutter = gutter_symmetric;
        bool try_symmetric = symmetric;

        for (int w = max_width; w > exhausted_width; w--) {

            if (w > 60) {
                // the maximum asymmetric search width is 60:
                continue;
            }

            // asymmetric subproblems:
            for (int lpad = 0; lpad <= w - middle_bits; lpad++) {

                if (((rproblems >> lpad) & 1) == 0) { continue; }

                int rpad = w - middle_bits - lpad;
                auto sp = get_instance(prime_implicants, lpad, rpad, lookahead);

                find_multiple_solutions(sp, w == max_width, lambda, [&](){
                    rproblems ^= (1ull << lpad);
                });

                subproblems += 1;
            }

            rproblems &= (rproblems >> 1);

            if (w > 30) {
                // the maximum symmetric search width is 30:
                continue;
            }

            // gutter-symmetric subproblems:
            if (try_gutter) {

                int lpad = w - ((middle_bits - 1) >> 1);

                if (lpad >= 0) {
                    auto sp = get_instance(prime_implicants, lpad, lpad, lookahead);
                    sp.symmetrise();
                    sp.gutterise();

                    find_multiple_solutions(sp, w == max_width, lambda, [&](){
                        try_gutter = false;
                    });

                    subproblems += 1;
                }
            }

            // symmetric subproblems:
            if (try_symmetric) {

                int lpad = w - ((middle_bits + 1) >> 1);

                if (lpad >= 0) {
                    auto sp = get_instance(prime_implicants, lpad, lpad, lookahead);
                    sp.symmetrise();

                    find_multiple_solutions(sp, w == max_width, lambda, [&](){
                        try_gutter = false;
                        try_symmetric = false;
                    });

                    subproblems += 1;
                }
            }
        }

        return subproblems;
    }
};

