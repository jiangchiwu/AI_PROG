
#include "cjtag.h"
#include "gpio_soft.h"

static void CJTAG_Delay(CJTAG_HandleTypeDef* hcjtag)
{
    if (hcjtag-&gt;speed_hz &gt;= 10000000) {
        for (volatile uint32_t i = 0; i &lt; 5; i++);
    } else if (hcjtag-&gt;speed_hz &gt;= 1000000) {
        for (volatile uint32_t i = 0; i &lt; 50; i++);
    } else {
        for (volatile uint32_t i = 0; i &lt; 500; i++);
    }
}

static void CJTAG_ToggleTCK(CJTAG_HandleTypeDef* hcjtag)
{
    HAL_GPIO_WritePin(hcjtag-&gt;tck_port, hcjtag-&gt;tck_pin, GPIO_PIN_SET);
    CJTAG_Delay(hcjtag);
    HAL_GPIO_WritePin(hcjtag-&gt;tck_port, hcjtag-&gt;tck_pin, GPIO_PIN_RESET);
    CJTAG_Delay(hcjtag);
}

HAL_StatusTypeDef CJTAG_Init(CJTAG_HandleTypeDef* hcjtag)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = hcjtag-&gt;tck_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(hcjtag-&gt;tck_port, &amp;GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = hcjtag-&gt;tms_pin;
    HAL_GPIO_Init(hcjtag-&gt;tms_port, &amp;GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = hcjtag-&gt;tdi_pin;
    HAL_GPIO_Init(hcjtag-&gt;tdi_port, &amp;GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = hcjtag-&gt;tdo_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(hcjtag-&gt;tdo_port, &amp;GPIO_InitStruct);
    
    if (hcjtag-&gt;rtck_port != NULL) {
        GPIO_InitStruct.Pin = hcjtag-&gt;rtck_pin;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(hcjtag-&gt;rtck_port, &amp;GPIO_InitStruct);
    }
    
    HAL_GPIO_WritePin(hcjtag-&gt;tck_port, hcjtag-&gt;tck_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(hcjtag-&gt;tms_port, hcjtag-&gt;tms_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(hcjtag-&gt;tdi_port, hcjtag-&gt;tdi_pin, GPIO_PIN_RESET);
    
    return HAL_OK;
}

HAL_StatusTypeDef CJTAG_DeInit(CJTAG_HandleTypeDef* hcjtag)
{
    HAL_GPIO_DeInit(hcjtag-&gt;tck_port, hcjtag-&gt;tck_pin);
    HAL_GPIO_DeInit(hcjtag-&gt;tms_port, hcjtag-&gt;tms_pin);
    HAL_GPIO_DeInit(hcjtag-&gt;tdi_port, hcjtag-&gt;tdi_pin);
    HAL_GPIO_DeInit(hcjtag-&gt;tdo_port, hcjtag-&gt;tdo_pin);
    if (hcjtag-&gt;rtck_port != NULL) {
        HAL_GPIO_DeInit(hcjtag-&gt;rtck_port, hcjtag-&gt;rtck_pin);
    }
    return HAL_OK;
}

HAL_StatusTypeDef CJTAG_Reset(CJTAG_HandleTypeDef* hcjtag)
{
    HAL_GPIO_WritePin(hcjtag-&gt;tms_port, hcjtag-&gt;tms_pin, GPIO_PIN_SET);
    for (uint8_t i = 0; i &lt; 5; i++) {
        CJTAG_ToggleTCK(hcjtag);
    }
    HAL_GPIO_WritePin(hcjtag-&gt;tms_port, hcjtag-&gt;tms_pin, GPIO_PIN_RESET);
    CJTAG_ToggleTCK(hcjtag);
    return HAL_OK;
}

HAL_StatusTypeDef CJTAG_SwitchToJTAG(CJTAG_HandleTypeDef* hcjtag)
{
    CJTAG_Reset(hcjtag);
    
    uint8_t switch_seq = 0x1F;
    for (uint8_t i = 0; i &lt; 8; i++) {
        HAL_GPIO_WritePin(hcjtag-&gt;tms_port, hcjtag-&gt;tms_pin, (switch_seq &amp; 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        CJTAG_ToggleTCK(hcjtag);
        switch_seq &gt;&gt;= 1;
    }
    
    hcjtag-&gt;mode = 0; 
    return HAL_OK;
}

HAL_StatusTypeDef CJTAG_SwitchToCJTAG(CJTAG_HandleTypeDef* hcjtag)
{
    CJTAG_Reset(hcjtag);
    
    uint8_t switch_seq = 0x3F;
    for (uint8_t i = 0; i &lt; 8; i++) {
        HAL_GPIO_WritePin(hcjtag-&gt;tms_port, hcjtag-&gt;tms_pin, (switch_seq &amp; 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        CJTAG_ToggleTCK(hcjtag);
        switch_seq &gt;&gt;= 1;
    }
    
    hcjtag-&gt;mode = 1; 
    return HAL_OK;
}

static uint8_t CJTAG_ShiftBit(CJTAG_HandleTypeDef* hcjtag, uint8_t tms, uint8_t tdi)
{
    uint8_t tdo = 0;
    
    HAL_GPIO_WritePin(hcjtag-&gt;tms_port, hcjtag-&gt;tms_pin, tms ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(hcjtag-&gt;tdi_port, hcjtag-&gt;tdi_pin, tdi ? GPIO_PIN_SET : GPIO_PIN_RESET);
    CJTAG_Delay(hcjtag);
    
    tdo = HAL_GPIO_ReadPin(hcjtag-&gt;tdo_port, hcjtag-&gt;tdo_pin) ? 1 : 0;
    
    HAL_GPIO_WritePin(hcjtag-&gt;tck_port, hcjtag-&gt;tck_pin, GPIO_PIN_SET);
    CJTAG_Delay(hcjtag);
    HAL_GPIO_WritePin(hcjtag-&gt;tck_port, hcjtag-&gt;tck_pin, GPIO_PIN_RESET);
    CJTAG_Delay(hcjtag);
    
    return tdo;
}

HAL_StatusTypeDef CJTAG_TAP_GotoState(CJTAG_HandleTypeDef* hcjtag, CJTAG_TAP_State_t target_state)
{
    static const uint8_t path[16][8] = {
        {0, 1, 2, 3, 4, 5, 6, 7},     
        {0, 1, 8, 9, 10, 11, 12, 13}, 
        {0, 1, 2, 9, 10, 11, 12, 13}, 
        {0, 1, 2, 3, 10, 11, 12, 13}, 
        {0, 1, 2, 3, 4, 11, 12, 13}, 
        {0, 1, 2, 3, 4, 5, 12, 13}, 
        {0, 1, 2, 3, 4, 5, 6, 13}, 
        {0, 1, 2, 3, 4, 5, 6, 7}, 
        {0, 1, 8, 9, 10, 11, 12, 13}, 
        {0, 1, 2, 9, 10, 11, 12, 13}, 
        {0, 1, 2, 3, 10, 11, 12, 13}, 
        {0, 1, 2, 3, 4, 11, 12, 13}, 
        {0, 1, 2, 3, 4, 5, 12, 13}, 
        {0, 1, 2, 3, 4, 5, 6, 13}, 
        {0, 1, 2, 3, 4, 5, 6, 7}, 
        {0, 1, 8, 9, 10, 11, 12, 13}  
    };
    
    static const uint8_t tms_seq[16] = {
        0b11111, 0b0, 0b10, 0b110, 0b1110, 0b11110, 0b111110, 0b0, 
        0b1, 0b11, 0b111, 0b1111, 0b11111, 0b0, 0b0, 0b0
    };
    
    uint8_t seq = tms_seq[target_state];
    for (uint8_t i = 0; i &lt; 5; i++) {
        CJTAG_ShiftBit(hcjtag, (seq &gt;&gt; i) &amp; 0x01, 0);
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef CJTAG_WriteIR(CJTAG_HandleTypeDef* hcjtag, uint8_t* ir_data, uint16_t ir_length)
{
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_SHIFT_IR);
    
    for (uint16_t i = 0; i &lt; ir_length; i++) {
        uint8_t byte_idx = i / 8;
        uint8_t bit_idx = i % 8;
        uint8_t tdi = (ir_data[byte_idx] &gt;&gt; bit_idx) &amp; 0x01;
        uint8_t tms = (i == ir_length - 1) ? 1 : 0;
        
        CJTAG_ShiftBit(hcjtag, tms, tdi);
    }
    
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_RUN_TEST_IDLE);
    
    return HAL_OK;
}

HAL_StatusTypeDef CJTAG_ReadIR(CJTAG_HandleTypeDef* hcjtag, uint8_t* ir_data, uint16_t ir_length)
{
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_SHIFT_IR);
    
    for (uint16_t i = 0; i &lt; ir_length; i++) {
        uint8_t byte_idx = i / 8;
        uint8_t bit_idx = i % 8;
        uint8_t tms = (i == ir_length - 1) ? 1 : 0;
        
        uint8_t tdo = CJTAG_ShiftBit(hcjtag, tms, 0);
        
        if (bit_idx == 0) {
            ir_data[byte_idx] = 0;
        }
        ir_data[byte_idx] |= (tdo &lt;&lt; bit_idx);
    }
    
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_RUN_TEST_IDLE);
    
    return HAL_OK;
}

HAL_StatusTypeDef CJTAG_WriteDR(CJTAG_HandleTypeDef* hcjtag, uint8_t* dr_data, uint16_t dr_length)
{
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_SHIFT_DR);
    
    for (uint16_t i = 0; i &lt; dr_length; i++) {
        uint8_t byte_idx = i / 8;
        uint8_t bit_idx = i % 8;
        uint8_t tdi = (dr_data[byte_idx] &gt;&gt; bit_idx) &amp; 0x01;
        uint8_t tms = (i == dr_length - 1) ? 1 : 0;
        
        CJTAG_ShiftBit(hcjtag, tms, tdi);
    }
    
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_RUN_TEST_IDLE);
    
    return HAL_OK;
}

HAL_StatusTypeDef CJTAG_ReadDR(CJTAG_HandleTypeDef* hcjtag, uint8_t* dr_data, uint16_t dr_length)
{
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_SHIFT_DR);
    
    for (uint16_t i = 0; i &lt; dr_length; i++) {
        uint8_t byte_idx = i / 8;
        uint8_t bit_idx = i % 8;
        uint8_t tms = (i == dr_length - 1) ? 1 : 0;
        
        uint8_t tdo = CJTAG_ShiftBit(hcjtag, tms, 0);
        
        if (bit_idx == 0) {
            dr_data[byte_idx] = 0;
        }
        dr_data[byte_idx] |= (tdo &lt;&lt; bit_idx);
    }
    
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_RUN_TEST_IDLE);
    
    return HAL_OK;
}

uint32_t CJTAG_ReadIDCODE(CJTAG_HandleTypeDef* hcjtag)
{
    uint8_t ir_data[2] = {0x01, 0x00}; 
    uint8_t dr_data[4] = {0};
    
    CJTAG_WriteIR(hcjtag, ir_data, 4);
    CJTAG_ReadDR(hcjtag, dr_data, 32);
    
    return (dr_data[3] &lt;&lt; 24) | (dr_data[2] &lt;&lt; 16) | (dr_data[1] &lt;&lt; 8) | dr_data[0];
}
