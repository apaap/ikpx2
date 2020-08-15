#pragma once

#include "../lifelib/hashtrees/numtheory.h"
#include "../lifelib/ssplit.h"
#include <iostream>
#include <cmath>


typedef std::vector<uint64_t> u64seq;


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


std::vector<int> get_transformation(int vd, int hd, int p) {

    int vdp = apg::euclid_gcd(vd, p);

    if (apg::euclid_gcd(vdp, hd) != 1) {
        std::cerr << "Error: vertical displacement, horizontal displacement, and period must be coprime" << std::endl;
        exit(1);
    }

    if ((p <= 0) || (vd <= 0)) {
        std::cerr << "Error: vertical displacement and period must be positive" << std::endl;
        exit(1);
    }

    if (hd < 0) {
        std::cerr << "Error: horizontal displacement must be nonnegative" << std::endl;
        exit(1);
    }

    if (hd > vd) {
        std::cerr << "Error: horizontal displacement cannot exceed vertical displacement" << std::endl;
        exit(1);
    }

    int du_dx =  vd / apg::euclid_gcd(vd, hd);
    int du_dy = -hd / apg::euclid_gcd(vd, hd);

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

    return {du_dy, dv_dy,
            du_dx, dv_dx,
                0, dv_dt};

}

struct Velocity {

    std::vector<int> jacobian;
    std::vector<int> iacobjan;
    int vd; int hd; int p; int det;

    Velocity(const std::string &velocity) {

        bool diagonal = (velocity.find("d") != std::string::npos);
        std::vector<int64_t> ints;

        {
            std::istringstream str(velocity);
            apg::onlyints(ints, str);

            if ((ints.size() != 1) && (ints.size() != 2) && (ints.size() != 3)) {
                std::cerr << "Incomprehensible velocity string: " << velocity << std::endl;
                exit(1);
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
        det = inv2x2(jacobian, iacobjan);
    }

    int hradius() const {
        return std::abs(jacobian[0]) + std::abs(jacobian[2]);
    }

    int vradius() const {
        return std::abs(jacobian[1]) + std::abs(jacobian[3]);
    }

};

