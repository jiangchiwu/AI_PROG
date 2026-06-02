
#ifndef __ICSP_H
#define __ICSP_H

#include "stm32h7xx_hal.h"
#include &lt;stdint.h&gt;

typedef struct {
    GPIO_TypeDef* pgc_port;
    uint16_t pgc_pin;
    GPIO_TypeDef* pgd_port;
    uint16_t pgd_pin;
    uint32_t speed_hz;
} ICSP_HandleTypeDef;

HAL_StatusTypeDef ICSP_Init(ICSP_HandleTypeDef* hicsp);
HAL_StatusTypeDef ICSP_EnterProgram(ICSP_HandleTypeDef* hicsp);
HAL_StatusTypeDef ICSP_ExitProgram(ICSP_HandleTypeDef* hicsp);
HAL_StatusTypeDef ICSP_SendCommand(ICSP_HandleTypeDef* hicsp, uint8_t cmd);
HAL_StatusTypeDef ICSP_WriteData(ICSP_HandleTypeDef* hicsp, uint16_t data);
uint16_t ICSP_ReadData(ICSP_HandleTypeDef* hicsp);
HAL_StatusTypeDef ICSP_BulkErase(ICSP_HandleTypeDef* hicsp);
HAL_StatusTypeDef ICSP_ErasePage(ICSP_HandleTypeDef* hicsp, uint32_t addr);
HAL_StatusTypeDef ICSP_WriteMem(ICSP_HandleTypeDef* hicsp, uint32_t addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef ICSP_ReadMem(ICSP_HandleTypeDef* hicsp, uint32_t addr, uint8_t* data, uint16_t len);

#endif
