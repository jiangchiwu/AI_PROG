
#ifndef __CHIP_VENDORS_H
#define __CHIP_VENDORS_H

#include &lt;stdint.h&gt;

typedef enum {
    VENDOR_ST = 0x20,
    VENDOR_NXP = 0x15,
    VENDOR_INFINEON = 0x05,
    VENDOR_CYPRESS = 0x04,
    VENDOR_RENESAS = 0x23,
    VENDOR_GD = 0x48,
    VENDOR_MICROCHIP = 0x03,
    VENDOR_TI = 0x17,
    VENDOR_MAXIM = 0x1C,
    VENDOR_UNKNOWN = 0xFF
} Chip_Vendor_t;

typedef enum {
    // STM32系列
    CHIP_STM32F0,
    CHIP_STM32F1,
    CHIP_STM32F2,
    CHIP_STM32F3,
    CHIP_STM32F4,
    CHIP_STM32F7,
    CHIP_STM32H7,
    CHIP_STM32L0,
    CHIP_STM32L1,
    CHIP_STM32L4,
    CHIP_STM32L5,
    CHIP_STM32G0,
    CHIP_STM32G4,
    CHIP_STM32WB,
    CHIP_STM32WL,
    // GD32系列
    CHIP_GD32F1,
    CHIP_GD32F3,
    CHIP_GD32F4,
    CHIP_GD32E2,
    CHIP_GD32E5,
    CHIP_GD32L2,
    // NXP S32K系列
    CHIP_NXP_S32K14,
    CHIP_NXP_S32K3,
    CHIP_NXP_LPC55,
    CHIP_NXP_MK60,
    CHIP_NXP_MIMXRT,
    // NXP 摩托罗拉系列 - HCS12/S12X
    CHIP_NXP_HCS12,
    CHIP_NXP_HCS12X,
    CHIP_NXP_MC9S12,
    CHIP_NXP_MC9S12X,
    // NXP 摩托罗拉系列 - HCS08
    CHIP_NXP_HCS08,
    CHIP_NXP_MC9S08,
    CHIP_NXP_RS08,
    // NXP 摩托罗拉系列 - HC08
    CHIP_NXP_HC05,
    CHIP_NXP_HC08,
    CHIP_NXP_HCS08_QE,
    // NXP 摩托罗拉系列 - HC11
    CHIP_NXP_HC11,
    CHIP_NXP_MC9S11,
    // NXP Power Architecture系列
    CHIP_NXP_MPC555,
    CHIP_NXP_MPC560,
    CHIP_NXP_MPC564,
    CHIP_NXP_MPC5777,
    CHIP_NXP_SPC560,
    CHIP_NXP_SPC564,
    CHIP_NXP_SPC574,
    // Infineon系列
    CHIP_INFINEON_XMC1,
    CHIP_INFINEON_XMC4,
    CHIP_INFINEON_TLE984,
    CHIP_INFINEON_AURIX_TC2,
    CHIP_INFINEON_AURIX_TC3,
    // Cypress系列
    CHIP_CYPRESS_PSOC4,
    CHIP_CYPRESS_PSOC5,
    CHIP_CYPRESS_PSOC6,
    CHIP_CYPRESS_TRAVEO,
    // Renesas系列
    CHIP_RENESAS_RL78,
    CHIP_RENESAS_RX,
    CHIP_RENESAS_RZ,
    CHIP_RENESAS_RA,
    // Renesas 78K系列
    CHIP_RENESAS_78K0,
    CHIP_RENESAS_78K0R,
    CHIP_RENESAS_78K0S,
    // Renesas V850系列
    CHIP_RENESAS_V850,
    CHIP_RENESAS_V850ES,
    CHIP_RENESAS_V850E,
    CHIP_RENESAS_V850E2,
    // Renesas RH850系列
    CHIP_RENESAS_RH850,
    // Renesas R8C/M16C/M32C系列
    CHIP_RENESAS_R8C,
    CHIP_RENESAS_M16C,
    CHIP_RENESAS_M32C,
    CHIP_RENESAS_R5C,
    // TI MSP430系列
    CHIP_TI_MSP430,
    CHIP_TI_MSP430FR,
    // TI MSP432系列
    CHIP_TI_MSP432,
    // TI CC2530/CC26xx系列
    CHIP_TI_CC2530,
    CHIP_TI_CC2538,
    CHIP_TI_CC26xx,
    CHIP_TI_CC13xx,
    // TI TMS320 DSP系列
    CHIP_TI_TMS320C2000,
    CHIP_TI_TMS320C5000,
    CHIP_TI_TMS320C6000,
    // TI Hercules系列
    CHIP_TI_TMS570,
    CHIP_TI_RM4,
    CHIP_TI_TM470,
    // 国产芯片 - 兆易创新
    CHIP_GD_GD32F1,
    CHIP_GD_GD32F4,
    // 国产芯片 - 国民技术
    CHIP_NATION_N32,
    CHIP_NATION_N32G,
    CHIP_NATION_N32L,
    // 国产芯片 - 华大
    CHIP_HD_HC32,
    CHIP_HD_HC32L,
    CHIP_HD_HC32F,
    // 国产芯片 - 航顺
    CHIP_HS_HS32,
    CHIP_HS_HS66,
    // 国产芯片 - 芯恒微
    CHIP_XH_XH32,
    // 英飞凌TC系列
    CHIP_INFINEON_TC2XX,
    CHIP_INFINEON_TC3XX,
    CHIP_INFINEON_TC4XX,
    CHIP_UNKNOWN
} Chip_Model_t;

typedef struct {
    Chip_Vendor_t vendor;
    Chip_Model_t model;
    uint32_t flash_size;
    uint32_t ram_size;
    uint32_t chip_id;
    uint8_t name[32];
} Chip_Info_t;

typedef struct {
    HAL_StatusTypeDef (*Init)(void);
    HAL_StatusTypeDef (*Erase)(uint32_t start_addr, uint32_t size);
    HAL_StatusTypeDef (*Write)(uint32_t addr, uint8_t* data, uint32_t size);
    HAL_StatusTypeDef (*Read)(uint32_t addr, uint8_t* data, uint32_t size);
    HAL_StatusTypeDef (*ReadID)(uint32_t* id);
    HAL_StatusTypeDef (*Reset)(void);
} Chip_Driver_t;

Chip_Vendor_t Chip_GetVendor(uint32_t idcode);
Chip_Model_t Chip_GetModel(uint32_t idcode);
HAL_StatusTypeDef Chip_GetInfo(Chip_Info_t* info);
const Chip_Driver_t* Chip_GetDriver(Chip_Model_t model);

#endif
