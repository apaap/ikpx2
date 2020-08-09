#pragma once

#include "../lifelib/hashtrees/numtheory.h"


int signedinc(int x) {
    if (x == 0) {
        return -1;
    } else if (x < 0) {
        return -x;
    } else {
        return -(1+x);
    }
}


std::vector<int> get_transformation(int vd, int hd, int p) {

    int vdp = apg::euclid_gcd(vd, p);

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
