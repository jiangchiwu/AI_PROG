/**
  ******************************************************************************
  * @file    swd.h
  * @brief   SWD (Serial Wire Debug) 协议实现
  *          ARM Cortex-M调试接口
  ******************************************************************************
  */

#ifndef __SWD_H__
#define __SWD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

#define SWD_OK              0x00
#define SWD_ERR            0x01
#define SWD_ERR_FAULT      0x02
#define SWD_ERR_TIMEOUT    0x03
#define SWD_ERR_PARITY     0x04
#define SWD_ERR_NO_ACK     0x05

#define SWD_LINE_RESET     0x00
#define SWD_LINE_SWD       0x01
#define SWD_LINE_JTAG      0x02

#define SWD_CLOCK_1MHZ    1000000
#define SWD_CLOCK_4MHZ    4000000
#define SWD_CLOCK_10MHZ   10000000
#define SWD_CLOCK_20MHZ   20000000

#define SWD_DEFAULT_CLOCK  SWD_CLOCK_4MHZ

typedef struct {
    GPIO_TypeDef *swdio_port;
    uint16_t swdio_pin;
    GPIO_TypeDef *swclk_port;
    uint16_t swclk_pin;
    GPIO_TypeDef *reset_port;
    uint16_t reset_pin;

    uint32_t clock;
    uint8_t line_mode;
    uint8_t initialized;
} SWD_Config_TypeDef;

typedef struct {
    uint32_t dp_idcode;
    uint32_t ctrl_stat;
    uint32_t select;
    uint8_t ap;
    uint8_t protocol_ver;
} SWD_State_TypeDef;

extern SWD_Config_TypeDef g_swd_config;
extern SWD_State_TypeDef g_swd_state;

HAL_StatusTypeDef SWD_Init(SWD_Config_TypeDef *config);
HAL_StatusTypeDef SWD_DeInit(void);

HAL_StatusTypeDef SWD_LineReset(void);
HAL_StatusTypeDef SWD_SwitchMode(uint8_t mode);

HAL_StatusTypeDef SWD_Write(uint8_t addr, uint32_t data, uint8_t apnwp);
uint32_t SWD_Read(uint8_t addr, uint8_t *ack);

HAL_StatusTypeDef SWD_WriteDP(uint8_t addr, uint32_t data);
uint32_t SWD_ReadDP(uint8_t addr);

HAL_StatusTypeDef SWD_WriteAP(uint32_t addr, uint32_t data);
uint32_t SWD_ReadAP(uint32_t addr);

HAL_StatusTypeDef SWD_WriteAPReg(uint8_t ap, uint8_t reg, uint32_t data);
uint32_t SWD_ReadAPReg(uint8_t ap, uint8_t reg);

HAL_StatusTypeDef SWD_WriteMem(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef SWD_ReadMem(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef SWD_WriteWord(uint32_t addr, uint32_t data);
uint32_t SWD_ReadWord(uint32_t addr);

HAL_StatusTypeDef SWD_SetClock(uint32_t clock);
uint32_t SWD_GetClock(void);

uint32_t SWD_GetDPID(void);
uint8_t SWD_GetProtocolVersion(void);

void SWD_GPIO_Init(void);
void SWD_GPIO_DeInit(void);

#define SWD_Enter()      SWD_LineReset()
#define SWD_Exit()       SWD_LineReset()

#define SWD_Enable()     HAL_GPIO_WritePin(g_swd_config.reset_port, g_swd_config.reset_pin, GPIO_PIN_SET)
#define SWD_Disable()    HAL_GPIO_WritePin(g_swd_config.reset_port, g_swd_config.reset_pin, GPIO_PIN_RESET)
#define SWD_Reset()      HAL_GPIO_WritePin(g_swd_config.reset_port, g_swd_config.reset_pin, GPIO_PIN_RESET); \
                         HAL_Delay(1); \
                         HAL_GPIO_WritePin(g_swd_config.reset_port, g_swd_config.reset_pin, GPIO_PIN_SET)

#ifdef __cplusplus
}
#endif

#endif /* __SWD_H__ */
