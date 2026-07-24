#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

/*
 * 某个采样时刻的累计网络流量。
 */
typedef struct {
    uint64_t rx_bytes;  /* 累计接收字节数 */
    uint64_t tx_bytes;  /* 累计发送字节数 */
} NetworkInfo;

/*
 * 两次采样之间计算出的实时网速。
 *
 * 单位统一为 B/s。
 */
typedef struct {
    double download_bytes_per_sec;
    double upload_bytes_per_sec;
} NetworkSpeed;

/*
 * 自动转换单位后的显示结果。
 *
 * 例如：
 *     value = 2.50
 *     unit  = "KB/s"
 */
typedef struct {
    double value;
    const char *unit;
} NetworkSpeedDisplay;

/*
 * 读取所有非回环接口的累计网络流量。
 *
 * 成功返回 0，失败返回 -1。
 */
int read_network_info(NetworkInfo *info);

/*
 * 根据两次累计流量和时间间隔计算实时网速。
 */
int calculate_network_speed(
    const NetworkInfo *previous,
    const NetworkInfo *current,
    double elapsed_seconds,
    NetworkSpeed *speed
);

/*
 * 将 B/s 自动转换为 B/s、KB/s、MB/s 或 GB/s。
 */
int convert_network_speed(
    double bytes_per_second,
    NetworkSpeedDisplay *display
);

#endif
