#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>
#include <omp.h>
#include <time.h>

#define ITERATIONS 1000000000000ULL  // 1 Trillion
#define NUM_THREADS 4  // i3-7020U has 4 logical threads

void count_to_1_trillion() {
    uint64_t count[4] = {0, 0, 0, 0};  // 256-bit counter (64-bit x 4)
    __m256i counter = _mm256_setzero_si256();
    __m256i one = _mm256_set1_epi64x(1);

    #pragma omp parallel num_threads(NUM_THREADS)
    {
        uint64_t local_count = 0;
        for (uint64_t i = 0; i < ITERATIONS / NUM_THREADS; i += 4) {
            counter = _mm256_add_epi64(counter, one);
            local_count += 4;
        }
        #pragma omp critical
        count[0] += local_count;
    }

    // Print final count (converted to 256-bit hex)
    printf("Final Count: %016llx%016llx%016llx%016llx\n", count[3], count[2], count[1], count[0]);
}

int main() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    count_to_1_trillion();

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Time taken: %.6f seconds\n", time_taken);
    printf("Speed: %.2f billion increments per second\n", ITERATIONS / (time_taken * 1e9));

    return 0;
}
