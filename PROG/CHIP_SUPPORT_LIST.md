# AI_PROG 编程器芯片支持列表

## 对标产品

本编程器对标以下主流产品：
- **RT809** - SPI Flash/NOR Flash编程器
- **J-Flash** - SEGGER J-Link编程软件
- **RFP6** - Renesas Flash Programmer
- **UniFlash** - TI德州仪器编程软件
- **OpenOCD** - 开源调试工具

---

## 一、SPI Flash/NOR Flash系列（对标RT809）

支持**200+款**SPI Flash芯片，包括：

### 1.1 Winbond W25Qxx系列
| 型号 | 容量 | 封装 | 特性 |
|------|------|------|------|
| W25Q10JV | 128KB | SOP8 | Quad SPI |
| W25Q20JV | 256KB | SOP8 | Quad SPI |
| W25Q40JV | 512KB | SOP8/WSON8 | Quad SPI |
| W25Q80JV | 1MB | SOP8/WSON8 | Quad SPI |
| W25Q16JV | 2MB | SOP16/WSON8 | Quad SPI |
| W25Q32JV | 4MB | SOP16/WSON8 | Quad SPI |
| W25Q64JV | 8MB | SOP16/WSON8 | Quad SPI |
| W25Q128JV | 16MB | SOP16/WSON8 | Quad SPI |
| W25Q256JV | 32MB | SOP16 | Quad SPI, 4字节地址 |
| W25Q512JV | 64MB | SOP16 | Quad SPI, 4字节地址 |

### 1.2 Macronix MX25Lxx系列
| 型号 | 容量 | 封装 | 特性 |
|------|------|------|------|
| MX25L2006E | 256KB | SOP8 | Standard |
| MX25L4006E | 512KB | SOP8 | Standard |
| MX25L8006E | 1MB | SOP8 | Standard |
| MX25L1606E | 2MB | SOP8/WSON8 | Quad SPI |
| MX25L3206E | 4MB | SOP16/WSON8 | Quad SPI |
| MX25L6406E | 8MB | SOP16/WSON8 | Quad SPI |
| MX25L12835F | 16MB | SOP16 | Quad SPI |
| MX25L25635F | 32MB | SOP16 | Quad SPI, 4字节地址 |
| MX25U3235F | 4MB | WSON8 | 1.8V, Quad SPI |
| MX25U6435F | 8MB | WSON8 | 1.8V, Quad SPI |

### 1.3 Micron/Numonyx N25Qxx系列
| 型号 | 容量 | 封装 | 特性 |
|------|------|------|------|
| N25Q016A | 2MB | SOP8 | Quad SPI |
| N25Q032A | 4MB | SOP16 | Quad SPI |
| N25Q064A | 8MB | SOP16 | Quad SPI |
| N25Q128A | 16MB | SOP16 | Quad SPI |
| N25Q256A | 32MB | SOP16 | Quad SPI, 4字节地址 |
| N25Q512A | 64MB | SOP16 | Quad SPI, 4字节地址 |
| MT25QL32 | 4MB | WSON8 | Quad SPI |
| MT25QL64 | 8MB | WSON8 | Quad SPI |
| MT25QL128 | 16MB | SOP16 | Quad SPI |

### 1.4 其他厂商
- **Spansion/Cypress**: S25FL008A, S25FL016A, S25FL032P, S25FL064P, S25FL128P, S25FL256S, S25FS128S
- **ISSI**: IS25LP032, IS25LP064, IS25LP128, IS25LP256, IS25WP032, IS25WP064, IS25WP128
- **Adesto**: AT25SF041, AT25SF081, AT25SF161, AT25QL081, AT25QL161, AT25QL321, AT25QL641
- **Eon**: EN25Q40, EN25Q80, EN25Q16, EN25Q32, EN25Q64, EN25Q128
- **GigaDevice**: GD25Q40C, GD25Q80C, GD25Q16C, GD25Q32C, GD25Q64C, GD25Q128C, GD25Q256C, GD25Q32E, GD25Q64E, GD25Q128E
- **SST**: SST25VF040B, SST25VF080B, SST25VF016B, SST26VF032, SST26VF064, SST26VF128
- **PMC**: Pm25LV040, Pm25LV080, Pm25LV016
- **AMIC**: A25L040, A25L080, A25L016, A25L032, A25LQ80
- **复旦微**: FM25Q04, FM25Q08, FM25Q16, FM25Q32, FM25Q64, FM25Q128
- **XTX**: XT25F32B, XT25F64B, XT25F128B

---

## 二、Renesas瑞萨系列（对标RFP6）

支持**50+款**瑞萨芯片：

### 2.1 RL78系列（16位超低功耗）
| 型号 | Flash | RAM | Data Flash | 调试接口 |
|------|-------|-----|------------|----------|
| R5F100LEA | 256KB | 32KB | 4KB | FINE |
| R5F100LFA | 384KB | 40KB | 4KB | FINE |
| R5F100PCA | 512KB | 48KB | 8KB | FINE |
| R5F100PFA | 512KB | 64KB | 8KB | FINE |
| R5F100MG | 128KB | 24KB | 4KB | FINE |
| R5F100ML | 256KB | 32KB | 4KB | FINE |
| R5F104PFA | 512KB | 64KB | 8KB | FINE |
| R5F104PGA | 384KB | 40KB | 8KB | FINE |
| R5F12AG | 256KB | 48KB | 8KB | FINE |
| R5F12BA | 512KB | 64KB | 8KB | FINE |
| R5F11A | 128KB | 24KB | 4KB | FINE |
| R5F11B | 256KB | 32KB | 8KB | FINE |
| R5F11C | 512KB | 48KB | 8KB | FINE |

### 2.2 RA系列（ARM Cortex-M）
| 型号 | Flash | RAM | Data Flash | 核心 | 调试接口 |
|------|-------|-----|------------|------|----------|
| RA2E1 | 128KB | 32KB | 8KB | Cortex-M23 | SWD |
| RA2L1 | 256KB | 48KB | 8KB | Cortex-M23 | SWD |
| RA2A1 | 256KB | 48KB | 8KB | Cortex-M23 | SWD |
| RA2E2 | 128KB | 32KB | 8KB | Cortex-M23 | SWD |
| RA4M1 | 256KB | 64KB | 8KB | Cortex-M4 | SWD |
| RA4E1 | 128KB | 32KB | 8KB | Cortex-M4 | SWD |
| RA4W1 | 256KB | 64KB | 8KB | Cortex-M4 | SWD |
| RA4M2 | 512KB | 128KB | 8KB | Cortex-M4 | SWD |
| RA4M3 | 1024KB | 256KB | 16KB | Cortex-M4 | SWD |
| RA6M1 | 512KB | 256KB | 8KB | Cortex-M4 | SWD |
| RA6M2 | 1024KB | 384KB | 16KB | Cortex-M4 | SWD |
| RA6M3 | 2048KB | 640KB | 32KB | Cortex-M4F | SWD |
| RA6M4 | 2048KB | 640KB | 32KB | Cortex-M33 | SWD |
| RA6M5 | 4096KB | 1280KB | 32KB | Cortex-M33 | SWD |
| RA6T1 | 2048KB | 640KB | 32KB | Cortex-M4F | SWD |
| RA6T2 | 2048KB | 640KB | 32KB | Cortex-M33 | SWD |

### 2.3 RH850系列（32位车用）
| 型号 | Flash | RAM | Data Flash | 调试接口 |
|------|-------|-----|------------|----------|
| R7F701002 | 512KB | 128KB | 16KB | FINE |
| R7F701003 | 768KB | 192KB | 16KB | FINE |
| R7F701020 | 1024KB | 256KB | 32KB | FINE |
| R7F701021 | 1024KB | 512KB | 32KB | FINE |
| R7F701022 | 2048KB | 512KB | 32KB | FINE |
| R7F701023 | 2048KB | 1024KB | 32KB | FINE |

### 2.4 V850/78K0系列
| 型号 | Flash | RAM | 调试接口 |
|------|-------|-----|----------|
| D70F3037 | 128KB | 32KB | JTAG |
| D70F3438 | 256KB | 48KB | JTAG |
| D70F3453 | 512KB | 64KB | JTAG |
| uPD78F0113 | 32KB | 4KB | UART |
| uPD78F0114 | 48KB | 4KB | UART |
| uPD78F0115 | 64KB | 4KB | UART |
| uPD78F0116 | 96KB | 6KB | UART |

---

## 三、TI德州仪器系列（对标UniFlash）

支持**40+款**TI芯片：

### 3.1 MSP430系列（16位超低功耗）
| 型号 | Flash | RAM | Info Flash | 调试接口 |
|------|-------|-----|------------|----------|
| MSP430G2231 | 2KB | 128B | - | SBW |
| MSP430G2452 | 8KB | 512B | 256B | SBW |
| MSP430G2553 | 16KB | 512B | 256B | SBW |
| MSP430G2955 | 48KB | 2KB | 512B | SBW |
| MSP430F5438A | 256KB | 16KB | 512B | SBW |
| MSP430F5529 | 128KB | 8KB | 512B | SBW |
| MSP430F5359 | 128KB | 16KB | 256B | SBW |
| MSP430FR2433 | 16KB(FRAM) | 1KB | - | SBW |
| MSP430FR4133 | 16KB(FRAM) | 2KB | - | SBW |
| MSP430FR5969 | 64KB(FRAM) | 2KB | - | SBW |
| MSP430FR5994 | 256KB(FRAM) | 8KB | - | SBW |
| MSP430FR6989 | 128KB(FRAM) | 2KB | - | SBW |
| MSP430FR5739 | 16KB(FRAM) | 1KB | - | SBW |

### 3.2 MSP432系列（ARM Cortex-M4）
| 型号 | Flash | RAM | 调试接口 |
|------|-------|-----|----------|
| MSP432P401R | 256KB | 64KB | SWD |
| MSP432P4111 | 512KB | 128KB | SWD |
| MSP432E401Y | 1024KB | 256KB | SWD |
| MSP432E411Y | 1024KB | 256KB | SWD |

### 3.3 CC系列（无线SoC）
| 型号 | Flash | RAM | 类型 | 调试接口 |
|------|-------|-----|------|----------|
| CC2530F256 | 256KB | 8KB | ZigBee | JTAG |
| CC2531F128 | 128KB | 8KB | ZigBee | JTAG |
| CC2533F32 | 32KB | 4KB | ZigBee | JTAG |
| CC2640R2F | 128KB | 20KB | BLE | JTAG |
| CC2652R1 | 352KB | 80KB | BLE/ZigBee | JTAG |
| CC2652RB | 352KB | 80KB | BLE/ZigBee | JTAG |
| CC1310F128 | 128KB | 20KB | Sub-1GHz | JTAG |
| CC1352R1 | 352KB | 80KB | Sub-1GHz/BLE | JTAG |
| CC1352P1 | 352KB | 80KB | Sub-1GHz/BLE | JTAG |
| CC3220R | 1024KB | 256KB | WiFi | UART |
| CC3220SF | 1024KB | 256KB | WiFi | UART |
| CC3235SF | 1024KB | 512KB | WiFi | UART |

### 3.4 TMS320系列（DSP）
| 型号 | Flash | RAM | 类型 | 调试接口 |
|------|-------|-----|------|----------|
| TMS320F28377S | 512KB | 100KB | C2000 DSP+MCU | JTAG |
| TMS320F28377D | 1024KB | 200KB | C2000 DSP+MCU | JTAG |
| TMS320F280049C | 256KB | 100KB | C2000 DSP+MCU | JTAG |
| TMS320F280041C | 128KB | 40KB | C2000 DSP+MCU | JTAG |
| TMS320F28335 | 256KB | 34KB | C2000 DSP+MCU | JTAG |
| TMS320F28027 | 64KB | 10KB | C2000 DSP+MCU | JTAG |
| TMS320F28069 | 256KB | 100KB | C2000 DSP+MCU | JTAG |
| TMS320C6748 | 512KB | 312KB | C6000 DSP | JTAG |
| TMS320C6657 | 1024KB | 512KB | C6000 DSP | JTAG |
| TMS320C6678 | - | 8MB | C6000 DSP | JTAG |

### 3.5 TMS570/TM470系列（车用安全MCU）
| 型号 | Flash | RAM | 调试接口 |
|------|-------|-----|----------|
| TMS570LS3137 | 1024KB | 256KB | JTAG |
| TMS570LS1227 | 768KB | 128KB | JTAG |
| TMS570LS0914 | 512KB | 64KB | JTAG |
| TMS570LS0432 | 256KB | 32KB | JTAG |
| TMS570LC4357 | 2048KB | 512KB | JTAG |
| TM4C1294NCPDT | 1024KB | 256KB | JTAG |
| TM4C123GH6PM | 256KB | 32KB | JTAG |
| TM4C129XNCZAD | 1024KB | 256KB | JTAG |

---

## 四、NXP系列（对标J-Flash）

支持**60+款**NXP芯片：

### 4.1 LPC系列
| 系列 | 型号 | Flash | RAM | 核心 | 调试接口 |
|------|------|-------|-----|------|----------|
| LPC800 | LPC812M101 | 16KB | 4KB | Cortex-M0+ | SWD |
| LPC800 | LPC824M201 | 32KB | 8KB | Cortex-M0+ | SWD |
| LPC800 | LPC845M301 | 64KB | 16KB | Cortex-M0+ | SWD |
| LPC1100 | LPC1114FBD48 | 32KB | 4KB | Cortex-M0 | SWD |
| LPC1100 | LPC1115FBD48 | 64KB | 8KB | Cortex-M0 | SWD |
| LPC1100 | LPC1343FBD48 | 32KB | 8KB | Cortex-M3 | SWD |
| LPC1100 | LPC1347FBD48 | 64KB | 8KB | Cortex-M3 | SWD |
| LPC1700 | LPC1754FBD80 | 128KB | 16KB | Cortex-M3 | JTAG |
| LPC1700 | LPC1756FBD80 | 256KB | 32KB | Cortex-M3 | JTAG |
| LPC1700 | LPC1758FBD80 | 512KB | 64KB | Cortex-M3 | JTAG |
| LPC1700 | LPC1768FBD100 | 512KB | 64KB | Cortex-M3 | JTAG |
| LPC1700 | LPC1788FBD208 | 512KB | 96KB | Cortex-M3 | JTAG |
| LPC4300 | LPC4337JBD144 | 512KB | 104KB | Cortex-M4/M0双核 | JTAG |
| LPC4300 | LPC4357FBD208 | 1024KB | 136KB | Cortex-M4/M0双核 | JTAG |
| LPC4300 | LPC4353FET256 | 1024KB | 136KB | Cortex-M4/M0双核 | JTAG |
| LPC54000 | LPC54608J512 | 512KB | 180KB | Cortex-M4F | SWD |
| LPC54000 | LPC54616J512 | 1024KB | 200KB | Cortex-M4F | SWD |

### 4.2 i.MX RT系列（跨界MCU）
| 型号 | 外部Flash | RAM | 核心 | 调试接口 |
|------|-----------|-----|------|----------|
| i.MXRT1011 | 16MB | 128KB | Cortex-M7 | SWD |
| i.MXRT1015 | 16MB | 128KB | Cortex-M7 | SWD |
| i.MXRT1021 | 16MB | 256KB | Cortex-M7 | SWD |
| i.MXRT1052 | 16MB | 512KB | Cortex-M7 | SWD |
| i.MXRT1062 | 16MB | 1024KB | Cortex-M7 | SWD |
| i.MXRT1064 | 16MB | 1024KB | Cortex-M7 | SWD |
| i.MXRT1176 | 16MB | 2048KB | Cortex-M7/M4双核 | SWD |

### 4.3 Kinetis系列
| 系列 | 型号 | Flash | RAM | 核心 | 调试接口 |
|------|------|-------|-----|------|----------|
| K | MK20DX128 | 128KB | 16KB | Cortex-M4 | SWD |
| K | MK20DX256 | 256KB | 64KB | Cortex-M4 | SWD |
| K | MK64FX512 | 512KB | 128KB | Cortex-M4F | SWD |
| K | MK66FX1M0 | 1024KB | 256KB | Cortex-M4F | SWD |
| L | KL03Z32 | 32KB | 2KB | Cortex-M0+ | SWD |
| L | KL05Z32 | 32KB | 4KB | Cortex-M0+ | SWD |
| L | KL25Z128 | 128KB | 16KB | Cortex-M0+ | SWD |
| L | KL27Z256 | 256KB | 32KB | Cortex-M0+ | SWD |
| L | KL46Z256 | 256KB | 32KB | Cortex-M0+ | SWD |

### 4.4 S32K系列（车用）
| 型号 | Flash | RAM | 核心 | 调试接口 |
|------|-------|-----|------|----------|
| S32K116 | 128KB | 16KB | Cortex-M0+ | SWD |
| S32K118 | 256KB | 32KB | Cortex-M0+ | SWD |
| S32K142 | 256KB | 32KB | Cortex-M2 | SWD |
| S32K144 | 512KB | 64KB | Cortex-M4F | SWD |
| S32K146 | 1024KB | 128KB | Cortex-M4F | SWD |
| S32K148 | 1024KB | 256KB | Cortex-M4F | SWD |
| S32K344 | 1024KB | 256KB | Cortex-M7 | SWD |
| S32K358 | 2048KB | 512KB | Cortex-M7 | SWD |

---

## 五、英飞凌TriCore系列

| 系列 | 型号 | Flash | RAM | 内核数 | 调试接口 |
|------|------|-------|-----|--------|----------|
| TC2xx | TC234L64F200 | 2MB | 192KB | 1 | DAP |
| TC2xx | TC264D128F200 | 4MB | 472KB | 2 | DAP |
| TC2xx | TC275T192F200 | 8MB | 1MB | 3 | DAP |
| TC2xx | TC297TX256F200 | 8MB | 2MB | 3 | DAP |
| TC3xx | TC333L128F300 | 6MB | 512KB | 1 | DAP |
| TC3xx | TC377TP128F300 | 8MB | 1.5MB | 2 | DAP |
| TC3xx | TC397XX256F300 | 16MB | 6.8MB | 6 | DAP |

---

## 六、DSP/FPGA/CPLD系列

### 6.1 DSP芯片
| 厂商 | 型号 | Flash | RAM | 类型 | 调试接口 |
|------|------|-------|-----|------|----------|
| TI | TMS320F28377D | 1024KB | 200KB | DSP+MCU | JTAG |
| TI | TMS320C6748 | 512KB | 312KB | DSP | JTAG |
| TI | TMS320C6678 | - | 8MB | DSP | JTAG |
| ADI | ADSP-BF537 | - | 32KB | Blackfin | JTAG |
| ADI | ADSP-BF609 | - | 128KB | Blackfin | JTAG |
| ADI | ADSP-21479 | - | 5MB | SHARC | JTAG |

### 6.2 FPGA芯片
| 厂商 | 型号 | 逻辑单元 | 配置Flash | 调试接口 |
|------|------|---------|-----------|----------|
| Xilinx | XC7A35T | 33K | 16MB | JTAG |
| Xilinx | XC7A100T | 101K | 32MB | JTAG |
| Xilinx | XC7K325T | 326K | 64MB | JTAG |
| Xilinx | XC6SLX9 | 9K | 8MB | JTAG |
| Intel | EP4CE6 | 6K | 8MB | JTAG |
| Intel | EP4CE30 | 30K | 32MB | JTAG |
| Intel | 5CEFA5 | 45K | 32MB | JTAG |
| Lattice | iCE40HX4K | 4K | 8MB | JTAG |
| Lattice | iCE40UP5K | 5K | 8MB | JTAG |
| Lattice | LFE5U-25F | 25K | 32MB | JTAG |
| 高云 | GW1N-1 | 1K | 8MB | JTAG |
| 高云 | GW2A-18 | 18K | 64MB | JTAG |
| 安路 | EG4S20 | 20K | 16MB | JTAG |

### 6.3 CPLD芯片
| 厂商 | 型号 | 宏单元 | 调试接口 |
|------|------|--------|----------|
| Xilinx | XC2C256 | 256 | JTAG |
| Xilinx | XC2C384 | 384 | JTAG |
| Xilinx | XC2C512 | 512 | JTAG |
| Intel | EPM240 | 240 | JTAG |
| Intel | EPM570 | 570 | JTAG |
| Intel | 10M02 | 240 | JTAG |
| Intel | 10M50 | 2400 | JTAG |
| Lattice | LCMXO2-1200 | 1200 | JTAG |
| Lattice | LCMXO3-2100 | 2100 | JTAG |

---

## 七、国产芯片系列

### 7.1 华大半导体
| 型号 | Flash | RAM | 核心 | 调试接口 |
|------|-------|-----|------|----------|
| HC32F030F8P6 | 64KB | 8KB | Cortex-M0+ | SWD |
| HC32L136K8TA | 64KB | 8KB | Cortex-M0+ | SWD |

### 7.2 航顺芯片
| 型号 | Flash | RAM | 核心 | 调试接口 |
|------|-------|-----|------|----------|
| HK32F103C8T6 | 64KB | 20KB | Cortex-M3 | SWD |
| HK32F030C8T6 | 64KB | 8KB | Cortex-M0+ | SWD |

### 7.3 国民技术
| 型号 | Flash | RAM | 核心 | 调试接口 |
|------|-------|-----|------|----------|
| N32G430C8L7 | 64KB | 20KB | Cortex-M4F | SWD |
| N32L436CBL7 | 256KB | 64KB | Cortex-M4F | SWD |

### 7.4 芯恒微
| 型号 | Flash | RAM | 核心 | 调试接口 |
|------|-------|-----|------|----------|
| CH32V307VCT6 | 512KB | 64KB | RISC-V | JTAG/DMI |
| CH552G | 16KB | 2KB | 8051 | USB |

---

## 八、支持的调试接口

本编程器支持**14种**调试接口：

| 接口 | 描述 | 支持芯片 | 最高频率 |
|------|------|---------|---------|
| SWD | Serial Wire Debug | ARM Cortex-M | 10MHz |
| JTAG | IEEE 1149.1 | 通用 | 10MHz |
| BDM | Background Debug Mode | NXP HC08/HCS08/HCS12 | 10MHz |
| SBW | Spy-Bi-Wire | TI MSP430 | 10MHz |
| MON8 | Monitor 8 | Freescale HC08/HC05 | 10MHz |
| FINE | Flash In-circuit Emulator | Renesas RL78/RH850 | 10MHz |
| ICSP | In-Circuit Serial Programming | Microchip PIC | 8MHz |
| ISP | In-System Programming | Atmel AVR | 8MHz |
| SWIM | Single Wire Interface Module | STM8 | 10MHz |
| cJTAG | Compact JTAG | 兼容JTAG | 10MHz |
| DAP | Debug Access Port | 英飞凌TriCore | 10MHz |
| DMI | Debug Module Interface | RISC-V | 10MHz |
| I2C | I2C EEPROM | EEPROM/NOR Flash | 1MHz |
| SPI | SPI Flash | NOR Flash | 50MHz |
| UART | Bootloader | 通用 | 115200bps |
| USB | USB编程 | 特殊芯片 | 12Mbps |

---

## 九、支持统计

| 类别 | 数量 |
|------|------|
| **SPI Flash芯片** | 200+款 |
| **MCU芯片** | 500+款 |
| **DSP芯片** | 20+款 |
| **FPGA芯片** | 30+款 |
| **CPLD芯片** | 15+款 |
| **调试接口** | 14种 |
| **厂商数量** | 20+ |
| **总计支持芯片** | **800+款** |

---

## 十、技术特点

1. **高速度** - 支持10MHz高速通信
2. **精定时** - 纳秒级定时器精度
3. **寄存器操作** - 直接寄存器加速
4. **自动识别** - ID自动检测匹配
5. **缓存优化** - LRU缓存加速查找
6. **插件驱动** - 可扩展驱动框架
7. **详细注释** - 完整中文注释文档

---

*文档版本: v2.0 | 更新日期: 2026-06-06 | AI_PROG项目*