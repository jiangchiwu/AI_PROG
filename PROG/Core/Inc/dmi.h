/**
 ******************************************************************************
 * @file    dmi.h
 * @brief   RISC-V Debug Module Interface (DMI) 头文件
 ******************************************************************************
 */

#ifndef __DMI_H
#define __DMI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

// DMI 指令定义
#define DMI_CMD_NOP        0x00
#define DMI_CMD_READ       0x01
#define DMI_CMD_WRITE      0x02

// DMI 状态
#define DMI_STAT_OK        0x00
#define DMI_STAT_FAILED    0x02
#define DMI_STAT_BUSY      0x03

// DMI 寄存器地址
#define DMI_REG_DTMCS      0x10
#define DMI_REG_DMI        0x11

// DTMCS 寄存器位定义
#define DTMCS_DMIHARDRESET (1 << 17)
#define DTMCS_DMIRESET     (1 << 16)
#define DTMCS_IDLE         (3 << 12)
#define DTMCS_DMISTAT      (3 << 10)
#define DMI_ABORT          (0x0F << 0)

// DM 寄存器地址（常用）
#define DM_REG_DATA0       0x04
#define DM_REG_DATA1       0x05
#define DM_REG_DMCONTROL   0x10
#define DM_REG_DMSTATUS    0x11
#define DM_REG_HARTINFO    0x12
#define DM_REG_HALTSUM     0x13
#define DM_REG_HAWINDOWSEL 0x14
#define DM_REG_HAWINDOW    0x15
#define DM_REG_SBADDRESS0  0x38
#define DM_REG_SBADDRESS1  0x39
#define DM_REG_SBADDRESS2  0x3A
#define DM_REG_SBDATA0     0x3C
#define DM_REG_SBDATA1     0x3D
#define DM_REG_SBDATA2     0x3E
#define DM_REG_SBCONTROL   0x3F

// DMCONTROL 寄存器位定义
#define DMCONTROL_DMACTIVE    (1 << 0)
#define DMCONTROL_NDMRESET    (1 << 1)
#define DMCONTROL_HARTRESET   (1 << 2)
#define DMCONTROL_HALTREQ     (1 << 31)
#define DMCONTROL_RESUMEREQ   (1 << 30)
#define DMCONTROL_HARTSEL(n)  ((n) << 16)

// DMI 状态结构
typedef struct {
    uint32_t dtmcs;
    uint32_t dmi_stat;
    uint8_t idle_cycles;
    bool initialized;
} DMI_State_t;

// 函数声明
HAL_StatusTypeDef DMI_Init(void);
HAL_StatusTypeDef DMI_DeInit(void);
HAL_StatusTypeDef DMI_Reset(void);
HAL_StatusTypeDef DMI_ActivateDM(void);
HAL_StatusTypeDef DMI_HaltHart(uint32_t hart_id);
HAL_StatusTypeDef DMI_ResumeHart(uint32_t hart_id);
HAL_StatusTypeDef DMI_ReadReg(uint8_t addr, uint32_t *data);
HAL_StatusTypeDef DMI_WriteReg(uint8_t addr, uint32_t data);
HAL_StatusTypeDef DMI_ReadMemory(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef DMI_WriteMemory(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef DMI_WaitIdle(void);

#ifdef __cplusplus
}
#endif

#endif /* __DMI_H */
