# EdgeSentinel-Linux

EdgeSentinel-Linux 是一个使用 C 语言开发的 Linux 系统资源监控程序。

项目通过 Linux 系统接口采集 CPU、内存、磁盘、网络和系统运行状态，并根据配置的告警阈值输出 NORMAL、WARNING 或 CRITICAL 状态。同时支持日志记录、日志轮转和外部配置文件。

## 当前版本

v0.9.0

## 功能

- 系统运行时间监控
- 系统负载监控
- CPU 使用率监控
- 内存使用率监控
- 磁盘使用率监控
- 网络流量与实时网速监控
- CPU、内存和磁盘告警分级
- 综合系统状态判断
- 程序启动、状态变化和退出日志
- 日志文件自动轮转
- 外部配置文件支持
- 配置格式和数值合法性校验
- 配置错误时自动回退到默认值
- Ctrl+C 安全退出

## 项目结构

```text
EdgeSentinel-Linux/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── config/
│   └── edgesentinel.conf
├── include/
│   ├── alert.h
│   ├── config.h
│   ├── cpu_monitor.h
│   ├── disk_monitor.h
│   ├── logger.h
│   ├── network.h
│   ├── system_monitor.h
│   └── system_status.h
├── src/
│   ├── alert.c
│   ├── config.c
│   ├── cpu_monitor.c
│   ├── disk_monitor.c
│   ├── logger.c
│   ├── main.c
│   ├── network.c
│   ├── system_monitor.c
│   └── system_status.c
└── build/
