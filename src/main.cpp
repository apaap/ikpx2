#include <vector>

#include "../lifelib/pattern2.h"
#include "lattice.hpp"
#include "isosat.hpp"
#include "solver.hpp"
#include "banner.hpp"


std::vector<int> truth_table_for_rule(apg::lifetree_abstract<uint32_t> *lab, std::string rule) {

    std::vector<int> truthtab;

    apg::pattern cell(lab, "o!", rule);
    for (int i = 0; i < 512; i++) {
        apg::pattern x(lab, "", rule);

        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                if ((i >> (3*j+k)) & 1) {
                    x += cell(j, k);
                }
            }
        }

        truthtab.push_back(x[1].getcell(1, 1));
    }

    return truthtab;

}


int main() {

    print_banner();

    auto rvec = apg::get_all_rules();
    std::string rule = rvec[0];
    apg::lifetree<uint32_t, 1> lt(1000);

    auto truth_table = truth_table_for_rule(&lt, rule);
    auto prime_implicants = truth_table_to_prime_implicants(truth_table);
    int npi = prime_implicants.size();

    std::cout << "ikpx2 has been compiled for the rule\033[32;1m " << rule;
    std::cout << " \033[0m(" << npi << " prime implicants).\n" << std::endl;

    return 0;

}
