#include <vector>

extern "C" {
#include "../kissat/src/kissat.h"
}

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

    if (res == 10) {
        // include satisfying assignments:
        for (int i = 1; i <= literals_to_return; i++) {
            solution.push_back(kissat_value(solver, i));
        }
    }

    kissat_release(solver);
    return solution;
}

int main() {

    return 0;

}
