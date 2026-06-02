
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
    CHIP_STM32F1,
    CHIP_STM32F4,
    CHIP_STM32F7,
    CHIP_STM32H7,
    CHIP_STM32L4,
    CHIP_STM32G0,
    CHIP_STM32G4,
    CHIP_STM32L5,
    CHIP_STM32WB,
    CHIP_NXP_S32K14,
    CHIP_NXP_S32K3,
    CHIP_NXP_LPC55,
    CHIP_NXP_MK60,
    CHIP_NXP_MIMXRT,
    CHIP_INFINEON_XMC1,
    CHIP_INFINEON_XMC4,
    CHIP_INFINEON_TLE984,
    CHIP_INFINEON_AURIX_TC2,
    CHIP_INFINEON_AURIX_TC3,
    CHIP_CYPRESS_PSOC4,
    CHIP_CYPRESS_PSOC5,
    CHIP_CYPRESS_PSOC6,
    CHIP_CYPRESS_TRAVEO,
    CHIP_RENESAS_RL78,
    CHIP_RENESAS_RX,
    CHIP_RENESAS_RZ,
    CHIP_RENESAS_RA,
    CHIP_GD32F1,
    CHIP_GD32F4,
    CHIP_GD32F3,
    CHIP_GD32E5,
    CHIP_GD32L4,
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
