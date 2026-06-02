
#include "chip_vendors.h"
#include "swd.h"
#include "jtag.h"
#include "cjtag.h"
#include "dap.h"

static Chip_Info_t chip_database[] = {
    {VENDOR_ST, CHIP_STM32F1, 64*1024, 20*1024, 0x1BA01477, "STM32F103C8"},
    {VENDOR_ST, CHIP_STM32F4, 1024*1024, 192*1024, 0x410FC241, "STM32F407ZGT6"},
    {VENDOR_ST, CHIP_STM32F7, 1024*1024, 512*1024, 0x44900483, "STM32F767IGT6"},
    {VENDOR_ST, CHIP_STM32H7, 2048*1024, 1024*1024, 0x45000483, "STM32H743VIT6"},
    {VENDOR_ST, CHIP_STM32L4, 512*1024, 128*1024, 0x46100483, "STM32L431RCT6"},
    {VENDOR_ST, CHIP_STM32G0, 64*1024, 8*1024, 0x46002641, "STM32G071RB"},
    {VENDOR_NXP, CHIP_NXP_S32K14, 512*1024, 64*1024, 0x1BA01477, "S32K144"},
    {VENDOR_NXP, CHIP_NXP_S32K3, 4096*1024, 512*1024, 0x1BA02477, "S32K344"},
    {VENDOR_NXP, CHIP_NXP_LPC55, 256*1024, 144*1024, 0x1BA03477, "LPC55S69"},
    {VENDOR_NXP, CHIP_NXP_MIMXRT, 0, 1024*1024, 0x1BA04477, "i.MXRT1052"},
    {VENDOR_INFINEON, CHIP_INFINEON_XMC1, 64*1024, 16*1024, 0x2BA01477, "XMC1400"},
    {VENDOR_INFINEON, CHIP_INFINEON_XMC4, 512*1024, 80*1024, 0x2BA02477, "XMC4700"},
    {VENDOR_INFINEON, CHIP_INFINEON_TLE984, 128*1024, 32*1024, 0x2BA03477, "TLE9844"},
    {VENDOR_INFINEON, CHIP_INFINEON_AURIX_TC2, 2048*1024, 512*1024, 0x2BA04477, "TC275"},
    {VENDOR_INFINEON, CHIP_INFINEON_AURIX_TC3, 8192*1024, 2048*1024, 0x2BA05477, "TC397"},
    {VENDOR_CYPRESS, CHIP_CYPRESS_PSOC4, 128*1024, 16*1024, 0x3BA01477, "PSoC4"},
    {VENDOR_CYPRESS, CHIP_CYPRESS_PSOC6, 1024*1024, 288*1024, 0x3BA02477, "PSoC6"},
    {VENDOR_CYPRESS, CHIP_CYPRESS_TRAVEO, 4096*1024, 1024*1024, 0x3BA03477, "Traveo II"},
    {VENDOR_RENESAS, CHIP_RENESAS_RL78, 64*1024, 8*1024, 0x4BA01477, "RL78/G13"},
    {VENDOR_RENESAS, CHIP_RENESAS_RX, 512*1024, 64*1024, 0x4BA02477, "RX65N"},
    {VENDOR_RENESAS, CHIP_RENESAS_RA, 256*1024, 32*1024, 0x4BA03477, "RA4M1"},
    {VENDOR_GD, CHIP_GD32F1, 64*1024, 20*1024, 0x5BA01477, "GD32F103"},
    {VENDOR_GD, CHIP_GD32F4, 1024*1024, 192*1024, 0x5BA02477, "GD32F407"},
    {VENDOR_GD, CHIP_GD32F3, 512*1024, 96*1024, 0x5BA03477, "GD32F303"},
    {VENDOR_GD, CHIP_GD32E5, 512*1024, 128*1024, 0x5BA04477, "GD32E503"},
    {VENDOR_GD, CHIP_GD32L4, 256*1024, 64*1024, 0x5BA05477, "GD32L433"},
};

Chip_Vendor_t Chip_GetVendor(uint32_t idcode)
{
    uint8_t jep106 = (idcode &gt;&gt; 1) &amp; 0x7F;
    switch (jep106) {
        case 0x20: return VENDOR_ST;
        case 0x15: return VENDOR_NXP;
        case 0x05: return VENDOR_INFINEON;
        case 0x04: return VENDOR_CYPRESS;
        case 0x23: return VENDOR_RENESAS;
        case 0x48: return VENDOR_GD;
        default: return VENDOR_UNKNOWN;
    }
}

Chip_Model_t Chip_GetModel(uint32_t idcode)
{
    for (uint32_t i = 0; i &lt; sizeof(chip_database)/sizeof(chip_database[0]); i++) {
        if (chip_database[i].chip_id == idcode) {
            return chip_database[i].model;
        }
    }
    return CHIP_UNKNOWN;
}

HAL_StatusTypeDef Chip_GetInfo(Chip_Info_t* info)
{
    uint32_t idcode = 0;
    SWD_ReadIDCODE(&amp;idcode);
    
    info-&gt;vendor = Chip_GetVendor(idcode);
    info-&gt;chip_id = idcode;
    info-&gt;model = Chip_GetModel(idcode);
    
    for (uint32_t i = 0; i &lt; sizeof(chip_database)/sizeof(chip_database[0]); i++) {
        if (chip_database[i].chip_id == idcode) {
            info-&gt;flash_size = chip_database[i].flash_size;
            info-&gt;ram_size = chip_database[i].ram_size;
            for (uint8_t j = 0; j &lt; 32; j++) {
                info-&gt;name[j] = chip_database[i].name[j];
            }
            return HAL_OK;
        }
    }
    
    info-&gt;flash_size = 0;
    info-&gt;ram_size = 0;
    return HAL_ERROR;
}

static HAL_StatusTypeDef STM32_Init(void)
{
    return HAL_OK;
}

static HAL_StatusTypeDef STM32_Erase(uint32_t start_addr, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef STM32_Write(uint32_t addr, uint8_t* data, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef STM32_Read(uint32_t addr, uint8_t* data, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef STM32_ReadID(uint32_t* id)
{
    return SWD_ReadIDCODE(id);
}

static HAL_StatusTypeDef STM32_Reset(void)
{
    return SWD_Reset();
}

static HAL_StatusTypeDef NXP_Init(void)
{
    return HAL_OK;
}

static HAL_StatusTypeDef NXP_Erase(uint32_t start_addr, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef NXP_Write(uint32_t addr, uint8_t* data, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef NXP_Read(uint32_t addr, uint8_t* data, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef NXP_ReadID(uint32_t* id)
{
    return SWD_ReadIDCODE(id);
}

static HAL_StatusTypeDef NXP_Reset(void)
{
    return SWD_Reset();
}

static HAL_StatusTypeDef Infineon_Init(void)
{
    return HAL_OK;
}

static HAL_StatusTypeDef Infineon_Erase(uint32_t start_addr, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef Infineon_Write(uint32_t addr, uint8_t* data, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef Infineon_Read(uint32_t addr, uint8_t* data, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef Infineon_ReadID(uint32_t* id)
{
    return SWD_ReadIDCODE(id);
}

static HAL_StatusTypeDef Infineon_Reset(void)
{
    return SWD_Reset();
}

static HAL_StatusTypeDef Cypress_Init(void)
{
    return HAL_OK;
}

static HAL_StatusTypeDef Cypress_Erase(uint32_t start_addr, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef Cypress_Write(uint32_t addr, uint8_t* data, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef Cypress_Read(uint32_t addr, uint8_t* data, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef Cypress_ReadID(uint32_t* id)
{
    return SWD_ReadIDCODE(id);
}

static HAL_StatusTypeDef Cypress_Reset(void)
{
    return SWD_Reset();
}

static HAL_StatusTypeDef Renesas_Init(void)
{
    return HAL_OK;
}

static HAL_StatusTypeDef Renesas_Erase(uint32_t start_addr, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef Renesas_Write(uint32_t addr, uint8_t* data, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef Renesas_Read(uint32_t addr, uint8_t* data, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef Renesas_ReadID(uint32_t* id)
{
    return SWD_ReadIDCODE(id);
}

static HAL_StatusTypeDef Renesas_Reset(void)
{
    return SWD_Reset();
}

static HAL_StatusTypeDef GD_Init(void)
{
    return HAL_OK;
}

static HAL_StatusTypeDef GD_Erase(uint32_t start_addr, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef GD_Write(uint32_t addr, uint8_t* data, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef GD_Read(uint32_t addr, uint8_t* data, uint32_t size)
{
    return HAL_OK;
}

static HAL_StatusTypeDef GD_ReadID(uint32_t* id)
{
    return SWD_ReadIDCODE(id);
}

static HAL_StatusTypeDef GD_Reset(void)
{
    return SWD_Reset();
}

static const Chip_Driver_t st_driver = {
    .Init = STM32_Init,
    .Erase = STM32_Erase,
    .Write = STM32_Write,
    .Read = STM32_Read,
    .ReadID = STM32_ReadID,
    .Reset = STM32_Reset
};

static const Chip_Driver_t nxp_driver = {
    .Init = NXP_Init,
    .Erase = NXP_Erase,
    .Write = NXP_Write,
    .Read = NXP_Read,
    .ReadID = NXP_ReadID,
    .Reset = NXP_Reset
};

static const Chip_Driver_t infineon_driver = {
    .Init = Infineon_Init,
    .Erase = Infineon_Erase,
    .Write = Infineon_Write,
    .Read = Infineon_Read,
    .ReadID = Infineon_ReadID,
    .Reset = Infineon_Reset
};

static const Chip_Driver_t cypress_driver = {
    .Init = Cypress_Init,
    .Erase = Cypress_Erase,
    .Write = Cypress_Write,
    .Read = Cypress_Read,
    .ReadID = Cypress_ReadID,
    .Reset = Cypress_Reset
};

static const Chip_Driver_t renesas_driver = {
    .Init = Renesas_Init,
    .Erase = Renesas_Erase,
    .Write = Renesas_Write,
    .Read = Renesas_Read,
    .ReadID = Renesas_ReadID,
    .Reset = Renesas_Reset
};

static const Chip_Driver_t gd_driver = {
    .Init = GD_Init,
    .Erase = GD_Erase,
    .Write = GD_Write,
    .Read = GD_Read,
    .ReadID = GD_ReadID,
    .Reset = GD_Reset
};

const Chip_Driver_t* Chip_GetDriver(Chip_Model_t model)
{
    switch (model) {
        case CHIP_STM32F1:
        case CHIP_STM32F4:
        case CHIP_STM32F7:
        case CHIP_STM32H7:
        case CHIP_STM32L4:
        case CHIP_STM32G0:
        case CHIP_STM32G4:
        case CHIP_STM32L5:
        case CHIP_STM32WB:
            return &amp;st_driver;
        
        case CHIP_NXP_S32K14:
        case CHIP_NXP_S32K3:
        case CHIP_NXP_LPC55:
        case CHIP_NXP_MK60:
        case CHIP_NXP_MIMXRT:
            return &amp;nxp_driver;
        
        case CHIP_INFINEON_XMC1:
        case CHIP_INFINEON_XMC4:
        case CHIP_INFINEON_TLE984:
        case CHIP_INFINEON_AURIX_TC2:
        case CHIP_INFINEON_AURIX_TC3:
            return &amp;infineon_driver;
        
        case CHIP_CYPRESS_PSOC4:
        case CHIP_CYPRESS_PSOC5:
        case CHIP_CYPRESS_PSOC6:
        case CHIP_CYPRESS_TRAVEO:
            return &amp;cypress_driver;
        
        case CHIP_RENESAS_RL78:
        case CHIP_RENESAS_RX:
        case CHIP_RENESAS_RZ:
        case CHIP_RENESAS_RA:
            return &amp;renesas_driver;
        
        case CHIP_GD32F1:
        case CHIP_GD32F4:
        case CHIP_GD32F3:
        case CHIP_GD32E5:
        case CHIP_GD32L4:
            return &amp;gd_driver;
        
        default:
            return NULL;
    }
}
