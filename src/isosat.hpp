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
