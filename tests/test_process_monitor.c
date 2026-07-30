#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "process_monitor.h"

/*
 * 计算两个单调时钟时间点之间经过的秒数。
 *
 * start：
 *     开始时间对象的地址。
 *
 * end：
 *     结束时间对象的地址。
 *
 * 返回值：
 *     两个时间点之间经过的秒数。
 */
static double calculate_elapsed_seconds(
    const struct timespec *start,
    const struct timespec *end
)
{
    double whole_seconds;
    double fractional_seconds;

    /*
     * 计算整秒部分的差值。
     */
    whole_seconds =
        (double)(end->tv_sec - start->tv_sec);

    /*
     * 计算纳秒部分的差值，
     * 再除以 10 亿，将纳秒转换成秒。
     */
    fractional_seconds =
        (double)(end->tv_nsec - start->tv_nsec)
        / 1000000000.0;

    /*
     * 整秒差值加上小数秒差值，
     * 得到完整的经过时间。
     */
    return whole_seconds + fractional_seconds;
}

/*
 * 持续执行 CPU 密集型计算。
 *
 * 这里不使用 sleep()，
 * 而是让当前进程不断进行加法运算，
 * 从而产生明显的 CPU 使用时间。
 *
 * duration_seconds：
 *     希望持续计算的时间，单位为秒。
 *
 * 返回值：
 *     成功返回 0；
 *     失败返回 -1。
 */
static int perform_cpu_work(double duration_seconds)
{
    struct timespec start_time;
    struct timespec current_time;

    /*
     * 保存计算结果。
     *
     * volatile 告诉编译器：
     * 这个变量的读写不能被随意优化掉。
     */
    volatile unsigned long long accumulator = 0;

    unsigned int number;

    if (duration_seconds <= 0.0) {
        return -1;
    }

    /*
     * 记录 CPU 密集计算开始时的时间。
     */
    if (
        clock_gettime(
            CLOCK_MONOTONIC,
            &start_time
        ) != 0
    ) {
        perror("clock_gettime");
        return -1;
    }

    do {
        /*
         * 每轮执行一批加法运算。
         */
        for (
            number = 0;
            number < 100000U;
            number++
        ) {
            accumulator += number;
        }

        /*
         * 获取当前时间，
         * 用来判断已经计算了多长时间。
         */
        if (
            clock_gettime(
                CLOCK_MONOTONIC,
                &current_time
            ) != 0
        ) {
            perror("clock_gettime");
            return -1;
        }

    } while (
        calculate_elapsed_seconds(
            &start_time,
            &current_time
        ) < duration_seconds
    );

    /*
     * 显式使用 accumulator，
     * 避免出现“变量未使用”的警告。
     */
    (void)accumulator;

    return 0;
}

static int test_find_process_by_name(void)
{
    int current_pid;
    int found_pid;
    ProcessInfo current_process;
    ProcessInfo found_process;

    current_pid = (int)getpid();

    if (read_process_info(current_pid, &current_process) != 0) {
        printf("[FAIL] Could not read current process information.\n");
        return -1;
    }

    printf("Current test process:\n");
    printf("  Name: %s\n", current_process.name);
    printf("  PID:  %d\n", current_pid);

    if (find_process_by_name(current_process.name, &found_pid) != 0) {
        printf("[FAIL] Could not find process by name: %s\n",
               current_process.name);
        return -1;
    }

    if (read_process_info(found_pid, &found_process) != 0) {
        printf("[FAIL] Could not read found process information.\n");
        return -1;
    }

    if (strcmp(found_process.name, current_process.name) != 0) {
        printf("[FAIL] Found process name does not match.\n");
        return -1;
    }

    printf("[PASS] Found process by name.\n");
    printf("  Name: %s\n", found_process.name);
    printf("  PID:  %d\n", found_pid);

    return 0;
}

int main(void)
{
    /*
     * 保存当前测试进程的基本信息。
     */
    ProcessInfo process_info;

    /*
     * 保存第一次和第二次读取到的
     * 进程累计 CPU 时间。
     */
    ProcessCpuTimes previous_cpu_times;
    ProcessCpuTimes current_cpu_times;

    /*
     * 保存两次 CPU 采样对应的实际时间。
     */
    struct timespec start_time;
    struct timespec end_time;

    /*
     * 两次采样之间实际经过的秒数。
     */
    double elapsed_seconds;

    /*
     * 最终计算得到的进程 CPU 使用率。
     */
    double process_cpu_usage;

    /*
     * getpid() 返回当前测试程序自己的 PID。
     */
    int current_pid = (int)getpid();

    printf("Testing process monitor...\n");
    printf(
        "Current test process PID: %d\n\n",
        current_pid
    );

    /*
     * 读取当前测试进程的基本信息。
     */
    if (
        read_process_info(
            current_pid,
            &process_info
        ) != 0
    ) {
        fprintf(
            stderr,
            "Failed to read process information for PID %d\n",
            current_pid
        );

        return 1;
    }

    /*
     * 输出当前测试进程的基本信息。
     */
    printf("Process information:\n");

    printf(
        "  PID:             %d\n",
        process_info.pid
    );

    printf(
        "  Parent PID:      %d\n",
        process_info.parent_pid
    );

    printf(
        "  Name:            %s\n",
        process_info.name
    );

    printf(
        "  State:           %s\n",
        process_info.state
    );

    printf(
        "  Resident memory: %lu kB\n\n",
        process_info.resident_memory_kb
    );

    /*
     * 记录第一次 CPU 采样对应的实际时间。
     */
    if (
        clock_gettime(
            CLOCK_MONOTONIC,
            &start_time
        ) != 0
    ) {
        perror("clock_gettime");
        return 1;
    }

    /*
     * 第一次读取进程累计 CPU tick。
     *
     * 这次读取的数据将作为基准值。
     */
    if (
        read_process_cpu_times(
            current_pid,
            &previous_cpu_times
        ) != 0
    ) {
        fprintf(
            stderr,
            "Failed to read previous CPU times for PID %d\n",
            current_pid
        );

        return 1;
    }

    printf(
        "Performing CPU-intensive work "
        "for about 2 seconds...\n"
    );

    /*
     * 连续执行约 2 秒 CPU 密集计算。
     */
    if (perform_cpu_work(2.0) != 0) {
        fprintf(
            stderr,
            "Failed to perform CPU work\n"
        );

        return 1;
    }

    /*
     * 第二次读取进程累计 CPU tick。
     */
    if (
        read_process_cpu_times(
            current_pid,
            &current_cpu_times
        ) != 0
    ) {
        fprintf(
            stderr,
            "Failed to read current CPU times for PID %d\n",
            current_pid
        );

        return 1;
    }

    /*
     * 记录第二次 CPU 采样对应的实际时间。
     */
    if (
        clock_gettime(
            CLOCK_MONOTONIC,
            &end_time
        ) != 0
    ) {
        perror("clock_gettime");
        return 1;
    }

    /*
     * 计算两次采样之间经过的真实时间。
     */
    elapsed_seconds =
        calculate_elapsed_seconds(
            &start_time,
            &end_time
        );

    /*
     * 使用前后两次累计 tick 和真实时间，
     * 计算当前测试进程的 CPU 使用率。
     */
    process_cpu_usage =
        calculate_process_cpu_usage(
            &previous_cpu_times,
            &current_cpu_times,
            elapsed_seconds
        );

    if (process_cpu_usage < 0.0) {
        fprintf(
            stderr,
            "Failed to calculate process CPU usage\n"
        );

        return 1;
    }

    /*
     * 输出两次 CPU 采样数据和计算结果。
     */
    printf("\nCPU sampling result:\n");

    printf(
        "  Previous user ticks:   %llu\n",
        previous_cpu_times.user_ticks
    );

    printf(
        "  Previous system ticks: %llu\n",
        previous_cpu_times.system_ticks
    );

    printf(
        "  Previous total ticks:  %llu\n",
        previous_cpu_times.user_ticks +
        previous_cpu_times.system_ticks
    );

    printf(
        "  Current user ticks:    %llu\n",
        current_cpu_times.user_ticks
    );

    printf(
        "  Current system ticks:  %llu\n",
        current_cpu_times.system_ticks
    );

    printf(
        "  Current total ticks:   %llu\n",
        current_cpu_times.user_ticks +
        current_cpu_times.system_ticks
    );

    printf(
        "  Elapsed time:          %.6f seconds\n",
        elapsed_seconds
    );

    printf(
        "  Process CPU usage:     %.2f%%\n",
        process_cpu_usage
    );


    printf("\nTesting process lookup by name...\n");

    if (test_find_process_by_name() != 0) {
        return 1;
    }

    return 0;
}

