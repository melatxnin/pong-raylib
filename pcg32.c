#include "pcg32.h"

#include <stdint.h>
#include <time.h>
#include <inttypes.h>

#ifdef _WIN32
    #include <windows.h>
    #define GETPID() GetCurrentProcessId()
#else
    #include <unistd.h>
    #define GETPID() getpid()
#endif

static uint64_t pcg_state;
static uint64_t pcg_inc;

void pcg_init(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);

    uint64_t initstate =
        ((uint64_t)ts.tv_sec << 32) ^
        (uint64_t)ts.tv_nsec;

    uint64_t initseq =
        ((uint64_t)GETPID() << 32) ^
        (uint64_t)clock();

    pcg_seed(initstate, initseq);
}

void pcg_seed(uint64_t initstate, uint64_t initseq)
{
    pcg_state = 0u;
    pcg_inc = (initseq << 1u) | 1u;
    pcg32();
    pcg_state += initstate;
    pcg32();
}

uint32_t pcg32(void)
{
    uint64_t oldstate = pcg_state;

    pcg_state = oldstate * 6364136223846793005ULL + pcg_inc;
    
    uint32_t xorshifted =
        (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);

    uint32_t rot = (uint32_t)(oldstate >> 59u);

    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

uint32_t pcg_range_uint(uint32_t max)
{
    if (max == 0)
        return 0;

    uint32_t threshold = (-max) % max;
    uint32_t r;

    do
    {
        r = pcg32();
    } while (r < threshold);
    
    return r % max;
}

int pcg_range_int(int max)
{
    if (max <= 0)
        return 0;

    return (int)pcg_range_uint((uint32_t)max);
}
