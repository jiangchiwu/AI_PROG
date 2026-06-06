/**
 ******************************************************************************
 * @file    mon8.h
 * @brief   MON8 接口协议实现
 *          Freescale HC08/HC05 8位单片机调试接口
 *          支持最高10MHz时钟频率，使用寄存器操作和定时器精确定时
 ******************************************************************************
 */

#ifndef __MON8_H__
#define __MON8_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

#define MON8_OK              0x00
#define MON8_ERR             0x01
#define MON8_ERR_FAULT       0x02
#define MON8_ERR_TIMEOUT     0x03
#define MON8_ERR_PARITY      0x04
#define MON8_ERR_NO_ACK      0x05

#define MON8_CLOCK_100KHZ    100000
#define MON8_CLOCK_200KHZ    200000
#define MON8_CLOCK_500KHZ    500000
#define MON8_CLOCK_1MHZ      1000000
#define MON8_CLOCK_2MHZ      2000000
#define MON8_CLOCK_5MHZ      5000000
#define MON8_CLOCK_10MHZ     10000000

#define MON8_DEFAULT_CLOCK   MON8_CLOCK_100KHZ

#define MON8_TIM_INSTANCE    TIM12
#define MON8_TIM_CLK_ENABLE() do { __HAL_RCC_TIM12_CLK_ENABLE(); } while(0)

#define MON8_CMD_RESET       0x80
#define MON8_CMD_SECURE      0x81
#define MON8_CMD_READ        0x82
#define MON8_CMD_WRITE       0x83
#define MON8_CMD_ERASE       0x84
#define MON8_CMD_READ_VER    0x85
#define MON8_CMD_RUN         0x86
#define MON8_CMD_STOP        0x87

typedef struct {
    GPIO_TypeDef *bkpt_port;
    uint16_t bkpt_pin;
    GPIO_TypeDef *rst_port;
    uint16_t rst_pin;
    GPIO_TypeDef *ptx_port;
    uint16_t ptx_pin;
    GPIO_TypeDef *prx_port;
    uint16_t prx_pin;

    uint32_t speed_hz;
    uint32_t tick_ns;
    uint32_t prescaler;
    uint32_t period;
    uint8_t initialized;
} MON8_Config_TypeDef;

typedef struct {
    uint8_t version;
    uint8_t device_id;
    uint8_t status;
} MON8_State_TypeDef;

extern MON8_Config_TypeDef g_mon8_config;
extern MON8_State_TypeDef g_mon8_state;

HAL_StatusTypeDef MON8_Init(MON8_Config_TypeDef *config);
HAL_StatusTypeDef MON8_DeInit(void);

HAL_StatusTypeDef MON8_Enter(void);
HAL_StatusTypeDef MON8_Exit(void);

HAL_StatusTypeDef MON8_Reset(void);
HAL_StatusTypeDef MON8_Run(uint16_t addr);
HAL_StatusTypeDef MON8_Stop(void);

HAL_StatusTypeDef MON8_WriteByte(uint8_t data);
uint8_t MON8_ReadByte(void);

HAL_StatusTypeDef MON8_ReadMem(uint16_t addr, uint8_t *data, uint16_t size);
HAL_StatusTypeDef MON8_WriteMem(uint16_t addr, uint8_t *data, uint16_t size);

HAL_StatusTypeDef MON8_Erase(uint16_t addr, uint16_t size);
HAL_StatusTypeDef MON8_Secure(void);

uint8_t MON8_GetVersion(void);
uint8_t MON8_GetDeviceID(void);

void MON8_SetSpeed(uint32_t speed_hz);
uint32_t MON8_GetSpeed(void);

void MON8_DelayNs(uint32_t ns);
void MON8_DelayUs(uint32_t us);

void MON8_GPIO_Init(void);
void MON8_GPIO_DeInit(void);

#define MON8_Enable()      ((g_mon8_config.rst_port)->BSRR = (1 << g_mon8_config.rst_pin))
#define MON8_Disable()     ((g_mon8_config.rst_port)->BSRR = (1 << g_mon8_config.rst_pin) << 16)

#ifdef __cplusplus
}
#endif

#endif /* __MON8_H__ */
