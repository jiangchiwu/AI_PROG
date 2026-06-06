/**
 ******************************************************************************
 * @file    debug_if.h
 * @brief   统一调试接口配置文件
 *          集成所有特殊调试接口(SBW/BDM/MON8/FINE)
 *          提供统一的初始化和操作接口
 ******************************************************************************
 */

#ifndef __DEBUG_IF_H__
#define __DEBUG_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "sbw.h"
#include "bdm.h"
#include "mon8.h"
#include "fine.h"
#include "swd.h"

typedef enum {
    DEBUG_IF_NONE = 0x00,
    DEBUG_IF_SWD,      // ARM SWD接口
    DEBUG_IF_SBW,      // TI MSP430 SBW接口
    DEBUG_IF_BDM,      // NXP/Freescale BDM接口
    DEBUG_IF_MON8,     // Freescale HC08 MON8接口
    DEBUG_IF_FINE,     // Renesas FINE接口
    DEBUG_IF_JTAG,     // JTAG接口
} Debug_IF_TypeDef;

typedef enum {
    DEBUG_CHIP_NONE = 0,
    DEBUG_CHIP_MSP430,
    DEBUG_CHIP_HC08,
    DEBUG_CHIP_HCS08,
    DEBUG_CHIP_HCS12,
    DEBUG_CHIP_RENESAS_78K,
    DEBUG_CHIP_RENESAS_V850,
    DEBUG_CHIP_RENESAS_RH850,
} Debug_Chip_TypeDef;

typedef struct {
    Debug_IF_TypeDef interface_type;
    uint32_t speed_hz;
    uint8_t initialized;
} Debug_IF_Config_TypeDef;

typedef struct {
    Debug_Chip_TypeDef chip_type;
    uint32_t chip_id;
    uint16_t jtag_id;
    uint8_t version;
    uint8_t status;
} Debug_Chip_Info_TypeDef;

extern Debug_IF_Config_TypeDef g_debug_if_config;
extern Debug_Chip_Info_TypeDef g_debug_chip_info;

HAL_StatusTypeDef Debug_IF_Init(Debug_IF_TypeDef if_type, uint32_t speed_hz);
HAL_StatusTypeDef Debug_IF_DeInit(void);

HAL_StatusTypeDef Debug_IF_Enter(void);
HAL_StatusTypeDef Debug_IF_Exit(void);
HAL_StatusTypeDef Debug_IF_Reset(void);

HAL_StatusTypeDef Debug_IF_ReadMem(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef Debug_IF_WriteMem(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef Debug_IF_Erase(uint32_t addr, uint32_t size);
HAL_StatusTypeDef Debug_IF_EraseChip(void);

uint32_t Debug_IF_GetChipID(void);
uint8_t Debug_IF_GetVersion(void);

Debug_IF_TypeDef Debug_IF_AutoDetect(void);
Debug_Chip_TypeDef Debug_Chip_AutoDetect(void);

const char* Debug_IF_GetName(Debug_IF_TypeDef if_type);
const char* Debug_Chip_GetName(Debug_Chip_TypeDef chip_type);

void Debug_IF_SetSpeed(uint32_t speed_hz);
uint32_t Debug_IF_GetSpeed(void);

#ifdef __cplusplus
}
#endif

#endif /* __DEBUG_IF_H__ */
