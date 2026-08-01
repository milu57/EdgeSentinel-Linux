#include <stdio.h>
#include <string.h>

#include "config.h"

/*
 * 测试 config_set_defaults()。
 *
 * 目标：
 *     确认 AppConfig 中的所有主要成员
 *     都被设置成预期的默认值。
 *
 * 返回值：
 *     测试通过返回 0；
 *     测试失败返回 -1。
 */
static int test_config_defaults(void)
{
    AppConfig config;
    unsigned int process_index;

    /*
     * 调用被测试函数，
     * 为 config 设置默认配置。
     */
    config_set_defaults(&config);

    if (config.monitor_interval != 1) {
        fprintf(
            stderr,
            "unexpected monitor_interval: %u\n",
            config.monitor_interval
        );

        return -1;
    }

    if (config.process_pid != 0) {
        fprintf(
            stderr,
            "unexpected process_pid: %u\n",
            config.process_pid
        );

        return -1;
    }

    if (config.process_name[0] != '\0') {
        fprintf(
            stderr,
            "process_name should be empty: %s\n",
/*
 * 测试 process_names 中包含空名称时，
 * config_load() 是否拒绝配置。
 *
 * sleep,,bash 中两个逗号之间没有内容，
 * 因此中间存在一个非法的空进程名称。
 */
            config.process_name
        );

        return -1;
    }

    if (config.process_name_count != 0) {
        fprintf(
            stderr,
            "unexpected process_name_count: %u\n",
            config.process_name_count
        );

        return -1;
    }

    /*
     * 默认情况下，
     * process_names 数组中的每个名称都应该为空。
     */
    for (
        process_index = 0;
        process_index < CONFIG_MAX_PROCESS_NAMES;
        process_index++
    ) {
        if (
            config.process_names[process_index][0] != '\0'
        ) {
            fprintf(
                stderr,
                "process_names[%u] should be empty\n",
                process_index
            );

            return -1;
        }
    }

    if (config.cpu_warning_threshold != 70.0) {
        fprintf(
            stderr,
            "unexpected CPU warning threshold: %.2f\n",
            config.cpu_warning_threshold
        );

        return -1;
    }

    if (config.cpu_critical_threshold != 90.0) {
        fprintf(
            stderr,
            "unexpected CPU critical threshold: %.2f\n",
            config.cpu_critical_threshold
        );

        return -1;
    }

    if (
        config.process_cpu_warning_threshold != 70.0
    ) {
        fprintf(
            stderr,
            "unexpected process CPU warning threshold: %.2f\n",
            config.process_cpu_warning_threshold
        );

        return -1;
    }

    if (
        config.process_cpu_critical_threshold != 90.0
    ) {
        fprintf(
            stderr,
            "unexpected process CPU critical threshold: %.2f\n",
            config.process_cpu_critical_threshold
        );

        return -1;
    }
/*
 * 测试 process_names 中包含空名称时，
 * config_load() 是否拒绝配置。
 *
 * sleep,,bash 中两个逗号之间没有内容，
 * 因此中间存在一个非法的空进程名称。
 */
    if (
        config.process_memory_warning_threshold_mib != 100.0
    ) {
        fprintf(
            stderr,
            "unexpected process memory warning threshold: %.2f\n",
            config.process_memory_warning_threshold_mib
        );

        return -1;
    }

    if (
        config.process_memory_critical_threshold_mib != 200.0
    ) {
        fprintf(
            stderr,
            "unexpected process memory critical threshold: %.2f\n",
            config.process_memory_critical_threshold_mib
        );
/*
 * 测试 process_names 中包含空名称时，
 * config_load() 是否拒绝配置。
 *
 * sleep,,bash 中两个逗号之间没有内容，
 * 因此中间存在一个非法的空进程名称。
 */
        return -1;
    }

    if (config.memory_warning_threshold != 75.0) {
        fprintf(
            stderr,
            "unexpected memory warning threshold: %.2f\n",
            config.memory_warning_threshold
        );

        return -1;
    }

    if (config.memory_critical_threshold != 90.0) {
        fprintf(
            stderr,
            "unexpected memory critical threshold: %.2f\n",
            config.memory_critical_threshold
        );

        return -1;
    }

    if (config.disk_warning_threshold != 80.0) {
        fprintf(
/*
 * 测试 process_name_count 超过数组容量时，
 * config_validate() 是否拒绝配置。
 */
            stderr,
            "unexpected disk warning threshold: %.2f\n",
            config.disk_warning_threshold
        );

        return -1;
    }

    if (config.disk_critical_threshold != 90.0) {
        fprintf(
            stderr,
            "unexpected disk critical threshold: %.2f\n",
            config.disk_critical_threshold
        );

        return -1;
    }

    if (
        strcmp(
            config.log_file,
            "logs/edgesentinel.log"
        ) != 0
    ) {
/*
 * 测试 process_names 以逗号开头或结尾时，
 * config_load() 是否拒绝配置。
 *
 * ,sleep 表示第一个名称为空；
 * sleep, 表示最后一个名称为空。
 */
        fprintf(
            stderr,
            "unexpected log file: %s\n",
            config.log_file
        );

        return -1;
    }

    if (config.log_max_size != 1048576UL) {
        fprintf(
            stderr,
            "unexpected log_max_size: %lu\n",
            config.log_max_size
        );

        return -1;
    }

    printf("configuration defaults test passed\n");

    return 0;
}

/*
 * 测试从配置文件读取多个进程名称。
 *
 * 配置内容：
 *     process_names=sleep,bash,tail
 *
 * 预期结果：
 *     process_name_count 等于 3；
 *     三个数组元素分别为 sleep、bash 和 tail；
 *     兼容字段 process_name 保存第一个名称 sleep。
 */
static int test_multiple_process_names(void)
{
    const char *test_filename =
        "/tmp/edgesentinel_test_config.conf";

    FILE *file;
    AppConfig config;

    /*
     * 创建临时配置文件。
     */
    file = fopen(test_filename, "w");

    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    if (
        fprintf(
            file,
            "monitor_interval=3\n"
            "process_names=sleep,bash,tail\n"
        ) < 0
    ) {
        fprintf(
            stderr,
            "failed to write temporary configuration file\n"
        );

        fclose(file);
        remove(test_filename);

        return -1;
    }

    if (fclose(file) != 0) {
        perror("fclose");
        remove(test_filename);
/*
 * 测试 process_name_count 超过数组容量时，
 * config_validate() 是否拒绝配置。
 */

        return -1;
    }

    /*
     * config_load() 会在调用者当前配置的基础上
     * 加载配置文件，所以要先设置默认值。
     */
 /*
 * 测试 process_names 中包含空名称时，
 * config_load() 是否拒绝配置。
 *
 * sleep,,bash 中两个逗号之间没有内容，
 * 因此中间存在一个非法的空进程名称。
 */
   config_set_defaults(&config);

    if (config_load(test_filename, &config) != 0) {
        fprintf(
            stderr,
            "config_load failed for valid configuration\n"
        );

        remove(test_filename);

        return -1;
    }

    /*
     * 配置文件已经读取完毕，
     * 临时文件不再需要。
     */
    if (remove(test_filename) != 0) {
        perror("remove");
        return -1;
    }

    if (config.monitor_interval != 3) {
        fprintf(
            stderr,
            "unexpected monitor_interval: %u\n",
            config.monitor_interval
        );

        return -1;
    }

    if (config.process_name_count != 3) {
        fprintf(
            stderr,
            "unexpected process_name_count: %u\n",
            config.process_name_count
        );

        return -1;
    }

    if (
        strcmp(
            config.process_names[0],
            "sleep"
        ) != 0
    ) {
        fprintf(
            stderr,
            "unexpected process_names[0]: %s\n",
            config.process_names[0]
        );

        return -1;
    }

    if (
        strcmp(
            config.process_names[1],
            "bash"
        ) != 0
    ) {
        fprintf(
            stderr,
            "unexpected process_names[1]: %s\n",
            config.process_names[1]
        );

        return -1;
    }

    if (
        strcmp(
            config.process_names[2],
            "tail"
        ) != 0
    ) {
        fprintf(
            stderr,
            "unexpected process_names[2]: %s\n",
            config.process_names[2]
        );

        return -1;
    }

    /*
     * v1.6 当前为了兼容旧单进程代码，
     * 会把第一个多进程名称同时保存到 process_name。
     */
    if (
        strcmp(
            config.process_name,
            "sleep"
        ) != 0
    ) {
        fprintf(
            stderr,
            "unexpected compatibility process_name: %s\n",
            config.process_name
        );

        return -1;
    }

    printf("multiple process names test passed\n");

    return 0;
}

/*
 * 测试 process_names 的最大合法数量。
 *
 * CONFIG_MAX_PROCESS_NAMES 当前等于 8，
 * 因此配置 8 个进程名称应该加载成功。
 */
static int test_maximum_process_names(void)
{
    const char *test_filename =
        "/tmp/edgesentinel_test_max_names.conf";

    const char *expected_names[CONFIG_MAX_PROCESS_NAMES] = {
        "process1",
        "process2",
        "process3",
        "process4",
        "process5",
        "process6",
        "process7",
        "process8"
    };

    FILE *file;
    AppConfig config;
    unsigned int process_index;

    file = fopen(test_filename, "w");

    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    if (
        fprintf(
            file,
            "process_names="
            "process1,process2,process3,process4,"
            "process5,process6,process7,process8\n"
        ) < 0
    ) {
        fprintf(
            stderr,
            "failed to write maximum process names configuration\n"
        );

        fclose(file);
        remove(test_filename);

        return -1;
    }

    if (fclose(file) != 0) {
        perror("fclose");
        remove(test_filename);

        return -1;
    }

    config_set_defaults(&config);

    if (config_load(test_filename, &config) != 0) {
        fprintf(
            stderr,
            "config_load rejected maximum valid process names\n"
        );

        remove(test_filename);

        return -1;
    }

    if (remove(test_filename) != 0) {
        perror("remove");
        return -1;
    }

    if (
        config.process_name_count !=
        CONFIG_MAX_PROCESS_NAMES
    ) {
        fprintf(
            stderr,
            "unexpected maximum process_name_count: %u\n",
            config.process_name_count
        );

        return -1;
    }

    /*
     * 逐个检查 8 个名称是否按照原顺序
     * 保存到了 process_names 数组中。
     */
    for (
        process_index = 0;
        process_index < CONFIG_MAX_PROCESS_NAMES;
        process_index++
    ) {
        if (
            strcmp(
                config.process_names[process_index],
                expected_names[process_index]
            ) != 0
        ) {
            fprintf(
                stderr,
                "unexpected process_names[%u]: %s\n",
                process_index,
                config.process_names[process_index]
            );

            return -1;
        }
    }

    printf("maximum process names test passed\n");

    return 0;
}

/*
 * 测试超过 process_names 最大数量时，
 * config_load() 是否拒绝配置。
 *
 * CONFIG_MAX_PROCESS_NAMES 当前等于 8，
 * 因此配置 9 个名称必须失败。
 *
 * 同时验证：
 * 配置文件前面已经解析成功的内容，
 * 也不能部分覆盖调用者原来的配置。
 */
static int test_too_many_process_names(void)
{
    const char *test_filename =
        "/tmp/edgesentinel_test_too_many_names.conf";

    FILE *file;
    AppConfig config;

    /*
     * 先准备一份调用者当前正在使用的配置。
     * 后面加载非法配置失败时，
     * 这些值必须保持不变。
     */
    config_set_defaults(&config);

    config.monitor_interval = 7;
    config.process_pid = 1234;

    snprintf(
        config.process_name,
        sizeof(config.process_name),
        "%s",
        "original"
    );

    file = fopen(test_filename, "w");

    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    /*
     * 前两项本身合法，
     * 但 process_names 中包含 9 个名称。
     */
    if (
        fprintf(
            file,
            "monitor_interval=20\n"
            "process_pid=9999\n"
            "process_names="
            "process1,process2,process3,process4,"
            "process5,process6,process7,process8,"
            "process9\n"
        ) < 0
    ) {
        fprintf(
            stderr,
            "failed to write too many process names configuration\n"
        );

        fclose(file);
        remove(test_filename);

        return -1;
    }

    if (fclose(file) != 0) {
        perror("fclose");
        remove(test_filename);

        return -1;
    }

    /*
     * 9 个名称超过最大容量，
     * 因此 config_load() 必须返回失败。
     */
    if (config_load(test_filename, &config) == 0) {
        fprintf(
            stderr,
            "config_load accepted too many process names\n"
        );

        remove(test_filename);

        return -1;
    }

    if (remove(test_filename) != 0) {
        perror("remove");
        return -1;
    }

    /*
     * 虽然非法配置文件中的 monitor_interval
     * 和 process_pid 已经先被解析，
     * 但整个配置加载失败后不能应用任何修改。
     */
    if (config.monitor_interval != 7) {
        fprintf(
            stderr,
            "failed configuration changed monitor_interval: %u\n",
            config.monitor_interval
        );

        return -1;
    }

    if (config.process_pid != 1234) {
        fprintf(
            stderr,
            "failed configuration changed process_pid: %u\n",
            config.process_pid
        );

        return -1;
    }

    if (
        strcmp(
            config.process_name,
            "original"
        ) != 0
    ) {
        fprintf(
            stderr,
            "failed configuration changed process_name: %s\n",
            config.process_name
        );

        return -1;
    }

    if (config.process_name_count != 0) {
        fprintf(
            stderr,
            "failed configuration changed process_name_count: %u\n",
            config.process_name_count
        );

        return -1;
    }

    printf("too many process names rejection test passed\n");

    return 0;
}

/*
 * 测试 process_names 中包含空名称时，
 * config_load() 是否拒绝配置。
 *
 * sleep,,bash 中两个逗号之间没有内容，
 * 因此中间存在一个非法的空进程名称。
 */

/*
 * 测试 process_names 中包含空名称时，
 * config_load() 是否拒绝配置。
 *
 * sleep,,bash 中两个逗号之间没有内容，
 * 因此中间存在一个非法的空进程名称。
 */
static int test_empty_process_name_rejected(void)
{
    const char *test_filename =
        "/tmp/edgesentinel_test_empty_name.conf";

    FILE *file;
    AppConfig config;

    config_set_defaults(&config);

    config.monitor_interval = 9;
    config.process_pid = 4321;

    snprintf(
        config.process_name,
        sizeof(config.process_name),
        "%s",
        "original"
    );

    file = fopen(test_filename, "w");

    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    if (
        fprintf(
            file,
            "monitor_interval=20\n"
            "process_names=sleep,,bash\n"
        ) < 0
    ) {
        fprintf(
            stderr,
            "failed to write empty process name configuration\n"
        );

        fclose(file);
        remove(test_filename);

        return -1;
    }

    if (fclose(file) != 0) {
        perror("fclose");
        remove(test_filename);

        return -1;
    }

    if (config_load(test_filename, &config) == 0) {
        fprintf(
            stderr,
            "config_load accepted an empty process name\n"
        );

        remove(test_filename);

        return -1;
    }

    if (remove(test_filename) != 0) {
        perror("remove");
        return -1;
    }

    if (config.monitor_interval != 9) {
        fprintf(
            stderr,
            "failed configuration changed monitor_interval: %u\n",
            config.monitor_interval
        );

        return -1;
    }

    if (config.process_pid != 4321) {
        fprintf(
            stderr,
            "failed configuration changed process_pid: %u\n",
            config.process_pid
        );

        return -1;
    }

    if (
        strcmp(
            config.process_name,
            "original"
        ) != 0
    ) {
        fprintf(
            stderr,
            "failed configuration changed process_name: %s\n",
            config.process_name
        );

        return -1;
    }

    if (config.process_name_count != 0) {
        fprintf(
            stderr,
            "failed configuration changed process_name_count: %u\n",
            config.process_name_count
        );

        return -1;
    }

    printf("empty process name rejection test passed\n");

    return 0;
}

/*
 * 测试 process_names 以逗号开头或结尾时，
 * config_load() 是否拒绝配置。
 *
 * ,sleep 表示第一个名称为空；
 * sleep, 表示最后一个名称为空。
 */




static int test_edge_empty_process_names_rejected(void)
{
    const char *test_filename =
        "/tmp/edgesentinel_test_edge_empty_name.conf";

    const char *invalid_values[] = {
        ",sleep",
        "sleep,"
    };

    const unsigned int invalid_value_count =
        sizeof(invalid_values) /
        sizeof(invalid_values[0]);

    FILE *file;
    AppConfig config;
    unsigned int case_index;

    for (
        case_index = 0;
        case_index < invalid_value_count;
        case_index++
    ) {
        config_set_defaults(&config);

        /*
         * 保存一份原有配置，
         * 确认加载失败后不会被部分覆盖。
         */
        config.monitor_interval = 11;
        config.process_pid = 5678;

        snprintf(
            config.process_name,
            sizeof(config.process_name),
            "%s",
            "original"
        );

        file = fopen(test_filename, "w");

        if (file == NULL) {
            perror("fopen");
            return -1;
        }

        if (
            fprintf(
                file,
                "monitor_interval=30\n"
                "process_names=%s\n",
                invalid_values[case_index]
            ) < 0
        ) {
            fprintf(
                stderr,
                "failed to write edge empty name configuration\n"
            );

            fclose(file);
            remove(test_filename);

            return -1;
        }

        if (fclose(file) != 0) {
            perror("fclose");
            remove(test_filename);

            return -1;
        }

        /*
         * 以逗号开头或结尾都代表存在空名称，
         * config_load() 必须返回失败。
         */
        if (config_load(test_filename, &config) == 0) {
            fprintf(
                stderr,
                "config_load accepted invalid process_names: %s\n",
                invalid_values[case_index]
            );

            remove(test_filename);

            return -1;
        }

        if (remove(test_filename) != 0) {
            perror("remove");
            return -1;
        }

        /*
         * 非法配置不能覆盖原有配置。
         */
        if (config.monitor_interval != 11) {
            fprintf(
                stderr,
                "failed configuration changed monitor_interval: %u\n",
                config.monitor_interval
            );

            return -1;
        }

        if (config.process_pid != 5678) {
            fprintf(
                stderr,
                "failed configuration changed process_pid: %u\n",
                config.process_pid
            );

            return -1;
        }

        if (
            strcmp(
                config.process_name,
                "original"
            ) != 0
        ) {
            fprintf(
                stderr,
                "failed configuration changed process_name: %s\n",
                config.process_name
            );

            return -1;
        }

        if (config.process_name_count != 0) {
            fprintf(
                stderr,
                "failed configuration changed process_name_count: %u\n",
                config.process_name_count
            );

            return -1;
        }
    }

    printf("edge empty process names rejection test passed\n");

    return 0;
}


/*
 * 测试 process_names 中出现重复名称时，
 * config_load() 是否拒绝配置。
 *
 * sleep,bash,sleep 中 sleep 出现了两次。
 * 如果允许这种配置，程序会重复监控同一个目标进程。
 */
static int test_duplicate_process_names_rejected(void)
{
    const char *test_filename =
        "/tmp/edgesentinel_test_duplicate_names.conf";

    FILE *file;
    AppConfig config;
    int load_result;

    /*
     * 设置一份原有配置。
     * 非法配置加载失败后，
     * 原有配置必须保持不变。
     */
    config_set_defaults(&config);

    config.monitor_interval = 13;
    config.process_pid = 6789;

    snprintf(
        config.process_name,
        sizeof(config.process_name),
        "%s",
        "original"
    );

    file = fopen(test_filename, "w");

    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    if (
        fprintf(
            file,
            "monitor_interval=40\n"
            "process_names=sleep,bash,sleep\n"
        ) < 0
    ) {
        fprintf(
            stderr,
            "failed to write duplicate process names configuration\n"
        );

        fclose(file);
        remove(test_filename);

        return -1;
    }

    if (fclose(file) != 0) {
        perror("fclose");
        remove(test_filename);

        return -1;
    }

    load_result = config_load(test_filename, &config);

    if (remove(test_filename) != 0) {
        perror("remove");
        return -1;
    }

    /*
     * 重复名称必须被拒绝。
     */
    if (load_result == 0) {
        fprintf(
            stderr,
            "config_load accepted a duplicate process name\n"
        );

        return -1;
    }

    /*
     * 非法配置不能部分覆盖原配置。
     */
    if (config.monitor_interval != 13) {
        fprintf(
            stderr,
            "failed configuration changed monitor_interval: %u\n",
            config.monitor_interval
        );

        return -1;
    }

    if (config.process_pid != 6789) {
        fprintf(
            stderr,
            "failed configuration changed process_pid: %u\n",
            config.process_pid
        );

        return -1;
    }

    if (
        strcmp(
            config.process_name,
            "original"
        ) != 0
    ) {
        fprintf(
            stderr,
            "failed configuration changed process_name: %s\n",
            config.process_name
        );

        return -1;
    }

    if (config.process_name_count != 0) {
        fprintf(
            stderr,
            "failed configuration changed process_name_count: %u\n",
            config.process_name_count
        );

        return -1;
    }

    printf("duplicate process names rejection test passed\n");

    return 0;
}

/*
 * 测试 process_name_count 超过数组容量时，
 * config_validate() 是否拒绝配置。
 */
static int test_process_name_count_validation(void)
{
    AppConfig config;

    config_set_defaults(&config);

    config.process_name_count =
        CONFIG_MAX_PROCESS_NAMES + 1;

    if (config_validate(&config) == 0) {
        fprintf(
            stderr,
            "config_validate accepted excessive "
            "process_name_count\n"
        );

        return -1;
    }

    printf("process name count validation test passed\n");

    return 0;
}



/*
 * 测试 process_name_count 声明了有效元素数量，
 * 但 process_names 数组中的某个有效元素为空时，
 * config_validate() 是否拒绝配置。
 */
static int test_empty_process_name_slot_validation(void)
{
    AppConfig config;

    config_set_defaults(&config);

    config.process_name_count = 2;

    snprintf(
        config.process_names[0],
        sizeof(config.process_names[0]),
        "%s",
        "sleep"
    );

    /*
     * process_names[1] 保持默认的空字符串，
     * 但 process_name_count 又声明它属于有效范围。
     */
    if (config_validate(&config) == 0) {
        fprintf(
            stderr,
            "config_validate accepted an empty "
            "process_names element\n"
        );

        return -1;
    }

    printf("empty process name slot validation test passed\n");

    return 0;
}



/*
 * 测试进程名称数组中不存在字符串结束符 '\0' 时，
 * config_validate() 是否拒绝配置。
 *
 * C 字符串必须在数组范围内包含 '\0'，
 * 否则 strlen()、strcmp() 等函数可能越界访问内存。
 */
static int test_process_name_null_termination_validation(void)
{
    AppConfig config;

    config_set_defaults(&config);

    config.process_name_count = 1;

    /*
     * 使用字符 'A' 填满整个数组，
     * 故意不保留字符串结束符 '\0'。
     */
    memset(
        config.process_names[0],
        'A',
        sizeof(config.process_names[0])
    );

    if (config_validate(&config) == 0) {
        fprintf(
            stderr,
            "config_validate accepted a process name "
            "without null termination\n"
        );

        return -1;
    }

    printf(
        "process name null termination validation test passed\n"
    );

    return 0;
}



/*
 * 测试 monitor_interval 等于 0 时，
 * config_validate() 是否拒绝配置。
 */
static int test_zero_monitor_interval_validation(void)
{
    AppConfig config;

    config_set_defaults(&config);

    config.monitor_interval = 0;

    if (config_validate(&config) == 0) {
        fprintf(
            stderr,
            "config_validate accepted zero monitor_interval\n"
        );

        return -1;
    }

    printf("zero monitor interval validation test passed\n");

    return 0;
}



/*
 * 测试不同类型告警阈值的合法性检查。
 */
static int test_alert_threshold_validation(void)
{
    AppConfig config;

    /*
     * 系统 CPU 的 WARNING 必须小于 CRITICAL。
     */
    config_set_defaults(&config);
    config.cpu_warning_threshold = 90.0;
    config.cpu_critical_threshold = 80.0;

    if (config_validate(&config) == 0) {
        fprintf(
            stderr,
            "config_validate accepted invalid CPU thresholds\n"
        );

        return -1;
    }

    /*
     * 系统内存百分比不能小于 0。
     */
    config_set_defaults(&config);
    config.memory_warning_threshold = -1.0;

    if (config_validate(&config) == 0) {
        fprintf(
            stderr,
            "config_validate accepted negative memory threshold\n"
        );

        return -1;
    }

    /*
     * 磁盘使用率属于百分比，
     * 因此不能超过 100。
     */
    config_set_defaults(&config);
    config.disk_critical_threshold = 101.0;

    if (config_validate(&config) == 0) {
        fprintf(
            stderr,
            "config_validate accepted disk threshold above 100\n"
        );

        return -1;
    }

    /*
     * 进程 CPU 可以超过 100%，
     * 但 WARNING 仍然必须小于 CRITICAL。
     */
    config_set_defaults(&config);
    config.process_cpu_warning_threshold = 200.0;
    config.process_cpu_critical_threshold = 150.0;

    if (config_validate(&config) == 0) {
        fprintf(
            stderr,
            "config_validate accepted invalid process CPU thresholds\n"
        );

        return -1;
    }

    /*
     * 进程内存使用 MiB，不受 100 的限制，
     * 但 WARNING 仍然必须小于 CRITICAL。
     */
    config_set_defaults(&config);
    config.process_memory_warning_threshold_mib = 300.0;
    config.process_memory_critical_threshold_mib = 200.0;

    if (config_validate(&config) == 0) {
        fprintf(
            stderr,
            "config_validate accepted invalid process memory thresholds\n"
        );

        return -1;
    }

    /*
     * 验证进程 CPU 阈值确实允许超过 100%。
     *
     * 多线程进程可能同时占用多个 CPU 核心，
     * 所以 150% 和 200% 是合法配置。
     */
    config_set_defaults(&config);
    config.process_cpu_warning_threshold = 150.0;
    config.process_cpu_critical_threshold = 200.0;

    if (config_validate(&config) != 0) {
        fprintf(
            stderr,
            "config_validate rejected valid process CPU thresholds "
            "above 100\n"
        );

        return -1;
    }

    printf("alert threshold validation test passed\n");

    return 0;
}



/*
 * 测试配置文件中的非法数字格式是否会被拒绝。
 */
static int test_invalid_numeric_formats_rejected(void)
{
    const char *test_filename =
        "/tmp/edgesentinel_test_invalid_number.conf";

    const char *invalid_lines[] = {
        "monitor_interval=-1",
        "monitor_interval=3abc",
        "cpu_warning_threshold=nan",
        "log_max_size=-100"
    };

    const unsigned int invalid_line_count =
        sizeof(invalid_lines) /
        sizeof(invalid_lines[0]);

    FILE *file;
    AppConfig config;
    unsigned int case_index;
    int load_result;

    for (
        case_index = 0;
        case_index < invalid_line_count;
        case_index++
    ) {
        /*
         * 每个测试用例开始前都恢复一份确定的原配置。
         */
        config_set_defaults(&config);

        config.monitor_interval = 17;
        config.process_pid = 2468;
        config.cpu_warning_threshold = 60.0;
        config.log_max_size = 7777UL;

        file = fopen(test_filename, "w");

        if (file == NULL) {
            perror("fopen");
            return -1;
        }

        if (
            fprintf(
                file,
                "%s\n",
                invalid_lines[case_index]
            ) < 0
        ) {
            fprintf(
                stderr,
                "failed to write invalid numeric configuration\n"
            );

            fclose(file);
            remove(test_filename);

            return -1;
        }

        if (fclose(file) != 0) {
            perror("fclose");
            remove(test_filename);

            return -1;
        }

        load_result = config_load(test_filename, &config);

        if (remove(test_filename) != 0) {
            perror("remove");
            return -1;
        }

        if (load_result == 0) {
            fprintf(
                stderr,
                "config_load accepted invalid numeric value: %s\n",
                invalid_lines[case_index]
            );

            return -1;
        }

        /*
         * 加载失败后，原有配置不能被部分覆盖。
         */
        if (config.monitor_interval != 17) {
            fprintf(
                stderr,
                "invalid numeric configuration changed "
                "monitor_interval: %u\n",
                config.monitor_interval
            );

            return -1;
        }

        if (config.process_pid != 2468) {
            fprintf(
                stderr,
                "invalid numeric configuration changed "
                "process_pid: %u\n",
                config.process_pid
            );

            return -1;
        }

        if (config.cpu_warning_threshold != 60.0) {
            fprintf(
                stderr,
                "invalid numeric configuration changed "
                "CPU warning threshold: %.2f\n",
                config.cpu_warning_threshold
            );

            return -1;
        }

        if (config.log_max_size != 7777UL) {
            fprintf(
                stderr,
                "invalid numeric configuration changed "
                "log_max_size: %lu\n",
                config.log_max_size
            );

            return -1;
        }
    }

    printf("invalid numeric formats rejection test passed\n");

    return 0;
}



/*
 * 测试未知配置项和缺少等号的配置行
 * 是否会被 config_load() 拒绝。
 */
static int test_invalid_config_lines_rejected(void)
{
    const char *test_filename =
        "/tmp/edgesentinel_test_invalid_lines.conf";

    const char *invalid_lines[] = {
        "unknown_option=123",
        "monitor_interval 3"
    };

    const unsigned int invalid_line_count =
        sizeof(invalid_lines) /
        sizeof(invalid_lines[0]);

    FILE *file;
    AppConfig config;
    unsigned int case_index;
    int load_result;

    for (
        case_index = 0;
        case_index < invalid_line_count;
        case_index++
    ) {
        config_set_defaults(&config);

        /*
         * 设置一份原有配置，用来验证加载失败后
         * 配置结构体不会被部分修改。
         */
        config.monitor_interval = 19;
        config.process_pid = 1357;

        file = fopen(test_filename, "w");

        if (file == NULL) {
            perror("fopen");
            return -1;
        }

        if (
            fprintf(
                file,
                "monitor_interval=30\n"
                "%s\n",
                invalid_lines[case_index]
            ) < 0
        ) {
            fprintf(
                stderr,
                "failed to write invalid configuration line\n"
            );

            fclose(file);
            remove(test_filename);

            return -1;
        }

        if (fclose(file) != 0) {
            perror("fclose");
            remove(test_filename);

            return -1;
        }

        load_result = config_load(test_filename, &config);

        if (remove(test_filename) != 0) {
            perror("remove");
            return -1;
        }

        if (load_result == 0) {
            fprintf(
                stderr,
                "config_load accepted invalid line: %s\n",
                invalid_lines[case_index]
            );

            return -1;
        }

        /*
         * 第一行 monitor_interval=30 虽然已经成功解析，
         * 但后续行出现错误后，整个配置不能生效。
         */
        if (config.monitor_interval != 19) {
            fprintf(
                stderr,
                "invalid configuration changed "
                "monitor_interval: %u\n",
                config.monitor_interval
            );

            return -1;
        }

        if (config.process_pid != 1357) {
            fprintf(
                stderr,
                "invalid configuration changed process_pid: %u\n",
                config.process_pid
            );

            return -1;
        }
    }

    printf("invalid configuration lines rejection test passed\n");

    return 0;
}


int main(void)
{
    if (test_config_defaults() != 0) {
        fprintf(
            stderr,
            "configuration defaults test failed\n"
        );

        return 1;
    }

    if (test_multiple_process_names() != 0) {
        fprintf(
            stderr,
            "multiple process names test failed\n"
        );

        return 1;
    }

    if (test_maximum_process_names() != 0) {
        fprintf(
            stderr,
            "maximum process names test failed\n"
        );

        return 1;
    }

    if (test_too_many_process_names() != 0) {
        fprintf(
            stderr,
            "too many process names rejection test failed\n"
        );

        return 1;
    }

    if (test_empty_process_name_rejected() != 0) {
        fprintf(
            stderr,
            "empty process name rejection test failed\n"
        );

        return 1;
    }

    if (test_edge_empty_process_names_rejected() != 0) {
        fprintf(
            stderr,
            "edge empty process names rejection test failed\n"
        );

        return 1;
    }

    if (test_duplicate_process_names_rejected() != 0) {
        fprintf(
            stderr,
            "duplicate process names rejection test failed\n"
        );

        return 1;
    }

    if (test_process_name_count_validation() != 0) {
        fprintf(
            stderr,
            "process name count validation test failed\n"
        );

        return 1;
}

    if (test_empty_process_name_slot_validation() != 0) {
        fprintf(
            stderr,
            "empty process name slot validation test failed\n"
        );

        return 1;
    }

    if (
        test_process_name_null_termination_validation() != 0
    ) {
        fprintf(
            stderr,
            "process name null termination validation test failed\n"
        );

        return 1;
    }

    if (test_zero_monitor_interval_validation() != 0) {
        fprintf(
            stderr,
            "zero monitor interval validation test failed\n"
        );

        return 1;
    }

    if (test_alert_threshold_validation() != 0) {
        fprintf(
            stderr,
            "alert threshold validation test failed\n"
        );

        return 1;
    }

    if (test_invalid_numeric_formats_rejected() != 0) {
        fprintf(
            stderr,
            "invalid numeric formats rejection test failed\n"
        );

        return 1;
    }

    if (test_invalid_config_lines_rejected() != 0) {
        fprintf(
            stderr,
            "invalid configuration lines rejection test failed\n"
        );

        return 1;
    }

    printf("all configuration tests passed\n");

    return 0;
}
