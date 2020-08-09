#include "isosat.hpp"
#include <iostream>

int main(int argc, char* argv[]) {

    (void) argc; (void) argv;

    int truthtab[1024];

    for (int i = 0; i < 512; i++) {

        int ncount = __builtin_popcount(i & 495);
        int islive = (i & 16) ? ((ncount == 2) || (ncount == 3)) : (ncount == 3);

        truthtab[i] = 1 - islive;
        truthtab[i + 512] = islive;
    }

    auto pi = get_prime_implicants(truthtab, 10);

    std::cout << pi.size() << " prime implicants: [";

    for (auto&& x : pi) {
        std::cout << x << ", ";
    }

    std::cout << "]" << std::endl;

    return 0;
}
