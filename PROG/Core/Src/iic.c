
#include "iic.h"
#include "gpio_soft.h"
#include "tim.h"

static void IIC_Delay(IIC_HandleTypeDef* hiic)
{
    if (hiic-&gt;speed_hz &gt;= 400000) {
        for (volatile uint32_t i = 0; i &lt; 10; i++);
    } else if (hiic-&gt;speed_hz &gt;= 100000) {
        for (volatile uint32_t i = 0; i &lt; 50; i++);
    } else {
        for (volatile uint32_t i = 0; i &lt; 200; i++);
    }
}

static void IIC_SCL_HIGH(IIC_HandleTypeDef* hiic)
{
    HAL_GPIO_WritePin(hiic-&gt;scl_port, hiic-&gt;scl_pin, GPIO_PIN_SET);
}

static void IIC_SCL_LOW(IIC_HandleTypeDef* hiic)
{
    HAL_GPIO_WritePin(hiic-&gt;scl_port, hiic-&gt;scl_pin, GPIO_PIN_RESET);
}

static void IIC_SDA_HIGH(IIC_HandleTypeDef* hiic)
{
    HAL_GPIO_WritePin(hiic-&gt;sda_port, hiic-&gt;sda_pin, GPIO_PIN_SET);
}

static void IIC_SDA_LOW(IIC_HandleTypeDef* hiic)
{
    HAL_GPIO_WritePin(hiic-&gt;sda_port, hiic-&gt;sda_pin, GPIO_PIN_RESET);
}

static GPIO_PinState IIC_SDA_READ(IIC_HandleTypeDef* hiic)
{
    return HAL_GPIO_ReadPin(hiic-&gt;sda_port, hiic-&gt;sda_pin);
}

HAL_StatusTypeDef IIC_Init(IIC_HandleTypeDef* hiic)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = hiic-&gt;scl_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(hiic-&gt;scl_port, &amp;GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = hiic-&gt;sda_pin;
    HAL_GPIO_Init(hiic-&gt;sda_port, &amp;GPIO_InitStruct);
    
    IIC_SCL_HIGH(hiic);
    IIC_SDA_HIGH(hiic);
    
    return HAL_OK;
}

HAL_StatusTypeDef IIC_Start(IIC_HandleTypeDef* hiic)
{
    IIC_SDA_HIGH(hiic);
    IIC_SCL_HIGH(hiic);
    IIC_Delay(hiic);
    IIC_SDA_LOW(hiic);
    IIC_Delay(hiic);
    IIC_SCL_LOW(hiic);
    IIC_Delay(hiic);
    return HAL_OK;
}

HAL_StatusTypeDef IIC_Stop(IIC_HandleTypeDef* hiic)
{
    IIC_SDA_LOW(hiic);
    IIC_Delay(hiic);
    IIC_SCL_HIGH(hiic);
    IIC_Delay(hiic);
    IIC_SDA_HIGH(hiic);
    IIC_Delay(hiic);
    return HAL_OK;
}

HAL_StatusTypeDef IIC_WriteByte(IIC_HandleTypeDef* hiic, uint8_t data)
{
    for (uint8_t i = 0; i &lt; 8; i++) {
        if (data &amp; 0x80) {
            IIC_SDA_HIGH(hiic);
        } else {
            IIC_SDA_LOW(hiic);
        }
        IIC_Delay(hiic);
        IIC_SCL_HIGH(hiic);
        IIC_Delay(hiic);
        IIC_SCL_LOW(hiic);
        IIC_Delay(hiic);
        data &lt;&lt;= 1;
    }
    
    IIC_SDA_HIGH(hiic);
    IIC_Delay(hiic);
    IIC_SCL_HIGH(hiic);
    IIC_Delay(hiic);
    
    GPIO_PinState ack = IIC_SDA_READ(hiic);
    
    IIC_SCL_LOW(hiic);
    IIC_Delay(hiic);
    
    if (ack == GPIO_PIN_RESET) {
        return HAL_OK;
    } else {
        return HAL_ERROR;
    }
}

uint8_t IIC_ReadByte(IIC_HandleTypeDef* hiic, uint8_t ack)
{
    uint8_t data = 0;
    
    IIC_SDA_HIGH(hiic);
    
    for (uint8_t i = 0; i &lt; 8; i++) {
        data &lt;&lt;= 1;
        IIC_SCL_HIGH(hiic);
        IIC_Delay(hiic);
        
        if (IIC_SDA_READ(hiic) == GPIO_PIN_SET) {
            data |= 0x01;
        }
        
        IIC_SCL_LOW(hiic);
        IIC_Delay(hiic);
    }
    
    if (ack == IIC_ACK) {
        IIC_SDA_LOW(hiic);
    } else {
        IIC_SDA_HIGH(hiic);
    }
    
    IIC_Delay(hiic);
    IIC_SCL_HIGH(hiic);
    IIC_Delay(hiic);
    IIC_SCL_LOW(hiic);
    IIC_Delay(hiic);
    IIC_SDA_HIGH(hiic);
    
    return data;
}

HAL_StatusTypeDef IIC_WriteData(IIC_HandleTypeDef* hiic, uint8_t dev_addr, uint8_t* data, uint16_t len)
{
    HAL_StatusTypeDef status;
    
    status = IIC_Start(hiic);
    if (status != HAL_OK) return status;
    
    status = IIC_WriteByte(hiic, (dev_addr &lt;&lt; 1) | 0);
    if (status != HAL_OK) {
        IIC_Stop(hiic);
        return status;
    }
    
    for (uint16_t i = 0; i &lt; len; i++) {
        status = IIC_WriteByte(hiic, data[i]);
        if (status != HAL_OK) {
            IIC_Stop(hiic);
            return status;
        }
    }
    
    IIC_Stop(hiic);
    return HAL_OK;
}

HAL_StatusTypeDef IIC_ReadData(IIC_HandleTypeDef* hiic, uint8_t dev_addr, uint8_t* data, uint16_t len)
{
    HAL_StatusTypeDef status;
    
    status = IIC_Start(hiic);
    if (status != HAL_OK) return status;
    
    status = IIC_WriteByte(hiic, (dev_addr &lt;&lt; 1) | 1);
    if (status != HAL_OK) {
        IIC_Stop(hiic);
        return status;
    }
    
    for (uint16_t i = 0; i &lt; len; i++) {
        if (i == len - 1) {
            data[i] = IIC_ReadByte(hiic, IIC_NACK);
        } else {
            data[i] = IIC_ReadByte(hiic, IIC_ACK);
        }
    }
    
    IIC_Stop(hiic);
    return HAL_OK;
}

HAL_StatusTypeDef IIC_WriteReg(IIC_HandleTypeDef* hiic, uint8_t dev_addr, uint8_t reg, uint8_t data)
{
    HAL_StatusTypeDef status;
    
    status = IIC_Start(hiic);
    if (status != HAL_OK) return status;
    
    status = IIC_WriteByte(hiic, (dev_addr &lt;&lt; 1) | 0);
    if (status != HAL_OK) {
        IIC_Stop(hiic);
        return status;
    }
    
    status = IIC_WriteByte(hiic, reg);
    if (status != HAL_OK) {
        IIC_Stop(hiic);
        return status;
    }
    
    status = IIC_WriteByte(hiic, data);
    if (status != HAL_OK) {
        IIC_Stop(hiic);
        return status;
    }
    
    IIC_Stop(hiic);
    return HAL_OK;
}

HAL_StatusTypeDef IIC_ReadReg(IIC_HandleTypeDef* hiic, uint8_t dev_addr, uint8_t reg, uint8_t* data)
{
    HAL_StatusTypeDef status;
    
    status = IIC_Start(hiic);
    if (status != HAL_OK) return status;
    
    status = IIC_WriteByte(hiic, (dev_addr &lt;&lt; 1) | 0);
    if (status != HAL_OK) {
        IIC_Stop(hiic);
        return status;
    }
    
    status = IIC_WriteByte(hiic, reg);
    if (status != HAL_OK) {
        IIC_Stop(hiic);
        return status;
    }
    
    IIC_Start(hiic);
    
    status = IIC_WriteByte(hiic, (dev_addr &lt;&lt; 1) | 1);
    if (status != HAL_OK) {
        IIC_Stop(hiic);
        return status;
    }
    
    *data = IIC_ReadByte(hiic, IIC_NACK);
    
    IIC_Stop(hiic);
    return HAL_OK;
}
