# 芯片抽象层设计文档

## 1. 芯片抽象层概述

芯片抽象层（Chip Abstraction Layer, CAL）位于内核抽象层之上，提供对具体芯片型号的访问和控制。CAL层封装了不同芯片的特定功能和Flash编程算法，使得上层应用可以透明地操作各种芯片。

## 2. 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                   应用层 (Application)                   │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐              │
│  │ Flash    │ │ Memory   │ │ Config   │              │
│  │ Program  │ │ Viewer   │ │ Tool    │              │
│  └──────────┘ └──────────┘ └──────────┘              │
└─────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────┐
│               芯片抽象层 (Chip Abstraction Layer)       │
│  ┌─────────────────────────────────────────────────┐  │
│  │              Chip Operations Interface             │  │
│  │  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐   │  │
│  │  │ Flash  │ │ Config │ │ OTP   │ │ Secu-  │   │  │
│  │  │ Access │ │ Access │ │ Access │ │ rity   │   │  │
│  │  └────────┘ └────────┘ └────────┘ └────────┘   │  │
│  └─────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────┐
│            内核抽象层 (Core Abstraction Layer)           │
│  ┌─────────────────────────────────────────────────┐  │
│  │        Core Operations Interface                   │  │
│  └─────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## 3. 芯片信息结构

```c
typedef struct {
    // 基本信息
    char name[32];                    // 芯片名称
    char family[16];                  // 系列名称
    uint32_t id;                      // 芯片ID
    uint32_t rev;                     // 芯片版本

    // 内核信息
    Core_Type_TypeDef core_type;     // 内核类型
    uint32_t core_freq;               // 内核频率 (Hz)

    // 内存信息
    uint32_t flash_base;             // Flash基地址
    uint32_t flash_size;             // Flash大小
    uint32_t flash_page_size;        // Flash页大小
    uint32_t flash_sector_size;      // Flash扇区大小
    uint32_t ram_base;               // RAM基地址
    uint32_t ram_size;               // RAM大小

    // Flash特性
    uint8_t flash_dual_bank;         // 双Bank支持
    uint8_t flash_eeprom_emulation;  // EEPROM仿真
    uint32_t eeprom_size;            // EEPROM大小

    // 安全特性
    uint8_t has_security;            // 安全位支持
    uint8_t has_mpu;                 // MPU支持
    uint8_t has_mmu;                 // MMU支持

    // 调试接口
    uint8_t debug_interface;         // 调试接口类型
    // 0 = JTAG/SWD, 1 = SWD only, 2 = JTAG only

    // 外设信息
    uint32_t peripheral_mask;         // 外设掩码

    // 封装信息
    char package[16];                // 封装类型

    // 特定芯片驱动
    const Chip_Driver_TypeDef *driver;
} Chip_Info_TypeDef;
```

## 4. 芯片操作接口

```c
typedef struct {
    // 芯片识别
    HAL_StatusTypeDef (*identify)(Chip_Info_TypeDef *info);
    HAL_StatusTypeDef (*verify_id)(void);

    // 连接/断开
    HAL_StatusTypeDef (*connect)(void);
    HAL_StatusTypeDef (*disconnect)(void);

    // Flash操作
    HAL_StatusTypeDef (*flash_erase)(uint32_t addr, uint32_t size);
    HAL_StatusTypeDef (*flash_erase_chip)(void);
    HAL_StatusTypeDef (*flash_write)(uint32_t addr, uint8_t *data, uint32_t size);
    HAL_StatusTypeDef (*flash_read)(uint32_t addr, uint8_t *data, uint32_t size);
    HAL_StatusTypeDef (*flash_verify)(uint32_t addr, uint8_t *data, uint32_t size);

    // RAM操作
    HAL_StatusTypeDef (*ram_write)(uint32_t addr, uint8_t *data, uint32_t size);
    HAL_StatusTypeDef (*ram_read)(uint32_t addr, uint8_t *data, uint32_t size);

    // 选项字节操作
    HAL_StatusTypeDef (*option_read)(uint32_t addr, uint8_t *data, uint32_t size);
    HAL_StatusTypeDef (*option_write)(uint32_t addr, uint8_t *data, uint32_t size);

    // 安全操作
    HAL_StatusTypeDef (*read_protect)(void);
    HAL_StatusTypeDef (*read_unprotect)(void);
    HAL_StatusTypeDef (*get_rp_status)(uint8_t *enabled);
    HAL_StatusTypeDef (*set_boot_source)(uint8_t source);
    HAL_StatusTypeDef (*get_boot_source)(uint8_t *source);

    // 加密操作
    HAL_StatusTypeDef (*encrypt)(uint8_t *key);
    HAL_StatusTypeDef (*decrypt)(uint8_t *key);
    HAL_StatusTypeDef (*mass_erase)(void);

    // 复位控制
    HAL_StatusTypeDef (*reset)(void);
    HAL_StatusTypeDef (*reset_halt)(void);

    // 校准
    HAL_StatusTypeDef (*calibrate)(void);
} Chip_Driver_TypeDef;
```

## 5. Flash编程算法

### 5.1 Flash算法接口

```c
typedef struct {
    // 算法信息
    char name[32];
    uint32_t version;

    // Flash特性
    uint32_t flash_base;
    uint32_t flash_size;
    uint32_t page_size;
    uint32_t sector_size;
    uint32_t block_size;

    // 编程参数
    uint32_t max_program_page;
    uint32_t program_timeout;
    uint32_t erase_timeout;
    uint32_t mass_erase_timeout;

    // 算法函数
    HAL_StatusTypeDef (*init)(void);
    HAL_StatusTypeDef (*uninit)(void);
    HAL_StatusTypeDef (*erase_sector)(uint32_t addr);
    HAL_StatusTypeDef (*erase_block)(uint32_t addr);
    HAL_StatusTypeDef (*erase_chip)(void);
    HAL_StatusTypeDef (*program_page)(uint32_t addr, uint8_t *data, uint32_t size);
    HAL_StatusTypeDef (*verify)(uint32_t addr, uint8_t *data, uint32_t size);
    HAL_StatusTypeDef (*blank_check)(uint32_t addr, uint32_t size);
} Flash_Algorithm_TypeDef;
```

### 5.2 通用Flash编程流程

```c
HAL_StatusTypeDef Chip_Flash_Write(Chip_Info_TypeDef *chip, uint32_t addr, uint8_t *data, uint32_t size)
{
    Flash_Algorithm_TypeDef *algo = chip->driver->flash_algo;

    // 1. 解锁Flash
    HAL_StatusTypeDef status = Flash_Unlock();
    if (status != HAL_OK) {
        return status;
    }

    // 2. 擦除需要编程的扇区
    uint32_t sector_size = algo->sector_size;
    uint32_t start_sector = addr / sector_size;
    uint32_t end_sector = (addr + size - 1) / sector_size;

    for (uint32_t i = start_sector; i <= end_sector; i++) {
        status = algo->erase_sector(i * sector_size);
        if (status != HAL_OK) {
            return status;
        }
    }

    // 3. 编程Flash页
    uint32_t page_size = algo->page_size;
    uint32_t offset = 0;

    while (offset < size) {
        uint32_t page_addr = addr + offset;
        uint32_t chunk_size = (size - offset) > page_size ? page_size : (size - offset);

        status = algo->program_page(page_addr, data + offset, chunk_size);
        if (status != HAL_OK) {
            return status;
        }

        offset += chunk_size;
    }

    // 4. 锁定Flash
    Flash_Lock();

    return HAL_OK;
}
```

## 6. 选项字节管理

### 6.1 选项字节接口

```c
typedef struct {
    uint32_t address;
    uint32_t default_value;
    uint32_t current_value;
    char name[32];
    char description[64];
} Option_Byte_TypeDef;

typedef struct {
    uint16_t count;
    Option_Byte_TypeDef *bytes;
} Option_Byte_Table_TypeDef;

HAL_StatusTypeDef Chip_Option_Read(Chip_Info_TypeDef *chip, Option_Byte_Table_TypeDef *table)
{
    for (uint16_t i = 0; i < table->count; i++) {
        table->bytes[i].current_value = Chip_Read32(chip, table->bytes[i].address);
    }
    return HAL_OK;
}

HAL_StatusTypeDef Chip_Option_Write(Chip_Info_TypeDef *chip, Option_Byte_Table_TypeDef *table)
{
    // 解锁选项字节
    HAL_StatusTypeDef status = Flash_Unlock_Option_Bytes();
    if (status != HAL_OK) {
        return status;
    }

    // 写入选项字节
    for (uint16_t i = 0; i < table->count; i++) {
        if (table->bytes[i].current_value != table->bytes[i].default_value) {
            Chip_Write32(chip, table->bytes[i].address, table->bytes[i].current_value);
        }
    }

    // 锁定选项字节
    Flash_Lock_Option_Bytes();

    // 触发复位加载新选项
    Chip_Reset(chip);

    return HAL_OK;
}
```

## 7. 安全特性管理

### 7.1 读保护

```c
typedef enum {
    RDP_LEVEL_0,   // 无保护
    RDP_LEVEL_1,   // 读保护级别1
    RDP_LEVEL_2,   // 读保护级别2 (不可逆)
} RDP_Level_TypeDef;

HAL_StatusTypeDef Chip_Set_Read_Protection(Chip_Info_TypeDef *chip, RDP_Level_TypeDef level)
{
    switch (level) {
        case RDP_LEVEL_0:
            // 移除读保护
            Flash_Unprotect();
            break;

        case RDP_LEVEL_1:
            // 设置读保护级别1
            Flash_Protect();
            break;

        case RDP_LEVEL_2:
            // 设置读保护级别2
            Flash_Protect_Permanent();
            break;

        default:
            return HAL_ERROR;
    }

    // 触发系统复位
    Chip_Reset(chip);

    return HAL_OK;
}

RDP_Level_TypeDef Chip_Get_Read_Protection(Chip_Info_TypeDef *chip)
{
    uint32_t option = Flash_Get_Option_Byte();

    if (option & (1 << 2)) {
        return RDP_LEVEL_2;
    } else if (option & (1 << 0)) {
        return RDP_LEVEL_1;
    } else {
        return RDP_LEVEL_0;
    }
}
```

### 7.2 芯片唯一ID

```c
typedef struct {
    uint8_t x[4];    // X坐标
    uint8_t y[4];    // Y坐标
    uint8_t wafer[2]; // 晶圆批次
    uint8_t lot[7];  // 批次号
} Chip_Unique_ID_TypeDef;

HAL_StatusTypeDef Chip_Get_Unique_ID(Chip_Info_TypeDef *chip, Chip_Unique_ID_TypeDef *uid)
{
    // 不同芯片的UID地址不同
    uint32_t uid_base = chip->uid_base;

    // 读取UID
    uint32_t x_y = Chip_Read32(chip, uid_base);
    uint32_t wafer_lot = Chip_Read32(chip, uid_base + 0x04);
    uint32_t lot_num = Chip_Read32(chip, uid_base + 0x08);

    // 解析UID
    uid->x[0] = (x_y >> 0) & 0xFF;
    uid->x[1] = (x_y >> 8) & 0xFF;
    uid->x[2] = (x_y >> 16) & 0xFF;
    uid->x[3] = (x_y >> 24) & 0xFF;

    uid->y[0] = (x_y >> 0) & 0xFF;
    uid->y[1] = (x_y >> 8) & 0xFF;
    uid->y[2] = (x_y >> 16) & 0xFF;
    uid->y[3] = (x_y >> 24) & 0xFF;

    uid->wafer[0] = (wafer_lot >> 0) & 0xFF;
    uid->wafer[1] = (wafer_lot >> 8) & 0xFF;

    uid->lot[0] = (lot_num >> 0) & 0xFF;
    uid->lot[1] = (lot_num >> 8) & 0xFF;
    uid->lot[2] = (lot_num >> 16) & 0xFF;
    uid->lot[3] = (lot_num >> 24) & 0xFF;
    uid->lot[4] = (lot_num >> 0) & 0xFF;
    uid->lot[5] = (lot_num >> 8) & 0xFF;
    uid->lot[6] = (lot_num >> 16) & 0xFF;

    return HAL_OK;
}
```

## 8. 芯片驱动注册

```c
// 芯片驱动注册表
typedef struct {
    uint32_t chip_id;
    const Chip_Driver_TypeDef *driver;
} Chip_Driver_Registry_TypeDef;

// 芯片驱动列表
static const Chip_Driver_Registry_TypeDef chip_driver_registry[] = {
    // STM32系列
    {0x0413, &stm32f1_driver},    // STM32F103
    {0x0419, &stm32f1_driver},    // STM32F105/107
    {0x0433, &stm32f4_driver},    // STM32F405/407
    {0x0435, &stm32f4_driver},    // STM32F415/417
    {0x0441, &stm32f4_driver},    // STM32F427/429
    {0x0451, &stm32f7_driver},    // STM32F746
    {0x0457, &stm32h7_driver},    // STM32H743

    // NXP S32K系列
    {0x32C0, &s32k11x_driver},   // S32K116
    {0x32C1, &s32k11x_driver},   // S32K118
    {0x32C2, &s32k14x_driver},   // S32K142
    {0x32C3, &s32k14x_driver},   // S32K144
    {0x32C4, &s32k14x_driver},   // S32K146
    {0x32C5, &s32k14x_driver},   // S32K148
    {0x32D0, &s32k34x_driver},   // S32K344
    {0x32D1, &s32k34x_driver},   // S32K344 with secure boot
    {0x32D2, &s32k34x_driver},   // S32K346
    {0x32D3, &s32k34x_driver},   // S32K348
    // ... 更多芯片

    {0xFFFFFFFF, NULL}  // 结束标记
};

const Chip_Driver_TypeDef* Chip_Find_Driver(uint32_t chip_id)
{
    for (int i = 0; chip_driver_registry[i].driver != NULL; i++) {
        if (chip_driver_registry[i].chip_id == chip_id) {
            return chip_driver_registry[i].driver;
        }
    }
    return NULL;
}
```

## 9. 芯片自动识别

```c
HAL_StatusTypeDef Chip_Auto_Detect(Chip_Info_TypeDef *info)
{
    // 1. 尝试通过调试接口读取芯片ID
    uint32_t chip_id = Chip_Read_ID();

    if (chip_id == 0 || chip_id == 0xFFFFFFFF) {
        return HAL_ERROR;  // 无法识别芯片
    }

    // 2. 在芯片数据库中查找匹配的芯片
    const Chip_Driver_TypeDef *driver = Chip_Find_Driver(chip_id);
    if (driver == NULL) {
        return HAL_ERROR;  // 未找到匹配的驱动
    }

    // 3. 调用驱动的识别函数
    info->id = chip_id;
    info->driver = driver;

    if (driver->identify != NULL) {
        return driver->identify(info);
    }

    return HAL_OK;
}
```

## 10. 支持的芯片列表

### 10.1 STM32系列

| 系列 | 内核 | Flash范围 | RAM范围 | 状态 |
|------|------|----------|---------|------|
| STM32F0 | Cortex-M0 | 16-256KB | 4-32KB | ⬜ |
| STM32F1 | Cortex-M3 | 16-512KB | 6-96KB | ⬜ |
| STM32F2 | Cortex-M3 | 128-1024KB | 64-128KB | ⬜ |
| STM32F3 | Cortex-M4 | 32-512KB | 16-80KB | ⬜ |
| STM32F4 | Cortex-M4 | 128-2048KB | 64-256KB | ⬜ |
| STM32F7 | Cortex-M7 | 256-2048KB | 256-512KB | ⬜ |
| STM32H7 | Cortex-M7 | 128-2048KB | 128-1024KB | ⬜ |
| STM32L0 | Cortex-M0+ | 16-192KB | 2-20KB | ⬜ |
| STM32L1 | Cortex-M3 | 32-512KB | 8-80KB | ⬜ |
| STM32L4 | Cortex-M4 | 64-1024KB | 32-256KB | ⬜ |
| STM32L5 | Cortex-M33 | 256-512KB | 256KB | ⬜ |
| STM32G0 | Cortex-M0+ | 32-512KB | 8-144KB | ⬜ |
| STM32G4 | Cortex-M4 | 32-512KB | 12-128KB | ⬜ |

### 10.2 NXP S32K系列

详见 S32K_CHIPS.md

### 10.3 其他芯片

| 厂商 | 系列 | 内核 | 状态 |
|------|------|------|------|
| NXP | LPC11xx | Cortex-M0 | ⬜ |
| NXP | LPC13xx | Cortex-M3 | ⬜ |
| NXP | LPC17xx | Cortex-M3 | ⬜ |
| NXP | LPC40xx | Cortex-M4 | ⬜ |
| NXP | Kinetis K | Cortex-M4 | ⬜ |
| TI | Tiva C | Cortex-M4 | ⬜ |
| TI | MSP432 | Cortex-M4F | ⬜ |
| Microchip | SAM D21 | Cortex-M0+ | ⬜ |
| Microchip | SAM E70 | Cortex-M7 | ⬜ |
| GD | GD32F1 | Cortex-M3 | ⬜ |
| GD | GD32F4 | Cortex-M4 | ⬜ |

## 11. 芯片资料管理

### 11.1 资料分类

```
芯片资料/
├── 数据手册 (Datasheet)
│   ├── 芯片特性
│   ├── 电气参数
│   ├── 引脚定义
│   └── 封装信息
│
├── 参考手册 (Reference Manual)
│   ├── 外设描述
│   ├── 寄存器定义
│   ├── 时序参数
│   └── 应用电路
│
├── 编程手册 (Programming Manual)
│   ├── 指令集
│   ├── 调试接口
│   ├── Flash编程算法
│   └── 安全特性
│
├── 应用笔记 (Application Note)
│   ├── 典型应用
│   ├── 设计指南
│   └── 问题解决方案
│
└── 勘误表 (Errata)
    ├── 已知问题
    └── 规避方案
```

### 11.2 资料下载清单

详见 CHIP_DOCUMENTS.md

## 12. 测试计划

### 12.1 功能测试

| 测试项目 | 测试内容 | 验收标准 |
|---------|---------|----------|
| 芯片识别 | 自动识别连接芯片 | 100%正确识别 |
| Flash擦除 | 扇区/块/全片擦除 | 所有模式正常 |
| Flash编程 | 页/多页编程 | 编程正确 |
| Flash读取 | 读取验证 | 数据一致 |
| 选项字节 | 读写选项字节 | 读写正确 |
| 读保护 | 设置/检查读保护 | 功能正常 |
| 写保护 | 设置/检查写保护 | 功能正常 |
| 芯片复位 | 软复位 | 复位成功 |

### 12.2 兼容性测试

- 每个系列至少测试3款芯片
- 测试覆盖率 > 90%

## 13. 文档版本

- 版本: v1.0
- 创建日期: 2026-06-01
- 最后更新: 2026-06-01
- 状态: 草稿
