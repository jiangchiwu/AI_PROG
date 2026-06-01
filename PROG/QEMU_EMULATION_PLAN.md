# QEMU 模拟器移植规划

## 概述

本文档描述如何使用 QEMU 模拟待编程芯片，为多功能编程器提供硬件测试支持

## 目标

- 提供无实际硬件情况下的协议测试
- 支持多种目标芯片模拟
- 验证编程器协议实现正确性

## 支持的 QEMU 架构

### ARM Cortex-M 系列
- Cortex-M0/M0+/M3/M4/M7
- Cortex-M33/M35P
- Cortex-M55/M85

### ARM Cortex-R 系列
- Cortex-R4/R5/R7/R8

### ARM Cortex-A 系列
- Cortex-A5/A7/A8/A9/A15
- Cortex-A32/A35/A53/A55/A72/A73/A75/A76/A77/A78/A710
- Cortex-X1/X2

### RISC-V 系列
- RV32/RV64

### NXP S32K 系列
- S32K1xx (Cortex-M4F/M0+)
- S32K3xx (Cortex-M7/M33)

## QEMU 配置

### 1. 下载和安装 QEMU

#### Windows
```powershell
# 使用 Chocolatey
choco install qemu
# 或下载官方安装包
# https://www.qemu.org/download/
```

#### Linux
```bash
sudo apt-get install qemu-system-arm qemu-system-riscv
```

### 2. 启动 QEMU 模拟 Cortex-M4

```bash
# STM32F4 Discovery (Cortex-M4F)
qemu-system-arm -machine stm32f4-discovery -nographic -kernel your_firmware.bin

# 或使用 generic Cortex-M4 机器
qemu-system-arm -machine mps2-an385 -nographic -kernel your_firmware.bin

# 带 GDB 调试支持
qemu-system-arm -machine stm32f4-discovery -nographic -gdb tcp::3333 -S
```

### 3. NXP S32K148 模拟

```bash
# 使用通用 Cortex-M4 作为基础，模拟 S32K148
# 自定义机器配置或使用 NXP 官方评估板
```

## 编程器与 QEMU 通信方案

### 方案 A: GDB 远程调试接口

使用 QEMU GDB 服务器模式提供调试接口

```bash
# 启动 QEMU 并等待 GDB 连接
qemu-system-arm -machine mps2-an385 -nographic -gdb tcp::3333 -S
```

### 方案 B: 虚拟串口通信

通过虚拟串口设备模拟硬件接口
- 虚拟 JTAG/SWD 接口
- 虚拟 UART 通信

### 方案 C: 自定义后端

开发 QEMU 扩展，模拟真实的调试接口

## 测试流程

### 1. 搭建测试环境
- 安装 QEMU
- 准备目标芯片固件
- 配置调试接口

### 2. 协议测试
- SWD 协议验证
- JTAG 协议验证
- 其他接口协议验证

### 3. 芯片驱动测试
- Flash 读写擦测试
- 内存操作测试
- 安全特性测试

## QEMU 常用命令

### 基本命令

```bash
# 列出可用机器
qemu-system-arm -machine help

# 列出可用 CPU
qemu-system-arm -cpu help

# 启动带调试支持
qemu-system-arm -machine mps2-an385 -nographic -gdb tcp::3333 -S

# 监控 QEMU 监视器
qemu-system-arm -machine mps2-an385 -nographic -monitor stdio
```

### GDB 调试命令

```gdb
# 连接到 QEMU
target remote localhost:3333

# 加载符号文件
file your_firmware.elf

# 设置断点
break main

# 继续执行
continue
```

## 自动化测试

### 使用 Python 脚本

```python
import subprocess
import gdb

# 自动启动 QEMU
qemu_process = subprocess.Popen([
    'qemu-system-arm',
    '-machine', 'mps2-an385',
    '-nographic',
    '-gdb', 'tcp::3333',
    '-S',
    '-kernel', 'firmware.bin'
])

# 使用 GDB 连接和编程器进行交互
```

## QEMU 扩展开发

如需模拟特定芯片，可能需要开发 QEMU 设备模拟：

1. 设备模型
2. 内存映射
3. 中断控制器
4. 外设模拟

## 项目结构

```
qemu/
├── machines/
│   ├── s32k148.c
│   └── s32k388.c
├── devices/
│   └── jtag.c
│   └── swd.c
└── scripts/
    └── test.py
```

## 后续步骤

1. [ ] 安装 QEMU
2. [ ] 配置基础测试环境
3. [ ] 开发 SWD/JTAG 模拟
4. [ ] 集成测试脚本
5. [ ] 验证芯片模拟
