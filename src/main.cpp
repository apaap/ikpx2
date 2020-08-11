#include "isosat.hpp"
#include "solver.hpp"
#include "banner.hpp"
#include "gollat.hpp"



int main() {

    print_banner();

    auto rvec = apg::get_all_rules();
    std::string rule = rvec[0];
    apg::lifetree<uint32_t, 1> lt(1000);

    auto truth_table = truth_table_for_rule(&lt, rule);
    auto prime_implicants = truth_table_to_prime_implicants(truth_table);
    int npi = prime_implicants.size();

    std::cerr << "ikpx2 has been compiled for the rule\033[32;1m " << rule;
    std::cerr << " \033[0m(" << npi << " prime implicants).\n" << std::endl;

    check_sat_solver();

    apg::pattern robin(&lt, "docs/almost.rle");
    Velocity vel("(2,1)c/6");
    std::vector<uint64_t> results;
    int n7 = ltransform(robin, vel, results);

    std::cerr << "n7 = " << n7 << std::endl;

    return 0;

}
