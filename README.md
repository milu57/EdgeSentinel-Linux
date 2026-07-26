# EdgeSentinel-Linux

EdgeSentinel-Linux 是一个使用 C 语言开发的 Linux 系统资源监控程序。

程序通过 Linux 系统接口采集 CPU、内存、磁盘、网络和系统运行状态，并根据配置的告警阈值输出 `NORMAL`、`WARNING` 或 `CRITICAL` 状态。

项目支持外部配置文件、日志记录、日志轮转、安全退出、systemd 服务管理、自动安装和安全卸载。

## 当前版本

v1.1.1

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
```

## 获取项目

```bash
git clone https://github.com/milu57/EdgeSentinel-Linux.git
cd EdgeSentinel-Linux
```

不要在已经存在的 `EdgeSentinel-Linux` 项目目录中再次执行 `git clone`，否则会产生嵌套仓库。

## 手动编译

配置构建目录：

```bash
cmake -S . -B build
```

编译：

```bash
cmake --build build
```

编译完成后，可执行程序位于：

```text
build/edgesentinel
```

## 手动运行

使用项目配置文件运行：

```bash
./build/edgesentinel -c config/edgesentinel.conf
```

不指定 `-c` 时，程序默认尝试读取：

```text
config/edgesentinel.conf
```

停止前台程序：

```text
Ctrl+C
```

程序收到 `SIGINT` 后会安全退出。

后台测试可以使用：

```bash
./build/edgesentinel -c config/edgesentinel.conf &
EDGESENTINEL_PID=$!

sleep 5

kill -TERM "${EDGESENTINEL_PID}"
wait "${EDGESENTINEL_PID}"
```

程序收到 `SIGTERM` 后也会执行安全退出流程。

## 配置文件

项目配置模板位于：

```text
config/edgesentinel.conf
```

主要配置包括：

```ini
monitor_interval=3

cpu_warning_threshold=70.0
cpu_critical_threshold=90.0

memory_warning_threshold=75.0
memory_critical_threshold=90.0

disk_warning_threshold=80.0
disk_critical_threshold=90.0

log_file=logs/edgesentinel.log
log_max_size=1048576
```

其中：

- `monitor_interval`：监控采样间隔，单位为秒；
- `cpu_warning_threshold`：CPU WARNING 阈值；
- `cpu_critical_threshold`：CPU CRITICAL 阈值；
- `memory_warning_threshold`：内存 WARNING 阈值；
- `memory_critical_threshold`：内存 CRITICAL 阈值；
- `disk_warning_threshold`：磁盘 WARNING 阈值；
- `disk_critical_threshold`：磁盘 CRITICAL 阈值；
- `log_file`：日志文件路径；
- `log_max_size`：单个日志文件的最大大小，单位为字节。

## 一键安装

先确保安装脚本具有执行权限：

```bash
chmod +x scripts/install.sh
```

执行：

```bash
./scripts/install.sh
```

不要使用：

```bash
sudo ./scripts/install.sh
```

安装脚本会使用普通用户完成 CMake 编译，只在写入系统目录和管理 systemd 时调用 `sudo`。

安装脚本会自动完成：

1. 检查所需命令和项目文件；
2. 使用 CMake 配置并编译；
3. 停止旧的 EdgeSentinel 服务；
4. 创建系统配置目录和日志目录；
5. 安装可执行程序；
6. 安装 systemd 服务文件；
7. 首次安装时创建系统配置文件；
8. 重新加载 systemd；
9. 启用并启动 EdgeSentinel 服务。

## 安装位置

安装完成后，主要文件位于：

```text
/usr/local/bin/edgesentinel
/etc/edgesentinel/edgesentinel.conf
/etc/systemd/system/edgesentinel.service
/var/log/edgesentinel/
```

各文件用途：

- `/usr/local/bin/edgesentinel`：系统安装后的可执行程序；
- `/etc/edgesentinel/edgesentinel.conf`：当前机器实际使用的配置文件；
- `/etc/systemd/system/edgesentinel.service`：systemd 服务定义；
- `/var/log/edgesentinel/`：程序日志目录。

再次运行安装脚本时，已有的：

```text
/etc/edgesentinel/edgesentinel.conf
```

不会被覆盖。

## systemd 服务管理

查看服务状态：

```bash
systemctl status edgesentinel --no-pager
```

启动服务：

```bash
sudo systemctl start edgesentinel
```

停止服务：

```bash
sudo systemctl stop edgesentinel
```

重新启动服务：

```bash
sudo systemctl restart edgesentinel
```

启用开机自启动并立即启动：

```bash
sudo systemctl enable --now edgesentinel
```

停止服务并取消开机自启动：

```bash
sudo systemctl disable --now edgesentinel
```

检查运行状态：

```bash
systemctl is-active edgesentinel
```

检查开机自启动状态：

```bash
systemctl is-enabled edgesentinel
```

## 修改系统配置

安装后实际使用的配置文件是：

```text
/etc/edgesentinel/edgesentinel.conf
```

编辑：

```bash
sudo nano /etc/edgesentinel/edgesentinel.conf
```

修改配置后重新启动服务：

```bash
sudo systemctl restart edgesentinel
```

项目中的：

```text
config/edgesentinel.conf
```

是安装模板，不是已经安装服务当前直接读取的机器配置。

## 查看日志

查看 systemd 日志：

```bash
sudo journalctl -u edgesentinel -n 30 --no-pager
```

只查看本次开机后的服务日志：

```bash
sudo journalctl -u edgesentinel -b --no-pager
```

实时查看 systemd 日志：

```bash
sudo journalctl -u edgesentinel -f
```

查看程序日志：

```bash
sudo tail -n 30 /var/log/edgesentinel/edgesentinel.log
```

查看日志目录占用：

```bash
sudo du -sh /var/log/edgesentinel
```

程序日志达到 `log_max_size` 后会执行日志轮转。

## 安全卸载

确保卸载脚本具有执行权限：

```bash
chmod +x scripts/uninstall.sh
```

执行：

```bash
./scripts/uninstall.sh
```

不要使用：

```bash
sudo ./scripts/uninstall.sh
```

卸载脚本会：

- 停止 EdgeSentinel 服务；
- 取消开机自动启动；
- 删除 `/usr/local/bin/edgesentinel`；
- 删除 `/etc/systemd/system/edgesentinel.service`；
- 重新加载 systemd。

为避免误删用户数据，卸载脚本默认保留：

```text
/etc/edgesentinel/
/var/log/edgesentinel/
```

因此系统配置和历史日志不会随程序一起删除。

重新安装：

```bash
./scripts/install.sh
```

重新安装时会继续使用已经保留的机器配置。

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
├── scripts/
│   ├── install.sh
│   └── uninstall.sh
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
├── systemd/
│   └── edgesentinel.service
└── build/
```

`build/` 是 CMake 构建输出目录，一般不提交其中自动生成的文件。

## 版本说明

### v1.1.1

- 补全 README 安装、配置、服务管理和卸载文档；
- 清理配置模板中的重复配置键；
- 保持 v1.1.0 功能兼容。

### v1.1.0

- 新增安全卸载脚本；
- 卸载时保留系统配置和历史日志；
- 支持卸载后重新安装。

### v1.0.0

- 支持通过 `-c` 指定配置文件；
- 支持 `SIGTERM` 安全退出；
- 新增 systemd 服务；
- 支持开机自动启动；
- 新增自动安装脚本；
- 安装时保留已有机器配置。

### v0.9.0

- 完成外部配置文件支持；
- 增加配置格式与数值合法性校验；
- 配置错误时回退到默认值。

## 后续计划

- 进程信息采集；
- 进程 CPU 和内存占用监控；
- Top CPU 和 Top Memory 进程排序；
- 标准命令行参数；
- JSON 输出；
- 自动化测试和持续集成；
- ARM Linux 开发板部署。

## License

本项目目前用于 Linux 系统编程学习、功能验证和项目实践。
