/**
 ******************************************************************************
 * @file    debug_if.c
 * @brief   统一调试接口配置文件
 *          集成所有特殊调试接口(SBW/BDM/MON8/FINE)
 *          提供统一的初始化和操作接口
 ******************************************************************************
 */

#include "debug_if.h"
#include "gpio.h"

Debug_IF_Config_TypeDef g_debug_if_config = {0};
Debug_Chip_Info_TypeDef g_debug_chip_info = {0};

static const char* g_debug_if_names[] = {
    "NONE",
    "SWD",
    "SBW",
    "BDM",
    "MON8",
    "FINE",
    "JTAG"
};

static const char* g_debug_chip_names[] = {
    "NONE",
    "MSP430",
    "HC08",
    "HC08",
    "HCS08",
    "HCS12",
    "78K",
    "V850",
    "RH850"
};

const char* Debug_IF_GetName(Debug_IF_TypeDef if_type)
{
    if (if_type <= DEBUG_IF_JTAG) {
        return g_debug_if_names[if_type];
    }
    return "UNKNOWN";
}

const char* Debug_Chip_GetName(Debug_Chip_TypeDef chip_type)
{
    if (chip_type <= DEBUG_CHIP_RENESAS_RH850) {
        return g_debug_chip_names[chip_type];
    }
    return "UNKNOWN";
}

HAL_StatusTypeDef Debug_IF_Init(Debug_IF_TypeDef if_type, uint32_t speed_hz)
{
    g_debug_if_config.interface_type = if_type;
    g_debug_if_config.speed_hz = speed_hz;
    g_debug_if_config.initialized = 0;
    
    switch (if_type) {
        case DEBUG_IF_SWD: {
            SWD_Config_TypeDef swd_config = {
                .swdio_port = GPIOB,
                .swdio_pin = GPIO_PIN_4,
                .swclk_port = GPIOB,
                .swclk_pin = GPIO_PIN_3,
                .reset_port = GPIOB,
                .reset_pin = GPIO_PIN_5,
                .clock = speed_hz,
                .line_mode = SWD_LINE_RESET,
                .initialized = 0
            };
            if (SWD_Init(&swd_config) != HAL_OK) {
                return HAL_ERROR;
            }
            break;
        }
        
        case DEBUG_IF_SBW: {
            SBW_Config_TypeDef sbw_config = {
                .tck_port = GPIOB,
                .tck_pin = GPIO_PIN_3,
                .tms_port = GPIOB,
                .tms_pin = GPIO_PIN_4,
                .rst_port = GPIOB,
                .rst_pin = GPIO_PIN_5,
                .test_port = GPIOB,
                .test_pin = GPIO_PIN_6,
                .speed_hz = speed_hz,
                .initialized = 0
            };
            if (SBW_Init(&sbw_config) != HAL_OK) {
                return HAL_ERROR;
            }
            break;
        }
        
        case DEBUG_IF_BDM: {
            BDM_HandleTypeDef bdm_config = {
                .bkpt_port = GPIOB,
                .bkpt_pin = GPIO_PIN_4,
                .reset_port = GPIOB,
                .reset_pin = GPIO_PIN_5,
                .speed_hz = speed_hz,
                .tick_ns = 0,
                .prescaler = 0,
                .period = 0
            };
            if (BDM_Init(&bdm_config) != HAL_OK) {
                return HAL_ERROR;
            }
            break;
        }
        
        case DEBUG_IF_MON8: {
            MON8_Config_TypeDef mon8_config = {
                .bkpt_port = GPIOB,
                .bkpt_pin = GPIO_PIN_4,
                .rst_port = GPIOB,
                .rst_pin = GPIO_PIN_5,
                .ptx_port = GPIOB,
                .ptx_pin = GPIO_PIN_6,
                .prx_port = GPIOB,
                .prx_pin = GPIO_PIN_7,
                .speed_hz = speed_hz,
                .initialized = 0
            };
            if (MON8_Init(&mon8_config) != HAL_OK) {
                return HAL_ERROR;
            }
            break;
        }
        
        case DEBUG_IF_FINE: {
            FINE_Config_TypeDef fine_config = {
                .flmd0_port = GPIOB,
                .flmd0_pin = GPIO_PIN_4,
                .flmd1_port = GPIOB,
                .flmd1_pin = GPIO_PIN_5,
                .flmd2_port = GPIOB,
                .flmd2_pin = GPIO_PIN_6,
                .flmd3_port = GPIOB,
                .flmd3_pin = GPIO_PIN_7,
                .flclk_port = GPIOC,
                .flclk_pin = GPIO_PIN_0,
                .reset_port = GPIOC,
                .reset_pin = GPIO_PIN_1,
                .speed_hz = speed_hz,
                .initialized = 0
            };
            if (FINE_Init(&fine_config) != HAL_OK) {
                return HAL_ERROR;
            }
            break;
        }
        
        default:
            return HAL_ERROR;
    }
    
    g_debug_if_config.initialized = 1;
    return HAL_OK;
}

HAL_StatusTypeDef Debug_IF_DeInit(void)
{
    switch (g_debug_if_config.interface_type) {
        case DEBUG_IF_SWD:
            SWD_DeInit();
            break;
        case DEBUG_IF_SBW:
            SBW_DeInit();
            break;
        case DEBUG_IF_BDM:
            BDM_DeInit(&g_bdm_handle);
            break;
        case DEBUG_IF_MON8:
            MON8_DeInit();
            break;
        case DEBUG_IF_FINE:
            FINE_DeInit();
            break;
        default:
            break;
    }
    
    g_debug_if_config.initialized = 0;
    return HAL_OK;
}

HAL_StatusTypeDef Debug_IF_Enter(void)
{
    if (!g_debug_if_config.initialized) {
        return HAL_ERROR;
    }
    
    switch (g_debug_if_config.interface_type) {
        case DEBUG_IF_SWD:
            return SWD_Enter() == HAL_OK ? HAL_OK : HAL_ERROR;
        case DEBUG_IF_SBW:
            return SBW_Enter();
        case DEBUG_IF_BDM:
            return BDM_EnterBDM(&g_bdm_handle);
        case DEBUG_IF_MON8:
            return MON8_Enter();
        case DEBUG_IF_FINE:
            return FINE_Enter();
        default:
            return HAL_ERROR;
    }
}

HAL_StatusTypeDef Debug_IF_Exit(void)
{
    if (!g_debug_if_config.initialized) {
        return HAL_ERROR;
    }
    
    switch (g_debug_if_config.interface_type) {
        case DEBUG_IF_SWD:
            return SWD_Exit() == HAL_OK ? HAL_OK : HAL_ERROR;
        case DEBUG_IF_SBW:
            return SBW_Exit();
        case DEBUG_IF_BDM:
            return BDM_ExitBDM(&g_bdm_handle);
        case DEBUG_IF_MON8:
            return MON8_Exit();
        case DEBUG_IF_FINE:
            return FINE_Exit();
        default:
            return HAL_ERROR;
    }
}

HAL_StatusTypeDef Debug_IF_Reset(void)
{
    if (!g_debug_if_config.initialized) {
        return HAL_ERROR;
    }
    
    switch (g_debug_if_config.interface_type) {
        case DEBUG_IF_SWD:
            return SWD_Reset() == HAL_OK ? HAL_OK : HAL_ERROR;
        case DEBUG_IF_SBW:
            return SBW_Reset();
        case DEBUG_IF_MON8:
            return MON8_Reset();
        case DEBUG_IF_FINE:
            return FINE_Reset();
        default:
            return HAL_ERROR;
    }
}

HAL_StatusTypeDef Debug_IF_ReadMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (!g_debug_if_config.initialized) {
        return HAL_ERROR;
    }
    
    switch (g_debug_if_config.interface_type) {
        case DEBUG_IF_SBW:
            return SBW_ReadMem(addr, data, size);
        case DEBUG_IF_BDM:
            return BDM_ReadMem(&g_bdm_handle, addr, data, (uint16_t)size);
        case DEBUG_IF_MON8:
            return MON8_ReadMem((uint16_t)addr, data, (uint16_t)size);
        case DEBUG_IF_FINE:
            return FINE_ReadMem(addr, data, size);
        default:
            return HAL_ERROR;
    }
}

HAL_StatusTypeDef Debug_IF_WriteMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (!g_debug_if_config.initialized) {
        return HAL_ERROR;
    }
    
    switch (g_debug_if_config.interface_type) {
        case DEBUG_IF_SBW:
            return SBW_WriteMem(addr, data, size);
        case DEBUG_IF_BDM:
            return BDM_WriteMem(&g_bdm_handle, addr, data, (uint16_t)size);
        case DEBUG_IF_MON8:
            return MON8_WriteMem((uint16_t)addr, data, (uint16_t)size);
        case DEBUG_IF_FINE:
            return FINE_WriteMem(addr, data, size);
        default:
            return HAL_ERROR;
    }
}

HAL_StatusTypeDef Debug_IF_Erase(uint32_t addr, uint32_t size)
{
    if (!g_debug_if_config.initialized) {
        return HAL_ERROR;
    }
    
    switch (g_debug_if_config.interface_type) {
        case DEBUG_IF_MON8:
            return MON8_Erase((uint16_t)addr, (uint16_t)size);
        case DEBUG_IF_FINE:
            return FINE_EraseSector(addr);
        default:
            return HAL_ERROR;
    }
}

HAL_StatusTypeDef Debug_IF_EraseChip(void)
{
    if (!g_debug_if_config.initialized) {
        return HAL_ERROR;
    }
    
    switch (g_debug_if_config.interface_type) {
        case DEBUG_IF_FINE:
            return FINE_EraseChip();
        default:
            return HAL_ERROR;
    }
}

uint32_t Debug_IF_GetChipID(void)
{
    if (!g_debug_if_config.initialized) {
        return 0;
    }
    
    switch (g_debug_if_config.interface_type) {
        case DEBUG_IF_SBW:
            g_debug_chip_info.chip_id = SBW_GetIDCode();
            break;
        case DEBUG_IF_MON8:
            g_debug_chip_info.chip_id = MON8_GetDeviceID();
            break;
        case DEBUG_IF_FINE:
            g_debug_chip_info.chip_id = FINE_GetChipID();
            break;
        default:
            g_debug_chip_info.chip_id = 0;
            break;
    }
    
    return g_debug_chip_info.chip_id;
}

uint8_t Debug_IF_GetVersion(void)
{
    if (!g_debug_if_config.initialized) {
        return 0;
    }
    
    switch (g_debug_if_config.interface_type) {
        case DEBUG_IF_MON8:
            g_debug_chip_info.version = MON8_GetVersion();
            break;
        default:
            g_debug_chip_info.version = 0;
            break;
    }
    
    return g_debug_chip_info.version;
}

Debug_IF_TypeDef Debug_IF_AutoDetect(void)
{
    return DEBUG_IF_SWD;
}

Debug_Chip_TypeDef Debug_Chip_AutoDetect(void)
{
    return DEBUG_CHIP_NONE;
}

void Debug_IF_SetSpeed(uint32_t speed_hz)
{
    g_debug_if_config.speed_hz = speed_hz;
    
    switch (g_debug_if_config.interface_type) {
        case DEBUG_IF_SBW:
            SBW_SetSpeed(speed_hz);
            break;
        case DEBUG_IF_BDM:
            BDM_SetSpeed(&g_bdm_handle, speed_hz);
            break;
        case DEBUG_IF_MON8:
            MON8_SetSpeed(speed_hz);
            break;
        case DEBUG_IF_FINE:
            FINE_SetSpeed(speed_hz);
            break;
        default:
            break;
    }
}

uint32_t Debug_IF_GetSpeed(void)
{
    if (!g_debug_if_config.initialized) {
        return 0;
    }
    
    switch (g_debug_if_config.interface_type) {
        case DEBUG_IF_SBW:
            g_debug_if_config.speed_hz = SBW_GetSpeed();
            break;
        case DEBUG_IF_BDM:
            g_debug_if_config.speed_hz = BDM_GetSpeed(&g_bdm_handle);
            break;
        case DEBUG_IF_MON8:
            g_debug_if_config.speed_hz = MON8_GetSpeed();
            break;
        case DEBUG_IF_FINE:
            g_debug_if_config.speed_hz = FINE_GetSpeed();
            break;
        default:
            break;
    }
    
    return g_debug_if_config.speed_hz;
}
