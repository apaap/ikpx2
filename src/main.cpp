#include "isosat.hpp"
#include "solver.hpp"
#include "banner.hpp"
#include "ikpxtree.hpp"



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

    // apg::pattern robin(&lt, "docs/partial2c7.rle");
    // Velocity vel("(2,1)c/7");
    apg::pattern robin(&lt, "docs/almost.rle");
    Velocity vel("(2,1)c/6");
    // apg::pattern robin(&lt, "docs/rakeend.rle");
    // Velocity vel("3c/7");

    std::vector<uint64_t> results;
    int n7 = ltransform(robin, vel, results);

    std::cerr << "n7 = " << n7 << std::endl;

    ikpxtree tree(n7 - 1);

    u64seq t;

    for (uint64_t i = 0; i < results.size(); i += n7) {
        auto lower_t = tree.inject(&(results[i]));
        if (lower_t.size()) { t = lower_t; }
        if (i == 100 * n7) { break; }
    }

    MetaProblem mp(t, vel);
    std::cerr << "middle_bits = " << mp.middle_bits << std::endl;
    auto sp = mp.get_instance(prime_implicants, 4, 4, 30);

    auto svec = sp.solve();

    for (int i = 0; i <= ((int) svec.size()) - n7; i += 1) {
        auto lower_t = tree.inject(&(svec[i]));
        if (lower_t.size()) { t = lower_t; }
    }

    auto pat = tree.materialise(&lt, t.data());
    ikpx2golly(pat, vel).write_rle(std::cerr);
    

    return 0;

}
