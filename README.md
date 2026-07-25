# EdgeSentinel-Linux

EdgeSentinel-Linux 是一个使用 C 语言开发的 Linux 系统资源监控程序。

程序通过 Linux 系统接口采集 CPU、内存、磁盘、网络和系统运行状态，并根据配置的告警阈值输出 `NORMAL`、`WARNING` 或 `CRITICAL` 状态。

项目支持外部配置文件、日志记录、日志轮转、安全退出、systemd 服务管理、自动安装和安全卸载。

## 当前版本

v1.1.0

## 主要功能

- 系统运行时间监控
- 系统负载监控
- CPU 使用率监控
- 内存使用率监控
- 磁盘使用率监控
- 网络累计流量监控
- 网络实时上传和下载速度监控
- CPU、内存和磁盘告警分级
- 综合系统状态判断
- 外部配置文件支持
- 配置格式和数值合法性校验
- 配置错误时回退到默认值
- 程序运行日志记录
- 日志文件自动轮转
- `SIGINT` 和 `SIGTERM` 安全退出
- systemd 服务管理
- 开机自动启动
- 服务异常退出后自动重启
- 自动编译和安装脚本
- 安全卸载脚本

## 运行环境

推荐环境：

- Linux
- GCC 或其他支持 C11 的编译器
- CMake 3.x
- systemd
- Bash
- sudo

Ubuntu 或 Debian 系统可以安装基础编译工具：

```bash
sudo apt update
sudo apt install -y build-essential cmake
