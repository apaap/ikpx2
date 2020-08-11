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


int ltransform(apg::pattern &x, const Velocity &vel, std::vector<uint64_t> &results) {

    apg::pattern y = x;
    apg::pattern ikpx(y.getlab(), "", y.getrule());
    apg::pattern onecell(y.getlab(), "o!", y.getrule());

    std::vector<int> truthtab = truth_table_for_rule(y.getlab(), y.getrule());

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

    int hradius = std::abs(vel.jacobian[0]) + std::abs(vel.jacobian[2]);
    int vradius = std::abs(vel.jacobian[1]) + std::abs(vel.jacobian[3]);

    if (ikpx.nonempty()) {

        int64_t bbox[4];
        ikpx.getrect(bbox);
        ikpx = ikpx.shift(hradius - bbox[0], vradius - bbox[1]);

        int  width = hradius * 2 + bbox[2];
        int height = vradius * 2 + bbox[3];

        int record_idx = 0;
        int record_length = 0;
        int current_length = 0;

        for (int j = 0; j < height; j++) {

            int line_errors = 0;

            for (int i = 0; i < width; i++) {

                int x = 0;
                for (int k = -1; k <= 1; k++) {
                    for (int l = -1; l <= 1; l++) {
                        x = (x << 1) + ikpx.getcell(i + vel.jacobian[0] * k + vel.jacobian[2] * l,
                                                    j + vel.jacobian[1] * k + vel.jacobian[3] * l);
                    }
                }

                if (truthtab[x] != ikpx.getcell(i + vel.jacobian[4], j + vel.jacobian[5])) {
                    line_errors += 1;
                }
            }

            if (line_errors == 0) {
                current_length += 1;
                if (current_length > record_length) {
                    record_length = current_length;
                    record_idx = j;
                }
            } else {
                current_length = 0;
            }
        }

        int start_idx = record_idx + 1 - record_length;
        std::cerr << "total rows: [0, " << (height - 1) << "]; ";
        std::cerr << "valid rows: [" << start_idx << ", " << record_idx << "]." << std::endl;

        for (int j = start_idx; j <= record_idx; j++) {

            apg::pattern slice = ikpx.subrect(0, j - vradius, width, vradius*2 + 1);
            if (slice.empty()) { continue; }
            slice.getrect(bbox);
            if (bbox[2] > 64) {
                std::cerr << "Warning: overfull hbox (badness " << (bbox[2] - 64) << ") at line " << j << std::endl;
                continue;
            }
            slice = slice(0 - bbox[0], vradius - j);

            for (int k = 0; k <= vradius*2; k++) {
                uint64_t row = 0;
                for (int i = 0; i < bbox[2]; i++) {
                    row |= slice.getcell(i, k) << i;
                }
                results.push_back(row);
            }
        }
    }

    return vradius * 2 + 1;

}
