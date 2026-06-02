
#ifndef __SWIM_H
#define __SWIM_H

#include "stm32h7xx_hal.h"
#include &lt;stdint.h&gt;

#define SWIM_ENTRY_SEQ_LEN 4
#define SWIM_RESET_DELAY_US 1000

typedef struct {
    GPIO_TypeDef* swim_port;
    uint16_t swim_pin;
    uint32_t speed_hz;
} SWIM_HandleTypeDef;

HAL_StatusTypeDef SWIM_Init(SWIM_HandleTypeDef* hswim);
HAL_StatusTypeDef SWIM_Entry(SWIM_HandleTypeDef* hswim);
HAL_StatusTypeDef SWIM_Exit(SWIM_HandleTypeDef* hswim);
HAL_StatusTypeDef SWIM_SendByte(SWIM_HandleTypeDef* hswim, uint8_t data);
uint8_t SWIM_ReceiveByte(SWIM_HandleTypeDef* hswim);
HAL_StatusTypeDef SWIM_WriteMem(SWIM_HandleTypeDef* hswim, uint32_t addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef SWIM_ReadMem(SWIM_HandleTypeDef* hswim, uint32_t addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef SWIM_WOByte(SWIM_HandleTypeDef* hswim, uint32_t addr, uint8_t data);
HAL_StatusTypeDef SWIM_WOWord(SWIM_HandleTypeDef* hswim, uint32_t addr, uint16_t data);
uint8_t SWIM_ROByte(SWIM_HandleTypeDef* hswim, uint32_t addr);
uint16_t SWIM_ROWord(SWIM_HandleTypeDef* hswim, uint32_t addr);

#endif
