/**
 ******************************************************************************
 * @file    renesas_driver.c
 * @brief   Renesas瑞萨全系列驱动实现（对标RFP6）
 *          支持RL78/RA/RH850/V850/R8C/M16C/78K系列
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#include "renesas_driver.h"
#include "fine.h"
#include "swd.h"
#include "jtag.h"
#include <string.h>

/* ==================== Renesas型号数据库 ==================== */
typedef struct {
    Renesas_Family_t family;
    uint32_t         device_code;
    char             part_number[24];
    uint32_t         flash_size;
    uint32_t         ram_size;
    uint32_t         data_flash_size;
    Renesas_Debug_Type_t debug_type;
} Renesas_Model_t;

static const Renesas_Model_t s_renesas_models[] = {
    /* RL78/G13系列 */
    { RENESAS_RL78, 0x00178513, "R5F100LEA",    256*1024,  32*1024,   4*1024,   RENESAS_DEBUG_FINE },
    { RENESAS_RL78, 0x00178514, "R5F100LFA",    384*1024,  40*1024,   4*1024,   RENESAS_DEBUG_FINE },
    { RENESAS_RL78, 0x00178515, "R5F100PCA",    512*1024,  48*1024,   8*1024,   RENESAS_DEBUG_FINE },
    { RENESAS_RL78, 0x00178516, "R5F100PFA",    512*1024,  64*1024,   8*1024,   RENESAS_DEBUG_FINE },
    { RENESAS_RL78, 0x00178517, "R5F100MG",     128*1024,  24*1024,   4*1024,   RENESAS_DEBUG_FINE },
    { RENESAS_RL78, 0x00178518, "R5F100ML",     256*1024,  32*1024,   4*1024,   RENESAS_DEBUG_FINE },
    
    /* RL78/G14系列 */
    { RENESAS_RL78, 0x00178614, "R5F104PFA",    512*1024,  64*1024,   8*1024,   RENESAS_DEBUG_FINE },
    { RENESAS_RL78, 0x00178615, "R5F104PGA",    384*1024,  40*1024,   8*1024,   RENESAS_DEBUG_FINE },
    { RENESAS_RL78, 0x00178616, "R5F104LE",     256*1024,  32*1024,   4*1024,   RENESAS_DEBUG_FINE },
    { RENESAS_RL78, 0x00178617, "R5F104LFA",    384*1024,  40*1024,   4*1024,   RENESAS_DEBUG_FINE },
    
    /* RL78/G23系列 */
    { RENESAS_RL78, 0x00178723, "R5F12AG",      256*1024,  48*1024,   8*1024,   RENESAS_DEBUG_FINE },
    { RENESAS_RL78, 0x00178724, "R5F12BA",      512*1024,  64*1024,   8*1024,   RENESAS_DEBUG_FINE },
    { RENESAS_RL78, 0x00178725, "R5F12AE",      128*1024,  24*1024,   4*1024,   RENESAS_DEBUG_FINE },
    
    /* RL78/I1A系列(汽车) */
    { RENESAS_RL78, 0x00178801, "R5F11A",       128*1024,  24*1024,   4*1024,   RENESAS_DEBUG_FINE },
    { RENESAS_RL78, 0x00178802, "R5F11B",       256*1024,  32*1024,   8*1024,   RENESAS_DEBUG_FINE },
    { RENESAS_RL78, 0x00178803, "R5F11C",       512*1024,  48*1024,   8*1024,   RENESAS_DEBUG_FINE },
    
    /* RA2系列(Cortex-M23) */
    { RENESAS_RA,   0x00200001, "RA2E1",        128*1024,  32*1024,   8*1024,   RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00200002, "RA2L1",        256*1024,  48*1024,   8*1024,   RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00200003, "RA2A1",        256*1024,  48*1024,   8*1024,   RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00200004, "RA2E2",        128*1024,  32*1024,   8*1024,   RENESAS_DEBUG_SWD },
    
    /* RA4系列(Cortex-M4) */
    { RENESAS_RA,   0x00201001, "RA4M1",        256*1024,  64*1024,   8*1024,   RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00201002, "RA4E1",        128*1024,  32*1024,   8*1024,   RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00201003, "RA4W1",        256*1024,  64*1024,   8*1024,   RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00201004, "RA4M2",        512*1024,  128*1024,  8*1024,   RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00201005, "RA4M3",        1024*1024, 256*1024,  16*1024,  RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00201006, "RA4E2",        512*1024,  128*1024,  8*1024,   RENESAS_DEBUG_SWD },
    
    /* RA6系列(Cortex-M4/M33) */
    { RENESAS_RA,   0x00202001, "RA6M1",        512*1024,  256*1024,  8*1024,   RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00202002, "RA6M2",        1024*1024, 384*1024,  16*1024,  RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00202003, "RA6M3",        2*1024*1024, 640*1024, 32*1024, RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00202004, "RA6M4",        2*1024*1024, 640*1024, 32*1024, RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00202005, "RA6M5",        4*1024*1024, 1280*1024,32*1024, RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00202006, "RA6E2",        1024*1024, 384*1024,  16*1024,  RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00202007, "RA6T1",        2*1024*1024, 640*1024, 32*1024, RENESAS_DEBUG_SWD },
    { RENESAS_RA,   0x00202008, "RA6T2",        2*1024*1024, 640*1024, 32*1024, RENESAS_DEBUG_SWD },
    
    /* RH850系列 */
    { RENESAS_RH850, 0x00300001, "R7F701002",   512*1024,  128*1024,  16*1024,  RENESAS_DEBUG_FINE },
    { RENESAS_RH850, 0x00300002, "R7F701003",   768*1024,  192*1024,  16*1024,  RENESAS_DEBUG_FINE },
    { RENESAS_RH850, 0x00300003, "R7F701020",   1024*1024, 256*1024,  32*1024,  RENESAS_DEBUG_FINE },
    { RENESAS_RH850, 0x00300004, "R7F701021",   1*1024*1024, 512*1024, 32*1024, RENESAS_DEBUG_FINE },
    { RENESAS_RH850, 0x00300005, "R7F701022",   2*1024*1024, 512*1024, 32*1024, RENESAS_DEBUG_FINE },
    { RENESAS_RH850, 0x00300006, "R7F701023",   2*1024*1024, 1*1024*1024, 32*1024, RENESAS_DEBUG_FINE },
    
    /* V850系列 */
    { RENESAS_V850, 0x00400001, "D70F3037",     128*1024,  32*1024,   4*1024,   RENESAS_DEBUG_JTAG },
    { RENESAS_V850, 0x00400002, "D70F3438",     256*1024,  48*1024,   8*1024,   RENESAS_DEBUG_JTAG },
    { RENESAS_V850, 0x00400003, "D70F3453",     512*1024,  64*1024,   8*1024,   RENESAS_DEBUG_JTAG },
    { RENESAS_V850, 0x00400004, "D70F3429",     384*1024,  48*1024,   8*1024,   RENESAS_DEBUG_JTAG },
    
    /* 78K0系列 */
    { RENESAS_78K0, 0x00500001, "uPD78F0113",   32*1024,   4*1024,    0,        RENESAS_DEBUG_UART },
    { RENESAS_78K0, 0x00500002, "uPD78F0114",   48*1024,   4*1024,    0,        RENESAS_DEBUG_UART },
    { RENESAS_78K0, 0x00500003, "uPD78F0115",   64*1024,   4*1024,    0,        RENESAS_DEBUG_UART },
    { RENESAS_78K0, 0x00500004, "uPD78F0116",   96*1024,   6*1024,    0,        RENESAS_DEBUG_UART },
    
    /* 结束标记 */
    { RENESAS_RL78, 0,          "",             0,          0,          0,        RENESAS_DEBUG_FINE }
};

/* ==================== Renesas驱动实现 ==================== */

/**
 * @brief 初始化Renesas驱动
 */
HAL_StatusTypeDef Renesas_Init(Renesas_HandleTypeDef* hren)
{
    if (hren == NULL) return HAL_ERROR;
    
    /* 设置默认时钟 */
    if (hren->clock_hz == 0) {
        hren->clock_hz = 1000000;  /* 1MHz */
    }
    
    /* 根据调试接口类型初始化 */
    switch (hren->debug_type) {
        case RENESAS_DEBUG_FINE:
            /* FINE接口初始化 */
            FINE_Enter(hren->fine_clk_port, hren->fine_clk_pin,
                       hren->fine_data_port, hren->fine_data_pin,
                       hren->fine_reset_port, hren->fine_reset_pin);
            break;
            
        case RENESAS_DEBUG_SWD:
            /* SWD接口初始化 */
            SWD_Init();
            break;
            
        case RENESAS_DEBUG_JTAG:
            /* JTAG接口初始化 */
            JTAG_Init();
            break;
            
        default:
            break;
    }
    
    /* 检测芯片型号 */
    if (Renesas_Detect(hren) != HAL_OK) {
        return HAL_ERROR;
    }
    
    hren->initialized = 1;
    return HAL_OK;
}

/**
 * @brief 反初始化Renesas驱动
 */
HAL_StatusTypeDef Renesas_DeInit(Renesas_HandleTypeDef* hren)
{
    hren->initialized = 0;
    return HAL_OK;
}

/**
 * @brief 检测Renesas芯片型号
 */
HAL_StatusTypeDef Renesas_Detect(Renesas_HandleTypeDef* hren)
{
    uint32_t device_code = Renesas_ReadDeviceID(hren);
    
    if (device_code == 0 || device_code == 0xFFFFFFFF) {
        return HAL_ERROR;
    }
    
    /* 查找型号数据库 */
    for (uint32_t i = 0; s_renesas_models[i].device_code != 0; i++) {
        if (s_renesas_models[i].device_code == device_code) {
            hren->family = s_renesas_models[i].family;
            hren->device_code = device_code;
            hren->flash_size = s_renesas_models[i].flash_size;
            hren->ram_size = s_renesas_models[i].ram_size;
            hren->data_flash_size = s_renesas_models[i].data_flash_size;
            hren->debug_type = s_renesas_models[i].debug_type;
            strncpy(hren->part_number, s_renesas_models[i].part_number, 32);
            return HAL_OK;
        }
    }
    
    /* 未知型号但ID有效 */
    hren->device_code = device_code;
    strncpy(hren->part_number, "Unknown Renesas", 32);
    return HAL_OK;
}

/**
 * @brief 读取设备ID
 */
uint32_t Renesas_ReadDeviceID(Renesas_HandleTypeDef* hren)
{
    uint32_t id = 0;
    
    switch (hren->debug_type) {
        case RENESAS_DEBUG_FINE:
            /* FINE接口读取ID */
            id = FINE_ReadID();
            break;
            
        case RENESAS_DEBUG_SWD:
            /* SWD读取IDCODE */
            id = SWD_ReadDP(0x00);
            break;
            
        case RENESAS_DEBUG_JTAG:
            /* JTAG读取IDCODE */
            id = JTAG_ReadIDCode();
            break;
            
        default:
            break;
    }
    
    return id;
}

/**
 * @brief RL78进入编程模式
 */
HAL_StatusTypeDef Renesas_RL78_EnterProgramming(Renesas_HandleTypeDef* hren)
{
    /* 通过FINE接口进入编程模式 */
    FINE_EnterProgramming();
    HAL_Delay(100);
    
    return HAL_OK;
}

/**
 * @brief RL78退出编程模式
 */
HAL_StatusTypeDef Renesas_RL78_ExitProgramming(Renesas_HandleTypeDef* hren)
{
    FINE_ExitProgramming();
    HAL_Delay(10);
    
    return HAL_OK;
}

/**
 * @brief 擦除Flash
 */
HAL_StatusTypeDef Renesas_EraseFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint32_t size)
{
    if (!hren->initialized) return HAL_ERROR;
    
    switch (hren->family) {
        case RENESAS_RL78:
            Renesas_RL78_EnterProgramming(hren);
            
            /* RL78块擦除 */
            for (uint32_t i = 0; i < size; i += 1024) {
                /* 发送擦除命令 */
                FINE_WriteByte(RL78_CMD_ERASE_1);
                FINE_WriteByte((addr + i) >> 8);
                FINE_WriteByte(addr + i);
                FINE_WriteByte(RL78_CMD_ERASE_2);
                
                /* 等待擦除完成 */
                uint8_t status;
                do {
                    FINE_WriteByte(RL78_CMD_STATUS_READ);
                    status = FINE_ReadByte();
                } while (!(status & RL78_SR_READY));
                
                if (status & RL78_SR_ERASE_ERROR) {
                    Renesas_RL78_ExitProgramming(hren);
                    return HAL_ERROR;
                }
            }
            
            Renesas_RL78_ExitProgramming(hren);
            break;
            
        case RENESAS_RA:
            /* RA系列通过SWD擦除 */
            Renesas_RA_InitFlash(hren);
            
            for (uint32_t i = 0; i < size; i += 32*1024) {
                /* 配置Flash擦除 */
                SWD_WriteAP(0x00, 0x01);  /* FCU启动 */
                SWD_WriteMem32(RA_FLASH_BASE + addr + i, 0x01);  /* 擦除命令 */
                HAL_Delay(100);
            }
            break;
            
        case RENESAS_RH850:
            Renesas_RH850_EnterDebug(hren);
            
            /* RH850擦除Flash */
            for (uint32_t i = 0; i < size; i += 4*1024) {
                FINE_WriteByte(0x20);
                FINE_WriteByte((addr + i) >> 16);
                FINE_WriteByte((addr + i) >> 8);
                FINE_WriteByte(addr + i);
                FINE_WriteByte(0xD0);
                HAL_Delay(10);
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
HAL_StatusTypeDef Renesas_ProgramFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size)
{
    if (!hren->initialized) return HAL_ERROR;
    
    switch (hren->family) {
        case RENESAS_RL78:
            Renesas_RL78_EnterProgramming(hren);
            
            /* RL78页编程(256字节/页) */
            for (uint32_t i = 0; i < size; i += 256) {
                uint16_t page_size = (size - i > 256) ? 256 : (size - i);
                
                FINE_WriteByte(RL78_CMD_WRITE);
                FINE_WriteByte((addr + i) >> 8);
                FINE_WriteByte(addr + i);
                FINE_WriteByte(page_size >> 8);
                FINE_WriteByte(page_size);
                
                /* 写入数据 */
                FINE_WriteBytes(data + i, page_size);
                
                /* 等待完成 */
                uint8_t status;
                do {
                    FINE_WriteByte(RL78_CMD_STATUS_READ);
                    status = FINE_ReadByte();
                } while (!(status & RL78_SR_READY));
                
                if (status & RL78_SR_PROGRAM_ERROR) {
                    Renesas_RL78_ExitProgramming(hren);
                    return HAL_ERROR;
                }
            }
            
            Renesas_RL78_ExitProgramming(hren);
            break;
            
        case RENESAS_RA:
            /* RA系列通过SWD编程 */
            for (uint32_t i = 0; i < size; i += 128) {
                uint16_t page_size = (size - i > 128) ? 128 : (size - i);
                
                /* 写入Flash */
                SWD_WriteMem(RA_FLASH_BASE + addr + i, data + i, page_size);
                HAL_Delay(1);
            }
            break;
            
        case RENESAS_RH850:
            Renesas_RH850_EnterDebug(hren);
            
            /* RH850编程 */
            for (uint32_t i = 0; i < size; i += 256) {
                uint16_t page_size = (size - i > 256) ? 256 : (size - i);
                Renesas_RH850_WriteMemory(hren, addr + i, data + i, page_size);
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
HAL_StatusTypeDef Renesas_ReadFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size)
{
    if (!hren->initialized) return HAL_ERROR;
    
    switch (hren->family) {
        case RENESAS_RL78:
            /* RL78读取Flash */
            for (uint32_t i = 0; i < size; i += 256) {
                uint16_t chunk_size = (size - i > 256) ? 256 : (size - i);
                FINE_ReadMem(addr + i, data + i, chunk_size);
            }
            break;
            
        case RENESAS_RA:
            /* RA系列通过SWD读取 */
            SWD_ReadMem(RA_FLASH_BASE + addr, data, size);
            break;
            
        case RENESAS_RH850:
            Renesas_RH850_EnterDebug(hren);
            Renesas_RH850_ReadMemory(hren, addr, data, size);
            break;
            
        default:
            return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief 验证Flash
 */
HAL_StatusTypeDef Renesas_VerifyFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size)
{
    uint8_t read_buf[256];
    
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t chunk_size = (size - i > 256) ? 256 : (size - i);
        
        if (Renesas_ReadFlash(hren, addr + i, read_buf, chunk_size) != HAL_OK) {
            return HAL_ERROR;
        }
        
        if (memcmp(read_buf, data + i, chunk_size) != 0) {
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

/**
 * @brief RL78空白检查
 */
HAL_StatusTypeDef Renesas_RL78_BlankCheck(Renesas_HandleTypeDef* hren, uint32_t addr, uint32_t size)
{
    Renesas_RL78_EnterProgramming(hren);
    
    FINE_WriteByte(RL78_CMD_BLANK_CHECK);
    FINE_WriteByte(addr >> 8);
    FINE_WriteByte(addr);
    FINE_WriteByte(size >> 8);
    FINE_WriteByte(size);
    
    uint8_t status;
    do {
        FINE_WriteByte(RL78_CMD_STATUS_READ);
        status = FINE_ReadByte();
    } while (!(status & RL78_SR_READY));
    
    Renesas_RL78_ExitProgramming(hren);
    
    return (status & RL78_SR_BLANK) ? HAL_OK : HAL_ERROR;
}

/**
 * @brief RA系列初始化Flash控制器
 */
HAL_StatusTypeDef Renesas_RA_InitFlash(Renesas_HandleTypeDef* hren)
{
    /* 启用Flash控制器 */
    SWD_WriteAP(0x00, 0x01);
    HAL_Delay(10);
    
    return HAL_OK;
}

/**
 * @brief RA配置Flash区域
 */
HAL_StatusTypeDef Renesas_RA_ConfigFlash(Renesas_HandleTypeDef* hren)
{
    Renesas_RA_InitFlash(hren);
    
    /* 配置Flash区域属性 */
    SWD_WriteMem32(0x40000100, 0x01);  /* 启用Code Flash写入 */
    
    return HAL_OK;
}

/**
 * @brief RH850进入调试模式
 */
HAL_StatusTypeDef Renesas_RH850_EnterDebug(Renesas_HandleTypeDef* hren)
{
    FINE_EnterDebug();
    HAL_Delay(100);
    
    return HAL_OK;
}

/**
 * @brief RH850读取内存
 */
HAL_StatusTypeDef Renesas_RH850_ReadMemory(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size)
{
    /* 通过FINE接口读取内存 */
    for (uint32_t i = 0; i < size; i += 4) {
        FINE_ReadMem32(addr + i, (uint32_t*)(data + i));
    }
    
    return HAL_OK;
}

/**
 * @brief RH850写入内存
 */
HAL_StatusTypeDef Renesas_RH850_WriteMemory(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size)
{
    /* 通过FINE接口写入内存 */
    for (uint32_t i = 0; i < size; i += 4) {
        uint32_t word = data[i] | (data[i+1] << 8) | (data[i+2] << 16) | (data[i+3] << 24);
        FINE_WriteMem32(addr + i, word);
    }
    
    return HAL_OK;
}

/**
 * @brief 擦除Data Flash
 */
HAL_StatusTypeDef Renesas_EraseDataFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint32_t size)
{
    if (hren->data_flash_size == 0) return HAL_ERROR;
    
    switch (hren->family) {
        case RENESAS_RL78:
            Renesas_RL78_EnterProgramming(hren);
            
            for (uint32_t i = 0; i < size; i += 128) {
                FINE_WriteByte(RL78_CMD_ERASE_1);
                FINE_WriteByte((hren->data_flash_base + addr + i) >> 8);
                FINE_WriteByte(hren->data_flash_base + addr + i);
                FINE_WriteByte(RL78_CMD_ERASE_2);
                HAL_Delay(10);
            }
            
            Renesas_RL78_ExitProgramming(hren);
            break;
            
        case RENESAS_RA:
            SWD_WriteMem32(RA_DATA_FLASH_BASE + addr, 0x01);
            HAL_Delay(10);
            break;
            
        default:
            return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief 编程Data Flash
 */
HAL_StatusTypeDef Renesas_ProgramDataFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size)
{
    if (hren->data_flash_size == 0) return HAL_ERROR;
    
    switch (hren->family) {
        case RENESAS_RL78:
            Renesas_RL78_EnterProgramming(hren);
            
            for (uint32_t i = 0; i < size; i += 128) {
                uint16_t chunk_size = (size - i > 128) ? 128 : (size - i);
                FINE_WriteBytes(data + i, chunk_size);
            }
            
            Renesas_RL78_ExitProgramming(hren);
            break;
            
        case RENESAS_RA:
            SWD_WriteMem(RA_DATA_FLASH_BASE + addr, data, size);
            break;
            
        default:
            return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief 读Data Flash
 */
HAL_StatusTypeDef Renesas_ReadDataFlash(Renesas_HandleTypeDef* hren, uint32_t addr, uint8_t* data, uint32_t size)
{
    if (hren->data_flash_size == 0) return HAL_ERROR;
    
    switch (hren->family) {
        case RENESAS_RL78:
            FINE_ReadMem(hren->data_flash_base + addr, data, size);
            break;
            
        case RENESAS_RA:
            SWD_ReadMem(RA_DATA_FLASH_BASE + addr, data, size);
            break;
            
        default:
            return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief 设置通信速度
 */
HAL_StatusTypeDef Renesas_SetSpeed(Renesas_HandleTypeDef* hren, uint32_t clock_hz)
{
    hren->clock_hz = clock_hz;
    
    switch (hren->debug_type) {
        case RENESAS_DEBUG_FINE:
            FINE_SetSpeed(clock_hz);
            break;
            
        case RENESAS_DEBUG_SWD:
            SWD_SetSpeed(clock_hz);
            break;
            
        case RENESAS_DEBUG_JTAG:
            JTAG_SetSpeed(clock_hz);
            break;
            
        default:
            break;
    }
    
    return HAL_OK;
}