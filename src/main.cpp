#include "banner.hpp"
#include "core.hpp"


void print_rule() {

    std::string rule = apg::get_all_rules()[0];
    apg::lifetree<uint32_t, 1> lt(100);

    auto truth_table = truth_table_for_rule(&lt, rule);
    auto pis = truth_table_to_prime_implicants(truth_table);

    std::cerr << "ikpx2 has been compiled for the rule\033[32;1m " << rule <<
    " \033[0m (" << pis.size() << " prime implicants).\n" << std::endl;
}


int main(int argc, char* argv[]) {

    print_banner();
    print_rule();

    std::vector<std::string> arguments;

    for (int i = 1; i < argc; i++) {
        arguments.emplace_back(argv[i]);
    }

    for (auto&& x : arguments) {
        if ((x == "-h") || (x == "--help")) {
            print_help();
            return 0;
        }
    }

    check_sat_solver();

    return run_ikpx(arguments);

}

/*

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
        if (((int) i) == 100 * n7) { break; }
    }

    MetaProblem mp(t, vel);
    std::cerr << "middle_bits = " << mp.middle_bits << std::endl;

    mp.find_all_solutions(29, 1, prime_implicants, 30, [&](const u64seq &svec) {

        for (int i = 0; i <= ((int) svec.size()) - n7; i += 1) {
            auto lower_t = tree.inject(&(svec[i]));
            if (lower_t.size()) { t = lower_t; }
        }

        auto pat = tree.materialise(&lt, t.data());
        ikpx2golly(pat, vel).write_rle(std::cerr);

    });
    

    return 0;

}

*/
