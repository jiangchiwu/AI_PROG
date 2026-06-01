/**
 ******************************************************************************
 * @file    core_cal.h
 * @brief   ARM 内核抽象层 (Core Abstraction Layer)
 ******************************************************************************
 */

#ifndef __CORE_CAL_H__
#define __CORE_CAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

// 内核类型枚举
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

// 内核状态枚举
typedef enum {
    CORE_STATE_HALTED,
    CORE_STATE_RUNNING,
    CORE_STATE_RESET,
    CORE_STATE_UNKNOWN,
} Core_State_TypeDef;

// 内核信息结构
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

// 内核操作接口
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
    HAL_StatusTypeDef (*read_memory)(uint32_t addr, uint8_t *data, uint32_t size);
    HAL_StatusTypeDef (*write_memory)(uint32_t addr, uint8_t *data, uint32_t size);
} Core_Ops_TypeDef;

// CoreDebug寄存器地址定义（Cortex-M，通过DAP访问目标芯片）
#define COREDEBUG_DHCSR_ADDR  0xE000EDF0UL
#define COREDEBUG_DCRDR_ADDR  0xE000EDF4UL
#define COREDEBUG_DCRSR_ADDR  0xE000EDF8UL
#define COREDEBUG_DEMCR_ADDR  0xE000EDFCUL

// DHCSR位定义
#define DHCSR_DBGKEY        0xA05F0000UL
#define DHCSR_C_SDE         (1 << 20)
#define DHCSR_C_MASKINT     (1 << 8)
#define DHCSR_C_STEP        (1 << 2)
#define DHCSR_C_HALT        (1 << 1)
#define DHCSR_C_DEBUGEN     (1 << 0)
#define DHCSR_S_REGRDY      (1 << 16)
#define DHCSR_S_RESET_ST    (1 << 25)
#define DHCSR_S_RETIRE_ST   (1 << 24)
#define DHCSR_S_LOCKUP      (1 << 19)
#define DHCSR_S_SLEEP       (1 << 18)
#define DHCSR_S_HALT        (1 << 17)

// DCRSR位定义
#define DCRSR_REGSEL        0x0000001FUL
#define DCRSR_REGWnR        (1 << 16)

// CPUID地址
#define CPUID_ADDR          0xE000ED00UL

// SCB寄存器地址
#define SCB_AIRCR_ADDR      0xE000ED0CUL
#define SCB_AIRCR_VECTKEY   0x05FA0000UL
#define SCB_AIRCR_SYSRESETREQ (1 << 2)

// 寄存器索引（Cortex-M）
#define CORE_REG_R0         0
#define CORE_REG_R1         1
#define CORE_REG_R2         2
#define CORE_REG_R3         3
#define CORE_REG_R4         4
#define CORE_REG_R5         5
#define CORE_REG_R6         6
#define CORE_REG_R7         7
#define CORE_REG_R8         8
#define CORE_REG_R9         9
#define CORE_REG_R10        10
#define CORE_REG_R11        11
#define CORE_REG_R12        12
#define CORE_REG_SP         13
#define CORE_REG_LR         14
#define CORE_REG_PC         15
#define CORE_REG_XPSR       16
#define CORE_REG_MSP        17
#define CORE_REG_PSP        18
#define CORE_REG_PRIMASK    20
#define CORE_REG_CONTROL    20

// 函数声明
HAL_StatusTypeDef Core_Init(void);
HAL_StatusTypeDef Core_DeInit(void);
HAL_StatusTypeDef Core_Detect(Core_Info_TypeDef *info);

HAL_StatusTypeDef Core_Reset(void);
Core_State_TypeDef Core_GetState(void);
HAL_StatusTypeDef Core_Halt(void);
HAL_StatusTypeDef Core_Resume(void);
HAL_StatusTypeDef Core_Step(void);

uint32_t Core_GetPC(void);
HAL_StatusTypeDef Core_SetPC(uint32_t pc);
uint32_t Core_GetRegister(uint8_t reg_index);
HAL_StatusTypeDef Core_SetRegister(uint8_t reg_index, uint32_t value);

HAL_StatusTypeDef Core_ReadMemory(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef Core_WriteMemory(uint32_t addr, uint8_t *data, uint32_t size);

uint32_t Core_ReadCPUID(void);

extern Core_Info_TypeDef g_core_info;
extern Core_Ops_TypeDef g_core_ops;

#ifdef __cplusplus
}
#endif

#endif
