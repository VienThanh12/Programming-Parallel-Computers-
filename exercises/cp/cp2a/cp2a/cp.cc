/*
This is the function you need to implement. Quick reference:
- input rows: 0 <= y < ny
- input columns: 0 <= x < nx
- element at row y and column x is stored in data[x + y*nx]
- the correlation between rows i and j has to be stored in result[i + j*ny]
- only elements with 0 <= j <= i < ny need to be filled
*/
// $$r_{ij} = \frac{\sum (x_i - \bar{x}_i)(x_j - \bar{x}_j)}{\sqrt{\sum (x_i - \bar{x}_i)^2 \sum (x_j - \bar{x}_j)^2}}$$
#include <cmath>
#include <vector>

void correlate(int ny, int nx, const float *data, float *result) {
    // Number of independent accumulators in the inner loop (ILP).
    constexpr int nb = 4;
    // Pad nx up to a multiple of nb so the inner loop is branch-free.
    int na = (nx + nb - 1) / nb;
    int nxp = na * nb;

    // Normalized rows, padded with 0.0 (zeros do not contribute to the dot product).
    std::vector<double> nor_data(static_cast<size_t>(ny) * nxp, 0.0);

    for (int i = 0; i < ny; i++) {
        // Mean of row i
        double mean = 0.0;
        for (int j = 0; j < nx; j++) {
            mean += data[j + i * nx];
        }
        mean /= nx;

        // Center and accumulate squared sum
        double sum_sq = 0.0;
        for (int j = 0; j < nx; j++) {
            double val = data[j + i * nx] - mean;
            nor_data[j + i * nxp] = val;
            sum_sq += val * val;
        }

        // Normalize so each row has unit L2 norm
        double norm = std::sqrt(sum_sq);
        if (norm > 0.0) {
            double inv = 1.0 / norm;
            for (int j = 0; j < nx; j++) {
                nor_data[j + i * nxp] *= inv;
            }
        }
    }

    // Dot products with nb independent accumulators → break the FP-add dep chain.
    for (int i = 0; i < ny; i++) {
        for (int j = 0; j <= i; j++) {
            double vv[nb];
            for (int kb = 0; kb < nb; ++kb) vv[kb] = 0.0;

            const double *ri = &nor_data[i * nxp];
            const double *rj = &nor_data[j * nxp];

            for (int ka = 0; ka < na; ++ka) {
                for (int kb = 0; kb < nb; ++kb) {
                    vv[kb] += ri[ka * nb + kb] * rj[ka * nb + kb];
                }
            }

            double sum = 0.0;
            for (int kb = 0; kb < nb; ++kb) sum += vv[kb];

            result[i + j * ny] = static_cast<float>(sum);
        }
    }
}
