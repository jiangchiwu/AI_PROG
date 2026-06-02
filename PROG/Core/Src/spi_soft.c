
#include "spi_soft.h"

static void SPI_Delay(SPI_HandleTypeDef* hspi)
{
    if (hspi-&gt;speed_hz &gt;= 10000000) {
        for (volatile uint32_t i = 0; i &lt; 5; i++);
    } else if (hspi-&gt;speed_hz &gt;= 1000000) {
        for (volatile uint32_t i = 0; i &lt; 50; i++);
    } else {
        for (volatile uint32_t i = 0; i &lt; 500; i++);
    }
}

static void SPI_SCK_LOW(SPI_HandleTypeDef* hspi)
{
    HAL_GPIO_WritePin(hspi-&gt;sck_port, hspi-&gt;sck_pin, GPIO_PIN_RESET);
}

static void SPI_SCK_HIGH(SPI_HandleTypeDef* hspi)
{
    HAL_GPIO_WritePin(hspi-&gt;sck_port, hspi-&gt;sck_pin, GPIO_PIN_SET);
}

static void SPI_MOSI_LOW(SPI_HandleTypeDef* hspi)
{
    HAL_GPIO_WritePin(hspi-&gt;mosi_port, hspi-&gt;mosi_pin, GPIO_PIN_RESET);
}

static void SPI_MOSI_HIGH(SPI_HandleTypeDef* hspi)
{
    HAL_GPIO_WritePin(hspi-&gt;mosi_port, hspi-&gt;mosi_pin, GPIO_PIN_SET);
}

static GPIO_PinState SPI_MISO_READ(SPI_HandleTypeDef* hspi)
{
    return HAL_GPIO_ReadPin(hspi-&gt;miso_port, hspi-&gt;miso_pin);
}

HAL_StatusTypeDef SPI_Init(SPI_HandleTypeDef* hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = hspi-&gt;sck_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(hspi-&gt;sck_port, &amp;GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = hspi-&gt;mosi_pin;
    HAL_GPIO_Init(hspi-&gt;mosi_port, &amp;GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = hspi-&gt;miso_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(hspi-&gt;miso_port, &amp;GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = hspi-&gt;cs_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(hspi-&gt;cs_port, &amp;GPIO_InitStruct);
    
    SPI_CS_Deselect(hspi);
    
    switch (hspi-&gt;mode) {
        case SPI_MODE0:
            SPI_SCK_LOW(hspi);
            break;
        case SPI_MODE1:
            SPI_SCK_LOW(hspi);
            break;
        case SPI_MODE2:
            SPI_SCK_HIGH(hspi);
            break;
        case SPI_MODE3:
            SPI_SCK_HIGH(hspi);
            break;
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef SPI_CS_Select(SPI_HandleTypeDef* hspi)
{
    HAL_GPIO_WritePin(hspi-&gt;cs_port, hspi-&gt;cs_pin, GPIO_PIN_RESET);
    SPI_Delay(hspi);
    return HAL_OK;
}

HAL_StatusTypeDef SPI_CS_Deselect(SPI_HandleTypeDef* hspi)
{
    HAL_GPIO_WritePin(hspi-&gt;cs_port, hspi-&gt;cs_pin, GPIO_PIN_SET);
    SPI_Delay(hspi);
    return HAL_OK;
}

uint8_t SPI_TransferByte(SPI_HandleTypeDef* hspi, uint8_t data)
{
    uint8_t rx_data = 0;
    
    for (uint8_t i = 0; i &lt; 8; i++) {
        switch (hspi-&gt;mode) {
            case SPI_MODE0:
            case SPI_MODE2:
                if (data &amp; 0x80) {
                    SPI_MOSI_HIGH(hspi);
                } else {
                    SPI_MOSI_LOW(hspi);
                }
                data &lt;&lt;= 1;
                SPI_Delay(hspi);
                
                if (hspi-&gt;mode == SPI_MODE0) {
                    SPI_SCK_HIGH(hspi);
                } else {
                    SPI_SCK_LOW(hspi);
                }
                SPI_Delay(hspi);
                
                rx_data &lt;&lt;= 1;
                if (SPI_MISO_READ(hspi) == GPIO_PIN_SET) {
                    rx_data |= 0x01;
                }
                
                if (hspi-&gt;mode == SPI_MODE0) {
                    SPI_SCK_LOW(hspi);
                } else {
                    SPI_SCK_HIGH(hspi);
                }
                break;
                
            case SPI_MODE1:
            case SPI_MODE3:
                if (hspi-&gt;mode == SPI_MODE1) {
                    SPI_SCK_HIGH(hspi);
                } else {
                    SPI_SCK_LOW(hspi);
                }
                SPI_Delay(hspi);
                
                if (data &amp; 0x80) {
                    SPI_MOSI_HIGH(hspi);
                } else {
                    SPI_MOSI_LOW(hspi);
                }
                data &lt;&lt;= 1;
                
                if (hspi-&gt;mode == SPI_MODE1) {
                    SPI_SCK_LOW(hspi);
                } else {
                    SPI_SCK_HIGH(hspi);
                }
                SPI_Delay(hspi);
                
                rx_data &lt;&lt;= 1;
                if (SPI_MISO_READ(hspi) == GPIO_PIN_SET) {
                    rx_data |= 0x01;
                }
                break;
        }
        SPI_Delay(hspi);
    }
    
    return rx_data;
}

HAL_StatusTypeDef SPI_Transfer(SPI_HandleTypeDef* hspi, uint8_t* tx_data, uint8_t* rx_data, uint16_t len)
{
    for (uint16_t i = 0; i &lt; len; i++) {
        uint8_t tx_byte = (tx_data != NULL) ? tx_data[i] : 0xFF;
        uint8_t rx_byte = SPI_TransferByte(hspi, tx_byte);
        if (rx_data != NULL) {
            rx_data[i] = rx_byte;
        }
    }
    return HAL_OK;
}

HAL_StatusTypeDef SPI_Write(SPI_HandleTypeDef* hspi, uint8_t* data, uint16_t len)
{
    return SPI_Transfer(hspi, data, NULL, len);
}

HAL_StatusTypeDef SPI_Read(SPI_HandleTypeDef* hspi, uint8_t* data, uint16_t len)
{
    return SPI_Transfer(hspi, NULL, data, len);
}
