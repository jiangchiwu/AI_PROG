/**
 ******************************************************************************
 * @file    debug_test.c
 * @brief   调试接口测试框架
 *          用于测试SBW/BDM/MON8/FINE接口功能
 ******************************************************************************
 */

#include "debug_test.h"
#include "usb_cdc.h"

Debug_Test_Report_TypeDef g_test_report = {0};

static Debug_Test_TypeDef g_tests[] = {
    {"SBW Interface",  Debug_Test_SBW,  DEBUG_TEST_SKIP, 0},
    {"BDM Interface",  Debug_Test_BDM,  DEBUG_TEST_SKIP, 0},
    {"MON8 Interface", Debug_Test_MON8, DEBUG_TEST_SKIP, 0},
    {"FINE Interface", Debug_Test_FINE, DEBUG_TEST_SKIP, 0},
    {"SWD Interface",  Debug_Test_SWD,  DEBUG_TEST_SKIP, 0},
};

#define TEST_COUNT (sizeof(g_tests) / sizeof(Debug_Test_TypeDef))

static uint32_t g_test_speed = 100000;

void Debug_Test_Init(void)
{
    memset(&g_test_report, 0, sizeof(Debug_Test_Report_TypeDef));
    
    for (uint32_t i = 0; i < TEST_COUNT; i++) {
        g_tests[i].result = DEBUG_TEST_SKIP;
        g_tests[i].elapsed_ms = 0;
    }
}

void Debug_Test_DeInit(void)
{
    Debug_IF_DeInit();
}

void Debug_Test_SetSpeed(uint32_t speed_hz)
{
    g_test_speed = speed_hz;
}

uint32_t Debug_Test_GetSpeed(void)
{
    return g_test_speed;
}

static uint32_t Test_GetTime(void)
{
    return HAL_GetTick();
}

uint8_t Debug_Test_SBW(void)
{
    uint32_t start_time = Test_GetTime();
    uint8_t result = DEBUG_TEST_FAIL;
    
    if (Debug_IF_Init(DEBUG_IF_SBW, g_test_speed) != HAL_OK) {
        return DEBUG_TEST_FAIL;
    }
    
    if (Debug_IF_Enter() != HAL_OK) {
        Debug_IF_DeInit();
        return DEBUG_TEST_FAIL;
    }
    
    uint32_t chip_id = Debug_IF_GetChipID();
    if (chip_id != 0) {
        result = DEBUG_TEST_PASS;
    }
    
    Debug_IF_Exit();
    Debug_IF_DeInit();
    
    return result;
}

uint8_t Debug_Test_BDM(void)
{
    uint32_t start_time = Test_GetTime();
    uint8_t result = DEBUG_TEST_FAIL;
    
    if (Debug_IF_Init(DEBUG_IF_BDM, g_test_speed) != HAL_OK) {
        return DEBUG_TEST_FAIL;
    }
    
    if (Debug_IF_Enter() != HAL_OK) {
        Debug_IF_DeInit();
        return DEBUG_TEST_FAIL;
    }
    
    uint8_t test_data[8] = {0x55, 0xAA, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    
    if (Debug_IF_WriteMem(0x100, test_data, 8) != HAL_OK) {
        Debug_IF_Exit();
        Debug_IF_DeInit();
        return DEBUG_TEST_FAIL;
    }
    
    uint8_t read_data[8] = {0};
    if (Debug_IF_ReadMem(0x100, read_data, 8) != HAL_OK) {
        Debug_IF_Exit();
        Debug_IF_DeInit();
        return DEBUG_TEST_FAIL;
    }
    
    for (int i = 0; i < 8; i++) {
        if (read_data[i] != test_data[i]) {
            Debug_IF_Exit();
            Debug_IF_DeInit();
            return DEBUG_TEST_FAIL;
        }
    }
    
    result = DEBUG_TEST_PASS;
    
    Debug_IF_Exit();
    Debug_IF_DeInit();
    
    return result;
}

uint8_t Debug_Test_MON8(void)
{
    uint32_t start_time = Test_GetTime();
    uint8_t result = DEBUG_TEST_FAIL;
    
    if (Debug_IF_Init(DEBUG_IF_MON8, g_test_speed) != HAL_OK) {
        return DEBUG_TEST_FAIL;
    }
    
    if (Debug_IF_Enter() != HAL_OK) {
        Debug_IF_DeInit();
        return DEBUG_TEST_FAIL;
    }
    
    uint8_t version = Debug_IF_GetVersion();
    if (version != 0) {
        result = DEBUG_TEST_PASS;
    }
    
    Debug_IF_Exit();
    Debug_IF_DeInit();
    
    return result;
}

uint8_t Debug_Test_FINE(void)
{
    uint32_t start_time = Test_GetTime();
    uint8_t result = DEBUG_TEST_FAIL;
    
    if (Debug_IF_Init(DEBUG_IF_FINE, g_test_speed) != HAL_OK) {
        return DEBUG_TEST_FAIL;
    }
    
    if (Debug_IF_Enter() != HAL_OK) {
        Debug_IF_DeInit();
        return DEBUG_TEST_FAIL;
    }
    
    uint32_t chip_id = Debug_IF_GetChipID();
    if (chip_id != 0) {
        result = DEBUG_TEST_PASS;
    }
    
    Debug_IF_Exit();
    Debug_IF_DeInit();
    
    return result;
}

uint8_t Debug_Test_SWD(void)
{
    uint32_t start_time = Test_GetTime();
    uint8_t result = DEBUG_TEST_FAIL;
    
    if (Debug_IF_Init(DEBUG_IF_SWD, g_test_speed) != HAL_OK) {
        return DEBUG_TEST_FAIL;
    }
    
    if (Debug_IF_Enter() != HAL_OK) {
        Debug_IF_DeInit();
        return DEBUG_TEST_FAIL;
    }
    
    uint32_t dp_id = SWD_GetDPID();
    if (dp_id != 0) {
        result = DEBUG_TEST_PASS;
    }
    
    Debug_IF_Exit();
    Debug_IF_DeInit();
    
    return result;
}

void Debug_Test_RunAll(void)
{
    Debug_Test_Init();
    
    uint32_t start_time = Test_GetTime();
    
    for (uint32_t i = 0; i < TEST_COUNT; i++) {
        uint32_t test_start = Test_GetTime();
        g_tests[i].result = g_tests[i].test_func();
        g_tests[i].elapsed_ms = Test_GetTime() - test_start;
        
        g_test_report.total_tests++;
        if (g_tests[i].result == DEBUG_TEST_PASS) {
            g_test_report.passed++;
        } else if (g_tests[i].result == DEBUG_TEST_FAIL) {
            g_test_report.failed++;
        } else {
            g_test_report.skipped++;
        }
    }
    
    g_test_report.total_time_ms = Test_GetTime() - start_time;
}

void Debug_Test_Run(Debug_IF_TypeDef if_type)
{
    Debug_Test_Init();
    
    uint32_t test_idx = 0;
    switch (if_type) {
        case DEBUG_IF_SBW:  test_idx = 0; break;
        case DEBUG_IF_BDM:  test_idx = 1; break;
        case DEBUG_IF_MON8: test_idx = 2; break;
        case DEBUG_IF_FINE: test_idx = 3; break;
        case DEBUG_IF_SWD:  test_idx = 4; break;
        default: return;
    }
    
    uint32_t start_time = Test_GetTime();
    g_tests[test_idx].result = g_tests[test_idx].test_func();
    g_tests[test_idx].elapsed_ms = Test_GetTime() - start_time;
    
    g_test_report.total_tests = 1;
    g_test_report.total_time_ms = g_tests[test_idx].elapsed_ms;
    
    if (g_tests[test_idx].result == DEBUG_TEST_PASS) {
        g_test_report.passed = 1;
        g_test_report.failed = 0;
    } else if (g_tests[test_idx].result == DEBUG_TEST_FAIL) {
        g_test_report.passed = 0;
        g_test_report.failed = 1;
    } else {
        g_test_report.skipped = 1;
    }
}

const char* Debug_Test_ResultStr(uint8_t result)
{
    switch (result) {
        case DEBUG_TEST_PASS: return "PASS";
        case DEBUG_TEST_FAIL: return "FAIL";
        case DEBUG_TEST_SKIP: return "SKIP";
        default: return "UNKNOWN";
    }
}

void Debug_Test_PrintReport(void)
{
    char buffer[256];
    int len;
    
    len = sprintf(buffer, "\r\n========== Debug Interface Test Report ==========\r\n");
    CDC_Transmit_FS((uint8_t*)buffer, len);
    
    for (uint32_t i = 0; i < TEST_COUNT; i++) {
        len = sprintf(buffer, "[%s] %s (%"PRIu32" ms)\r\n",
                      Debug_Test_ResultStr(g_tests[i].result),
                      g_tests[i].test_name,
                      g_tests[i].elapsed_ms);
        CDC_Transmit_FS((uint8_t*)buffer, len);
    }
    
    len = sprintf(buffer, "----------------------------------------------\r\n");
    CDC_Transmit_FS((uint8_t*)buffer, len);
    
    len = sprintf(buffer, "Total: %"PRIu32", Passed: %"PRIu32", Failed: %"PRIu32", Skipped: %"PRIu32"\r\n",
                  g_test_report.total_tests,
                  g_test_report.passed,
                  g_test_report.failed,
                  g_test_report.skipped);
    CDC_Transmit_FS((uint8_t*)buffer, len);
    
    len = sprintf(buffer, "Total Time: %"PRIu32" ms\r\n", g_test_report.total_time_ms);
    CDC_Transmit_FS((uint8_t*)buffer, len);
    
    len = sprintf(buffer, "==============================================\r\n");
    CDC_Transmit_FS((uint8_t*)buffer, len);
}
