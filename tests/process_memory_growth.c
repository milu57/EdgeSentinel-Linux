#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MIB_BYTES (1024UL * 1024UL)
#define BLOCK_SIZE_MIB 20
#define BLOCK_COUNT 5

int main(void)
{
    void *blocks[BLOCK_COUNT] = {NULL};
    int index;

    printf("Memory test process started.\n");
    printf("PID: %d\n", (int)getpid());
    printf("Memory allocation will begin after 30 seconds.\n");
    fflush(stdout);

    /*
     * 留出时间，让我们把该 PID 写入配置文件，
     * 并启动 EdgeSentinel。
     */
    sleep(30);

    for (index = 0; index < BLOCK_COUNT; index++)
    {
        blocks[index] = malloc(
            BLOCK_SIZE_MIB * MIB_BYTES
        );

        if (blocks[index] == NULL)
        {
            fprintf(stderr, "Failed to allocate memory\n");
            return 1;
        }

        /*
         * malloc 只是申请地址空间。
         * memset 实际访问这些内存页，
         * 才能让 VmRSS 明显增加。
         */
        memset(
            blocks[index],
            1,
            BLOCK_SIZE_MIB * MIB_BYTES
        );

        printf(
            "Allocated approximately %d MiB in total.\n",
            (index + 1) * BLOCK_SIZE_MIB
        );

        fflush(stdout);
        sleep(10);
    }

    printf("All memory allocated. Process is now holding memory.\n");
    fflush(stdout);

    /*
     * 保持进程运行，方便 EdgeSentinel 持续监控。
     */
    while (1)
    {
        sleep(60);
    }

    return 0;
}
