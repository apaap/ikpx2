#include "banner.hpp"
#include "apgluxe.hpp"
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
