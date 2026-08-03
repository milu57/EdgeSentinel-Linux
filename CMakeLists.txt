cmake_minimum_required(VERSION 3.10)

project(EdgeSentinelLinux C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 自动查找 src 目录下的所有 .c 文件
file(GLOB EDGESENTINEL_SOURCES
    CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.c"
)

add_executable(edgesentinel
    ${EDGESENTINEL_SOURCES}
)

target_include_directories(edgesentinel PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_compile_options(edgesentinel PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Build the process monitor test program.
add_executable(test_process_monitor
    tests/test_process_monitor.c
    src/process_monitor.c
)

target_include_directories(test_process_monitor PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_compile_options(test_process_monitor PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

enable_testing()

add_test(
    NAME process_monitor_test
    COMMAND test_process_monitor
)

# Build the configuration test program.
add_executable(test_config
    tests/test_config.c
    src/config.c
)

target_include_directories(test_config PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_compile_options(test_config PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME config_test
    COMMAND test_config
)


# Build the alert module test program.
add_executable(test_alert
    tests/test_alert.c
    src/alert.c
)

target_include_directories(test_alert PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_compile_options(test_alert PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME alert_test
    COMMAND test_alert
)


# Build the logger module test program.
add_executable(test_logger
    tests/test_logger.c
    src/logger.c
)

target_include_directories(test_logger PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_compile_options(test_logger PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME logger_test
    COMMAND test_logger
)


# Build the system resource smoke test program.
add_executable(test_system_resources
    tests/test_system_resources.c
    src/system_monitor.c
    src/disk_monitor.c
    src/system_status.c
)

target_include_directories(test_system_resources PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_compile_options(test_system_resources PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME system_resources_test
    COMMAND test_system_resources
)


# Build CPU and network calculation tests.
add_executable(test_calculations
    tests/test_calculations.c
    src/cpu_monitor.c
    src/network.c
    src/config.c
)

target_include_directories(test_calculations PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_compile_options(test_calculations PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME calculations_test
    COMMAND test_calculations
)


# Run the complete EdgeSentinel startup and shutdown test.
add_test(
    NAME startup_integration_test
    COMMAND bash
        "${PROJECT_SOURCE_DIR}/tests/test_startup.sh"
        "$<TARGET_FILE:edgesentinel>"
)

set_tests_properties(
    startup_integration_test
    PROPERTIES
        TIMEOUT 10
)


# Verify fallback to defaults when configuration parsing fails.
add_test(
    NAME invalid_config_startup_test
    COMMAND bash
        "${PROJECT_SOURCE_DIR}/tests/test_invalid_config_startup.sh"
        "$<TARGET_FILE:edgesentinel>"
)

set_tests_properties(
    invalid_config_startup_test
    PROPERTIES
        TIMEOUT 10
)

# Build the output module test program.
add_executable(test_output
    tests/test_output.c
    src/output.c
    src/alert.c
)

target_include_directories(test_output PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_compile_options(test_output PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME output_test
    COMMAND test_output
)

# Verify that JSON mode writes only JSON Lines to stdout.
add_test(
    NAME json_output_integration_test
    COMMAND bash
        "${PROJECT_SOURCE_DIR}/tests/test_json_output.sh"
        "$<TARGET_FILE:edgesentinel>"
)

set_tests_properties(
    json_output_integration_test
    PROPERTIES
        TIMEOUT 10
)

# Build the alert event module test program.
add_executable(test_alert_event
    tests/test_alert_event.c
    src/alert_event.c
)

target_include_directories(test_alert_event PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_compile_options(test_alert_event PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME alert_event_test
    COMMAND test_alert_event
)

# Build the notifier module test program.
add_executable(test_notifier
    tests/test_notifier.c
    src/notifier.c
    src/alert_event.c
    src/alert.c
)

target_include_directories(test_notifier PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_compile_options(test_notifier PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME notifier_test
    COMMAND test_notifier
)

# Build the memory target used by notification integration tests.
add_executable(notification_memory_target
    tests/notification_memory_target.c
)

target_compile_options(notification_memory_target PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Verify the complete process memory notification path.
add_test(
    NAME notification_integration_test
    COMMAND bash
        "${PROJECT_SOURCE_DIR}/tests/test_notification_integration.sh"
        "$<TARGET_FILE:edgesentinel>"
        "$<TARGET_FILE:notification_memory_target>"
)

set_tests_properties(
    notification_integration_test
    PROPERTIES
        TIMEOUT 20
)
