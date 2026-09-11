#include "electric_random.h"
#include <cmath>
#include <cstdio>

static int position;
static int values[12];
static int controlled_rand(void) { return values[position++]; }

static float legacy_safe_result(void)
{
    int sum = 0;
    int i;
    for (i = 0; i < 6; ++i)
        sum += values[i * 2] - values[i * 2 + 1];
    return (float)sum / ((float)RAND_MAX * 6.0f);
}

int main(void)
{
    int i;
    float actual, expected;
    for (i = 0; i < 12; ++i)
        values[i] = (i & 1) ? 0 : RAND_MAX;
    position = 0;
    actual = rf_electric_gauss(controlled_rand);
    if (position != 12 || actual < -1.0f || actual > 1.0f || actual != 1.0f)
        return 1;

    for (i = 0; i < 12; ++i)
        values[i] = 1000 + ((i & 1) ? i * 3 : i * 5);
    expected = legacy_safe_result();
    position = 0;
    actual = rf_electric_gauss(controlled_rand);
    if (position != 12 || actual != expected)
        return 1;
    std::fprintf(stderr, "electric gaussian accumulator: PASS\n");
    return 0;
}
