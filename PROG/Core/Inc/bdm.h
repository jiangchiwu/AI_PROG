/**
 ******************************************************************************
 * @file    bdm.h
 * @brief   BDM (Background Debug Mode) 接口实现
 *          NXP/Freescale HC08/HCS08/HCS12系列调试接口
 *          支持最高10MHz时钟频率，使用寄存器操作和定时器精确定时
 ******************************************************************************
 */

#ifndef __BDM_H
#define __BDM_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

#define BDM_OK              0x00
#define BDM_ERR             0x01
#define BDM_ERR_FAULT       0x02
#define BDM_ERR_TIMEOUT     0x03

#define BDM_CLOCK_100KHZ    100000
#define BDM_CLOCK_200KHZ    200000
#define BDM_CLOCK_500KHZ    500000
#define BDM_CLOCK_1MHZ      1000000
#define BDM_CLOCK_2MHZ      2000000
#define BDM_CLOCK_5MHZ      5000000
#define BDM_CLOCK_10MHZ     10000000

#define BDM_DEFAULT_CLOCK   BDM_CLOCK_100KHZ

#define BDM_TIM_INSTANCE    TIM8
#define BDM_TIM_CLK_ENABLE() __HAL_RCC_TIM8_CLK_ENABLE()

typedef struct {
    GPIO_TypeDef* bkpt_port;
    uint16_t bkpt_pin;
    GPIO_TypeDef* reset_port;
    uint16_t reset_pin;
    uint32_t speed_hz;
    
    uint32_t tick_ns;
    uint32_t prescaler;
    uint32_t period;
} BDM_HandleTypeDef;

extern BDM_HandleTypeDef g_bdm_handle;

HAL_StatusTypeDef BDM_Init(BDM_HandleTypeDef* hbdm);
HAL_StatusTypeDef BDM_DeInit(BDM_HandleTypeDef* hbdm);

HAL_StatusTypeDef BDM_EnterBDM(BDM_HandleTypeDef* hbdm);
HAL_StatusTypeDef BDM_ExitBDM(BDM_HandleTypeDef* hbdm);

HAL_StatusTypeDef BDM_WriteByte(BDM_HandleTypeDef* hbdm, uint8_t data);
uint8_t BDM_ReadByte(BDM_HandleTypeDef* hbdm);

HAL_StatusTypeDef BDM_WriteMem(BDM_HandleTypeDef* hbdm, uint32_t addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef BDM_ReadMem(BDM_HandleTypeDef* hbdm, uint32_t addr, uint8_t* data, uint16_t len);

void BDM_SetSpeed(BDM_HandleTypeDef* hbdm, uint32_t speed_hz);
uint32_t BDM_GetSpeed(BDM_HandleTypeDef* hbdm);

void BDM_TimerDelayNs(BDM_HandleTypeDef* hbdm, uint32_t ns);
void BDM_TimerDelayUs(BDM_HandleTypeDef* hbdm, uint32_t us);

#endif
