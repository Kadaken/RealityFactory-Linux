#include <stddef.h>
#include <stdio.h>
#include "rf_platform_compat.h"
struct BeforeHeader { char tag; void *pointer; };
#include <FreeImage.h>
struct AfterHeader { char tag; void *pointer; };
int main(void)
{
    fprintf(stderr, "header packing: pointer offsets %zu / %zu, sizes %zu / %zu\n",
            offsetof(BeforeHeader, pointer), offsetof(AfterHeader, pointer),
            sizeof(BeforeHeader), sizeof(AfterHeader));
    return sizeof(BeforeHeader) != sizeof(AfterHeader) ||
           offsetof(BeforeHeader, pointer) != offsetof(AfterHeader, pointer);
}
