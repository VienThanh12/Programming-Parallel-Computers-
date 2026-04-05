struct Result {
    float avg[3];
};

Result calculate(int ny, int nx, const float *data,
                 int y0, int x0, int y1, int x1) {
    double sum[3] = {0.0, 0.0, 0.0};
    int count = (y1 - y0) * (x1 - x0);

    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            for (int c = 0; c < 3; c++) {
                sum[c] += data[c + 3 * x + 3 * nx * y];
            }
        }
    }

    Result result;
    for (int c = 0; c < 3; c++) {
        result.avg[c] = static_cast<float>(sum[c] / count);
    }
    return result;
}
