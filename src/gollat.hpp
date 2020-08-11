#pragma once

#include "../lifelib/pattern2.h"
#include "lattice.hpp"

std::vector<int> truth_table_for_rule(apg::lifetree_abstract<uint32_t> *lab, std::string rule) {

    std::vector<int> truthtab;

    apg::pattern cell(lab, "o!", rule);
    for (int i = 0; i < 512; i++) {
        apg::pattern x(lab, "", rule);

        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                if ((i >> (3*j+k)) & 1) {
                    x += cell(j, k);
                }
            }
        }

        truthtab.push_back(x[1].getcell(1, 1));
    }

    return truthtab;
}


apg::pattern ltransform(apg::pattern &x, const Velocity &vel) {

    apg::pattern y = x;
    apg::pattern ikpx(y.getlab(), "", y.getrule());
    apg::pattern onecell(y.getlab(), "o!", y.getrule());

    for (int t = 0; t < vel.p; t++) {

        int pop = y.totalPopulation();
        std::vector<int64_t> coords(pop*2);
        y.get_coords(coords.data());

        for (int j = 0; j < pop; j++) {
            int ox = coords[2*j];
            int oy = coords[2*j+1];

            ikpx += onecell(vel.jacobian[0] * oy + vel.jacobian[2] * ox + vel.jacobian[4] * t,
                            vel.jacobian[1] * oy + vel.jacobian[3] * ox + vel.jacobian[5] * t);
        }

        y = y[1];
    }

    if (ikpx.nonempty()) {
        int64_t bbox[4];
        ikpx.getrect(bbox);
        ikpx = ikpx.shift(0 - bbox[0], 0 - bbox[1]);
    }

    return ikpx;

}
