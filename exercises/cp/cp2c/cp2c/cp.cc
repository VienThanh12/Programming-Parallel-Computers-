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

typedef float float8_t __attribute__ ((vector_size (8 * sizeof(float))));
typedef double double8_t __attribute__ ((vector_size (8 * sizeof(double))));

constexpr float8_t f8zero {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
constexpr double8_t d8zero {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

static inline double hsum8(double8_t v) {
    return v[0] + v[1] + v[2] + v[3] + v[4] + v[5] + v[6] + v[7];
}

void correlate(int ny, int nx, const float *data, float *result) {
    // Width of one SIMD lane group (8 doubles per double8_t).
    constexpr int nb = 8;
    // Number of double8_t vectors per padded row.
    int na = (nx + nb - 1) / nb;

    // Normalized rows, packed as double8_t. Padding lanes are zero so they
    // do not contribute to the dot product.
    std::vector<double8_t> nor_data(static_cast<size_t>(ny) * na, d8zero);

    #pragma omp parallel for
    for (int i = 0; i < ny; i++) {
        // Mean of row i
        double mean = 0.0;
        for (int j = 0; j < nx; j++) {
            mean += data[j + i * nx];
        }
        mean /= static_cast<double>(nx);

        // Centered values + squared sum (all in double)
        double sum_sq = 0.0;
        std::vector<double> centered(nx);
        for (int j = 0; j < nx; j++) {
            double val = static_cast<double>(data[j + i * nx]) - mean;
            centered[j] = val;
            sum_sq += val * val;
        }

        // Normalize in double; store directly into the SIMD buffer.
        double norm = std::sqrt(sum_sq);
        double inv = (norm > 0.0) ? (1.0 / norm) : 0.0;
        double *row = reinterpret_cast<double *>(&nor_data[i * na]);
        for (int j = 0; j < nx; j++) {
            row[j] = centered[j] * inv;
        }
    }


    constexpr int nd = 3;

    // Vectorized dot products in double precision.
    #pragma omp parallel for
    for (int ic = 0; ic < ny; ic += nd) {
        for (int jc = 0; jc <= ic; jc += nd) {
            double8_t vsum[nd][nd];
            for (int id = 0; id < nd; ++id) {
                for (int jd = 0; jd < nd; ++jd) {
                    vsum[id][jd] = d8zero;
                }
            }


            for (int k = 0; k < na; k++) {
                double8_t x0 = (ic + 0 < ny) ? nor_data[(ic + 0) * na + k] : d8zero;
                double8_t x1 = (ic + 1 < ny) ? nor_data[(ic + 1) * na + k] : d8zero;
                double8_t x2 = (ic + 2 < ny) ? nor_data[(ic + 2) * na + k] : d8zero;

                double8_t y0 = (jc + 0 < ny) ? nor_data[(jc + 0) * na + k] : d8zero;
                double8_t y1 = (jc + 1 < ny) ? nor_data[(jc + 1) * na + k] : d8zero;
                double8_t y2 = (jc + 2 < ny) ? nor_data[(jc + 2) * na + k] : d8zero;

                vsum[0][0] += x0 * y0;  vsum[0][1] += x0 * y1;  vsum[0][2] += x0 * y2;
                vsum[1][0] += x1 * y0;  vsum[1][1] += x1 * y1;  vsum[1][2] += x1 * y2;
                vsum[2][0] += x2 * y0;  vsum[2][1] += x2 * y1;  vsum[2][2] += x2 * y2;
            }
            for (int id = 0; id < nd; ++id) {
                for (int jd = 0; jd < nd; ++jd) {
                        int i = ic + id;
                        int j = jc + jd;
                        
                        if (i < ny && j <= i) {
                            result[i + j * ny] = static_cast<float>(hsum8(vsum[id][jd]));
                        }
                    }
            }
        }
    }
}
