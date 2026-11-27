// Checks shared/Rig.hpp (the mapping the Windows and Linux renderers call) against tests/rig/golden.tsv.
// Build: c++ -std=c++17 tests/rig/rig_test.cpp -o rig_test && ./rig_test tests/rig/golden.tsv
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "../../shared/Rig.hpp"

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: %s golden.tsv\n", argv[0]); return 2; }
    std::ifstream in(argv[1]);
    if (!in) { std::fprintf(stderr, "cannot open %s\n", argv[1]); return 2; }
    const float tol = 2e-3f; // float32 vs the float64 reference, in canvas units
    int n = 0, bad = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string kind;
        ss >> kind;
        std::vector<double> v;
        double d;
        while (ss >> d) v.push_back(d);
        kurorig::Vec2 got{};
        if (kind == "head" && v.size() == 12) {
            got = kurorig::headPoint((float)v[0], (float)v[1], v[2] != 0, v[3] != 0, (float)v[4],
                                     (float)v[5], (float)v[6], (float)v[7], (float)v[8], (float)v[9]);
        } else if (kind == "body" && v.size() == 10) {
            got = kurorig::bodyPoint((float)v[0], (float)v[1], (float)v[2], (float)v[3], (float)v[4], (float)v[5],
                                     (float)v[6], (float)v[7]);
        } else {
            std::fprintf(stderr, "bad golden line: %s\n", line.c_str());
            return 2;
        }
        double ex = v[v.size() - 2], ey = v[v.size() - 1];
        ++n;
        if (std::fabs(got.x - ex) > tol || std::fabs(got.y - ey) > tol) {
            if (++bad <= 10) std::fprintf(stderr, "MISMATCH %s -> got (%.4f, %.4f) want (%.4f, %.4f)\n", line.c_str(), got.x, got.y, ex, ey);
        }
    }
    if (bad) { std::fprintf(stderr, "C++ rig: %d of %d cases differ\n", bad, n); return 1; }
    std::printf("C++ rig (shared/Rig.hpp): %d cases match\n", n);
    return 0;
}
