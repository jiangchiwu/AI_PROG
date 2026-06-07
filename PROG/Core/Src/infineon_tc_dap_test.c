/**
 ******************************************************************************
 * @file    infineon_tc_dap_test.c
 * @brief   英飞凌TriCore TC系列 DAP 测试程序
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 * 
 * @details 本文件提供英飞凌TC系列DAP驱动的测试函数
 * 
 * @note    测试函数用于验证DAP驱动的基本功能
 * @note    实际使用时需要根据硬件连接配置GPIO引脚
 ******************************************************************************
 */

#include "infineon_tc_dap.h"

/**
 * @brief 测试TC DAP基本连接
 * @param htc_dap: TC DAP句柄
 * @return HAL状态
 */
HAL_StatusTypeDef TC_DAP_Test_Connection(TC_DAP_HandleTypeDef* htc_dap)
{
    HAL_StatusTypeDef status;
    uint32_t idcode;
    
    /* 初始化DAP接口 */
    status = TC_DAP_Init(htc_dap);
    if (status != HAL_OK) {
        printf("[FAIL] TC DAP Init failed\r\n");
        return status;
    }
    printf("[PASS] TC DAP Init success\r\n");
    
    /* 连接目标设备 */
    status = TC_DAP_Connect(htc_dap);
    if (status != HAL_OK) {
        printf("[FAIL] TC DAP Connect failed\r\n");
        return status;
    }
    printf("[PASS] TC DAP Connect success\r\n");
    
    /* 读取DP IDCODE */
    idcode = TC_DAP_ReadDP(htc_dap, TC_DAP_DP_IDCODE);
    printf("[INFO] DP IDCODE: 0x%08X\r\n", (unsigned int)idcode);
    
    /* 读取AP IDR */
    printf("[INFO] AP IDR: 0x%08X\r\n", (unsigned int)htc_dap->ap_idr);
    
    return HAL_OK;
}

/**
 * @brief 测试TC DAP内存读写
 * @param htc_dap: TC DAP句柄
 * @param test_addr: 测试地址
 * @return HAL状态
 */
HAL_StatusTypeDef TC_DAP_Test_MemoryAccess(TC_DAP_HandleTypeDef* htc_dap, uint32_t test_addr)
{
    uint32_t test_value = 0x12345678;
    uint32_t read_value;
    uint8_t test_buffer[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint8_t read_buffer[8];
    
    /* 测试单字读写 */
    printf("[INFO] Testing single word R/W at 0x%08X...\r\n", (unsigned int)test_addr);
    
    TC_DAP_WriteWord(htc_dap, test_addr, test_value);
    TC_DAP_ReadWord(htc_dap, test_addr, &read_value);
    
    if (read_value == test_value) {
        printf("[PASS] Single word R/W test: 0x%08X == 0x%08X\r\n", 
               (unsigned int)read_value, (unsigned int)test_value);
    } else {
        printf("[FAIL] Single word R/W test: 0x%08X != 0x%08X\r\n", 
               (unsigned int)read_value, (unsigned int)test_value);
        return HAL_ERROR;
    }
    
    /* 测试多字节读写 */
    printf("[INFO] Testing multi-byte R/W at 0x%08X...\r\n", (unsigned int)test_addr);
    
    TC_DAP_WriteMem(htc_dap, test_addr, test_buffer, 8);
    TC_DAP_ReadMem(htc_dap, test_addr, read_buffer, 8);
    
    bool match = true;
    for (int i = 0; i < 8; i++) {
        if (read_buffer[i] != test_buffer[i]) {
            match = false;
            break;
        }
    }
    
    if (match) {
        printf("[PASS] Multi-byte R/W test\r\n");
    } else {
        printf("[FAIL] Multi-byte R/W test\r\n");
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief TC DAP示例代码
 * @note  展示如何使用TC DAP驱动连接和访问英飞凌TC系列芯片
 */
void TC_DAP_Example(void)
{
    TC_DAP_HandleTypeDef htc_dap = {0};
    
    /* 配置GPIO引脚 - 需要根据实际硬件连接配置 */
    htc_dap.swdio_port = GPIOB;
    htc_dap.swdio_pin = GPIO_PIN_3;
    htc_dap.swclk_port = GPIOB;
    htc_dap.swclk_pin = GPIO_PIN_4;
    htc_dap.nrst_port = GPIOB;
    htc_dap.nrst_pin = GPIO_PIN_5;
    
    /* 设置通信速度 */
    TC_DAP_SetSpeed(&htc_dap, TC_DAP_CLOCK_1MHZ);
    
    /* 测试连接 */
    if (TC_DAP_Test_Connection(&htc_dap) != HAL_OK) {
        printf("[ERROR] TC DAP connection test failed\r\n");
        return;
    }
    
    /* 测试内存访问 - 使用SRAM地址 */
    uint32_t test_addr = 0x20000000;  /* SRAM起始地址 */
    if (TC_DAP_Test_MemoryAccess(&htc_dap, test_addr) != HAL_OK) {
        printf("[ERROR] TC DAP memory access test failed\r\n");
        return;
    }
    
    /* 断开连接 */
    TC_DAP_Disconnect(&htc_dap);
    TC_DAP_DeInit(&htc_dap);
    
    printf("[INFO] All tests passed!\r\n");
}
