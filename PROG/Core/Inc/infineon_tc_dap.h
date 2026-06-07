/**
 ******************************************************************************
 * @file    infineon_tc_dap.h
 * @brief   Infineon TriCore TC系列 DAP (Debug Access Port) 协议头文件
 ******************************************************************************
 */

#ifndef __INFINEON_TC_DAP_H__
#define __INFINEON_TC_DAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "swd.h"

#define TC_DAP_CLOCK_100KHZ   100000
#define TC_DAP_CLOCK_1MHZ     1000000
#define TC_DAP_CLOCK_5MHZ     5000000
#define TC_DAP_CLOCK_10MHZ    10000000

#define TC_DAP_DP_IDCODE      0x00
#define TC_DAP_DP_CTRL_STAT   0x04
#define TC_DAP_DP_RESEND      0x08
#define TC_DAP_DP_SELECT      0x0C
#define TC_DAP_DP_RDBUFF      0x10

#define TC_DAP_AP_CSW         0x00
#define TC_DAP_AP_TAR         0x04
#define TC_DAP_AP_DRW         0x0C
#define TC_DAP_AP_BD0         0x0C
#define TC_DAP_AP_BD1         0x10
#define TC_DAP_AP_BD2         0x14
#define TC_DAP_AP_BD3         0x18
#define TC_DAP_AP_IDR         0xFC

#define TC_DAP_CTRL_STAT_STICKYORUN    (1 << 30)
#define TC_DAP_CTRL_STAT_STICKYCMP     (1 << 29)
#define TC_DAP_CTRL_STAT_STICKYERR     (1 << 28)
#define TC_DAP_CTRL_STAT_WAITUP        (1 << 27)
#define TC_DAP_CTRL_STAT_WDATAERR      (1 << 25)
#define TC_DAP_CTRL_STAT_READOK        (1 << 24)
#define TC_DAP_CTRL_STAT_OVERUN        (1 << 23)
#define TC_DAP_CTRL_STAT_TRNNMOD       (1 << 11)
#define TC_DAP_CTRL_STAT_ORUNDETECT    (1 << 7)
#define TC_DAP_CTRL_STAT_STRESET       (1 << 6)
#define TC_DAP_CTRL_STAT_HALTCLK       (1 << 5)
#define TC_DAP_CTRL_STAT_CDBGPWRUPREQ  (1 << 2)
#define TC_DAP_CTRL_STAT_CDBGPWRUPACK  (1 << 1)
#define TC_DAP_CTRL_STAT_KEY           (1 << 0)

#define TC_DAP_AP_CSW_SIZE8     (0 << 0)
#define TC_DAP_AP_CSW_SIZE16    (1 << 0)
#define TC_DAP_AP_CSW_SIZE32    (2 << 0)
#define TC_DAP_AP_CSW_ADDRINC   (1 << 4)
#define TC_DAP_AP_CSW_DEFAULT   (0x23000012)

typedef struct {
    GPIO_TypeDef* swdio_port;
    uint16_t swdio_pin;
    GPIO_TypeDef* swclk_port;
    uint16_t swclk_pin;
    GPIO_TypeDef* nrst_port;
    uint16_t nrst_pin;
    
    uint32_t speed_hz;
    uint32_t prescaler;
    uint32_t period;
    uint32_t tick_ns;
    
    uint32_t dp_idcode;
    uint32_t ap_idr;
    uint8_t connected;
} TC_DAP_HandleTypeDef;

extern TC_DAP_HandleTypeDef g_tc_dap_handle;

void TC_DAP_SetSpeed(TC_DAP_HandleTypeDef* htc_dap, uint32_t speed_hz);

HAL_StatusTypeDef TC_DAP_Init(TC_DAP_HandleTypeDef* htc_dap);
HAL_StatusTypeDef TC_DAP_DeInit(TC_DAP_HandleTypeDef* htc_dap);
HAL_StatusTypeDef TC_DAP_Connect(TC_DAP_HandleTypeDef* htc_dap);
HAL_StatusTypeDef TC_DAP_Disconnect(TC_DAP_HandleTypeDef* htc_dap);

uint32_t TC_DAP_ReadDP(TC_DAP_HandleTypeDef* htc_dap, uint8_t addr);
void TC_DAP_WriteDP(TC_DAP_HandleTypeDef* htc_dap, uint8_t addr, uint32_t data);
uint32_t TC_DAP_ReadAP(TC_DAP_HandleTypeDef* htc_dap, uint8_t ap_num, uint8_t addr);
void TC_DAP_WriteAP(TC_DAP_HandleTypeDef* htc_dap, uint8_t ap_num, uint8_t addr, uint32_t data);

HAL_StatusTypeDef TC_DAP_ReadMem(TC_DAP_HandleTypeDef* htc_dap, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef TC_DAP_WriteMem(TC_DAP_HandleTypeDef* htc_dap, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef TC_DAP_ReadWord(TC_DAP_HandleTypeDef* htc_dap, uint32_t addr, uint32_t* value);
HAL_StatusTypeDef TC_DAP_WriteWord(TC_DAP_HandleTypeDef* htc_dap, uint32_t addr, uint32_t value);

HAL_StatusTypeDef TC_DAP_EraseFlash(TC_DAP_HandleTypeDef* htc_dap, uint32_t sector_addr);
HAL_StatusTypeDef TC_DAP_ProgramFlash(TC_DAP_HandleTypeDef* htc_dap, uint32_t addr, uint8_t* data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif