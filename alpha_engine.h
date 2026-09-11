#pragma once
#include <immintrin.h>

class VectorizedAlphaEngine {
public:
    static inline double compute_order_flow_imbalance(const double* volumes, const double* prices) {
        __m256d v_vol = _mm256_loadu_pd(volumes);
        __m256d v_price = _mm256_loadu_pd(prices);
        __m256d v_product = _mm256_mul_pd(v_vol, v_price);

        double result[4];
        _mm256_storeu_pd(result, v_product);

        return result[0] + result[1] + result[2] + result[3];
    }
};