#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

#include "network.h"

/*
 * 计算两个时间点之间经过的秒数。
 */
static double calculate_elapsed_seconds(
    const struct timespec *start,
    const struct timespec *end
)
{
    double seconds;
    double nanoseconds;

    seconds = (double)(end->tv_sec - start->tv_sec);

    nanoseconds =
        (double)(end->tv_nsec - start->tv_nsec)
        / 1000000000.0;

    return seconds + nanoseconds;
}

int main(void)
{
    NetworkInfo previous;
    NetworkInfo current;

    NetworkSpeed speed;

    NetworkSpeedDisplay download_display;
    NetworkSpeedDisplay upload_display;

    struct timespec previous_time;
    struct timespec current_time;
    struct timespec sleep_time;

    double elapsed_seconds;

    /*
     * 先读取第一次累计流量。
     *
     * 第一次读取只能作为计算基准，
     * 还不能单独计算实时网速。
     */
    if (read_network_info(&previous) != 0) {
        fprintf(stderr, "第一次读取网络流量失败\n");
        return 1;
    }

    /*
     * 记录第一次采样对应的时间。
     */
    if (clock_gettime(CLOCK_MONOTONIC, &previous_time) != 0) {
        perror("clock_gettime");
        return 1;
    }

    sleep_time.tv_sec = 1;
    sleep_time.tv_nsec = 0;

    printf("开始实时网络监控，按 Ctrl+C 停止。\n\n");

    while (1) {
        /*
         * 等待约1秒后进行下一次采样。
         */
        if (nanosleep(&sleep_time, NULL) != 0) {
            perror("nanosleep");
            return 1;
        }

        /*
         * 读取当前累计流量。
         */
        if (read_network_info(&current) != 0) {
            fprintf(stderr, "读取当前网络流量失败\n");
            return 1;
        }

        /*
         * 记录当前采样时间。
         */
        if (clock_gettime(CLOCK_MONOTONIC, &current_time) != 0) {
            perror("clock_gettime");
            return 1;
        }

        /*
         * 计算本轮两次采样之间的真实时间差。
         */
        elapsed_seconds =
            calculate_elapsed_seconds(
                &previous_time,
                &current_time
            );

        /*
         * 根据累计流量差值计算实时速度。
         */
        if (calculate_network_speed(
                &previous,
                &current,
                elapsed_seconds,
                &speed
            ) != 0) {
            fprintf(stderr, "计算实时网速失败\n");
            return 1;
        }

        /*
         * 转换下载速度显示单位。
         */
        if (convert_network_speed(
                speed.download_bytes_per_sec,
                &download_display
            ) != 0) {
            fprintf(stderr, "转换下载速度单位失败\n");
            return 1;
        }

        /*
         * 转换上传速度显示单位。
         */
        if (convert_network_speed(
                speed.upload_bytes_per_sec,
                &upload_display
            ) != 0) {
            fprintf(stderr, "转换上传速度单位失败\n");
            return 1;
        }

        printf(
            "下载：%8.2f %-4s | 上传：%8.2f %-4s"
            " | RX：%llu bytes | TX：%llu bytes\n",
            download_display.value,
            download_display.unit,
            upload_display.value,
            upload_display.unit,
            (unsigned long long)current.rx_bytes,
            (unsigned long long)current.tx_bytes
        );

        /*
         * 本轮的 current 将成为下一轮的 previous。
         *
         * 例如：
         *
         * 第1轮：样本A和样本B计算
         * 第2轮：样本B和样本C计算
         * 第3轮：样本C和样本D计算
         */
        previous = current;
        previous_time = current_time;
    }

    return 0;
}
