#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "alert.h"
#include "alert_event.h"
#include "notifier.h"
/*
 * 将告警事件格式化为统一通知文本。
 */
int notifier_format_message(
    const AlertEvent *event,
    char *buffer,
    size_t buffer_size
)
{
    struct tm local_time;
    char time_buffer[32];
    size_t formatted_time_length;
    int message_length;

    /*
     * event 是需要读取的告警事件；
     * buffer 是用于保存最终通知文本的数组。
     *
     * 任意一个为空指针，都不能继续执行。
     */
    if (
        event == NULL ||
        buffer == NULL ||
        buffer_size == 0
    ) {
        return -1;
    }

    /*
     * localtime_r() 将 time_t 时间转换成当前系统时区下的
     * 年、月、日、时、分、秒。
     *
     * 与 localtime() 不同，localtime_r() 将结果写入调用者
     * 提供的 local_time 对象，不使用共享的静态存储区。
     */
    if (
        localtime_r(
            &event->occurred_at,
            &local_time
        ) == NULL
    ) {
        return -1;
    }

    /*
     * 将时间格式化为：
     *
     *     2026-08-03 17:20:30
     */
    formatted_time_length = strftime(
        time_buffer,
        sizeof(time_buffer),
        "%Y-%m-%d %H:%M:%S",
        &local_time
    );

    if (formatted_time_length == 0) {
        return -1;
    }

    /*
     * snprintf() 最多向 buffer 中写入 buffer_size 个字节，
     * 因此不会越过数组边界。
     *
     * 返回值是不考虑 buffer 容量时，本来需要写入的字符数，
     * 不包含末尾的 '\0'。
     */
    message_length = snprintf(
        buffer,
        buffer_size,
        "[EdgeSentinel] "
        "%s target=%s status=%s->%s "
        "value=%.2f%s time=%s",
        alert_metric_to_string(event->metric),
        event->target,
        alert_level_to_string(event->previous_level),
        alert_level_to_string(event->current_level),
        event->current_value,
        event->unit,
        time_buffer
    );

    /*
     * snprintf() 返回负数，表示格式化发生错误。
     */
    if (message_length < 0) {
        return -1;
    }

    /*
     * message_length 不包含 '\0'。
     *
     * 如果它大于或等于 buffer_size，
     * 表示输出结果没有完整放入 buffer。
     */
    if ((size_t)message_length >= buffer_size) {
        return -1;
    }

    return 0;
}

/*
 * 把指定长度的数据完整写入文件描述符。
 *
 * write() 可能只写入部分数据，
 * 因此需要循环，直到所有数据都写完。
 */
static int write_all(
    int file_descriptor,
    const char *data,
    size_t data_length
)
{
    size_t total_written = 0;

    while (total_written < data_length) {
        ssize_t written;

        written = write(
            file_descriptor,
            data + total_written,
            data_length - total_written
        );

        if (written < 0) {
            /*
             * write() 被信号中断时可以重新尝试。
             */
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        /*
         * 没有写入任何数据时，
         * 避免循环永远无法结束。
         */
        if (written == 0) {
            return -1;
        }

        total_written += (size_t)written;
    }

    return 0;
}

/*
 * 等待指定子进程结束。
 *
 * waitpid() 被信号中断时重新等待。
 */
static int wait_for_child(
    pid_t child_pid,
    int *child_status
)
{
    pid_t wait_result;

    do {
        wait_result = waitpid(
            child_pid,
            child_status,
            0
        );
    } while (
        wait_result < 0 &&
        errno == EINTR
    );

    if (wait_result < 0) {
        return -1;
    }

    return 0;
}

/*
 * 执行外部通知程序并发送告警事件。
 */
int notifier_send(
    const AlertEvent *event,
    const char *command
)
{
    char message[NOTIFIER_MESSAGE_LENGTH];
    int pipe_descriptors[2];
    pid_t child_pid;
    int child_status;
    int send_result = 0;
    struct sigaction ignore_sigpipe;
    struct sigaction previous_sigpipe;

    if (
        event == NULL ||
        command == NULL ||
        command[0] == '\0'
    ) {
        return -1;
    }

    /*
     * 首先把告警事件转换成通知文本。
     */
    if (
        notifier_format_message(
            event,
            message,
            sizeof(message)
        ) != 0
    ) {
        return -1;
    }

    /*
     * pipe_descriptors[0]：读取端；
     * pipe_descriptors[1]：写入端。
     */
    if (pipe(pipe_descriptors) != 0) {
        return -1;
    }

    child_pid = fork();

    if (child_pid < 0) {
        close(pipe_descriptors[0]);
        close(pipe_descriptors[1]);

        return -1;
    }

    if (child_pid == 0) {
        /*
         * 子进程不使用管道写入端。
         */
        close(pipe_descriptors[1]);

        /*
         * 将管道读取端复制为标准输入。
         *
         * 外部通知程序之后可以通过 stdin
         * 读取 EdgeSentinel 发送的通知消息。
         */
        if (
            dup2(
                pipe_descriptors[0],
                STDIN_FILENO
            ) < 0
        ) {
            _exit(126);
        }

        close(pipe_descriptors[0]);

        /*
         * 直接执行配置的程序路径。
         *
         * 这里没有使用 Shell，也没有使用 system()，
         * 因此通知消息不会被解释成 Shell 命令。
         */
        execl(
            command,
            command,
            (char *)NULL
        );

        /*
         * exec 成功后不会返回。
         *
         * 返回到这里说明程序无法执行。
         */
        _exit(127);
    }

    /*
     * 父进程不使用管道读取端。
     */
    close(pipe_descriptors[0]);

    /*
     * 如果子进程提前退出，
     * 向管道写数据可能产生 SIGPIPE。
     *
     * 临时忽略 SIGPIPE，让 write() 返回错误，
     * 而不是直接终止 EdgeSentinel。
     */
    memset(
        &ignore_sigpipe,
        0,
        sizeof(ignore_sigpipe)
    );

    ignore_sigpipe.sa_handler = SIG_IGN;
    sigemptyset(&ignore_sigpipe.sa_mask);

    if (
        sigaction(
            SIGPIPE,
            &ignore_sigpipe,
            &previous_sigpipe
        ) != 0
    ) {
        close(pipe_descriptors[1]);
        wait_for_child(child_pid, &child_status);

        return -1;
    }

    /*
     * 将通知文本写入子进程的标准输入。
     */
    if (
        write_all(
            pipe_descriptors[1],
            message,
            strlen(message)
        ) != 0
    ) {
        send_result = -1;
    }

    /*
     * 在通知文本后添加换行，
     * 方便 Shell、Python 等脚本逐行读取。
     */
    if (
        send_result == 0 &&
        write_all(
            pipe_descriptors[1],
            "\n",
            1
        ) != 0
    ) {
        send_result = -1;
    }

    /*
     * 关闭写入端相当于向子进程发送 EOF。
     */
    if (close(pipe_descriptors[1]) != 0) {
        send_result = -1;
    }

    /*
     * 恢复原来的 SIGPIPE 处理方式。
     */
    if (
        sigaction(
            SIGPIPE,
            &previous_sigpipe,
            NULL
        ) != 0
    ) {
        send_result = -1;
    }

    if (
        wait_for_child(
            child_pid,
            &child_status
        ) != 0
    ) {
        return -1;
    }

    if (send_result != 0) {
        return -1;
    }

    /*
     * 子进程必须正常退出。
     */
    if (!WIFEXITED(child_status)) {
        return -1;
    }

    /*
     * 外部通知程序返回 0 表示发送成功。
     */
    if (WEXITSTATUS(child_status) != 0) {
        return -1;
    }

    return 0;
}
