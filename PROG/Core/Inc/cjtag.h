
#ifndef __CJTAG_H
#define __CJTAG_H

#include "stm32h7xx_hal.h"
#include &lt;stdint.h&gt;

#define CJTAG_TCK_PIN GPIO_PIN_0
#define CJTAG_TMS_PIN GPIO_PIN_1
#define CJTAG_TDI_PIN GPIO_PIN_2
#define CJTAG_TDO_PIN GPIO_PIN_3
#define CJTAG_RTCK_PIN GPIO_PIN_4

typedef struct {
    GPIO_TypeDef* tck_port;
    uint16_t tck_pin;
    GPIO_TypeDef* tms_port;
    uint16_t tms_pin;
    GPIO_TypeDef* tdi_port;
    uint16_t tdi_pin;
    GPIO_TypeDef* tdo_port;
    uint16_t tdo_pin;
    GPIO_TypeDef* rtck_port;
    uint16_t rtck_pin;
    uint32_t speed_hz;
    uint8_t mode; 
} CJTAG_HandleTypeDef;

typedef enum {
    CJTAG_STATE_TEST_LOGIC_RESET = 0,
    CJTAG_STATE_RUN_TEST_IDLE,
    CJTAG_STATE_SELECT_DR_SCAN,
    CJTAG_STATE_CAPTURE_DR,
    CJTAG_STATE_SHIFT_DR,
    CJTAG_STATE_EXIT1_DR,
    CJTAG_STATE_PAUSE_DR,
    CJTAG_STATE_EXIT2_DR,
    CJTAG_STATE_UPDATE_DR,
    CJTAG_STATE_SELECT_IR_SCAN,
    CJTAG_STATE_CAPTURE_IR,
    CJTAG_STATE_SHIFT_IR,
    CJTAG_STATE_EXIT1_IR,
    CJTAG_STATE_PAUSE_IR,
    CJTAG_STATE_EXIT2_IR,
    CJTAG_STATE_UPDATE_IR
} CJTAG_TAP_State_t;

HAL_StatusTypeDef CJTAG_Init(CJTAG_HandleTypeDef* hcjtag);
HAL_StatusTypeDef CJTAG_DeInit(CJTAG_HandleTypeDef* hcjtag);
HAL_StatusTypeDef CJTAG_Reset(CJTAG_HandleTypeDef* hcjtag);
HAL_StatusTypeDef CJTAG_SwitchToJTAG(CJTAG_HandleTypeDef* hcjtag);
HAL_StatusTypeDef CJTAG_SwitchToCJTAG(CJTAG_HandleTypeDef* hcjtag);
HAL_StatusTypeDef CJTAG_WriteIR(CJTAG_HandleTypeDef* hcjtag, uint8_t* ir_data, uint16_t ir_length);
HAL_StatusTypeDef CJTAG_ReadIR(CJTAG_HandleTypeDef* hcjtag, uint8_t* ir_data, uint16_t ir_length);
HAL_StatusTypeDef CJTAG_WriteDR(CJTAG_HandleTypeDef* hcjtag, uint8_t* dr_data, uint16_t dr_length);
HAL_StatusTypeDef CJTAG_ReadDR(CJTAG_HandleTypeDef* hcjtag, uint8_t* dr_data, uint16_t dr_length);
HAL_StatusTypeDef CJTAG_TAP_GotoState(CJTAG_HandleTypeDef* hcjtag, CJTAG_TAP_State_t target_state);
uint32_t CJTAG_ReadIDCODE(CJTAG_HandleTypeDef* hcjtag);

#endif
