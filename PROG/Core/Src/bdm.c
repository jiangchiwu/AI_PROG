
#include "bdm.h"

static void BDM_Delay(BDM_HandleTypeDef* hbdm)
{
    if (hbdm-&gt;speed_hz &gt;= 1000000) {
        for (volatile uint32_t i = 0; i &lt; 5; i++);
    } else if (hbdm-&gt;speed_hz &gt;= 100000) {
        for (volatile uint32_t i = 0; i &lt; 50; i++);
    } else {
        for (volatile uint32_t i = 0; i &lt; 500; i++);
    }
}

static void BDM_BKPT_LOW(BDM_HandleTypeDef* hbdm)
{
    HAL_GPIO_WritePin(hbdm-&gt;bkpt_port, hbdm-&gt;bkpt_pin, GPIO_PIN_RESET);
}

static void BDM_BKPT_HIGH(BDM_HandleTypeDef* hbdm)
{
    HAL_GPIO_WritePin(hbdm-&gt;bkpt_port, hbdm-&gt;bkpt_pin, GPIO_PIN_SET);
}

static void BDM_RESET_LOW(BDM_HandleTypeDef* hbdm)
{
    HAL_GPIO_WritePin(hbdm-&gt;reset_port, hbdm-&gt;reset_pin, GPIO_PIN_RESET);
}

static void BDM_RESET_HIGH(BDM_HandleTypeDef* hbdm)
{
    HAL_GPIO_WritePin(hbdm-&gt;reset_port, hbdm-&gt;reset_pin, GPIO_PIN_SET);
}

HAL_StatusTypeDef BDM_Init(BDM_HandleTypeDef* hbdm)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = hbdm-&gt;bkpt_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(hbdm-&gt;bkpt_port, &amp;GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = hbdm-&gt;reset_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(hbdm-&gt;reset_port, &amp;GPIO_InitStruct);
    
    BDM_BKPT_HIGH(hbdm);
    BDM_RESET_HIGH(hbdm);
    
    return HAL_OK;
}

HAL_StatusTypeDef BDM_EnterBDM(BDM_HandleTypeDef* hbdm)
{
    BDM_RESET_LOW(hbdm);
    for (volatile uint32_t i = 0; i &lt; 100; i++);
    BDM_BKPT_LOW(hbdm);
    for (volatile uint32_t i = 0; i &lt; 10; i++);
    BDM_RESET_HIGH(hbdm);
    for (volatile uint32_t i = 0; i &lt; 100; i++);
    
    return HAL_OK;
}

HAL_StatusTypeDef BDM_ExitBDM(BDM_HandleTypeDef* hbdm)
{
    BDM_BKPT_HIGH(hbdm);
    BDM_RESET_LOW(hbdm);
    for (volatile uint32_t i = 0; i &lt; 100; i++);
    BDM_RESET_HIGH(hbdm);
    
    return HAL_OK;
}

HAL_StatusTypeDef BDM_WriteByte(BDM_HandleTypeDef* hbdm, uint8_t data)
{
    for (uint8_t i = 0; i &lt; 8; i++) {
        if (data &amp; 0x80) {
            BDM_BKPT_HIGH(hbdm);
        } else {
            BDM_BKPT_LOW(hbdm);
        }
        BDM_Delay(hbdm);
        BDM_RESET_LOW(hbdm);
        BDM_Delay(hbdm);
        BDM_RESET_HIGH(hbdm);
        BDM_Delay(hbdm);
        data &lt;&lt;= 1;
    }
    
    BDM_BKPT_HIGH(hbdm);
    return HAL_OK;
}

uint8_t BDM_ReadByte(BDM_HandleTypeDef* hbdm)
{
    uint8_t data = 0;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = hbdm-&gt;bkpt_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(hbdm-&gt;bkpt_port, &amp;GPIO_InitStruct);
    
    for (uint8_t i = 0; i &lt; 8; i++) {
        data &lt;&lt;= 1;
        BDM_RESET_LOW(hbdm);
        BDM_Delay(hbdm);
        
        if (HAL_GPIO_ReadPin(hbdm-&gt;bkpt_port, hbdm-&gt;bkpt_pin) == GPIO_PIN_SET) {
            data |= 0x01;
        }
        
        BDM_RESET_HIGH(hbdm);
        BDM_Delay(hbdm);
    }
    
    GPIO_InitStruct.Pin = hbdm-&gt;bkpt_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(hbdm-&gt;bkpt_port, &amp;GPIO_InitStruct);
    
    return data;
}

HAL_StatusTypeDef BDM_WriteMem(BDM_HandleTypeDef* hbdm, uint32_t addr, uint8_t* data, uint16_t len)
{
    BDM_WriteByte(hbdm, 0x08);
    BDM_WriteByte(hbdm, (addr &gt;&gt; 24) &amp; 0xFF);
    BDM_WriteByte(hbdm, (addr &gt;&gt; 16) &amp; 0xFF);
    BDM_WriteByte(hbdm, (addr &gt;&gt; 8) &amp; 0xFF);
    BDM_WriteByte(hbdm, addr &amp; 0xFF);
    
    for (uint16_t i = 0; i &lt; len; i++) {
        BDM_WriteByte(hbdm, data[i]);
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef BDM_ReadMem(BDM_HandleTypeDef* hbdm, uint32_t addr, uint8_t* data, uint16_t len)
{
    BDM_WriteByte(hbdm, 0x04);
    BDM_WriteByte(hbdm, (addr &gt;&gt; 24) &amp; 0xFF);
    BDM_WriteByte(hbdm, (addr &gt;&gt; 16) &amp; 0xFF);
    BDM_WriteByte(hbdm, (addr &gt;&gt; 8) &amp; 0xFF);
    BDM_WriteByte(hbdm, addr &amp; 0xFF);
    
    for (uint16_t i = 0; i &lt; len; i++) {
        data[i] = BDM_ReadByte(hbdm);
    }
    
    return HAL_OK;
}
