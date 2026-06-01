/**
 ******************************************************************************
 * @file    gpio_soft.h
 * @brief   GPIO抽象层 - 软件模拟GPIO接口
 *          用于软件模拟各种协议（IIC/SPI/SWD/JTAG等）
 ******************************************************************************
 */

#ifndef __GPIO_SOFT_H__
#define __GPIO_SOFT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

#define GPIO_PORT_MAX  9
#define GPIO_PIN_MAX   16

typedef enum {
    SOFT_GPIO_MODE_INPUT          = 0x00,
    SOFT_GPIO_MODE_OUTPUT_PP      = 0x01,
    SOFT_GPIO_MODE_OUTPUT_OD      = 0x02,
    SOFT_GPIO_MODE_ANALOG         = 0x03,
} SOFT_GPIO_Mode_TypeDef;

typedef enum {
    SOFT_GPIO_PULL_NONE           = 0x00,
    SOFT_GPIO_PULL_UP             = 0x01,
    SOFT_GPIO_PULL_DOWN           = 0x02,
    SOFT_GPIO_PULL_UP_DOWN        = 0x03,
} SOFT_GPIO_Pull_TypeDef;

typedef enum {
    SOFT_GPIO_STATE_LOW           = 0x00,
    SOFT_GPIO_STATE_HIGH          = 0x01,
} SOFT_GPIO_State_TypeDef;

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} SOFT_GPIO_Pin_TypeDef;

typedef struct {
    SOFT_GPIO_Mode_TypeDef mode;
    SOFT_GPIO_Pull_TypeDef pull;
    SOFT_GPIO_State_TypeDef state;
    uint8_t reserved;
} SOFT_GPIO_Config_TypeDef;

typedef struct {
    SOFT_GPIO_State_TypeDef (*read_pin)(GPIO_TypeDef *port, uint16_t pin);
    void (*write_pin)(GPIO_TypeDef *port, uint16_t pin, SOFT_GPIO_State_TypeDef state);
    void (*toggle_pin)(GPIO_TypeDef *port, uint16_t pin);
    void (*set_mode)(GPIO_TypeDef *port, uint16_t pin, SOFT_GPIO_Mode_TypeDef mode, SOFT_GPIO_Pull_TypeDef pull);
} SOFT_GPIO_Ops_TypeDef;

extern SOFT_GPIO_Ops_TypeDef g_gpio_ops;

void GPIO_Soft_Init(void);
SOFT_GPIO_State_TypeDef GPIO_Soft_ReadPin(GPIO_TypeDef *port, uint16_t pin);
void GPIO_Soft_WritePin(GPIO_TypeDef *port, uint16_t pin, SOFT_GPIO_State_TypeDef state);
void GPIO_Soft_TogglePin(GPIO_TypeDef *port, uint16_t pin);
void GPIO_Soft_SetMode(GPIO_TypeDef *port, uint16_t pin, SOFT_GPIO_Mode_TypeDef mode, SOFT_GPIO_Pull_TypeDef pull);
SOFT_GPIO_State_TypeDef GPIO_Soft_ReadBit(GPIO_TypeDef *port, uint16_t pin);
void GPIO_Soft_WriteBit(GPIO_TypeDef *port, uint16_t pin, SOFT_GPIO_State_TypeDef state);

#define SOFT_GPIO_PORT_A   GPIOA
#define SOFT_GPIO_PORT_B   GPIOB
#define SOFT_GPIO_PORT_C   GPIOC
#define SOFT_GPIO_PORT_D   GPIOD
#define SOFT_GPIO_PORT_E   GPIOE
#define SOFT_GPIO_PORT_F   GPIOF
#define SOFT_GPIO_PORT_G   GPIOG
#define SOFT_GPIO_PORT_H   GPIOH
#define SOFT_GPIO_PORT_I   GPIOI

#define SOFT_GPIO_LOW      SOFT_GPIO_STATE_LOW
#define SOFT_GPIO_HIGH     SOFT_GPIO_STATE_HIGH

#define SOFT_GPIO_MODE_IN      SOFT_GPIO_MODE_INPUT
#define SOFT_GPIO_MODE_OUT_PP  SOFT_GPIO_MODE_OUTPUT_PP
#define SOFT_GPIO_MODE_OUT_OD  SOFT_GPIO_MODE_OUTPUT_OD

#define SOFT_GPIO_PULL_NONE    SOFT_GPIO_PULL_NONE
#define SOFT_GPIO_PULL_UP      SOFT_GPIO_PULL_UP
#define SOFT_GPIO_PULL_DOWN    SOFT_GPIO_PULL_DOWN
#define SOFT_GPIO_PULL_BOTH    SOFT_GPIO_PULL_UP_DOWN

#ifdef __cplusplus
}
#endif

#endif /* __GPIO_SOFT_H__ */
