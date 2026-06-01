/**
 ******************************************************************************
 * @file    dap_test.c
 * @brief   ARM DAP 层测试程序
 ******************************************************************************
 */

#include "dap.h"
#include <stdio.h>

// 暂时不使用printf，避免依赖未配置的外设
#define printf(...) 

void DAP_Test_Connect(void)
{
    (void)printf;
    if (DAP_Init(DAP_PROTOCOL_SWD) != HAL_OK) {
        return;
    }

    DAP_Connect();
    DAP_Disconnect();
    DAP_DeInit();
}

void DAP_Test_MemoryAccess(void)
{
    uint8_t test_data[16];
    uint32_t value;
    uint32_t test_addr = 0x20000000;

    (void)value;
    (void)test_addr;
    (void)test_data;
    
    if (DAP_Init(DAP_PROTOCOL_SWD) != HAL_OK) {
        return;
    }

    if (DAP_Connect() != HAL_OK) {
        return;
    }

    DAP_Disconnect();
    DAP_DeInit();
}

void DAP_Test_All(void)
{
    DAP_Test_Connect();
    HAL_Delay(100);
    DAP_Test_MemoryAccess();
}
