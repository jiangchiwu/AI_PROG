/**
  ******************************************************************************
  * @file    debug_interface_manager.h
  * @brief   调试接口管理器 - 统一管理所有调试接口
  * 
  *          功能描述:
  *          - 管理多种调试接口（SWD/JTAG/BDM/SBW/MON8/FINE/ICSP/ISP/UART/USB等）
  *          - 提供统一的接口创建和销毁机制
  *          - 支持动态注册新的调试接口类型
  *          - 提供统一的API封装
  * 
  *          支持的调试接口:
  *          - SWD  : Serial Wire Debug (ARM Cortex-M)
  *          - JTAG : Joint Test Action Group
  *          - BDM  : Background Debug Mode (Freescale/NXP)
  *          - SBW  : Spy-Bi-Wire (TI MSP430)
  *          - MON8 : Monitor Mode 8-bit (Freescale HC08)
  *          - FINE : Flash Interface Network for Easy Programming (Renesas)
  *          - ICSP : In-Circuit Serial Programming (Microchip PIC)
  *          - ISP  : In-System Programming (8051/LPC等)
  *          - UART : UART Bootloader接口
  *          - USB  : USB Programming接口
  * 
  * @author  AI_PROG项目
  * @date    2026-06-05
  * @version v1.0
  ******************************************************************************
  */

#ifndef __DEBUG_INTERFACE_MANAGER_H
#define __DEBUG_INTERFACE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "chip_driver_framework.h"

/* ==================== 配置宏定义 ==================== */

/** 最大支持的调试接口类型数量 */
#define DEBUG_IF_MAX_TYPES              32

/** 调试接口管理器版本 */
#define DEBUG_IF_MANAGER_VERSION        "1.0.0"

/* ==================== 错误码定义 ==================== */

/** 调试接口管理器错误码 */
typedef enum {
    DEBUG_IF_OK                   = 0x00,   /**< 操作成功 */
    DEBUG_IF_ERR                  = 0x01,   /**< 一般错误 */
    DEBUG_IF_ERR_INVALID_PARAM    = 0x02,   /**< 无效参数 */
    DEBUG_IF_ERR_NOT_FOUND        = 0x03,   /**< 接口未找到 */
    DEBUG_IF_ERR_ALREADY_EXISTS   = 0x04,   /**< 接口已存在 */
    DEBUG_IF_ERR_NO_MEMORY        = 0x05,   /**< 内存不足 */
    DEBUG_IF_ERR_NOT_INITIALIZED  = 0x06,   /**< 未初始化 */
    DEBUG_IF_ERR_INIT_FAILED      = 0x07,   /**< 初始化失败 */
    DEBUG_IF_ERR_CONNECT_FAILED   = 0x08,   /**< 连接失败 */
    DEBUG_IF_ERR_TIMEOUT          = 0x09,   /**< 超时 */
    DEBUG_IF_ERR_NOT_SUPPORTED    = 0x0A,   /**< 不支持的操作 */
    DEBUG_IF_ERR_BUSY             = 0x0B,   /**< 接口忙 */
} Debug_IF_Error_t;

/* ==================== 调试接口配置结构体 ==================== */

/**
 * @brief 调试接口通用配置结构体
 * @note  用于创建调试接口实例时传递配置参数
 */
typedef struct {
    Chip_Debug_Interface_t type;        /**< 接口类型 */
    uint32_t speed_hz;                  /**< 时钟速度 (Hz) */
    void* custom_config;                 /**< 自定义配置指针 */
} Debug_IF_Config_t;

/**
 * @brief 调试接口能力标志
 */
typedef enum {
    DEBUG_IF_CAP_MEM_READ       = (1 << 0),   /**< 支持内存读取 */
    DEBUG_IF_CAP_MEM_WRITE      = (1 << 1),   /**< 支持内存写入 */
    DEBUG_IF_CAP_REG_READ       = (1 << 2),   /**< 支持寄存器读取 */
    DEBUG_IF_CAP_REG_WRITE      = (1 << 3),   /**< 支持寄存器写入 */
    DEBUG_IF_CAP_FLASH_ERASE    = (1 << 4),   /**< 支持Flash擦除 */
    DEBUG_IF_CAP_FLASH_WRITE    = (1 << 5),   /**< 支持Flash写入 */
    DEBUG_IF_CAP_FLASH_READ     = (1 << 6),   /**< 支持Flash读取 */
    DEBUG_IF_CAP_HW_BREAKPOINT  = (1 << 7),   /**< 支持硬件断点 */
    DEBUG_IF_CAP_SW_BREAKPOINT  = (1 << 8),   /**< 支持软件断点 */
    DEBUG_IF_CAP_STEP           = (1 << 9),   /**< 支持单步执行 */
    DEBUG_IF_CAP_RUN            = (1 << 10),  /**< 支持运行控制 */
    DEBUG_IF_CAP_HALT           = (1 << 11),  /**< 支持暂停 */
    DEBUG_IF_CAP_RESET          = (1 << 12),  /**< 支持复位 */
    DEBUG_IF_CAP_SPEED_CHANGE   = (1 << 13),  /**< 支持速度调整 */
    DEBUG_IF_CAP_MULTI_TARGET   = (1 << 14),  /**< 支持多目标 */
} Debug_IF_Capability_t;

/* ==================== 调试接口操作函数结构体 ==================== */

/**
 * @brief 调试接口操作函数结构体
 * @note  每种调试接口需要实现这些函数
 */
typedef struct {
    /** 接口名称 */
    const char* name;
    
    /** 接口类型 */
    Chip_Debug_Interface_t type;
    
    /** 接口能力标志 */
    uint32_t capabilities;
    
    /**
     * @brief 初始化调试接口
     * @param config 配置参数
     * @return 成功返回true，失败返回false
     */
    bool (*Init)(const Debug_IF_Config_t* config);
    
    /**
     * @brief 关闭调试接口
     * @return 成功返回true，失败返回false
     */
    bool (*Close)(void);
    
    /**
     * @brief 连接目标设备
     * @return 成功返回true，失败返回false
     */
    bool (*Connect)(void);
    
    /**
     * @brief 断开目标设备
     * @return 成功返回true，失败返回false
     */
    bool (*Disconnect)(void);
    
    /**
     * @brief 读取目标设备ID
     * @param id 输出ID值
     * @return 成功返回true，失败返回false
     */
    bool (*ReadID)(uint32_t* id);
    
    /**
     * @brief 获取接口能力
     * @param caps 输出能力标志
     * @return 成功返回true，失败返回false
     */
    bool (*GetCapabilities)(uint32_t* caps);
    
    /**
     * @brief 写入目标内存
     * @param addr 目标地址
     * @param data 数据缓冲区
     * @param size 数据大小
     * @return 成功返回true，失败返回false
     */
    bool (*MemWrite)(uint32_t addr, const uint8_t* data, uint32_t size);
    
    /**
     * @brief 读取目标内存
     * @param addr 目标地址
     * @param data 数据缓冲区
     * @param size 数据大小
     * @return 成功返回true，失败返回false
     */
    bool (*MemRead)(uint32_t addr, uint8_t* data, uint32_t size);
    
    /**
     * @brief 写入目标寄存器
     * @param addr 寄存器地址
     * @param value 寄存器值
     * @return 成功返回true，失败返回false
     */
    bool (*RegWrite)(uint32_t addr, uint32_t value);
    
    /**
     * @brief 读取目标寄存器
     * @param addr 寄存器地址
     * @param value 输出寄存器值
     * @return 成功返回true，失败返回false
     */
    bool (*RegRead)(uint32_t addr, uint32_t* value);
    
    /**
     * @brief 复位目标设备
     * @return 成功返回true，失败返回false
     */
    bool (*Reset)(void);
    
    /**
     * @brief 暂停目标设备
     * @return 成功返回true，失败返回false
     */
    bool (*Halt)(void);
    
    /**
     * @brief 运行目标设备
     * @return 成功返回true，失败返回false
     */
    bool (*Run)(void);
    
    /**
     * @brief 单步执行
     * @return 成功返回true，失败返回false
     */
    bool (*Step)(void);
    
    /**
     * @brief 设置通信速度
     * @param speed_hz 速度 (Hz)
     * @return 成功返回true，失败返回false
     */
    bool (*SetSpeed)(uint32_t speed_hz);
    
    /**
     * @brief 获取通信速度
     * @return 当前速度 (Hz)
     */
    uint32_t (*GetSpeed)(void);
    
} Debug_IF_Ops_t;

/* ==================== 调试接口实例结构体 ==================== */

/**
 * @brief 调试接口实例结构体
 * @note  表示一个具体的调试接口实例
 */
typedef struct {
    Chip_Debug_Interface_t type;        /**< 接口类型 */
    const Debug_IF_Ops_t* ops;          /**< 操作函数指针 */
    void* private_data;                  /**< 私有数据 */
    uint32_t speed_hz;                  /**< 当前速度 */
    bool is_connected;                   /**< 是否已连接 */
    bool is_initialized;                 /**< 是否已初始化 */
} Debug_IF_Instance_t;

/* ==================== 调试接口注册表结构体 ==================== */

/**
 * @brief 调试接口注册表条目
 */
typedef struct {
    Chip_Debug_Interface_t type;        /**< 接口类型 */
    const char* name;                    /**< 接口名称 */
    const Debug_IF_Ops_t* ops;          /**< 操作函数 */
    bool is_registered;                  /**< 是否已注册 */
} Debug_IF_Registry_Entry_t;

/* ==================== 调试接口管理器结构体 ==================== */

/**
 * @brief 调试接口管理器结构体
 * @note  全局唯一实例，管理所有调试接口
 */
typedef struct {
    bool initialized;                                    /**< 管理器是否已初始化 */
    uint32_t registered_count;                           /**< 已注册接口数量 */
    Debug_IF_Registry_Entry_t registry[DEBUG_IF_MAX_TYPES]; /**< 接口注册表 */
    Debug_IF_Instance_t* active_instance;                /**< 当前活动实例 */
} Debug_IF_Manager_t;

/* ==================== 外部变量声明 ==================== */

/** 全局调试接口管理器实例 */
extern Debug_IF_Manager_t g_debug_if_manager;

/* ==================== 公共API函数声明 ==================== */

/**
 * @brief 初始化调试接口管理器
 * @return 成功返回true，失败返回false
 * @note  必须在使用其他函数之前调用
 */
bool Debug_IF_Manager_Init(void);

/**
 * @brief 关闭调试接口管理器
 * @return 成功返回true，失败返回false
 */
bool Debug_IF_Manager_Close(void);

/**
 * @brief 注册调试接口类型
 * @param type 接口类型
 * @param ops 接口操作函数
 * @return 成功返回true，失败返回false
 * @note  用于扩展新的调试接口类型
 */
bool Debug_IF_Register(Chip_Debug_Interface_t type, const Debug_IF_Ops_t* ops);

/**
 * @brief 注销调试接口类型
 * @param type 接口类型
 * @return 成功返回true，失败返回false
 */
bool Debug_IF_Unregister(Chip_Debug_Interface_t type);

/**
 * @brief 获取已注册的调试接口操作函数
 * @param type 接口类型
 * @return 操作函数指针，未找到返回NULL
 */
const Debug_IF_Ops_t* Debug_IF_Get(Chip_Debug_Interface_t type);

/**
 * @brief 创建调试接口实例
 * @param config 接口配置
 * @return 实例指针，失败返回NULL
 * @note  创建后需要调用实例的Init函数进行初始化
 */
Debug_IF_Instance_t* Debug_IF_Create(const Debug_IF_Config_t* config);

/**
 * @brief 销毁调试接口实例
 * @param instance 实例指针
 * @return 成功返回true，失败返回false
 */
bool Debug_IF_Destroy(Debug_IF_Instance_t* instance);

/**
 * @brief 获取接口类型名称
 * @param type 接口类型
 * @return 接口名称字符串
 */
const char* Debug_IF_GetTypeName(Chip_Debug_Interface_t type);

/**
 * @brief 检查接口类型是否已注册
 * @param type 接口类型
 * @return 已注册返回true，未注册返回false
 */
bool Debug_IF_IsRegistered(Chip_Debug_Interface_t type);

/**
 * @brief 获取已注册接口数量
 * @return 已注册接口数量
 */
uint32_t Debug_IF_GetRegisteredCount(void);

/**
 * @brief 获取所有已注册接口类型列表
 * @param types 输出类型数组
 * @param max_count 数组最大容量
 * @return 实际接口数量
 */
uint32_t Debug_IF_GetRegisteredTypes(Chip_Debug_Interface_t* types, uint32_t max_count);

/**
 * @brief 打印已注册接口列表（调试用）
 */
void Debug_IF_PrintRegisteredList(void);

/* ==================== 便捷API函数声明 ==================== */

/**
 * @brief 快速创建并初始化调试接口
 * @param type 接口类型
 * @param speed_hz 通信速度
 * @return 实例指针，失败返回NULL
 */
Debug_IF_Instance_t* Debug_IF_QuickCreate(Chip_Debug_Interface_t type, uint32_t speed_hz);

/**
 * @brief 快速销毁调试接口
 * @param instance 实例指针
 * @return 成功返回true，失败返回false
 */
bool Debug_IF_QuickDestroy(Debug_IF_Instance_t* instance);

/* ==================== 实例操作便捷宏定义 ==================== */

/**
 * @brief 初始化调试接口实例
 */
#define DEBUG_IF_INIT(inst, cfg)        ((inst)->ops->Init(cfg))

/**
 * @brief 关闭调试接口实例
 */
#define DEBUG_IF_CLOSE(inst)            ((inst)->ops->Close())

/**
 * @brief 连接目标设备
 */
#define DEBUG_IF_CONNECT(inst)          ((inst)->ops->Connect())

/**
 * @brief 断开目标设备
 */
#define DEBUG_IF_DISCONNECT(inst)       ((inst)->ops->Disconnect())

/**
 * @brief 读取目标ID
 */
#define DEBUG_IF_READ_ID(inst, id)       ((inst)->ops->ReadID(id))

/**
 * @brief 写入内存
 */
#define DEBUG_IF_MEM_WRITE(inst, addr, data, size) \
    ((inst)->ops->MemWrite(addr, data, size))

/**
 * @brief 读取内存
 */
#define DEBUG_IF_MEM_READ(inst, addr, data, size) \
    ((inst)->ops->MemRead(addr, data, size))

/**
 * @brief 写入寄存器
 */
#define DEBUG_IF_REG_WRITE(inst, addr, val) \
    ((inst)->ops->RegWrite(addr, val))

/**
 * @brief 读取寄存器
 */
#define DEBUG_IF_REG_READ(inst, addr, val) \
    ((inst)->ops->RegRead(addr, val))

/**
 * @brief 复位目标
 */
#define DEBUG_IF_RESET(inst)            ((inst)->ops->Reset())

/**
 * @brief 暂停目标
 */
#define DEBUG_IF_HALT(inst)             ((inst)->ops->Halt())

/**
 * @brief 运行目标
 */
#define DEBUG_IF_RUN(inst)              ((inst)->ops->Run())

/**
 * @brief 单步执行
 */
#define DEBUG_IF_STEP(inst)             ((inst)->ops->Step())

/**
 * @brief 设置速度
 */
#define DEBUG_IF_SET_SPEED(inst, speed) ((inst)->ops->SetSpeed(speed))

/**
 * @brief 获取速度
 */
#define DEBUG_IF_GET_SPEED(inst)        ((inst)->ops->GetSpeed())

#ifdef __cplusplus
}
#endif

#endif /* __DEBUG_INTERFACE_MANAGER_H */