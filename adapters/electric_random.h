#ifndef RF_ELECTRIC_RANDOM_H
#define RF_ELECTRIC_RANDOM_H

#include <stdint.h>
#include <stdlib.h>

/* Preserve the inherited six-pair distribution while making every partial
 * sum representable on LP64 and ILP32 hosts. */
static inline float rf_electric_gauss(int (*next_value)(void))
{
    int i;
    int64_t sum = 0;
    for (i = 0; i < 6; ++i)
        sum += (int64_t)next_value() - (int64_t)next_value();
    return (float)sum / ((float)RAND_MAX * 6.0f);
}

#endif
