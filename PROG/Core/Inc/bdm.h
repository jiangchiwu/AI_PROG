
#ifndef __BDM_H
#define __BDM_H

#include "stm32h7xx_hal.h"
#include &lt;stdint.h&gt;

typedef struct {
    GPIO_TypeDef* bkpt_port;
    uint16_t bkpt_pin;
    GPIO_TypeDef* reset_port;
    uint16_t reset_pin;
    uint32_t speed_hz;
} BDM_HandleTypeDef;

HAL_StatusTypeDef BDM_Init(BDM_HandleTypeDef* hbdm);
HAL_StatusTypeDef BDM_EnterBDM(BDM_HandleTypeDef* hbdm);
HAL_StatusTypeDef BDM_ExitBDM(BDM_HandleTypeDef* hbdm);
HAL_StatusTypeDef BDM_WriteByte(BDM_HandleTypeDef* hbdm, uint8_t data);
uint8_t BDM_ReadByte(BDM_HandleTypeDef* hbdm);
HAL_StatusTypeDef BDM_WriteMem(BDM_HandleTypeDef* hbdm, uint32_t addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef BDM_ReadMem(BDM_HandleTypeDef* hbdm, uint32_t addr, uint8_t* data, uint16_t len);

#endif
