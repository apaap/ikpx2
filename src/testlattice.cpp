#include "lattice.hpp"
#include <stdio.h>

void print_transformation(const char* v) {

    Velocity vel(v);
    auto vec = vel.jacobian;

    printf("%s : (%d, %d, %d) : ", v, vel.vd, vel.hd, vel.p);

    printf("[(%d, %d), (%d, %d), (%d, %d)]\n",
        vec[0], vec[1], vec[2], vec[3], vec[4], vec[5]);

}

int main(int argc, char* argv[]) {

    (void) argc; (void) argv;

    print_transformation("c/2");
    printf("\n");
    print_transformation("c/3");
    printf("\n");
    print_transformation("c/4o");
    print_transformation("c/4d");
    printf("\n");
    print_transformation("c/5o");
    print_transformation("2c/5");
    print_transformation("c/5d");
    printf("\n");
    print_transformation("c/6o");
    print_transformation("c/6d");
    print_transformation("(2,1)c/6");
    printf("\n");
    print_transformation("c/7");
    print_transformation("2c/7");
    print_transformation("3c/7");
    print_transformation("c/7d");
    print_transformation("(2,1)c/7");
    printf("\n");
    print_transformation("c/8");
    print_transformation("3c/8");
    print_transformation("c/8d");
    print_transformation("(2,1)c/8");
    print_transformation("(3,1)c/8");
    printf("\n");
    print_transformation("c/9");
    print_transformation("2c/9");
    print_transformation("4c/9");
    print_transformation("c/9d");
    print_transformation("(2,1)c/9");
    print_transformation("(3,1)c/9");
    print_transformation("2c/9d");

    return 0;

}
