# 芯片资料整理文档

## 目录结构
```
Docs/
├── chip_support_progress.md    # 工作进度记录
├── chip_reference_manuals/     # 芯片参考手册
│   ├── ST/                     # STMicroelectronics
│   ├── NXP/                    # NXP
│   ├── Renesas/                # Renesas
│   ├── TI/                     # Texas Instruments
│   ├── Infineon/               # Infineon
│   └── Domestic/               # 国产芯片
└── chip_database.md            # 本文档
```

---

## 一、STMicroelectronics (ST)

### 1.1 STM32F0系列 (Cortex-M0)
| 型号 | Flash | RAM | 封装 | 特性 |
|------|-------|-----|------|------|
| STM32F030C8 | 64KB | 8KB | LQFP48 | 基础款 |
| STM32F030CC | 256KB | 32KB | LQFP48 | 大容量 |
| STM32F070CB | 128KB | 16KB | LQFP48 | USB |

**调试接口**: SWD
**Flash扇区**: 1KB
**参考手册**: RM0091

### 1.2 STM32F1系列 (Cortex-M3)
| 型号 | Flash | RAM | 封装 | 特性 |
|------|-------|-----|------|------|
| STM32F103C8 | 64KB | 20KB | LQFP48 | 经典款 |
| STM32F103ZE | 512KB | 64KB | LQFP144 | 大容量 |

**调试接口**: SWD/JTAG
**Flash扇区**: 1KB/2KB/4KB(大容量)
**参考手册**: RM0008

### 1.3 STM32F4系列 (Cortex-M4F)
| 型号 | Flash | RAM | 封装 | 特性 |
|------|-------|-----|------|------|
| STM32F407VGT6 | 1MB | 192KB | LQFP100 | 以太网+USB OTG |
| STM32F429ZIT6 | 2MB | 256KB | LQFP144 | LCD+SDRAM |

**调试接口**: SWD/JTAG
**Flash扇区**: 16KB/64KB/128KB
**参考手册**: RM0090

### 1.4 STM32H7系列 (Cortex-M7)
| 型号 | Flash | RAM | 封装 | 特性 |
|------|-------|-----|------|------|
| STM32H743VIT6 | 2MB | 1MB | LQFP100 | 双核高性能 |
| STM32H750VBT6 | 128KB | 1MB | LQFP100 | Bootloader |

**调试接口**: SWD/JTAG
**Flash扇区**: 128KB
**参考手册**: RM0433

---

## 二、NXP (含摩托罗拉)

### 2.1 HCS12/S12X系列 (16位)
**架构**: HCS12 CPU
**调试接口**: BDM (Background Debug Mode)

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| MC9S12A | MC9S12A256 | 256KB | 12KB | 通用型 |
| MC9S12D | MC9S12D512 | 512KB | 14KB | CAN |
| MC9S12X | MC9S12XEP100 | 1MB | 64KB | XGATE协处理器 |

**注意事项**:
- BDM调试需要专用调试器
- Flash编程需要解锁

### 2.2 HCS08系列 (8位)
**架构**: HCS08 CPU
**调试接口**: BDM

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| MC9S08AW | MC9S08AW60 | 60KB | 4KB | 汽车级 |
| MC9S08QE | MC9S08QE128 | 128KB | 8KB | 低功耗 |

### 2.3 HC08/HC05系列 (8位)
**架构**: HC08/HC05 CPU
**调试接口**: MON8 (Monitor Mode)

**注意事项**:
- 部分老型号需要MON8调试模式
- OTP版本不可重复编程

### 2.4 MPC/SPC系列 (Power Architecture)
**架构**: PowerPC e200/e300
**调试接口**: JTAG/Nexus

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| MPC560x | MPC5607B | 512KB | 96KB | 车规 |
| SPC574x | SPC574K80 | 4MB | 512KB | 多核安全 |

**注意事项**:
- 需要专用调试器(如Lauterbach)
- Flash编程需要特殊序列

---

## 三、Renesas (瑞萨)

### 3.1 78K系列 (8位/16位)
**架构**: 78K0/78K0R CPU
**调试接口**: UART/Fine

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| 78K0 | uPD78F0024 | 32KB | 1KB | 8位 |
| 78K0R | uPD78F1168 | 256KB | 16KB | 16位 |

### 3.2 V850系列 (32位)
**架构**: V850E2 CPU
**调试接口**: JTAG/Fine

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| V850ES | uPD70F3216 | 256KB | 16KB | 中端 |
| V850E2 | uPD70F3538 | 2MB | 256KB | 高端 |

### 3.3 RH850系列 (32位车规)
**架构**: RH850 CPU
**调试接口**: JTAG/Auriga

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| RH850/F1L | RH850F1L400 | 512KB | 64KB | 单核 |
| RH850/E2x | RH850E2M4 | 4MB | 512KB | 多核 |

**注意事项**:
- 车规安全等级ASIL-B/D
- 需要专用调试器

### 3.4 R8C/M16C/M32C系列 (16位/32位)
**架构**: M16C/M32C CPU
**调试接口**: JTAG/Fine

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| R8C | R8C/27 | 64KB | 8KB | 16位精简 |
| M16C | M16C/62N | 512KB | 31KB | 16位 |
| M32C | M32C/89 | 2MB | 48KB | 32位 |

---

## 四、Texas Instruments (TI)

### 4.1 MSP430系列 (16位超低功耗)
**架构**: MSP430 CPU
**调试接口**: SBW (Spy-Bi-Wire) / JTAG

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| MSP430x2xx | MSP430F249 | 60KB | 2KB | 经典款 |
| MSP430x5xx | MSP430F5529 | 128KB | 8KB | USB |
| MSP430FR | MSP430FR5994 | 128KB(FRAM) | 4KB | FRAM |

**注意事项**:
- SBW为两线调试接口
- FRAM版本使用铁电存储器

### 4.2 MSP432系列 (32位Cortex-M4F)
**架构**: Cortex-M4F
**调试接口**: SWD/JTAG

| 型号 | Flash | RAM | 特性 |
|------|-------|-----|------|
| MSP432P401R | 256KB | 64KB | 基础款 |
| MSP432P411R | 512KB | 128KB | 大容量 |

### 4.3 CC2530/CC26xx系列 (无线SoC)
**架构**: 8051 (CC2530) / Cortex-M3 (CC26xx)
**调试接口**: JTAG/Serial

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| CC2530 | CC2530F256 | 256KB | 8KB | ZigBee |
| CC2652 | CC2652R1 | 352KB | 80KB | BLE5.0 |

### 4.4 TMS320 C2000系列 (DSP)
**架构**: C28x CPU
**调试接口**: JTAG

| 型号 | Flash | RAM | 特性 |
|------|-------|-----|------|
| TMS320F28335 | 256KB | 68KB | 浮点 |
| TMS320F28379D | 512KB | 200KB | 双核 |

### 4.5 Hercules系列 (车规安全MCU)
**架构**: Cortex-R4F/R5F
**调试接口**: JTAG

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| TMS570 | TMS570LS3137 | 4MB | 512KB | ARM R4 |
| RM4 | RM48 | 2MB | 256KB | 安全MCU |

---

## 五、Infineon (英飞凌)

### 5.1 AURIX TC2xx/TC3xx系列 (多核车规)
**架构**: TriCore
**调试接口**: JTAG/DAP

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| TC2xx | TC27x | 4MB | 592KB | 三核 |
| TC3xx | TC39x | 16MB | 4MB | 六核 |
| TC4xx | TC4xx | 16MB | 4MB | 新一代 |

**注意事项**:
- 多核调试需要专用调试器
- Flash编程需要特殊工具(Aurix Flash Tool)

---

## 六、国产芯片

### 6.1 国民技术 N32系列
**架构**: Cortex-M4F (兼容STM32)
**调试接口**: SWD

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| N32G | N32G457 | 512KB | 144KB | 通用型 |
| N32L | N32L48x | 512KB | 256KB | 低功耗 |

### 6.2 华大 HC32系列
**架构**: Cortex-M0+/M4F (兼容STM32)
**调试接口**: SWD

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| HC32F | HC32F460 | 512KB | 192KB | 通用型 |
| HC32L | HC32L196 | 256KB | 64KB | 低功耗 |

### 6.3 航顺 HS系列
**架构**: Cortex-M0/M3 (兼容STM32)
**调试接口**: SWD

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| HS32 | HS32F3380 | 512KB | 128KB | 通用型 |

### 6.4 芯恒微 XH系列
**架构**: Cortex-M3/M4 (兼容STM32)
**调试接口**: SWD

| 系列 | 型号示例 | Flash | RAM | 特性 |
|------|----------|-------|-----|------|
| XH32 | XH32F403 | 512KB | 192KB | 通用型 |

---

## 七、调试接口汇总

### 7.1 接口类型汇总表
| 芯片系列 | 调试接口 | 调试器推荐 | 软件模拟实现 |
|----------|----------|------------|--------------|
| STM32 | SWD/JTAG | ST-Link, J-Link | ✔️ 已实现 |
| GD32 | SWD/JTAG | J-Link, DAP-Link | ✔️ 已实现 |
| HCS12/S12X | BDM | Multilink, BDM Pod | ✔️ 已实现 (bdm.c) |
| HCS08 | BDM | Multilink | ✔️ 已实现 (bdm.c) |
| HC08/HC05 | MON8 | 串行监控 | ✔️ 已实现 (mon8.c) |
| MPC/SPC | JTAG/Nexus | Lauterbach, PE_Multilink | - |
| 78K | UART/Fine | E1, E2 | ✔️ 已实现 (fine.c) |
| V850 | JTAG/Fine | E1, E2 | ✔️ 已实现 (fine.c) |
| RH850 | JTAG/Auriga | E1, E2, Lauterbach | - |
| MSP430 | SBW/JTAG | MSP-FET | ✔️ 已实现 (sbw.c) |
| MSP432 | SWD/JTAG | XDS110 | ✔️ 已实现 |
| CC26xx | JTAG/Serial | XDS110 | - |
| TMS320C2000 | JTAG | XDS100, XDS200 | - |
| Hercules | JTAG | XDS100, XDS200 | - |
| AURIX | JTAG/DAP | Lauterbach, DAS | - |
| N32/HC32/HS/XH | SWD | J-Link, DAP-Link | ✔️ 已实现 |

### 7.2 软件模拟调试接口详情

#### SBW (Spy-Bi-Wire) - MSP430专用
**接口描述**: TI两线调试接口，减少引脚占用
**文件**: `sbw.h`, `sbw.c`
**引脚**:
- TCK/SBWTCK: 时钟线
- TMS/SBWIO: 双向数据线
- RESET: 复位线
- TEST: 测试模式选择(可选)

**功能**:
- JTAG指令集支持
- 内存读写
- Flash编程
- 芯片ID读取
- 时钟频率可配置(100KHz~1MHz)

#### BDM (Background Debug Mode) - NXP/Freescale专用
**接口描述**: NXP背景调试模式，单线/双线接口
**文件**: `bdm.h`, `bdm.c` (已优化)
**引脚**:
- BKGD: 双向数据线
- RESET: 复位线

**功能**:
- 后台调试模式
- 内存读写
- Flash编程
- 断点支持

#### MON8 - HC08/HC05专用
**接口描述**: Freescale监控模式，用于老型号8位MCU
**文件**: `mon8.h`, `mon8.c`
**引脚**:
- BKGD: 双向数据线
- RESET: 复位线
- PTX: 发送(可选)
- PRX: 接收(可选)

**功能**:
- MON8监控模式
- Flash读写、擦除
- 芯片加密
- 版本读取

#### FINE - Renesas瑞萨专用
**接口描述**: Renesas Flash编程接口，多线高速编程
**文件**: `fine.h`, `fine.c`
**引脚**:
- FLMD0: 数据
- FLMD1: 模式选择
- FLMD2: 模式选择
- FLMD3: 模式选择
- FLCLK: 时钟
- RESET: 复位

**功能**:
- FINE编程模式
- Flash高速读写
- 扇区/芯片擦除
- 校验功能
- 芯片ID读取

### 7.3 软件模拟实现说明

**通用特性**:
- 基于GPIO软件模拟时序
- 可配置时钟频率(100KHz~1MHz)
- 完整的复位、进入/退出调试模式
- 内存读写、Flash擦除、芯片ID读取
- 支持不同引脚配置
- 详细的代码注释

**依赖文件**:
- `gpio_soft.h/c`: GPIO软件抽象层
- `tim.h`: 定时器延时支持

---

## 八、Flash编程注意事项

### 8.1 通用注意事项
1. **解锁序列**: 大多数芯片需要先解锁Flash才能编程
2. **擦除要求**: Flash只能从1写到0，需要先擦除
3. **扇区对齐**: 擦除通常按扇区进行
4. **写入粒度**: 注意半字/字/双字对齐要求

### 8.2 特殊注意事项
| 芯片系列 | 特殊要求 |
|----------|----------|
| STM32F4/F7/H7 | 大扇区结构，需注意扇区边界 |
| HCS12 | 需要通过BDM命令编程 |
| MSP430FR | FRAM不需要擦除，可直接写入 |
| AURIX | 需要使用专用Flash工具 |
| RH850 | 需要解锁安全保护 |

---

## 九、参考资料链接

### STMicroelectronics
- STM32参考手册: https://www.st.com/zh/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html
- ST-Link调试器: https://www.st.com/zh/development-tools/st-link-v2.html

### NXP
- S32 Design Studio: https://www.nxp.com/design/software/development-tools/s32-design-studio-for-s32-platform:S32-DESIGN-STUDIO-IDE
- CodeWarrior: https://www.nxp.com/design/software/development-tools/codewarrior-development-tools:CODEWARRIOR-DEV-TOOLS

### Renesas
- e2 studio: https://www.renesas.com/us/en/tools-software/e2studio.html
- Flash编程器: https://www.renesas.com/us/en/tools-software/tools/flash-programmers

### TI
- MSP430工具: https://www.ti.com/tool/MSP-FET
- C2000工具: https://www.ti.com/tool/UNIFLASH

### Infineon
- AURIX工具: https://www.infineon.com/cms/en/product/microcontroller/32-bit-tricore-microcontroller/aurix-32-bit-tricore-microcontroller/
