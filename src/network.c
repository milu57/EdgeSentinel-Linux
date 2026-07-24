#include "network.h"

#include <stdio.h>
#include <string.h>

/*
 * 读取 /proc/net/dev 中所有非回环接口的累计网络流量。
 */
int read_network_info(NetworkInfo *info)
{
    FILE *file;
    char line[512];

    /*
     * 先把读取结果保存在局部变量中。
     * 整个读取过程成功后，再写入 info。
     */
    NetworkInfo result = {0, 0};

    if (info == NULL) {
        return -1;
    }

    file = fopen("/proc/net/dev", "r");

    if (file == NULL) {
        perror("fopen /proc/net/dev");
        return -1;
    }

    /*
     * 跳过 /proc/net/dev 的前两行标题。
     */
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return -1;
    }

    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return -1;
    }

    /*
     * 从第三行开始，每一行对应一个网络接口。
     */
    while (fgets(line, sizeof(line), file) != NULL) {
        char interface_name[32];
        unsigned long long fields[16];
        int matched;

        /*
         * 冒号前面读取接口名称。
         * 冒号后面读取16个无符号整数。
         */
        matched = sscanf(
            line,
            " %31[^:]:"
            " %llu %llu %llu %llu"
            " %llu %llu %llu %llu"
            " %llu %llu %llu %llu"
            " %llu %llu %llu %llu",
            interface_name,
            &fields[0],
            &fields[1],
            &fields[2],
            &fields[3],
            &fields[4],
            &fields[5],
            &fields[6],
            &fields[7],
            &fields[8],
            &fields[9],
            &fields[10],
            &fields[11],
            &fields[12],
            &fields[13],
            &fields[14],
            &fields[15]
        );

        /*
         * 接口名称1项，加上16个数字，共17项。
         */
        if (matched != 17) {
            continue;
        }

        /*
         * lo 是回环接口，只表示本机内部通信。
         */
        if (strcmp(interface_name, "lo") == 0) {
            continue;
        }

        /*
         * fields[0] 是接收字节数。
         * fields[8] 是发送字节数。
         */
        result.rx_bytes += (uint64_t)fields[0];
        result.tx_bytes += (uint64_t)fields[8];
    }

    fclose(file);

    /*
     * 将最终读取结果整体写入 info 指向的结构体。
     */
    *info = result;

    return 0;
}

/*
 * 根据两次累计流量计算实时上传和下载速度。
 */
int calculate_network_speed(
    const NetworkInfo *previous,
    const NetworkInfo *current,
    double elapsed_seconds,
    NetworkSpeed *speed
)
{
    uint64_t received_difference;
    uint64_t transmitted_difference;

    if (previous == NULL ||
        current == NULL ||
        speed == NULL ||
        elapsed_seconds <= 0.0) {
        return -1;
    }

    /*
     * 正常情况下，当前累计值应大于或等于上一次累计值。
     *
     * 如果接口重启，计数器可能清零。
     * 此时将差值设为0，避免无符号整数下溢。
     */
    if (current->rx_bytes >= previous->rx_bytes) {
        received_difference =
            current->rx_bytes - previous->rx_bytes;
    } else {
        received_difference = 0;
    }

    if (current->tx_bytes >= previous->tx_bytes) {
        transmitted_difference =
            current->tx_bytes - previous->tx_bytes;
    } else {
        transmitted_difference = 0;
    }

    /*
     * 实时速度 = 流量差值 ÷ 时间差
     */
    speed->download_bytes_per_sec =
        (double)received_difference / elapsed_seconds;

    speed->upload_bytes_per_sec =
        (double)transmitted_difference / elapsed_seconds;

    return 0;
}

/*
 * 自动选择合适的网速显示单位。
 */
int convert_network_speed(
    double bytes_per_second,
    NetworkSpeedDisplay *display
)
{
    const double kilobyte = 1024.0;
    const double megabyte = 1024.0 * 1024.0;
    const double gigabyte = 1024.0 * 1024.0 * 1024.0;

    if (display == NULL || bytes_per_second < 0.0) {
        return -1;
    }

    if (bytes_per_second >= gigabyte) {
        display->value = bytes_per_second / gigabyte;
        display->unit = "GB/s";
    } else if (bytes_per_second >= megabyte) {
        display->value = bytes_per_second / megabyte;
        display->unit = "MB/s";
    } else if (bytes_per_second >= kilobyte) {
        display->value = bytes_per_second / kilobyte;
        display->unit = "KB/s";
    } else {
        display->value = bytes_per_second;
        display->unit = "B/s";
    }

    return 0;
}
