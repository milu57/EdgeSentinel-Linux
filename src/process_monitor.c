#include "process_monitor.h"

#include <ctype.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * 初始化一个被监控进程对象。
 */
int monitored_process_init(
    MonitoredProcess *process,
    const char *target_name,
    int configured_pid,
    int current_pid
)
{
    int name_length;

    /*
     * process 必须指向一个有效的
     * MonitoredProcess 对象。
     *
     * PID 不允许是负数。
     */
    if (
        process == NULL ||
        configured_pid < 0 ||
        current_pid < 0
    ) {
        return -1;
    }

    /*
     * 将整个结构体占用的内存清零。
     *
     * 清零后：
     *
     * PID 为 0；
     * double 为 0.0；
     * 各种 initialized 标志为 0；
     * 字符数组为空字符串。
     */
    memset(process, 0, sizeof(*process));

    /*
     * 如果提供了进程名称，就把名称复制到结构体中。
     */
    if (
        target_name != NULL &&
        target_name[0] != '\0'
    ) {
        name_length = snprintf(
            process->target_name,
            sizeof(process->target_name),
            "%s",
            target_name
        );

        /*
         * 检查名称复制是否失败或者被截断。
         */
        if (
            name_length < 0 ||
            (size_t)name_length >=
                sizeof(process->target_name)
        ) {
            memset(process, 0, sizeof(*process));
            return -1;
        }
    }

    /*
     * 保存配置中的 PID和当前实际 PID。
     */
    process->configured_pid = configured_pid;
    process->current_pid = current_pid;

    /*
     * 明确设置告警等级初始值。
     */
    process->cpu_level = ALERT_NORMAL;
    process->previous_cpu_level = ALERT_NORMAL;

    process->memory_level = ALERT_NORMAL;
    process->previous_memory_level = ALERT_NORMAL;

    return 0;
}

/*
 * 从 /proc/<pid>/status 中读取指定进程的信息。
 *
 * 成功返回 0。
 * 失败返回 -1。
 */
int read_process_info(int pid, ProcessInfo *info)
{
    char status_path[64];
    char line[512];
    FILE *file;

    ProcessInfo result = {0};

    int found_name = 0;
    int found_state = 0;
    int found_parent_pid = 0;
    int found_memory = 0;

    int path_length;

    /*
     * pid 必须是正数。
     * info 必须指向一个有效的 ProcessInfo 对象。
     */
    if (pid <= 0 || info == NULL) {
        return -1;
    }

    /*
     * 根据 PID 构造进程状态文件路径。
     *
     * 例如：
     * pid = 1234
     *
     * 最终得到：
     * /proc/1234/status
     */
    path_length = snprintf(
        status_path,
        sizeof(status_path),
        "/proc/%d/status",
        pid
    );

    /*
     * snprintf 返回值小于 0，表示格式化失败。
     *
     * 返回值大于或等于数组容量，
     * 表示生成的路径超出了 status_path 的空间。
     */
    if (path_length < 0 ||
        (size_t)path_length >= sizeof(status_path)) {
        return -1;
    }

    /*
     * 打开指定进程的 status 文件。
     *
     * 如果 PID 不存在，或者进程已经退出，
     * fopen 就会失败。
     */
    file = fopen(status_path, "r");

    if (file == NULL) {
        return -1;
    }

    /*
     * 先把结果写入局部结构体 result。
     *
     * 等所有字段都读取成功后，
     * 再将 result 整体复制给调用者的 info。
     */
    result.pid = pid;

    /*
     * 逐行读取 /proc/<pid>/status。
     */
    while (fgets(line, sizeof(line), file) != NULL) {

        /*
         * 读取进程名称。
         *
         * status 文件中的内容类似：
         *
         * Name:   edgesentinel
         */
        if (strncmp(line, "Name:", 5) == 0) {
            char *name_start = line + 5;

            /*
             * 跳过 Name: 后面的空格和制表符。
             */
            while (*name_start == ' ' || *name_start == '\t') {
                name_start++;
            }

            /*
             * 删除 fgets 读取到的换行符。
             */
            name_start[strcspn(name_start, "\r\n")] = '\0';

            if (*name_start != '\0') {
                snprintf(
                    result.name,
                    sizeof(result.name),
                    "%s",
                    name_start
                );

                found_name = 1;
            }
        }

        /*
         * 读取进程状态。
         *
         * status 文件中的内容类似：
         *
         * State:  S (sleeping)
         *
         * 当前只保存状态字符 S。
         */

        else if (strncmp(line, "State:", 6) == 0) {
	    char *state_start = line + 6;
	
	    /*
	     * 跳过 State: 后面的空格和制表符。
	     */
	    while (*state_start == ' ' || *state_start == '\t') {
	        state_start++;
	    }
	
	    /*
	     * 删除 fgets 读取到的换行符。
	     *
	     * 例如原始内容是：
	     *
	     * S (sleeping)\n
	     *
	     * 删除后变为：
	     *
	     * S (sleeping)
	     */
	    state_start[strcspn(state_start, "\r\n")] = '\0';
	
	    /*
	     * 确保状态字符串不为空。
	     */
	    if (*state_start != '\0') {
	        snprintf(
	            result.state,
	            sizeof(result.state),
	            "%s",
	            state_start
	        );
	
	        found_state = 1;
	    }
	}

        /*
         * 读取父进程 PID。
         *
         * status 文件中的内容类似：
         *
         * PPid:   123
         */
        else if (strncmp(line, "PPid:", 5) == 0) {
            if (sscanf(
                    line,
                    "PPid: %d",
                    &result.parent_pid
                ) == 1) {
                found_parent_pid = 1;
            }
        }

        /*
         * 读取进程实际驻留在物理内存中的大小。
         *
         * status 文件中的内容类似：
         *
         * VmRSS:  1580 kB
         */
        else if (strncmp(line, "VmRSS:", 6) == 0) {
            if (sscanf(
                    line,
                    "VmRSS: %lu kB",
                    &result.resident_memory_kb
                ) == 1) {
                found_memory = 1;
            }
        }

        /*
         * 所有字段都读取完成后，
         * 不再继续读取剩余内容。
         */
        if (found_name &&
            found_state &&
            found_parent_pid &&
            found_memory) {
            break;
        }
    }

    fclose(file);

    /*
     * 只要有一个必要字段没有找到，
     * 本次读取就视为失败。
     */
    if (!found_name ||
        !found_state ||
        !found_parent_pid ||
        !found_memory) {
        return -1;
    }

    /*
     * 所有信息读取成功后，
     * 再整体写入调用者提供的结构体。
     */
    *info = result;

    return 0;
}

static int is_pid_directory_name(const char *name)
{
    const char *current;

    if (name == NULL || name[0] == '\0') {
        return 0;
    }

    current = name;

    while (*current != '\0') {
        if (!isdigit((unsigned char)*current)) {
            return 0;
        }

        current++;
    }

    return 1;
}

int find_process_by_name(const char *process_name, int *pid)
{
    DIR *proc_directory;
    struct dirent *entry;

    if (process_name == NULL || pid == NULL) {
        return -1;
    }

    proc_directory = opendir("/proc");

    if (proc_directory == NULL) {
        return -1;
    }

    while ((entry = readdir(proc_directory)) != NULL) {
        long pid_value;
        char *end_pointer;
        ProcessInfo process_info;

        if (!is_pid_directory_name(entry->d_name)) {
            continue;
        }

        pid_value = strtol(entry->d_name, &end_pointer, 10);

        if (*end_pointer != '\0' ||
            pid_value <= 0 ||
            pid_value > INT_MAX) {
            continue;
        }

        if (read_process_info((int)pid_value, &process_info) != 0) {
            continue;
        }

        if (strcmp(process_info.name, process_name) == 0) {
            *pid = (int)pid_value;
            closedir(proc_directory);
            return 0;
        }
    }

    closedir(proc_directory);
    return -1;
}

/*
 * 从 /proc/<pid>/stat 中读取进程累计 CPU 时间。
 *
 * /proc/<pid>/stat 中：
 *
 * 第 14 个字段是 utime，表示用户态累计 CPU 时间；
 * 第 15 个字段是 stime，表示内核态累计 CPU 时间。
 *
 * 时间单位是 clock ticks，而不是秒。
 */
int read_process_cpu_times(
    int pid,
    ProcessCpuTimes *times
)
{
    char stat_path[64];
    char line[4096];

    FILE *file;

    int path_length;
    int field_number;

    char *closing_parenthesis;
    char *cursor;
    char *end;

    ProcessCpuTimes result = {0};

    /*
     * PID 必须是正数。
     * times 必须指向有效的 ProcessCpuTimes 对象。
     */
    if (pid <= 0 || times == NULL) {
        return -1;
    }

    /*
     * 构造：
     *
     * /proc/<pid>/stat
     */
    path_length = snprintf(
        stat_path,
        sizeof(stat_path),
        "/proc/%d/stat",
        pid
    );

    if (
        path_length < 0 ||
        (size_t)path_length >= sizeof(stat_path)
    ) {
        return -1;
    }

    file = fopen(stat_path, "r");

    if (file == NULL) {
        return -1;
    }

    /*
     * /proc/<pid>/stat 的内容通常只有一行，
     * 因此一次读取整行。
     */
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        return -1;
    }

    /*
     * stat 文件前几个字段类似：
     *
     * 1234 (process name) S 1000 ...
     *
     * 第 1 个字段：PID
     * 第 2 个字段：进程名称
     * 第 3 个字段：状态
     *
     * 进程名称可能包含空格，因此不能简单地按照空格
     * 从整行开头逐个拆分。
     *
     * 这里先找到进程名称外面的最后一个右括号 ')'。
     */
    closing_parenthesis = strrchr(line, ')');

    if (closing_parenthesis == NULL) {
        return -1;
    }

    /*
     * cursor 从右括号后面开始。
     *
     * 此时后面的内容类似：
     *
     * S 1000 1000 1000 ...
     */
    cursor = closing_parenthesis + 1;

    /*
     * 跳过右括号后面的空白字符。
     */
    while (isspace((unsigned char)*cursor)) {
        cursor++;
    }

    /*
     * 当前位置应该是第 3 个字段：
     * 进程状态字符。
     */
    if (*cursor == '\0') {
        return -1;
    }

    /*
     * 跳过第 3 个字段的状态字符。
     */
    cursor++;

    /*
     * 跳过第 4～13 个字段。
     *
     * 跳过的字段包括：
     *
     * 4  ppid
     * 5  pgrp
     * 6  session
     * 7  tty_nr
     * 8  tpgid
     * 9  flags
     * 10 minflt
     * 11 cminflt
     * 12 majflt
     * 13 cmajflt
     *
     * 跳过以后，cursor 就会到达第 14 个字段 utime。
     */
    for (field_number = 4;
         field_number <= 13;
         field_number++)
    {
        /*
         * 先移动到当前字段的第一个字符。
         */
        while (isspace((unsigned char)*cursor)) {
            cursor++;
        }

        if (*cursor == '\0') {
            return -1;
        }

        /*
         * 跳过当前字段的所有非空白字符。
         */
        while (
            *cursor != '\0' &&
            !isspace((unsigned char)*cursor)
        ) {
            cursor++;
        }
    }

    /*
     * 移动到第 14 个字段 utime。
     */
    while (isspace((unsigned char)*cursor)) {
        cursor++;
    }

    if (*cursor == '\0') {
        return -1;
    }

    errno = 0;

    result.user_ticks = strtoull(
        cursor,
        &end,
        10
    );

    /*
     * end == cursor：
     *     没有读取到数字。
     *
     * errno == ERANGE：
     *     数值超过 unsigned long long 的范围。
     */
    if (end == cursor || errno == ERANGE) {
        return -1;
    }

    /*
     * 第 14 个字段读取结束后，
     * end 指向它后面的字符。
     */
    cursor = end;

    /*
     * 移动到第 15 个字段 stime。
     */
    while (isspace((unsigned char)*cursor)) {
        cursor++;
    }

    if (*cursor == '\0') {
        return -1;
    }

    errno = 0;

    result.system_ticks = strtoull(
        cursor,
        &end,
        10
    );

    if (end == cursor || errno == ERANGE) {
        return -1;
    }

    /*
     * 两个字段都读取成功后，
     * 再整体写入调用者提供的对象。
     */
    *times = result;

    return 0;
}

/*
 * 根据前后两次进程累计 CPU 时间，
 * 计算采样区间内的进程 CPU 使用率。
 *
 * 返回值以百分比表示：
 *
 * 0.0   表示采样期间基本没有使用 CPU；
 * 100.0 表示采样期间大约占满一个 CPU 核心。
 *
 * 多线程进程同时占用多个核心时，
 * 结果可能大于 100%。
 */
double calculate_process_cpu_usage(
    const ProcessCpuTimes *previous,
    const ProcessCpuTimes *current,
    double elapsed_seconds
)
{
    unsigned long long previous_total_ticks;
    unsigned long long current_total_ticks;
    unsigned long long delta_ticks;

    long ticks_per_second;

    double process_cpu_seconds;
    double cpu_usage;

    /*
     * 两个结构体指针必须有效。
     *
     * 实际经过时间必须大于 0，
     * 否则不能进行除法。
     */
    if (
        previous == NULL ||
        current == NULL ||
        elapsed_seconds <= 0.0
    ) {
        return -1.0;
    }

    /*
     * 计算上一次采样时，
     * 进程累计使用的 CPU tick 总数。
     */
    previous_total_ticks =
        previous->user_ticks +
        previous->system_ticks;

    /*
     * 计算当前采样时，
     * 进程累计使用的 CPU tick 总数。
     */
    current_total_ticks =
        current->user_ticks +
        current->system_ticks;

    /*
     * 累计值正常情况下只会增加。
     *
     * 当前值小于上一次值，
     * 可能表示 PID 对应的进程已经变化，
     * 或者采样数据无效。
     */
    if (current_total_ticks < previous_total_ticks) {
        return -1.0;
    }

    delta_ticks =
        current_total_ticks -
        previous_total_ticks;

    /*
     * 获取当前 Linux 系统每秒包含多少个 clock tick。
     *
     * 常见结果可能是：
     *
     * 100
     *
     * 表示 100 tick 等于 1 秒 CPU 时间。
     */
    ticks_per_second = sysconf(_SC_CLK_TCK);

    if (ticks_per_second <= 0) {
        return -1.0;
    }

    /*
     * 把 tick 差值换算成进程实际使用的 CPU 秒数。
     */
    process_cpu_seconds =
        (double)delta_ticks /
        (double)ticks_per_second;

    /*
     * 再除以两次采样之间经过的真实时间，
     * 得到进程 CPU 使用率。
     */
    cpu_usage =
        process_cpu_seconds /
        elapsed_seconds *
        100.0;

    return cpu_usage;
}


