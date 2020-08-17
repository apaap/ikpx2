#pragma once
#include "isosat.hpp"
#include "ikpxtree.hpp"
#include "../cqueue/blockingconcurrentqueue.h"

struct workitem {

    u64seq initial_rows;
    int16_t exhausted_width;
    int16_t maximum_width;
    int16_t lookahead;
    int16_t reverse;

};

int run_ikpx(const std::vector<std::string> &arguments) {

    std::string rule = apg::get_all_rules()[0];
    apg::lifetree<uint32_t, 1> lt(100);

    auto truth_table = truth_table_for_rule(&lt, rule);
    auto prime_implicants = truth_table_to_prime_implicants(truth_table);
    int npi = prime_implicants.size();

    std::cerr << "ikpx2 has been compiled for the rule\033[32;1m " << rule;
    std::cerr << " \033[0m(" << npi << " prime implicants).\n" << std::endl;

    std::cerr << "sizeof(workitem) = " << sizeof(workitem) << std::endl;

    std::string velocity = "";
    std::string directory = "";

    for (size_t i = 0; i < arguments.size(); i++) {

        std::string command = arguments[i];

        if ((command == "-v") || (command == "--velocity")) {
            velocity = arguments[++i];
        } else if ((command == "-d") || (command == "--directory")) {
            directory = arguments[++i];
        } else {
            ERREXIT("unknown command: " << command);
        }
    }

    if (velocity == "") {
        ERREXIT("velocity must be specified.");
    }

    Velocity vel(velocity);

    std::cerr << "Valid velocity: \033[32;1m(" << vel.vd << "," << vel.hd << ")c/" << vel.p << "\033[0m" << std::endl;

    return 0;

}

