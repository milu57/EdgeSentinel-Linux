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
