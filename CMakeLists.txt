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
