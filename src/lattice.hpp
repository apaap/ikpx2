#pragma once

#include "../apgmera/lifelib/hashtrees/numtheory.h"
#include "../apgmera/lifelib/ssplit.h"
#include "maths.hpp"
#include <iostream>
#include <cmath>

#define ERREXIT(x) std::cerr << "\033[31;1mError:\033[0m " << x << std::endl; exit(1)


typedef std::vector<uint64_t> u64seq;

typedef std::vector<uint128_t> u128seq;

int signedinc(int x) {
    if (x == 0) {
        return -1;
    } else if (x < 0) {
        return -x;
    } else {
        return -(1+x);
    }
}


int inv2x2(const std::vector<int> &M, std::vector<int> &N) {

    int det = M[0] * M[3] - M[1] * M[2];

    N.clear();
    N.push_back(M[3]);
    N.push_back(-M[1]);
    N.push_back(-M[2]);
    N.push_back(M[0]);

    if (det < 0) {
        det = -det;
        for (int i = 0; i < 4; i++) {
            N[i] = -N[i];
        }
    }

    return det;
}


int l1dist(int x, int y) { return std::abs(x) + std::abs(y); }
int sqdist(int x, int y) { return (x * x) + (y * y); }


std::vector<int> get_transformation(int vd, int hd, int p) {

    int vdp = apg::euclid_gcd(vd, p);

    if (apg::euclid_gcd(vdp, hd) != 1) {
        ERREXIT("vertical displacement, horizontal displacement, and period must be coprime");
    }

    if ((p <= 0) || (vd <= 0)) {
        ERREXIT("vertical displacement and period must be positive");
    }

    if (hd < 0) {
        ERREXIT("horizontal displacement must be nonnegative");
    }

    if (hd > vd) {
        ERREXIT("horizontal displacement cannot exceed vertical displacement");
    }

    // Find the first lattice basis vector:

    int vdhd = apg::euclid_gcd(vd, hd);
    int du_dx =  vd / vdhd;
    int du_dy = -hd / vdhd;

    // Find a second lattice basis vector:

    int dv_dx = 0;
    int dv_dy = 0;
    int dv_dt = 0;

    for (;; dv_dx = signedinc(dv_dx)) {

        if ((dv_dx * hd) % vdp) { continue; }

        dv_dy = 0;
        while  ((dv_dy * vd + dv_dx * hd) <= 0) { dv_dy += 1; }
        while  ((dv_dy * vd + dv_dx * hd)  % p) { dv_dy += 1; }
        dv_dt = (dv_dy * vd + dv_dx * hd) / p;

        int det = du_dy * dv_dx - dv_dy * du_dx;
        if (det < 0) { det = -det; }

        if (apg::euclid_gcd(det, dv_dt) != 1) { continue; }

        break;
    }

    // Reduce the second lattice basis vector against the first.
    // We use L1 distance as a primary objective and L2 distance
    // as a tie-breaker, which yields a total order on Z^2 / D8_1:
    // (0, 0) < (1, 0) < (1, 1) < (2, 0) < (2, 1) < (3, 0) <
    // (2, 2) < (3, 1) < (4, 0) < (3, 2) < (4, 1) < (5, 0) < ...

    while (sqdist(dv_dy, dv_dx) > sqdist(dv_dy + du_dy, dv_dx + du_dx)) {
        dv_dy += du_dy; dv_dx += du_dx;
    }

    while (sqdist(dv_dy, dv_dx) > sqdist(dv_dy - du_dy, dv_dx - du_dx)) {
        dv_dy -= du_dy; dv_dx -= du_dx;
    }

    while (l1dist(dv_dy, dv_dx) > l1dist(dv_dy + du_dy, dv_dx + du_dx)) {
        dv_dy += du_dy; dv_dx += du_dx;
    }

    while (l1dist(dv_dy, dv_dx) > l1dist(dv_dy - du_dy, dv_dx - du_dx)) {
        dv_dy -= du_dy; dv_dx -= du_dx;
    }

    return {du_dy, dv_dy,
            du_dx, dv_dx,
                0, dv_dt};

}

struct Velocity {

    std::vector<int> jacobian;
    std::vector<int> iacobjan;
    int vd; int hd; int p; int det;

    Velocity(const std::string &velocity) {

        bool reverse = (velocity.find("-") != std::string::npos);
        bool diagonal = (velocity.find("d") != std::string::npos);
        std::vector<int64_t> ints;

        {
            std::istringstream str(velocity);
            apg::onlyints(ints, str);

            if ((ints.size() != 1) && (ints.size() != 2) && (ints.size() != 3)) {
                ERREXIT("incomprehensible velocity string: " << velocity);
            }
        }

        vd = (ints.size() == 1) ? 1 : ints[0];
        hd = 0;
        p = ints[ints.size() - 1];

        if (ints.size() == 3) {
            hd = ints[1];
        } else if (diagonal) {
            hd = vd;
        }

        jacobian = get_transformation(vd, hd, p);

        if (reverse) { for (int i = 1; i < 6; i += 2) { jacobian[i] *= -1; } }

        det = inv2x2(jacobian, iacobjan);
    }

    int hradius() const {
        return std::abs(jacobian[0]) + std::abs(jacobian[2]);
    }

    int vradius() const {
        return std::abs(jacobian[1]) + std::abs(jacobian[3]);
    }

};

