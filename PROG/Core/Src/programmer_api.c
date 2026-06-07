/**
 ******************************************************************************
 * @file    programmer_api.c
 * @brief   统一编程器API接口实现
 *          提供上层应用调用的统一编程接口，屏蔽底层驱动差异
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#include "programmer_api.h"
#include "swd.h"
#include "jtag.h"
#include "bdm.h"
#include "sbw.h"
#include "mon8.h"
#include "fine.h"
#include "icsp.h"
#include "swim.h"
#include "spi_flash_driver.h"
#include "dsp_fpga_driver.h"
#include "renesas_driver.h"
#include "ti_driver.h"
#include "nxp_driver.h"
#include "infineon_tc_dap.h"
#include "chip_driver_framework.h"
#include "chip_cache.h"
#include <string.h>

/* ==================== 全局变量 ==================== */
static Programmer_Config_t  s_config = {0};
static Programmer_Status_t  s_status = {0};

/* 子驱动句柄 */
static SPI_Flash_HandleTypeDef s_spi_flash = {0};
static Renesas_HandleTypeDef   s_renesas = {0};
static TI_HandleTypeDef        s_ti = {0};
static NXP_HandleTypeDef       s_nxp = {0};
static DSP_HandleTypeDef       s_dsp = {0};
static FPGA_HandleTypeDef      s_fpga = {0};
static TC_DAP_HandleTypeDef    s_tc_dap = {0};

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 设置错误信息
 */
static void SetError(const char* msg)
{
    strncpy(s_status.last_error, msg, sizeof(s_status.last_error) - 1);
}

/**
 * @brief 更新进度
 */
static void UpdateProgress(uint32_t current, uint32_t total, const char* operation)
{
    s_status.progress_current = current;
    s_status.progress_total = total;
    
    if (s_config.progress_cb != NULL) {
        s_config.progress_cb(current, total, operation);
    }
}

/* ==================== 核心API实现 ==================== */

/**
 * @brief 初始化编程器
 */
Programmer_Result_t Programmer_Init(Programmer_Config_t* config)
{
    if (config == NULL) return PROG_ERROR_PARAMETER;
    
    memcpy(&s_config, config, sizeof(Programmer_Config_t));
    
    /* 初始化缓存模块 */
    Chip_Cache_Init();
    
    /* 初始化芯片驱动框架 */
    s_status.current_mode = config->mode;
    s_status.connected_chip = 0;
    s_status.chip_info = NULL;
    s_status.operation_status = 0;
    s_status.progress_current = 0;
    s_status.progress_total = 0;
    memset(s_status.last_error, 0, sizeof(s_status.last_error));
    
    return PROG_OK;
}

/**
 * @brief 反初始化编程器
 */
Programmer_Result_t Programmer_DeInit(void)
{
    Programmer_Disconnect();
    memset(&s_config, 0, sizeof(Programmer_Config_t));
    memset(&s_status, 0, sizeof(Programmer_Status_t));
    return PROG_OK;
}

/**
 * @brief 设置编程器模式
 */
Programmer_Result_t Programmer_SetMode(Programmer_Mode_t mode)
{
    /* 如果正在连接，先断开 */
    if (s_status.connected_chip != 0) {
        Programmer_Disconnect();
    }
    
    s_status.current_mode = mode;
    return PROG_OK;
}

/**
 * @brief 连接目标芯片
 */
Programmer_Result_t Programmer_Connect(void)
{
    switch (s_status.current_mode) {
        case PROG_MODE_CHIP_PROGRAM: {
            /* 根据芯片ID自动选择驱动 */
            uint32_t chip_id;
            if (Programmer_AutoDetect(&chip_id) != PROG_OK) {
                SetError("Auto detect failed");
                return PROG_ERROR_CONNECT;
            }
            break;
        }
        
        case PROG_MODE_SPI_FLASH:
            if (SPI_Flash_Init(&s_spi_flash) != HAL_OK) {
                SetError("SPI Flash init failed");
                return PROG_ERROR_CONNECT;
            }
            s_status.connected_chip = s_spi_flash.info.manufacturer_id << 16 | 
                                      s_spi_flash.info.device_id[0] << 8 |
                                      s_spi_flash.info.device_id[1];
            break;
        
        case PROG_MODE_DSP_PROGRAM:
            if (DSP_Init(&s_dsp) != HAL_OK) {
                SetError("DSP init failed");
                return PROG_ERROR_CONNECT;
            }
            s_status.connected_chip = s_dsp.device_id;
            break;
        
        case PROG_MODE_FPGA_CONFIG:
            if (FPGA_Init(&s_fpga) != HAL_OK) {
                SetError("FPGA init failed");
                return PROG_ERROR_CONNECT;
            }
            s_status.connected_chip = s_fpga.device_id;
            break;
        
        default:
            SetError("Unsupported mode");
            return PROG_ERROR_NOT_SUPPORTED;
    }
    
    return PROG_OK;
}

/**
 * @brief 断开目标芯片
 */
Programmer_Result_t Programmer_Disconnect(void)
{
    switch (s_status.current_mode) {
        case PROG_MODE_SPI_FLASH:
            SPI_Flash_DeInit(&s_spi_flash);
            break;
        case PROG_MODE_DSP_PROGRAM:
            DSP_DeInit(&s_dsp);
            break;
        case PROG_MODE_FPGA_CONFIG:
            FPGA_DeInit(&s_fpga);
            break;
        default:
            break;
    }
    
    s_status.connected_chip = 0;
    s_status.chip_info = NULL;
    
    return PROG_OK;
}

/**
 * @brief 自动检测目标芯片
 */
Programmer_Result_t Programmer_AutoDetect(uint32_t* detected_chip)
{
    if (detected_chip == NULL) return PROG_ERROR_PARAMETER;
    
    /* 尝试通过SWD读取ID */
    SWD_Init();
    uint32_t idcode = SWD_ReadDP(0x00);
    
    if (idcode != 0 && idcode != 0xFFFFFFFF) {
        *detected_chip = idcode;
        s_status.connected_chip = idcode;
        
        /* 在芯片数据库中查找 */
        const Chip_Info_t* info = Chip_FindByID(idcode);
        if (info != NULL) {
            s_status.chip_info = info;
            
            /* 缓存查找结果 */
            Chip_Cache_Add(idcode, idcode, 0, 0, NULL, info, NULL);
        }
        
        return PROG_OK;
    }
    
    /* 尝试JTAG */
    JTAG_Init();
    idcode = JTAG_ReadIDCode();
    
    if (idcode != 0 && idcode != 0xFFFFFFFF) {
        *detected_chip = idcode;
        s_status.connected_chip = idcode;
        return PROG_OK;
    }
    
    SetError("No chip detected");
    return PROG_ERROR_DETECT;
}

/**
 * @brief 获取芯片信息
 */
Programmer_Result_t Programmer_GetChipInfo(const Chip_Info_t** chip_info)
{
    if (chip_info == NULL) return PROG_ERROR_PARAMETER;
    *chip_info = s_status.chip_info;
    return (s_status.chip_info != NULL) ? PROG_OK : PROG_ERROR_DETECT;
}

/**
 * @brief 擦除Flash
 */
Programmer_Result_t Programmer_EraseFlash(uint32_t addr, uint32_t size)
{
    UpdateProgress(0, size, "Erasing...");
    
    Programmer_Result_t result = PROG_OK;
    
    switch (s_status.current_mode) {
        case PROG_MODE_SPI_FLASH:
            if (addr == 0 && size >= s_spi_flash.info.capacity) {
                result = (SPI_Flash_ChipErase(&s_spi_flash) == HAL_OK) ? PROG_OK : PROG_ERROR_ERASE;
            } else if (size <= 4096) {
                result = (SPI_Flash_SectorErase4K(&s_spi_flash, addr) == HAL_OK) ? PROG_OK : PROG_ERROR_ERASE;
            } else {
                result = (SPI_Flash_BlockErase64K(&s_spi_flash, addr) == HAL_OK) ? PROG_OK : PROG_ERROR_ERASE;
            }
            break;
        
        case PROG_MODE_CHIP_PROGRAM:
            /* 通过SWD/JTAG擦除 */
            if (s_status.chip_info != NULL) {
                /* 调用芯片特定的擦除函数 */
                result = PROG_OK;
            } else {
                result = PROG_ERROR_NOT_SUPPORTED;
            }
            break;
        
        default:
            result = PROG_ERROR_NOT_SUPPORTED;
            break;
    }
    
    UpdateProgress(size, size, "Erase complete");
    return result;
}

/**
 * @brief 编程Flash
 */
Programmer_Result_t Programmer_ProgramFlash(uint32_t addr, uint8_t* data, uint32_t size)
{
    if (data == NULL || size == 0) return PROG_ERROR_PARAMETER;
    
    UpdateProgress(0, size, "Programming...");
    
    Programmer_Result_t result = PROG_OK;
    
    switch (s_status.current_mode) {
        case PROG_MODE_SPI_FLASH:
            result = (SPI_Flash_Write(&s_spi_flash, addr, data, size) == HAL_OK) ? PROG_OK : PROG_ERROR_PROGRAM;
            break;
        
        case PROG_MODE_CHIP_PROGRAM:
            /* 通过SWD写入Flash */
            SWD_WriteMem(addr, data, size);
            result = PROG_OK;
            break;
        
        case PROG_MODE_DSP_PROGRAM:
            result = (DSP_ProgramFlash(&s_dsp, addr, data, size) == HAL_OK) ? PROG_OK : PROG_ERROR_PROGRAM;
            break;
        
        default:
            result = PROG_ERROR_NOT_SUPPORTED;
            break;
    }
    
    UpdateProgress(size, size, "Program complete");
    return result;
}

/**
 * @brief 读Flash
 */
Programmer_Result_t Programmer_ReadFlash(uint32_t addr, uint8_t* data, uint32_t size)
{
    if (data == NULL || size == 0) return PROG_ERROR_PARAMETER;
    
    UpdateProgress(0, size, "Reading...");
    
    Programmer_Result_t result = PROG_OK;
    
    switch (s_status.current_mode) {
        case PROG_MODE_SPI_FLASH:
            result = (SPI_Flash_Read(&s_spi_flash, addr, data, size) == HAL_OK) ? PROG_OK : PROG_ERROR_READ;
            break;
        
        case PROG_MODE_CHIP_PROGRAM:
            SWD_ReadMem(addr, data, size);
            result = PROG_OK;
            break;
        
        case PROG_MODE_DSP_PROGRAM:
            result = (DSP_ReadFlash(&s_dsp, addr, data, size) == HAL_OK) ? PROG_OK : PROG_ERROR_READ;
            break;
        
        default:
            result = PROG_ERROR_NOT_SUPPORTED;
            break;
    }
    
    UpdateProgress(size, size, "Read complete");
    return result;
}

/**
 * @brief 验证Flash
 */
Programmer_Result_t Programmer_VerifyFlash(uint32_t addr, uint8_t* data, uint32_t size)
{
    if (data == NULL || size == 0) return PROG_ERROR_PARAMETER;
    
    uint8_t read_buf[256];
    UpdateProgress(0, size, "Verifying...");
    
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t chunk = (size - i > 256) ? 256 : (size - i);
        
        if (Programmer_ReadFlash(addr + i, read_buf, chunk) != PROG_OK) {
            SetError("Verify read failed");
            return PROG_ERROR_VERIFY;
        }
        
        if (memcmp(read_buf, data + i, chunk) != 0) {
            SetError("Verify mismatch");
            return PROG_ERROR_VERIFY;
        }
        
        UpdateProgress(i + chunk, size, "Verifying...");
    }
    
    UpdateProgress(size, size, "Verify complete");
    return PROG_OK;
}

/**
 * @brief 全片擦除
 */
Programmer_Result_t Programmer_ChipErase(void)
{
    switch (s_status.current_mode) {
        case PROG_MODE_SPI_FLASH:
            return (SPI_Flash_ChipErase(&s_spi_flash) == HAL_OK) ? PROG_OK : PROG_ERROR_ERASE;
        default:
            return Programmer_EraseFlash(0, 0xFFFFFFFF);
    }
}

/**
 * @brief 空片检查
 */
Programmer_Result_t Programmer_BlankCheck(uint32_t addr, uint32_t size)
{
    uint8_t read_buf[256];
    
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t chunk = (size - i > 256) ? 256 : (size - i);
        
        if (Programmer_ReadFlash(addr + i, read_buf, chunk) != PROG_OK) {
            return PROG_ERROR_READ;
        }
        
        for (uint16_t j = 0; j < chunk; j++) {
            if (read_buf[j] != 0xFF) {
                SetError("Blank check failed: non-0xFF found");
                return PROG_ERROR_VERIFY;
            }
        }
    }
    
    return PROG_OK;
}

/**
 * @brief 解锁芯片
 */
Programmer_Result_t Programmer_Unlock(void)
{
    switch (s_status.current_mode) {
        case PROG_MODE_SPI_FLASH:
            return (SPI_Flash_GlobalUnlock(&s_spi_flash) == HAL_OK) ? PROG_OK : PROG_ERROR_PROGRAM;
        default:
            return PROG_OK;
    }
}

/**
 * @brief 加锁芯片
 */
Programmer_Result_t Programmer_Lock(void)
{
    switch (s_status.current_mode) {
        case PROG_MODE_SPI_FLASH:
            return (SPI_Flash_GlobalLock(&s_spi_flash) == HAL_OK) ? PROG_OK : PROG_ERROR_PROGRAM;
        default:
            return PROG_OK;
    }
}

/**
 * @brief 获取状态
 */
Programmer_Result_t Programmer_GetStatus(Programmer_Status_t* status)
{
    if (status == NULL) return PROG_ERROR_PARAMETER;
    memcpy(status, &s_status, sizeof(Programmer_Status_t));
    return PROG_OK;
}

/**
 * @brief 设置通信速度
 */
Programmer_Result_t Programmer_SetSpeed(uint32_t clock_hz)
{
    s_config.clock_hz = clock_hz;
    
    switch (s_status.current_mode) {
        case PROG_MODE_CHIP_PROGRAM:
            SWD_SetSpeed(clock_hz);
            break;
        default:
            break;
    }
    
    return PROG_OK;
}

/* ==================== DSP/FPGA专用API ==================== */

/**
 * @brief 配置FPGA
 */
Programmer_Result_t Programmer_ConfigFPGA(uint8_t* bitstream, uint32_t size)
{
    if (bitstream == NULL || size == 0) return PROG_ERROR_PARAMETER;
    
    s_status.current_mode = PROG_MODE_FPGA_CONFIG;
    
    if (FPGA_Init(&s_fpga) != HAL_OK) {
        SetError("FPGA init failed");
        return PROG_ERROR_INIT;
    }
    
    if (FPGA_Configure(&s_fpga, bitstream, size) != HAL_OK) {
        SetError("FPGA configure failed");
        return PROG_ERROR_PROGRAM;
    }
    
    return PROG_OK;
}

/**
 * @brief 配置CPLD
 */
Programmer_Result_t Programmer_ProgramCPLD(uint8_t* data, uint32_t size)
{
    if (data == NULL || size == 0) return PROG_ERROR_PARAMETER;
    
    s_status.current_mode = PROG_MODE_CPLD_PROGRAM;
    
    if (FPGA_Init(&s_fpga) != HAL_OK) {
        SetError("CPLD init failed");
        return PROG_ERROR_INIT;
    }
    
    if (FPGA_Configure(&s_fpga, data, size) != HAL_OK) {
        SetError("CPLD program failed");
        return PROG_ERROR_PROGRAM;
    }
    
    return PROG_OK;
}

/**
 * @brief DSP复位
 */
Programmer_Result_t Programmer_DSP_Reset(void)
{
    return (DSP_TMS320_Reset(&s_dsp) == HAL_OK) ? PROG_OK : PROG_ERROR_NOT_SUPPORTED;
}

/**
 * @brief DSP暂停
 */
Programmer_Result_t Programmer_DSP_Halt(void)
{
    return (DSP_TMS320_Halt(&s_dsp) == HAL_OK) ? PROG_OK : PROG_ERROR_NOT_SUPPORTED;
}

/**
 * @brief DSP运行
 */
Programmer_Result_t Programmer_DSP_Run(void)
{
    return (DSP_TMS320_Run(&s_dsp) == HAL_OK) ? PROG_OK : PROG_ERROR_NOT_SUPPORTED;
}

/**
 * @brief DSP内存读写
 */
Programmer_Result_t Programmer_DSP_MemoryAccess(uint32_t addr, uint8_t* data, uint32_t size, uint8_t write)
{
    if (data == NULL) return PROG_ERROR_PARAMETER;
    
    if (write) {
        return (DSP_WriteMem(&s_dsp, addr, data, size) == HAL_OK) ? PROG_OK : PROG_ERROR_PROGRAM;
    } else {
        return (DSP_ReadMem(&s_dsp, addr, data, size) == HAL_OK) ? PROG_OK : PROG_ERROR_READ;
    }
}

/* ==================== 调试API ==================== */

/**
 * @brief 读内存
 */
Programmer_Result_t Programmer_ReadMemory(uint32_t addr, uint8_t* data, uint32_t size)
{
    if (data == NULL) return PROG_ERROR_PARAMETER;
    SWD_ReadMem(addr, data, size);
    return PROG_OK;
}

/**
 * @brief 写内存
 */
Programmer_Result_t Programmer_WriteMemory(uint32_t addr, uint8_t* data, uint32_t size)
{
    if (data == NULL) return PROG_ERROR_PARAMETER;
    SWD_WriteMem(addr, data, size);
    return PROG_OK;
}

/**
 * @brief CPU复位
 */
Programmer_Result_t Programmer_CPU_Reset(void)
{
    SWD_ResetTarget();
    return PROG_OK;
}

/**
 * @brief CPU暂停
 */
Programmer_Result_t Programmer_CPU_Halt(void)
{
    SWD_WriteAP(0x00, 0x03);  /* Halt request */
    return PROG_OK;
}

/**
 * @brief CPU运行
 */
Programmer_Result_t Programmer_CPU_Run(void)
{
    SWD_WriteAP(0x00, 0x01);  /* Resume request */
    return PROG_OK;
}

/**
 * @brief 设置断点
 */
Programmer_Result_t Programmer_SetBreakpoint(uint32_t addr)
{
    /* 设置Flash断点(通过FPB) */
    SWD_WriteMem32(0xE0002000, addr);  /* FPB Comparator */
    SWD_WriteMem32(0xE0002004, 0x01);  /* Enable */
    return PROG_OK;
}

/**
 * @brief 清除断点
 */
Programmer_Result_t Programmer_ClearBreakpoint(uint32_t addr)
{
    SWD_WriteMem32(0xE0002000, 0x00);  /* Disable */
    return PROG_OK;
}

/**
 * @brief 单步执行
 */
Programmer_Result_t Programmer_Step(void)
{
    SWD_WriteAP(0x00, 0x07);  /* Mask interrupt + step */
    return PROG_OK;
}

/**
 * @brief 读寄存器
 */
Programmer_Result_t Programmer_ReadRegister(uint32_t reg_num, uint32_t* value)
{
    if (value == NULL) return PROG_ERROR_PARAMETER;
    /* 通过DAP读取核心寄存器 */
    SWD_WriteAP(0x04, reg_num);        /* DCRSR - 选择寄存器 */
    *value = SWD_ReadAP(0x08);         /* DCRDR - 读取值 */
    return PROG_OK;
}

/**
 * @brief 写寄存器
 */
Programmer_Result_t Programmer_WriteRegister(uint32_t reg_num, uint32_t value)
{
    SWD_WriteAP(0x08, value);           /* DCRDR - 写入值 */
    SWD_WriteAP(0x04, reg_num | 0x10000); /* DCRSR - 选择寄存器+写 */
    return PROG_OK;
}

/* ==================== 文件操作API（占位） ==================== */

Programmer_Result_t Programmer_ProgramFromFile(uint32_t addr, const char* filepath)
{
    /* 需要文件系统支持 */
    SetError("File operation not implemented yet");
    return PROG_ERROR_NOT_SUPPORTED;
}

Programmer_Result_t Programmer_ReadToFile(uint32_t addr, uint32_t size, const char* filepath)
{
    SetError("File operation not implemented yet");
    return PROG_ERROR_NOT_SUPPORTED;
}

Programmer_Result_t Programmer_VerifyWithFile(uint32_t addr, const char* filepath)
{
    SetError("File operation not implemented yet");
    return PROG_ERROR_NOT_SUPPORTED;
}