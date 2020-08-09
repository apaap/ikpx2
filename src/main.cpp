#include <vector>

#include "../lifelib/pattern2.h"
#include "lattice.hpp"
#include "isosat.hpp"
#include "solver.hpp"
#include "banner.hpp"

int main() {

    print_banner();

    auto rvec = apg::get_all_rules();

    std::cout << "ikpx2 has been configured for the rule\033[32;1m " << rvec[0] << " \033[0m\n" << std::endl;

    return 0;

}
