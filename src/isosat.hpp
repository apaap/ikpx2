#pragma once

#include <stdint.h>
#include <vector>

void get_all_implicants(const int* truthtab, int* output, int depth) {

    if (depth == 0) {
        output[0] = (truthtab[0] == 0);
        return;
    }

    int twon = 1;
    int threen = 1;

    for (int i = 1; i < depth; i++) { twon *= 2; threen *= 3; }

    for (int i = 0; i < 2; i++) {
        get_all_implicants(truthtab + twon*i, output + threen*i, depth-1);
    }

    for (int i = 0; i < threen; i++) {
        output[i + threen*2] = output[i] & output[i + threen];
    }

}

void only_prime_implicants(const int* implicants, int* output, int depth) {

    if (depth == 0) {
        output[0] = implicants[0];
        return;
    }

    int threen = 1;
    for (int i = 1; i < depth; i++) { threen *= 3; }

    for (int i = 0; i < 3; i++) {
        only_prime_implicants(implicants + threen*i, output + threen*i, depth-1);
    }

    for (int i = 0; i < threen; i++) {
        if (implicants[i + threen*2]) {
            output[i + threen] = 0;
            output[i] = 0;
        }
    }
}

void extract_prime_implicants(const int* prime_implicants, std::vector<int> &output, int depth, int mask=0) {

    if (depth == 0) {
        if (prime_implicants[0]) {
            output.push_back(mask);
        }
        return;
    }

    int threen = 1;
    for (int i = 1; i < depth; i++) { threen *= 3; }

    for (int i = 0; i < 3; i++) {
        extract_prime_implicants(prime_implicants + threen*i, output, depth-1, mask | ((2-i) << ((depth-1)*2)));
    }

}

std::vector<int> get_prime_implicants(const int* truthtab, int depth) {

    int threen = 1;
    for (int i = 0; i < depth; i++) { threen *= 3; }

    std::vector<int> all_implicants(threen);
    std::vector<int> prime_implicants(threen);

    get_all_implicants(truthtab, all_implicants.data(), depth);
    only_prime_implicants(all_implicants.data(), prime_implicants.data(), depth);

    std::vector<int> result;

    extract_prime_implicants(prime_implicants.data(), result, depth);

    return result;
}

std::vector<int> truth_table_to_prime_implicants(std::vector<int> &truth_table) {

    int truthtab[1024];

    for (int i = 0; i < 512; i++) {
        int islive = truth_table[i];
        truthtab[i] = 1 - islive;
        truthtab[i + 512] = islive;
    }

    return get_prime_implicants(truthtab, 10);
}

/**
 * Append clauses to a CNF corresponding to a napkin.
 */
void include_pi(std::vector<int> &cnf, const std::vector<int> &prime_implicants, const std::vector<int> &vars) {

    for (auto&& x : prime_implicants) {
        for (size_t i = 0; i < vars.size(); i++) {
            int v = (x >> (i*2)) & 3;
            if (v == 1) {
                cnf.push_back(-vars[i]);
            } else if (v == 2) {
                cnf.push_back(vars[i]);
            }
        }
        cnf.push_back(0);
    }
}
