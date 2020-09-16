#pragma once

#define SOLVER_KISSAT 1
// #define SOLVER_CADICAL 2

// ------------------------------------

#ifdef SOLVER_KISSAT
extern "C" {
#include "../kissat/src/kissat.h"
}
#endif

const static int SOLVER_MASK =
#ifdef SOLVER_KISSAT
(1 << SOLVER_KISSAT) +
#endif
#ifdef SOLVER_CADICAL
(1 << SOLVER_CADICAL) +
#endif
0;

#include <iostream>
#include <vector>

std::vector<int> solve_using_kissat(const std::vector<int> &cnf, int literals_to_return, int max_decisions=10000000) {

    auto solver = kissat_init();
    std::vector<int> solution;

    for (auto&& x : cnf) {
        kissat_add(solver, x);
    }

    if (max_decisions > 0) {
        // corresponds to a time limit of roughly 15 minutes:
        kissat_set_decision_limit(solver, max_decisions);
    }

    int res = kissat_solve(solver);
    solution.push_back(res);

    if (res == 0) {
        std::cerr << "\033[33;1mWarning:\033[0m SAT solver reached decision limit" << std::endl;
    } else if (res == 10) {
        // include satisfying assignments:
        for (int i = 1; i <= literals_to_return; i++) {
            solution.push_back(kissat_value(solver, i));
        }
    }

    kissat_release(solver);
    return solution;
}

template<typename Fn>
int multisolve(std::vector<int> &cnf, int solver_idx, const std::vector<int> &unique_literals,
                const std::vector<int> &zero_literals, int literals_to_return, Fn lambda) {

    int res = 0;

    #ifdef SOLVER_KISSAT
    if (solver_idx == SOLVER_KISSAT) {

        size_t origsize = cnf.size();

        int total_sols = 0;
        std::vector<int> ulits;

        while (true) {
            auto solution = solve_using_kissat(cnf, literals_to_return);
            res = solution[0];
            if (res != 10) { break; }
            total_sols += 1;
            for (auto&& x : unique_literals) {
                if (solution[x]) {
                    cnf.push_back(-solution[x]);
                    ulits.push_back(solution[x]);
                }
            }
            cnf.push_back(0);
            lambda(solution);
        }

        cnf.resize(origsize);

        if (total_sols > 0) {

            // try to find completion:

            if (total_sols == 1) {
                for (auto&& x : ulits) {
                    cnf.push_back(x);
                    cnf.push_back(0);
                }
            }

            for (auto&& x : zero_literals) {
                cnf.push_back(-x);
                cnf.push_back(0);
            }

            auto solution = solve_using_kissat(cnf, literals_to_return);
            if (solution[0] == 10) { lambda(solution); }
            cnf.resize(origsize);
        }
    }
    #endif

    #ifdef SOLVER_CADICAL
    if (solver_idx == SOLVER_CADICAL) {
        // TODO incremental solve

    }
    #endif

    return res;
}


std::vector<int> ramsey4(int N) {

    std::vector<int> cnf;

    for (int a = 0; a < N; a++) {
        for (int b = 0; b < a; b++) {
            for (int c = 0; c < b; c++) {
                for (int d = 0; d < c; d++) {
                    for (int k = -1; k <= 1; k += 2) {
                        cnf.push_back((((a * (a + 1)) >> 1) - b) * k);
                        cnf.push_back((((a * (a + 1)) >> 1) - c) * k);
                        cnf.push_back((((a * (a + 1)) >> 1) - d) * k);
                        cnf.push_back((((b * (b + 1)) >> 1) - c) * k);
                        cnf.push_back((((b * (b + 1)) >> 1) - d) * k);
                        cnf.push_back((((c * (c + 1)) >> 1) - d) * k);
                        cnf.push_back(0);
                    }
                }
            }
        }
    }

    return solve_using_kissat(cnf, (N * (N - 1)) >> 1);

}

/**
 * Checks the speed and correctness of the SAT solver by requesting it to
 * find a red/blue colouring of the edges of a complete graph K_17 such
 * that there is no monochromatic K_4 subgraph.
 *
 * It is known that there is a unique solution up to isomorphism, namely
 * the 17-vertex Paley graph.
 */
void check_sat_solver() {

    std::cerr << "Checking SAT solver..." << std::endl;

    auto paley = ramsey4(17);

    if (paley.size() != 137) {
        std::cerr << "Error: incorrect number of literals" << std::endl;
        exit(1);
    }

    int degrees[17] = {0};

    for (int a = 0; a < 17; a++) {
        for (int b = 0; b < a; b++) {
            if (paley[((a * (a + 1)) >> 1) - b] > 0) {
                degrees[a] += 1;
                degrees[b] += 1;
            }
        }
    }

    for (int i = 0; i < 17; i++) {
        if (degrees[i] != 8) {
            std::cerr << "Error: vertex " << i << " has incorrect degree " << degrees[i] << std::endl;
            exit(1);
        }
    }

    std::cerr << "...check complete!\n" << std::endl;
}
