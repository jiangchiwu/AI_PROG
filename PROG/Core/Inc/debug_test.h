/**
 ******************************************************************************
 * @file    debug_test.h
 * @brief   调试接口测试框架
 *          用于测试SBW/BDM/MON8/FINE接口功能
 ******************************************************************************
 */

#ifndef __DEBUG_TEST_H__
#define __DEBUG_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "debug_if.h"

#define DEBUG_TEST_PASS    0x00
#define DEBUG_TEST_FAIL    0x01
#define DEBUG_TEST_SKIP    0x02

typedef struct {
    const char* test_name;
    uint8_t (*test_func)(void);
    uint8_t result;
    uint32_t elapsed_ms;
} Debug_Test_TypeDef;

typedef struct {
    uint32_t total_tests;
    uint32_t passed;
    uint32_t failed;
    uint32_t skipped;
    uint32_t total_time_ms;
} Debug_Test_Report_TypeDef;

extern Debug_Test_Report_TypeDef g_test_report;

void Debug_Test_Init(void);
void Debug_Test_DeInit(void);

uint8_t Debug_Test_SBW(void);
uint8_t Debug_Test_BDM(void);
uint8_t Debug_Test_MON8(void);
uint8_t Debug_Test_FINE(void);
uint8_t Debug_Test_SWD(void);

void Debug_Test_RunAll(void);
void Debug_Test_Run(Debug_IF_TypeDef if_type);

void Debug_Test_PrintReport(void);
const char* Debug_Test_ResultStr(uint8_t result);

void Debug_Test_SetSpeed(uint32_t speed_hz);
uint32_t Debug_Test_GetSpeed(void);

#ifdef __cplusplus
}
#endif

#endif /* __DEBUG_TEST_H__ */
