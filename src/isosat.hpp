#pragma once

#include "lattice.hpp"
#include "maths.hpp"
#include "solver.hpp"

#include <unordered_map>
#include <chrono>
#include <atomic>
#include <set>
#include <iostream>


struct PreferredSolver {

    std::vector<std::atomic<uint64_t>> timings;

    PreferredSolver() : timings(1 + floor_log2(SOLVER_MASK)) { }

    double get_loss(int i) const {

        uint64_t f = timings[i];
        double loss = 0;
        if (f & 0xffff) {
            loss = (((double) (f >> 16))) / ((double) (f & 0xffff));
        }
        return loss;

    }

    int choose(const u64seq &seed) const {

        // create random seed:
        uint64_t h = 42;
        for (auto&& x : seed) {
            h = h * 6364136223846793005ull + x;
        }
        uint64_t xorshifted = ((h >> ((h >> 59u) + 5u)) ^ h);
        xorshifted *= 11400714819323198485ull;

        int idx = -1;
        double best = 0.0;

        // with probability 1/8, choose a random solver;
        // with probability 3/8, choose the best solver;
        // with probability 4/8, compromise between these:
        bool fully_random = (xorshifted < 0x2000000000000000ull);
        bool randomise    = (xorshifted < 0xa000000000000000ull);

        for (uint64_t i = 0; i < timings.size(); i++) {

            if ((SOLVER_MASK >> i) & 1) {

                xorshifted *= 11400714819323198485ull;

                double loss = 100.0;
                if (!fully_random) { loss += get_loss(i); }
                if (randomise) { loss *= ((double) xorshifted); }

                if ((idx == -1) || (loss < best)) {
                    idx = i;
                    best = loss;
                }
            }
        }

        return idx;
    }

    void update(int solver_idx, uint64_t micros) {

        timings[solver_idx] += (micros << 16) + 1;

        uint64_t u = timings[solver_idx];

        if (u & 0xff0000000000ff00ull) {
            // rescale every 128 iterations:
            timings[solver_idx] = (u >> 1) & 0x00ffffffffff00ffull;
        }
    }
};


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

/**
 * Memoized simplification and subsumption for collections of clauses
 * arising from a partially-determined napkin.
 */
std::vector<int>& simplify_and_subsume(int simp_mask, const std::vector<int> &prime_implicants, std::unordered_map<int, std::vector<int>> &memdict) {

    {
        auto it = memdict.find(simp_mask);
        if (it != memdict.end()) { return it->second; }
    }

    std::vector<int> &res = memdict[simp_mask];

    int simp_mask2 = ((simp_mask & 0x55555) << 1) | ((simp_mask & 0xaaaaa) >> 1);

    std::set<int> encountered;

    // make unique and order:
    for (auto&& x : prime_implicants) {
        int y = x &~ simp_mask2; // unit propagation
        if (x & simp_mask) { continue; /* clause already true */ }

        encountered.insert(y);
    }

    // subsume:
    for (auto&& x : encountered) {

        bool subsumed = false;
        for (auto&& y : res) {
            if ((x & y) == y) { subsumed = true; break; }
        }

        if (!subsumed) { res.push_back(x); }
    }

    return res;
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
    int find_all_solutions(Fn lambda, int solver_idx) {

        if (impossible) { return 20; }

        std::vector<int> unique_literals;
        for (int i = 0; i < fullwidth; i++) {
            unique_literals.push_back(coords2var(i, vdiam));
        }

        std::vector<int> zero_literals;
        for (int j = fullheight - vdiam; j < fullheight; j++) {
            for (int i = hdiam; i < fullwidth - hdiam; i++) {
                zero_literals.push_back(coords2var(i, j));
            }
        }

        return multisolve(cnf, solver_idx, unique_literals, zero_literals, fullwidth * fullheight, [&](std::vector<int> &solution) {

            lambda(sol2res(solution));

        });
    }

    void include_pi_inner(const std::vector<int> &prime_implicants, const int* vars) {

        if ((prime_implicants.size() == 1) && (prime_implicants[0] == 0)) {
            impossible = true;
            return;
        }

        for (auto&& y : prime_implicants) {

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

    /**
     * Append clauses to a CNF corresponding to a napkin.
     */
    void include_pi(const std::vector<int> &prime_implicants, const std::vector<int> &coords,
                    std::unordered_map<int, std::vector<int>> &memdict) {

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

        if (simp_mask) {
            include_pi_inner(simplify_and_subsume(simp_mask, prime_implicants, memdict), vars);
        } else {
            include_pi_inner(prime_implicants, vars);
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
    uint64_t shadow;

    MetaProblem(const u64seq &initial_rows, const Velocity &vel, bool reverse=false) :
        initial_rows(initial_rows), vel(vel), reverse(reverse) {

        shadow = 0;
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

    SubProblem get_instance(const std::vector<int> &prime_implicants, int lpad, int rpad, int lookahead,
                            bool symmetric, bool gutter, std::unordered_map<int, std::vector<int>> &memdict) {

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

        if (symmetric) { sp.symmetrise(); }
        if (gutter) { sp.gutterise(); }

        int maxx = symmetric ? ((fullwidth - 1) >> 1) : (fullwidth - hradius - 1);

        for (int j = vradius; j < fullheight - vradius; j += 1) {
            for (int i = hradius; i <= maxx; i += 1) {

                vars.clear();

                for (int l = -1; l <= 1; l++) {
                    for (int k = -1; k <= 1; k++) {
                        vars.push_back(i + jacobian[0] * k + jacobian[2] * l);
                        vars.push_back(j + jacobian[1] * k + jacobian[3] * l);
                    }
                }

                vars.push_back(i + jacobian[4]);
                vars.push_back(j + jacobian[5]);
                sp.include_pi(prime_implicants, vars, memdict);
            }
        }

        return sp;
    }

    template<typename Fn>
    void find_multiple_solutions(SubProblem &sp, Fn lambda, PreferredSolver& solver, PreferredSolver& scounts) {

        if (sp.impossible) {
            // UNSAT obtained in preprocessing:
            scounts.timings[0] += 1;
            return;
        }

        int solver_idx = solver.choose(initial_rows);

        auto start = std::chrono::high_resolution_clock::now();

        sp.find_all_solutions(lambda, solver_idx);

        auto end = std::chrono::high_resolution_clock::now();

        int64_t micros = std::chrono::duration_cast<std::chrono::microseconds>( end - start ).count();
        if (micros < 1) { micros = 1; }

        solver.update(solver_idx, micros);
        scounts.timings[solver_idx] += 1;
    }

    template<typename Fn>
    int find_all_solutions(int max_width, const std::vector<int> &prime_implicants, int lookahead, Fn lambda,
                            std::unordered_map<int, std::vector<int>> &memdict, PreferredSolver *solvers) {

        int subproblems = 0;

        int maxlpad = (shadow ? max_width : 0) - middle_bits;

        // asymmetric subproblems:
        for (int lpad = 0; lpad <= maxlpad; lpad++) {

            int rpad = max_width - middle_bits - lpad;
            auto sp = get_instance(prime_implicants, lpad, rpad, lookahead, false, false, memdict);

            find_multiple_solutions(sp, lambda, solvers[3 + middle_bits], solvers[0]);
            subproblems += 1;
        }

        // gutter-symmetric subproblems:
        if (gutter_symmetric) {

            int lpad = max_width - ((middle_bits - 1) >> 1);

            if (lpad >= 0) {
                auto sp = get_instance(prime_implicants, lpad, lpad, lookahead, true, true, memdict);

                find_multiple_solutions(sp, lambda, solvers[2], solvers[0]);
                subproblems += 1;
            }
        }

        // symmetric subproblems:
        if (symmetric) {

            int lpad = max_width - ((middle_bits + 1) >> 1);

            if (lpad >= 0) {
                auto sp = get_instance(prime_implicants, lpad, lpad, lookahead, true, false, memdict);

                find_multiple_solutions(sp, lambda, solvers[1], solvers[0]);
                subproblems += 1;
            }
        }

        return subproblems;
    }
};

