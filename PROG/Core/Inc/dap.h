/**
 ******************************************************************************
 * @file    dap.h
 * @brief   ARM DAP (Debug Access Port) 层实现
 ******************************************************************************
 */

#ifndef __DAP_H__
#define __DAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "swd.h"
#include "jtag.h"

#define DAP_OK              0x00
#define DAP_ERR            0x01
#define DAP_ERR_NOT_CONNECTED 0x02
#define DAP_ERR_TIMEOUT    0x03
#define DAP_ERR_WDATA_ERR  0x04
#define DAP_ERR_STICKY_ERR 0x05
#define DAP_ERR_FAULT      0x06

#define DAP_PROTOCOL_SWD   0x00
#define DAP_PROTOCOL_JTAG  0x01

#define DAP_DP_IDCODE      0x00
#define DAP_DP_CTRL_STAT   0x04
#define DAP_DP_RESEND      0x08
#define DAP_DP_SELECT      0x0C
#define DAP_DP_RDBUFF      0x10

#define DAP_AP_CSW         0x00
#define DAP_AP_TAR         0x04
#define DAP_AP_DRW         0x08
#define DAP_AP_BD0         0x0C
#define DAP_AP_BD1         0x10
#define DAP_AP_BD2         0x14
#define DAP_AP_BD3         0x18
#define DAP_AP_IDR         0xFC

#define DAP_CTRL_STAT_STICKYORUN    (1 << 30)
#define DAP_CTRL_STAT_STICKYCMP     (1 << 29)
#define DAP_CTRL_STAT_STICKYERR     (1 << 28)
#define DAP_CTRL_STAT_WAITUP        (1 << 27)
#define DAP_CTRL_STAT_WDATAERR      (1 << 25)
#define DAP_CTRL_STAT_READOK        (1 << 24)
#define DAP_CTRL_STAT_OVERUN        (1 << 23)
#define DAP_CTRL_STAT_TRNNMOD       (1 << 11)
#define DAP_CTRL_STAT_ORUNDETECT    (1 << 7)
#define DAP_CTRL_STAT_STRESET       (1 << 6)
#define DAP_CTRL_STAT_HALTCLK       (1 << 5)
#define DAP_CTRL_STAT_CDBGPWRUPREQ  (1 << 2)
#define DAP_CTRL_STAT_CDBGPWRUPACK  (1 << 1)
#define DAP_CTRL_STAT_KEY           (1 << 0)

#define DAP_AP_CSW_SIZE8     (0 << 0)
#define DAP_AP_CSW_SIZE16    (1 << 0)
#define DAP_AP_CSW_SIZE32    (2 << 0)
#define DAP_AP_CSW_ADDRINC   (1 << 4)
#define DAP_AP_CSW_DEFAULT   (0x23000012)

typedef struct {
    uint32_t dp_idcode;
    uint32_t ap_idr;
    uint8_t ap_count;
    uint8_t selected_ap;
    uint8_t protocol;
    uint8_t connected;
} DAP_Info_TypeDef;

typedef struct {
    uint32_t (*read_dp)(uint8_t addr);
    void (*write_dp)(uint8_t addr, uint32_t data);
    uint32_t (*read_ap)(uint8_t ap_num, uint8_t addr);
    void (*write_ap)(uint8_t ap_num, uint8_t addr, uint32_t data);
} DAP_Ops_TypeDef;

extern DAP_Info_TypeDef g_dap_info;
extern DAP_Ops_TypeDef g_dap_ops;

HAL_StatusTypeDef DAP_Init(uint8_t protocol);
HAL_StatusTypeDef DAP_DeInit(void);
HAL_StatusTypeDef DAP_Connect(void);
HAL_StatusTypeDef DAP_Disconnect(void);

uint32_t DAP_ReadDP(uint8_t addr);
void DAP_WriteDP(uint8_t addr, uint32_t data);

uint32_t DAP_ReadAP(uint8_t ap_num, uint8_t addr);
void DAP_WriteAP(uint8_t ap_num, uint8_t addr, uint32_t data);

HAL_StatusTypeDef DAP_ReadMem(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef DAP_WriteMem(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef DAP_ReadWord(uint32_t addr, uint32_t *value);
HAL_StatusTypeDef DAP_WriteWord(uint32_t addr, uint32_t value);

HAL_StatusTypeDef DAP_ClearErrors(void);
HAL_StatusTypeDef DAP_PowerUpDebug(void);
HAL_StatusTypeDef DAP_SelectAP(uint8_t ap_num);

#ifdef __cplusplus
}
#endif

#endif
