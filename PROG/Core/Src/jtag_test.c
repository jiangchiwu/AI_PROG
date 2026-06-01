/**
 ******************************************************************************
 * @file    jtag_test.c
 * @brief   JTAG 协议测试程序
 ******************************************************************************
 */

#include "jtag.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

// 暂时不使用printf，避免依赖未配置的外设
#define printf(...) 

void JTAG_Test_Connection(void)
{
    (void)printf;
    JTAG_Config_TypeDef config = {
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
        .clock = JTAG_CLOCK_4MHZ,
    };

    JTAG_Init(&config);
    JTAG_TAP_Reset();
    JTAG_DetectChain();
}

void JTAG_Test_IR_DR(void)
{
    JTAG_GetIDCode();
}

void JTAG_Test_StateMachine(void)
{
    JTAG_Goto_State(TAP_STATE_IDLE);
    JTAG_Goto_State(TAP_STATE_SHIFT_IR);
    JTAG_Goto_State(TAP_STATE_IDLE);
    JTAG_Goto_State(TAP_STATE_SHIFT_DR);
    JTAG_Goto_State(TAP_STATE_IDLE);
    JTAG_TAP_Reset();
}

void JTAG_Test_All(void)
{
    JTAG_Test_Connection();
    HAL_Delay(100);

    JTAG_Test_StateMachine();
    HAL_Delay(100);

    JTAG_Test_IR_DR();
    HAL_Delay(100);
}
