#include <stdio.h>

#include "disk_monitor.h"
#include "system_monitor.h"
#include "system_status.h"

/*
 * 测试系统内存信息读取。
 */
static int test_memory_reading(void)
{
    MemoryInfo memory;

    if (get_memory_info(&memory) != 0) {
        fprintf(stderr, "get_memory_info failed\n");
        return -1;
    }

    if (memory.total_kb == 0) {
        fprintf(stderr, "memory total is zero\n");
        return -1;
    }

    if (memory.available_kb > memory.total_kb) {
        fprintf(
            stderr,
            "memory available exceeds total\n"
        );

        return -1;
    }

    if (
        memory.used_kb !=
        memory.total_kb - memory.available_kb
    ) {
        fprintf(stderr, "memory used value is incorrect\n");
        return -1;
    }

    if (
        memory.usage_percent < 0.0 ||
        memory.usage_percent > 100.0
    ) {
        fprintf(
            stderr,
            "invalid memory usage: %.2f\n",
            memory.usage_percent
        );

        return -1;
    }

    printf(
        "memory reading test passed: %.2f%% used\n",
        memory.usage_percent
    );

    return 0;
}

/*
 * 测试根文件系统的磁盘信息读取。
 */
static int test_disk_reading(void)
{
    DiskInfo disk;

    if (get_disk_info("/", &disk) != 0) {
        fprintf(stderr, "get_disk_info failed\n");
        return -1;
    }

    if (disk.total_bytes == 0) {
        fprintf(stderr, "disk total is zero\n");
        return -1;
    }

    if (disk.used_bytes > disk.total_bytes) {
        fprintf(stderr, "disk used exceeds total\n");
        return -1;
    }

    if (disk.available_bytes > disk.total_bytes) {
        fprintf(stderr, "disk available exceeds total\n");
        return -1;
    }

    if (
        disk.usage_percent < 0.0 ||
        disk.usage_percent > 100.0
    ) {
        fprintf(
            stderr,
            "invalid disk usage: %.2f\n",
            disk.usage_percent
        );

        return -1;
    }

    printf(
        "disk reading test passed: %.2f%% used\n",
        disk.usage_percent
    );

    return 0;
}

/*
 * 测试系统运行时间及时间换算。
 */
static int test_uptime_reading(void)
{
    unsigned long long uptime_seconds;
    SystemUptime uptime;

    if (get_system_uptime(&uptime_seconds) != 0) {
        fprintf(stderr, "get_system_uptime failed\n");
        return -1;
    }

    convert_uptime(uptime_seconds, &uptime);

    if (uptime.hours >= 24) {
        fprintf(stderr, "invalid uptime hours\n");
        return -1;
    }

    if (uptime.minutes >= 60) {
        fprintf(stderr, "invalid uptime minutes\n");
        return -1;
    }

    if (uptime.seconds >= 60) {
        fprintf(stderr, "invalid uptime seconds\n");
        return -1;
    }

    printf(
        "uptime reading test passed: "
        "%llu days %u:%u:%u\n",
        uptime.days,
        uptime.hours,
        uptime.minutes,
        uptime.seconds
    );

    return 0;
}

/*
 * 测试系统平均负载读取。
 */
static int test_load_average_reading(void)
{
    LoadAverage load;

    if (get_load_average(&load) != 0) {
        fprintf(stderr, "get_load_average failed\n");
        return -1;
    }

    if (
        load.one_minute < 0.0 ||
        load.five_minutes < 0.0 ||
        load.fifteen_minutes < 0.0
    ) {
        fprintf(stderr, "load average contains negative value\n");
        return -1;
    }

    printf(
        "load average test passed: %.2f %.2f %.2f\n",
        load.one_minute,
        load.five_minutes,
        load.fifteen_minutes
    );

    return 0;
}

/*
 * 测试当前时间读取。
 */
static int test_current_time_reading(void)
{
    CurrentTime current_time;

    if (get_current_time(&current_time) != 0) {
        fprintf(stderr, "get_current_time failed\n");
        return -1;
    }

    if (
        current_time.year < 1970 ||
        current_time.month < 1 ||
        current_time.month > 12 ||
        current_time.day < 1 ||
        current_time.day > 31 ||
        current_time.hour < 0 ||
        current_time.hour > 23 ||
        current_time.minute < 0 ||
        current_time.minute > 59 ||
        current_time.second < 0 ||
        current_time.second > 60
    ) {
        fprintf(stderr, "current time contains invalid values\n");
        return -1;
    }

    printf(
        "current time test passed: "
        "%04d-%02d-%02d %02d:%02d:%02d\n",
        current_time.year,
        current_time.month,
        current_time.day,
        current_time.hour,
        current_time.minute,
        current_time.second
    );

    return 0;
}


/*
 * 测试系统资源读取函数是否拒绝错误参数。
 */
static int test_invalid_resource_arguments(void)
{
    MemoryInfo memory;
    DiskInfo disk;

    if (get_memory_info(NULL) == 0) {
        fprintf(
            stderr,
            "get_memory_info accepted NULL\n"
        );

        return -1;
    }

    if (get_disk_info(NULL, &disk) == 0) {
        fprintf(
            stderr,
            "get_disk_info accepted NULL path\n"
        );

        return -1;
    }

    if (get_disk_info("/", NULL) == 0) {
        fprintf(
            stderr,
            "get_disk_info accepted NULL result\n"
        );

        return -1;
    }

    /*
     * 这个路径不存在，statvfs() 应该失败。
     */
    if (
        get_disk_info(
            "/path/that/does/not/exist",
            &disk
        ) == 0
    ) {
        fprintf(
            stderr,
            "get_disk_info accepted nonexistent path\n"
        );

        return -1;
    }

    if (get_system_uptime(NULL) == 0) {
        fprintf(
            stderr,
            "get_system_uptime accepted NULL\n"
        );

        return -1;
    }

    if (get_load_average(NULL) == 0) {
        fprintf(
            stderr,
            "get_load_average accepted NULL\n"
        );

        return -1;
    }

    if (get_current_time(NULL) == 0) {
        fprintf(
            stderr,
            "get_current_time accepted NULL\n"
        );

        return -1;
    }

    /*
     * convert_uptime() 返回 void。
     * 传入 NULL 时只需要确认不会崩溃。
     */
    convert_uptime(1000ULL, NULL);

    /*
     * 防止编译器认为 memory 没有被使用。
     */
    (void)memory;

    printf("invalid system resource argument tests passed\n");

    return 0;
}

int main(void)
{
    if (test_memory_reading() != 0) {
        return 1;
    }

    if (test_disk_reading() != 0) {
        return 1;
    }

    if (test_uptime_reading() != 0) {
        return 1;
    }

    if (test_load_average_reading() != 0) {
        return 1;
    }

    if (test_current_time_reading() != 0) {
        return 1;
    }

    if (test_invalid_resource_arguments() != 0) {
        fprintf(
            stderr,
            "invalid system resource argument tests failed\n"
        );

        return 1;
    }

    printf("all system resource smoke tests passed\n");

    return 0;
}
