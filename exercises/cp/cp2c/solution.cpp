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

    

    // Vectorized dot products in double precision.
    #pragma omp parallel for
    for (int i = 0; i < ny; i++) {
        for (int j = 0; j <= i; j++) {
            double8_t vsum = d8zero;
            const double8_t *ri = &nor_data[i * na];
            const double8_t *rj = &nor_data[j * na];
            for (int k = 0; k < na; k++) {
                vsum += ri[k] * rj[k];
            }
            result[i + j * ny] = static_cast<float>(hsum8(vsum));
        }
    }
}
