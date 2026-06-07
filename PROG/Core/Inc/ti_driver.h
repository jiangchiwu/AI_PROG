/**
 ******************************************************************************
 * @file    ti_driver.h
 * @brief   TI德州仪器全系列驱动头文件（对标UniFlash）
 *          支持MSP430/MSP432/CC253x/CC26xx/TMS320/TMS570/TM470系列
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#ifndef __TI_DRIVER_H__
#define __TI_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* ==================== TI芯片系列定义 ==================== */
typedef enum {
    TI_FAMILY_MSP430 = 0,       /* MSP430系列(16位超低功耗) */
    TI_FAMILY_MSP432,           /* MSP432系列(ARM Cortex-M4) */
    TI_FAMILY_CC253X,           /* CC253x系列(ZigBee SoC) */
    TI_FAMILY_CC26XX,           /* CC26xx系列(无线MCU) */
    TI_FAMILY_CC13XX,           /* CC13xx系列(Sub-1GHz) */
    TI_FAMILY_TMS320C2000,      /* TMS320C2000系列(DSP+MCU) */
    TI_FAMILY_TMS320C6000,      /* TMS320C6000系列(DSP) */
    TI_FAMILY_TMS570,           /* TMS570系列(车用安全MCU) */
    TI_FAMILY_TM470,            /* TM470系列(车用MCU) */
    TI_FAMILY_CC32XX,           /* CC32xx系列(WiFi MCU) */
} TI_Family_t;

/* ==================== TI调试接口类型 ==================== */
typedef enum {
    TI_DEBUG_SBW = 0,           /* Spy-Bi-Wire (MSP430) */
    TI_DEBUG_JTAG,              /* JTAG (CC253x/TMS320) */
    TI_DEBUG_SWD,               /* SWD (MSP432) */
    TI_DEBUG_UART,              /* UART Bootloader */
} TI_Debug_Type_t;

/* ==================== TI句柄结构体 ==================== */
typedef struct {
    TI_Family_t         family;             /* 芯片系列 */
    TI_Debug_Type_t     debug_type;         /* 调试接口类型 */
    uint32_t            device_id;          /* 设备ID */
    uint32_t            flash_size;         /* Flash大小 */
    uint32_t            ram_size;           /* RAM大小 */
    uint32_t            flash_base;         /* Flash基地址 */
    uint32_t            info_flash_base;    /* Info Flash基地址 */
    uint32_t            info_flash_size;    /* Info Flash大小 */
    uint32_t            clock_hz;           /* 通信时钟 */
    char                part_number[32];    /* 型号 */
    uint8_t             initialized;        /* 初始化标志 */
    
    /* SBW接口引脚 */
    GPIO_TypeDef*       sbw_tck_port;
    uint16_t            sbw_tck_pin;
    GPIO_TypeDef*       sbw_tms_port;
    uint16_t            sbw_tms_pin;
    GPIO_TypeDef*       sbw_reset_port;
    uint16_t            sbw_reset_pin;
    
    /* JTAG/SWD接口 */
    GPIO_TypeDef*       tck_port;
    uint16_t            tck_pin;
    GPIO_TypeDef*       tms_port;
    uint16_t            tms_pin;
    GPIO_TypeDef*       tdi_port;
    uint16_t            tdi_pin;
    GPIO_TypeDef*       tdo_port;
    uint16_t            tdo_pin;
} TI_HandleTypeDef;

/* ==================== MSP430 Flash控制器定义 ==================== */
#define MSP430_FLASH_BASE           0x00000400  /* Flash控制器基地址 */
#define MSP430_FLASH_CTL1           0x00000400  /* Flash控制寄存器1 */
#define MSP430_FLASH_CTL2           0x00000404  /* Flash控制寄存器2 */
#define MSP430_FLASH_BUSY           0x0001      /* Flash忙标志 */
#define MSP430_FLASH_KEY            0xA5A5      /* Flash解锁密钥 */

/* ==================== TMS320 C2000 Flash定义 ==================== */
#define TMS320C2000_FLASH_BASE      0x00080000  /* Flash基地址 */
#define TMS320C2000_FLASH_CTRL      0x000AE000  /* Flash控制器基地址 */
#define TMS320C2000_FLASH_SECTOR    0x2000      /* Flash扇区大小(8KB) */

/* ==================== 函数声明 ==================== */

/* 初始化 */
HAL_StatusTypeDef TI_Init(TI_HandleTypeDef* hti);
HAL_StatusTypeDef TI_DeInit(TI_HandleTypeDef* hti);

/* 检测 */
HAL_StatusTypeDef TI_Detect(TI_HandleTypeDef* hti);

/* Flash操作 */
HAL_StatusTypeDef TI_EraseFlash(TI_HandleTypeDef* hti, uint32_t addr, uint32_t size);
HAL_StatusTypeDef TI_ProgramFlash(TI_HandleTypeDef* hti, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef TI_ReadFlash(TI_HandleTypeDef* hti, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef TI_VerifyFlash(TI_HandleTypeDef* hti, uint32_t addr, uint8_t* data, uint32_t size);

/* MSP430专用操作 */
HAL_StatusTypeDef TI_MSP430_EnterSBW(TI_HandleTypeDef* hti);
HAL_StatusTypeDef TI_MSP430_EraseSegment(TI_HandleTypeDef* hti, uint32_t addr);
HAL_StatusTypeDef TI_MSP430_WriteFlash(TI_HandleTypeDef* hti, uint32_t addr, uint16_t data);
HAL_StatusTypeDef TI_MSP430_ReadInfoFlash(TI_HandleTypeDef* hti, uint32_t addr, uint8_t* data, uint32_t size);

/* MSP432专用操作 */
HAL_StatusTypeDef TI_MSP432_InitFlash(TI_HandleTypeDef* hti);
HAL_StatusTypeDef TI_MSP432_EraseSector(TI_HandleTypeDef* hti, uint32_t addr);

/* CC253x专用操作 */
HAL_StatusTypeDef TI_CC253x_InitFlash(TI_HandleTypeDef* hti);
HAL_StatusTypeDef TI_CC253x_WriteFlash(TI_HandleTypeDef* hti, uint32_t addr, uint8_t* data, uint32_t size);

/* TMS320 C2000专用操作 */
HAL_StatusTypeDef TI_TMS320C2000_InitFlash(TI_HandleTypeDef* hti);
HAL_StatusTypeDef TI_TMS320C2000_EraseSector(TI_HandleTypeDef* hti, uint32_t sector);
HAL_StatusTypeDef TI_TMS320C2000_ProgramFlash(TI_HandleTypeDef* hti, uint32_t addr, uint16_t* data, uint32_t count);

/* TMS570/TM470专用操作 */
HAL_StatusTypeDef TI_TMS570_InitFlash(TI_HandleTypeDef* hti);
HAL_StatusTypeDef TI_TMS570_EraseBank(TI_HandleTypeDef* hti, uint32_t bank);

/* 速度设置 */
HAL_StatusTypeDef TI_SetSpeed(TI_HandleTypeDef* hti, uint32_t clock_hz);

/* ID读取 */
uint32_t TI_ReadDeviceID(TI_HandleTypeDef* hti);
uint16_t TI_MSP430_ReadDeviceID(TI_HandleTypeDef* hti);

#ifdef __cplusplus
}
#endif

#endif /* __TI_DRIVER_H__ */