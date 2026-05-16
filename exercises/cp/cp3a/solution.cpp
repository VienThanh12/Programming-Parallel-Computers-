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
    // Giả sử bạn có hàm tạo double8_t từ mảng hoặc dùng intrinsic nạp dữ liệu
    // Ở đây viết dạng tường minh, trình biên dịch sẽ tự tối ưu thành lệnh vector
    double8_t res;
    for (int i = 0; i < 8; ++i) res[i] = tmp[i];
    return res;
}


void correlate(int ny, int nx, const float *data, float *result) {
    // Width of one SIMD lane group (8 doubles per double8_t).
    constexpr int nb = 8;
    // Number of double8_t vectors per padded row.
    int na = (nx + nb - 1) / nb;

    // Normalized rows, packed as double8_t. Padding lanes are zero so they
    // do not contribute to the dot product.
    std::vector<double8_t> nor_data(static_cast<size_t>(ny) * na, d8zero);

   // Hàm bổ trợ để nạp dữ liệu an toàn (tránh out-of-bounds) vào vector
// Nếu vượt quá nx, tự động điền số 0 (không làm ảnh hưởng đến phép cộng tổng)

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < ny; i++) {
        const float* row_in = &data[i * nx];
        double* row_out = reinterpret_cast<double*>(&nor_data[i * na]);

        // --- BƯỚC 1: TÍNH MEAN BẰNG VECTOR ---
        double8_t vmean = d8zero;
        // Chạy qua các khối 8 phần tử
        for (int j = 0; j < nx; j += 8) {
            if (j + 8 <= nx) {
                // Trường hợp lý tưởng: nạp thẳng 8 phần tử float, ép kiểu sang double
                // (Tùy thuộc vào cách bạn định nghĩa toán tử nạp, ví dụ dưới đây là tường minh)
                double8_t v;
                for(int b=0; b<8; ++b) v[b] = static_cast<double>(row_in[j + b]);
                vmean += v;
            } else {
                vmean += load_partial(row_in, j, nx);
            }
        }
        // Cộng ngang vector để lấy tổng số đơn lẻ (scalar)
        double mean = hsum8(vmean) / static_cast<double>(nx);
        double8_t vmean_broadcast; // Tạo một vector chứa toàn giá trị mean
        for(int b=0; b<8; ++b) vmean_broadcast[b] = mean;

        // --- BƯỚC 2: TÍNH SUM_SQ BẰNG VECTOR ---
        double8_t vsum_sq = d8zero;
        for (int j = 0; j < nx; j += 8) {
            double8_t v;
            if (j + 8 <= nx) {
                for(int b=0; b<8; ++b) v[b] = static_cast<double>(row_in[j + b]);
            } else {
                v = load_partial(row_in, j, nx);
            }
            
            double8_t vcentered = v - vmean_broadcast;
            
            // Nếu ở đoạn đuôi ma trận (phần dư), ta phải xóa các giá trị rác ngoài biên nx về 0
            // để không làm sai lệch tổng sum_sq
            if (j + 8 > nx) {
                for (int b = nx - j; b < 8; ++b) vcentered[b] = 0.0;
            }
            
            vsum_sq += vcentered * vcentered;
        }
        double sum_sq = hsum8(vsum_sq);

        // --- BƯỚC 3: CHUẨN HÓA VÀ GHI THẲNG VÀO BUFFER (VECTOR) ---
        double norm = std::sqrt(sum_sq);
        double inv = (norm > 0.0) ? (1.0 / norm) : 0.0;
        double8_t vinv_broadcast;
        for(int b=0; b<8; ++b) vinv_broadcast[b] = inv;

        // Duyệt lượt cuối để ghi kết quả (ghi đủ cho cả các phần padding của nor_data)
        // na là số lượng vector double8_t trên một hàng của nor_data
        for (int ka = 0; ka < na; ka++) {
            int j = ka * 8;
            double8_t v;
            if (j + 8 <= nx) {
                for(int b=0; b<8; ++b) v[b] = static_cast<double>(row_in[j + b]);
                double8_t vres = (v - vmean_broadcast) * vinv_broadcast;
                
                // Ép trình biên dịch ghi thẳng vector 512-bit vào RAM (vmoveapd)
                *reinterpret_cast<double8_t*>(&row_out[j]) = vres;
            } else {
                // Xử lý đoạn cuối hàng: vừa chuẩn hóa vừa điền 0.0 vào phần padding
                double8_t v_partial = load_partial(row_in, j, nx);
                double8_t vres = (v_partial - vmean_broadcast) * vinv_broadcast;
                
                // Phần dư vượt quá nx thì gán bằng 0.0 (Yêu cầu bắt buộc của Version 3 & 4)
                for (int b = nx - j; b < 8; ++b) {
                    vres[b] = 0.0;
                }
                *reinterpret_cast<double8_t*>(&row_out[j]) = vres;
            }
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
