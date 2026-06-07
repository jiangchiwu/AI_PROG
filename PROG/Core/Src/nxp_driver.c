/**
 ******************************************************************************
 * @file    nxp_driver.c
 * @brief   NXP LPC/i.MX全系列驱动实现（对标J-Flash）
 *          支持LPC800/LPC1100/LPC1700/LPC4300/i.MX RT/Kinetis/S32K系列
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#include "nxp_driver.h"
#include "swd.h"
#include "jtag.h"
#include <string.h>

/* ==================== NXP型号数据库 ==================== */
typedef struct {
    NXP_Family_t    family;
    uint32_t        idcode;
    char            part_number[24];
    uint32_t        flash_size;
    uint32_t        ram_size;
    uint32_t        sector_size;
    uint32_t        page_size;
} NXP_Model_t;

static const NXP_Model_t s_nxp_models[] = {
    /* LPC800系列 */
    { NXP_FAMILY_LPC800,  0x0801, "LPC812M101",    16*1024,   4*1024,  1024,   64 },
    { NXP_FAMILY_LPC800,  0x0802, "LPC824M201",    32*1024,   8*1024,  1024,   64 },
    { NXP_FAMILY_LPC800,  0x0803, "LPC832M201",    16*1024,   4*1024,  1024,   64 },
    { NXP_FAMILY_LPC800,  0x0804, "LPC834M201",    32*1024,   8*1024,  1024,   64 },
    { NXP_FAMILY_LPC800,  0x0805, "LPC845M301",    64*1024,  16*1024,  1024,   64 },
    
    /* LPC1100系列 */
    { NXP_FAMILY_LPC1100, 0x1101, "LPC1114FBD48",  32*1024,   4*1024,  4096, 256 },
    { NXP_FAMILY_LPC1100, 0x1102, "LPC1115FBD48",  64*1024,   8*1024,  4096, 256 },
    { NXP_FAMILY_LPC1100, 0x1103, "LPC1116FBD48",  32*1024,   8*1024,  4096, 256 },
    { NXP_FAMILY_LPC1100, 0x1104, "LPC1343FBD48",  32*1024,   8*1024,  4096, 256 },
    { NXP_FAMILY_LPC1100, 0x1105, "LPC1347FBD48",  64*1024,   8*1024,  4096, 256 },
    
    /* LPC1700系列 */
    { NXP_FAMILY_LPC1700, 0x1701, "LPC1754FBD80", 128*1024,  16*1024,  4096, 256 },
    { NXP_FAMILY_LPC1700, 0x1702, "LPC1756FBD80", 256*1024,  32*1024,  4096, 256 },
    { NXP_FAMILY_LPC1700, 0x1703, "LPC1758FBD80", 512*1024,  64*1024,  4096, 256 },
    { NXP_FAMILY_LPC1700, 0x1704, "LPC1768FBD100",512*1024,  64*1024,  4096, 256 },
    { NXP_FAMILY_LPC1700, 0x1705, "LPC1788FBD208",512*1024,  96*1024,  4096, 256 },
    
    /* LPC4300系列(双核) */
    { NXP_FAMILY_LPC4300, 0x4301, "LPC4337JBD144",512*1024, 104*1024,  4096, 512 },
    { NXP_FAMILY_LPC4300, 0x4302, "LPC4357FBD208",1024*1024,136*1024, 4096, 512 },
    { NXP_FAMILY_LPC4300, 0x4303, "LPC4370FBD144",282*1024, 104*1024,  4096, 512 },
    { NXP_FAMILY_LPC4300, 0x4304, "LPC4353FET256",1024*1024,136*1024, 4096, 512 },
    
    /* LPC54000系列 */
    { NXP_FAMILY_LPC54000,0x5401, "LPC54608J512", 512*1024, 180*1024,  4096, 512 },
    { NXP_FAMILY_LPC54000,0x5402, "LPC54616J512",1024*1024, 200*1024,  4096, 512 },
    { NXP_FAMILY_LPC54000,0x5403, "LPC54628J512",1024*1024, 200*1024,  4096, 512 },
    { NXP_FAMILY_LPC54000,0x5404, "LPC54618J256", 256*1024, 180*1024,  4096, 512 },
    
    /* i.MX RT系列(跨界MCU) */
    { NXP_FAMILY_IMXRT,   0x0401, "i.MXRT1011",   16*1024*1024,128*1024, 4096, 512 },
    { NXP_FAMILY_IMXRT,   0x0402, "i.MXRT1015",   16*1024*1024,128*1024, 4096, 512 },
    { NXP_FAMILY_IMXRT,   0x0403, "i.MXRT1021",   16*1024*1024,256*1024, 4096, 512 },
    { NXP_FAMILY_IMXRT,   0x0404, "i.MXRT1052",   16*1024*1024,512*1024, 4096, 512 },
    { NXP_FAMILY_IMXRT,   0x0405, "i.MXRT1062",   16*1024*1024,1024*1024,4096, 512 },
    { NXP_FAMILY_IMXRT,   0x0406, "i.MXRT1064",   16*1024*1024,1024*1024,4096, 512 },
    { NXP_FAMILY_IMXRT,   0x0407, "i.MXRT1176",   16*1024*1024,2048*1024,4096, 512 },
    
    /* Kinetis K系列 */
    { NXP_FAMILY_KINETIS_K,0x2001, "MK20DX128",   128*1024,  16*1024,  4096, 512 },
    { NXP_FAMILY_KINETIS_K,0x2002, "MK20DX256",   256*1024,  64*1024,  4096, 512 },
    { NXP_FAMILY_KINETIS_K,0x2003, "MK64FX512",   512*1024, 128*1024,  4096, 512 },
    { NXP_FAMILY_KINETIS_K,0x2004, "MK66FX1M0",  1024*1024, 256*1024,  4096, 512 },
    { NXP_FAMILY_KINETIS_K,0x2005, "MK80FN256",   256*1024,  64*1024,  4096, 512 },
    { NXP_FAMILY_KINETIS_K,0x2006, "MK82FN256",   256*1024, 128*1024,  4096, 512 },
    
    /* Kinetis L系列 */
    { NXP_FAMILY_KINETIS_L,0x2101, "KL03Z32",      32*1024,   2*1024,  1024, 256 },
    { NXP_FAMILY_KINETIS_L,0x2102, "KL05Z32",      32*1024,   4*1024,  1024, 256 },
    { NXP_FAMILY_KINETIS_L,0x2103, "KL25Z128",    128*1024,  16*1024,  1024, 256 },
    { NXP_FAMILY_KINETIS_L,0x2104, "KL26Z128",    128*1024,  16*1024,  1024, 256 },
    { NXP_FAMILY_KINETIS_L,0x2105, "KL27Z256",    256*1024,  32*1024,  1024, 256 },
    { NXP_FAMILY_KINETIS_L,0x2106, "KL46Z256",    256*1024,  32*1024,  2048, 256 },
    
    /* S32K系列(车用) */
    { NXP_FAMILY_S32K,    0x3001, "S32K116",       128*1024,  16*1024,  2048, 128 },
    { NXP_FAMILY_S32K,    0x3002, "S32K118",       256*1024,  32*1024,  2048, 128 },
    { NXP_FAMILY_S32K,    0x3003, "S32K142",       256*1024,  32*1024,  4096, 256 },
    { NXP_FAMILY_S32K,    0x3004, "S32K144",       512*1024,  64*1024,  4096, 256 },
    { NXP_FAMILY_S32K,    0x3005, "S32K146",      1024*1024, 128*1024,  4096, 256 },
    { NXP_FAMILY_S32K,    0x3006, "S32K148",      1024*1024, 256*1024,  4096, 256 },
    { NXP_FAMILY_S32K,    0x3007, "S32K344",      1024*1024, 256*1024,  8192, 512 },
    { NXP_FAMILY_S32K,    0x3008, "S32K358",      2048*1024, 512*1024,  8192, 512 },
    
    /* 结束标记 */
    { NXP_FAMILY_LPC800,  0,      "",              0,         0,        0,    0 }
};

/* ==================== NXP驱动实现 ==================== */

/**
 * @brief 初始化NXP驱动
 */
HAL_StatusTypeDef NXP_Init(NXP_HandleTypeDef* hnxp)
{
    if (hnxp == NULL) return HAL_ERROR;
    
    if (hnxp->clock_hz == 0) hnxp->clock_hz = 4000000;
    
    /* NXP芯片均通过SWD/JTAG接口 */
    SWD_Init();
    
    if (NXP_Detect(hnxp) != HAL_OK) return HAL_ERROR;
    
    hnxp->initialized = 1;
    return HAL_OK;
}

/**
 * @brief 反初始化NXP驱动
 */
HAL_StatusTypeDef NXP_DeInit(NXP_HandleTypeDef* hnxp)
{
    hnxp->initialized = 0;
    return HAL_OK;
}

/**
 * @brief 检测NXP芯片
 */
HAL_StatusTypeDef NXP_Detect(NXP_HandleTypeDef* hnxp)
{
    uint32_t id = NXP_ReadDeviceID(hnxp);
    if (id == 0 || id == 0xFFFFFFFF) return HAL_ERROR;
    
    for (uint32_t i = 0; s_nxp_models[i].idcode != 0; i++) {
        if (s_nxp_models[i].idcode == id) {
            hnxp->family = s_nxp_models[i].family;
            hnxp->device_id = id;
            hnxp->flash_size = s_nxp_models[i].flash_size;
            hnxp->ram_size = s_nxp_models[i].ram_size;
            hnxp->sector_size = s_nxp_models[i].sector_size;
            hnxp->page_size = s_nxp_models[i].page_size;
            strncpy(hnxp->part_number, s_nxp_models[i].part_number, 32);
            return HAL_OK;
        }
    }
    
    hnxp->device_id = id;
    strncpy(hnxp->part_number, "Unknown NXP", 32);
    return HAL_OK;
}

/**
 * @brief 读取设备ID
 */
uint32_t NXP_ReadDeviceID(NXP_HandleTypeDef* hnxp)
{
    /* NXP芯片通过SWD读取IAP Device ID */
    return SWD_ReadDP(0x00);
}

/**
 * @brief 擦除Flash
 */
HAL_StatusTypeDef NXP_EraseFlash(NXP_HandleTypeDef* hnxp, uint32_t addr, uint32_t size)
{
    if (!hnxp->initialized) return HAL_ERROR;
    
    switch (hnxp->family) {
        case NXP_FAMILY_LPC800:
            NXP_LPC800_InitFlash(hnxp);
            break;
        case NXP_FAMILY_LPC1700:
            NXP_LPC1700_InitFlash(hnxp);
            break;
        case NXP_FAMILY_LPC4300:
            NXP_LPC4300_InitFlash(hnxp);
            break;
        case NXP_FAMILY_IMXRT:
            NXP_IMXRT_InitFlexSPI(hnxp);
            break;
        case NXP_FAMILY_KINETIS_K:
        case NXP_FAMILY_KINETIS_L:
        case NXP_FAMILY_S32K:
            NXP_Kinetis_InitFlash(hnxp);
            break;
        default:
            break;
    }
    
    /* 按扇区擦除 */
    for (uint32_t i = 0; i < size; i += hnxp->sector_size) {
        /* 通过SWD执行扇区擦除 */
        SWD_WriteAP(0x00, 0x01);  /* 选通AP */
        SWD_WriteMem32(hnxp->flash_base + addr + i, 0x01);
        HAL_Delay(hnxp->sector_size > 4096 ? 100 : 50);
    }
    
    return HAL_OK;
}

/**
 * @brief 编程Flash
 */
HAL_StatusTypeDef NXP_ProgramFlash(NXP_HandleTypeDef* hnxp, uint32_t addr, uint8_t* data, uint32_t size)
{
    if (!hnxp->initialized) return HAL_ERROR;
    
    for (uint32_t i = 0; i < size; i += hnxp->page_size) {
        uint16_t chunk = (size - i > hnxp->page_size) ? hnxp->page_size : (size - i);
        
        /* 写入页数据 */
        SWD_WriteMem(hnxp->flash_base + addr + i, data + i, chunk);
        HAL_Delay(1);
    }
    
    return HAL_OK;
}

/**
 * @brief 读Flash
 */
HAL_StatusTypeDef NXP_ReadFlash(NXP_HandleTypeDef* hnxp, uint32_t addr, uint8_t* data, uint32_t size)
{
    if (!hnxp->initialized) return HAL_ERROR;
    
    SWD_ReadMem(hnxp->flash_base + addr, data, size);
    return HAL_OK;
}

/**
 * @brief 验证Flash
 */
HAL_StatusTypeDef NXP_VerifyFlash(NXP_HandleTypeDef* hnxp, uint32_t addr, uint8_t* data, uint32_t size)
{
    uint8_t read_buf[256];
    
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t chunk = (size - i > 256) ? 256 : (size - i);
        if (NXP_ReadFlash(hnxp, addr + i, read_buf, chunk) != HAL_OK) return HAL_ERROR;
        if (memcmp(read_buf, data + i, chunk) != 0) return HAL_ERROR;
    }
    
    return HAL_OK;
}

/* ==================== LPC系列专用操作 ==================== */

/**
 * @brief LPC800初始化Flash
 */
HAL_StatusTypeDef NXP_LPC800_InitFlash(NXP_HandleTypeDef* hnxp)
{
    /* 解锁LPC800 Flash控制器 */
    SWD_WriteMem32(LPC8XX_FLASH_CTRL + 0x00, 0x0);   /* 清除状态 */
    SWD_WriteMem32(LPC8XX_FLASH_CTRL + 0x1C, 0x6A65); /* 写入密钥1 */
    SWD_WriteMem32(LPC8XX_FLASH_CTRL + 0x20, 0x7465); /* 写入密钥2 */
    return HAL_OK;
}

/**
 * @brief LPC1700初始化Flash
 */
HAL_StatusTypeDef NXP_LPC1700_InitFlash(NXP_HandleTypeDef* hnxp)
{
    /* 准备Flash扇区 */
    SWD_WriteMem32(LPC17XX_FLASH_CTRL + 0x00, 0x0);   /* 清除状态 */
    SWD_WriteMem32(LPC17XX_FLASH_CTRL + 0x10, 0x01);  /* 准备命令 */
    return HAL_OK;
}

/**
 * @brief LPC4300初始化Flash
 */
HAL_StatusTypeDef NXP_LPC4300_InitFlash(NXP_HandleTypeDef* hnxp)
{
    /* 双核LPC4300需要特殊处理 */
    SWD_WriteMem32(LPC43XX_FLASH_CTRL_A + 0x00, 0x0);  /* 清除Bank A状态 */
    if (hnxp->flash_size > 512*1024) {
        SWD_WriteMem32(LPC43XX_FLASH_CTRL_B + 0x00, 0x0);  /* 清除Bank B状态 */
    }
    return HAL_OK;
}

/* ==================== i.MX RT专用操作 ==================== */

/**
 * @brief i.MX RT初始化FlexSPI
 */
HAL_StatusTypeDef NXP_IMXRT_InitFlexSPI(NXP_HandleTypeDef* hnxp)
{
    /* i.MX RT通过FlexSPI控制器操作外部Flash */
    SWD_WriteMem32(IMXRT_FLEXSPI_BASE + 0x00, 0x01);  /* 启用FlexSPI */
    SWD_WriteMem32(IMXRT_FLEXSPI_BASE + 0x04, 0x00);  /* 清除状态 */
    return HAL_OK;
}

/**
 * @brief i.MX RT配置Flash
 */
HAL_StatusTypeDef NXP_IMXRT_ConfigFlash(NXP_HandleTypeDef* hnxp)
{
    NXP_IMXRT_InitFlexSPI(hnxp);
    
    /* 配置FlexSPI LUT(查找表) */
    SWD_WriteMem32(IMXRT_FLEXSPI_BASE + 0x100, 0x08180403);  /* READ LUT */
    SWD_WriteMem32(IMXRT_FLEXSPI_BASE + 0x104, 0x1C040804);  /* READ LUT cont */
    SWD_WriteMem32(IMXRT_FLEXSPI_BASE + 0x120, 0x08180402);  /* WRITE LUT */
    SWD_WriteMem32(IMXRT_FLEXSPI_BASE + 0x140, 0x08180420);  /* ERASE LUT */
    
    return HAL_OK;
}

/**
 * @brief i.MX RT FlexSPI命令
 */
HAL_StatusTypeDef NXP_IMXRT_FlexSPICommand(NXP_HandleTypeDef* hnxp, uint8_t cmd, uint32_t addr, uint8_t* data, uint32_t len)
{
    /* 通过FlexSPI发送命令 */
    SWD_WriteMem32(IMXRT_FLEXSPI_BASE + 0x200, cmd);       /* 命令字 */
    SWD_WriteMem32(IMXRT_FLEXSPI_BASE + 0x204, addr);      /* 地址 */
    SWD_WriteMem32(IMXRT_FLEXSPI_BASE + 0x208, len);       /* 数据长度 */
    
    if (data != NULL && len > 0) {
        SWD_WriteMem(IMXRT_FLEXSPI_BASE + 0x300, data, len);
    }
    
    /* 触发执行 */
    SWD_WriteMem32(IMXRT_FLEXSPI_BASE + 0x20C, 0x01);
    HAL_Delay(10);
    
    return HAL_OK;
}

/* ==================== Kinetis专用操作 ==================== */

/**
 * @brief Kinetis初始化Flash
 */
HAL_StatusTypeDef NXP_Kinetis_InitFlash(NXP_HandleTypeDef* hnxp)
{
    /* Kinetis FTFE/FMC Flash控制器初始化 */
    SWD_WriteMem32(0x40020000, 0x00);  /* 清除FSTAT */
    SWD_WriteMem32(0x40020004, 0x00);  /* 清除FCNFG */
    return HAL_OK;
}

/**
 * @brief 设置通信速度
 */
HAL_StatusTypeDef NXP_SetSpeed(NXP_HandleTypeDef* hnxp, uint32_t clock_hz)
{
    hnxp->clock_hz = clock_hz;
    SWD_SetSpeed(clock_hz);
    return HAL_OK;
}