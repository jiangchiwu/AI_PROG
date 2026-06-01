# NXP S32K系列芯片详细列表

## 1. S32K系列概述

NXP S32K系列是面向汽车和工业应用的可扩展、超低功耗微控制器系列，基于ARM Cortex-M内核，支持SWD/JTAG调试接口。

### 1.1 主要特性

- **内核**：ARM Cortex-M4/M7 (可选FPU)
- **频率**：最高120MHz (S32K1), 160MHz (S32K3)
- **Flash**：32KB - 2MB
- **RAM**：8KB - 384KB
- **工作温度**：-40°C to 125°C (汽车级)
- **调试接口**：SWD, JTAG, cJTAG
- **封装**：LQFP, BGA, QFN

## 2. S32K1系列 (Cortex-M4F)

### 2.1 S32K116

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M4F |
| 频率 | 48MHz |
| Flash | 128KB |
| RAM | 24KB |
| 封装 | 32/48/64-pin LQFP |
| ADC | 12-bit, 16ch |
| 通信 | 2x UART, 1x SPI, 1x I2C |
| 定时器 | 1x FlexTimer (8ch), 2x LPIT |
| 安全 | 无 |
| 状态 | ⬜ |

### 2.2 S32K118

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M4F |
| 频率 | 48MHz |
| Flash | 256KB |
| RAM | 32KB |
| 封装 | 48/64-pin LQFP |
| ADC | 12-bit, 24ch |
| 通信 | 4x UART, 1x SPI, 1x I2C |
| 定时器 | 1x FlexTimer (8ch), 2x LPIT |
| 安全 | 无 |
| 状态 | ⬜ |

### 2.3 S32K142

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M4F |
| 频率 | 80MHz |
| Flash | 256KB |
| RAM | 32KB |
| 封装 | 32/48/64-pin LQFP |
| ADC | 12-bit, 16ch |
| 通信 | 3x UART, 2x SPI, 2x I2C |
| 定时器 | 2x FlexTimer (16ch), 4x LPIT |
| 安全 | 无 |
| 状态 | ⬜ |

### 2.4 S32K144

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M4F |
| 频率 | 80MHz / 112MHz (overdrive) |
| Flash | 512KB |
| RAM | 64KB |
| 封装 | 64/100-pin LQFP |
| ADC | 12-bit, 24ch |
| 通信 | 4x UART, 3x SPI, 3x I2C, CAN |
| 定时器 | 2x FlexTimer (16ch), 4x LPIT |
| 安全 | 可选 |
| 状态 | ⬜ |

### 2.5 S32K146

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M4F |
| 频率 | 80MHz / 112MHz (overdrive) |
| Flash | 1MB |
| RAM | 128KB |
| 封装 | 100/144-pin LQFP |
| ADC | 12-bit, 32ch |
| 通信 | 6x UART, 4x SPI, 4x I2C, 2x CAN |
| 定时器 | 3x FlexTimer (24ch), 4x LPIT |
| 安全 | 可选 |
| 状态 | ⬜ |

### 2.6 S32K148

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M4F |
| 频率 | 80MHz / 112MHz (overdrive) |
| Flash | 1.5MB |
| RAM | 192KB |
| 封装 | 100/144-pin LQFP |
| ADC | 12-bit, 32ch |
| 通信 | 8x UART, 6x SPI, 4x I2C, 3x CAN, Ethernet |
| 定时器 | 4x FlexTimer (32ch), 4x LPIT |
| 安全 | 可选 |
| 状态 | ⬜ |

## 3. S32K3系列 (Cortex-M7)

### 3.1 S32K312

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M7 (单核) |
| 频率 | 160MHz |
| Flash | 2MB (Dual Bank) |
| RAM | 384KB |
| 封装 | 100/144/176-pin |
| ADC | 12-bit, 48ch |
| 通信 | 8x UART, 6x SPI, 4x I2C, 5x CAN FD |
| 定时器 | FlexPWM, eTimer |
| 安全 | HSM |
| 状态 | ⬜ |

### 3.2 S32K322

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M7 (单核) |
| 频率 | 160MHz |
| Flash | 2MB (Dual Bank) |
| RAM | 512KB |
| 封装 | 144/176/256-pin |
| ADC | 12-bit, 48ch |
| 通信 | 12x UART, 8x SPI, 6x I2C, 8x CAN FD, Ethernet |
| 定时器 | FlexPWM, eTimer |
| 安全 | HSM, CSEc |
| 状态 | ⬜ |

### 3.3 S32K324

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M7 (单核) |
| 频率 | 160MHz |
| Flash | 2MB (Dual Bank) |
| RAM | 384KB |
| 封装 | 100/144-pin |
| ADC | 12-bit, 32ch |
| 通信 | 6x UART, 4x SPI, 3x I2C, 3x CAN FD |
| 定时器 | FlexPWM, eTimer |
| 安全 | HSM |
| 状态 | ⬜ |

### 3.4 S32K342

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M7 (单核) |
| 频率 | 160MHz |
| Flash | 2MB (Dual Bank) |
| RAM | 512KB |
| 封装 | 176-pin |
| ADC | 12-bit, 48ch |
| 通信 | 8x UART, 6x SPI, 4x I2C, 5x CAN FD |
| 定时器 | FlexPWM, eTimer |
| 安全 | HSM, CSEc |
| 状态 | ⬜ |

### 3.5 S32K344

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M7 (单核) |
| 频率 | 160MHz |
| Flash | 2MB (Dual Bank) |
| RAM | 384KB |
| 封装 | 100/144-pin |
| ADC | 12-bit, 32ch |
| 通信 | 6x UART, 4x SPI, 3x I2C, 3x CAN FD |
| 定时器 | FlexPWM, eTimer |
| 安全 | Secure Boot, CSEc |
| 状态 | ⬜ |

### 3.6 S32K346

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M7 (单核) |
| 频率 | 160MHz |
| Flash | 2MB (Dual Bank) |
| RAM | 512KB |
| 封装 | 176-pin |
| ADC | 12-bit, 48ch |
| 通信 | 8x UART, 6x SPI, 4x I2C, 5x CAN FD |
| 定时器 | FlexPWM, eTimer |
| 安全 | Secure Boot, CSEc |
| 状态 | ⬜ |

### 3.7 S32K348

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M7 (单核) |
| 频率 | 160MHz |
| Flash | 2MB (Dual Bank) |
| RAM | 512KB + 128KB TCM |
| 封装 | 176/256-pin |
| ADC | 12-bit, 64ch |
| 通信 | 12x UART, 8x SPI, 6x I2C, 8x CAN FD, Ethernet |
| 定时器 | FlexPWM, eTimer |
| 安全 | Secure Boot, CSEc, HSM |
| 状态 | ⬜ |

### 3.8 S32K366

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M7 (双核) |
| 频率 | 160MHz (每个核心) |
| Flash | 2x 2MB (Dual Bank) |
| RAM | 1MB |
| 封装 | 256-pin BGA |
| ADC | 12-bit, 64ch |
| 通信 | 16x UART, 10x SPI, 8x I2C, 12x CAN FD, 2x Ethernet |
| 定时器 | FlexPWM, eTimer |
| 安全 | Secure Boot, CSEc, HSM |
| 状态 | ⬜ |

### 3.9 S32K388

| 参数 | 值 |
|------|-----|
| 内核 | Cortex-M7 (双核) + Cortex-M4 |
| 频率 | 160MHz / 80MHz |
| Flash | 2x 2MB + 512KB |
| RAM | 1.5MB |
| 封装 | 256-pin BGA |
| ADC | 12-bit, 64ch |
| 通信 | 20x UART, 12x SPI, 10x I2C, 16x CAN FD, 2x Ethernet |
| 定时器 | FlexPWM, eTimer |
| 安全 | Secure Boot, CSEc, HSM |
| 状态 | ⬜ |

## 4. S32K1系列详细参数对比

| 型号 | Flash | RAM | CPU频率 | CAN | ADC通道 | 封装 | 状态 |
|------|-------|-----|--------|-----|---------|------|------|
| S32K116 | 128KB | 24KB | 48MHz | 0 | 16 | 32/48/64 | ⬜ |
| S32K118 | 256KB | 32KB | 48MHz | 0 | 24 | 48/64 | ⬜ |
| S32K142 | 256KB | 32KB | 80MHz | 0 | 16 | 32/48/64 | ⬜ |
| S32K144 | 512KB | 64KB | 112MHz | 1 | 24 | 64/100 | ⬜ |
| S32K146 | 1MB | 128KB | 112MHz | 2 | 32 | 100/144 | ⬜ |
| S32K148 | 1.5MB | 192KB | 112MHz | 3 | 32 | 100/144 | ⬜ |

## 5. S32K3系列详细参数对比

| 型号 | Flash | RAM | CPU频率 | CAN FD | ADC通道 | 封装 | 状态 |
|------|-------|-----|--------|--------|---------|------|------|
| S32K312 | 2MB | 384KB | 160MHz | 5 | 48 | 100/144/176 | ⬜ |
| S32K322 | 2MB | 512KB | 160MHz | 8 | 48 | 144/176/256 | ⬜ |
| S32K324 | 2MB | 384KB | 160MHz | 3 | 32 | 100/144 | ⬜ |
| S32K342 | 2MB | 512KB | 160MHz | 5 | 48 | 176 | ⬜ |
| S32K344 | 2MB | 384KB | 160MHz | 3 | 32 | 100/144 | ⬜ |
| S32K346 | 2MB | 512KB | 160MHz | 5 | 48 | 176 | ⬜ |
| S32K348 | 2MB | 512KB+128KB | 160MHz | 8 | 64 | 176/256 | ⬜ |
| S32K366 | 4MB | 1MB | 160MHz | 12 | 64 | 256 BGA | ⬜ |
| S32K388 | 4.5MB | 1.5MB | 160MHz | 16 | 64 | 256 BGA | ⬜ |

## 6. 调试接口

### 6.1 支持的调试接口

| 接口 | S32K1 | S32K3 | 说明 |
|------|-------|-------|------|
| SWD | ✅ | ✅ | Serial Wire Debug |
| JTAG | ✅ | ✅ | IEEE 1149.1 |
| cJTAG | ✅ | ✅ | Compact JTAG |

### 6.2 调试接口引脚

```
SWD接口：
├── SWDIO  - 双向数据线
├── SWCLK  - 时钟线
├── SWO    - 跟踪输出 (可选)
└── RESET  - 系统复位 (可选)

JTAG接口：
├── TMS    - 测试模式选择
├── TCK    - 测试时钟
├── TDI    - 测试数据输入
├── TDO    - 测试数据输出
└── TRST   - 测试复位 (可选)
```

### 6.3 调试连接

```c
// SWD连接配置
#define SWD_SWDIO_PORT  GPIO_PORT_A
#define SWD_SWDIO_PIN   GPIO_PIN_13
#define SWD_SWCLK_PORT  GPIO_PORT_A
#define SWD_SWCLK_PIN   GPIO_PIN_14
#define SWD_RESET_PORT  GPIO_PORT_A
#define SWD_RESET_PIN   GPIO_PIN_15

// JTAG连接配置
#define JTAG_TMS_PORT   GPIO_PORT_A
#define JTAG_TMS_PIN    GPIO_PIN_13
#define JTAG_TCK_PORT   GPIO_PORT_A
#define JTAG_TCK_PIN    GPIO_PIN_14
#define JTAG_TDI_PORT   GPIO_PORT_A
#define JTAG_TDI_PIN    GPIO_PIN_15
#define JTAG_TDO_PORT   GPIO_PORT_A
#define JTAG_TDO_PIN    GPIO_PIN_17
```

## 7. Flash组织结构

### 7.1 S32K1系列Flash

```
Flash布局 (以S32K146为例):
┌────────────────┐ 0x00000000
│   Flash Block 0 │
│    (512KB)     │
├────────────────┤ 0x00080000
│   Flash Block 1 │
│    (512KB)     │
├────────────────┤ 0x00100000
│                │
│   (预留空间)    │
│                │
├────────────────┤ 0x00400000
│   FlexRAM      │
│    (64KB)      │
├────────────────┤ 0x00410000
│  Flash Config   │
│    (32B)       │
├────────────────┤ 0x00410020
│    Flash IFR   │
│    (2KB)       │
├────────────────┤ 0x00410820
│   Version ID   │
│    (256B)     │
├────────────────┤ 0x00411000
│                │
│   (Reserved)   │
│                │
└────────────────┘ 0x00420000
```

### 7.2 S32K3系列Flash

```
Flash布局 (以S32K344为例):
┌────────────────┐ 0x00000000
│   Flash Block 0 │
│    (1MB)       │
├────────────────┤ 0x00100000
│   Flash Block 1 │
│    (1MB)       │
├────────────────┤ 0x00400000
│   FlexRAM      │
│    (384KB)    │
├────────────────┤ 0x00460000
│  Flash Config   │
│    (32B)       │
├────────────────┤ 0x00460020
│   Flash IFR   │
│    (32KB)      │
├────────────────┤ 0x00468000
│                │
│   (Reserved)   │
│                │
└────────────────┘ 0x00470000
```

## 8. 安全特性

### 8.1 S32K1安全功能

| 功能 | S32K116 | S32K118 | S32K142 | S32K144 | S32K146 | S32K148 |
|------|---------|---------|---------|---------|---------|---------|
| CRC | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Watchdog | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 低功耗模式 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 唯一ID | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 密码保护 | - | - | - | ✅ | ✅ | ✅ |

### 8.2 S32K3安全功能

| 功能 | S32K312 | S32K322 | S32K324 | S32K344 | S32K346 | S32K348 | S32K366 | S32K388 |
|------|---------|---------|---------|---------|---------|---------|---------|---------|
| CSEc | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Secure Boot | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| HSM | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 密码保护 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 唯一ID | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| DICE | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

## 9. 开发资源

### 9.1 官方SDK

- **S32K1 SDK**: S32K1XX_SDK
- **S32K3 SDK**: S32K3XX_SDK
- **IDE**: S32 Design Studio, IAR, Keil, GCC

### 9.2 开发板

| 开发板 | 芯片 | 特点 |
|-------|------|------|
| S32K116EVB | S32K116 | 入门级评估 |
| S32K118EVB | S32K118 | 入门级评估 |
| S32K142EVB | S32K142 | 标准评估 |
| S32K144EVB | S32K144 | 标准评估 |
| S32K146Q100 | S32K146 | 100-pin评估 |
| S32K148EVB | S32K148 | 高级评估 |
| S32K344EVB | S32K344 | 高级评估 |
| S32K3XXEVB | S32K3XX | 多型号兼容 |

## 10. 芯片选型指南

### 10.1 按应用场景

| 应用场景 | 推荐芯片 | 原因 |
|---------|---------|------|
| 简单GPIO控制 | S32K116/118 | 成本低，功能足够 |
| CAN通信 | S32K144/146 | 集成CAN控制器 |
| 电机控制 | S32K146/148 | 多个PWM通道 |
| 车身电子 | S32K3xx | 高可靠性，安全特性 |
| 网关 | S32K366/388 | 多CAN FD，多核 |
| 安全应用 | S32K3xx | HSM, Secure Boot |

### 10.2 按资源需求

| 资源需求 | 推荐芯片 |
|---------|---------|
| Flash < 256KB | S32K116/118/142 |
| Flash 512KB-1MB | S32K144/146/148 |
| Flash > 1MB | S32K3xx |
| RAM < 64KB | S32K116/118/142 |
| RAM 64KB-256KB | S32K144/146/148 |
| RAM > 256KB | S32K3xx |

## 11. 采购信息

### 11.1 常见封装

| 封装 | 引脚数 | 间距 | 适用型号 |
|------|--------|------|----------|
| LQFP | 32 | 0.8mm | S32K116 |
| LQFP | 48 | 0.5mm | S32K116/118/142 |
| LQFP | 64 | 0.5mm | S32K118/142/144 |
| LQFP | 100 | 0.5mm | S32K144/146/148 |
| LQFP | 144 | 0.5mm | S32K146/148 |
| BGA | 256 | 0.8mm | S32K3xx高端 |
| MAPBGA | 257 | 0.65mm | S32K3xx |

### 11.2 温度等级

| 温度等级 | 温度范围 | 适用场景 |
|---------|---------|---------|
| V | -40°C to 105°C | 消费级 |
| Q | -40°C to 125°C | 汽车级 |
| M | -40°C to 125°C | 汽车级, AEC-Q100 |

## 12. 状态追踪

| 芯片 | 优先级 | 驱动状态 | 测试状态 | 备注 |
|------|--------|---------|---------|------|
| S32K116 | P1 | ⬜ | ⬜ | 入门级 |
| S32K118 | P1 | ⬜ | ⬜ | 入门级 |
| S32K142 | P1 | ⬜ | ⬜ | 基础级 |
| S32K144 | P1 | ⬜ | ⬜ | 标准级 |
| S32K146 | P1 | ⬜ | ⬜ | 增强级 |
| S32K148 | P1 | ⬜ | ⬜ | 增强级 |
| S32K312 | P2 | ⬜ | ⬜ | 入门级 |
| S32K322 | P2 | ⬜ | ⬜ | 标准级 |
| S32K324 | P2 | ⬜ | ⬜ | 基础级 |
| S32K342 | P2 | ⬜ | ⬜ | 标准级 |
| S32K344 | P1 | ⬜ | ⬜ | 热门型号 |
| S32K346 | P2 | ⬜ | ⬜ | 标准级 |
| S32K348 | P2 | ⬜ | ⬜ | 增强级 |
| S32K366 | P3 | ⬜ | ⬜ | 高端 |
| S32K388 | P3 | ⬜ | ⬜ | 高端 |

## 13. 文档版本

- 版本: v1.0
- 创建日期: 2026-06-01
- 最后更新: 2026-06-01
- 状态: 草稿
