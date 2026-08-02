#include <stdio.h>
#include <string.h>

#include "cpu_monitor.h"
#include "network.h"

/*
 * 判断两个 double 数值是否足够接近。
 */
static int double_is_close(
    double actual,
    double expected
)
{
    double difference = actual - expected;

    if (difference < 0.0) {
        difference = -difference;
    }

    return difference < 0.0001;
}

/*
 * 测试 CPU 使用率计算。
 */
static int test_cpu_usage_calculation(void)
{
    /*
     * previous 总时间为 1000：
     *
     * busy = 200
     * idle + iowait = 800
     */
    CpuTimes previous = {
        100ULL,
        0ULL,
        100ULL,
        700ULL,
        100ULL,
        0ULL,
        0ULL,
        0ULL
    };

    /*
     * current 总时间为 1200。
     *
     * 两次采样之间：
     * total_difference = 200
     * idle_difference  = 100
     *
     * CPU 使用率：
     * (200 - 100) / 200 × 100% = 50%
     */
    CpuTimes current = {
        150ULL,
        0ULL,
        150ULL,
        750ULL,
        150ULL,
        0ULL,
        0ULL,
        0ULL
    };

    double usage;

    usage = calculate_cpu_usage(
        &previous,
        &current
    );

    if (!double_is_close(usage, 50.0)) {
        fprintf(
            stderr,
            "unexpected CPU usage: %.2f\n",
            usage
        );

        return -1;
    }

    /*
     * 两次数据完全相同时，总差值为 0，
     * 函数应返回 0%，不能发生除零。
     */
    usage = calculate_cpu_usage(
        &previous,
        &previous
    );

    if (!double_is_close(usage, 0.0)) {
        fprintf(
            stderr,
            "zero CPU difference returned %.2f\n",
            usage
        );

        return -1;
    }

    if (
        calculate_cpu_usage(
            NULL,
            &current
        ) != -1.0
    ) {
        fprintf(
            stderr,
            "CPU calculation accepted NULL previous sample\n"
        );

        return -1;
    }

    if (
        calculate_cpu_usage(
            &previous,
            NULL
        ) != -1.0
    ) {
        fprintf(
            stderr,
            "CPU calculation accepted NULL current sample\n"
        );

        return -1;
    }

    printf("CPU usage calculation tests passed\n");

    return 0;
}

/*
 * 测试网络速度计算。
 */
static int test_network_speed_calculation(void)
{
    NetworkInfo previous = {
        1000ULL,
        2000ULL
    };

    NetworkInfo current = {
        5000ULL,
        6000ULL
    };

    NetworkInfo reset_counter = {
        500ULL,
        1000ULL
    };

    NetworkSpeed speed;

    /*
     * 接收和发送都增加 4000 字节，
     * 经过时间为 2 秒，因此速度都是 2000 B/s。
     */
    if (
        calculate_network_speed(
            &previous,
            &current,
            2.0,
            &speed
        ) != 0
    ) {
        fprintf(
            stderr,
            "calculate_network_speed failed\n"
        );

        return -1;
    }

    if (
        !double_is_close(
            speed.download_bytes_per_sec,
            2000.0
        ) ||
        !double_is_close(
            speed.upload_bytes_per_sec,
            2000.0
        )
    ) {
        fprintf(
            stderr,
            "unexpected network speed: %.2f %.2f\n",
            speed.download_bytes_per_sec,
            speed.upload_bytes_per_sec
        );

        return -1;
    }

    /*
     * 当前累计值小于上一次时，表示网络计数器可能重置。
     * 函数应把速度设为 0，避免无符号整数下溢。
     */
    if (
        calculate_network_speed(
            &previous,
            &reset_counter,
            1.0,
            &speed
        ) != 0
    ) {
        fprintf(
            stderr,
            "network counter reset calculation failed\n"
        );

        return -1;
    }

    if (
        !double_is_close(
            speed.download_bytes_per_sec,
            0.0
        ) ||
        !double_is_close(
            speed.upload_bytes_per_sec,
            0.0
        )
    ) {
        fprintf(
            stderr,
            "counter reset did not produce zero speed\n"
        );

        return -1;
    }

    if (
        calculate_network_speed(
            &previous,
            &current,
            0.0,
            &speed
        ) == 0
    ) {
        fprintf(
            stderr,
            "network calculation accepted zero elapsed time\n"
        );

        return -1;
    }

    if (
        calculate_network_speed(
            NULL,
            &current,
            1.0,
            &speed
        ) == 0
    ) {
        fprintf(
            stderr,
            "network calculation accepted NULL sample\n"
        );

        return -1;
    }

    printf("network speed calculation tests passed\n");

    return 0;
}

/*
 * 检查一次网速单位转换。
 */
static int check_speed_conversion(
    double input,
    double expected_value,
    const char *expected_unit
)
{
    NetworkSpeedDisplay display;

    if (
        convert_network_speed(
            input,
            &display
        ) != 0
    ) {
        fprintf(
            stderr,
            "speed conversion failed for %.2f\n",
            input
        );

        return -1;
    }

    if (
        !double_is_close(
            display.value,
            expected_value
        ) ||
        strcmp(
            display.unit,
            expected_unit
        ) != 0
    ) {
        fprintf(
            stderr,
            "unexpected conversion: %.2f %s\n",
            display.value,
            display.unit
        );

        return -1;
    }

    return 0;
}

/*
 * 测试 B/s、KB/s、MB/s 和 GB/s 转换。
 */
static int test_network_speed_conversion(void)
{
    if (
        check_speed_conversion(
            500.0,
            500.0,
            "B/s"
        ) != 0
    ) {
        return -1;
    }

    if (
        check_speed_conversion(
            2048.0,
            2.0,
            "KB/s"
        ) != 0
    ) {
        return -1;
    }

    if (
        check_speed_conversion(
            3.0 * 1024.0 * 1024.0,
            3.0,
            "MB/s"
        ) != 0
    ) {
        return -1;
    }

    if (
        check_speed_conversion(
            2.0 * 1024.0 * 1024.0 * 1024.0,
            2.0,
            "GB/s"
        ) != 0
    ) {
        return -1;
    }

    if (
        convert_network_speed(
            -1.0,
            &(NetworkSpeedDisplay){0}
        ) == 0
    ) {
        fprintf(
            stderr,
            "speed conversion accepted negative value\n"
        );

        return -1;
    }

    if (
        convert_network_speed(
            1000.0,
            NULL
        ) == 0
    ) {
        fprintf(
            stderr,
            "speed conversion accepted NULL result\n"
        );

        return -1;
    }

    printf("network speed conversion tests passed\n");

    return 0;
}

/*
 * 测试根据配置过滤网络接口的读取逻辑。
 */
static int test_filtered_network_reading(void)
{
    AppConfig config;
    NetworkInfo loopback_info;
    NetworkInfo missing_interface_info;

    /*
     * 明确选择 lo 时，
     * read_network_info_filtered() 应正常读取。
     */
    config_set_defaults(&config);

    config.network_interface_count = 1;

    snprintf(
        config.network_interfaces[0],
        sizeof(config.network_interfaces[0]),
        "%s",
        "lo"
    );

    if (
        read_network_info_filtered(
            &config,
            &loopback_info
        ) != 0
    ) {
        fprintf(
            stderr,
            "failed to read selected loopback interface\n"
        );

        return -1;
    }

    /*
     * 配置一个不存在的接口时，
     * 读取函数本身仍然成功，
     * 但累计接收和发送字节数都应为 0。
     */
    config_set_defaults(&config);

    config.network_interface_count = 1;

    snprintf(
        config.network_interfaces[0],
        sizeof(config.network_interfaces[0]),
        "%s",
        "edgesentinel_missing0"
    );

    if (
        read_network_info_filtered(
            &config,
            &missing_interface_info
        ) != 0
    ) {
        fprintf(
            stderr,
            "failed to process a missing network interface\n"
        );

        return -1;
    }

    if (
        missing_interface_info.rx_bytes != 0 ||
        missing_interface_info.tx_bytes != 0
    ) {
        fprintf(
            stderr,
            "missing network interface produced traffic: "
            "rx=%llu tx=%llu\n",
            (unsigned long long)
                missing_interface_info.rx_bytes,
            (unsigned long long)
                missing_interface_info.tx_bytes
        );

        return -1;
    }

    /*
     * 接口数量超过配置数组容量时，
     * 读取函数必须拒绝该配置。
     */
    config.network_interface_count =
        CONFIG_MAX_NETWORK_INTERFACES + 1;

    if (
        read_network_info_filtered(
            &config,
            &missing_interface_info
        ) == 0
    ) {
        fprintf(
            stderr,
            "filtered network reading accepted excessive "
            "interface count\n"
        );

        return -1;
    }

    printf(
        "filtered network reading tests passed: "
        "lo_rx=%llu lo_tx=%llu\n",
        (unsigned long long)loopback_info.rx_bytes,
        (unsigned long long)loopback_info.tx_bytes
    );

    return 0;
}

/*
 * 验证 CPU 和网络累计数据能从当前 Linux 系统读取。
 */
static int test_live_cpu_and_network_reading(void)
{
    CpuTimes cpu_times;
    NetworkInfo network_info;

    if (read_cpu_times(&cpu_times) != 0) {
        fprintf(stderr, "read_cpu_times failed\n");
        return -1;
    }

    if (read_network_info(&network_info) != 0) {
        fprintf(stderr, "read_network_info failed\n");
        return -1;
    }

    if (read_cpu_times(NULL) == 0) {
        fprintf(stderr, "read_cpu_times accepted NULL\n");
        return -1;
    }

    if (read_network_info(NULL) == 0) {
        fprintf(stderr, "read_network_info accepted NULL\n");
        return -1;
    }

    printf(
        "live CPU and network reading test passed: "
        "rx=%llu tx=%llu\n",
        (unsigned long long)network_info.rx_bytes,
        (unsigned long long)network_info.tx_bytes
    );

    return 0;
}

int main(void)
{
    if (test_cpu_usage_calculation() != 0) {
        return 1;
    }

    if (test_network_speed_calculation() != 0) {
        return 1;
    }

    if (test_network_speed_conversion() != 0) {
        return 1;
    }

    if (test_filtered_network_reading() != 0) {
        return 1;
    }

    if (test_live_cpu_and_network_reading() != 0) {
        return 1;
    }

    printf("all calculation tests passed\n");

    return 0;
}
