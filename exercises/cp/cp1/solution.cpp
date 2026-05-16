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
void correlate(int ny, int nx, const float *data, float *result) {

    double* nor_data = new double[ny*nx];
    for (int i = 0; i < ny; i++) {
       
        // mean of i 
        double mean = 0.0f;
        for (int j = 0; j < nx; j++) {
            mean += data[j + i * nx];
        }
        mean /= nx;
        
        double sum_sq = 0.0f;
        for (int j = 0; j < nx; j++) {
            double val = data[j + i * nx] - mean;
            nor_data[j + i * nx] = val;
            sum_sq += val * val;
        }
        double norm = sqrt(sum_sq);
        for (int j = 0; j < nx; j++) {
            if (norm > 0.0f) {
                nor_data[j + i * nx] /= norm;
            } else {
                nor_data[j + i * nx] = 0.0f; 
            }
        }
    }
    
    
    for (int i = 0; i < ny; i++)    {
        for (int j = 0; j <= i; j++) {
            double sum = 0.0f;
            for (int k = 0; k < nx; k++) {
                sum += nor_data[k + i * nx] * nor_data[k + j * nx];
            }
            result[i + j * ny] = sum;
        }
    }
    delete[] nor_data;
}
