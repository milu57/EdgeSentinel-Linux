#include <stdio.h>
#include <stdlib.h>
#include "system_monitor.h"

int main(void)
{
    MemoryInfo memory_info;

    printf("EdgeSentinel-Linux v0.1\n");

    if (get_memory_info(&memory_info) != 0)
    {
        fprintf(stderr,
                "Error: failed to read memory information.\n");

        return EXIT_FAILURE;
    }

    print_memory_info(&memory_info);

    return EXIT_SUCCESS;
}
