
#ifndef __IIC_H
#define __IIC_H

#include "stm32h7xx_hal.h"
#include &lt;stdint.h&gt;

#define IIC_ACK     0
#define IIC_NACK    1

typedef struct {
    GPIO_TypeDef* scl_port;
    uint16_t scl_pin;
    GPIO_TypeDef* sda_port;
    uint16_t sda_pin;
    uint32_t speed_hz;
} IIC_HandleTypeDef;

HAL_StatusTypeDef IIC_Init(IIC_HandleTypeDef* hiic);
HAL_StatusTypeDef IIC_Start(IIC_HandleTypeDef* hiic);
HAL_StatusTypeDef IIC_Stop(IIC_HandleTypeDef* hiic);
HAL_StatusTypeDef IIC_WriteByte(IIC_HandleTypeDef* hiic, uint8_t data);
uint8_t IIC_ReadByte(IIC_HandleTypeDef* hiic, uint8_t ack);
HAL_StatusTypeDef IIC_WriteData(IIC_HandleTypeDef* hiic, uint8_t dev_addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef IIC_ReadData(IIC_HandleTypeDef* hiic, uint8_t dev_addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef IIC_WriteReg(IIC_HandleTypeDef* hiic, uint8_t dev_addr, uint8_t reg, uint8_t data);
HAL_StatusTypeDef IIC_ReadReg(IIC_HandleTypeDef* hiic, uint8_t dev_addr, uint8_t reg, uint8_t* data);

#endif
