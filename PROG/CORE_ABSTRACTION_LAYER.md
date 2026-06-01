# ARM内核抽象层设计文档

## 1. 内核抽象层概述

为了支持多种ARM内核，我们需要在DAP层之上实现一个通用的内核抽象层（Core Abstraction Layer, CAL）。该层提供统一的接口，隐藏不同内核的差异，使得上层应用可以透明地访问各种ARM处理器。

## 2. 支持的内核列表

### 2.1 Cortex-M系列（微控制器）

| 内核 | 位宽 | FPU | DSP | 应用 | 代表芯片 |
|------|------|-----|-----|------|---------|
| Cortex-M0 | 32-bit | - | - | 低成本 | STM32F0, LPC11xx |
| Cortex-M0+ | 32-bit | - | - | 低功耗 | STM32L0, SAMD21 |
| Cortex-M1 | 32-bit | - | - | FPGA | - |
| Cortex-M3 | 32-bit | - | - | 通用 | STM32F1, LPC17xx |
| Cortex-M4 | 32-bit | 可选 | 是 | 数字信号 | STM32F4, LPC43xx |
| Cortex-M7 | 32-bit | 是 | 是 | 高性能 | STM32F7, STM32H7 |
| Cortex-M33 | 32-bit | 可选 | 是 | 安全 | STM32L5, STM32G0 |
| Cortex-M35P | 32-bit | 是 | 是 | 安全+防篡改 | - |
| Cortex-M55 | 32-bit | 是 | 是 | AI/ML | - |
| Cortex-M85 | 32-bit | 是 | 是 | 高性能+AI | - |

### 2.2 Cortex-R系列（实时处理器）

| 内核 | 位宽 | FPU | L1 Cache | 应用 | 代表芯片 |
|------|------|-----|----------|------|---------|
| Cortex-R4 | 32-bit | 可选 | - | 汽车/工业 | - |
| Cortex-R5 | 32-bit | 可选 | - | 汽车 | - |
| Cortex-R7 | 32-bit | 可选 | 可选 | 高性能实时 | - |
| Cortex-R8 | 32-bit | 可选 | 可选 | 汽车 | - |

### 2.3 Cortex-A系列（应用处理器）

| 内核 | 位宽 | FPU | L1/L2 Cache | 应用 | 代表芯片 |
|------|------|-----|-------------|------|---------|
| Cortex-A5 | 32-bit | VFPv4 | 可选 | 低成本 | - |
| Cortex-A7 | 32-bit | VFPv4 | 可选 | 入门级 | i.MX6ULL |
| Cortex-A8 | 32-bit | VFPv3 | 可选 | 多媒体 | AM335x |
| Cortex-A9 | 32-bit | VFPv3 | 可选 | 多核 | i.MX6, Zynq |
| Cortex-A12 | 32-bit | VFPv4 | 可选 | 移动设备 | - |
| Cortex-A15 | 32-bit | VFPv4 | 可选 | 高性能 | Exynos 5 |
| Cortex-A32 | 32-bit | VFPv4 | 可选 | 嵌入式/IoT | - |
| Cortex-A35 | 64-bit | FP-ARMv8 | 可选 | 效率 | - |
| Cortex-A53 | 64-bit | FP-ARMv8 | 可选 | 平衡 | i.MX8M |
| Cortex-A55 | 64-bit | FP-ARMv8 | 可选 | 高效率 | - |
| Cortex-A72 | 64-bit | FP-ARMv8 | 可选 | 高性能 | - |
| Cortex-A73 | 64-bit | FP-ARMv8 | 可选 | 移动 | - |
| Cortex-A75 | 64-bit | FP-ARMv8 | 可选 | 高性能 | - |
| Cortex-A76 | 64-bit | FP-ARMv8 | 可选 | 高性能笔记本 | - |
| Cortex-A77 | 64-bit | FP-ARMv8 | 可选 | 高性能 | - |
| Cortex-A78 | 64-bit | FP-ARMv8 | 可选 | 旗舰移动 | - |
| Cortex-A710 | 64-bit | FP-ARMv8 | 可选 | 高效 | - |
| Cortex-X1 | 64-bit | FP-ARMv8 | 可选 | 旗舰 | - |
| Cortex-X2 | 64-bit | FP-ARMv8 | 可选 | 旗舰 | - |

## 3. 架构设计

```
┌─────────────────────────────────────────────────────┐
│              应用层 (Application)                    │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐         │
│  │ Flash    │ │ Memory   │ │ Debug    │         │
│  │ Program  │ │ Viewer   │ │ Monitor  │         │
│  └──────────┘ └──────────┘ └──────────┘         │
└─────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────┐
│          内核抽象层 (Core Abstraction Layer)         │
│  ┌────────────────────────────────────────────┐  │
│  │           Core Operations Interface         │  │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐   │  │
│  │  │CoreID│ │Reg   │ │Mem   │ │Debug │   │  │
│  │  │Detect│ │Access│ │Access│ │Event │   │  │
│  │  └──────┘ └──────┘ └──────┘ └──────┘   │  │
│  └────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────┐
│        调试接口层 (Debug Interface Layer)           │
│  ┌──────────────┐ ┌──────────────┐                 │
│  │   DAP       │ │   JTAG-DP   │                 │
│  │ (SWD/JTAG)  │ │  (IEEE1149) │                 │
│  └──────────────┘ └──────────────┘                 │
└─────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────┐
│        物理接口层 (Physical Interface Layer)        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐           │
│  │   SWD   │ │   JTAG   │ │   cJTAG  │           │
│  │  GPIO   │ │   GPIO   │ │  (ARMv8) │           │
│  └──────────┘ └──────────┘ └──────────┘           │
└─────────────────────────────────────────────────────┘
```

## 4. 内核抽象层接口定义

### 4.1 核心类型定义

```c
typedef enum {
    CORE_TYPE_CORTEX_M0,
    CORE_TYPE_CORTEX_M0P,
    CORE_TYPE_CORTEX_M1,
    CORE_TYPE_CORTEX_M3,
    CORE_TYPE_CORTEX_M4,
    CORE_TYPE_CORTEX_M7,
    CORE_TYPE_CORTEX_M33,
    CORE_TYPE_CORTEX_M35P,
    CORE_TYPE_CORTEX_M55,
    CORE_TYPE_CORTEX_M85,
    CORE_TYPE_CORTEX_R4,
    CORE_TYPE_CORTEX_R5,
    CORE_TYPE_CORTEX_R7,
    CORE_TYPE_CORTEX_R8,
    CORE_TYPE_CORTEX_A5,
    CORE_TYPE_CORTEX_A7,
    CORE_TYPE_CORTEX_A8,
    CORE_TYPE_CORTEX_A9,
    CORE_TYPE_CORTEX_A12,
    CORE_TYPE_CORTEX_A15,
    CORE_TYPE_CORTEX_A32,
    CORE_TYPE_CORTEX_A35,
    CORE_TYPE_CORTEX_A53,
    CORE_TYPE_CORTEX_A55,
    CORE_TYPE_CORTEX_A72,
    CORE_TYPE_CORTEX_A73,
    CORE_TYPE_CORTEX_A75,
    CORE_TYPE_CORTEX_A76,
    CORE_TYPE_CORTEX_A77,
    CORE_TYPE_CORTEX_A78,
    CORE_TYPE_CORTEX_A710,
    CORE_TYPE_CORTEX_X1,
    CORE_TYPE_CORTEX_X2,
    CORE_TYPE_UNKNOWN,
} Core_Type_TypeDef;

typedef enum {
    CORE_STATE_HALTED,
    CORE_STATE_RUNNING,
    CORE_STATE_RESET,
    CORE_STATE_UNKNOWN,
} Core_State_TypeDef;

typedef struct {
    Core_Type_TypeDef type;
    char name[32];
    uint32_t idcode;
    uint8_t has_fpu;
    uint8_t has_dsp;
    uint8_t has_mpu;
    uint8_t has_mmu;
    uint32_t rom_base;
    uint32_t rom_size;
    uint32_t ram_base;
    uint32_t ram_size;
} Core_Info_TypeDef;
```

### 4.2 内核操作接口

```c
typedef struct {
    HAL_StatusTypeDef (*init)(void);
    HAL_StatusTypeDef (*detect)(Core_Info_TypeDef *info);
    HAL_StatusTypeDef (*reset)(void);
    Core_State_TypeDef (*get_state)(void);
    HAL_StatusTypeDef (*halt)(void);
    HAL_StatusTypeDef (*resume)(void);
    HAL_StatusTypeDef (*step)(void);
    HAL_StatusTypeDef (*set_pc)(uint32_t pc);
    uint32_t (*get_pc)(void);
    uint32_t (*get_reg)(uint8_t reg_index);
    HAL_StatusTypeDef (*set_reg)(uint8_t reg_index, uint32_t value);
    uint32_t (*read_memory)(uint32_t addr);
    HAL_StatusTypeDef (*write_memory)(uint32_t addr, uint32_t value);
    HAL_StatusTypeDef (*read_memory_block)(uint32_t addr, uint8_t *data, uint32_t size);
    HAL_StatusTypeDef (*write_memory_block)(uint32_t addr, uint8_t *data, uint32_t size);
    HAL_StatusTypeDef (*halt_on_breakpoint)(uint32_t addr);
    HAL_StatusTypeDef (*remove_breakpoint)(uint32_t addr);
    HAL_StatusTypeDef (*halt_on_exception)(uint8_t exception_num);
} Core_Ops_TypeDef;
```

### 4.3 通用内核操作函数

```c
// 通用初始化
HAL_StatusTypeDef Core_Init(void);

// 内核检测
HAL_StatusTypeDef Core_Detect(Core_Info_TypeDef *info);

// 内核控制
HAL_StatusTypeDef Core_Reset(void);
Core_State_TypeDef Core_GetState(void);
HAL_StatusTypeDef Core_Halt(void);
HAL_StatusTypeDef Core_Resume(void);
HAL_StatusTypeDef Core_Step(void);

// 寄存器访问
uint32_t Core_GetPC(void);
HAL_StatusTypeDef Core_SetPC(uint32_t pc);
uint32_t Core_GetRegister(uint8_t reg_index);
HAL_StatusTypeDef Core_SetRegister(uint8_t reg_index, uint32_t value);

// 内存访问
uint32_t Core_ReadWord(uint32_t addr);
uint16_t Core_ReadHalfWord(uint32_t addr);
uint8_t Core_ReadByte(uint32_t addr);
HAL_StatusTypeDef Core_WriteWord(uint32_t addr, uint32_t value);
HAL_StatusTypeDef Core_WriteHalfWord(uint32_t addr, uint16_t value);
HAL_StatusTypeDef Core_WriteByte(uint32_t addr, uint8_t value);
HAL_StatusTypeDef Core_ReadMemory(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef Core_WriteMemory(uint32_t addr, uint8_t *data, uint32_t size);

// 断点管理
HAL_StatusTypeDef Core_SetBreakpoint(uint32_t addr);
HAL_StatusTypeDef Core_ClearBreakpoint(uint32_t addr);

// 异常处理
HAL_StatusTypeDef Core_HaltOnException(uint8_t exception_num);
```

## 5. 不同内核的差异处理

### 5.1 Cortex-M系列

#### 5.1.1 寄存器映射

| 通用寄存器 | 别名 | 说明 |
|-----------|------|------|
| R0-R12 | - | 通用寄存器 |
| SP | R13 | 堆栈指针 |
| LR | R14 | 连接寄存器 |
| PC | R15 | 程序计数器 |
| xPSR | - | 程序状态寄存器 |
| MSP | - | 主堆栈指针 |
| PSP | - | 进程堆栈指针 |
| PRIMASK | - | 中断掩码 |
| CONTROL | - | 控制寄存器 |

#### 5.1.2 调试组件

```
Cortex-M调试组件：
├── CoreDebug (DCB)
│   ├── DHCSR (Debug Halting Control and Status)
│   ├── DCRDR (Debug Core Register Data)
│   ├── DCRSR (Debug Core Register Selector)
│   └── DEMCR (Debug Exception and Monitor Control)
├── ITM (Instrumentation Trace Macrocell)
├── DWT (Data Watchpoint and Trace)
├── FPB (Flash Patch and Breakpoint)
└── TPIU (Trace Port Interface Unit)
```

#### 5.1.3 调试寄存器定义

```c
// CoreDebug寄存器地址
#define CoreDebug_BASE       0xE000EDF00UL
#define CoreDebug_DHCSR      (*(volatile uint32_t *)(CoreDebug_BASE + 0x00))
#define CoreDebug_DCRDR      (*(volatile uint32_t *)(CoreDebug_BASE + 0x04))
#define CoreDebug_DCRSR      (*(volatile uint32_t *)(CoreDebug_BASE + 0x08))
#define CoreDebug_DEMCR      (*(volatile uint32_t *)(CoreDebug_BASE + 0x0C))

// DHCSR位定义
#define DHCSR_DBGKEY        0xA05F0000UL
#define DHCSR_C_SDE         (1 << 20)  // 监视器调试使能
#define DHCSR_C_MASKINT     (1 << 8)   // 屏蔽调试异常
#define DHCSR_C_STEP        (1 << 2)   // 单步
#define DHCSR_C_HALT        (1 << 1)   // 暂停
#define DHCSR_C_DEBUGEN     (1 << 0)   // 调试使能

// DCRSR位定义
#define DCRSR_REGSEL        0x0000001FUL  // 寄存器选择
#define DCRSR_REGWnR        (1 << 16)      // 写操作
#define DCRSR_CM3CxLPC      (1 << 17)      // Cortex-M3 Core Register Lock

// xPSR位定义
#define xPSR_T              (1 << 24)   // Thumb状态
#define xPSR_IT             0x06000000   // IT状态
#define xPSR_Q              (1 << 27)   // 饱和标志
#define xPSR_V              (1 << 28)   // 溢出标志
#define xPSR_C              (1 << 29)   // 进位标志
#define xPSR_Z              (1 << 30)   // 零标志
#define xPSR_N              (1 << 31)   // 负数标志
```

### 5.2 Cortex-R系列

#### 5.2.1 寄存器映射

| 通用寄存器 | 别名 | 说明 |
|-----------|------|------|
| R0-R12 | - | 通用寄存器 |
| R13 | SP | 堆栈指针 (有多个BANK) |
| R14 | LR | 连接寄存器 |
| R15 | PC | 程序计数器 |
| CPSR | - | 当前程序状态寄存器 |
| SPSR | - | 保存的程序状态寄存器 |

#### 5.2.2 调试组件

```
Cortex-R调试组件：
├── Debug Control Block (DCB)
│   ├── EDESR (External Debug Event Status)
│   ├── EDAUTHSTATUS (External Debug Authentication)
│   └── EDDREVIDR (Debug ID Register)
├── Embedded Trace Macrocell (ETM)
├── Performance Monitor Unit (PMU)
├── Cross Trigger Interface (CTI)
└── Breakpoint Unit (BRP)
```

### 5.3 Cortex-A系列

#### 5.3.1 寄存器映射

| 通用寄存器 | 说明 |
|-----------|------|
| X0-X30 | 64位通用寄存器 (兼容32位W0-W30) |
| SP | 堆栈指针 |
| PC | 程序计数器 |
| CPSR | 当前程序状态寄存器 |
| SPSR | 保存的程序状态寄存器 |

#### 5.3.2 安全状态

```
Cortex-A支持多种安全状态：
├── Secure State (安全态)
│   ├── Monitor Mode
│   ├── Secure User Mode
│   └── Secure Privileged Mode
└── Non-Secure State (非安全态)
    ├── User Mode
    └── Privileged Mode
```

#### 5.3.3 调试组件

```
Cortex-A调试组件：
├── External Debug
│   ├── EDPRSR (Debug Status)
│   ├── EDRCR (Debug Run Control)
│   └── EDLAR (Auth Register)
├── In Circuit Emulation (ICE)
│   ├── Management Registers
│   └── Watchpoint Registers
├── Performance Monitoring Unit (PMU)
├── Trace Macrocell (ETM/PTM)
└── Cross Trigger Interface (CTI)
```

## 6. 内核检测流程

### 6.1 通用检测流程

```c
HAL_StatusTypeDef Core_Detect(Core_Info_TypeDef *info)
{
    // 1. 读取CPU ID
    uint32_t cpuid = Core_ReadCPUID();

    // 2. 解析CPU ID
    uint32_t implementor = (cpuid >> 24) & 0x7F;
    uint32_t variant = (cpuid >> 20) & 0x0F;
    uint32_t architecture = (cpuid >> 16) & 0x0F;
    uint32_t partno = (cpuid >> 4) & 0xFFF;
    uint32_t revision = cpuid & 0x0F;

    // 3. 识别内核类型
    if (architecture == 0xC) {
        // Cortex-M系列
        if (partno == 0xC60) {
            info->type = CORE_TYPE_CORTEX_M0;
        } else if (partno == 0xC61) {
            info->type = CORE_TYPE_CORTEX_M0P;
        } else if (partno == 0xC23) {
            info->type = CORE_TYPE_CORTEX_M3;
        } else if (partno == 0xC24) {
            info->type = CORE_TYPE_CORTEX_M4;
        } else if (partno == 0xC27) {
            info->type = CORE_TYPE_CORTEX_M7;
        }
        // ...
    } else if (architecture == 0xF) {
        // Cortex-A系列
        if (partno == 0xC07) {
            info->type = CORE_TYPE_CORTEX_A7;
        } else if (partno == 0xC09) {
            info->type = CORE_TYPE_CORTEX_A9;
        } else if (partno == 0xC0D) {
            info->type = CORE_TYPE_CORTEX_A53;
        } else if (partno == 0xC0F) {
            info->type = CORE_TYPE_CORTEX_A72;
        }
        // ...
    }

    return HAL_OK;
}
```

### 6.2 内核特定初始化

```c
HAL_StatusTypeDef Core_M0_Init(void)
{
    // Cortex-M0/M0+ 特殊初始化
    // ...
    return HAL_OK;
}

HAL_StatusTypeDef Core_M3_Init(void)
{
    // Cortex-M3 特殊初始化
    // ...
    return HAL_OK;
}

HAL_StatusTypeDef Core_M4_Init(void)
{
    // Cortex-M4 特殊初始化 (检查FPU)
    uint32_t cpuid = Core_ReadCPUID();
    uint32_t partno = (cpuid >> 4) & 0xFFF;

    if (partno == 0xC24) {
        // 检查CPACR中的FPU位
        uint32_t cpacr = Core_ReadReg(16);  // CPACR
        if (cpacr & (0xF << 20)) {
            // FPU存在
        }
    }
    return HAL_OK;
}

HAL_StatusTypeDef Core_A7_Init(void)
{
    // Cortex-A7 特殊初始化
    // 需要配置MMU、缓存等
    // ...
    return HAL_OK;
}
```

## 7. 寄存器访问实现

### 7.1 Cortex-M寄存器访问

```c
// Cortex-M使用CoreDebug接口访问寄存器
uint32_t Core_M_ReadRegister(uint8_t reg_index)
{
    // 等待上一次操作完成
    while (CoreDebug_DCRSR & (1 << 16));

    // 选择寄存器
    CoreDebug_DCRSR = reg_index;

    // 触发读取
    CoreDebug_DCRSR = reg_index | DCRSR_REGWnR;

    // 等待读取完成
    while (!(CoreDebug_DHCSR & (1 << 18)));

    // 读取数据
    return CoreDebug_DCRDR;
}

HAL_StatusTypeDef Core_M_WriteRegister(uint8_t reg_index, uint32_t value)
{
    // 等待上一次操作完成
    while (CoreDebug_DCRSR & (1 << 16));

    // 写入数据
    CoreDebug_DCRDR = value;

    // 选择寄存器
    CoreDebug_DCRSR = reg_index;

    // 触发写入
    CoreDebug_DCRSR = reg_index | DCRSR_REGWnR;

    // 等待写入完成
    while (CoreDebug_DCRSR & (1 << 16));

    return HAL_OK;
}
```

### 7.2 Cortex-A寄存器访问

```c
// Cortex-A使用MSR/MRS指令访问寄存器
uint64_t Core_A_ReadRegister(uint8_t reg_index)
{
    uint64_t value;

    // 通过内存映射的寄存器访问
    uint32_t *reg_addr = (uint32_t *)COREGPIO_BASE + reg_index;
    value = *reg_addr;

    return value;
}

HAL_StatusTypeDef Core_A_WriteRegister(uint8_t reg_index, uint64_t value)
{
    uint32_t *reg_addr = (uint32_t *)COREGPIO_BASE + reg_index;
    *reg_addr = (uint32_t)(value & 0xFFFFFFFF);

    if (reg_index >= 32) {
        // 64位寄存器的上半部分
        * (reg_addr + 1) = (uint32_t)((value >> 32) & 0xFFFFFFFF);
    }

    return HAL_OK;
}
```

## 8. 断点管理

### 8.1 Cortex-M断点

```c
// Cortex-M使用FPB (Flash Patch and Breakpoint) 单元
typedef struct {
    uint32_t enabled : 1;
    uint32_t reserved : 1;
    uint32_t match : 2;       // 00=地址匹配, 01=低位字节匹配
    uint32_t reserved2 : 4;
    uint32_t breakpoint_num : 4;
    uint32_t reserved3 : 20;
} FP_COMP_TypeDef;

#define FPB_BASE     0xE00020000UL
#define FPB_CTRL     (*(volatile uint32_t *)(FPB_BASE + 0x00))
#define FPB_REMAP    (*(volatile uint32_t *)(FPB_BASE + 0x04))
#define FPB_COMP0    (*(volatile uint32_t *)(FPB_BASE + 0x08))

HAL_StatusTypeDef Core_M_SetBreakpoint(uint32_t addr)
{
    // 检查是否有可用的断点单元
    uint32_t num_code = (FPB_CTRL >> 8) & 0x0F;

    if (num_code == 0) {
        return HAL_ERROR;  // 没有可用的断点
    }

    // 配置断点
    FPB_COMP0 = addr | 0x01;  // 使能断点

    return HAL_OK;
}

HAL_StatusTypeDef Core_M_ClearBreakpoint(uint32_t addr)
{
    FPB_COMP0 = 0;  // 禁用断点
    return HAL_OK;
}
```

### 8.2 Cortex-A断点

```c
// Cortex-A使用多个断点单元
typedef struct {
    uint32_t enabled : 1;
    uint32_t linked_bp : 4;
    uint32_t reserved : 1;
    uint32_t byte_address_select : 16;
    uint32_t security_state : 2;
    uint32_t privileged : 1;
    uint32_t reserved2 : 1;
    uint32_t match_type : 2;
    uint32_t reserved3 : 4;
} BP_CTRL_TypeDef;

HAL_StatusTypeDef Core_A_SetBreakpoint(uint32_t addr)
{
    // 配置断点控制寄存器
    uint32_t *bp_ctrl = (uint32_t *)0x80040000;
    *bp_ctrl = addr | 0x01;  // 使能

    return HAL_OK;
}

HAL_StatusTypeDef Core_A_ClearBreakpoint(uint32_t addr)
{
    // 禁用断点
    uint32_t *bp_ctrl = (uint32_t *)0x80040000;
    *bp_ctrl = 0;

    return HAL_OK;
}
```

## 9. 状态控制

### 9.1 暂停/恢复

```c
// Cortex-M暂停/恢复
HAL_StatusTypeDef Core_M_Halt(void)
{
    CoreDebug_DHCSR = DHCSR_DBGKEY | DHCSR_C_DEBUGEN | DHCSR_C_HALT;
    return HAL_OK;
}

HAL_StatusTypeDef Core_M_Resume(void)
{
    CoreDebug_DHCSR = DHCSR_DBGKEY | DHCSR_C_DEBUGEN;
    return HAL_OK;
}

HAL_StatusTypeDef Core_M_Step(void)
{
    CoreDebug_DHCSR = DHCSR_DBGKEY | DHCSR_C_DEBUGEN | DHCSR_C_STEP;
    return HAL_OK;
}

Core_State_TypeDef Core_M_GetState(void)
{
    if (CoreDebug_DHCSR & DHCSR_C_HALT) {
        return CORE_STATE_HALTED;
    } else {
        return CORE_STATE_RUNNING;
    }
}
```

### 9.2 复位

```c
// Cortex-M复位
HAL_StatusTypeDef Core_M_Reset(void)
{
    // 通过AIRCR触发复位
    uint32_t *aircr = (uint32_t *)0xE000ED0C;

    *aircr = 0x05FA0004;  // SYSRESETREQ

    return HAL_OK;
}

// Cortex-A复位
HAL_StatusTypeDef Core_A_Reset(void)
{
    // 通过系统寄存器触发复位
    uint32_t *reset_ctrl = (uint32_t *)0x80000000;
    *reset_ctrl = 0x01;  // 触发复位

    return HAL_OK;
}
```

## 10. 支持的芯片列表

### 10.1 STM32系列

| 芯片系列 | 内核 | 状态 |
|---------|------|------|
| STM32F0 | Cortex-M0 | ⬜ |
| STM32F1 | Cortex-M3 | ⬜ |
| STM32F2 | Cortex-M3 | ⬜ |
| STM32F3 | Cortex-M4 | ⬜ |
| STM32F4 | Cortex-M4 | ⬜ |
| STM32F7 | Cortex-M7 | ⬜ |
| STM32H7 | Cortex-M7 | ⬜ |
| STM32L0 | Cortex-M0+ | ⬜ |
| STM32L1 | Cortex-M3 | ⬜ |
| STM32L4 | Cortex-M4 | ⬜ |
| STM32L5 | Cortex-M33 | ⬜ |
| STM32G0 | Cortex-M0+ | ⬜ |
| STM32G4 | Cortex-M4 | ⬜ |

### 10.2 其他ARM芯片

| 芯片系列 | 内核 | 状态 |
|---------|------|------|
| NXP LPC11xx | Cortex-M0 | ⬜ |
| NXP LPC13xx | Cortex-M3 | ⬜ |
| NXP LPC17xx | Cortex-M3 | ⬜ |
| NXP LPC43xx | Cortex-M4 | ⬜ |
| NXP i.MX6 | Cortex-A9 | ⬜ |
| NXP i.MX7 | Cortex-A7 | ⬜ |
| NXP i.MX8M | Cortex-A53 | ⬜ |
| TI Tiva C | Cortex-M4 | ⬜ |
| TI Stellaris | Cortex-M4 | ⬜ |
| TI AM335x | Cortex-A8 | ⬜ |

## 11. 测试计划

### 11.1 内核检测测试

| 测试项目 | 测试内容 | 预期结果 |
|---------|---------|----------|
| Cortex-M0检测 | 检测CPUID，识别内核 | 正确识别 |
| Cortex-M3检测 | 检测CPUID，识别内核 | 正确识别 |
| Cortex-M4检测 | 检测CPUID，识别FPU | 正确识别 |
| Cortex-M7检测 | 检测CPUID，识别FPU/DSP | 正确识别 |
| Cortex-A7检测 | 检测CPUID，识别内核 | 正确识别 |
| Cortex-A9检测 | 检测CPUID，识别多核 | 正确识别 |

### 11.2 寄存器访问测试

| 测试项目 | 测试内容 | 预期结果 |
|---------|---------|----------|
| 通用寄存器读写 | R0-R12, SP, LR, PC | 读写正确 |
| 特殊寄存器读写 | xPSR, CONTROL, PRIMASK | 读写正确 |
| 64位寄存器读写 | X0-X30 (Cortex-A) | 读写正确 |

### 11.3 调试功能测试

| 测试项目 | 测试内容 | 预期结果 |
|---------|---------|----------|
| 暂停功能 | Halt指令 | 内核暂停 |
| 恢复功能 | Resume指令 | 内核运行 |
| 单步功能 | Step指令 | 执行一条指令 |
| 断点功能 | SetBreakpoint | 在断点处暂停 |
| 复位功能 | Reset指令 | 内核复位 |

## 12. 文档版本

- 版本: v1.0
- 创建日期: 2026-06-01
- 最后更新: 2026-06-01
- 状态: 草稿
