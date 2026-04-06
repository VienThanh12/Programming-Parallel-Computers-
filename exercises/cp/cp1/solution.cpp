#include <cmath>
#include <vector>

void correlate(int ny, int nx, const float* data, float* result) {
    std::vector<double> normalized(ny * nx);

    for (int y = 0; y < ny; y++) {
        double sum = 0.0;
        for (int x = 0; x < nx; x++) {
            sum += data[x + y * nx];
        }
        double mean = sum / nx;

        double sqsum = 0.0;
        for (int x = 0; x < nx; x++) {
            double val = data[x + y * nx] - mean;
            normalized[x + y * nx] = val;
            sqsum += val * val;
        }

        double inv = 1.0 / std::sqrt(sqsum);
        for (int x = 0; x < nx; x++) {
            normalized[x + y * nx] *= inv;
        }
    }

    for (int i = 0; i < ny; i++) {
        for (int j = 0; j <= i; j++) {
            double dot = 0.0;
            for (int x = 0; x < nx; x++) {
                dot += normalized[x + i * nx] * normalized[x + j * nx];
            }
            result[i + j * ny] = static_cast<float>(dot);
        }
    }
}
