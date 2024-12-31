#include <iostream>
#include <vector>
#include <gnuplot-iostream.h>

int main() {
    Gnuplot gp;

    // Prepare data
    std::vector<std::vector<double>> data;
    int width = 10;
    int height = 10;
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            data.push_back({static_cast<double>(x), static_cast<double>(y), static_cast<double>(x * y)});
        }
    }

    // Plot heatmap
    gp << "set pm3d map\n";
    gp << "set palette rgbformulae 33,13,10\n";
    gp << "splot '-' using 1:2:3 with pm3d\n";
    gp.send1d(data);

    return 0;
}