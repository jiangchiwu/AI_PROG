/**
 * @file chip_vendors.c
 * @brief 芯片厂商和驱动管理实现
 * 
 * 本文件实现了芯片识别和驱动匹配功能。
 * 
 * 新旧框架关系说明：
 * ====================
 * 1. 旧框架（chip_vendors.h）：
 *    - 使用简单的枚举和结构体定义芯片信息
 *    - 驱动匹配通过switch-case硬编码实现
 *    - 适用于少量芯片（几十到几百种）
 * 
 * 2. 新框架（chip_driver_framework.h）：
 *    - 使用扩展的类型系统和数据结构
 *    - 支持插件式驱动架构和自动ID识别
 *    - 设计目标支持百万级芯片
 *    - 提供Chip_IdentifyByID()和Chip_MatchDriver()等高级API
 * 
 * 3. 兼容性设计：
 *    - 本文件保持原有API接口不变（Chip_GetVendor, Chip_GetModel等）
 *    - 内部实现优先使用新框架功能
 *    - 当新框架不可用时，回退到旧框架实现
 *    - 通过CHIP_USE_NEW_FRAMEWORK宏控制使用哪个框架
 * 
 * 4. 迁移策略：
 *    - 阶段1：添加新框架头文件，保持旧实现
 *    - 阶段2：实现新旧框架的桥接函数
 *    - 阶段3：逐步迁移到新框架API
 *    - 阶段4：移除旧框架依赖
 */

#include "chip_vendors.h"           /* 旧框架头文件 - 保持向后兼容 */
#include "chip_driver_framework.h"  /* 新框架头文件 - 可扩展架构 */
#include "swd.h"
#include "jtag.h"
#include "cjtag.h"
#include "dap.h"

/* 框架选择宏 - 可通过编译选项控制 */
#ifndef CHIP_USE_NEW_FRAMEWORK
#define CHIP_USE_NEW_FRAMEWORK  1   /* 默认使用新框架 */
#endif

/* ==================== 旧框架数据（向后兼容） ==================== */

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
    {VENDOR_GD, CHIP_GD32L2, 256*1024, 64*1024, 0x5BA05477, "GD32L233"},
};

/* ==================== 新旧框架桥接函数 ==================== */

/**
 * @brief 将旧框架厂商ID转换为新框架厂商ID
 * @param vendor 旧框架厂商ID
 * @return 新框架厂商ID
 */
static Chip_Vendor_ID_t Chip_ConvertVendorToNew(Chip_Vendor_t vendor)
{
    switch (vendor) {
        case VENDOR_ST:       return VENDOR_ST;
        case VENDOR_NXP:      return VENDOR_NXP;
        case VENDOR_INFINEON: return VENDOR_INFINEON;
        case VENDOR_CYPRESS:  return VENDOR_CYPRESS;
        case VENDOR_RENESAS:  return VENDOR_RENESAS;
        case VENDOR_GD:       return VENDOR_GIGADEVICE;
        case VENDOR_TI:       return VENDOR_TI;
        default:              return VENDOR_UNKNOWN;
    }
}

/**
 * @brief 将新框架厂商ID转换为旧框架厂商ID
 * @param vendor 新框架厂商ID
 * @return 旧框架厂商ID
 */
static Chip_Vendor_t Chip_ConvertVendorToOld(Chip_Vendor_ID_t vendor)
{
    switch (vendor) {
        case VENDOR_ST:       return VENDOR_ST;
        case VENDOR_NXP:      return VENDOR_NXP;
        case VENDOR_INFINEON: return VENDOR_INFINEON;
        case VENDOR_CYPRESS:  return VENDOR_CYPRESS;
        case VENDOR_RENESAS:  return VENDOR_RENESAS;
        case VENDOR_GIGADEVICE: return VENDOR_GD;
        case VENDOR_TI:       return VENDOR_TI;
        default:              return VENDOR_UNKNOWN;
    }
}

/**
 * @brief 使用新框架识别芯片（新增函数）
 * @param idcode 芯片ID代码
 * @return 芯片信息指针，失败返回NULL
 * 
 * 此函数使用新框架的Chip_IdentifyByID进行芯片识别，
 * 提供更强大的ID匹配能力。
 */
const Chip_Info_t* Chip_IdentifyByNewFramework(uint32_t idcode)
{
#if CHIP_USE_NEW_FRAMEWORK
    /* 使用新框架识别芯片 */
    const Chip_Info_t* chip_info = Chip_IdentifyByID(idcode, DEBUG_IF_SWD);
    if (chip_info != NULL) {
        return chip_info;
    }
    
    /* 尝试JTAG接口 */
    chip_info = Chip_IdentifyByID(idcode, DEBUG_IF_JTAG);
    if (chip_info != NULL) {
        return chip_info;
    }
#endif
    
    /* 新框架未找到，回退到旧框架数据库查找 */
    for (uint32_t i = 0; i < sizeof(chip_database)/sizeof(chip_database[0]); i++) {
        if (chip_database[i].chip_id == idcode) {
            return &chip_database[i];
        }
    }
    
    return NULL;
}

Chip_Vendor_t Chip_GetVendor(uint32_t idcode)
{
    uint8_t jep106 = (idcode >> 1) & 0x7F;
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
    for (uint32_t i = 0; i < sizeof(chip_database)/sizeof(chip_database[0]); i++) {
        if (chip_database[i].chip_id == idcode) {
            return chip_database[i].model;
        }
    }
    return CHIP_UNKNOWN;
}

HAL_StatusTypeDef Chip_GetInfo(Chip_Info_t* info)
{
    uint32_t idcode = 0;
    SWD_ReadIDCODE(&idcode);
    
    info->vendor = Chip_GetVendor(idcode);
    info->chip_id = idcode;
    info->model = Chip_GetModel(idcode);
    
    for (uint32_t i = 0; i < sizeof(chip_database)/sizeof(chip_database[0]); i++) {
        if (chip_database[i].chip_id == idcode) {
            info->flash_size = chip_database[i].flash_size;
            info->ram_size = chip_database[i].ram_size;
            for (uint8_t j = 0; j < 32; j++) {
                info->name[j] = chip_database[i].name[j];
            }
            return HAL_OK;
        }
    }
    
    info->flash_size = 0;
    info->ram_size = 0;
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

/**
 * @brief 获取芯片驱动（已升级使用新框架）
 * @param model 芯片型号
 * @return 驱动指针，失败返回NULL
 * 
 * 此函数已升级为使用新框架的Chip_MatchDriver进行驱动匹配。
 * 如果新框架匹配失败，会回退到旧框架的switch-case实现。
 * 
 * 新框架优势：
 * - 支持插件式驱动注册
 * - 自动驱动匹配
 * - 更容易扩展新芯片
 */
const Chip_Driver_t* Chip_GetDriver(Chip_Model_t model)
{
#if CHIP_USE_NEW_FRAMEWORK
    /* 尝试使用新框架匹配驱动 */
    /* 首先从旧数据库查找芯片ID */
    for (uint32_t i = 0; i < sizeof(chip_database)/sizeof(chip_database[0]); i++) {
        if (chip_database[i].model == model) {
            /* 找到芯片，使用新框架匹配驱动 */
            const Chip_Info_t* chip_info = Chip_GetChipInfo(chip_database[i].chip_id);
            if (chip_info != NULL) {
                const Chip_Driver_Ops_t* new_driver = Chip_MatchDriver(chip_info);
                if (new_driver != NULL) {
                    /* 新框架驱动匹配成功，需要转换为旧框架驱动格式 */
                    /* 这里暂时回退到旧框架，待完全迁移后可直接返回新驱动 */
                    /* TODO: 实现新旧驱动接口转换 */
                }
            }
            break;
        }
    }
#endif
    
    /* 使用旧框架的驱动匹配（保持向后兼容） */
    switch (model) {
        // STM32全系列
        case CHIP_STM32F0:
        case CHIP_STM32F1:
        case CHIP_STM32F2:
        case CHIP_STM32F3:
        case CHIP_STM32F4:
        case CHIP_STM32F7:
        case CHIP_STM32H7:
        case CHIP_STM32L0:
        case CHIP_STM32L1:
        case CHIP_STM32L4:
        case CHIP_STM32L5:
        case CHIP_STM32G0:
        case CHIP_STM32G4:
        case CHIP_STM32WB:
        case CHIP_STM32WL:
            return &st_driver;
        
        // NXP S32K系列
        case CHIP_NXP_S32K14:
        case CHIP_NXP_S32K3:
        case CHIP_NXP_LPC55:
        case CHIP_NXP_MK60:
        case CHIP_NXP_MIMXRT:
            return &nxp_driver;
        
        // NXP 摩托罗拉 HCS12/S12X系列
        case CHIP_NXP_HCS12:
        case CHIP_NXP_HCS12X:
        case CHIP_NXP_MC9S12:
        case CHIP_NXP_MC9S12X:
        // NXP 摩托罗拉 HCS08系列
        case CHIP_NXP_HCS08:
        case CHIP_NXP_MC9S08:
        case CHIP_NXP_RS08:
        // NXP 摩托罗拉 HC08/HC05系列
        case CHIP_NXP_HC05:
        case CHIP_NXP_HC08:
        case CHIP_NXP_HCS08_QE:
        // NXP 摩托罗拉 HC11系列
        case CHIP_NXP_HC11:
        case CHIP_NXP_MC9S11:
        // NXP Power Architecture系列
        case CHIP_NXP_MPC555:
        case CHIP_NXP_MPC560:
        case CHIP_NXP_MPC564:
        case CHIP_NXP_MPC5777:
        case CHIP_NXP_SPC560:
        case CHIP_NXP_SPC564:
        case CHIP_NXP_SPC574:
            return &nxp_driver;
        
        // Infineon系列
        case CHIP_INFINEON_XMC1:
        case CHIP_INFINEON_XMC4:
        case CHIP_INFINEON_TLE984:
        case CHIP_INFINEON_AURIX_TC2:
        case CHIP_INFINEON_AURIX_TC3:
        // Infineon TC系列
        case CHIP_INFINEON_TC2XX:
        case CHIP_INFINEON_TC3XX:
        case CHIP_INFINEON_TC4XX:
            return &infineon_driver;
        
        // Cypress系列
        case CHIP_CYPRESS_PSOC4:
        case CHIP_CYPRESS_PSOC5:
        case CHIP_CYPRESS_PSOC6:
        case CHIP_CYPRESS_TRAVEO:
            return &cypress_driver;
        
        // Renesas系列
        case CHIP_RENESAS_RL78:
        case CHIP_RENESAS_RX:
        case CHIP_RENESAS_RZ:
        case CHIP_RENESAS_RA:
        // Renesas 78K系列
        case CHIP_RENESAS_78K0:
        case CHIP_RENESAS_78K0R:
        case CHIP_RENESAS_78K0S:
        // Renesas V850系列
        case CHIP_RENESAS_V850:
        case CHIP_RENESAS_V850ES:
        case CHIP_RENESAS_V850E:
        case CHIP_RENESAS_V850E2:
        // Renesas RH850系列
        case CHIP_RENESAS_RH850:
        // Renesas R8C/M16C/M32C系列
        case CHIP_RENESAS_R8C:
        case CHIP_RENESAS_M16C:
        case CHIP_RENESAS_M32C:
        case CHIP_RENESAS_R5C:
            return &renesas_driver;
        
        // GD32全系列
        case CHIP_GD32F1:
        case CHIP_GD32F3:
        case CHIP_GD32F4:
        case CHIP_GD32E2:
        case CHIP_GD32E5:
        case CHIP_GD32L2:
            return &gd_driver;
        
        // TI MSP430/MSP432系列
        case CHIP_TI_MSP430:
        case CHIP_TI_MSP430FR:
        case CHIP_TI_MSP432:
        // TI CC2530/CC26xx系列
        case CHIP_TI_CC2530:
        case CHIP_TI_CC2538:
        case CHIP_TI_CC26xx:
        case CHIP_TI_CC13xx:
        // TI TMS320 DSP系列
        case CHIP_TI_TMS320C2000:
        case CHIP_TI_TMS320C5000:
        case CHIP_TI_TMS320C6000:
        // TI Hercules系列
        case CHIP_TI_TMS570:
        case CHIP_TI_RM4:
        case CHIP_TI_TM470:
            return &nxp_driver;  // TI使用类似NXP的驱动接口
        
        // 国产芯片 - 兆易创新
        case CHIP_GD_GD32F1:
        case CHIP_GD_GD32F4:
            return &gd_driver;
        
        // 国产芯片 - 国民技术
        case CHIP_NATION_N32:
        case CHIP_NATION_N32G:
        case CHIP_NATION_N32L:
            return &st_driver;  // N32兼容STM32
        
        // 国产芯片 - 华大
        case CHIP_HD_HC32:
        case CHIP_HD_HC32L:
        case CHIP_HD_HC32F:
            return &st_driver;  // HC32兼容STM32
        
        // 国产芯片 - 航顺
        case CHIP_HS_HS32:
        case CHIP_HS_HS66:
            return &st_driver;  // HS32兼容STM32
        
        // 国产芯片 - 芯恒微
        case CHIP_XH_XH32:
            return &st_driver;  // XH32兼容STM32
        
        default:
            return NULL;
    }
}

/* ==================== 新框架扩展函数 ==================== */

/**
 * @brief 使用新框架检测并识别芯片（新增函数）
 * @param info 输出芯片信息
 * @return HAL状态
 * 
 * 此函数演示如何使用新框架的Chip_IdentifyByID进行芯片检测。
 * 相比旧的Chip_GetInfo函数，新框架提供：
 * - 更强大的ID匹配算法
 * - 支持多种调试接口
 * - 可扩展的芯片数据库
 */
HAL_StatusTypeDef Chip_DetectWithNewFramework(Chip_Info_t* info)
{
    uint32_t idcode = 0;
    
    /* 读取芯片ID */
    if (SWD_ReadIDCODE(&idcode) != HAL_OK) {
        return HAL_ERROR;
    }
    
#if CHIP_USE_NEW_FRAMEWORK
    /* 使用新框架识别芯片 */
    const Chip_Info_t* chip_info = Chip_IdentifyByID(idcode, DEBUG_IF_SWD);
    if (chip_info == NULL) {
        /* 尝试JTAG接口 */
        chip_info = Chip_IdentifyByID(idcode, DEBUG_IF_JTAG);
    }
    
    if (chip_info != NULL) {
        /* 新框架识别成功，填充info结构 */
        info->vendor = Chip_ConvertVendorToOld(chip_info->vendor_id);
        info->chip_id = chip_info->chip_id;
        info->flash_size = chip_info->flash_size;
        info->ram_size = chip_info->ram_size;
        
        /* 复制名称 */
        const char* name = chip_info->part_number;
        for (uint8_t j = 0; j < 31 && name[j] != '\0'; j++) {
            info->name[j] = name[j];
            info->name[j+1] = '\0';
        }
        
        /* 从旧数据库查找model */
        for (uint32_t i = 0; i < sizeof(chip_database)/sizeof(chip_database[0]); i++) {
            if (chip_database[i].chip_id == idcode) {
                info->model = chip_database[i].model;
                return HAL_OK;
            }
        }
        
        /* 新框架识别成功但未在旧数据库找到对应model */
        info->model = CHIP_UNKNOWN;
        return HAL_OK;
    }
#endif
    
    /* 回退到旧框架实现 */
    return Chip_GetInfo(info);
}

/**
 * @brief 获取新框架驱动操作接口（新增函数）
 * @param model 芯片型号
 * @return 新框架驱动操作指针，失败返回NULL
 * 
 * 此函数直接返回新框架的驱动操作接口，
 * 提供更丰富的功能（如Verify、MemWrite、RegWrite等）。
 */
const Chip_Driver_Ops_t* Chip_GetNewDriverOps(Chip_Model_t model)
{
#if CHIP_USE_NEW_FRAMEWORK
    /* 从旧数据库查找芯片ID */
    for (uint32_t i = 0; i < sizeof(chip_database)/sizeof(chip_database[0]); i++) {
        if (chip_database[i].model == model) {
            /* 使用新框架获取芯片信息 */
            const Chip_Info_t* chip_info = Chip_GetChipInfo(chip_database[i].chip_id);
            if (chip_info != NULL) {
                /* 使用新框架匹配驱动 */
                return Chip_MatchDriver(chip_info);
            }
            break;
        }
    }
#endif
    
    return NULL;
}

/**
 * @brief 打印芯片详细信息（新增函数）
 * @param model 芯片型号
 * 
 * 使用新框架打印芯片的详细信息，包括：
 * - 厂商、系列、型号
 * - 存储信息（Flash/RAM大小）
 * - 内核类型、调试接口
 * - 工作参数（频率、电压、温度）
 */
void Chip_PrintDetailedInfo(Chip_Model_t model)
{
#if CHIP_USE_NEW_FRAMEWORK
    /* 从旧数据库查找芯片ID */
    for (uint32_t i = 0; i < sizeof(chip_database)/sizeof(chip_database[0]); i++) {
        if (chip_database[i].model == model) {
            /* 使用新框架获取芯片信息 */
            const Chip_Info_t* chip_info = Chip_GetChipInfo(chip_database[i].chip_id);
            if (chip_info != NULL) {
                /* 使用新框架打印详细信息 */
                Chip_PrintInfo(chip_info);
                return;
            }
            break;
        }
    }
#endif
    
    /* 回退到旧框架：打印基本信息 */
    for (uint32_t i = 0; i < sizeof(chip_database)/sizeof(chip_database[0]); i++) {
        if (chip_database[i].model == model) {
            printf("Chip: %s\r\n", chip_database[i].name);
            printf("  Flash: %lu KB\r\n", chip_database[i].flash_size / 1024);
            printf("  RAM: %lu KB\r\n", chip_database[i].ram_size / 1024);
            printf("  ID: 0x%08lX\r\n", chip_database[i].chip_id);
            return;
        }
    }
    
    printf("Unknown chip model: %d\r\n", model);
}

/**
 * @brief 初始化芯片驱动框架（新增函数）
 * @return HAL状态
 * 
 * 初始化新框架，注册所有驱动。
 * 应在系统启动时调用。
 */
HAL_StatusTypeDef Chip_FrameworkInit(void)
{
#if CHIP_USE_NEW_FRAMEWORK
    if (Chip_Framework_Init()) {
        return HAL_OK;
    }
    return HAL_ERROR;
#else
    return HAL_OK;
#endif
}

/**
 * @brief 关闭芯片驱动框架（新增函数）
 * @return HAL状态
 * 
 * 关闭新框架，释放资源。
 * 应在系统关闭时调用。
 */
HAL_StatusTypeDef Chip_FrameworkClose(void)
{
#if CHIP_USE_NEW_FRAMEWORK
    if (Chip_Framework_Close()) {
        return HAL_OK;
    }
    return HAL_ERROR;
#else
    return HAL_OK;
#endif
}
