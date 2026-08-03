#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ALLOCATION_SIZE \
    (32UL * 1024UL * 1024UL)

#define MEMORY_PAGE_SIZE 4096UL

int main(void)
{
    volatile unsigned char *memory;
    size_t offset;

    /*
     * 先保持低内存占用，
     * 给 EdgeSentinel 留出建立 NORMAL 基准的时间。
     */
    sleep(4);

    memory = malloc(ALLOCATION_SIZE);

    if (memory == NULL)
    {
        perror("malloc");
        return 1;
    }

    /*
     * 逐页写入申请的内存。
     *
     * 只有实际访问内存页，
     * 这些内存才会进入进程的 VmRSS。
     */
    for (
        offset = 0;
        offset < ALLOCATION_SIZE;
        offset += MEMORY_PAGE_SIZE
    )
    {
        memory[offset] = 1;
    }

    /*
     * 保证最后一个字节也被实际访问。
     */
    memory[ALLOCATION_SIZE - 1] = 1;

    /*
     * 保持高内存状态，
     * 让 EdgeSentinel 有足够时间完成采样。
     */
    sleep(6);

    free((void *)memory);

    return 0;
}
