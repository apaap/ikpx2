#pragma once

#include "../lifelib/pattern2.h"
#include "lattice.hpp"

typedef apg::lifetree_abstract<uint32_t> lab32_t;


std::vector<int> truth_table_for_rule(lab32_t *lab, std::string rule) {

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


apg::pattern golly2ikpx(apg::pattern &x, const Velocity &vel) {

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

    return ikpx;
}


std::vector<std::set<std::pair<int32_t, int32_t>>> ikpx2phases(apg::pattern &x, const Velocity &vel) {

    int pop = x.totalPopulation();
    std::vector<int64_t> coords(pop*2);
    x.get_coords(coords.data());

    std::vector<std::set<std::pair<int32_t, int32_t>>> phases(vel.p);

    for (int i = 0; i < vel.p; i++) {
        for (int j = 0; j < pop; j++) {
            int ou = coords[2*j] + vel.jacobian[4] * i;
            int ov = coords[2*j+1] + vel.jacobian[5] * i;

            int oy = vel.iacobjan[0] * ou + vel.iacobjan[2] * ov;
            int ox = vel.iacobjan[1] * ou + vel.iacobjan[3] * ov;

            if ((ox % vel.det == 0) && (oy % vel.det == 0)) {
                phases[i].emplace(oy / vel.det, ox / vel.det);
            }
        }
    }
    
    return phases;
}


apg::pattern ikpx2golly(apg::pattern &x, const Velocity &vel) {

    auto phases = ikpx2phases(x, vel);
    apg::pattern golly(x.getlab(), "", x.getrule());
    apg::pattern onecell(x.getlab(), "o!", x.getrule());

    for (auto&& x : phases[0]) {
        golly += onecell(x.second, x.first);
    }

    return golly;
}

std::string cells2rle(const std::set<std::pair<int32_t, int32_t>> &cells, const std::string &rule, bool isSeed) {

    int32_t minx = 999999999;
    int32_t miny = 999999999;
    int32_t maxx = -minx;
    int32_t maxy = -miny;

    for (auto&& cell : cells) {
        minx = std::min(minx, cell.second);
        miny = std::min(miny, cell.first);
        maxx = std::max(maxx, cell.second);
        maxy = std::max(maxy, cell.first);
    }

    std::ostringstream outstream;

    if (isSeed) {
        outstream << '-';
    } else {
        outstream << "x = " << (maxx - minx + 1) << ", y = " << (maxy - miny + 1) << ", rule = " << rule << std::endl;
    }

    apg::RleWriter rw(outstream, (isSeed ? '-' : '\n'), 66, 1);

    for (auto&& cell : cells) {
        uint64_t y = cell.first - miny;
        uint64_t x = cell.second - minx;
        rw.addcell(x, y, 1);
    }

    rw.finalise();

    if (isSeed) { outstream << std::endl; }

    return outstream.str();
}


int extract_rows(apg::pattern &ikpx, const Velocity &vel, const std::vector<int> &truthtab, std::vector<uint64_t> &results) {

    int hradius = vel.hradius();
    int vradius = vel.vradius();

    if (ikpx.nonempty()) {

        int64_t bbox[4] = {0ull, 0ull, 0ull, 0ull};
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

                if (truthtab[x] != ((int) ikpx.getcell(i + vel.jacobian[4], j + vel.jacobian[5]))) {
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

int ltransform(apg::pattern &x, const Velocity &vel, std::vector<uint64_t> &results) {

    apg::pattern ikpx = golly2ikpx(x, vel);
    std::vector<int> truthtab = truth_table_for_rule(x.getlab(), x.getrule());
    return extract_rows(ikpx, vel, truthtab, results);

}

