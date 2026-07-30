#include "config.h"

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * 删除字符串开头和结尾的空白字符。
 *
 * 例如：
 *     "  monitor_interval  "
 *
 * 处理后：
 *     "monitor_interval"
 */
static char *trim_whitespace(char *text)
{
    char *end;

    if (text == NULL) {
        return NULL;
    }

    /*
     * 跳过字符串开头的空格、Tab 和换行符。
     */
    while (isspace((unsigned char)*text)) {
        text++;
    }

    /*
     * 如果字符串全部由空白字符组成，
     * 那么此时 text 指向字符串结束符 '\0'。
     */
    if (*text == '\0') {
        return text;
    }

    /*
     * end 指向字符串最后一个有效字符。
     */
    end = text + strlen(text) - 1;

    /*
     * 从字符串末尾向前删除空白字符。
     */
    while (end > text && isspace((unsigned char)*end)) {
        end--;
    }

    /*
     * 在最后一个非空白字符后放置字符串结束符。
     */
    end[1] = '\0';

    return text;
}

/*
 * 设置默认配置。
 *
 * 即使配置文件不存在，程序也可以使用这些默认值运行。
 */
void config_set_defaults(AppConfig *config)
{
    if (config == NULL) {
        return;
    }

    config->monitor_interval = 1;

    config->process_pid = 0;
    config->process_name[0] = '\0';

    memset(
        config->process_names,
        0,
        sizeof(config->process_names)
    );

    config->process_name_count = 0;

    config->cpu_warning_threshold = 70.0;
    config->cpu_critical_threshold = 90.0;

    config->process_cpu_warning_threshold = 70.0;
    config->process_cpu_critical_threshold = 90.0;

    config->process_memory_warning_threshold_mib = 100.0;
    config->process_memory_critical_threshold_mib = 200.0;

    config->memory_warning_threshold = 75.0;
    config->memory_critical_threshold = 90.0;

    config->disk_warning_threshold = 80.0;
    config->disk_critical_threshold = 90.0;

    snprintf(
        config->log_file,
        sizeof(config->log_file),
        "%s",
        "logs/edgesentinel.log"
    );

    config->log_max_size = 1048576UL;
}

/*
 * config_set_value() 的返回状态。
 */
enum {
    CONFIG_VALUE_INVALID = -1,
    CONFIG_VALUE_OK = 0,
    CONFIG_KEY_UNKNOWN = 1
};

/*
 * 把字符串严格转换成 unsigned int。
 *
 * 合法示例：
 *     "1"
 *     "30"
 *
 * 非法示例：
 *     "3abc"
 *     "-1"
 *     "1.5"
 *     ""
 */
static int parse_unsigned_int(
    const char *text,
    unsigned int *result
)
{
    char *end;
    unsigned long value;

    if (
        text == NULL ||
        result == NULL ||
        *text == '\0'
    ) {
        return -1;
    }

    /*
     * unsigned int 配置必须以数字开头。
     * 这样可以排除 -1、+1 和 abc。
     */
    if (!isdigit((unsigned char)text[0])) {
        return -1;
    }

    errno = 0;

    value = strtoul(text, &end, 10);

    /*
     * end == text：
     *     一个数字都没有读取到。
     *
     * *end != '\0'：
     *     数字后面还有非法字符，例如 3abc。
     *
     * errno == ERANGE：
     *     数值超过了 strtoul() 的表示范围。
     *
     * value > UINT_MAX：
     *     数值超过了 unsigned int 的表示范围。
     */
    if (
        end == text ||
        *end != '\0' ||
        errno == ERANGE ||
        value > UINT_MAX
    ) {
        return -1;
    }

    *result = (unsigned int)value;

    return 0;
}

/*
 * 把字符串严格转换成 unsigned long。
 *
 * 主要用于解析 log_max_size。
 */
static int parse_unsigned_long(
    const char *text,
    unsigned long *result
)
{
    char *end;
    unsigned long value;

    if (
        text == NULL ||
        result == NULL ||
        *text == '\0'
    ) {
        return -1;
    }

    if (!isdigit((unsigned char)text[0])) {
        return -1;
    }

    errno = 0;

    value = strtoul(text, &end, 10);

    if (
        end == text ||
        *end != '\0' ||
        errno == ERANGE
    ) {
        return -1;
    }

    *result = value;

    return 0;
}

/*
 * 把字符串严格转换成 double。
 *
 * 合法示例：
 *     "70"
 *     "70.0"
 *     "85.5"
 *
 * 非法示例：
 *     "70abc"
 *     "abc"
 *     "nan"
 *     "inf"
 */
static int parse_double(
    const char *text,
    double *result
)
{
    char *end;
    double value;

    if (
        text == NULL ||
        result == NULL ||
        *text == '\0'
    ) {
        return -1;
    }

    errno = 0;

    value = strtod(text, &end);

    if (
        end == text ||
        *end != '\0' ||
        errno == ERANGE ||
        !isfinite(value)
    ) {
        return -1;
    }

    *result = value;

    return 0;
}

/*
 * 根据配置项名称设置对应的配置成员。
 *
 * 返回值：
 *
 * CONFIG_VALUE_OK：
 *     配置项和值均合法。
 *
 * CONFIG_KEY_UNKNOWN：
 *     程序不认识该配置项。
 *
 * CONFIG_VALUE_INVALID：
 *     配置项存在，但值的格式不合法。
 */
static int config_set_value(
    AppConfig *config,
    const char *key,
    const char *value
)
{
    unsigned int unsigned_int_value;
    unsigned long unsigned_long_value;
    double double_value;
    size_t value_length;

    if (
        config == NULL ||
        key == NULL ||
        value == NULL
    ) {
        return CONFIG_VALUE_INVALID;
    }

    if (strcmp(key, "monitor_interval") == 0) {
        if (
            parse_unsigned_int(
                value,
                &unsigned_int_value
            ) != 0
        ) {
            return CONFIG_VALUE_INVALID;
        }

        config->monitor_interval = unsigned_int_value;
    } else if (strcmp(key, "process_pid") == 0) {
        /*
         * 把配置文件中的 PID 字符串转换成 unsigned int。
         *
         * 例如：
         *     "0"    → 0
         *     "1234" → 1234
         */
        if (
            parse_unsigned_int(
                value,
                &unsigned_int_value
            ) != 0
        ) {
            return CONFIG_VALUE_INVALID;
        }

        config->process_pid = unsigned_int_value;
    } else if (strcmp(key, "process_name") == 0) {
        value_length = strlen(value);

        /*
         * 进程名称必须能够完整放入字符数组，
         * 并为字符串结束符 '\0' 保留一个位置。
         */
        if (value_length >= sizeof(config->process_name)) {
            return CONFIG_VALUE_INVALID;
        }

        memcpy(
            config->process_name,
            value,
            value_length + 1
        );
    } else if (strcmp(key, "process_names") == 0) {
        char names_buffer[
            CONFIG_MAX_PROCESS_NAMES *
            CONFIG_PROCESS_NAME_LENGTH
        ];

        char parsed_names
            [CONFIG_MAX_PROCESS_NAMES]
            [CONFIG_PROCESS_NAME_LENGTH] = {{0}};

        char *name_token;
        unsigned int name_count = 0;
        size_t names_length;
        size_t name_length;

        /*
         * 先复制配置字符串，因为 strtok() 会修改字符串内容。
         */
        names_length = strlen(value);

        if (names_length >= sizeof(names_buffer)) {
            return CONFIG_VALUE_INVALID;
        }

        memcpy(
            names_buffer,
            value,
            names_length + 1
        );

        /*
         * 使用逗号分割多个进程名称。
         *
         * 示例：
         *     sleep,bash,sshd
         */
        name_token = strtok(names_buffer, ",");

        while (name_token != NULL) {
            if (name_count >= CONFIG_MAX_PROCESS_NAMES) {
                return CONFIG_VALUE_INVALID;
            }

            name_length = strlen(name_token);

            if (
                name_length == 0 ||
                name_length >= CONFIG_PROCESS_NAME_LENGTH
            ) {
                return CONFIG_VALUE_INVALID;
            }

            memcpy(
                parsed_names[name_count],
                name_token,
                name_length + 1
            );

            name_count++;

            name_token = strtok(NULL, ",");
        }

        /*
         * process_names 不能为空。
         */
        if (name_count == 0) {
            return CONFIG_VALUE_INVALID;
        }

        /*
         * 所有名称解析成功后，再写入正式配置结构体。
         */
        memcpy(
            config->process_names,
            parsed_names,
            sizeof(parsed_names)
        );

        config->process_name_count = name_count;

        /*
         * 暂时让旧 process_name 保存第一个名称，
         * 保持现有单进程代码继续正常工作。
         */
        memcpy(
            config->process_name,
            config->process_names[0],
            strlen(config->process_names[0]) + 1
        );

    } else if (
        strcmp(key, "cpu_warning_threshold") == 0
    ) {
        if (parse_double(value, &double_value) != 0) {
            return CONFIG_VALUE_INVALID;
        }

        config->cpu_warning_threshold = double_value;
    } else if (
        strcmp(key, "cpu_critical_threshold") == 0
    ) {
        if (parse_double(value, &double_value) != 0) {
            return CONFIG_VALUE_INVALID;
        }

        config->cpu_critical_threshold = double_value;
    } else if (
        strcmp(key, "process_cpu_warning_threshold") == 0
    ) {
        if (parse_double(value, &double_value) != 0) {
            return CONFIG_VALUE_INVALID;
        }

        config->process_cpu_warning_threshold = double_value;
    } else if (
        strcmp(key, "process_cpu_critical_threshold") == 0
    ) {
        if (parse_double(value, &double_value) != 0) {
            return CONFIG_VALUE_INVALID;
        }

        config->process_cpu_critical_threshold = double_value;
    } else if (
        strcmp(key, "process_memory_warning_threshold_mib") == 0
    ) {
        if (parse_double(value, &double_value) != 0) {
            return CONFIG_VALUE_INVALID;
        }

        config->process_memory_warning_threshold_mib = double_value;
    } else if (
        strcmp(key, "process_memory_critical_threshold_mib") == 0
    ) {
        if (parse_double(value, &double_value) != 0) {
            return CONFIG_VALUE_INVALID;
        }

        config->process_memory_critical_threshold_mib = double_value;
    }else if (
        strcmp(key, "memory_warning_threshold") == 0
    ) {
        if (parse_double(value, &double_value) != 0) {
            return CONFIG_VALUE_INVALID;
        }

        config->memory_warning_threshold = double_value;
    } else if (
        strcmp(key, "memory_critical_threshold") == 0
    ) {
        if (parse_double(value, &double_value) != 0) {
            return CONFIG_VALUE_INVALID;
        }

        config->memory_critical_threshold = double_value;
    } else if (
        strcmp(key, "disk_warning_threshold") == 0
    ) {
        if (parse_double(value, &double_value) != 0) {
            return CONFIG_VALUE_INVALID;
        }

        config->disk_warning_threshold = double_value;
    } else if (
        strcmp(key, "disk_critical_threshold") == 0
    ) {
        if (parse_double(value, &double_value) != 0) {
            return CONFIG_VALUE_INVALID;
        }

        config->disk_critical_threshold = double_value;
    } else if (strcmp(key, "log_file") == 0) {
        value_length = strlen(value);

        /*
         * 防止日志路径超过结构体数组容量。
         */
        if (value_length >= sizeof(config->log_file)) {
            return CONFIG_VALUE_INVALID;
        }

        memcpy(
            config->log_file,
            value,
            value_length + 1
        );
    } else if (strcmp(key, "log_max_size") == 0) {
        if (
            parse_unsigned_long(
                value,
                &unsigned_long_value
            ) != 0
        ) {
            return CONFIG_VALUE_INVALID;
        }

        config->log_max_size = unsigned_long_value;
    } else {
        return CONFIG_KEY_UNKNOWN;
    }

    return CONFIG_VALUE_OK;
}

/*
 * 从配置文件读取配置。
 *
 * 使用临时结构体 loaded_config 进行解析。
 * 只有整个配置文件都正确时，才把结果写回 config。
 *
 * 因此不会出现只加载一半配置的情况。
 */
int config_load(
    const char *filename,
    AppConfig *config
)
{
    FILE *file;
    char line[512];
    unsigned int line_number = 0;
    int has_error = 0;

    /*
     * 临时配置。
     *
     * 初始内容来自调用者已经设置好的默认配置。
     */
    AppConfig loaded_config;

    if (filename == NULL || config == NULL) {
        return -1;
    }

    loaded_config = *config;

    file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(
            stderr,
            "Warning: cannot open config file: %s\n",
            filename
        );

        return -1;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char *key;
        char *value;
        char *separator;
        int set_result;

        line_number++;

        key = trim_whitespace(line);

        /*
         * 跳过空行和注释行。
         */
        if (*key == '\0' || *key == '#') {
            continue;
        }

        separator = strchr(key, '=');

        if (separator == NULL) {
            fprintf(
                stderr,
                "Warning: invalid config line %u: %s\n",
                line_number,
                key
            );

            has_error = 1;
            continue;
        }

        /*
         * 把等号位置改成字符串结束符，
         * 从而把一行拆分成 key 和 value。
         */
        *separator = '\0';

        value = separator + 1;

        key = trim_whitespace(key);
        value = trim_whitespace(value);

        if (*key == '\0' || *value == '\0') {
            fprintf(
                stderr,
                "Warning: empty key or value at line %u\n",
                line_number
            );

            has_error = 1;
            continue;
        }

        set_result = config_set_value(
            &loaded_config,
            key,
            value
        );

        if (set_result == CONFIG_KEY_UNKNOWN) {
            fprintf(
                stderr,
                "Warning: unknown config key at line %u: %s\n",
                line_number,
                key
            );

            has_error = 1;
        } else if (
            set_result == CONFIG_VALUE_INVALID
        ) {
            fprintf(
                stderr,
                "Warning: invalid config value at line %u: "
                "%s=%s\n",
                line_number,
                key,
                value
            );

            has_error = 1;
        }
    }

    if (fclose(file) != 0) {
        perror("fclose");
        return -1;
    }

    /*
     * 任何一行存在错误，都不应用临时配置。
     *
     * config 仍然保留调用 config_load() 前的默认值。
     */
    if (has_error) {
        return -1;
    }

    /*
     * 整个配置文件解析成功后，
     * 才把临时配置复制给正式配置。
     */
    *config = loaded_config;

    return 0;
}

/*
 * 检查一组告警阈值是否合法。
 *
 * 要求：
 *
 * 1. WARNING 阈值必须位于 0～100；
 * 2. CRITICAL 阈值必须位于 0～100；
 * 3. WARNING 必须小于 CRITICAL。
 */
static int validate_threshold_pair(
    const char *resource_name,
    double warning_threshold,
    double critical_threshold
)
{
    if (
        warning_threshold < 0.0 ||
        warning_threshold > 100.0
    ) {
        fprintf(
            stderr,
            "Invalid %s warning threshold: %.2f\n",
            resource_name,
            warning_threshold
        );

        return -1;
    }

    if (
        critical_threshold < 0.0 ||
        critical_threshold > 100.0
    ) {
        fprintf(
            stderr,
            "Invalid %s critical threshold: %.2f\n",
            resource_name,
            critical_threshold
        );

        return -1;
    }

    if (warning_threshold >= critical_threshold) {
        fprintf(
            stderr,
            "Invalid %s thresholds: "
            "warning %.2f must be lower than critical %.2f\n",
            resource_name,
            warning_threshold,
            critical_threshold
        );

        return -1;
    }

    return 0;
}

/*
 * 检查进程 CPU 告警阈值是否合法。
 *
 * 与系统 CPU 使用率不同，
 * 单个多线程进程的 CPU 使用率可能超过 100%。
 *
 * 因此这里只要求：
 *
 * 1. WARNING 不能小于 0；
 * 2. CRITICAL 不能小于 0；
 * 3. WARNING 必须小于 CRITICAL。
 */
static int validate_process_cpu_threshold_pair(
    double warning_threshold,
    double critical_threshold
)
{
    if (warning_threshold < 0.0) {
        fprintf(
            stderr,
            "Invalid process CPU warning threshold: %.2f\n",
            warning_threshold
        );

        return -1;
    }

    if (critical_threshold < 0.0) {
        fprintf(
            stderr,
            "Invalid process CPU critical threshold: %.2f\n",
            critical_threshold
        );

        return -1;
    }

    if (warning_threshold >= critical_threshold) {
        fprintf(
            stderr,
            "Invalid process CPU thresholds: "
            "warning %.2f must be lower than critical %.2f\n",
            warning_threshold,
            critical_threshold
        );

        return -1;
    }

    return 0;
}

/*
 * 检查进程常驻内存告警阈值是否合法。
 *
 * 单位：MiB。
 *
 * 进程内存可能超过 100 MiB，
 * 因此不能使用只允许 0～100 的百分比校验函数。
 *
 * 要求：
 *
 * 1. WARNING 不能小于 0；
 * 2. CRITICAL 不能小于 0；
 * 3. WARNING 必须小于 CRITICAL。
 */
static int validate_process_memory_threshold_pair(
    double warning_threshold,
    double critical_threshold
)
{
    if (warning_threshold < 0.0) {
        fprintf(
            stderr,
            "Invalid process memory warning threshold: %.2f MiB\n",
            warning_threshold
        );

        return -1;
    }

    if (critical_threshold < 0.0) {
        fprintf(
            stderr,
            "Invalid process memory critical threshold: %.2f MiB\n",
            critical_threshold
        );

        return -1;
    }

    if (warning_threshold >= critical_threshold) {
        fprintf(
            stderr,
            "Invalid process memory thresholds: "
            "warning %.2f MiB must be lower than "
            "critical %.2f MiB\n",
            warning_threshold,
            critical_threshold
        );

        return -1;
    }

    return 0;
}


/*
 * 检查整个配置结构体是否合法。
 */
int config_validate(const AppConfig *config)
{
    unsigned int process_index;

    if (config == NULL) {
        fprintf(stderr, "Configuration pointer is NULL\n");
        return -1;
    }

    /*
     * sleep(0) 不会产生有效的采样间隔，
     * 因此采样间隔必须大于 0。
     */
    if (config->monitor_interval == 0) {
        fprintf(
            stderr,
            "Invalid monitor_interval: must be greater than 0\n"
        );

        return -1;
    }

    /*
     * 进程名称数量不能超过数组容量。
     */
    if (
        config->process_name_count >
        CONFIG_MAX_PROCESS_NAMES
    ) {
        fprintf(
            stderr,
            "Invalid process_name_count: %u "
            "exceeds maximum %d\n",
            config->process_name_count,
            CONFIG_MAX_PROCESS_NAMES
        );

        return -1;
    }

    /*
     * 检查 process_names 中每一个有效名称。
     */
    for (
        process_index = 0;
        process_index < config->process_name_count;
        process_index++
    ) {
        /*
         * 进程名称不能为空。
         */
        if (
            config->process_names[process_index][0] == '\0'
        ) {
            fprintf(
                stderr,
                "Invalid process_names[%u]: name is empty\n",
                process_index
            );

            return -1;
        }

        /*
         * 名称必须在字符数组范围内包含字符串结束符。
         */
        if (
            memchr(
                config->process_names[process_index],
                '\0',
                CONFIG_PROCESS_NAME_LENGTH
            ) == NULL
        ) {
            fprintf(
                stderr,
                "Invalid process_names[%u]: "
                "name is not null-terminated\n",
                process_index
            );

            return -1;
        }
    }

    if (
        validate_threshold_pair(
            "CPU",
            config->cpu_warning_threshold,
            config->cpu_critical_threshold
        ) != 0
    ) {
        return -1;
    }

    /*
     * 进程 CPU 使用率可能超过 100%，
     * 所以使用专门的校验函数。
     */
    if (
        validate_process_cpu_threshold_pair(
            config->process_cpu_warning_threshold,
            config->process_cpu_critical_threshold
        ) != 0
    ) {
        return -1;
    }

    if (
        validate_process_memory_threshold_pair(
            config->process_memory_warning_threshold_mib,
            config->process_memory_critical_threshold_mib
        ) != 0
    ) {
        return -1;
    }

    if (
        validate_threshold_pair(
            "memory",
            config->memory_warning_threshold,
            config->memory_critical_threshold
        ) != 0
    ) {
        return -1;
    }

    if (
        validate_threshold_pair(
            "disk",
            config->disk_warning_threshold,
            config->disk_critical_threshold
        ) != 0
    ) {
        return -1;
    }

    /*
     * 日志文件路径不能为空。
     */
    if (config->log_file[0] == '\0') {
        fprintf(stderr, "Invalid log_file: path is empty\n");
        return -1;
    }

    /*
     * 日志最大大小必须大于 0。
     */
    if (config->log_max_size == 0) {
        fprintf(
            stderr,
            "Invalid log_max_size: must be greater than 0\n"
        );

        return -1;
    }

    return 0;
}


/*
 * 打印当前生效的配置。
 */
void config_print(const AppConfig *config)
{
    unsigned int process_index;

    if (config == NULL) {
        return;
    }

    printf("========== EdgeSentinel Configuration ==========\n");
    printf("monitor_interval          : %u second(s)\n",
           config->monitor_interval);

    if (config->process_name[0] != '\0') {
        /*
         * 配置了 process_name 时，
         * process_name 的优先级高于 process_pid。
         */
        printf(
            "process_pid               : %u "
            "(ignored: process_name has priority)\n",
            config->process_pid
        );
    } else if (config->process_pid == 0) {
        /*
         * 没有配置 process_name，并且 process_pid 为 0，
         * 才表示监控 EdgeSentinel 自身。
         */
        printf(
            "process_pid               : %u (self)\n",
            config->process_pid
        );
    } else {
        /*
         * 没有配置 process_name，
         * 使用配置文件中的固定 PID。
         */
        printf(
            "process_pid               : %u\n",
            config->process_pid
        );
    }

    printf(
        "process_name              : %s\n",
        config->process_name[0] != '\0'
            ? config->process_name
            : "(not set)"
    );

    printf(
        "process_name_count        : %u\n",
        config->process_name_count
    );

    for (
        process_index = 0;
        process_index < config->process_name_count;
        process_index++
    )
    {
        printf(
            "process_names[%u]         : %s\n",
            process_index,
            config->process_names[process_index]
        );
    }

    printf("cpu_warning_threshold     : %.2f%%\n",
           config->cpu_warning_threshold);
    printf("cpu_critical_threshold    : %.2f%%\n",
           config->cpu_critical_threshold);

    printf(
        "process_cpu_warning_threshold  : %.2f%%\n",
        config->process_cpu_warning_threshold
    );

    printf(
        "process_cpu_critical_threshold : %.2f%%\n",
        config->process_cpu_critical_threshold
    );

    printf(
        "process_memory_warning_threshold_mib  : %.2f MiB\n",
        config->process_memory_warning_threshold_mib
    );

    printf(
        "process_memory_critical_threshold_mib : %.2f MiB\n",
        config->process_memory_critical_threshold_mib
    );

    printf("memory_warning_threshold  : %.2f%%\n",
           config->memory_warning_threshold);
    printf("memory_critical_threshold : %.2f%%\n",
           config->memory_critical_threshold);

    printf("disk_warning_threshold    : %.2f%%\n",
           config->disk_warning_threshold);
    printf("disk_critical_threshold   : %.2f%%\n",
           config->disk_critical_threshold);

    printf("log_file                  : %s\n",
           config->log_file);
    printf("log_max_size              : %lu bytes\n",
           config->log_max_size);

    printf("================================================\n");
}
