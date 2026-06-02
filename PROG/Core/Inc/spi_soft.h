
#ifndef __SPI_H
#define __SPI_H

#include "stm32h7xx_hal.h"
#include &lt;stdint.h&gt;

#define SPI_MODE0  0
#define SPI_MODE1  1
#define SPI_MODE2  2
#define SPI_MODE3  3

typedef struct {
    GPIO_TypeDef* sck_port;
    uint16_t sck_pin;
    GPIO_TypeDef* mosi_port;
    uint16_t mosi_pin;
    GPIO_TypeDef* miso_port;
    uint16_t miso_pin;
    GPIO_TypeDef* cs_port;
    uint16_t cs_pin;
    uint8_t mode;
    uint32_t speed_hz;
} SPI_HandleTypeDef;

HAL_StatusTypeDef SPI_Init(SPI_HandleTypeDef* hspi);
HAL_StatusTypeDef SPI_CS_Select(SPI_HandleTypeDef* hspi);
HAL_StatusTypeDef SPI_CS_Deselect(SPI_HandleTypeDef* hspi);
uint8_t SPI_TransferByte(SPI_HandleTypeDef* hspi, uint8_t data);
HAL_StatusTypeDef SPI_Transfer(SPI_HandleTypeDef* hspi, uint8_t* tx_data, uint8_t* rx_data, uint16_t len);
HAL_StatusTypeDef SPI_Write(SPI_HandleTypeDef* hspi, uint8_t* data, uint16_t len);
HAL_StatusTypeDef SPI_Read(SPI_HandleTypeDef* hspi, uint8_t* data, uint16_t len);

#endif
