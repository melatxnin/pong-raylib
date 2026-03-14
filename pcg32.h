#ifndef PCG32_H
#define PCG32_H

#include <stdint.h>

void pcg_init(void);

void pcg_seed(uint64_t initstate, uint64_t initseq);

uint32_t pcg32(void);

uint32_t pcg_range_uint(uint32_t max);

int pcg_range_int(int max);

#endif
