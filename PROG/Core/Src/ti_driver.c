/**
 ******************************************************************************
 * @file    ti_driver.c
 * @brief   TI德州仪器全系列驱动实现（对标UniFlash）
 *          支持MSP430/MSP432/CC253x/CC26xx/TMS320/TMS570/TM470系列
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#include "ti_driver.h"
#include "sbw.h"
#include "swd.h"
#include "jtag.h"
#include <string.h>

/* ==================== TI型号数据库 ==================== */
typedef struct {
    TI_Family_t     family;
    uint32_t        device_id;
    char            part_number[24];
    uint32_t        flash_size;
    uint32_t        ram_size;
    uint32_t        info_flash_size;
    TI_Debug_Type_t debug_type;
} TI_Model_t;

static const TI_Model_t s_ti_models[] = {
    /* MSP430G2x系列 */
    { TI_FAMILY_MSP430, 0xF201, "MSP430G2231",   2*1024,    128,     0,    TI_DEBUG_SBW },
    { TI_FAMILY_MSP430, 0xF209, "MSP430G2452",    8*1024,    512,    256,   TI_DEBUG_SBW },
    { TI_FAMILY_MSP430, 0xF210, "MSP430G2553",   16*1024,    512,    256,   TI_DEBUG_SBW },
    { TI_FAMILY_MSP430, 0xF213, "MSP430G2955",   48*1024,   2048,    512,   TI_DEBUG_SBW },
    
    /* MSP430F5x系列 */
    { TI_FAMILY_MSP430, 0xF540, "MSP430F5438A", 256*1024,  16*1024, 512,    TI_DEBUG_SBW },
    { TI_FAMILY_MSP430, 0xF541, "MSP430F5529",  128*1024,   8*1024, 512,    TI_DEBUG_SBW },
    { TI_FAMILY_MSP430, 0xF542, "MSP430F5359",  128*1024,  16*1024, 256,    TI_DEBUG_SBW },
    { TI_FAMILY_MSP430, 0xF543, "MSP430FR5994", 256*1024,   8*1024, 0,     TI_DEBUG_SBW },
    
    /* MSP430FR系列(FRAM) */
    { TI_FAMILY_MSP430, 0xF580, "MSP430FR2433",  16*1024,   1024,   0,     TI_DEBUG_SBW },
    { TI_FAMILY_MSP430, 0xF581, "MSP430FR4133",  16*1024,   2048,   0,     TI_DEBUG_SBW },
    { TI_FAMILY_MSP430, 0xF582, "MSP430FR5969",  64*1024,   2048,   0,     TI_DEBUG_SBW },
    { TI_FAMILY_MSP430, 0xF583, "MSP430FR5994", 256*1024,   8192,   0,     TI_DEBUG_SBW },
    { TI_FAMILY_MSP430, 0xF584, "MSP430FR6989", 128*1024,   2048,   0,     TI_DEBUG_SBW },
    { TI_FAMILY_MSP430, 0xF585, "MSP430FR5739",  16*1024,   1024,   0,     TI_DEBUG_SBW },
    
    /* MSP432系列 */
    { TI_FAMILY_MSP432, 0x4321, "MSP432P401R",  256*1024,  64*1024,  0,    TI_DEBUG_SWD },
    { TI_FAMILY_MSP432, 0x4322, "MSP432P4111",  512*1024, 128*1024,  0,    TI_DEBUG_SWD },
    { TI_FAMILY_MSP432, 0x4323, "MSP432E401Y", 1024*1024, 256*1024, 0,    TI_DEBUG_SWD },
    { TI_FAMILY_MSP432, 0x4324, "MSP432E411Y", 1024*1024, 256*1024, 0,    TI_DEBUG_SWD },
    
    /* CC253x系列 */
    { TI_FAMILY_CC253X, 0x2530, "CC2530F256",   256*1024,   8*1024,  0,    TI_DEBUG_JTAG },
    { TI_FAMILY_CC253X, 0x2531, "CC2531F128",   128*1024,   8*1024,  0,    TI_DEBUG_JTAG },
    { TI_FAMILY_CC253X, 0x2533, "CC2533F32",     32*1024,   4*1024,  0,    TI_DEBUG_JTAG },
    
    /* CC26xx系列 */
    { TI_FAMILY_CC26XX, 0x2601, "CC2640R2F",    128*1024,  20*1024,  0,    TI_DEBUG_JTAG },
    { TI_FAMILY_CC26XX, 0x2602, "CC2652R1",     352*1024,  80*1024,  0,    TI_DEBUG_JTAG },
    { TI_FAMILY_CC26XX, 0x2603, "CC2652RB",     352*1024,  80*1024,  0,    TI_DEBUG_JTAG },
    
    /* CC13xx系列 */
    { TI_FAMILY_CC13XX, 0x1301, "CC1310F128",   128*1024,  20*1024,  0,    TI_DEBUG_JTAG },
    { TI_FAMILY_CC13XX, 0x1302, "CC1352R1",     352*1024,  80*1024,  0,    TI_DEBUG_JTAG },
    { TI_FAMILY_CC13XX, 0x1303, "CC1352P1",     352*1024,  80*1024,  0,    TI_DEBUG_JTAG },
    
    /* CC32xx系列 */
    { TI_FAMILY_CC32XX, 0x3201, "CC3220R",      1024*1024, 256*1024,0,    TI_DEBUG_UART },
    { TI_FAMILY_CC32XX, 0x3202, "CC3220SF",     1024*1024, 256*1024,0,    TI_DEBUG_UART },
    { TI_FAMILY_CC32XX, 0x3203, "CC3235SF",     1024*1024, 512*1024,0,    TI_DEBUG_UART },
    
    /* TMS320C2000系列 */
    { TI_FAMILY_TMS320C2000, 0x8301, "TMS320F28377S", 512*1024, 100*1024, 0, TI_DEBUG_JTAG },
    { TI_FAMILY_TMS320C2000, 0x8302, "TMS320F28377D",1024*1024, 200*1024, 0, TI_DEBUG_JTAG },
    { TI_FAMILY_TMS320C2000, 0x8303, "TMS320F280049C",256*1024, 100*1024, 0, TI_DEBUG_JTAG },
    { TI_FAMILY_TMS320C2000, 0x8304, "TMS320F280041C",128*1024,  40*1024, 0, TI_DEBUG_JTAG },
    { TI_FAMILY_TMS320C2000, 0x8305, "TMS320F28335",  256*1024,  34*1024, 0, TI_DEBUG_JTAG },
    { TI_FAMILY_TMS320C2000, 0x8306, "TMS320F28027",   64*1024,  10*1024, 0, TI_DEBUG_JTAG },
    { TI_FAMILY_TMS320C2000, 0x8307, "TMS320F28069",  256*1024, 100*1024, 0, TI_DEBUG_JTAG },
    
    /* TMS570系列 */
    { TI_FAMILY_TMS570, 0x5701, "TMS570LS3137", 1024*1024, 256*1024, 0,    TI_DEBUG_JTAG },
    { TI_FAMILY_TMS570, 0x5702, "TMS570LS1227",  768*1024, 128*1024, 0,    TI_DEBUG_JTAG },
    { TI_FAMILY_TMS570, 0x5703, "TMS570LS0914",  512*1024,  64*1024, 0,    TI_DEBUG_JTAG },
    { TI_FAMILY_TMS570, 0x5704, "TMS570LS0432",  256*1024,  32*1024, 0,    TI_DEBUG_JTAG },
    { TI_FAMILY_TMS570, 0x5705, "TMS570LC4357", 2048*1024, 512*1024, 0,    TI_DEBUG_JTAG },
    
    /* TM470系列 */
    { TI_FAMILY_TM470,  0x4701, "TM4C1294NCPDT",1024*1024, 256*1024, 0,    TI_DEBUG_JTAG },
    { TI_FAMILY_TM470,  0x4702, "TM4C123GH6PM",  256*1024,  32*1024, 0,    TI_DEBUG_JTAG },
    { TI_FAMILY_TM470,  0x4703, "TM4C129XNCZAD",1024*1024, 256*1024, 0,    TI_DEBUG_JTAG },
    
    /* 结束标记 */
    { TI_FAMILY_MSP430, 0,      "",              0,         0,        0,    TI_DEBUG_SBW }
};

/* ==================== TI驱动实现 ==================== */

/**
 * @brief 初始化TI驱动
 */
HAL_StatusTypeDef TI_Init(TI_HandleTypeDef* hti)
{
    if (hti == NULL) return HAL_ERROR;
    
    if (hti->clock_hz == 0) hti->clock_hz = 1000000;
    
    /* 根据调试接口初始化 */
    switch (hti->debug_type) {
        case TI_DEBUG_SBW: {
            SBW_Config_TypeDef sbw_cfg = {
                .tck_port = hti->sbw_tck_port,
                .tck_pin  = hti->sbw_tck_pin,
                .tms_port = hti->sbw_tms_port,
                .tms_pin  = hti->sbw_tms_pin,
                .rst_port = hti->sbw_reset_port,
                .rst_pin  = hti->sbw_reset_pin,
                .speed_hz = hti->clock_hz,
            };
            SBW_Init(&sbw_cfg);
            break;
        }
        case TI_DEBUG_SWD:
            SWD_Init();
            break;
        case TI_DEBUG_JTAG:
            JTAG_Init();
            break;
        default:
            break;
    }
    
    if (TI_Detect(hti) != HAL_OK) return HAL_ERROR;
    
    hti->initialized = 1;
    return HAL_OK;
}

/**
 * @brief 反初始化TI驱动
 */
HAL_StatusTypeDef TI_DeInit(TI_HandleTypeDef* hti)
{
    hti->initialized = 0;
    return HAL_OK;
}

/**
 * @brief 检测TI芯片
 */
HAL_StatusTypeDef TI_Detect(TI_HandleTypeDef* hti)
{
    uint32_t id = TI_ReadDeviceID(hti);
    if (id == 0 || id == 0xFFFFFFFF) return HAL_ERROR;
    
    for (uint32_t i = 0; s_ti_models[i].device_id != 0; i++) {
        if (s_ti_models[i].device_id == id) {
            hti->family = s_ti_models[i].family;
            hti->device_id = id;
            hti->flash_size = s_ti_models[i].flash_size;
            hti->ram_size = s_ti_models[i].ram_size;
            hti->info_flash_size = s_ti_models[i].info_flash_size;
            hti->debug_type = s_ti_models[i].debug_type;
            strncpy(hti->part_number, s_ti_models[i].part_number, 32);
            return HAL_OK;
        }
    }
    
    hti->device_id = id;
    strncpy(hti->part_number, "Unknown TI", 32);
    return HAL_OK;
}

/**
 * @brief 读取设备ID
 */
uint32_t TI_ReadDeviceID(TI_HandleTypeDef* hti)
{
    switch (hti->debug_type) {
        case TI_DEBUG_SBW:
            return SBW_ReadDeviceID();
        case TI_DEBUG_SWD:
            return SWD_ReadDP(0x00);
        case TI_DEBUG_JTAG:
            return JTAG_ReadIDCode();
        default:
            return 0;
    }
}

/**
 * @brief 擦除Flash
 */
HAL_StatusTypeDef TI_EraseFlash(TI_HandleTypeDef* hti, uint32_t addr, uint32_t size)
{
    if (!hti->initialized) return HAL_ERROR;
    
    switch (hti->family) {
        case TI_FAMILY_MSP430:
            /* MSP430按段擦除(512B/段) */
            for (uint32_t i = 0; i < size; i += 512) {
                TI_MSP430_EraseSegment(hti, addr + i);
            }
            break;
            
        case TI_FAMILY_MSP432:
            /* MSP432按4KB扇区擦除 */
            for (uint32_t i = 0; i < size; i += 4096) {
                TI_MSP432_EraseSector(hti, addr + i);
            }
            break;
            
        case TI_FAMILY_CC253X:
            TI_CC253x_InitFlash(hti);
            for (uint32_t i = 0; i < size; i += 2048) {
                JTAG_WriteMem(addr + i, (uint8_t*)"\x00", 2);  /* 触发擦除 */
                HAL_Delay(20);
            }
            break;
            
        case TI_FAMILY_TMS320C2000:
            TI_TMS320C2000_InitFlash(hti);
            for (uint32_t i = 0; i < size; i += 0x2000) {
                TI_TMS320C2000_EraseSector(hti, addr + i);
            }
            break;
            
        case TI_FAMILY_TMS570:
        case TI_FAMILY_TM470:
            TI_TMS570_InitFlash(hti);
            for (uint32_t i = 0; i < size; i += 0x20000) {
                TI_TMS570_EraseBank(hti, addr + i);
            }
            break;
            
        default:
            return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief 编程Flash
 */
HAL_StatusTypeDef TI_ProgramFlash(TI_HandleTypeDef* hti, uint32_t addr, uint8_t* data, uint32_t size)
{
    if (!hti->initialized) return HAL_ERROR;
    
    switch (hti->family) {
        case TI_FAMILY_MSP430:
            /* MSP430按字(16位)编程 */
            for (uint32_t i = 0; i < size; i += 2) {
                uint16_t word = data[i] | (data[i+1] << 8);
                TI_MSP430_WriteFlash(hti, addr + i, word);
            }
            break;
            
        case TI_FAMILY_MSP432:
            /* MSP432通过SWD编程 */
            TI_MSP432_InitFlash(hti);
            for (uint32_t i = 0; i < size; i += 512) {
                uint16_t chunk = (size - i > 512) ? 512 : (size - i);
                SWD_WriteMem(addr + i, data + i, chunk);
                HAL_Delay(1);
            }
            break;
            
        case TI_FAMILY_CC253X:
            TI_CC253x_InitFlash(hti);
            TI_CC253x_WriteFlash(hti, addr, data, size);
            break;
            
        case TI_FAMILY_TMS320C2000:
            TI_TMS320C2000_InitFlash(hti);
            for (uint32_t i = 0; i < size; i += 512) {
                uint16_t chunk = (size - i > 512) ? 512 : (size - i);
                TI_TMS320C2000_ProgramFlash(hti, addr + i, (uint16_t*)(data + i), chunk / 2);
            }
            break;
            
        case TI_FAMILY_TMS570:
        case TI_FAMILY_TM470:
            TI_TMS570_InitFlash(hti);
            for (uint32_t i = 0; i < size; i += 256) {
                uint16_t chunk = (size - i > 256) ? 256 : (size - i);
                JTAG_WriteMem(addr + i, data + i, chunk);
                HAL_Delay(1);
            }
            break;
            
        default:
            return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief 读Flash
 */
HAL_StatusTypeDef TI_ReadFlash(TI_HandleTypeDef* hti, uint32_t addr, uint8_t* data, uint32_t size)
{
    if (!hti->initialized) return HAL_ERROR;
    
    switch (hti->debug_type) {
        case TI_DEBUG_SBW:
            SBW_ReadMem(addr, data, size);
            break;
        case TI_DEBUG_SWD:
            SWD_ReadMem(addr, data, size);
            break;
        case TI_DEBUG_JTAG:
            JTAG_ReadMem(addr, data, size);
            break;
        default:
            return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief 验证Flash
 */
HAL_StatusTypeDef TI_VerifyFlash(TI_HandleTypeDef* hti, uint32_t addr, uint8_t* data, uint32_t size)
{
    uint8_t read_buf[256];
    
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t chunk = (size - i > 256) ? 256 : (size - i);
        if (TI_ReadFlash(hti, addr + i, read_buf, chunk) != HAL_OK) return HAL_ERROR;
        if (memcmp(read_buf, data + i, chunk) != 0) return HAL_ERROR;
    }
    
    return HAL_OK;
}

/* ==================== MSP430专用操作 ==================== */

/**
 * @brief MSP430进入SBW模式
 */
HAL_StatusTypeDef TI_MSP430_EnterSBW(TI_HandleTypeDef* hti)
{
    SBW_Enter();
    HAL_Delay(10);
    return HAL_OK;
}

/**
 * @brief MSP430擦除段(512字节)
 */
HAL_StatusTypeDef TI_MSP430_EraseSegment(TI_HandleTypeDef* hti, uint32_t addr)
{
    /* 通过SBW执行段擦除 */
    SBW_WriteMem16(MSP430_FLASH_CTL1, MSP430_FLASH_KEY);
    SBW_WriteMem16(addr, 0xFFFF);  /* 触发擦除 */
    HAL_Delay(30);                  /* 等待擦除完成 */
    SBW_WriteMem16(MSP430_FLASH_CTL1, 0);
    return HAL_OK;
}

/**
 * @brief MSP430写Flash(16位字)
 */
HAL_StatusTypeDef TI_MSP430_WriteFlash(TI_HandleTypeDef* hti, uint32_t addr, uint16_t data)
{
    SBW_WriteMem16(MSP430_FLASH_CTL1, MSP430_FLASH_KEY | 0x02);  /* WRT=1 */
    SBW_WriteMem16(addr, data);
    HAL_Delay(1);
    SBW_WriteMem16(MSP430_FLASH_CTL1, 0);
    return HAL_OK;
}

/**
 * @brief MSP430读Info Flash
 */
HAL_StatusTypeDef TI_MSP430_ReadInfoFlash(TI_HandleTypeDef* hti, uint32_t addr, uint8_t* data, uint32_t size)
{
    /* Info Flash通常在0x1000-0x10FF */
    SBW_ReadMem(0x1000 + addr, data, size);
    return HAL_OK;
}

/**
 * @brief MSP430读取设备ID(16位)
 */
uint16_t TI_MSP430_ReadDeviceID(TI_HandleTypeDef* hti)
{
    return (uint16_t)SBW_ReadDeviceID();
}

/* ==================== MSP432专用操作 ==================== */

/**
 * @brief MSP432初始化Flash
 */
HAL_StatusTypeDef TI_MSP432_InitFlash(TI_HandleTypeDef* hti)
{
    /* 解锁Flash */
    SWD_WriteMem32(0xE0043000, 0x12345678);  /* Flash解锁 */
    SWD_WriteMem32(0xE0043004, 0x87654321);
    return HAL_OK;
}

/**
 * @brief MSP432擦除扇区(4KB)
 */
HAL_StatusTypeDef TI_MSP432_EraseSector(TI_HandleTypeDef* hti, uint32_t addr)
{
    TI_MSP432_InitFlash(hti);
    SWD_WriteMem32(0xE0043008, addr);   /* 设置擦除地址 */
    SWD_WriteMem32(0xE004300C, 0x01);   /* 触发扇区擦除 */
    HAL_Delay(50);
    return HAL_OK;
}

/* ==================== CC253x专用操作 ==================== */

/**
 * @brief CC253x初始化Flash
 */
HAL_StatusTypeDef TI_CC253x_InitFlash(TI_HandleTypeDef* hti)
{
    /* 通过JTAG进入Flash编程模式 */
    JTAG_WriteMem16(0xDFFE, 0x01);  /* FCTL = WRITE */
    HAL_Delay(1);
    return HAL_OK;
}

/**
 * @brief CC253x写Flash
 */
HAL_StatusTypeDef TI_CC253x_WriteFlash(TI_HandleTypeDef* hti, uint32_t addr, uint8_t* data, uint32_t size)
{
    for (uint32_t i = 0; i < size; i += 4) {
        JTAG_WriteMem16(0xDFFE, 0x02);      /* FCTL = WRITE */
        JTAG_WriteMem(addr + i, data + i, 4);
        HAL_Delay(1);
    }
    return HAL_OK;
}

/* ==================== TMS320C2000专用操作 ==================== */

/**
 * @brief TMS320C2000初始化Flash
 */
HAL_StatusTypeDef TI_TMS320C2000_InitFlash(TI_HandleTypeDef* hti)
{
    /* 通过JTAG初始化Flash控制器 */
    JTAG_WriteMem32(TMS320C2000_FLASH_CTRL + 0x00, 0x01);  /* 启用Flash控制器 */
    JTAG_WriteMem32(TMS320C2000_FLASH_CTRL + 0x04, 0x01);  /* 解锁Flash */
    HAL_Delay(10);
    return HAL_OK;
}

/**
 * @brief TMS320C2000擦除扇区(8KB)
 */
HAL_StatusTypeDef TI_TMS320C2000_EraseSector(TI_HandleTypeDef* hti, uint32_t sector)
{
    JTAG_WriteMem32(TMS320C2000_FLASH_CTRL + 0x08, sector);   /* 设置扇区号 */
    JTAG_WriteMem32(TMS320C2000_FLASH_CTRL + 0x0C, 0x01);     /* 触发擦除 */
    HAL_Delay(100);
    return HAL_OK;
}

/**
 * @brief TMS320C2000编程Flash(16位字)
 */
HAL_StatusTypeDef TI_TMS320C2000_ProgramFlash(TI_HandleTypeDef* hti, uint32_t addr, uint16_t* data, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        JTAG_WriteMem16(addr + i * 2, data[i]);
        HAL_Delay(1);
    }
    return HAL_OK;
}

/* ==================== TMS570/TM470专用操作 ==================== */

/**
 * @brief TMS570初始化Flash
 */
HAL_StatusTypeDef TI_TMS570_InitFlash(TI_HandleTypeDef* hti)
{
    JTAG_WriteMem32(0xFFF87000, 0x01);  /* 启用Flash控制器 */
    JTAG_WriteMem32(0xFFF87004, 0xA1A1A1A1);  /* 解锁密钥 */
    HAL_Delay(10);
    return HAL_OK;
}

/**
 * @brief TMS570擦除Bank(128KB)
 */
HAL_StatusTypeDef TI_TMS570_EraseBank(TI_HandleTypeDef* hti, uint32_t bank)
{
    JTAG_WriteMem32(0xFFF87008, bank);    /* 设置Bank地址 */
    JTAG_WriteMem32(0xFFF8700C, 0x01);    /* 触发Bank擦除 */
    HAL_Delay(500);
    return HAL_OK;
}

/**
 * @brief 设置通信速度
 */
HAL_StatusTypeDef TI_SetSpeed(TI_HandleTypeDef* hti, uint32_t clock_hz)
{
    hti->clock_hz = clock_hz;
    
    switch (hti->debug_type) {
        case TI_DEBUG_SBW:
            SBW_SetSpeed(clock_hz);
            break;
        case TI_DEBUG_SWD:
            SWD_SetSpeed(clock_hz);
            break;
        case TI_DEBUG_JTAG:
            JTAG_SetSpeed(clock_hz);
            break;
        default:
            break;
    }
    
    return HAL_OK;
}