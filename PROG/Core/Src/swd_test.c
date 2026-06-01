/**
  ******************************************************************************
  * @file    swd_test.c
  * @brief   SWD协议测试程序
  *          用于测试SWD连接和基本功能
  ******************************************************************************
  */

#include "main.h"
#include "swd.h"
#include "lcd.h"

// 暂时不使用UART输出，避免依赖未配置的外设
void SWD_Test_Print(const char *str)
{
    (void)str;
    // 实际使用时可通过LCD或其他方式输出
}

void SWD_Test_Connection(void)
{
    SWD_Test_Print("\r\n========== SWD Connection Test ==========\r\n");

    SWD_Config_TypeDef config = {
        .swdio_port = GPIOA,
        .swdio_pin  = GPIO_PIN_13,
        .swclk_port = GPIOA,
        .swclk_pin  = GPIO_PIN_14,
        .reset_port = GPIOA,
        .reset_pin  = GPIO_PIN_15,
        .clock      = SWD_CLOCK_4MHZ,
    };

    SWD_Init(&config);

    SWD_Test_Print("SWD Initialized\r\n");

    SWD_LineReset();

    SWD_Test_Print("Line Reset Done\r\n");

    uint32_t dp_idcode = SWD_ReadDP(0x00);

    char buffer[128];
    sprintf(buffer, "DP IDCODE: 0x%08X\r\n", (unsigned int)dp_idcode);
    SWD_Test_Print(buffer);

    if (dp_idcode == 0x4BA01477) {
        SWD_Test_Print("✓ SWD Connection: SUCCESS\r\n");
    } else if (dp_idcode == 0x06411041) {
        SWD_Test_Print("✓ SWD Connection: SUCCESS (STM32F1)\r\n");
    } else if (dp_idcode == 0x4BA02477) {
        SWD_Test_Print("✓ SWD Connection: SUCCESS (STM32F4)\r\n");
    } else if (dp_idcode == 0x6B461047) {
        SWD_Test_Print("✓ SWD Connection: SUCCESS (STM32H7)\r\n");
    } else {
        SWD_Test_Print("✗ SWD Connection: FAILED\r\n");
    }

    uint32_t ctrl_stat = SWD_ReadDP(0x04);
    sprintf(buffer, "CTRL/STAT: 0x%08X\r\n", (unsigned int)ctrl_stat);
    SWD_Test_Print(buffer);

    SWD_Test_Print("=========================================\r\n");
}

void SWD_Test_Memory(void)
{
    SWD_Test_Print("\r\n========== SWD Memory Test ==========\r\n");

    uint32_t test_addr = 0x20000000;

    SWD_WriteWord(0xE000ED00, 0x20000000);

    uint32_t readback = SWD_ReadWord(0xE000ED00);
    char buffer[128];
    sprintf(buffer, "Read 0xE000ED00: 0x%08X\r\n", (unsigned int)readback);
    SWD_Test_Print(buffer);

    uint8_t test_data[8] = {0x55, 0xAA, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};

    SWD_WriteMem(test_addr, test_data, 8);

    uint8_t read_data[8];
    SWD_ReadMem(test_addr, read_data, 8);

    sprintf(buffer, "Written: ");
    SWD_Test_Print(buffer);
    for (uint8_t i = 0; i < 8; i++) {
        sprintf(buffer, "%02X ", test_data[i]);
        SWD_Test_Print(buffer);
    }
    SWD_Test_Print("\r\n");

    sprintf(buffer, "Read:    ");
    SWD_Test_Print(buffer);
    for (uint8_t i = 0; i < 8; i++) {
        sprintf(buffer, "%02X ", read_data[i]);
        SWD_Test_Print(buffer);
    }
    SWD_Test_Print("\r\n");

    uint8_t match = 1;
    for (uint8_t i = 0; i < 8; i++) {
        if (test_data[i] != read_data[i]) {
            match = 0;
            break;
        }
    }

    if (match) {
        SWD_Test_Print("✓ Memory Test: PASS\r\n");
    } else {
        SWD_Test_Print("✗ Memory Test: FAIL\r\n");
    }

    SWD_Test_Print("=========================================\r\n");
}

void SWD_Test_AP(void)
{
    SWD_Test_Print("\r\n========== SWD AP Test ==========\r\n");

    uint8_t ap = 0;

    uint32_t ap_idr = SWD_ReadAPReg(ap, 0xFC);
    char buffer[128];
    sprintf(buffer, "AHB-AP IDR: 0x%08X\r\n", (unsigned int)ap_idr);
    SWD_Test_Print(buffer);

    if (ap_idr != 0) {
        SWD_Test_Print("✓ AP Access: SUCCESS\r\n");
    } else {
        SWD_Test_Print("✗ AP Access: FAILED\r\n");
    }

    SWD_WriteAPReg(ap, 0x00, 0x23000012);

    uint32_t ap_csw = SWD_ReadAPReg(ap, 0x00);
    sprintf(buffer, "AHB-AP CSW: 0x%08X\r\n", (unsigned int)ap_csw);
    SWD_Test_Print(buffer);

    SWD_Test_Print("=========================================\r\n");
}

void SWD_Test_All(void)
{
    SWD_Test_Connection();
    HAL_Delay(100);
    SWD_Test_AP();
    HAL_Delay(100);
    SWD_Test_Memory();
}
