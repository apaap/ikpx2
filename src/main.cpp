#include <vector>

#include "../lifelib/pattern2.h"
#include "lattice.hpp"
#include "isosat.hpp"
#include "solver.hpp"
#include "banner.hpp"


std::vector<int> prime_implicants_for_rule(apg::lifetree_abstract<uint32_t> *lab, std::string rule) {

    int truthtab[1024];

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

        int islive = x[1].getcell(1, 1);
        truthtab[i] = 1 - islive;
        truthtab[i + 512] = islive;
    }

    return get_prime_implicants(truthtab, 10);

}


int main() {

    print_banner();

    auto rvec = apg::get_all_rules();
    std::string rule = rvec[0];
    apg::lifetree<uint32_t, 1> lt(1000);
    auto prime_implicants = prime_implicants_for_rule(&lt, rule);

    int npi = prime_implicants.size();

    std::cout << "ikpx2 has been compiled for the rule\033[32;1m " << rule;
    std::cout << " \033[0m(" << npi << " prime implicants).\n" << std::endl;

    return 0;

}
