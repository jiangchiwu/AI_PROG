# AI_PROG 项目文件资料分类整理

## 目录结构

```
F:\work\AI_PROG\
│
├── PROG/                          # 嵌入式主工程 (STM32H750)
│   ├── Core/                      # 核心代码
│   │   ├── Inc/                   # 头文件
│   │   │   ├── chip_driver_framework.h    # [核心] 可扩展驱动框架
│   │   │   ├── chip_vendors.h             # [核心] 芯片厂商定义
│   │   │   ├── debug_interface_manager.h  # [核心] 调试接口管理器
│   │   │   ├── swd.h, jtag.h              # SWD/JTAG调试接口
│   │   │   ├── bdm.h, sbw.h               # BDM/SBW调试接口
│   │   │   ├── mon8.h, fine.h             # MON8/FINE调试接口
│   │   │   └── ...                        # 其他外设头文件
│   │   │
│   │   └── Src/                   # 源文件
│   │       ├── chip_driver_framework.c    # [核心] 驱动框架实现
│   │       ├── chip_vendors.c             # [核心] 芯片厂商驱动
│   │       ├── debug_interface_manager.c  # [核心] 调试接口管理
│   │       └── ...                        # 其他外设源文件
│   │
│   ├── Drivers/                   # STM32 HAL驱动库
│   │   ├── CMSIS/                 # CMSIS核心
│   │   └── STM32H7xx_HAL_Driver/  # HAL驱动
│   │
│   ├── MDK-ARM/                   # Keil工程文件
│   │   ├── PROG.uvprojx           # 主工程文件
│   │   └── build.bat              # 构建脚本
│   │
│   └── Docs/                      # 工程文档
│       ├── chip_database.md               # 芯片数据库说明
│       ├── chip_reference_manuals.md      # 芯片资料下载链接
│       └── chip_support_progress.md       # 工作进度记录
│
├── pc/                            # PC端工具 (Python)
│   ├── chip_database_sqlite.py    # [核心] SQLite百万级芯片数据库
│   ├── chip_driver_framework.py   # [核心] 插件驱动框架
│   ├── chip_data_importer.py      # [工具] 批量数据导入
│   ├── generate_drivers.py        # [工具] 驱动模板生成
│   │
│   ├── generated_drivers/         # 生成的驱动文件
│   │   ├── driver_stm32.py        # STM32驱动
│   │   ├── driver_gd32.py         # GD32驱动
│   │   ├── driver_msp430.py       # MSP430驱动
│   │   └── ...                    # 其他驱动
│   │
│   ├── chips_million.db           # SQLite数据库 (不纳入git)
│   └── import_report.txt          # 导入报告
│
├── chip_docs/                     # 芯片文档 (不纳入git)
│   ├── GD/                        # 兆易创新
│   ├── Renesas/                   # 瑞萨
│   └── ...                        # 其他厂商
│
├── reports/                       # 测试报告
│   └── test_report_*.md
│
└── 开发资料/                      # 开发资料 (不纳入git)
    └── MiniSTM32H7xx/
```

## 文件分类说明

### 1. 核心框架文件 (必须纳入版本控制)

| 文件 | 说明 | 大小 |
|------|------|------|
| `Core/Inc/chip_driver_framework.h` | 可扩展驱动框架头文件 | ~30KB |
| `Core/Src/chip_driver_framework.c` | 驱动框架实现 | ~50KB |
| `Core/Inc/debug_interface_manager.h` | 调试接口管理器头文件 | ~15KB |
| `Core/Src/debug_interface_manager.c` | 调试接口管理器实现 | ~30KB |
| `pc/chip_database_sqlite.py` | SQLite百万级数据库 | ~40KB |
| `pc/chip_driver_framework.py` | 插件驱动框架 | ~50KB |
| `pc/chip_data_importer.py` | 批量数据导入工具 | ~80KB |

### 2. 调试接口实现

| 接口 | 头文件 | 源文件 | 适用芯片 |
|------|--------|--------|----------|
| SWD | swd.h | swd.c | ARM Cortex系列 |
| JTAG | jtag.h | jtag.c | ARM/PowerPC等 |
| BDM | bdm.h | bdm.c | NXP HCS12/HCS08 |
| SBW | sbw.h | sbw.c | TI MSP430 |
| MON8 | mon8.h | mon8.c | NXP HC08 |
| FINE | fine.h | fine.c | Renesas RH850 |

### 3. 生成的驱动文件

位于 `pc/generated_drivers/` 目录，共14个驱动模板：
- driver_arm_cortex_m.py - ARM Cortex-M通用
- driver_stm32.py - ST STM32系列
- driver_gd32.py - 兆易创新GD32系列
- driver_s32k.py - NXP S32K系列
- driver_msp430.py - TI MSP430系列
- driver_hcs12.py - NXP HCS12系列
- driver_rh850.py - Renesas RH850系列
- driver_aurix.py - Infineon AURIX系列
- driver_pic.py - Microchip PIC系列
- driver_avr.py - Microchip AVR系列
- driver_riscv.py - RISC-V通用
- driver_ch32v.py - WCH CH32V系列
- driver_8051.py - 8051通用
- driver_stc.py - STC系列

### 4. 文档文件

| 文件 | 说明 |
|------|------|
| `Docs/chip_database.md` | 芯片数据库结构说明 |
| `Docs/chip_reference_manuals.md` | 各厂商资料下载链接汇总 |
| `Docs/chip_support_progress.md` | 工作进度和版本记录 |
| `PROJECT_PLAN.md` | 项目计划 |
| `README.md` | 项目说明 |

### 5. 不纳入版本控制的文件

| 类型 | 原因 |
|------|------|
| `*.pdf` | 文件过大 (>10MB) |
| `*.db` | 数据库文件，可重新生成 |
| `chip_docs/` | 下载的芯片文档 |
| `开发资料/` | 第三方开发资料 |
| `__pycache__/` | Python缓存 |
| `*.log` | 日志文件 |

## Git管理建议

### 1. 使用.gitignore排除大文件

```gitignore
# 大文件排除
*.pdf
*.db
chip_docs/
开发资料/

# 编译产物
*.bin
*.hex
*.elf
```

### 2. 大文件存储方案

对于必须保留的大文件（如PDF文档），建议：
1. 使用Git LFS (Large File Storage)
2. 或存储在云盘，在文档中提供下载链接
3. 或只保留小尺寸的预览版

### 3. 数据库文件处理

SQLite数据库文件 `chips_million.db` 不纳入版本控制，但：
- 保留数据库创建脚本 `chip_database_sqlite.py`
- 保留数据导入工具 `chip_data_importer.py`
- 保留导入报告 `import_report.txt`
- 用户可自行运行脚本重建数据库

## 文件大小统计

| 目录 | 文件数 | 总大小 | 说明 |
|------|--------|--------|------|
| PROG/Core/Inc/ | ~30 | ~200KB | 头文件 |
| PROG/Core/Src/ | ~30 | ~500KB | 源文件 |
| pc/ | ~20 | ~300KB | Python工具 |
| pc/generated_drivers/ | 14 | ~150KB | 生成驱动 |
| chip_docs/ | ~3 | ~5MB | PDF文档(不纳入git) |
| 开发资料/ | ~1 | ~36MB | PDF文档(不纳入git) |

**纳入Git的总大小**: 约 1.2MB (不含编译产物)

## 更新记录

| 日期 | 版本 | 更新内容 |
|------|------|----------|
| 2026-06-03 | v1.0 | 初始版本，整理文件分类 |
| 2026-06-03 | v2.0 | 添加.gitignore，排除大文件 |