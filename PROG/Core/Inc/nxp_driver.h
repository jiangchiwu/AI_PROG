/**
 ******************************************************************************
 * @file    nxp_driver.h
 * @brief   NXP LPC/i.MX全系列驱动头文件（对标J-Flash）
 *          支持LPC800/LPC1100/LPC1700/LPC4300/i.MX RT系列
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#ifndef __NXP_DRIVER_H__
#define __NXP_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* ==================== NXP芯片系列定义 ==================== */
typedef enum {
    NXP_FAMILY_LPC800 = 0,      /* LPC800系列(Cortex-M0+) */
    NXP_FAMILY_LPC1100,         /* LPC1100系列(Cortex-M0/M0+) */
    NXP_FAMILY_LPC1700,         /* LPC1700系列(Cortex-M3) */
    NXP_FAMILY_LPC4300,         /* LPC4300系列(Cortex-M4/M0双核) */
    NXP_FAMILY_LPC54000,        /* LPC54000系列(Cortex-M4) */
    NXP_FAMILY_IMXRT,           /* i.MX RT系列(Cortex-M7跨界) */
    NXP_FAMILY_IMX6,            /* i.MX 6系列(Cortex-A9) */
    NXP_FAMILY_IMX8,            /* i.MX 8系列(Cortex-A53/M4) */
    NXP_FAMILY_KINETIS_K,       /* Kinetis K系列(Cortex-M4) */
    NXP_FAMILY_KINETIS_L,       /* Kinetis L系列(Cortex-M0+) */
    NXP_FAMILY_S32K,            /* S32K系列(车用) */
} NXP_Family_t;

/* ==================== NXP句柄结构体 ==================== */
typedef struct {
    NXP_Family_t         family;             /* 芯片系列 */
    uint32_t             device_id;          /* 设备ID */
    uint32_t             flash_size;         /* Flash大小 */
    uint32_t             ram_size;           /* RAM大小 */
    uint32_t             flash_base;         /* Flash基地址 */
    uint32_t             sector_size;        /* 扇区大小 */
    uint32_t             page_size;          /* 页大小 */
    uint32_t             clock_hz;           /* 通信时钟 */
    char                 part_number[32];    /* 型号 */
    uint8_t              initialized;        /* 初始化标志 */
    uint8_t              dual_core;          /* 双核标志 */
} NXP_HandleTypeDef;

/* ==================== LPC Flash控制器定义 ==================== */
#define LPC_FLASH_BASE              0x00000000  /* Flash基地址 */
#define LPC_FLASH_CTRL_BASE         0x40000000  /* Flash控制器基地址 */

/* LPC800 Flash控制器 */
#define LPC8XX_FLASH_CTRL           0x40020000
#define LPC8XX_FLASH_SECTOR_SIZE    1024        /* 1KB扇区 */
#define LPC8XX_FLASH_PAGE_SIZE      64          /* 64B页 */

/* LPC1700 Flash控制器 */
#define LPC17XX_FLASH_CTRL          0x4003C000
#define LPC17XX_FLASH_SECTOR_SIZE   4096        /* 4KB扇区 */

/* LPC4300 Flash控制器 */
#define LPC43XX_FLASH_CTRL_A        0x40050000  /* Flash Bank A控制器 */
#define LPC43XX_FLASH_CTRL_B        0x40051000  /* Flash Bank B控制器 */

/* i.MX RT FlexSPI控制器 */
#define IMXRT_FLEXSPI_BASE          0x402A8000
#define IMXRT_SECTOR_SIZE           4096        /* 4KB扇区 */

/* ==================== 函数声明 ==================== */

/* 初始化 */
HAL_StatusTypeDef NXP_Init(NXP_HandleTypeDef* hnxp);
HAL_StatusTypeDef NXP_DeInit(NXP_HandleTypeDef* hnxp);

/* 检测 */
HAL_StatusTypeDef NXP_Detect(NXP_HandleTypeDef* hnxp);

/* Flash操作 */
HAL_StatusTypeDef NXP_EraseFlash(NXP_HandleTypeDef* hnxp, uint32_t addr, uint32_t size);
HAL_StatusTypeDef NXP_ProgramFlash(NXP_HandleTypeDef* hnxp, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef NXP_ReadFlash(NXP_HandleTypeDef* hnxp, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef NXP_VerifyFlash(NXP_HandleTypeDef* hnxp, uint32_t addr, uint8_t* data, uint32_t size);

/* LPC系列专用操作 */
HAL_StatusTypeDef NXP_LPC800_InitFlash(NXP_HandleTypeDef* hnxp);
HAL_StatusTypeDef NXP_LPC1700_InitFlash(NXP_HandleTypeDef* hnxp);
HAL_StatusTypeDef NXP_LPC4300_InitFlash(NXP_HandleTypeDef* hnxp);

/* i.MX RT专用操作 */
HAL_StatusTypeDef NXP_IMXRT_InitFlexSPI(NXP_HandleTypeDef* hnxp);
HAL_StatusTypeDef NXP_IMXRT_ConfigFlash(NXP_HandleTypeDef* hnxp);
HAL_StatusTypeDef NXP_IMXRT_FlexSPICommand(NXP_HandleTypeDef* hnxp, uint8_t cmd, uint32_t addr, uint8_t* data, uint32_t len);

/* Kinetis专用操作 */
HAL_StatusTypeDef NXP_Kinetis_InitFlash(NXP_HandleTypeDef* hnxp);

/* 速度设置 */
HAL_StatusTypeDef NXP_SetSpeed(NXP_HandleTypeDef* hnxp, uint32_t clock_hz);

/* ID读取 */
uint32_t NXP_ReadDeviceID(NXP_HandleTypeDef* hnxp);

#ifdef __cplusplus
}
#endif

#endif /* __NXP_DRIVER_H__ */