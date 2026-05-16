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

static inline double8_t load_partial(const float* base, int start, int nx) {
    double tmp[8] = {0};
    for (int i = 0; i < 8 && (start + i) < nx; ++i) {
        tmp[i] = static_cast<double>(base[start + i]);
    }
    double8_t res;
    for (int i = 0; i < 8; ++i) res[i] = tmp[i];
    return res;
}

void correlate(int ny, int nx, const float *data, float *result) {
    constexpr int nb = 8;
    int na = (nx + nb - 1) / nb;

    std::vector<double8_t> nor_data(static_cast<size_t>(ny) * na, d8zero);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < ny; i++) {
        const float* row_in = &data[i * nx];
        
        double sum = 0.0;
        for (int j = 0; j < nx; j++) {
            sum += static_cast<double>(row_in[j]);
        }
        double mean = sum / static_cast<double>(nx);

        double sum_sq = 0.0;
        for (int j = 0; j < nx; j++) {
            double centered = static_cast<double>(row_in[j]) - mean;
            sum_sq += centered * centered;
        }

        double norm = std::sqrt(sum_sq);
        double inv = (norm > 0.0) ? (1.0 / norm) : 0.0;

        double* row_out = reinterpret_cast<double*>(&nor_data[i * na]);
        for (int j = 0; j < nx; j++) {
            row_out[j] = (static_cast<double>(row_in[j]) - mean) * inv;
        }
    }
    constexpr int nd = 3;

    // Vectorized dot products in double precision.
    #pragma omp parallel for schedule(dynamic)
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
