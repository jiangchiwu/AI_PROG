/**
 ******************************************************************************
 * @file    infineon_tc_dap_test.h
 * @brief   英飞凌TriCore TC系列 DAP 测试头文件
 ******************************************************************************
 */

#ifndef __INFINEON_TC_DAP_TEST_H__
#define __INFINEON_TC_DAP_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "infineon_tc_dap.h"

HAL_StatusTypeDef TC_DAP_Test_Connection(TC_DAP_HandleTypeDef* htc_dap);
HAL_StatusTypeDef TC_DAP_Test_MemoryAccess(TC_DAP_HandleTypeDef* htc_dap, uint32_t test_addr);
void TC_DAP_Example(void);

#ifdef __cplusplus
}
#endif

#endif