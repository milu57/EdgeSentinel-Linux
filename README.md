# EdgeSentinel-Linux

EdgeSentinel-Linux 是一个使用 C 语言开发的 Linux 系统与进程资源监控程序。

程序通过 Linux 提供的 `/proc` 虚拟文件系统、文件系统接口和系统调用，持续采集 CPU、内存、磁盘、网络、系统运行状态以及指定进程的运行信息，并根据配置的告警阈值输出：

- `NORMAL`
- `WARNING`
- `CRITICAL`

项目支持外部配置文件、日志记录、日志轮转、安全退出、systemd 服务管理、自动安装和安全卸载。

---

## 当前版本

**v1.3.0**

---

## 主要功能

### 系统监控

- 系统当前时间显示
- 系统运行时间监控
- 系统平均负载监控
- 系统 CPU 使用率监控
- 系统内存使用率监控
- 根文件系统磁盘使用率监控
- 网络累计接收和发送流量监控
- 网络实时上传和下载速度监控
- 自动忽略回环网络接口
- CPU、内存和磁盘告警分级
- 综合系统状态判断

### 进程监控

- 支持监控 EdgeSentinel 自身进程
- 支持通过 PID 监控指定进程
- 读取进程名称
- 读取进程 PID
- 读取父进程 PID
- 读取进程完整运行状态
- 读取进程驻留物理内存 `VmRSS`
- 将进程 `VmRSS` 从 kB 转换为 MiB
- 支持配置进程常驻内存告警阈值
- 进程内存 `NORMAL`、`WARNING`、`CRITICAL` 分级
- 进程内存告警等级变化日志
- 读取进程用户态累计 CPU 时间
- 读取进程内核态累计 CPU 时间
- 计算采样区间内的进程 CPU 使用率
- 进程 CPU `NORMAL`、`WARNING`、`CRITICAL` 分级
- 进程 CPU 告警等级变化日志
- 目标进程退出和不可用检测
- 目标进程不可用时继续执行系统监控

### 配置与日志

- 外部配置文件支持
- 支持通过 `-c` 指定配置文件
- 配置格式校验
- 配置数值合法性校验
- 配置错误时回退到默认值
- 程序启动和停止日志
- 系统资源周期日志
- 系统告警状态变化日志
- 进程可用状态变化日志
- 进程 CPU 告警状态变化日志
- 进程内存告警状态变化日志
- 日志文件自动轮转

### 服务管理

- `SIGINT` 安全退出
- `SIGTERM` 安全退出
- systemd 服务管理
- 开机自动启动
- 服务异常退出后自动重启
- 自动编译和安装脚本
- 安全卸载脚本
- 卸载时保留系统配置和历史日志

---

## 运行环境

推荐环境：

- Linux
- GCC 或其他支持 C11 的编译器
- CMake 3.10 或更高版本
- systemd
- Bash
- sudo

Ubuntu 或 Debian 系统可以安装基础编译工具：

```bash
sudo apt update
sudo apt install -y build-essential cmake
```

---

## 获取项目

```bash
git clone https://github.com/milu57/EdgeSentinel-Linux.git
cd EdgeSentinel-Linux
```

不要在已经存在的 `EdgeSentinel-Linux` 项目目录中再次执行 `git clone`，否则会产生嵌套仓库。

---

## 项目结构

```text
EdgeSentinel-Linux/
├── CMakeLists.txt
├── Makefile
├── README.md
├── .gitignore
│
├── config/
│   └── edgesentinel.conf
│
├── include/
│   ├── alert.h
│   ├── config.h
│   ├── cpu_monitor.h
│   ├── disk_monitor.h
│   ├── logger.h
│   ├── network.h
│   ├── process_monitor.h
│   ├── system_monitor.h
│   └── system_status.h
│
├── src/
│   ├── alert.c
│   ├── config.c
│   ├── cpu_monitor.c
│   ├── disk_monitor.c
│   ├── logger.c
│   ├── main.c
│   ├── network.c
│   ├── process_monitor.c
│   ├── system_monitor.c
│   └── system_status.c
│
├── tests/
│   └── test_process_monitor.c
│   └── process_memory_growth.c
│
├── scripts/
│   ├── install.sh
│   └── uninstall.sh
│
├── systemd/
│   └── edgesentinel.service
│
├── logs/
│   └── edgesentinel.log
│
└── build/
```

其中：

- `include/`：保存各模块的头文件和公开接口；
- `src/`：保存各模块的 C 源文件；
- `config/`：保存项目配置模板；
- `tests/`：保存独立测试程序；
- `scripts/`：保存安装和卸载脚本；
- `systemd/`：保存 systemd 服务文件；
- `logs/`：手动运行时保存程序日志；
- `build/`：保存 CMake 自动生成的构建文件和可执行程序。

`build/` 中的大部分内容由 CMake 自动生成，一般不提交到 Git 仓库。

---

## 手动编译

### 1. 配置构建目录

```bash
cmake -S . -B build
```

其中：

- `-S .`：指定当前目录为源码目录；
- `-B build`：指定 `build/` 为构建目录。

### 2. 编译项目

```bash
cmake --build build
```

编译完成后，可执行程序位于：

```text
build/edgesentinel
```

### 3. 全量重新编译

```bash
cmake --build build --clean-first
```

该命令会先清理旧的目标文件，再重新编译整个项目。

---

## 手动运行

使用项目配置文件运行：

```bash
./build/edgesentinel -c config/edgesentinel.conf
```

不指定 `-c` 时，程序默认尝试读取：

```text
config/edgesentinel.conf
```

运行：

```bash
./build/edgesentinel
```

停止前台程序：

```text
Ctrl+C
```

程序收到 `SIGINT` 后会安全退出，并写入停止日志。

---

## 后台运行

可以将 EdgeSentinel 放到后台运行：

```bash
./build/edgesentinel -c config/edgesentinel.conf &
```

查看刚启动的后台进程 PID：

```bash
echo $!
```

也可以保存 PID：

```bash
./build/edgesentinel -c config/edgesentinel.conf &
EDGESENTINEL_PID=$!
```

停止后台程序：

```bash
kill -TERM "${EDGESENTINEL_PID}"
```

等待进程退出：

```bash
wait "${EDGESENTINEL_PID}"
```

程序收到 `SIGTERM` 后也会执行安全退出流程。

---

## 配置文件

项目配置模板位于：

```text
config/edgesentinel.conf
```

完整配置示例：

```ini
# EdgeSentinel-Linux configuration file

# 监控采样间隔，单位：秒
monitor_interval=3

# 要监控的进程 PID
# 0 表示监控 EdgeSentinel 自身
# 大于 0 表示监控指定 PID
process_pid=0

# 整个系统的 CPU 使用率告警阈值
cpu_warning_threshold=70.0
cpu_critical_threshold=90.0

# 被监控进程的 CPU 使用率告警阈值
# 100% 约等于占满一个 CPU 核心
# 多线程进程的使用率可能超过 100%
process_cpu_warning_threshold=70.0
process_cpu_critical_threshold=90.0

# 被监控进程的常驻内存告警阈值，单位：MiB
process_memory_warning_threshold_mib=100.0
process_memory_critical_threshold_mib=200.0

# 内存使用率告警阈值
memory_warning_threshold=75.0
memory_critical_threshold=90.0

# 磁盘使用率告警阈值
disk_warning_threshold=80.0
disk_critical_threshold=90.0

# 日志文件位置
log_file=logs/edgesentinel.log

# 单个日志文件最大大小，单位：字节
# 1048576 字节等于 1 MiB
log_max_size=1048576
```

### 配置项说明

- `monitor_interval`：监控采样间隔，单位为秒；
- `process_pid`：目标进程 PID；
- `cpu_warning_threshold`：系统 CPU WARNING 阈值；
- `cpu_critical_threshold`：系统 CPU CRITICAL 阈值；
- `process_cpu_warning_threshold`：目标进程 CPU WARNING 阈值；
- `process_cpu_critical_threshold`：目标进程 CPU CRITICAL 阈值；
- `process_memory_warning_threshold_mib`：目标进程常驻内存 WARNING 阈值，单位为 MiB；
- `process_memory_critical_threshold_mib`：目标进程常驻内存 CRITICAL 阈值，单位为 MiB；
- `memory_warning_threshold`：内存 WARNING 阈值；
- `memory_critical_threshold`：内存 CRITICAL 阈值；
- `disk_warning_threshold`：磁盘 WARNING 阈值；
- `disk_critical_threshold`：磁盘 CRITICAL 阈值；
- `log_file`：程序日志文件路径；
- `log_max_size`：单个日志文件的最大大小，单位为字节。

对于普通系统资源阈值，需要满足：

```text
0 <= WARNING < CRITICAL <= 100
```

进程 CPU 使用率可能超过 `100%`，因此进程 CPU 阈值只要求：

```text
0 <= WARNING < CRITICAL
```

进程常驻内存阈值使用 MiB 作为单位，数值也可以超过 `100`，需要满足：

```text
0 <= WARNING < CRITICAL

---

## 进程监控

EdgeSentinel 可以监控自身进程，也可以监控指定 PID 对应的进程。

### 监控自身

配置：

```ini
process_pid=0
```

这里的 `0` 不是一个真正要读取的 PID，而是 EdgeSentinel 自己规定的特殊配置值。

程序检测到：

```text
process_pid=0
```

后，会调用：

```c
getpid()
```

自动取得 EdgeSentinel 自身的真实 PID。

因此每次重新启动时，即使 EdgeSentinel 的 PID 发生变化，也不需要修改配置文件。

### 监控指定进程

例如监控 PID 为 `1234` 的进程：

```ini
process_pid=1234
```

程序将读取：

```text
/proc/1234/status
/proc/1234/stat
```

---

## 进程基本信息来源

EdgeSentinel 从：

```text
/proc/<pid>/status
```

读取以下字段：

```text
Name
State
PPid
VmRSS
```

对应保存的信息包括：

- 进程名称；
- 完整进程状态；
- 父进程 PID；
- 驻留物理内存。

示例输出：

```text
Process:          edgesentinel PID=13183 PPID=1949
Process State:    R (running)
Process Memory:   1824 kB
```

常见进程状态包括：

```text
R (running)        正在运行
S (sleeping)       可中断睡眠
D (disk sleep)     不可中断睡眠
T (stopped)        已暂停
Z (zombie)         僵尸进程
I (idle)           空闲内核线程
```

---

## 进程 CPU 使用率

EdgeSentinel 从：

```text
/proc/<pid>/stat
```

读取：

- 第 14 个字段 `utime`；
- 第 15 个字段 `stime`。

其中：

```text
utime
    进程在用户态运行的累计 CPU 时间

stime
    进程在内核态运行的累计 CPU 时间
```

这些值的单位是 `clock ticks`，不是秒。

程序通过：

```c
sysconf(_SC_CLK_TCK)
```

获取当前系统每秒包含多少个 clock tick。

进程 CPU 使用率不是通过一次读取直接获得，而是需要前后两次采样：

```text
第一次累计 CPU 时间
        ↓
等待一个采样区间
        ↓
第二次累计 CPU 时间
        ↓
计算累计时间增量
        ↓
换算成 CPU 秒数
        ↓
除以实际经过时间
        ↓
得到进程 CPU 使用率
```

第一次采样只有基准数据，因此会显示：

```text
Process CPU:      [COLLECTING]
```

从第二次采样开始，显示具体使用率：

```text
Process CPU:        0.00% [NORMAL]
Process CPU:       75.00% [WARNING]
Process CPU:       99.00% [CRITICAL]
```

当前采用的进程 CPU 百分比定义是：

```text
100% 约等于占满一个 CPU 核心
```

如果一个多线程进程同时使用多个 CPU 核心，进程 CPU 使用率可能超过 `100%`。

例如：

```text
100%  约等于占满一个核心
200%  约等于占满两个核心
```

---

## 进程不可用处理

如果目标 PID：

- 不存在；
- 已经退出；
- 无法读取；
- 在采样过程中消失；

EdgeSentinel 会显示：

```text
Process:          PID=1234 [UNAVAILABLE]
```

并写入日志：

```text
[WARNING] Monitored process unavailable: PID=1234
```

目标进程不可用不会导致 EdgeSentinel 退出。

以下系统监控仍会继续执行：

- 系统 CPU；
- 系统内存；
- 系统磁盘；
- 系统负载；
- 系统运行时间；
- 网络流量；
- 网络速度。

---

## 进程 CPU 告警

进程 CPU 使用率根据配置阈值划分为三个等级。

例如：

```ini
process_cpu_warning_threshold=70.0
process_cpu_critical_threshold=90.0
```

判断规则：

```text
CPU < 70%
    NORMAL

70% <= CPU < 90%
    WARNING

CPU >= 90%
    CRITICAL
```

输出示例：

```text
Process CPU:       25.00% [NORMAL]
Process CPU:       75.00% [WARNING]
Process CPU:       99.00% [CRITICAL]
```

程序只在告警等级发生变化时写入日志。

例如：

```text
[WARNING] Process CPU status changed:
NORMAL -> WARNING PID=13204 CPU=36.33%
```

```text
[CRITICAL] Process CPU status changed:
WARNING -> CRITICAL PID=13204 CPU=99.89%
```

如果进程持续保持同一告警等级，不会在每个采样周期重复写入相同的状态变化日志。

---

## 独立进程监控测试

项目提供：

```text
tests/test_process_monitor.c
```

用于独立测试进程信息读取和进程 CPU 使用率计算。

### 编译测试程序

```bash
gcc \
    -Wall \
    -Wextra \
    -Wpedantic \
    -std=c11 \
    -D_POSIX_C_SOURCE=200809L \
    -Iinclude \
    tests/test_process_monitor.c \
    src/process_monitor.c \
    -o build/test_process_monitor
```

### 运行测试程序

```bash
./build/test_process_monitor
```

测试程序会：

1. 获取测试程序自身 PID；
2. 读取自身进程基本信息；
3. 读取第一次累计 CPU tick；
4. 连续执行约两秒 CPU 密集计算；
5. 读取第二次累计 CPU tick；
6. 计算测试程序的进程 CPU 使用率。

正常结果类似：

```text
Testing process monitor...
Current test process PID: 12833

Process information:
  PID:             12833
  Parent PID:      1949
  Name:            test_process_mo
  State:           R (running)
  Resident memory: 1592 kB

Performing CPU-intensive work for about 2 seconds...

CPU sampling result:
  Previous user ticks:   0
  Previous system ticks: 0
  Current user ticks:    198
  Current system ticks:  0
  Elapsed time:          2.000046 seconds
  Process CPU usage:     99.00%
```

单线程持续计算时，CPU 使用率通常接近 `100%`。

---

## 高 CPU 进程测试

可以使用 `yes` 命令创建一个高 CPU 测试进程：

```bash
yes > /dev/null &
```

查看后台进程 PID：

```bash
echo $!
```

假设得到：

```text
13204
```

修改配置：

```ini
process_pid=13204
```

启动 EdgeSentinel：

```bash
./build/edgesentinel -c config/edgesentinel.conf
```

通常可以看到：

```text
Process:          yes PID=13204 PPID=1949
Process State:    R (running)
Process CPU:       99.00% [CRITICAL]
```

测试完成后必须关闭测试进程：

```bash
kill 13204
```

然后将配置恢复为：

```ini
process_pid=0
```

---

## 查看程序日志

手动运行时，默认日志文件为：

```text
logs/edgesentinel.log
```

实时查看：

```bash
tail -f logs/edgesentinel.log
```

停止查看：

```text
Ctrl+C
```

查看最后 30 行：

```bash
tail -n 30 logs/edgesentinel.log
```

查看最后 100 行：

```bash
tail -n 100 logs/edgesentinel.log
```

查看全部日志：

```bash
cat logs/edgesentinel.log
```

分页查看：

```bash
less logs/edgesentinel.log
```

在 `less` 中按：

```text
q
```

退出。

---

## 日志示例

程序启动：

```text
[INFO] EdgeSentinel started
```

周期系统资源日志：

```text
[NORMAL] CPU=51.26% Memory=15.00% Disk=18.90%
```

系统状态变化：

```text
[WARNING] System status changed:
NORMAL -> WARNING CPU=75.00% Memory=45.00% Disk=30.00%
```

目标进程不可用：

```text
[WARNING] Monitored process unavailable: PID=13177
```

进程 CPU 状态变化：

```text
[WARNING] Process CPU status changed:
NORMAL -> WARNING PID=13204 CPU=36.33%
```

```text
[CRITICAL] Process CPU status changed:
WARNING -> CRITICAL PID=13204 CPU=99.89%
```

程序安全停止：

```text
[INFO] EdgeSentinel stopped safely
```

---

## 日志轮转

配置项：

```ini
log_max_size=1048576
```

表示单个日志文件最大大小为：

```text
1048576 字节
```

也就是：

```text
1 MiB
```

当日志文件达到配置大小后，程序会执行日志轮转，避免单个日志文件无限增长。

---

## 一键安装

首先确保安装脚本具有执行权限：

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

安装脚本会使用普通用户执行 CMake 编译，只在写入系统目录和管理 systemd 时调用 `sudo`。

安装脚本会自动完成：

1. 检查所需命令和项目文件；
2. 使用 CMake 配置和编译；
3. 停止旧的 EdgeSentinel 服务；
4. 创建系统配置目录；
5. 创建系统日志目录；
6. 安装可执行程序；
7. 安装 systemd 服务文件；
8. 首次安装时创建系统配置文件；
9. 重新加载 systemd；
10. 启用并启动 EdgeSentinel 服务。

---

## 安装位置

安装完成后，主要文件位于：

```text
/usr/local/bin/edgesentinel
/etc/edgesentinel/edgesentinel.conf
/etc/systemd/system/edgesentinel.service
/var/log/edgesentinel/
```

各文件用途：

- `/usr/local/bin/edgesentinel`：安装后的可执行程序；
- `/etc/edgesentinel/edgesentinel.conf`：当前机器实际使用的配置文件；
- `/etc/systemd/system/edgesentinel.service`：systemd 服务定义；
- `/var/log/edgesentinel/`：程序日志目录。

再次运行安装脚本时，已有的：

```text
/etc/edgesentinel/edgesentinel.conf
```

不会被覆盖。

---

## systemd 服务管理

### 查看服务状态

```bash
systemctl status edgesentinel --no-pager
```

### 启动服务

```bash
sudo systemctl start edgesentinel
```

### 停止服务

```bash
sudo systemctl stop edgesentinel
```

### 重启服务

```bash
sudo systemctl restart edgesentinel
```

### 启用开机自启动并立即启动

```bash
sudo systemctl enable --now edgesentinel
```

### 停止服务并取消开机自启动

```bash
sudo systemctl disable --now edgesentinel
```

### 检查服务是否正在运行

```bash
systemctl is-active edgesentinel
```

### 检查是否已启用开机启动

```bash
systemctl is-enabled edgesentinel
```

---

## 修改系统配置

安装后，systemd 服务实际使用的配置文件是：

```text
/etc/edgesentinel/edgesentinel.conf
```

编辑：

```bash
sudo nano /etc/edgesentinel/edgesentinel.conf
```

修改配置后重启服务：

```bash
sudo systemctl restart edgesentinel
```

项目目录中的：

```text
config/edgesentinel.conf
```

是安装模板，不是已经安装的 systemd 服务当前直接读取的机器配置。

---

## 查看 systemd 日志

查看最近 30 行：

```bash
sudo journalctl -u edgesentinel -n 30 --no-pager
```

查看本次开机后的服务日志：

```bash
sudo journalctl -u edgesentinel -b --no-pager
```

实时查看：

```bash
sudo journalctl -u edgesentinel -f
```

---

## 查看安装后的程序日志

查看最近 30 行：

```bash
sudo tail -n 30 /var/log/edgesentinel/edgesentinel.log
```

实时查看：

```bash
sudo tail -f /var/log/edgesentinel/edgesentinel.log
```

查看日志目录占用：

```bash
sudo du -sh /var/log/edgesentinel
```

---

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

为了避免误删用户数据，卸载脚本默认保留：

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

---

## Git 提交前检查

查看修改状态：

```bash
git status
```

查看具体修改：

```bash
git diff
```

重新编译：

```bash
cmake --build build --clean-first
```

运行独立测试：

```bash
./build/test_process_monitor
```

运行主程序：

```bash
./build/edgesentinel -c config/edgesentinel.conf
```

正常停止：

```text
Ctrl+C
```

查看最新日志：

```bash
tail -n 30 logs/edgesentinel.log
```

---

## 版本说明

### v1.2.0

- 新增指定 PID 的进程监控；
- 支持 `process_pid=0` 自动监控 EdgeSentinel 自身；
- 从 `/proc/<pid>/status` 读取进程名称、状态、PPID 和 `VmRSS`；
- 从 `/proc/<pid>/stat` 读取用户态和内核态累计 CPU 时间；
- 新增进程 CPU 使用率计算；
- 新增进程 CPU WARNING 和 CRITICAL 告警阈值；
- 新增进程 CPU 告警等级变化日志；
- 新增目标进程不可用检测；
- 目标进程退出后系统监控仍可继续运行；
- 新增独立进程监控测试程序。

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

### v0.8.0

- 新增程序日志记录；
- 支持后台运行；
- 支持安全停止；
- 增加日志文件输出。

### v0.7.0

- 新增 CPU、内存和磁盘告警等级；
- 支持 `NORMAL`、`WARNING` 和 `CRITICAL`；
- 增加综合系统状态判断。

### v0.6.0

- 新增网络累计流量监控；
- 新增实时下载速度；
- 新增实时上传速度；
- 自动忽略回环接口。

### v0.5.0

- 新增系统运行时间；
- 新增 1、5、15 分钟平均负载；
- 新增当前运行和总进程数量读取。

### v0.4.0

- 新增根文件系统磁盘容量监控；
- 使用 `statvfs()` 获取磁盘信息。

### v0.3.0

- 新增系统 CPU 使用率监控；
- 通过 `/proc/stat` 前后两次采样计算 CPU 使用率。

### v0.2.0

- 新增 `SIGINT` 信号处理；
- 支持通过 `Ctrl+C` 安全退出。

### v0.1.0

- 新增内存监控；
- 从 `/proc/meminfo` 读取 `MemTotal` 和 `MemAvailable`；
- 计算系统内存使用率。

---

## 后续计划

- 支持根据进程名称自动查找 PID；
- 支持同时监控多个目标进程；
- 增加进程内存告警阈值；
- 增加进程内存告警状态变化日志；
- 支持 `SIGHUP` 热加载配置；
- 增加自动化单元测试；
- 增加网络接口过滤配置；
- 增加更多通知和告警方式；
- 增加 JSON 或其他结构化输出格式；
- 为后续边缘设备部署进行适配。
