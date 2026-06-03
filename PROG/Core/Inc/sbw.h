/**
 ******************************************************************************
 * @file    sbw.h
 * @brief   SBW (Spy-Bi-Wire) 协议实现
 *          MSP430 调试接口 - 两线JTAG替代方案
 ******************************************************************************
 */

#ifndef __SBW_H__
#define __SBW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

#define SBW_OK              0x00
#define SBW_ERR             0x01
#define SBW_ERR_FAULT       0x02
#define SBW_ERR_TIMEOUT     0x03
#define SBW_ERR_PARITY      0x04
#define SBW_ERR_NO_ACK      0x05

#define SBW_CLOCK_100KHZ    100000
#define SBW_CLOCK_200KHZ    200000
#define SBW_CLOCK_400KHZ    400000
#define SBW_CLOCK_1MHZ      1000000

#define SBW_DEFAULT_CLOCK   SBW_CLOCK_400KHZ

typedef struct {
    GPIO_TypeDef *tck_port;
    uint16_t tck_pin;
    GPIO_TypeDef *tms_port;
    uint16_t tms_pin;
    GPIO_TypeDef *rst_port;
    uint16_t rst_pin;
    GPIO_TypeDef *test_port;
    uint16_t test_pin;

    uint32_t clock;
    uint8_t initialized;
} SBW_Config_TypeDef;

typedef struct {
    uint32_t idcode;
    uint16_t jtag_id;
    uint8_t cpu_type;
} SBW_State_TypeDef;

extern SBW_Config_TypeDef g_sbw_config;
extern SBW_State_TypeDef g_sbw_state;

HAL_StatusTypeDef SBW_Init(SBW_Config_TypeDef *config);
HAL_StatusTypeDef SBW_DeInit(void);

HAL_StatusTypeDef SBW_Enter(void);
HAL_StatusTypeDef SBW_Exit(void);

HAL_StatusTypeDef SBW_Reset(void);
HAL_StatusTypeDef SBW_Start(void);
HAL_StatusTypeDef SBW_Stop(void);

HAL_StatusTypeDef SBW_TapReset(void);
HAL_StatusTypeDef SBW_TapShiftIR(uint16_t instruction);
HAL_StatusTypeDef SBW_TapShiftDR(uint16_t *data, uint32_t bitlen);
uint16_t SBW_TapReadDR(uint32_t bitlen);

HAL_StatusTypeDef SBW_WriteWord(uint32_t addr, uint16_t data);
uint16_t SBW_ReadWord(uint32_t addr);

HAL_StatusTypeDef SBW_WriteMem(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef SBW_ReadMem(uint32_t addr, uint8_t *data, uint32_t size);

uint16_t SBW_GetJTAGID(void);
uint32_t SBW_GetIDCode(void);

void SBW_DelayUs(uint32_t us);
void SBW_SendBit(uint8_t bit);
uint8_t SBW_ReceiveBit(void);

void SBW_GPIO_Init(void);
void SBW_GPIO_DeInit(void);

#define SBW_Enable()      HAL_GPIO_WritePin(g_sbw_config.rst_port, g_sbw_config.rst_pin, GPIO_PIN_SET)
#define SBW_Disable()     HAL_GPIO_WritePin(g_sbw_config.rst_port, g_sbw_config.rst_pin, GPIO_PIN_RESET)

#ifdef __cplusplus
}
#endif

#endif /* __SBW_H__ */
