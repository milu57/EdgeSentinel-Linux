#!/usr/bin/env bash

set -eu

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 edgesentinel_executable" >&2
    exit 1
fi

# 转换成绝对路径，后面切换目录后仍能找到程序。
edgesentinel_executable="$(realpath "$1")"

if [ ! -x "${edgesentinel_executable}" ]; then
    echo "Executable not found: ${edgesentinel_executable}" >&2
    exit 1
fi

test_directory="$(
    mktemp -d /tmp/edgesentinel-invalid-config-test.XXXXXX
)"

config_file="${test_directory}/invalid.conf"
output_file="${test_directory}/output.txt"
default_log_file="${test_directory}/logs/edgesentinel.log"

process_pid=""

cleanup()
{
    if (
        [ -n "${process_pid}" ] &&
        kill -0 "${process_pid}" 2>/dev/null
    ); then
        kill -TERM "${process_pid}" 2>/dev/null || true
        wait "${process_pid}" 2>/dev/null || true
    fi

    rm -rf "${test_directory}"
}

trap cleanup EXIT

# 故意写入无法解析的采样间隔。
cat > "${config_file}" <<CONFIG
monitor_interval=3abc
CONFIG

# 切换到临时目录。
# 默认日志 logs/edgesentinel.log 会创建在临时目录中，
# 不会污染项目原来的 logs 目录。
cd "${test_directory}"

"${edgesentinel_executable}" \
    -c "${config_file}" \
    > "${output_file}" 2>&1 &

process_pid=$!

sleep 2

if ! kill -0 "${process_pid}" 2>/dev/null; then
    echo "EdgeSentinel exited unexpectedly" >&2
    cat "${output_file}" >&2

    wait "${process_pid}" 2>/dev/null || true
    process_pid=""

    exit 1
fi

kill -INT "${process_pid}"

if wait "${process_pid}"; then
    run_status=0
else
    run_status=$?
fi

process_pid=""

if [ "${run_status}" -ne 0 ]; then
    echo "EdgeSentinel returned status ${run_status}" >&2
    cat "${output_file}" >&2
    exit 1
fi

# 检查程序是否发现了错误配置。
if ! grep -Fq \
    "using default configuration" \
    "${output_file}"
then
    echo "Default configuration warning was not found" >&2
    cat "${output_file}" >&2
    exit 1
fi

# 默认 monitor_interval 应为 1 秒。
if ! grep -Fq \
    "monitor_interval          : 1 second(s)" \
    "${output_file}"
then
    echo "Default monitor_interval was not applied" >&2
    cat "${output_file}" >&2
    exit 1
fi

# 默认情况下监控 EdgeSentinel 自身。
if ! grep -Fq \
    "process_pid               : 0 (self)" \
    "${output_file}"
then
    echo "Default process_pid was not applied" >&2
    cat "${output_file}" >&2
    exit 1
fi

if ! grep -Fq \
    "EdgeSentinel system monitor started." \
    "${output_file}"
then
    echo "Program did not start after configuration fallback" >&2
    cat "${output_file}" >&2
    exit 1
fi

if ! grep -Fq \
    "EdgeSentinel stopped safely." \
    "${output_file}"
then
    echo "Program did not stop safely" >&2
    cat "${output_file}" >&2
    exit 1
fi

# 使用默认配置后，应创建默认日志文件。
if [ ! -f "${default_log_file}" ]; then
    echo "Default log file was not created" >&2
    cat "${output_file}" >&2
    exit 1
fi

if ! grep -Fq \
    "EdgeSentinel started" \
    "${default_log_file}"
then
    echo "Startup message was not written to default log" >&2
    cat "${default_log_file}" >&2
    exit 1
fi

if ! grep -Fq \
    "EdgeSentinel stopped safely" \
    "${default_log_file}"
then
    echo "Shutdown message was not written to default log" >&2
    cat "${default_log_file}" >&2
    exit 1
fi

echo "invalid configuration fallback test passed"
