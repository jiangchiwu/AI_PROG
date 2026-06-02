
#include "icsp.h"

static void ICSP_Delay(ICSP_HandleTypeDef* hicsp)
{
    if (hicsp-&gt;speed_hz &gt;= 1000000) {
        for (volatile uint32_t i = 0; i &lt; 5; i++);
    } else if (hicsp-&gt;speed_hz &gt;= 100000) {
        for (volatile uint32_t i = 0; i &lt; 50; i++);
    } else {
        for (volatile uint32_t i = 0; i &lt; 500; i++);
    }
}

static void ICSP_PGC_LOW(ICSP_HandleTypeDef* hicsp)
{
    HAL_GPIO_WritePin(hicsp-&gt;pgc_port, hicsp-&gt;pgc_pin, GPIO_PIN_RESET);
}

static void ICSP_PGC_HIGH(ICSP_HandleTypeDef* hicsp)
{
    HAL_GPIO_WritePin(hicsp-&gt;pgc_port, hicsp-&gt;pgc_pin, GPIO_PIN_SET);
}

static void ICSP_PGD_LOW(ICSP_HandleTypeDef* hicsp)
{
    HAL_GPIO_WritePin(hicsp-&gt;pgd_port, hicsp-&gt;pgd_pin, GPIO_PIN_RESET);
}

static void ICSP_PGD_HIGH(ICSP_HandleTypeDef* hicsp)
{
    HAL_GPIO_WritePin(hicsp-&gt;pgd_port, hicsp-&gt;pgd_pin, GPIO_PIN_SET);
}

static void ICSP_PGD_SetInput(ICSP_HandleTypeDef* hicsp)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = hicsp-&gt;pgd_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(hicsp-&gt;pgd_port, &amp;GPIO_InitStruct);
}

static void ICSP_PGD_SetOutput(ICSP_HandleTypeDef* hicsp)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = hicsp-&gt;pgd_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(hicsp-&gt;pgd_port, &amp;GPIO_InitStruct);
}

static GPIO_PinState ICSP_PGD_Read(ICSP_HandleTypeDef* hicsp)
{
    return HAL_GPIO_ReadPin(hicsp-&gt;pgd_port, hicsp-&gt;pgd_pin);
}

HAL_StatusTypeDef ICSP_Init(ICSP_HandleTypeDef* hicsp)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = hicsp-&gt;pgc_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(hicsp-&gt;pgc_port, &amp;GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = hicsp-&gt;pgd_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(hicsp-&gt;pgd_port, &amp;GPIO_InitStruct);
    
    ICSP_PGC_HIGH(hicsp);
    ICSP_PGD_HIGH(hicsp);
    
    return HAL_OK;
}

HAL_StatusTypeDef ICSP_EnterProgram(ICSP_HandleTypeDef* hicsp)
{
    ICSP_PGC_LOW(hicsp);
    ICSP_PGD_LOW(hicsp);
    for (volatile uint32_t i = 0; i &lt; 10; i++);
    ICSP_PGD_HIGH(hicsp);
    for (volatile uint32_t i = 0; i &lt; 10; i++);
    ICSP_PGD_LOW(hicsp);
    for (volatile uint32_t i = 0; i &lt; 10; i++);
    ICSP_PGD_HIGH(hicsp);
    
    return HAL_OK;
}

HAL_StatusTypeDef ICSP_ExitProgram(ICSP_HandleTypeDef* hicsp)
{
    ICSP_PGC_HIGH(hicsp);
    ICSP_PGD_HIGH(hicsp);
    for (volatile uint32_t i = 0; i &lt; 100; i++);
    
    return HAL_OK;
}

HAL_StatusTypeDef ICSP_SendCommand(ICSP_HandleTypeDef* hicsp, uint8_t cmd)
{
    ICSP_PGD_SetOutput(hicsp);
    
    for (uint8_t i = 0; i &lt; 8; i++) {
        if (cmd &amp; 0x80) {
            ICSP_PGD_HIGH(hicsp);
        } else {
            ICSP_PGD_LOW(hicsp);
        }
        ICSP_Delay(hicsp);
        ICSP_PGC_HIGH(hicsp);
        ICSP_Delay(hicsp);
        ICSP_PGC_LOW(hicsp);
        ICSP_Delay(hicsp);
        cmd &lt;&lt;= 1;
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef ICSP_WriteData(ICSP_HandleTypeDef* hicsp, uint16_t data)
{
    ICSP_PGD_SetOutput(hicsp);
    
    for (uint8_t i = 0; i &lt; 16; i++) {
        if (data &amp; 0x8000) {
            ICSP_PGD_HIGH(hicsp);
        } else {
            ICSP_PGD_LOW(hicsp);
        }
        ICSP_Delay(hicsp);
        ICSP_PGC_HIGH(hicsp);
        ICSP_Delay(hicsp);
        ICSP_PGC_LOW(hicsp);
        ICSP_Delay(hicsp);
        data &lt;&lt;= 1;
    }
    
    return HAL_OK;
}

uint16_t ICSP_ReadData(ICSP_HandleTypeDef* hicsp)
{
    uint16_t data = 0;
    
    ICSP_PGD_SetInput(hicsp);
    
    for (uint8_t i = 0; i &lt; 16; i++) {
        data &lt;&lt;= 1;
        ICSP_PGC_HIGH(hicsp);
        ICSP_Delay(hicsp);
        
        if (ICSP_PGD_Read(hicsp) == GPIO_PIN_SET) {
            data |= 0x0001;
        }
        
        ICSP_PGC_LOW(hicsp);
        ICSP_Delay(hicsp);
    }
    
    ICSP_PGD_SetOutput(hicsp);
    
    return data;
}

HAL_StatusTypeDef ICSP_BulkErase(ICSP_HandleTypeDef* hicsp)
{
    ICSP_SendCommand(hicsp, 0x09);
    ICSP_WriteData(hicsp, 0x0000);
    ICSP_WriteData(hicsp, 0x0000);
    ICSP_WriteData(hicsp, 0x0000);
    
    return HAL_OK;
}

HAL_StatusTypeDef ICSP_ErasePage(ICSP_HandleTypeDef* hicsp, uint32_t addr)
{
    ICSP_SendCommand(hicsp, 0x0A);
    ICSP_WriteData(hicsp, (addr &gt;&gt; 16) &amp; 0xFFFF);
    ICSP_WriteData(hicsp, addr &amp; 0xFFFF);
    ICSP_WriteData(hicsp, 0x0000);
    
    return HAL_OK;
}

HAL_StatusTypeDef ICSP_WriteMem(ICSP_HandleTypeDef* hicsp, uint32_t addr, uint8_t* data, uint16_t len)
{
    ICSP_SendCommand(hicsp, 0x02);
    ICSP_WriteData(hicsp, (addr &gt;&gt; 16) &amp; 0xFFFF);
    ICSP_WriteData(hicsp, addr &amp; 0xFFFF);
    
    for (uint16_t i = 0; i &lt; len; i += 2) {
        uint16_t word = data[i];
        if (i + 1 &lt; len) {
            word |= (data[i + 1] &lt;&lt; 8);
        }
        ICSP_WriteData(hicsp, word);
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef ICSP_ReadMem(ICSP_HandleTypeDef* hicsp, uint32_t addr, uint8_t* data, uint16_t len)
{
    ICSP_SendCommand(hicsp, 0x01);
    ICSP_WriteData(hicsp, (addr &gt;&gt; 16) &amp; 0xFFFF);
    ICSP_WriteData(hicsp, addr &amp; 0xFFFF);
    
    for (uint16_t i = 0; i &lt; len; i += 2) {
        uint16_t word = ICSP_ReadData(hicsp);
        data[i] = word &amp; 0xFF;
        if (i + 1 &lt; len) {
            data[i + 1] = (word &gt;&gt; 8) &amp; 0xFF;
        }
    }
    
    return HAL_OK;
}
