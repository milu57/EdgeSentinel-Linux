# EdgeSentinel-Linux

EdgeSentinel-Linux 是一个面向 Linux 边缘设备的系统状态监测、异常判断与告警项目。

项目将从基础的 Linux 系统资源监测开始，逐步加入配置管理、日志记录、多线程、网络通信、嵌入式 Linux 开发板移植、传感器接入以及边缘 AI 推理功能。

本项目主要用于学习和实践 C 语言、Linux 应用开发、系统编程、网络编程、嵌入式 Linux 和边缘 AI，并作为个人求职项目持续维护。

---

## 项目状态

当前版本：`v0.1.0-dev`

当前阶段：项目基础工程搭建与 Linux 内存监测功能开发。

目前已经完成：

- 建立项目目录结构
- 创建 Git 和 GitHub 仓库
- 配置 `.gitignore`
- 创建基础配置文件
- 创建 C 源文件和头文件
- 规划 Makefile 构建流程

当前正在开发：

- 读取 `/proc/meminfo`
- 获取系统总内存和可用内存
- 计算内存使用率
- 根据阈值判断内存状态
- 在终端显示监测结果

> 当前项目仍处于早期开发阶段，部分规划功能尚未实现。

---

## 项目目标

EdgeSentinel-Linux 最终计划实现以下功能：

### Linux 系统状态监测

- 内存使用率监测
- CPU 使用率监测
- 系统负载监测
- 磁盘使用率监测
- CPU 温度监测
- 系统运行时间监测
- 网络接口状态监测

### 异常判断与告警

- 支持自定义告警阈值
- 内存使用率过高告警
- CPU 使用率过高告警
- 磁盘空间不足告警
- 设备温度过高告警
- 传感器异常告警
- 本地终端告警
- 日志告警记录

### Linux 工程化功能

- 多文件 C 项目结构
- Makefile 自动构建
- 配置文件解析
- 日志管理
- 命令行参数
- GDB 调试
- 单元测试
- 错误处理
- 信号处理
- 守护进程运行

### 并发与网络通信

- 多线程状态采集
- 互斥锁和线程同步
- TCP 或 HTTP 状态上报
- JSON 数据封装
- 网络断线重连
- 远程设备状态查询
- 异常信息远程上报

### 嵌入式 Linux 与边缘 AI

- ARM Linux 开发板移植
- 交叉编译
- GPIO 控制
- 温湿度传感器接入
- 烟雾或气体传感器接入
- 蜂鸣器和指示灯控制
- USB 或 CSI 摄像头接入
- 轻量级目标检测
- ONNX Runtime 或 NCNN 推理
- 本地智能异常判断

---

## 项目结构

```text
EdgeSentinel-Linux/
├── src/
│   ├── main.c
│   └── system_monitor.c
├── include/
│   └── system_monitor.h
├── config/
│   └── edgesentinel.conf
├── logs/
│   └── .gitkeep
├── build/
├── tests/
│   └── .gitkeep
├── docs/
│   └── .gitkeep
├── scripts/
│   └── .gitkeep
├── Makefile
├── README.md
└── .gitignore
```

### 目录说明

| 路径 | 说明 |
|---|---|
| `src/` | 存放 C 语言源文件 |
| `include/` | 存放项目头文件 |
| `config/` | 存放运行配置文件 |
| `logs/` | 存放程序运行日志 |
| `build/` | 存放编译生成的目标文件 |
| `tests/` | 存放测试代码 |
| `docs/` | 存放设计文档和开发记录 |
| `scripts/` | 存放构建、测试和部署脚本 |
| `Makefile` | 定义项目编译和清理规则 |
| `README.md` | 项目说明文档 |
| `.gitignore` | 定义 Git 不跟踪的文件 |

---

## 核心模块规划

### `main.c`

程序入口，负责：

- 初始化项目模块
- 调用系统状态采集函数
- 输出监测结果
- 控制程序运行流程
- 处理程序退出状态

### `system_monitor.c`

系统状态监测模块，计划负责：

- 读取 `/proc/meminfo`
- 读取 `/proc/stat`
- 读取 `/proc/loadavg`
- 获取磁盘使用率
- 获取系统温度
- 计算资源使用率
- 返回统一的系统状态数据

### `system_monitor.h`

对外声明：

- 系统状态数据结构
- 内存监测函数
- CPU 监测函数
- 磁盘监测函数
- 温度监测函数
- 系统状态采集接口

### `edgesentinel.conf`

存放运行配置，例如：

```ini
memory_warning_threshold=80.0
cpu_warning_threshold=80.0
disk_warning_threshold=85.0
temperature_warning_threshold=75.0
monitor_interval_seconds=5
```

配置文件解析功能将在后续版本中实现。

---

## 开发环境

当前开发环境：

- 操作系统：Ubuntu Linux
- 编程语言：C
- 编译器：GCC
- 构建工具：GNU Make
- 调试工具：GDB
- 版本管理：Git
- 代码托管：GitHub

建议安装以下工具：

```bash
sudo apt update
sudo apt install build-essential gdb git make
```

检查工具版本：

```bash
gcc --version
make --version
gdb --version
git --version
```

---

## 编译项目

进入项目根目录：

```bash
cd ~/EdgeSentinel-Linux
```

执行编译：

```bash
make
```

编译成功后，项目根目录将生成：

```text
edgesentinel
```

---

## 运行项目

```bash
./edgesentinel
```

预期输出形式：

```text
EdgeSentinel-Linux v0.1.0
--------------------------------
Total memory:     7834.50 MB
Available memory: 3921.32 MB
Memory usage:     49.95%
System status:    NORMAL
```

实际输出会随着系统当前状态发生变化。

---

## 清理编译文件

执行：

```bash
make clean
```

该命令将删除：

- `build/` 中的目标文件
- 项目生成的可执行程序

源代码、配置文件和项目文档不会被删除。

---

## 手动编译方式

在 Makefile 尚未完成时，可以手动编译。

创建构建目录：

```bash
mkdir -p build
```

编译 `main.c`：

```bash
gcc -Wall -Wextra -g -Iinclude \
    -c src/main.c \
    -o build/main.o
```

编译 `system_monitor.c`：

```bash
gcc -Wall -Wextra -g -Iinclude \
    -c src/system_monitor.c \
    -o build/system_monitor.o
```

链接目标文件：

```bash
gcc build/main.o build/system_monitor.o \
    -o edgesentinel
```

运行：

```bash
./edgesentinel
```

---

## Linux 系统信息来源

项目将主要通过 Linux 虚拟文件系统获取系统状态。

### 内存信息

```bash
cat /proc/meminfo
```

主要读取：

```text
MemTotal
MemAvailable
```

内存使用率计算方式：

```text
memory_usage =
    (MemTotal - MemAvailable) / MemTotal × 100%
```

### CPU 信息

```bash
cat /proc/stat
```

### 系统负载

```bash
cat /proc/loadavg
```

### 系统运行时间

```bash
cat /proc/uptime
```

### 温度信息

不同设备的温度文件路径可能不同，常见位置包括：

```text
/sys/class/thermal/thermal_zone0/temp
```

---

## 版本规划

### v0.1：基础系统监测

- [ ] 读取内存信息
- [ ] 计算内存使用率
- [ ] 根据阈值判断内存状态
- [ ] 完成基础 Makefile
- [ ] 完善错误处理

### v0.2：资源监测扩展

- [ ] CPU 使用率监测
- [ ] 系统负载监测
- [ ] 磁盘使用率监测
- [ ] 系统运行时间监测
- [ ] 温度监测

### v0.3：配置与日志系统

- [ ] 配置文件解析
- [ ] 日志等级
- [ ] 日志文件写入
- [ ] 日志时间戳
- [ ] 日志文件管理

### v0.4：持续运行与并发

- [ ] 定时采集
- [ ] 多线程监测
- [ ] 互斥锁
- [ ] 信号处理
- [ ] 安全退出
- [ ] 守护进程

### v0.5：网络通信

- [ ] TCP 通信
- [ ] HTTP 状态上报
- [ ] JSON 数据格式
- [ ] 断线重连
- [ ] 远程状态查询

### v1.0：嵌入式 Linux 版本

- [ ] ARM 开发板移植
- [ ] 交叉编译
- [ ] GPIO 控制
- [ ] 环境传感器接入
- [ ] 蜂鸣器告警
- [ ] 摄像头接入

### v2.0：边缘 AI 版本

- [ ] 轻量级目标检测
- [ ] 本地模型推理
- [ ] 传感器与视觉信息融合
- [ ] 智能异常判断
- [ ] 本地与远程联合告警

---

## Git 提交流程

查看当前修改：

```bash
git status
```

添加修改：

```bash
git add .
```

提交修改：

```bash
git commit -m "feat: add memory monitoring"
```

推送到 GitHub：

```bash
git push
```

建议使用以下提交类型：

| 类型 | 用途 |
|---|---|
| `feat` | 新增功能 |
| `fix` | 修复问题 |
| `docs` | 修改文档 |
| `refactor` | 重构代码 |
| `test` | 添加或修改测试 |
| `build` | 修改构建配置 |
| `chore` | 项目维护工作 |

示例：

```bash
git commit -m "feat: read memory information from proc"
git commit -m "fix: handle meminfo open failure"
git commit -m "build: add Makefile"
git commit -m "docs: update project roadmap"
```

---

## 学习目标

通过本项目计划掌握：

- C 语言工程化开发
- 指针、结构体和文件操作
- 多文件项目组织
- GCC 编译与链接
- Makefile 自动构建
- GDB 程序调试
- Linux 文件系统
- Linux 系统调用
- 进程与信号
- 多线程和线程同步
- Socket 网络编程
- 嵌入式 Linux 开发
- 交叉编译与程序移植
- 传感器和摄像头接入
- 边缘 AI 模型部署

---

## 开发原则

项目开发遵循以下原则：

1. 每次只增加一个可以测试的功能。
2. 每个模块拥有明确职责。
3. 头文件用于声明公共接口。
4. 源文件用于实现具体功能。
5. 所有错误都需要明确处理。
6. 编译时启用警告选项。
7. 编译产物不提交到 GitHub。
8. 每完成一个功能进行一次 Git 提交。
9. 文档与代码同步更新。
10. 优先保证程序正确，再逐步优化性能。

---

## License

本项目当前用于个人学习、工程实践和求职展示。

后续如需公开发布或允许他人使用，将补充正式的开源许可证。
