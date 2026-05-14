/*
This is the function you need to implement. Quick reference:
- input rows: 0 <= y < ny
- input columns: 0 <= x < nx
- element at row y and column x is stored in data[x + y*nx]
- the correlation between rows i and j has to be stored in result[i + j*ny]
- only elements with 0 <= j <= i < ny need to be filled
*/
void correlate(int ny, int nx, const float *data, float *result) {
    for (int i = 0; i < ny; i++)    {
        for (int j = 0; j <= i; j++) {
            float sum = 0.0f;
            for (int k = 0; k < nx; k++) {
                sum += data[k + i * nx] * data[k + j * nx];
            }
            result[i + j * ny] = sum;
        }
    }
}
