#include <stdio.h>
#include <string.h>

#include "output.h"

/*
 * 将用户输入的字符串转换为 OutputFormat。
 *
 * 例如：
 *
 *     "text" -> OUTPUT_FORMAT_TEXT
 *     "json" -> OUTPUT_FORMAT_JSON
 *
 * text：
 *     用户输入的格式名称。
 *
 * format：
 *     用来接收转换后的枚举值。
 *
 * 成功返回 0，失败返回 -1。
 */
int output_parse_format(
    const char *text,
    OutputFormat *format
)
{
    /*
     * text 和 format 都必须指向合法对象。
     */
    if (text == NULL || format == NULL)
    {
        return -1;
    }

    /*
     * strcmp 返回 0，表示两个字符串内容完全相同。
     */
    if (strcmp(text, "text") == 0)
    {
        *format = OUTPUT_FORMAT_TEXT;
        return 0;
    }

    if (strcmp(text, "json") == 0)
    {
        *format = OUTPUT_FORMAT_JSON;
        return 0;
    }

    /*
     * 输入的字符串既不是 text，也不是 json。
     */
    return -1;
}

/*
 * 将 OutputFormat 转换为可读字符串。
 */
const char *output_format_to_string(
    OutputFormat format
)
{
    switch (format)
    {
        case OUTPUT_FORMAT_TEXT:
            return "text";

        case OUTPUT_FORMAT_JSON:
            return "json";

        default:
            return "unknown";
    }
}

/*
 * 使用普通文本形式输出一轮监控快照。
 *
 * 当前先建立函数接口。
 * 后面把 main.c 中原有的 printf 输出逐步移动到这里。
 */
int output_print_text(
    const MonitorSnapshot *snapshot
)
{
    if (snapshot == NULL)
    {
        return -1;
    }

    printf(
        "Time:             "
        "%04d-%02d-%02d %02d:%02d:%02d\n",
        snapshot->current_time.year,
        snapshot->current_time.month,
        snapshot->current_time.day,
        snapshot->current_time.hour,
        snapshot->current_time.minute,
        snapshot->current_time.second
    );

    printf(
        "CPU Usage:        %6.2f%% [%s]\n",
        snapshot->cpu_usage_percent,
        alert_level_to_string(snapshot->cpu_level)
    );

    printf(
        "Memory Usage:     %6.2f%% [%s]\n",
        snapshot->memory_usage_percent,
        alert_level_to_string(snapshot->memory_level)
    );

    printf(
        "Disk Usage:       %6.2f%% [%s]\n",
        snapshot->disk_usage_percent,
        alert_level_to_string(snapshot->disk_level)
    );

    printf(
        "Network Download: %.2f B/s\n",
        snapshot->download_bytes_per_second
    );

    printf(
        "Network Upload:   %.2f B/s\n",
        snapshot->upload_bytes_per_second
    );

    return 0;
}

/*
 * 输出一个符合 JSON 语法的字符串。
 *
 * JSON 字符串中的双引号、反斜杠、换行等字符
 * 不能直接输出，必须先进行转义。
 */
static void output_print_json_string(
    const char *text
)
{
    const unsigned char *cursor;

    putchar('"');

    if (text != NULL)
    {
        cursor = (const unsigned char *)text;

        while (*cursor != '\0')
        {
            switch (*cursor)
            {
                case '"':
                    printf("\\\"");
                    break;

                case '\\':
                    printf("\\\\");
                    break;

                case '\b':
                    printf("\\b");
                    break;

                case '\f':
                    printf("\\f");
                    break;

                case '\n':
                    printf("\\n");
                    break;

                case '\r':
                    printf("\\r");
                    break;

                case '\t':
                    printf("\\t");
                    break;

                default:
                    /*
                     * JSON 不允许字符串中直接出现
                     * 0x00 到 0x1f 的控制字符。
                     */
                    if (*cursor < 0x20)
                    {
                        printf(
                            "\\u%04x",
                            (unsigned int)*cursor
                        );
                    }
                    else
                    {
                        putchar((int)*cursor);
                    }

                    break;
            }

            cursor++;
        }
    }

    putchar('"');
}

int output_print_json(
    const MonitorSnapshot *snapshot
)
{
    size_t process_index;

    if (snapshot == NULL)
    {
        return -1;
    }

    /*
     * 每一轮监控结果输出为一行完整 JSON。
     *
     * 这种格式称为 JSON Lines，
     * 便于 shell、Python、日志平台逐行处理。
     */
    printf("{");

    /*
     * 当前时间。
     */
    printf(
        "\"timestamp\":"
        "\"%04d-%02d-%02dT%02d:%02d:%02d\",",
        snapshot->current_time.year,
        snapshot->current_time.month,
        snapshot->current_time.day,
        snapshot->current_time.hour,
        snapshot->current_time.minute,
        snapshot->current_time.second
    );

    /*
     * 系统运行时间。
     */
    printf(
        "\"uptime\":{"
        "\"days\":%llu,"
        "\"hours\":%u,"
        "\"minutes\":%u,"
        "\"seconds\":%u"
        "},",
        snapshot->uptime.days,
        snapshot->uptime.hours,
        snapshot->uptime.minutes,
        snapshot->uptime.seconds
    );

    /*
     * 系统平均负载。
     */
    printf(
        "\"load_average\":{"
        "\"one_minute\":%.2f,"
        "\"five_minutes\":%.2f,"
        "\"fifteen_minutes\":%.2f"
        "},",
        snapshot->load_average.one_minute,
        snapshot->load_average.five_minutes,
        snapshot->load_average.fifteen_minutes
    );

    /*
     * 系统资源状态。
     */
    printf("\"system\":{");

    printf("\"status\":");
    output_print_json_string(
        alert_level_to_string(snapshot->system_level)
    );
    printf(",");

    printf(
        "\"cpu\":{"
        "\"usage_percent\":%.2f,"
        "\"status\":",
        snapshot->cpu_usage_percent
    );

    output_print_json_string(
        alert_level_to_string(snapshot->cpu_level)
    );

    printf("},");

    printf(
        "\"memory\":{"
        "\"total_kb\":%llu,"
        "\"available_kb\":%llu,"
        "\"used_kb\":%llu,"
        "\"usage_percent\":%.2f,"
        "\"status\":",
        snapshot->total_memory_kb,
        snapshot->available_memory_kb,
        snapshot->used_memory_kb,
        snapshot->memory_usage_percent
    );

    output_print_json_string(
        alert_level_to_string(snapshot->memory_level)
    );

    printf("},");

    printf(
        "\"disk\":{"
        "\"mount_point\":\"/\","
        "\"total_bytes\":%llu,"
        "\"used_bytes\":%llu,"
        "\"available_bytes\":%llu,"
        "\"usage_percent\":%.2f,"
        "\"status\":",
        snapshot->total_disk_bytes,
        snapshot->used_disk_bytes,
        snapshot->available_disk_bytes,
        snapshot->disk_usage_percent
    );

    output_print_json_string(
        alert_level_to_string(snapshot->disk_level)
    );

    printf("}");

    printf("},");

    /*
     * 网络累计流量和实时速度。
     */
    printf(
        "\"network\":{"
        "\"receive_total_bytes\":%llu,"
        "\"transmit_total_bytes\":%llu,"
        "\"download_bytes_per_second\":%.2f,"
        "\"upload_bytes_per_second\":%.2f"
        "},",
        snapshot->network_receive_total_bytes,
        snapshot->network_transmit_total_bytes,
        snapshot->download_bytes_per_second,
        snapshot->upload_bytes_per_second
    );

    /*
     * 被监控进程数组。
     */
    printf("\"processes\":[");

    for (
        process_index = 0;
        process_index < snapshot->process_count;
        process_index++
    )
    {
        const MonitoredProcess *process;
        const char *cpu_sample_state;

        process = &snapshot->processes[process_index];

        if (process_index > 0)
        {
            printf(",");
        }

        if (process->cpu_usage_valid)
        {
            cpu_sample_state = "valid";
        }
        else if (process->cpu_sample_initialized)
        {
            cpu_sample_state = "collecting";
        }
        else
        {
            cpu_sample_state = "unavailable";
        }

        printf("{");

        printf(
            "\"index\":%zu,",
            process_index
        );

        printf("\"target_name\":");
        output_print_json_string(process->target_name);
        printf(",");

        printf(
            "\"available\":%s,",
            process->available ? "true" : "false"
        );

        printf(
            "\"pid\":%d,",
            process->current_pid
        );

        /*
         * 进程不可用时，这些字段没有有效值，
         * 因此输出 JSON 的 null。
         */
        printf("\"name\":");

        if (process->available)
        {
            output_print_json_string(process->info.name);
        }
        else
        {
            printf("null");
        }

        printf(",");

        printf("\"parent_pid\":");

        if (process->available)
        {
            printf("%d", process->info.parent_pid);
        }
        else
        {
            printf("null");
        }

        printf(",");

        printf("\"state\":");

        if (process->available)
        {
            output_print_json_string(process->info.state);
        }
        else
        {
            printf("null");
        }

        printf(",");

        printf("\"memory_mib\":");

        if (process->available)
        {
            printf("%.2f", process->memory_mib);
        }
        else
        {
            printf("null");
        }

        printf(",");

        printf("\"memory_status\":");

        if (process->available)
        {
            output_print_json_string(
                alert_level_to_string(
                    process->memory_level
                )
            );
        }
        else
        {
            printf("null");
        }

        printf(",");

        printf("\"cpu_sample_state\":");
        output_print_json_string(cpu_sample_state);
        printf(",");

        printf("\"cpu_usage_percent\":");

        if (process->cpu_usage_valid)
        {
            printf("%.2f", process->cpu_usage);
        }
        else
        {
            printf("null");
        }

        printf(",");

        printf("\"cpu_status\":");

        if (process->cpu_usage_valid)
        {
            output_print_json_string(
                alert_level_to_string(
                    process->cpu_level
                )
            );
        }
        else
        {
            printf("null");
        }

        printf("}");
    }

    printf("]");

    printf("}\n");

    return 0;
}

/*
 * 根据 format 选择实际输出函数。
 */
int output_print(
    const MonitorSnapshot *snapshot,
    OutputFormat format
)
{
    if (snapshot == NULL)
    {
        return -1;
    }

    switch (format)
    {
        case OUTPUT_FORMAT_TEXT:
            return output_print_text(snapshot);

        case OUTPUT_FORMAT_JSON:
            return output_print_json(snapshot);

        default:
            return -1;
    }
}
