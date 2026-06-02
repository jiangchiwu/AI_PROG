
#include "swim.h"

static void SWIM_Delay(SWIM_HandleTypeDef* hswim)
{
    if (hswim-&gt;speed_hz &gt;= 1000000) {
        for (volatile uint32_t i = 0; i &lt; 5; i++);
    } else if (hswim-&gt;speed_hz &gt;= 100000) {
        for (volatile uint32_t i = 0; i &lt; 50; i++);
    } else {
        for (volatile uint32_t i = 0; i &lt; 500; i++);
    }
}

static void SWIM_LOW(SWIM_HandleTypeDef* hswim)
{
    HAL_GPIO_WritePin(hswim-&gt;swim_port, hswim-&gt;swim_pin, GPIO_PIN_RESET);
}

static void SWIM_HIGH(SWIM_HandleTypeDef* hswim)
{
    HAL_GPIO_WritePin(hswim-&gt;swim_port, hswim-&gt;swim_pin, GPIO_PIN_SET);
}

static void SWIM_SetInput(SWIM_HandleTypeDef* hswim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = hswim-&gt;swim_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(hswim-&gt;swim_port, &amp;GPIO_InitStruct);
}

static void SWIM_SetOutput(SWIM_HandleTypeDef* hswim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = hswim-&gt;swim_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(hswim-&gt;swim_port, &amp;GPIO_InitStruct);
}

static GPIO_PinState SWIM_Read(SWIM_HandleTypeDef* hswim)
{
    return HAL_GPIO_ReadPin(hswim-&gt;swim_port, hswim-&gt;swim_pin);
}

HAL_StatusTypeDef SWIM_Init(SWIM_HandleTypeDef* hswim)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    
    SWIM_SetOutput(hswim);
    SWIM_HIGH(hswim);
    
    return HAL_OK;
}

HAL_StatusTypeDef SWIM_Entry(SWIM_HandleTypeDef* hswim)
{
    const uint8_t entry_seq[SWIM_ENTRY_SEQ_LEN] = {0xA0, 0x00, 0x00, 0x00};
    
    SWIM_HIGH(hswim);
    for (volatile uint32_t i = 0; i &lt; 1000; i++);
    
    SWIM_LOW(hswim);
    for (volatile uint32_t i = 0; i &lt; 100; i++);
    
    SWIM_HIGH(hswim);
    for (volatile uint32_t i = 0; i &lt; 10; i++);
    
    for (uint8_t i = 0; i &lt; SWIM_ENTRY_SEQ_LEN; i++) {
        SWIM_SendByte(hswim, entry_seq[i]);
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef SWIM_Exit(SWIM_HandleTypeDef* hswim)
{
    SWIM_SendByte(hswim, 0x00);
    SWIM_HIGH(hswim);
    return HAL_OK;
}

HAL_StatusTypeDef SWIM_SendByte(SWIM_HandleTypeDef* hswim, uint8_t data)
{
    SWIM_SetOutput(hswim);
    
    SWIM_LOW(hswim);
    SWIM_Delay(hswim);
    
    for (uint8_t i = 0; i &lt; 8; i++) {
        if (data &amp; 0x01) {
            SWIM_HIGH(hswim);
        } else {
            SWIM_LOW(hswim);
        }
        SWIM_Delay(hswim);
        SWIM_HIGH(hswim);
        SWIM_Delay(hswim);
        data &gt;&gt;= 1;
    }
    
    SWIM_HIGH(hswim);
    return HAL_OK;
}

uint8_t SWIM_ReceiveByte(SWIM_HandleTypeDef* hswim)
{
    uint8_t data = 0;
    
    SWIM_SetInput(hswim);
    
    for (uint8_t i = 0; i &lt; 8; i++) {
        data &gt;&gt;= 1;
        if (SWIM_Read(hswim) == GPIO_PIN_SET) {
            data |= 0x80;
        }
        SWIM_Delay(hswim);
        SWIM_Delay(hswim);
    }
    
    return data;
}

HAL_StatusTypeDef SWIM_WriteMem(SWIM_HandleTypeDef* hswim, uint32_t addr, uint8_t* data, uint16_t len)
{
    SWIM_SendByte(hswim, 0x08);
    SWIM_SendByte(hswim, (addr &gt;&gt; 16) &amp; 0xFF);
    SWIM_SendByte(hswim, (addr &gt;&gt; 8) &amp; 0xFF);
    SWIM_SendByte(hswim, addr &amp; 0xFF);
    SWIM_SendByte(hswim, len);
    
    for (uint16_t i = 0; i &lt; len; i++) {
        SWIM_SendByte(hswim, data[i]);
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef SWIM_ReadMem(SWIM_HandleTypeDef* hswim, uint32_t addr, uint8_t* data, uint16_t len)
{
    SWIM_SendByte(hswim, 0x04);
    SWIM_SendByte(hswim, (addr &gt;&gt; 16) &amp; 0xFF);
    SWIM_SendByte(hswim, (addr &gt;&gt; 8) &amp; 0xFF);
    SWIM_SendByte(hswim, addr &amp; 0xFF);
    SWIM_SendByte(hswim, len);
    
    for (uint16_t i = 0; i &lt; len; i++) {
        data[i] = SWIM_ReceiveByte(hswim);
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef SWIM_WOByte(SWIM_HandleTypeDef* hswim, uint32_t addr, uint8_t data)
{
    return SWIM_WriteMem(hswim, addr, &amp;data, 1);
}

HAL_StatusTypeDef SWIM_WOWord(SWIM_HandleTypeDef* hswim, uint32_t addr, uint16_t data)
{
    uint8_t buf[2] = {data &amp; 0xFF, (data &gt;&gt; 8) &amp; 0xFF};
    return SWIM_WriteMem(hswim, addr, buf, 2);
}

uint8_t SWIM_ROByte(SWIM_HandleTypeDef* hswim, uint32_t addr)
{
    uint8_t data;
    SWIM_ReadMem(hswim, addr, &amp;data, 1);
    return data;
}

uint16_t SWIM_ROWord(SWIM_HandleTypeDef* hswim, uint32_t addr)
{
    uint8_t buf[2];
    SWIM_ReadMem(hswim, addr, buf, 2);
    return (buf[1] &lt;&lt; 8) | buf[0];
}
