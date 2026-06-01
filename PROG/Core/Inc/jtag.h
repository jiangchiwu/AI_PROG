/**
 ******************************************************************************
 * @file    jtag.h
 * @brief   JTAG (Joint Test Action Group) 协议实现
 *          IEEE 1149.1 边界扫描标准
 ******************************************************************************
 */

#ifndef __JTAG_H__
#define __JTAG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

#define JTAG_OK              0x00
#define JTAG_ERR            0x01
#define JTAG_ERR_TIMEOUT    0x02
#define JTAG_ERR_NO_TAP     0x03

#define JTAG_CLOCK_1MHZ    1000000
#define JTAG_CLOCK_4MHZ    4000000
#define JTAG_CLOCK_10MHZ   10000000
#define JTAG_CLOCK_20MHZ   20000000

#define JTAG_DEFAULT_CLOCK  JTAG_CLOCK_4MHZ

typedef enum {
    TAP_STATE_RESET = 0,
    TAP_STATE_IDLE,
    TAP_STATE_SELECT_DR,
    TAP_STATE_CAPTURE_DR,
    TAP_STATE_SHIFT_DR,
    TAP_STATE_EXIT1_DR,
    TAP_STATE_PAUSE_DR,
    TAP_STATE_EXIT2_DR,
    TAP_STATE_UPDATE_DR,
    TAP_STATE_SELECT_IR,
    TAP_STATE_CAPTURE_IR,
    TAP_STATE_SHIFT_IR,
    TAP_STATE_EXIT1_IR,
    TAP_STATE_PAUSE_IR,
    TAP_STATE_EXIT2_IR,
    TAP_STATE_UPDATE_IR,
} JTAG_TAP_State_TypeDef;

typedef struct {
    GPIO_TypeDef *tck_port;
    uint16_t tck_pin;
    GPIO_TypeDef *tms_port;
    uint16_t tms_pin;
    GPIO_TypeDef *tdi_port;
    uint16_t tdi_pin;
    GPIO_TypeDef *tdo_port;
    uint16_t tdo_pin;
    GPIO_TypeDef *nrst_port;
    uint16_t nrst_pin;
    GPIO_TypeDef *ntrst_port;
    uint16_t ntrst_pin;

    uint32_t clock;
    uint8_t initialized;
} JTAG_Config_TypeDef;

typedef struct {
    JTAG_TAP_State_TypeDef tap_state;
    uint32_t ir_length;
    uint32_t current_ir;
    uint32_t idcode;
    uint8_t tap_count;
} JTAG_State_TypeDef;

extern JTAG_Config_TypeDef g_jtag_config;
extern JTAG_State_TypeDef g_jtag_state;

HAL_StatusTypeDef JTAG_Init(JTAG_Config_TypeDef *config);
HAL_StatusTypeDef JTAG_DeInit(void);

HAL_StatusTypeDef JTAG_Reset(void);
HAL_StatusTypeDef JTAG_TAP_Reset(void);
HAL_StatusTypeDef JTAG_Goto_State(JTAG_TAP_State_TypeDef state);

HAL_StatusTypeDef JTAG_Write_IR(uint32_t ir, uint32_t length);
uint32_t JTAG_Read_IR(uint32_t length);

HAL_StatusTypeDef JTAG_Write_DR(uint32_t dr, uint32_t length);
uint32_t JTAG_Read_DR(uint32_t length);

HAL_StatusTypeDef JTAG_Write_IR_DR(uint32_t ir, uint32_t ir_length, uint32_t dr, uint32_t dr_length);
uint32_t JTAG_Read_IR_DR(uint32_t ir, uint32_t ir_length, uint32_t dr_length);

HAL_StatusTypeDef JTAG_WriteBits(uint8_t *data, uint32_t length);
HAL_StatusTypeDef JTAG_ReadBits(uint8_t *data_out, uint32_t length);
HAL_StatusTypeDef JTAG_Write_IR_Bits(uint8_t *ir, uint32_t ir_length, uint8_t *dr, uint32_t dr_length, uint8_t *dr_out);

uint32_t JTAG_GetIDCode(void);
HAL_StatusTypeDef JTAG_DetectChain(void);

HAL_StatusTypeDef JTAG_SetClock(uint32_t clock);
uint32_t JTAG_GetClock(void);

HAL_StatusTypeDef JTAG_AssertReset(void);
HAL_StatusTypeDef JTAG_DeassertReset(void);

void JTAG_GPIO_Init(void);
void JTAG_GPIO_DeInit(void);

#ifdef __cplusplus
}
#endif

#endif /* __JTAG_H__ */
