#include "system_status.h"

#include <stdio.h>

#include <time.h>
/*
 * 时间换算常量。
 */
#define SECONDS_PER_MINUTE 60ULL
#define SECONDS_PER_HOUR   3600ULL
#define SECONDS_PER_DAY    86400ULL

int get_system_uptime(
    unsigned long long *uptime_seconds
)
{
    FILE *file;
    double uptime_value;

    /*
     * 防止调用者传入空指针。
     */
    if (uptime_seconds == NULL)
    {
        return -1;
    }

    /*
     * 打开 Linux 提供的系统运行时间文件。
     */
    file = fopen("/proc/uptime", "r");

    if (file == NULL)
    {
        perror("fopen /proc/uptime");
        return -1;
    }

    /*
     * /proc/uptime 第一项带有小数，例如：
     *
     * 12534.72
     *
     * 因此先使用 double 接收。
     */
    if (fscanf(file, "%lf", &uptime_value) != 1)
    {
        fprintf(stderr, "Failed to parse /proc/uptime\n");
        fclose(file);
        return -1;
    }

    fclose(file);

    /*
     * 当前显示只需要完整秒数，
     * 因此去掉小数部分。
     */
    *uptime_seconds =
        (unsigned long long)uptime_value;

    return 0;
}

void convert_uptime(
    unsigned long long total_seconds,
    SystemUptime *uptime
)
{
    unsigned long long remaining_seconds;

    if (uptime == NULL)
    {
        return;
    }

    /*
     * 先计算完整天数。
     */
    uptime->days =
        total_seconds / SECONDS_PER_DAY;

    /*
     * % 表示取余数。
     *
     * 去掉完整天数以后，还剩多少秒。
     */
    remaining_seconds =
        total_seconds % SECONDS_PER_DAY;

    /*
     * 用剩余秒数计算小时。
     */
    uptime->hours =
        (unsigned int)(
            remaining_seconds / SECONDS_PER_HOUR
        );

    /*
     * 去掉完整小时后剩余的秒数。
     */
    remaining_seconds =
        remaining_seconds % SECONDS_PER_HOUR;

    /*
     * 计算分钟。
     */
    uptime->minutes =
        (unsigned int)(
            remaining_seconds / SECONDS_PER_MINUTE
        );

    /*
     * 最后的余数就是秒。
     */
    uptime->seconds =
        (unsigned int)(
            remaining_seconds % SECONDS_PER_MINUTE
        );
}


int get_load_average(LoadAverage *load)
{
    FILE *file;
    int matched;

    /*
     * 防止调用者传入空指针。
     */
    if (load == NULL)
    {
        return -1;
    }

    /*
     * 打开 Linux 的系统负载文件。
     */
    file = fopen("/proc/loadavg", "r");

    if (file == NULL)
    {
        perror("fopen /proc/loadavg");
        return -1;
    }

    /*
     * /proc/loadavg 开头三个数字分别是：
     *
     * 1 分钟、5 分钟、15 分钟平均负载。
     */
    matched = fscanf(
        file,
        "%lf %lf %lf",
        &load->one_minute,
        &load->five_minutes,
        &load->fifteen_minutes
    );

    fclose(file);

    /*
     * 必须成功读取三个数字。
     */
    if (matched != 3)
    {
        fprintf(stderr, "Failed to parse /proc/loadavg\n");
        return -1;
    }

    return 0;
}

int get_current_time(CurrentTime *current_time)
{
    time_t current_seconds;
    struct tm local_time;

    /*
     * 防止调用者传入空指针。
     */
    if (current_time == NULL)
    {
        return -1;
    }

    /*
     * time(NULL) 获取当前时间。
     *
     * 返回的是从 1970-01-01 00:00:00 UTC
     * 到当前时刻经过的秒数。
     */
    current_seconds = time(NULL);

    if (current_seconds == (time_t)-1)
    {
        perror("time");
        return -1;
    }

    /*
     * 将秒数转换成当前系统时区下的年月日和时分秒。
     *
     * localtime_r() 把结果写入 local_time。
     */
    if (localtime_r(&current_seconds, &local_time) == NULL)
    {
        perror("localtime_r");
        return -1;
    }

    /*
     * tm_year 表示从 1900 年开始经过了多少年。
     * 因此需要加 1900。
     */
    current_time->year = local_time.tm_year + 1900;

    /*
     * tm_mon 的范围是 0～11，
     * 因此需要加 1，转换成正常的 1～12 月。
     */
    current_time->month = local_time.tm_mon + 1;

    current_time->day = local_time.tm_mday;
    current_time->hour = local_time.tm_hour;
    current_time->minute = local_time.tm_min;
    current_time->second = local_time.tm_sec;

    return 0;
}
