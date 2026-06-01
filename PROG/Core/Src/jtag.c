/**
 ******************************************************************************
 * @file    jtag.c
 * @brief   JTAG (Joint Test Action Group) 协议实现
 *          IEEE 1149.1 边界扫描标准
 ******************************************************************************
 */

#include "jtag.h"
#include "gpio_soft.h"
#include <string.h>

JTAG_Config_TypeDef g_jtag_config = {
    .tck_port = GPIOB,
    .tck_pin = GPIO_PIN_0,
    .tms_port = GPIOB,
    .tms_pin = GPIO_PIN_1,
    .tdi_port = GPIOB,
    .tdi_pin = GPIO_PIN_2,
    .tdo_port = GPIOB,
    .tdo_pin = GPIO_PIN_3,
    .nrst_port = GPIOB,
    .nrst_pin = GPIO_PIN_4,
    .ntrst_port = GPIOB,
    .ntrst_pin = GPIO_PIN_5,
    .clock = JTAG_DEFAULT_CLOCK,
    .initialized = 0,
};

JTAG_State_TypeDef g_jtag_state = {
    .tap_state = TAP_STATE_RESET,
    .ir_length = 4,
    .current_ir = 0,
    .idcode = 0,
    .tap_count = 0,
};

#define JTAG_DELAY()     do { __NOP(); __NOP(); __NOP(); __NOP(); } while(0)

static void JTAG_TCK_Out(uint8_t bit)
{
    if (bit) {
        HAL_GPIO_WritePin(g_jtag_config.tck_port, g_jtag_config.tck_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(g_jtag_config.tck_port, g_jtag_config.tck_pin, GPIO_PIN_RESET);
    }
}

static void JTAG_TMS_Out(uint8_t bit)
{
    if (bit) {
        HAL_GPIO_WritePin(g_jtag_config.tms_port, g_jtag_config.tms_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(g_jtag_config.tms_port, g_jtag_config.tms_pin, GPIO_PIN_RESET);
    }
}

static void JTAG_TDI_Out(uint8_t bit)
{
    if (bit) {
        HAL_GPIO_WritePin(g_jtag_config.tdi_port, g_jtag_config.tdi_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(g_jtag_config.tdi_port, g_jtag_config.tdi_pin, GPIO_PIN_RESET);
    }
}

static uint8_t JTAG_TDO_In(void)
{
    return (HAL_GPIO_ReadPin(g_jtag_config.tdo_port, g_jtag_config.tdo_pin) == GPIO_PIN_SET) ? 1 : 0;
}

static void JTAG_Clock(void)
{
    JTAG_DELAY();
    JTAG_TCK_Out(1);
    JTAG_DELAY();
    JTAG_TCK_Out(0);
}

static void JTAG_TapTransition(uint8_t tms)
{
    JTAG_TMS_Out(tms);
    JTAG_Clock();
}

void JTAG_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = g_jtag_config.tck_pin | g_jtag_config.tms_pin | 
                          g_jtag_config.tdi_pin | g_jtag_config.nrst_pin | 
                          g_jtag_config.ntrst_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(g_jtag_config.tck_port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = g_jtag_config.tdo_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(g_jtag_config.tdo_port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(g_jtag_config.tck_port, g_jtag_config.tck_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(g_jtag_config.tms_port, g_jtag_config.tms_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(g_jtag_config.tdi_port, g_jtag_config.tdi_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(g_jtag_config.nrst_port, g_jtag_config.nrst_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(g_jtag_config.ntrst_port, g_jtag_config.ntrst_pin, GPIO_PIN_SET);
}

void JTAG_GPIO_DeInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = g_jtag_config.tck_pin | g_jtag_config.tms_pin | 
                          g_jtag_config.tdi_pin | g_jtag_config.tdo_pin |
                          g_jtag_config.nrst_pin | g_jtag_config.ntrst_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(g_jtag_config.tck_port, &GPIO_InitStruct);
}

HAL_StatusTypeDef JTAG_Init(JTAG_Config_TypeDef *config)
{
    if (config != NULL) {
        g_jtag_config.tck_port = config->tck_port;
        g_jtag_config.tck_pin = config->tck_pin;
        g_jtag_config.tms_port = config->tms_port;
        g_jtag_config.tms_pin = config->tms_pin;
        g_jtag_config.tdi_port = config->tdi_port;
        g_jtag_config.tdi_pin = config->tdi_pin;
        g_jtag_config.tdo_port = config->tdo_port;
        g_jtag_config.tdo_pin = config->tdo_pin;
        g_jtag_config.nrst_port = config->nrst_port;
        g_jtag_config.nrst_pin = config->nrst_pin;
        g_jtag_config.ntrst_port = config->ntrst_port;
        g_jtag_config.ntrst_pin = config->ntrst_pin;
        g_jtag_config.clock = config->clock;
    }

    JTAG_GPIO_Init();

    g_jtag_config.initialized = 1;

    JTAG_TAP_Reset();

    return HAL_OK;
}

HAL_StatusTypeDef JTAG_DeInit(void)
{
    JTAG_GPIO_DeInit();
    g_jtag_config.initialized = 0;
    return HAL_OK;
}

HAL_StatusTypeDef JTAG_TAP_Reset(void)
{
    for (uint8_t i = 0; i < 5; i++) {
        JTAG_TapTransition(1);
    }

    g_jtag_state.tap_state = TAP_STATE_IDLE;
    g_jtag_state.current_ir = 0;

    return HAL_OK;
}

HAL_StatusTypeDef JTAG_Reset(void)
{
    JTAG_TAP_Reset();
    return HAL_OK;
}

HAL_StatusTypeDef JTAG_Goto_State(JTAG_TAP_State_TypeDef state)
{
    switch (state) {
        case TAP_STATE_RESET:
            JTAG_TAP_Reset();
            break;

        case TAP_STATE_IDLE:
            while (g_jtag_state.tap_state != TAP_STATE_IDLE) {
                switch (g_jtag_state.tap_state) {
                    case TAP_STATE_RESET:
                        JTAG_TapTransition(0);
                        g_jtag_state.tap_state = TAP_STATE_IDLE;
                        break;
                    case TAP_STATE_SELECT_DR:
                    case TAP_STATE_SELECT_IR:
                    case TAP_STATE_CAPTURE_DR:
                    case TAP_STATE_CAPTURE_IR:
                    case TAP_STATE_SHIFT_DR:
                    case TAP_STATE_SHIFT_IR:
                    case TAP_STATE_EXIT1_DR:
                    case TAP_STATE_EXIT1_IR:
                    case TAP_STATE_PAUSE_DR:
                    case TAP_STATE_PAUSE_IR:
                    case TAP_STATE_EXIT2_DR:
                    case TAP_STATE_EXIT2_IR:
                    case TAP_STATE_UPDATE_DR:
                    case TAP_STATE_UPDATE_IR:
                        JTAG_TapTransition(1);
                        if (g_jtag_state.tap_state == TAP_STATE_UPDATE_DR || 
                            g_jtag_state.tap_state == TAP_STATE_UPDATE_IR) {
                            g_jtag_state.tap_state = TAP_STATE_IDLE;
                        } else {
                            g_jtag_state.tap_state = TAP_STATE_SELECT_DR;
                        }
                        break;
                    default:
                        break;
                }
            }
            break;

        case TAP_STATE_SHIFT_IR:
            JTAG_Goto_State(TAP_STATE_IDLE);
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_SELECT_DR;
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_SELECT_IR;
            JTAG_TapTransition(0);
            g_jtag_state.tap_state = TAP_STATE_CAPTURE_IR;
            JTAG_TapTransition(0);
            g_jtag_state.tap_state = TAP_STATE_SHIFT_IR;
            break;

        case TAP_STATE_SHIFT_DR:
            JTAG_Goto_State(TAP_STATE_IDLE);
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_SELECT_DR;
            JTAG_TapTransition(0);
            g_jtag_state.tap_state = TAP_STATE_CAPTURE_DR;
            JTAG_TapTransition(0);
            g_jtag_state.tap_state = TAP_STATE_SHIFT_DR;
            break;

        case TAP_STATE_UPDATE_IR:
            JTAG_Goto_State(TAP_STATE_SHIFT_IR);
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_EXIT1_IR;
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_UPDATE_IR;
            break;

        case TAP_STATE_UPDATE_DR:
            JTAG_Goto_State(TAP_STATE_SHIFT_DR);
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_EXIT1_DR;
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_UPDATE_DR;
            break;

        default:
            break;
    }

    return HAL_OK;
}

HAL_StatusTypeDef JTAG_Write_IR(uint32_t ir, uint32_t length)
{
    JTAG_Goto_State(TAP_STATE_SHIFT_IR);

    for (uint32_t i = 0; i < length; i++) {
        JTAG_TDI_Out((ir >> i) & 0x01);
        if (i == length - 1) {
            JTAG_TMS_Out(1);
        }
        JTAG_Clock();
    }

    g_jtag_state.current_ir = ir;
    g_jtag_state.tap_state = TAP_STATE_EXIT1_IR;

    JTAG_TapTransition(1);
    g_jtag_state.tap_state = TAP_STATE_UPDATE_IR;

    JTAG_TapTransition(0);
    g_jtag_state.tap_state = TAP_STATE_IDLE;

    return HAL_OK;
}

uint32_t JTAG_Read_IR(uint32_t length)
{
    uint32_t ir = 0;

    JTAG_Goto_State(TAP_STATE_SHIFT_IR);

    for (uint32_t i = 0; i < length; i++) {
        JTAG_TDI_Out(0);
        if (i == length - 1) {
            JTAG_TMS_Out(1);
        }
        ir |= (JTAG_TDO_In() << i);
        JTAG_Clock();
    }

    g_jtag_state.tap_state = TAP_STATE_EXIT1_IR;

    JTAG_TapTransition(1);
    g_jtag_state.tap_state = TAP_STATE_UPDATE_IR;

    JTAG_TapTransition(0);
    g_jtag_state.tap_state = TAP_STATE_IDLE;

    return ir;
}

HAL_StatusTypeDef JTAG_Write_DR(uint32_t dr, uint32_t length)
{
    JTAG_Goto_State(TAP_STATE_SHIFT_DR);

    for (uint32_t i = 0; i < length; i++) {
        JTAG_TDI_Out((dr >> i) & 0x01);
        if (i == length - 1) {
            JTAG_TMS_Out(1);
        }
        JTAG_Clock();
    }

    g_jtag_state.tap_state = TAP_STATE_EXIT1_DR;

    JTAG_TapTransition(1);
    g_jtag_state.tap_state = TAP_STATE_UPDATE_DR;

    JTAG_TapTransition(0);
    g_jtag_state.tap_state = TAP_STATE_IDLE;

    return HAL_OK;
}

uint32_t JTAG_Read_DR(uint32_t length)
{
    uint32_t dr = 0;

    JTAG_Goto_State(TAP_STATE_SHIFT_DR);

    for (uint32_t i = 0; i < length; i++) {
        JTAG_TDI_Out(0);
        if (i == length - 1) {
            JTAG_TMS_Out(1);
        }
        dr |= (JTAG_TDO_In() << i);
        JTAG_Clock();
    }

    g_jtag_state.tap_state = TAP_STATE_EXIT1_DR;

    JTAG_TapTransition(1);
    g_jtag_state.tap_state = TAP_STATE_UPDATE_DR;

    JTAG_TapTransition(0);
    g_jtag_state.tap_state = TAP_STATE_IDLE;

    return dr;
}

HAL_StatusTypeDef JTAG_Write_IR_DR(uint32_t ir, uint32_t ir_length, uint32_t dr, uint32_t dr_length)
{
    JTAG_Write_IR(ir, ir_length);
    return JTAG_Write_DR(dr, dr_length);
}

uint32_t JTAG_Read_IR_DR(uint32_t ir, uint32_t ir_length, uint32_t dr_length)
{
    JTAG_Write_IR(ir, ir_length);
    return JTAG_Read_DR(dr_length);
}

uint32_t JTAG_GetIDCode(void)
{
    JTAG_Write_IR(0x02, 4);
    g_jtag_state.idcode = JTAG_Read_DR(32);
    return g_jtag_state.idcode;
}

HAL_StatusTypeDef JTAG_DetectChain(void)
{
    uint32_t idcode = 0;
    uint8_t tap_count = 0;

    JTAG_TAP_Reset();

    JTAG_Write_IR(0x02, 4);

    do {
        idcode = JTAG_Read_DR(32);
        if (idcode != 0xFFFFFFFF && idcode != 0x00000000) {
            tap_count++;
        } else {
            break;
        }
    } while (tap_count < 10);

    g_jtag_state.tap_count = tap_count;

    if (tap_count == 0) {
        return HAL_ERROR;
    }

    g_jtag_state.idcode = idcode;

    return HAL_OK;
}

HAL_StatusTypeDef JTAG_SetClock(uint32_t clock)
{
    g_jtag_config.clock = clock;
    return HAL_OK;
}

uint32_t JTAG_GetClock(void)
{
    return g_jtag_config.clock;
}

HAL_StatusTypeDef JTAG_AssertReset(void)
{
    HAL_GPIO_WritePin(g_jtag_config.nrst_port, g_jtag_config.nrst_pin, GPIO_PIN_RESET);
    return HAL_OK;
}

HAL_StatusTypeDef JTAG_DeassertReset(void)
{
    HAL_GPIO_WritePin(g_jtag_config.nrst_port, g_jtag_config.nrst_pin, GPIO_PIN_SET);
    return HAL_OK;
}

HAL_StatusTypeDef JTAG_WriteBits(uint8_t *data, uint32_t length)
{
    JTAG_Goto_State(TAP_STATE_SHIFT_DR);
    
    for (uint32_t i = 0; i < length; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        uint8_t bit = (data[byte_idx] >> bit_idx) & 0x01;
        
        // 最后一位时置TMS为1
        if (i == length - 1) {
            JTAG_TMS_Out(1);
        } else {
            JTAG_TMS_Out(0);
        }
        
        JTAG_TDI_Out(bit);
        JTAG_Clock();
    }
    
    JTAG_Goto_State(TAP_STATE_UPDATE_DR);
    JTAG_Goto_State(TAP_STATE_IDLE);
    
    return HAL_OK;
}

HAL_StatusTypeDef JTAG_ReadBits(uint8_t *data_out, uint32_t length)
{
    memset(data_out, 0, (length + 7) / 8);
    
    JTAG_Goto_State(TAP_STATE_SHIFT_DR);
    
    for (uint32_t i = 0; i < length; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        
        // 最后一位时置TMS为1
        if (i == length - 1) {
            JTAG_TMS_Out(1);
        } else {
            JTAG_TMS_Out(0);
        }
        
        JTAG_TDI_Out(0);  // 发送0
        uint8_t bit = JTAG_TDO_In();
        JTAG_Clock();
        
        if (bit) {
            data_out[byte_idx] |= (1 << bit_idx);
        }
    }
    
    JTAG_Goto_State(TAP_STATE_UPDATE_DR);
    JTAG_Goto_State(TAP_STATE_IDLE);
    
    return HAL_OK;
}

HAL_StatusTypeDef JTAG_Write_IR_Bits(uint8_t *ir, uint32_t ir_length, uint8_t *dr, uint32_t dr_length, uint8_t *dr_out)
{
    // 写入IR
    JTAG_Goto_State(TAP_STATE_SHIFT_IR);
    
    for (uint32_t i = 0; i < ir_length; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        uint8_t bit = (ir[byte_idx] >> bit_idx) & 0x01;
        
        if (i == ir_length - 1) {
            JTAG_TMS_Out(1);
        } else {
            JTAG_TMS_Out(0);
        }
        
        JTAG_TDI_Out(bit);
        JTAG_Clock();
    }
    
    JTAG_Goto_State(TAP_STATE_UPDATE_IR);
    
    // 写入/读取DR
    JTAG_Goto_State(TAP_STATE_SHIFT_DR);
    
    if (dr_out == NULL) {
        // 仅写入
        for (uint32_t i = 0; i < dr_length; i++) {
            uint32_t byte_idx = i / 8;
            uint32_t bit_idx = i % 8;
            uint8_t bit = (dr[byte_idx] >> bit_idx) & 0x01;
            
            if (i == dr_length - 1) {
                JTAG_TMS_Out(1);
            } else {
                JTAG_TMS_Out(0);
            }
            
            JTAG_TDI_Out(bit);
            JTAG_Clock();
        }
    } else {
        // 写入并读取
        memset(dr_out, 0, (dr_length + 7) / 8);
        
        for (uint32_t i = 0; i < dr_length; i++) {
            uint32_t byte_idx = i / 8;
            uint32_t bit_idx = i % 8;
            uint8_t bit = (dr[byte_idx] >> bit_idx) & 0x01;
            
            if (i == dr_length - 1) {
                JTAG_TMS_Out(1);
            } else {
                JTAG_TMS_Out(0);
            }
            
            JTAG_TDI_Out(bit);
            uint8_t read_bit = JTAG_TDO_In();
            JTAG_Clock();
            
            if (read_bit) {
                dr_out[byte_idx] |= (1 << bit_idx);
            }
        }
    }
    
    JTAG_Goto_State(TAP_STATE_UPDATE_DR);
    JTAG_Goto_State(TAP_STATE_IDLE);
    
    return HAL_OK;
}
