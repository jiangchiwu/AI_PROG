/**
 ******************************************************************************
 * @file    renesas_driver.h
 * @brief   Renesas瑞萨全系列驱动头文件（对标RFP6）
 *          支持RL78/RA/RH850/V850/R8C/M16C/78K系列
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#ifndef __RENESAS_DRIVER_H__
#define __RENESAS_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* ==================== Renesas芯片系列定义 ==================== */
typedef enum {
    RENESAS_RL78 = 0,       /* RL78系列(低功耗16位) */
    RENESAS_RA,             /* RA系列(ARM Cortex-M) */
    RENESAS_RH850,          /* RH850系列(车用32位) */
    RENESAS_V850,           /* V850系列(32位) */
    RENESAS_R8C,            /* R8C系列(16位) */
    RENESAS_M16C,           /* M16C系列(16位) */
    RENESAS_78K0,           /* 78K0系列(8位) */
    RENESAS_78K0R,          /* 78K0R系列(8位) */
    RENESAS_H8,             /* H8系列(8/16/32位) */
} Renesas_Family_t;

/* ==================== 调试接口类型 ==================== */
typedef enum {
    RENESAS_DEBUG_FINE = 0,     /* FINE接口(RL78/RH850) */
    RENESAS_DEBUG_JTAG,         /* JTAG接口(RA/RH850) */
    RENESAS_DEBUG_SWD,          /* SWD接口(RA系列) */
    RENESAS_DEBUG_E1,           /* E1仿真器接口 */
    RENESAS_DEBUG_UART,         /* UART Bootloader */
} Renesas_Debug_Type_t;

/* ==================== Renesas句柄结构体 ==================== */
typedef struct {
    Renesas_Family_t    family;         /* 芯片系列 */
    Renesas_Debug_Type_t debug_type;    /* 调试接口类型 */
    uint32_t            device_code;    /* 设备代码 */
    uint32_t            flash_size;     /* Flash大小 */
    uint32_t            ram_size;       /* RAM大小 */
    uint32_t            flash_base;     /* Flash基地址 */
    uint32_t            data_flash_base;/* Data Flash基地址 */
    uint32_t            data_flash_size;/* Data Flash大小 */
    uint32_t            clock_hz;       /* 通信时钟 */
    char                part_number[32];/* 型号 */
    uint8_t             initialized;    /* 初始化标志 */
    
    /* FINE接口引脚 */
    GPIO_TypeDef*       fine_clk_port;
    uint16_t            fine_clk_pin;
    GPIO_TypeDef*       fine_data_port;
    uint16_t            fine_data_pin;
    GPIO_TypeDef*       fine_reset_port;
    uint16_t            fine_reset_pin;
    
    /* JTAG/SWD接口引脚 */
    GPIO_TypeDef*       tck_port;
    uint16_t            tck_pin;
    GPIO_TypeDef*       tms_port;
    uint16_t            tms_pin;
    GPIO_TypeDef*       tdi_port;
    uint16_t            tdi_pin;
    GPIO_TypeDef*       tdo_port;
    uint16_t            tdo_pin;
    
    /* UART接口 */
    UART_HandleTypeDef* huart;
} Renesas_HandleTypeDef;

/* ==================== RL78 Flash命令定义 ==================== */
#define RL78_CMD_ENTER_CODE         0x00    /* 进入编程模式命令代码 */
#define RL78_CMD_ERASE_1            0x20    /* 块擦除命令1 */
#define RL78_CMD_ERASE_2            0xD0    /* 块擦除命令2 */
#define RL78_CMD_WRITE              0x40    /* 编程命令 */
#define RL78_CMD_BLANK_CHECK        0x71    /* 空白检查命令 */
#define RL78_CMD_STATUS_READ        0x70    /* 状态读取命令 */

/* RL78 Flash状态位 */
#define RL78_SR_READY               0x80    /* 就绪 */
#define RL78_SR_ERASE_SUSPEND       0x40    /* 擦除挂起 */
#define RL78_SR_PROGRAM_ERROR       0x10    /* 编程错误 */
#define RL78_SR_ERASE_ERROR         0x08    /* 擦除错误 */
#define RL78_SR_BLANK               0x04    /* 空白 */

/* ==================== RA系列定义 ==================== */
#define RA_FLASH_BASE               0x00000000  /* Code Flash基地址 */
#define RA_DATA_FLASH_BASE          0x08000000  /* Data Flash基地址 */

/* ==================== 函数声明 ==================== */

/* 初始化 */
HAL_StatusTypeDef Renesas_Init(Renesas_HandleTypeDef* hren);
HAL_StatusTypeDef Renesas_DeInit(Renesas_HandleTypeDef* hren);

/* 检测 */
HAL_StatusTypeDef Renesas_Detect(Renesas_HandleTypeDef* hren);

/* Code Flash操作 */
HAL_StatusTypeDef Renesas_EraseFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint32_t size);
HAL_StatusTypeDef Renesas_ProgramFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef Renesas_ReadFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef Renesas_VerifyFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size);

/* Data Flash操作 */
HAL_StatusTypeDef Renesas_EraseDataFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint32_t size);
HAL_StatusTypeDef Renesas_ProgramDataFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef Renesas_ReadDataFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size);

/* RL78专用操作 */
HAL_StatusTypeDef Renesas_RL78_EnterProgramming(Renesas_HandleTypeDef* hren);
HAL_StatusTypeDef Renesas_RL78_ExitProgramming(Renesas_HandleTypeDef* hren);
HAL_StatusTypeDef Renesas_RL78_BlankCheck(Renesas_HandleTypeDef* hren, uint32_t addr, uint32_t size);

/* RA系列专用操作 */
HAL_StatusTypeDef Renesas_RA_InitFlash(Renesas_HandleTypeDef* hren);
HAL_StatusTypeDef Renesas_RA_ConfigFlash(Renesas_HandleTypeDef* hren);

/* RH850专用操作 */
HAL_StatusTypeDef Renesas_RH850_EnterDebug(Renesas_HandleTypeDef* hren);
HAL_StatusTypeDef Renesas_RH850_ReadMemory(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef Renesas_RH850_WriteMemory(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size);

/* 速度设置 */
HAL_StatusTypeDef Renesas_SetSpeed(Renesas_HandleTypeDef* hren, uint32_t clock_hz);

/* ID读取 */
uint32_t Renesas_ReadDeviceID(Renesas_HandleTypeDef* hren);

#ifdef __cplusplus
}
#endif

#endif /* __RENESAS_DRIVER_H__ */