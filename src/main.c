#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "system_monitor.h"

/*
 * 程序运行状态标志。
 *
 * 1：继续运行
 * 0：停止运行
 */
static volatile sig_atomic_t keep_running = 1;

/*
 * SIGINT 信号处理函数。
 *
 * 用户按下 Ctrl+C 时，
 * 操作系统会调用这个函数。
 */
static void handle_sigint(int signal_number)
{
    /*
     * 当前没有使用该参数，
     * 显式转换为 void 可以避免编译器警告。
     */
    (void)signal_number;

    keep_running = 0;
}

int main(void)
{
    MemoryInfo memory_info;
    const unsigned int interval_seconds = 2;

    /*
     * 定义一个信号处理配置对象。
     */
    struct sigaction action = {0};

    /*
     * 收到 SIGINT 时，
     * 调用 handle_sigint 函数。
     */
    action.sa_handler = handle_sigint;

    /*
     * 初始化信号屏蔽集合。
     */
    sigemptyset(&action.sa_mask);

    /*
     * 当前不启用额外选项。
     */
    action.sa_flags = 0;

    /*
     * 注册 SIGINT 信号处理函数。
     */
    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    printf("EdgeSentinel-Linux v0.2\n");
    printf("Sampling interval: %u seconds\n",
           interval_seconds);
    printf("Press Ctrl+C to stop.\n\n");

    /*
     * keep_running 为 1 时持续采样。
     *
     * Ctrl+C 会使 keep_running 变成 0，
     * 循环随后结束。
     */
    while (keep_running)
    {
        if (get_memory_info(&memory_info) != 0)
        {
            fprintf(stderr,
                    "Error: failed to read memory information.\n");

            return EXIT_FAILURE;
        }

        print_memory_info(&memory_info);

        sleep(interval_seconds);
    }

    printf("\nStopping EdgeSentinel-Linux...\n");
    printf("Monitor stopped safely.\n");

    return EXIT_SUCCESS;
}
