/**
 ******************************************************************************
 * @file    programmer_api.h
 * @brief   统一编程器API接口头文件
 *          提供上层应用调用的统一编程接口
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#ifndef __PROGRAMMER_API_H__
#define __PROGRAMMER_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "chip_driver_framework.h"

/* ==================== 编程器模式定义 ==================== */
typedef enum {
    PROG_MODE_CHIP_PROGRAM = 0,    /* 芯片编程模式 */
    PROG_MODE_SPI_FLASH,           /* SPI Flash编程模式 */
    PROG_MODE_EEPROM,              /* EEPROM编程模式 */
    PROG_MODE_DSP_PROGRAM,         /* DSP编程模式 */
    PROG_MODE_FPGA_CONFIG,         /* FPGA配置模式 */
    PROG_MODE_CPLD_PROGRAM,        /* CPLD编程模式 */
    PROG_MODE_LOGIC_ANALYZER,      /* 逻辑分析仪模式 */
    PROG_MODE_OSCILLOSCOPE,        /* 示波器模式 */
    PROG_MODE_JTAG_DEBUG,          /* JTAG调试模式 */
    PROG_MODE_SWD_DEBUG,           /* SWD调试模式 */
} Programmer_Mode_t;

/* ==================== 操作结果定义 ==================== */
typedef enum {
    PROG_OK = 0,
    PROG_ERROR_INIT,
    PROG_ERROR_CONNECT,
    PROG_ERROR_DETECT,
    PROG_ERROR_ERASE,
    PROG_ERROR_PROGRAM,
    PROG_ERROR_VERIFY,
    PROG_ERROR_READ,
    PROG_ERROR_TIMEOUT,
    PROG_ERROR_NOT_SUPPORTED,
    PROG_ERROR_PARAMETER,
} Programmer_Result_t;

/* ==================== 编程进度回调函数 ==================== */
typedef void (*Prog_Progress_Callback_t)(uint32_t current, uint32_t total, const char* operation);

/* ==================== 编程器配置结构体 ==================== */
typedef struct {
    Programmer_Mode_t       mode;               /* 编程器模式 */
    uint32_t                clock_hz;           /* 通信时钟 */
    uint32_t                target_chip_id;     /* 目标芯片ID */
    uint32_t                flash_base_addr;    /* Flash基地址 */
    uint32_t                flash_size;         /* Flash大小 */
    uint32_t                sector_size;        /* 扇区大小 */
    uint32_t                page_size;          /* 页大小 */
    uint8_t                 verify_enable;      /* 验证使能 */
    uint8_t                 erase_enable;       /* 擦除使能 */
    uint8_t                 lock_enable;        /* 加锁使能 */
    Prog_Progress_Callback_t progress_cb;       /* 进度回调 */
} Programmer_Config_t;

/* ==================== 编程器状态结构体 ==================== */
typedef struct {
    Programmer_Mode_t       current_mode;       /* 当前模式 */
    uint32_t                connected_chip;     /* 已连接芯片ID */
    const Chip_Info_t*      chip_info;          /* 芯片信息 */
    uint32_t                operation_status;   /* 操作状态 */
    uint32_t                progress_current;   /* 当前进度 */
    uint32_t                progress_total;     /* 总进度 */
    char                    last_error[64];     /* 最后错误信息 */
} Programmer_Status_t;

/* ==================== 核心API函数 ==================== */

/**
 * @brief 初始化编程器
 * @param config: 编程器配置
 * @return 操作结果
 */
Programmer_Result_t Programmer_Init(Programmer_Config_t* config);

/**
 * @brief 反初始化编程器
 * @return 操作结果
 */
Programmer_Result_t Programmer_DeInit(void);

/**
 * @brief 设置编程器模式
 * @param mode: 目标模式
 * @return 操作结果
 */
Programmer_Result_t Programmer_SetMode(Programmer_Mode_t mode);

/**
 * @brief 连接目标芯片
 * @return 操作结果
 */
Programmer_Result_t Programmer_Connect(void);

/**
 * @brief 断开目标芯片
 * @return 操作结果
 */
Programmer_Result_t Programmer_Disconnect(void);

/**
 * @brief 自动检测目标芯片
 * @param detected_chip: 输出检测到的芯片ID
 * @return 操作结果
 */
Programmer_Result_t Programmer_AutoDetect(uint32_t* detected_chip);

/**
 * @brief 获取芯片信息
 * @param chip_info: 输出芯片信息
 * @return 操作结果
 */
Programmer_Result_t Programmer_GetChipInfo(const Chip_Info_t** chip_info);

/**
 * @brief 擦除Flash
 * @param addr: 起始地址
 * @param size: 擦除大小
 * @return 操作结果
 */
Programmer_Result_t Programmer_EraseFlash(uint32_t addr, uint32_t size);

/**
 * @brief 编程Flash
 * @param addr: 起始地址
 * @param data: 数据指针
 * @param size: 数据大小
 * @return 操作结果
 */
Programmer_Result_t Programmer_ProgramFlash(uint32_t addr, uint8_t* data, uint32_t size);

/**
 * @brief 读Flash
 * @param addr: 起始地址
 * @param data: 数据缓冲区
 * @param size: 读取大小
 * @return 操作结果
 */
Programmer_Result_t Programmer_ReadFlash(uint32_t addr, uint8_t* data, uint32_t size);

/**
 * @brief 验证Flash
 * @param addr: 起始地址
 * @param data: 验证数据
 * @param size: 验证大小
 * @return 操作结果
 */
Programmer_Result_t Programmer_VerifyFlash(uint32_t addr, uint8_t* data, uint32_t size);

/**
 * @brief 全片擦除
 * @return 操作结果
 */
Programmer_Result_t Programmer_ChipErase(void);

/**
 * @brief 空片检查
 * @param addr: 起始地址
 * @param size: 检查大小
 * @return 操作结果
 */
Programmer_Result_t Programmer_BlankCheck(uint32_t addr, uint32_t size);

/**
 * @brief 解锁芯片
 * @return 操作结果
 */
Programmer_Result_t Programmer_Unlock(void);

/**
 * @brief 加锁芯片
 * @return 操作结果
 */
Programmer_Result_t Programmer_Lock(void);

/**
 * @brief 获取编程器状态
 * @param status: 输出状态
 * @return 操作结果
 */
Programmer_Result_t Programmer_GetStatus(Programmer_Status_t* status);

/**
 * @brief 设置通信速度
 * @param clock_hz: 目标时钟频率
 * @return 操作结果
 */
Programmer_Result_t Programmer_SetSpeed(uint32_t clock_hz);

/* ==================== 文件操作API ==================== */

/**
 * @brief 从文件编程Flash
 * @param addr: 起始地址
 * @param filepath: 文件路径
 * @return 操作结果
 */
Programmer_Result_t Programmer_ProgramFromFile(uint32_t addr, const char* filepath);

/**
 * @brief 读Flash到文件
 * @param addr: 起始地址
 * @param size: 读取大小
 * @param filepath: 文件路径
 * @return 操作结果
 */
Programmer_Result_t Programmer_ReadToFile(uint32_t addr, uint32_t size, const char* filepath);

/**
 * @brief 验证Flash与文件
 * @param addr: 起始地址
 * @param filepath: 文件路径
 * @return 操作结果
 */
Programmer_Result_t Programmer_VerifyWithFile(uint32_t addr, const char* filepath);

/* ==================== DSP/FPGA专用API ==================== */

/**
 * @brief 配置FPGA
 * @param bitstream: 配置数据
 * @param size: 数据大小
 * @return 操作结果
 */
Programmer_Result_t Programmer_ConfigFPGA(uint8_t* bitstream, uint32_t size);

/**
 * @brief 配置CPLD
 * @param data: 配置数据
 * @param size: 数据大小
 * @return 操作结果
 */
Programmer_Result_t Programmer_ProgramCPLD(uint8_t* data, uint32_t size);

/**
 * @brief DSP复位
 * @return 操作结果
 */
Programmer_Result_t Programmer_DSP_Reset(void);

/**
 * @brief DSP暂停
 * @return 操作结果
 */
Programmer_Result_t Programmer_DSP_Halt(void);

/**
 * @brief DSP运行
 * @return 操作结果
 */
Programmer_Result_t Programmer_DSP_Run(void);

/**
 * @brief DSP内存读写
 * @param addr: 地址
 * @param data: 数据
 * @param size: 大小
 * @param write: 1=写, 0=读
 * @return 操作结果
 */
Programmer_Result_t Programmer_DSP_MemoryAccess(uint32_t addr, uint8_t* data, uint32_t size, uint8_t write);

/* ==================== 调试API ==================== */

/**
 * @brief 读内存
 * @param addr: 地址
 * @param data: 数据缓冲区
 * @param size: 大小
 * @return 操作结果
 */
Programmer_Result_t Programmer_ReadMemory(uint32_t addr, uint8_t* data, uint32_t size);

/**
 * @brief 写内存
 * @param addr: 地址
 * @param data: 数据
 * @param size: 大小
 * @return 操作结果
 */
Programmer_Result_t Programmer_WriteMemory(uint32_t addr, uint8_t* data, uint32_t size);

/**
 * @brief CPU复位
 * @return 操作结果
 */
Programmer_Result_t Programmer_CPU_Reset(void);

/**
 * @brief CPU暂停
 * @return 操作结果
 */
Programmer_Result_t Programmer_CPU_Halt(void);

/**
 * @brief CPU运行
 * @return 操作结果
 */
Programmer_Result_t Programmer_CPU_Run(void);

/**
 * @brief 设置断点
 * @param addr: 断点地址
 * @return 操作结果
 */
Programmer_Result_t Programmer_SetBreakpoint(uint32_t addr);

/**
 * @brief 清除断点
 * @param addr: 断点地址
 * @return 操作结果
 */
Programmer_Result_t Programmer_ClearBreakpoint(uint32_t addr);

/**
 * @brief 单步执行
 * @return 操作结果
 */
Programmer_Result_t Programmer_Step(void);

/**
 * @brief 读寄存器
 * @param reg_num: 寄存器号
 * @param value: 输出值
 * @return 操作结果
 */
Programmer_Result_t Programmer_ReadRegister(uint32_t reg_num, uint32_t* value);

/**
 * @brief 写寄存器
 * @param reg_num: 寄存器号
 * @param value: 写入值
 * @return 操作结果
 */
Programmer_Result_t Programmer_WriteRegister(uint32_t reg_num, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif /* __PROGRAMMER_API_H__ */