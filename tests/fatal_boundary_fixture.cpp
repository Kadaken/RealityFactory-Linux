#include "rf_platform_compat.h"
#include <pthread.h>

static bool reenter;
extern "C" int RF_HostBootStage(void) { return 2; }
extern "C" int RF_HostFatalShutdown(void)
{
    if (reenter) exit(9);
    return 1;
}
static void *lose_allocation(void *)
{
    volatile char *memory = (volatile char *)malloc(8192);
    if (memory) memory[0] = 1;
    return NULL;
}
int main(int argc, char **argv)
{
    if (argc != 2) return 3;
    reenter = !strcmp(argv[1], "reentry");
    if (!strcmp(argv[1], "leak")) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, lose_allocation, NULL) || pthread_join(thread, NULL)) return 3;
    }
    exit(7);
}
