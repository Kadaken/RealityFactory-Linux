// Measurement-only link wrappers. Values are forwarded unchanged; no clocks
// or seeds are replaced. Never linked into the production executable.
#include "rf_platform_compat.h"
#include <time.h>
#include <pthread.h>
#include <stdio.h>

struct Site { const char *clock; void *caller; unsigned long calls; };
static Site sites[1024];
static unsigned count;
static bool active;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void record(const char *clock, void *caller)
{
    pthread_mutex_lock(&lock);
    if (active) {
        unsigned i;
        for (i = 0; i < count; ++i)
            if (sites[i].caller == caller && sites[i].clock == clock) break;
        if (i == count && count < sizeof(sites)/sizeof(sites[0])) {
            sites[count].clock = clock; sites[count].caller = caller; ++count;
        }
        if (i < count) ++sites[i].calls;
    }
    pthread_mutex_unlock(&lock);
}
extern "C" bool __real__Z20RF_BootGameBeginTickv(void);
extern "C" bool __wrap__Z20RF_BootGameBeginTickv(void)
{
    const bool result = __real__Z20RF_BootGameBeginTickv();
    pthread_mutex_lock(&lock); active = result; pthread_mutex_unlock(&lock);
    return result;
}
extern "C" bool __real__Z18RF_BootGameEndTickv(void);
extern "C" bool __wrap__Z18RF_BootGameEndTickv(void)
{
    pthread_mutex_lock(&lock); active = false; pthread_mutex_unlock(&lock);
    return __real__Z18RF_BootGameEndTickv();
}
extern "C" DWORD __real_timeGetTime(void);
extern "C" DWORD __wrap_timeGetTime(void)
{ record("timeGetTime", __builtin_return_address(0)); return __real_timeGetTime(); }
extern "C" time_t __real_time(time_t *);
extern "C" time_t __wrap_time(time_t *p)
{ record("time", __builtin_return_address(0)); return __real_time(p); }
extern "C" clock_t __real_clock(void);
extern "C" clock_t __wrap_clock(void)
{ record("clock", __builtin_return_address(0)); return __real_clock(); }
extern "C" DWORD __real__ZN11CCommonData18FreeRunningCounterEv(void *);
extern "C" DWORD __wrap__ZN11CCommonData18FreeRunningCounterEv(void *self)
{
    record("FreeRunningCounter", __builtin_return_address(0));
    return __real__ZN11CCommonData18FreeRunningCounterEv(self);
}
extern "C" float __real__ZN11CCommonData20FreeRunningCounter_FEv(void *);
extern "C" float __wrap__ZN11CCommonData20FreeRunningCounter_FEv(void *self)
{
    record("FreeRunningCounter_F", __builtin_return_address(0));
    return __real__ZN11CCommonData20FreeRunningCounter_FEv(self);
}

struct ClockReceipt {
    ~ClockReceipt() {
        for (unsigned i = 0; i < count; ++i)
            fprintf(stderr, "RF frame clock: api=%s caller=%p calls=%lu\n",
                sites[i].clock, sites[i].caller, sites[i].calls);
        fprintf(stderr, "RF frame clock: unique_sites=%u capacity=%lu\n", count,
            (unsigned long)(sizeof(sites)/sizeof(sites[0])));
    }
};
static ClockReceipt receipt;
