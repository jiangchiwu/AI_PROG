/**
 ******************************************************************************
 * @file    gpio_soft.c
 * @brief   GPIO抽象层实现 - 软件模拟GPIO接口
 *          用于软件模拟各种协议（IIC/SPI/SWD/JTAG等）
 ******************************************************************************
 */

#include "gpio_soft.h"

SOFT_GPIO_Ops_TypeDef g_gpio_ops;

static GPIO_PinState HAL_State_Map[SOFT_GPIO_STATE_HIGH + 1] = {
    [SOFT_GPIO_STATE_LOW]  = GPIO_PIN_RESET,
    [SOFT_GPIO_STATE_HIGH] = GPIO_PIN_SET,
};

static void GPIO_Soft_Enable_Clock(GPIO_TypeDef *port)
{
    if (port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    } else if (port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    } else if (port == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    } else if (port == GPIOF) {
        __HAL_RCC_GPIOF_CLK_ENABLE();
    } else if (port == GPIOG) {
        __HAL_RCC_GPIOG_CLK_ENABLE();
    } else if (port == GPIOH) {
        __HAL_RCC_GPIOH_CLK_ENABLE();
    }
}

static uint32_t Pull_Map[SOFT_GPIO_PULL_UP_DOWN + 1] = {
    [SOFT_GPIO_PULL_NONE]    = GPIO_NOPULL,
    [SOFT_GPIO_PULL_UP]      = GPIO_PULLUP,
    [SOFT_GPIO_PULL_DOWN]    = GPIO_PULLDOWN,
    [SOFT_GPIO_PULL_UP_DOWN] = GPIO_PULLUP,
};

static uint32_t Mode_Map[SOFT_GPIO_MODE_ANALOG + 1] = {
    [SOFT_GPIO_MODE_INPUT]      = GPIO_MODE_INPUT,
    [SOFT_GPIO_MODE_OUTPUT_PP]  = GPIO_MODE_OUTPUT_PP,
    [SOFT_GPIO_MODE_OUTPUT_OD]  = GPIO_MODE_OUTPUT_OD,
    [SOFT_GPIO_MODE_ANALOG]     = GPIO_MODE_ANALOG,
};

void GPIO_Soft_Init(void)
{
    g_gpio_ops.read_pin    = GPIO_Soft_ReadPin;
    g_gpio_ops.write_pin   = GPIO_Soft_WritePin;
    g_gpio_ops.toggle_pin  = GPIO_Soft_TogglePin;
    g_gpio_ops.set_mode    = GPIO_Soft_SetMode;
}

SOFT_GPIO_State_TypeDef GPIO_Soft_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_PinState state = HAL_GPIO_ReadPin(port, pin);
    return (SOFT_GPIO_State_TypeDef)(state == GPIO_PIN_SET ? SOFT_GPIO_STATE_HIGH : SOFT_GPIO_STATE_LOW);
}

void GPIO_Soft_WritePin(GPIO_TypeDef *port, uint16_t pin, SOFT_GPIO_State_TypeDef state)
{
    HAL_GPIO_WritePin(port, pin, HAL_State_Map[state]);
}

void GPIO_Soft_TogglePin(GPIO_TypeDef *port, uint16_t pin)
{
    HAL_GPIO_TogglePin(port, pin);
}

void GPIO_Soft_SetMode(GPIO_TypeDef *port, uint16_t pin, SOFT_GPIO_Mode_TypeDef mode, SOFT_GPIO_Pull_TypeDef pull)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_Soft_Enable_Clock(port);

    GPIO_InitStruct.Pin   = pin;
    GPIO_InitStruct.Mode  = Mode_Map[mode];
    GPIO_InitStruct.Pull  = Pull_Map[pull];
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    if (mode == SOFT_GPIO_MODE_OUTPUT_OD || mode == SOFT_GPIO_MODE_OUTPUT_PP) {
        GPIO_InitStruct.Mode = Mode_Map[mode];
    }

    HAL_GPIO_Init(port, &GPIO_InitStruct);

    if (mode == SOFT_GPIO_MODE_OUTPUT_OD) {
        if (pull == SOFT_GPIO_PULL_UP || pull == SOFT_GPIO_PULL_UP_DOWN) {
            HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
        }
    }
}

SOFT_GPIO_State_TypeDef GPIO_Soft_ReadBit(GPIO_TypeDef *port, uint16_t pin)
{
    return GPIO_Soft_ReadPin(port, pin);
}

void GPIO_Soft_WriteBit(GPIO_TypeDef *port, uint16_t pin, SOFT_GPIO_State_TypeDef state)
{
    GPIO_Soft_WritePin(port, pin, state);
}
