#include "lattice.hpp"
#include <stdio.h>

void print_transformation(int vd, int hd, int p) {

    printf("(%d, %d)c/%d : ", vd, hd, p);

    auto vec = get_transformation(vd, hd, p);

    printf("[(%d, %d), (%d, %d), (%d, %d)]\n",
        vec[0], vec[1], vec[2], vec[3], vec[4], vec[5]);

}

int main(int argc, char* argv[]) {

    (void) argc; (void) argv;

    print_transformation(1, 0, 2);
    printf("\n");
    print_transformation(1, 0, 3);
    printf("\n");
    print_transformation(1, 0, 4);
    printf("\n");
    print_transformation(1, 0, 5);
    print_transformation(2, 0, 5);
    printf("\n");
    print_transformation(1, 0, 6);
    print_transformation(2, 1, 6);
    printf("\n");
    print_transformation(1, 0, 7);
    print_transformation(2, 0, 7);
    print_transformation(3, 0, 7);
    print_transformation(2, 1, 7);
    printf("\n");
    print_transformation(1, 0, 8);
    print_transformation(3, 0, 8);
    print_transformation(2, 1, 8);
    print_transformation(3, 1, 8);
    printf("\n");
    print_transformation(1, 0, 9);
    print_transformation(2, 0, 9);
    print_transformation(4, 0, 9);
    print_transformation(2, 1, 9);
    print_transformation(3, 1, 9);
    printf("\n");
    print_transformation(1, 0, 10);
    print_transformation(3, 0, 10);
    print_transformation(2, 1, 10);
    print_transformation(3, 1, 10);
    print_transformation(4, 1, 10);
    print_transformation(3, 2, 10);
    printf("\n");
    print_transformation(1, 0, 11);
    print_transformation(2, 0, 11);
    print_transformation(3, 0, 11);
    print_transformation(4, 0, 11);
    print_transformation(5, 0, 11);
    print_transformation(6, 0, 11);
    print_transformation(2, 1, 11);
    print_transformation(3, 1, 11);
    print_transformation(4, 1, 11);
    print_transformation(3, 2, 11);

    return 0;

}
