/**
  ******************************************************************************
  * @file    debug_interface_manager.c
  * @brief   调试接口管理器 - 实现
  * 
  *          实现功能:
  *          - 调试接口注册表管理（最多32种接口类型）
  *          - 接口创建工厂函数
  *          - 接口统一API封装
  *          - 支持SWD/JTAG/BDM/SBW/MON8/FINE/ICSP/ISP/UART/USB等接口
  * 
  * @author  AI_PROG项目
  * @date    2026-06-05
  * @version v1.0
  ******************************************************************************
  */

#include "debug_interface_manager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ==================== 已有接口头文件 ==================== */
#include "swd.h"    /* SWD接口 */
#include "bdm.h"    /* BDM接口 */
#include "sbw.h"    /* SBW接口 */
#include "mon8.h"   /* MON8接口 */
#include "fine.h"   /* FINE接口 */

/* ==================== 全局变量 ==================== */

/** 调试接口管理器全局实例 */
Debug_IF_Manager_t g_debug_if_manager = {
    .initialized = false,
    .registered_count = 0,
    .active_instance = NULL
};

/* ==================== 内部函数声明 ==================== */

/* 已有接口的操作函数实现 */
static bool SWD_IF_Init(const Debug_IF_Config_t* config);
static bool SWD_IF_Close(void);
static bool SWD_IF_Connect(void);
static bool SWD_IF_Disconnect(void);
static bool SWD_IF_ReadID(uint32_t* id);
static bool SWD_IF_GetCapabilities(uint32_t* caps);
static bool SWD_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size);
static bool SWD_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size);
static bool SWD_IF_RegWrite(uint32_t addr, uint32_t value);
static bool SWD_IF_RegRead(uint32_t addr, uint32_t* value);
static bool SWD_IF_Reset(void);
static bool SWD_IF_Halt(void);
static bool SWD_IF_Run(void);
static bool SWD_IF_Step(void);
static bool SWD_IF_SetSpeed(uint32_t speed_hz);
static uint32_t SWD_IF_GetSpeed(void);

static bool JTAG_IF_Init(const Debug_IF_Config_t* config);
static bool JTAG_IF_Close(void);
static bool JTAG_IF_Connect(void);
static bool JTAG_IF_Disconnect(void);
static bool JTAG_IF_ReadID(uint32_t* id);
static bool JTAG_IF_GetCapabilities(uint32_t* caps);
static bool JTAG_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size);
static bool JTAG_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size);
static bool JTAG_IF_RegWrite(uint32_t addr, uint32_t value);
static bool JTAG_IF_RegRead(uint32_t addr, uint32_t* value);
static bool JTAG_IF_Reset(void);
static bool JTAG_IF_Halt(void);
static bool JTAG_IF_Run(void);
static bool JTAG_IF_Step(void);
static bool JTAG_IF_SetSpeed(uint32_t speed_hz);
static uint32_t JTAG_IF_GetSpeed(void);

static bool BDM_IF_Init(const Debug_IF_Config_t* config);
static bool BDM_IF_Close(void);
static bool BDM_IF_Connect(void);
static bool BDM_IF_Disconnect(void);
static bool BDM_IF_ReadID(uint32_t* id);
static bool BDM_IF_GetCapabilities(uint32_t* caps);
static bool BDM_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size);
static bool BDM_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size);
static bool BDM_IF_RegWrite(uint32_t addr, uint32_t value);
static bool BDM_IF_RegRead(uint32_t addr, uint32_t* value);
static bool BDM_IF_Reset(void);
static bool BDM_IF_Halt(void);
static bool BDM_IF_Run(void);
static bool BDM_IF_Step(void);
static bool BDM_IF_SetSpeed(uint32_t speed_hz);
static uint32_t BDM_IF_GetSpeed(void);

static bool SBW_IF_Init(const Debug_IF_Config_t* config);
static bool SBW_IF_Close(void);
static bool SBW_IF_Connect(void);
static bool SBW_IF_Disconnect(void);
static bool SBW_IF_ReadID(uint32_t* id);
static bool SBW_IF_GetCapabilities(uint32_t* caps);
static bool SBW_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size);
static bool SBW_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size);
static bool SBW_IF_RegWrite(uint32_t addr, uint32_t value);
static bool SBW_IF_RegRead(uint32_t addr, uint32_t* value);
static bool SBW_IF_Reset(void);
static bool SBW_IF_Halt(void);
static bool SBW_IF_Run(void);
static bool SBW_IF_Step(void);
static bool SBW_IF_SetSpeed(uint32_t speed_hz);
static uint32_t SBW_IF_GetSpeed(void);

static bool MON8_IF_Init(const Debug_IF_Config_t* config);
static bool MON8_IF_Close(void);
static bool MON8_IF_Connect(void);
static bool MON8_IF_Disconnect(void);
static bool MON8_IF_ReadID(uint32_t* id);
static bool MON8_IF_GetCapabilities(uint32_t* caps);
static bool MON8_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size);
static bool MON8_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size);
static bool MON8_IF_RegWrite(uint32_t addr, uint32_t value);
static bool MON8_IF_RegRead(uint32_t addr, uint32_t* value);
static bool MON8_IF_Reset(void);
static bool MON8_IF_Halt(void);
static bool MON8_IF_Run(void);
static bool MON8_IF_Step(void);
static bool MON8_IF_SetSpeed(uint32_t speed_hz);
static uint32_t MON8_IF_GetSpeed(void);

static bool FINE_IF_Init(const Debug_IF_Config_t* config);
static bool FINE_IF_Close(void);
static bool FINE_IF_Connect(void);
static bool FINE_IF_Disconnect(void);
static bool FINE_IF_ReadID(uint32_t* id);
static bool FINE_IF_GetCapabilities(uint32_t* caps);
static bool FINE_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size);
static bool FINE_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size);
static bool FINE_IF_RegWrite(uint32_t addr, uint32_t value);
static bool FINE_IF_RegRead(uint32_t addr, uint32_t* value);
static bool FINE_IF_Reset(void);
static bool FINE_IF_Halt(void);
static bool FINE_IF_Run(void);
static bool FINE_IF_Step(void);
static bool FINE_IF_SetSpeed(uint32_t speed_hz);
static uint32_t FINE_IF_GetSpeed(void);

/* ==================== 新增接口的占位符实现 ==================== */
/* ICSP接口 - In-Circuit Serial Programming (Microchip PIC) */
/* TODO: 实现ICSP接口功能 */

static bool ICSP_IF_Init(const Debug_IF_Config_t* config) { return false; }
static bool ICSP_IF_Close(void) { return false; }
static bool ICSP_IF_Connect(void) { return false; }
static bool ICSP_IF_Disconnect(void) { return false; }
static bool ICSP_IF_ReadID(uint32_t* id) { if (id) *id = 0; return false; }
static bool ICSP_IF_GetCapabilities(uint32_t* caps) { if (caps) *caps = 0; return false; }
static bool ICSP_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size) { return false; }
static bool ICSP_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size) { return false; }
static bool ICSP_IF_RegWrite(uint32_t addr, uint32_t value) { return false; }
static bool ICSP_IF_RegRead(uint32_t addr, uint32_t* value) { return false; }
static bool ICSP_IF_Reset(void) { return false; }
static bool ICSP_IF_Halt(void) { return false; }
static bool ICSP_IF_Run(void) { return false; }
static bool ICSP_IF_Step(void) { return false; }
static bool ICSP_IF_SetSpeed(uint32_t speed_hz) { return false; }
static uint32_t ICSP_IF_GetSpeed(void) { return 0; }

/* ISP接口 - In-System Programming (8051/LPC等) */
/* TODO: 实现ISP接口功能 */

static bool ISP_IF_Init(const Debug_IF_Config_t* config) { return false; }
static bool ISP_IF_Close(void) { return false; }
static bool ISP_IF_Connect(void) { return false; }
static bool ISP_IF_Disconnect(void) { return false; }
static bool ISP_IF_ReadID(uint32_t* id) { if (id) *id = 0; return false; }
static bool ISP_IF_GetCapabilities(uint32_t* caps) { if (caps) *caps = 0; return false; }
static bool ISP_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size) { return false; }
static bool ISP_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size) { return false; }
static bool ISP_IF_RegWrite(uint32_t addr, uint32_t value) { return false; }
static bool ISP_IF_RegRead(uint32_t addr, uint32_t* value) { return false; }
static bool ISP_IF_Reset(void) { return false; }
static bool ISP_IF_Halt(void) { return false; }
static bool ISP_IF_Run(void) { return false; }
static bool ISP_IF_Step(void) { return false; }
static bool ISP_IF_SetSpeed(uint32_t speed_hz) { return false; }
static uint32_t ISP_IF_GetSpeed(void) { return 0; }

/* UART接口 - UART Bootloader接口 */
/* TODO: 实现UART接口功能 */

static bool UART_IF_Init(const Debug_IF_Config_t* config) { return false; }
static bool UART_IF_Close(void) { return false; }
static bool UART_IF_Connect(void) { return false; }
static bool UART_IF_Disconnect(void) { return false; }
static bool UART_IF_ReadID(uint32_t* id) { if (id) *id = 0; return false; }
static bool UART_IF_GetCapabilities(uint32_t* caps) { if (caps) *caps = 0; return false; }
static bool UART_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size) { return false; }
static bool UART_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size) { return false; }
static bool UART_IF_RegWrite(uint32_t addr, uint32_t value) { return false; }
static bool UART_IF_RegRead(uint32_t addr, uint32_t* value) { return false; }
static bool UART_IF_Reset(void) { return false; }
static bool UART_IF_Halt(void) { return false; }
static bool UART_IF_Run(void) { return false; }
static bool UART_IF_Step(void) { return false; }
static bool UART_IF_SetSpeed(uint32_t speed_hz) { return false; }
static uint32_t UART_IF_GetSpeed(void) { return 0; }

/* USB接口 - USB Programming接口 */
/* TODO: 实现USB接口功能 */

static bool USB_IF_Init(const Debug_IF_Config_t* config) { return false; }
static bool USB_IF_Close(void) { return false; }
static bool USB_IF_Connect(void) { return false; }
static bool USB_IF_Disconnect(void) { return false; }
static bool USB_IF_ReadID(uint32_t* id) { if (id) *id = 0; return false; }
static bool USB_IF_GetCapabilities(uint32_t* caps) { if (caps) *caps = 0; return false; }
static bool USB_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size) { return false; }
static bool USB_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size) { return false; }
static bool USB_IF_RegWrite(uint32_t addr, uint32_t value) { return false; }
static bool USB_IF_RegRead(uint32_t addr, uint32_t* value) { return false; }
static bool USB_IF_Reset(void) { return false; }
static bool USB_IF_Halt(void) { return false; }
static bool USB_IF_Run(void) { return false; }
static bool USB_IF_Step(void) { return false; }
static bool USB_IF_SetSpeed(uint32_t speed_hz) { return false; }
static uint32_t USB_IF_GetSpeed(void) { return 0; }

/* ==================== 接口操作函数表定义 ==================== */

/** SWD接口操作函数表 */
static const Debug_IF_Ops_t SWD_IF_Ops = {
    .name = "SWD",
    .type = DEBUG_IF_SWD,
    .capabilities = DEBUG_IF_CAP_MEM_READ | DEBUG_IF_CAP_MEM_WRITE |
                    DEBUG_IF_CAP_REG_READ | DEBUG_IF_CAP_REG_WRITE |
                    DEBUG_IF_CAP_HW_BREAKPOINT | DEBUG_IF_CAP_STEP |
                    DEBUG_IF_CAP_RUN | DEBUG_IF_CAP_HALT | DEBUG_IF_CAP_RESET |
                    DEBUG_IF_CAP_SPEED_CHANGE,
    .Init = SWD_IF_Init,
    .Close = SWD_IF_Close,
    .Connect = SWD_IF_Connect,
    .Disconnect = SWD_IF_Disconnect,
    .ReadID = SWD_IF_ReadID,
    .GetCapabilities = SWD_IF_GetCapabilities,
    .MemWrite = SWD_IF_MemWrite,
    .MemRead = SWD_IF_MemRead,
    .RegWrite = SWD_IF_RegWrite,
    .RegRead = SWD_IF_RegRead,
    .Reset = SWD_IF_Reset,
    .Halt = SWD_IF_Halt,
    .Run = SWD_IF_Run,
    .Step = SWD_IF_Step,
    .SetSpeed = SWD_IF_SetSpeed,
    .GetSpeed = SWD_IF_GetSpeed,
};

/** JTAG接口操作函数表 */
static const Debug_IF_Ops_t JTAG_IF_Ops = {
    .name = "JTAG",
    .type = DEBUG_IF_JTAG,
    .capabilities = DEBUG_IF_CAP_MEM_READ | DEBUG_IF_CAP_MEM_WRITE |
                    DEBUG_IF_CAP_REG_READ | DEBUG_IF_CAP_REG_WRITE |
                    DEBUG_IF_CAP_HW_BREAKPOINT | DEBUG_IF_CAP_STEP |
                    DEBUG_IF_CAP_RUN | DEBUG_IF_CAP_HALT | DEBUG_IF_CAP_RESET |
                    DEBUG_IF_CAP_SPEED_CHANGE,
    .Init = JTAG_IF_Init,
    .Close = JTAG_IF_Close,
    .Connect = JTAG_IF_Connect,
    .Disconnect = JTAG_IF_Disconnect,
    .ReadID = JTAG_IF_ReadID,
    .GetCapabilities = JTAG_IF_GetCapabilities,
    .MemWrite = JTAG_IF_MemWrite,
    .MemRead = JTAG_IF_MemRead,
    .RegWrite = JTAG_IF_RegWrite,
    .RegRead = JTAG_IF_RegRead,
    .Reset = JTAG_IF_Reset,
    .Halt = JTAG_IF_Halt,
    .Run = JTAG_IF_Run,
    .Step = JTAG_IF_Step,
    .SetSpeed = JTAG_IF_SetSpeed,
    .GetSpeed = JTAG_IF_GetSpeed,
};

/** BDM接口操作函数表 */
static const Debug_IF_Ops_t BDM_IF_Ops = {
    .name = "BDM",
    .type = DEBUG_IF_BDM,
    .capabilities = DEBUG_IF_CAP_MEM_READ | DEBUG_IF_CAP_MEM_WRITE |
                    DEBUG_IF_CAP_REG_READ | DEBUG_IF_CAP_REG_WRITE |
                    DEBUG_IF_CAP_RUN | DEBUG_IF_CAP_HALT | DEBUG_IF_CAP_RESET |
                    DEBUG_IF_CAP_SPEED_CHANGE,
    .Init = BDM_IF_Init,
    .Close = BDM_IF_Close,
    .Connect = BDM_IF_Connect,
    .Disconnect = BDM_IF_Disconnect,
    .ReadID = BDM_IF_ReadID,
    .GetCapabilities = BDM_IF_GetCapabilities,
    .MemWrite = BDM_IF_MemWrite,
    .MemRead = BDM_IF_MemRead,
    .RegWrite = BDM_IF_RegWrite,
    .RegRead = BDM_IF_RegRead,
    .Reset = BDM_IF_Reset,
    .Halt = BDM_IF_Halt,
    .Run = BDM_IF_Run,
    .Step = BDM_IF_Step,
    .SetSpeed = BDM_IF_SetSpeed,
    .GetSpeed = BDM_IF_GetSpeed,
};

/** SBW接口操作函数表 */
static const Debug_IF_Ops_t SBW_IF_Ops = {
    .name = "SBW",
    .type = DEBUG_IF_SBW,
    .capabilities = DEBUG_IF_CAP_MEM_READ | DEBUG_IF_CAP_MEM_WRITE |
                    DEBUG_IF_CAP_REG_READ | DEBUG_IF_CAP_REG_WRITE |
                    DEBUG_IF_CAP_FLASH_ERASE | DEBUG_IF_CAP_FLASH_WRITE |
                    DEBUG_IF_CAP_RUN | DEBUG_IF_CAP_HALT | DEBUG_IF_CAP_RESET |
                    DEBUG_IF_CAP_SPEED_CHANGE,
    .Init = SBW_IF_Init,
    .Close = SBW_IF_Close,
    .Connect = SBW_IF_Connect,
    .Disconnect = SBW_IF_Disconnect,
    .ReadID = SBW_IF_ReadID,
    .GetCapabilities = SBW_IF_GetCapabilities,
    .MemWrite = SBW_IF_MemWrite,
    .MemRead = SBW_IF_MemRead,
    .RegWrite = SBW_IF_RegWrite,
    .RegRead = SBW_IF_RegRead,
    .Reset = SBW_IF_Reset,
    .Halt = SBW_IF_Halt,
    .Run = SBW_IF_Run,
    .Step = SBW_IF_Step,
    .SetSpeed = SBW_IF_SetSpeed,
    .GetSpeed = SBW_IF_GetSpeed,
};

/** MON8接口操作函数表 */
static const Debug_IF_Ops_t MON8_IF_Ops = {
    .name = "MON8",
    .type = DEBUG_IF_MON8,
    .capabilities = DEBUG_IF_CAP_MEM_READ | DEBUG_IF_CAP_MEM_WRITE |
                    DEBUG_IF_CAP_FLASH_ERASE | DEBUG_IF_CAP_FLASH_WRITE |
                    DEBUG_IF_CAP_RUN | DEBUG_IF_CAP_RESET |
                    DEBUG_IF_CAP_SPEED_CHANGE,
    .Init = MON8_IF_Init,
    .Close = MON8_IF_Close,
    .Connect = MON8_IF_Connect,
    .Disconnect = MON8_IF_Disconnect,
    .ReadID = MON8_IF_ReadID,
    .GetCapabilities = MON8_IF_GetCapabilities,
    .MemWrite = MON8_IF_MemWrite,
    .MemRead = MON8_IF_MemRead,
    .RegWrite = MON8_IF_RegWrite,
    .RegRead = MON8_IF_RegRead,
    .Reset = MON8_IF_Reset,
    .Halt = MON8_IF_Halt,
    .Run = MON8_IF_Run,
    .Step = MON8_IF_Step,
    .SetSpeed = MON8_IF_SetSpeed,
    .GetSpeed = MON8_IF_GetSpeed,
};

/** FINE接口操作函数表 */
static const Debug_IF_Ops_t FINE_IF_Ops = {
    .name = "FINE",
    .type = DEBUG_IF_FINE,
    .capabilities = DEBUG_IF_CAP_MEM_READ | DEBUG_IF_CAP_MEM_WRITE |
                    DEBUG_IF_CAP_FLASH_ERASE | DEBUG_IF_CAP_FLASH_WRITE |
                    DEBUG_IF_CAP_FLASH_READ | DEBUG_IF_CAP_RESET |
                    DEBUG_IF_CAP_SPEED_CHANGE,
    .Init = FINE_IF_Init,
    .Close = FINE_IF_Close,
    .Connect = FINE_IF_Connect,
    .Disconnect = FINE_IF_Disconnect,
    .ReadID = FINE_IF_ReadID,
    .GetCapabilities = FINE_IF_GetCapabilities,
    .MemWrite = FINE_IF_MemWrite,
    .MemRead = FINE_IF_MemRead,
    .RegWrite = FINE_IF_RegWrite,
    .RegRead = FINE_IF_RegRead,
    .Reset = FINE_IF_Reset,
    .Halt = FINE_IF_Halt,
    .Run = FINE_IF_Run,
    .Step = FINE_IF_Step,
    .SetSpeed = FINE_IF_SetSpeed,
    .GetSpeed = FINE_IF_GetSpeed,
};

/** ICSP接口操作函数表 */
static const Debug_IF_Ops_t ICSP_IF_Ops = {
    .name = "ICSP",
    .type = DEBUG_IF_ICSP,
    .capabilities = DEBUG_IF_CAP_MEM_READ | DEBUG_IF_CAP_MEM_WRITE |
                    DEBUG_IF_CAP_FLASH_ERASE | DEBUG_IF_CAP_FLASH_WRITE |
                    DEBUG_IF_CAP_FLASH_READ | DEBUG_IF_CAP_RESET |
                    DEBUG_IF_CAP_SPEED_CHANGE,
    .Init = ICSP_IF_Init,
    .Close = ICSP_IF_Close,
    .Connect = ICSP_IF_Connect,
    .Disconnect = ICSP_IF_Disconnect,
    .ReadID = ICSP_IF_ReadID,
    .GetCapabilities = ICSP_IF_GetCapabilities,
    .MemWrite = ICSP_IF_MemWrite,
    .MemRead = ICSP_IF_MemRead,
    .RegWrite = ICSP_IF_RegWrite,
    .RegRead = ICSP_IF_RegRead,
    .Reset = ICSP_IF_Reset,
    .Halt = ICSP_IF_Halt,
    .Run = ICSP_IF_Run,
    .Step = ICSP_IF_Step,
    .SetSpeed = ICSP_IF_SetSpeed,
    .GetSpeed = ICSP_IF_GetSpeed,
};

/** ISP接口操作函数表 */
static const Debug_IF_Ops_t ISP_IF_Ops = {
    .name = "ISP",
    .type = DEBUG_IF_ISP,
    .capabilities = DEBUG_IF_CAP_MEM_READ | DEBUG_IF_CAP_MEM_WRITE |
                    DEBUG_IF_CAP_FLASH_ERASE | DEBUG_IF_CAP_FLASH_WRITE |
                    DEBUG_IF_CAP_FLASH_READ | DEBUG_IF_CAP_RESET |
                    DEBUG_IF_CAP_SPEED_CHANGE,
    .Init = ISP_IF_Init,
    .Close = ISP_IF_Close,
    .Connect = ISP_IF_Connect,
    .Disconnect = ISP_IF_Disconnect,
    .ReadID = ISP_IF_ReadID,
    .GetCapabilities = ISP_IF_GetCapabilities,
    .MemWrite = ISP_IF_MemWrite,
    .MemRead = ISP_IF_MemRead,
    .RegWrite = ISP_IF_RegWrite,
    .RegRead = ISP_IF_RegRead,
    .Reset = ISP_IF_Reset,
    .Halt = ISP_IF_Halt,
    .Run = ISP_IF_Run,
    .Step = ISP_IF_Step,
    .SetSpeed = ISP_IF_SetSpeed,
    .GetSpeed = ISP_IF_GetSpeed,
};

/** UART接口操作函数表 */
static const Debug_IF_Ops_t UART_IF_Ops = {
    .name = "UART",
    .type = DEBUG_IF_UART,
    .capabilities = DEBUG_IF_CAP_MEM_READ | DEBUG_IF_CAP_MEM_WRITE |
                    DEBUG_IF_CAP_FLASH_ERASE | DEBUG_IF_CAP_FLASH_WRITE |
                    DEBUG_IF_CAP_FLASH_READ | DEBUG_IF_CAP_RESET |
                    DEBUG_IF_CAP_SPEED_CHANGE,
    .Init = UART_IF_Init,
    .Close = UART_IF_Close,
    .Connect = UART_IF_Connect,
    .Disconnect = UART_IF_Disconnect,
    .ReadID = UART_IF_ReadID,
    .GetCapabilities = UART_IF_GetCapabilities,
    .MemWrite = UART_IF_MemWrite,
    .MemRead = UART_IF_MemRead,
    .RegWrite = UART_IF_RegWrite,
    .RegRead = UART_IF_RegRead,
    .Reset = UART_IF_Reset,
    .Halt = UART_IF_Halt,
    .Run = UART_IF_Run,
    .Step = UART_IF_Step,
    .SetSpeed = UART_IF_SetSpeed,
    .GetSpeed = UART_IF_GetSpeed,
};

/** USB接口操作函数表 */
static const Debug_IF_Ops_t USB_IF_Ops = {
    .name = "USB",
    .type = DEBUG_IF_USB,
    .capabilities = DEBUG_IF_CAP_MEM_READ | DEBUG_IF_CAP_MEM_WRITE |
                    DEBUG_IF_CAP_FLASH_ERASE | DEBUG_IF_CAP_FLASH_WRITE |
                    DEBUG_IF_CAP_FLASH_READ | DEBUG_IF_CAP_RESET,
    .Init = USB_IF_Init,
    .Close = USB_IF_Close,
    .Connect = USB_IF_Connect,
    .Disconnect = USB_IF_Disconnect,
    .ReadID = USB_IF_ReadID,
    .GetCapabilities = USB_IF_GetCapabilities,
    .MemWrite = USB_IF_MemWrite,
    .MemRead = USB_IF_MemRead,
    .RegWrite = USB_IF_RegWrite,
    .RegRead = USB_IF_RegRead,
    .Reset = USB_IF_Reset,
    .Halt = USB_IF_Halt,
    .Run = USB_IF_Run,
    .Step = USB_IF_Step,
    .SetSpeed = USB_IF_SetSpeed,
    .GetSpeed = USB_IF_GetSpeed,
};

/* ==================== SWD接口实现 ==================== */

/**
 * @brief SWD接口初始化
 */
static bool SWD_IF_Init(const Debug_IF_Config_t* config)
{
    SWD_Config_TypeDef swd_config = {0};
    
    /* 配置SWD引脚 - 使用默认配置 */
    swd_config.clock = config->speed_hz > 0 ? config->speed_hz : SWD_DEFAULT_CLOCK;
    swd_config.line_mode = SWD_LINE_SWD;
    
    /* 如果有自定义配置，使用自定义配置 */
    if (config->custom_config != NULL) {
        SWD_Config_TypeDef* custom = (SWD_Config_TypeDef*)config->custom_config;
        swd_config.swdio_port = custom->swdio_port;
        swd_config.swdio_pin = custom->swdio_pin;
        swd_config.swclk_port = custom->swclk_port;
        swd_config.swclk_pin = custom->swclk_pin;
        swd_config.reset_port = custom->reset_port;
        swd_config.reset_pin = custom->reset_pin;
    }
    
    return (SWD_Init(&swd_config) == HAL_OK);
}

/**
 * @brief SWD接口关闭
 */
static bool SWD_IF_Close(void)
{
    return (SWD_DeInit() == HAL_OK);
}

/**
 * @brief SWD接口连接
 */
static bool SWD_IF_Connect(void)
{
    return (SWD_LineReset() == HAL_OK);
}

/**
 * @brief SWD接口断开
 */
static bool SWD_IF_Disconnect(void)
{
    return (SWD_LineReset() == HAL_OK);
}

/**
 * @brief SWD读取ID
 */
static bool SWD_IF_ReadID(uint32_t* id)
{
    if (id == NULL) return false;
    *id = SWD_GetDPID();
    return (*id != 0);
}

/**
 * @brief SWD获取能力
 */
static bool SWD_IF_GetCapabilities(uint32_t* caps)
{
    if (caps == NULL) return false;
    *caps = SWD_IF_Ops.capabilities;
    return true;
}

/**
 * @brief SWD写入内存
 */
static bool SWD_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size)
{
    return (SWD_WriteMem(addr, (uint8_t*)data, size) == HAL_OK);
}

/**
 * @brief SWD读取内存
 */
static bool SWD_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size)
{
    return (SWD_ReadMem(addr, data, size) == HAL_OK);
}

/**
 * @brief SWD写入寄存器
 */
static bool SWD_IF_RegWrite(uint32_t addr, uint32_t value)
{
    return (SWD_WriteWord(addr, value) == HAL_OK);
}

/**
 * @brief SWD读取寄存器
 */
static bool SWD_IF_RegRead(uint32_t addr, uint32_t* value)
{
    if (value == NULL) return false;
    *value = SWD_ReadWord(addr);
    return true;
}

/**
 * @brief SWD复位
 */
static bool SWD_IF_Reset(void)
{
    SWD_Reset();
    return true;
}

/**
 * @brief SWD暂停 - 占位实现
 */
static bool SWD_IF_Halt(void)
{
    /* TODO: 实现SWD暂停功能 */
    return false;
}

/**
 * @brief SWD运行 - 占位实现
 */
static bool SWD_IF_Run(void)
{
    /* TODO: 实现SWD运行功能 */
    return false;
}

/**
 * @brief SWD单步 - 占位实现
 */
static bool SWD_IF_Step(void)
{
    /* TODO: 实现SWD单步功能 */
    return false;
}

/**
 * @brief SWD设置速度
 */
static bool SWD_IF_SetSpeed(uint32_t speed_hz)
{
    return (SWD_SetClock(speed_hz) == HAL_OK);
}

/**
 * @brief SWD获取速度
 */
static uint32_t SWD_IF_GetSpeed(void)
{
    return SWD_GetClock();
}

/* ==================== JTAG接口实现（复用SWD部分代码） ==================== */

/**
 * @brief JTAG接口初始化
 */
static bool JTAG_IF_Init(const Debug_IF_Config_t* config)
{
    /* JTAG使用SWD的底层实现，切换到JTAG模式 */
    SWD_Config_TypeDef jtag_config = {0};
    
    jtag_config.clock = config->speed_hz > 0 ? config->speed_hz : SWD_DEFAULT_CLOCK;
    jtag_config.line_mode = SWD_LINE_JTAG;  /* 切换到JTAG模式 */
    
    if (config->custom_config != NULL) {
        SWD_Config_TypeDef* custom = (SWD_Config_TypeDef*)config->custom_config;
        jtag_config.swdio_port = custom->swdio_port;
        jtag_config.swdio_pin = custom->swdio_pin;
        jtag_config.swclk_port = custom->swclk_port;
        jtag_config.swclk_pin = custom->swclk_pin;
        jtag_config.reset_port = custom->reset_port;
        jtag_config.reset_pin = custom->reset_pin;
    }
    
    return (SWD_Init(&jtag_config) == HAL_OK);
}

/**
 * @brief JTAG接口关闭
 */
static bool JTAG_IF_Close(void)
{
    return (SWD_DeInit() == HAL_OK);
}

/**
 * @brief JTAG接口连接
 */
static bool JTAG_IF_Connect(void)
{
    SWD_SwitchMode(SWD_LINE_JTAG);
    return (SWD_LineReset() == HAL_OK);
}

/**
 * @brief JTAG接口断开
 */
static bool JTAG_IF_Disconnect(void)
{
    return (SWD_LineReset() == HAL_OK);
}

/**
 * @brief JTAG读取ID
 */
static bool JTAG_IF_ReadID(uint32_t* id)
{
    if (id == NULL) return false;
    *id = SWD_GetDPID();
    return (*id != 0);
}

/**
 * @brief JTAG获取能力
 */
static bool JTAG_IF_GetCapabilities(uint32_t* caps)
{
    if (caps == NULL) return false;
    *caps = JTAG_IF_Ops.capabilities;
    return true;
}

/**
 * @brief JTAG写入内存
 */
static bool JTAG_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size)
{
    return (SWD_WriteMem(addr, (uint8_t*)data, size) == HAL_OK);
}

/**
 * @brief JTAG读取内存
 */
static bool JTAG_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size)
{
    return (SWD_ReadMem(addr, data, size) == HAL_OK);
}

/**
 * @brief JTAG写入寄存器
 */
static bool JTAG_IF_RegWrite(uint32_t addr, uint32_t value)
{
    return (SWD_WriteWord(addr, value) == HAL_OK);
}

/**
 * @brief JTAG读取寄存器
 */
static bool JTAG_IF_RegRead(uint32_t addr, uint32_t* value)
{
    if (value == NULL) return false;
    *value = SWD_ReadWord(addr);
    return true;
}

/**
 * @brief JTAG复位
 */
static bool JTAG_IF_Reset(void)
{
    SWD_Reset();
    return true;
}

/**
 * @brief JTAG暂停 - 占位实现
 */
static bool JTAG_IF_Halt(void)
{
    /* TODO: 实现JTAG暂停功能 */
    return false;
}

/**
 * @brief JTAG运行 - 占位实现
 */
static bool JTAG_IF_Run(void)
{
    /* TODO: 实现JTAG运行功能 */
    return false;
}

/**
 * @brief JTAG单步 - 占位实现
 */
static bool JTAG_IF_Step(void)
{
    /* TODO: 实现JTAG单步功能 */
    return false;
}

/**
 * @brief JTAG设置速度
 */
static bool JTAG_IF_SetSpeed(uint32_t speed_hz)
{
    return (SWD_SetClock(speed_hz) == HAL_OK);
}

/**
 * @brief JTAG获取速度
 */
static uint32_t JTAG_IF_GetSpeed(void)
{
    return SWD_GetClock();
}

/* ==================== BDM接口实现 ==================== */

/**
 * @brief BDM接口初始化
 */
static bool BDM_IF_Init(const Debug_IF_Config_t* config)
{
    BDM_HandleTypeDef bdm_config = {0};
    
    bdm_config.speed_hz = config->speed_hz > 0 ? config->speed_hz : BDM_DEFAULT_CLOCK;
    
    if (config->custom_config != NULL) {
        BDM_HandleTypeDef* custom = (BDM_HandleTypeDef*)config->custom_config;
        bdm_config.bkpt_port = custom->bkpt_port;
        bdm_config.bkpt_pin = custom->bkpt_pin;
        bdm_config.reset_port = custom->reset_port;
        bdm_config.reset_pin = custom->reset_pin;
    }
    
    return (BDM_Init(&bdm_config) == HAL_OK);
}

/**
 * @brief BDM接口关闭
 */
static bool BDM_IF_Close(void)
{
    return (BDM_DeInit(&g_bdm_handle) == HAL_OK);
}

/**
 * @brief BDM接口连接
 */
static bool BDM_IF_Connect(void)
{
    return (BDM_EnterBDM(&g_bdm_handle) == HAL_OK);
}

/**
 * @brief BDM接口断开
 */
static bool BDM_IF_Disconnect(void)
{
    return (BDM_ExitBDM(&g_bdm_handle) == HAL_OK);
}

/**
 * @brief BDM读取ID
 */
static bool BDM_IF_ReadID(uint32_t* id)
{
    /* TODO: 实现BDM读取ID功能 */
    if (id == NULL) return false;
    *id = 0;
    return false;
}

/**
 * @brief BDM获取能力
 */
static bool BDM_IF_GetCapabilities(uint32_t* caps)
{
    if (caps == NULL) return false;
    *caps = BDM_IF_Ops.capabilities;
    return true;
}

/**
 * @brief BDM写入内存
 */
static bool BDM_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size)
{
    return (BDM_WriteMem(&g_bdm_handle, addr, (uint8_t*)data, (uint16_t)size) == HAL_OK);
}

/**
 * @brief BDM读取内存
 */
static bool BDM_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size)
{
    return (BDM_ReadMem(&g_bdm_handle, addr, data, (uint16_t)size) == HAL_OK);
}

/**
 * @brief BDM写入寄存器
 */
static bool BDM_IF_RegWrite(uint32_t addr, uint32_t value)
{
    /* TODO: 实现BDM寄存器写入 */
    return false;
}

/**
 * @brief BDM读取寄存器
 */
static bool BDM_IF_RegRead(uint32_t addr, uint32_t* value)
{
    /* TODO: 实现BDM寄存器读取 */
    if (value == NULL) return false;
    return false;
}

/**
 * @brief BDM复位
 */
static bool BDM_IF_Reset(void)
{
    /* TODO: 实现BDM复位 */
    return false;
}

/**
 * @brief BDM暂停
 */
static bool BDM_IF_Halt(void)
{
    /* TODO: 实现BDM暂停功能 */
    return false;
}

/**
 * @brief BDM运行
 */
static bool BDM_IF_Run(void)
{
    /* TODO: 实现BDM运行功能 */
    return false;
}

/**
 * @brief BDM单步
 */
static bool BDM_IF_Step(void)
{
    /* TODO: 实现BDM单步功能 */
    return false;
}

/**
 * @brief BDM设置速度
 */
static bool BDM_IF_SetSpeed(uint32_t speed_hz)
{
    BDM_SetSpeed(&g_bdm_handle, speed_hz);
    return true;
}

/**
 * @brief BDM获取速度
 */
static uint32_t BDM_IF_GetSpeed(void)
{
    return BDM_GetSpeed(&g_bdm_handle);
}

/* ==================== SBW接口实现 ==================== */

/**
 * @brief SBW接口初始化
 */
static bool SBW_IF_Init(const Debug_IF_Config_t* config)
{
    SBW_Config_TypeDef sbw_config = {0};
    
    sbw_config.speed_hz = config->speed_hz > 0 ? config->speed_hz : SBW_CLOCK_100KHZ;
    
    if (config->custom_config != NULL) {
        SBW_Config_TypeDef* custom = (SBW_Config_TypeDef*)config->custom_config;
        sbw_config.tck_port = custom->tck_port;
        sbw_config.tck_pin = custom->tck_pin;
        sbw_config.tms_port = custom->tms_port;
        sbw_config.tms_pin = custom->tms_pin;
        sbw_config.rst_port = custom->rst_port;
        sbw_config.rst_pin = custom->rst_pin;
        sbw_config.test_port = custom->test_port;
        sbw_config.test_pin = custom->test_pin;
    }
    
    return (SBW_Init(&sbw_config) == HAL_OK);
}

/**
 * @brief SBW接口关闭
 */
static bool SBW_IF_Close(void)
{
    return (SBW_DeInit() == HAL_OK);
}

/**
 * @brief SBW接口连接
 */
static bool SBW_IF_Connect(void)
{
    return (SBW_Enter() == HAL_OK);
}

/**
 * @brief SBW接口断开
 */
static bool SBW_IF_Disconnect(void)
{
    return (SBW_Exit() == HAL_OK);
}

/**
 * @brief SBW读取ID
 */
static bool SBW_IF_ReadID(uint32_t* id)
{
    if (id == NULL) return false;
    *id = SBW_GetIDCode();
    return (*id != 0);
}

/**
 * @brief SBW获取能力
 */
static bool SBW_IF_GetCapabilities(uint32_t* caps)
{
    if (caps == NULL) return false;
    *caps = SBW_IF_Ops.capabilities;
    return true;
}

/**
 * @brief SBW写入内存
 */
static bool SBW_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size)
{
    return (SBW_WriteMem(addr, (uint8_t*)data, size) == HAL_OK);
}

/**
 * @brief SBW读取内存
 */
static bool SBW_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size)
{
    return (SBW_ReadMem(addr, data, size) == HAL_OK);
}

/**
 * @brief SBW写入寄存器
 */
static bool SBW_IF_RegWrite(uint32_t addr, uint32_t value)
{
    return (SBW_WriteWord(addr, (uint16_t)value) == HAL_OK);
}

/**
 * @brief SBW读取寄存器
 */
static bool SBW_IF_RegRead(uint32_t addr, uint32_t* value)
{
    if (value == NULL) return false;
    *value = SBW_ReadWord(addr);
    return true;
}

/**
 * @brief SBW复位
 */
static bool SBW_IF_Reset(void)
{
    return (SBW_Reset() == HAL_OK);
}

/**
 * @brief SBW暂停
 */
static bool SBW_IF_Halt(void)
{
    /* TODO: 实现SBW暂停功能 */
    return false;
}

/**
 * @brief SBW运行
 */
static bool SBW_IF_Run(void)
{
    /* TODO: 实现SBW运行功能 */
    return false;
}

/**
 * @brief SBW单步
 */
static bool SBW_IF_Step(void)
{
    /* TODO: 实现SBW单步功能 */
    return false;
}

/**
 * @brief SBW设置速度
 */
static bool SBW_IF_SetSpeed(uint32_t speed_hz)
{
    SBW_SetSpeed(speed_hz);
    return true;
}

/**
 * @brief SBW获取速度
 */
static uint32_t SBW_IF_GetSpeed(void)
{
    return SBW_GetSpeed();
}

/* ==================== MON8接口实现 ==================== */

/**
 * @brief MON8接口初始化
 */
static bool MON8_IF_Init(const Debug_IF_Config_t* config)
{
    MON8_Config_TypeDef mon8_config = {0};
    
    mon8_config.speed_hz = config->speed_hz > 0 ? config->speed_hz : MON8_DEFAULT_CLOCK;
    
    if (config->custom_config != NULL) {
        MON8_Config_TypeDef* custom = (MON8_Config_TypeDef*)config->custom_config;
        mon8_config.bkpt_port = custom->bkpt_port;
        mon8_config.bkpt_pin = custom->bkpt_pin;
        mon8_config.rst_port = custom->rst_port;
        mon8_config.rst_pin = custom->rst_pin;
        mon8_config.ptx_port = custom->ptx_port;
        mon8_config.ptx_pin = custom->ptx_pin;
        mon8_config.prx_port = custom->prx_port;
        mon8_config.prx_pin = custom->prx_pin;
    }
    
    return (MON8_Init(&mon8_config) == HAL_OK);
}

/**
 * @brief MON8接口关闭
 */
static bool MON8_IF_Close(void)
{
    return (MON8_DeInit() == HAL_OK);
}

/**
 * @brief MON8接口连接
 */
static bool MON8_IF_Connect(void)
{
    return (MON8_Enter() == HAL_OK);
}

/**
 * @brief MON8接口断开
 */
static bool MON8_IF_Disconnect(void)
{
    return (MON8_Exit() == HAL_OK);
}

/**
 * @brief MON8读取ID
 */
static bool MON8_IF_ReadID(uint32_t* id)
{
    if (id == NULL) return false;
    *id = MON8_GetDeviceID();
    return true;
}

/**
 * @brief MON8获取能力
 */
static bool MON8_IF_GetCapabilities(uint32_t* caps)
{
    if (caps == NULL) return false;
    *caps = MON8_IF_Ops.capabilities;
    return true;
}

/**
 * @brief MON8写入内存
 */
static bool MON8_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size)
{
    return (MON8_WriteMem((uint16_t)addr, (uint8_t*)data, (uint16_t)size) == HAL_OK);
}

/**
 * @brief MON8读取内存
 */
static bool MON8_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size)
{
    return (MON8_ReadMem((uint16_t)addr, data, (uint16_t)size) == HAL_OK);
}

/**
 * @brief MON8写入寄存器
 */
static bool MON8_IF_RegWrite(uint32_t addr, uint32_t value)
{
    /* TODO: 实现MON8寄存器写入 */
    return false;
}

/**
 * @brief MON8读取寄存器
 */
static bool MON8_IF_RegRead(uint32_t addr, uint32_t* value)
{
    /* TODO: 实现MON8寄存器读取 */
    if (value == NULL) return false;
    return false;
}

/**
 * @brief MON8复位
 */
static bool MON8_IF_Reset(void)
{
    return (MON8_Reset() == HAL_OK);
}

/**
 * @brief MON8暂停
 */
static bool MON8_IF_Halt(void)
{
    return (MON8_Stop() == HAL_OK);
}

/**
 * @brief MON8运行
 */
static bool MON8_IF_Run(void)
{
    return (MON8_Run(0) == HAL_OK);
}

/**
 * @brief MON8单步
 */
static bool MON8_IF_Step(void)
{
    /* TODO: 实现MON8单步功能 */
    return false;
}

/**
 * @brief MON8设置速度
 */
static bool MON8_IF_SetSpeed(uint32_t speed_hz)
{
    MON8_SetSpeed(speed_hz);
    return true;
}

/**
 * @brief MON8获取速度
 */
static uint32_t MON8_IF_GetSpeed(void)
{
    return MON8_GetSpeed();
}

/* ==================== FINE接口实现 ==================== */

/**
 * @brief FINE接口初始化
 */
static bool FINE_IF_Init(const Debug_IF_Config_t* config)
{
    FINE_Config_TypeDef fine_config = {0};
    
    fine_config.speed_hz = config->speed_hz > 0 ? config->speed_hz : FINE_DEFAULT_CLOCK;
    
    if (config->custom_config != NULL) {
        FINE_Config_TypeDef* custom = (FINE_Config_TypeDef*)config->custom_config;
        fine_config.flmd0_port = custom->flmd0_port;
        fine_config.flmd0_pin = custom->flmd0_pin;
        fine_config.flmd1_port = custom->flmd1_port;
        fine_config.flmd1_pin = custom->flmd1_pin;
        fine_config.flmd2_port = custom->flmd2_port;
        fine_config.flmd2_pin = custom->flmd2_pin;
        fine_config.flmd3_port = custom->flmd3_port;
        fine_config.flmd3_pin = custom->flmd3_pin;
        fine_config.flclk_port = custom->flclk_port;
        fine_config.flclk_pin = custom->flclk_pin;
        fine_config.reset_port = custom->reset_port;
        fine_config.reset_pin = custom->reset_pin;
    }
    
    return (FINE_Init(&fine_config) == HAL_OK);
}

/**
 * @brief FINE接口关闭
 */
static bool FINE_IF_Close(void)
{
    return (FINE_DeInit() == HAL_OK);
}

/**
 * @brief FINE接口连接
 */
static bool FINE_IF_Connect(void)
{
    return (FINE_Enter() == HAL_OK);
}

/**
 * @brief FINE接口断开
 */
static bool FINE_IF_Disconnect(void)
{
    return (FINE_Exit() == HAL_OK);
}

/**
 * @brief FINE读取ID
 */
static bool FINE_IF_ReadID(uint32_t* id)
{
    if (id == NULL) return false;
    *id = FINE_GetChipID();
    return (*id != 0);
}

/**
 * @brief FINE获取能力
 */
static bool FINE_IF_GetCapabilities(uint32_t* caps)
{
    if (caps == NULL) return false;
    *caps = FINE_IF_Ops.capabilities;
    return true;
}

/**
 * @brief FINE写入内存
 */
static bool FINE_IF_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size)
{
    return (FINE_WriteMem(addr, (uint8_t*)data, size) == HAL_OK);
}

/**
 * @brief FINE读取内存
 */
static bool FINE_IF_MemRead(uint32_t addr, uint8_t* data, uint32_t size)
{
    return (FINE_ReadMem(addr, data, size) == HAL_OK);
}

/**
 * @brief FINE写入寄存器
 */
static bool FINE_IF_RegWrite(uint32_t addr, uint32_t value)
{
    /* TODO: 实现FINE寄存器写入 */
    return false;
}

/**
 * @brief FINE读取寄存器
 */
static bool FINE_IF_RegRead(uint32_t addr, uint32_t* value)
{
    /* TODO: 实现FINE寄存器读取 */
    if (value == NULL) return false;
    return false;
}

/**
 * @brief FINE复位
 */
static bool FINE_IF_Reset(void)
{
    return (FINE_Reset() == HAL_OK);
}

/**
 * @brief FINE暂停
 */
static bool FINE_IF_Halt(void)
{
    /* TODO: 实现FINE暂停功能 */
    return false;
}

/**
 * @brief FINE运行
 */
static bool FINE_IF_Run(void)
{
    /* TODO: 实现FINE运行功能 */
    return false;
}

/**
 * @brief FINE单步
 */
static bool FINE_IF_Step(void)
{
    /* TODO: 实现FINE单步功能 */
    return false;
}

/**
 * @brief FINE设置速度
 */
static bool FINE_IF_SetSpeed(uint32_t speed_hz)
{
    FINE_SetSpeed(speed_hz);
    return true;
}

/**
 * @brief FINE获取速度
 */
static uint32_t FINE_IF_GetSpeed(void)
{
    return FINE_GetSpeed();
}

/* ==================== 管理器函数实现 ==================== */

/**
 * @brief 初始化调试接口管理器
 */
bool Debug_IF_Manager_Init(void)
{
    uint32_t i;
    
    /* 检查是否已初始化 */
    if (g_debug_if_manager.initialized) {
        return true;
    }
    
    /* 清空注册表 */
    memset(g_debug_if_manager.registry, 0, sizeof(g_debug_if_manager.registry));
    g_debug_if_manager.registered_count = 0;
    g_debug_if_manager.active_instance = NULL;
    
    /* 注册内置调试接口 */
    Debug_IF_Register(DEBUG_IF_SWD, &SWD_IF_Ops);
    Debug_IF_Register(DEBUG_IF_JTAG, &JTAG_IF_Ops);
    Debug_IF_Register(DEBUG_IF_BDM, &BDM_IF_Ops);
    Debug_IF_Register(DEBUG_IF_SBW, &SBW_IF_Ops);
    Debug_IF_Register(DEBUG_IF_MON8, &MON8_IF_Ops);
    Debug_IF_Register(DEBUG_IF_FINE, &FINE_IF_Ops);
    Debug_IF_Register(DEBUG_IF_ICSP, &ICSP_IF_Ops);
    Debug_IF_Register(DEBUG_IF_ISP, &ISP_IF_Ops);
    Debug_IF_Register(DEBUG_IF_UART, &UART_IF_Ops);
    Debug_IF_Register(DEBUG_IF_USB, &USB_IF_Ops);
    
    g_debug_if_manager.initialized = true;
    
    return true;
}

/**
 * @brief 关闭调试接口管理器
 */
bool Debug_IF_Manager_Close(void)
{
    uint32_t i;
    
    if (!g_debug_if_manager.initialized) {
        return true;
    }
    
    /* 销毁活动实例 */
    if (g_debug_if_manager.active_instance != NULL) {
        Debug_IF_Destroy(g_debug_if_manager.active_instance);
        g_debug_if_manager.active_instance = NULL;
    }
    
    /* 清空注册表 */
    for (i = 0; i < DEBUG_IF_MAX_TYPES; i++) {
        g_debug_if_manager.registry[i].is_registered = false;
        g_debug_if_manager.registry[i].ops = NULL;
        g_debug_if_manager.registry[i].name = NULL;
        g_debug_if_manager.registry[i].type = DEBUG_IF_UNKNOWN;
    }
    
    g_debug_if_manager.registered_count = 0;
    g_debug_if_manager.initialized = false;
    
    return true;
}

/**
 * @brief 注册调试接口类型
 */
bool Debug_IF_Register(Chip_Debug_Interface_t type, const Debug_IF_Ops_t* ops)
{
    uint32_t i;
    
    /* 参数检查 */
    if (ops == NULL || type == DEBUG_IF_UNKNOWN) {
        return false;
    }
    
    /* 检查是否已注册 */
    for (i = 0; i < DEBUG_IF_MAX_TYPES; i++) {
        if (g_debug_if_manager.registry[i].type == type && 
            g_debug_if_manager.registry[i].is_registered) {
            /* 已存在，更新操作函数 */
            g_debug_if_manager.registry[i].ops = ops;
            g_debug_if_manager.registry[i].name = ops->name;
            return true;
        }
    }
    
    /* 查找空闲槽位 */
    for (i = 0; i < DEBUG_IF_MAX_TYPES; i++) {
        if (!g_debug_if_manager.registry[i].is_registered) {
            g_debug_if_manager.registry[i].type = type;
            g_debug_if_manager.registry[i].name = ops->name;
            g_debug_if_manager.registry[i].ops = ops;
            g_debug_if_manager.registry[i].is_registered = true;
            g_debug_if_manager.registered_count++;
            return true;
        }
    }
    
    return false;  /* 注册表已满 */
}

/**
 * @brief 注销调试接口类型
 */
bool Debug_IF_Unregister(Chip_Debug_Interface_t type)
{
    uint32_t i;
    
    for (i = 0; i < DEBUG_IF_MAX_TYPES; i++) {
        if (g_debug_if_manager.registry[i].type == type &&
            g_debug_if_manager.registry[i].is_registered) {
            g_debug_if_manager.registry[i].is_registered = false;
            g_debug_if_manager.registry[i].ops = NULL;
            g_debug_if_manager.registry[i].name = NULL;
            g_debug_if_manager.registry[i].type = DEBUG_IF_UNKNOWN;
            g_debug_if_manager.registered_count--;
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 获取已注册的调试接口操作函数
 */
const Debug_IF_Ops_t* Debug_IF_Get(Chip_Debug_Interface_t type)
{
    uint32_t i;
    
    for (i = 0; i < DEBUG_IF_MAX_TYPES; i++) {
        if (g_debug_if_manager.registry[i].type == type &&
            g_debug_if_manager.registry[i].is_registered) {
            return g_debug_if_manager.registry[i].ops;
        }
    }
    
    return NULL;
}

/**
 * @brief 创建调试接口实例
 */
Debug_IF_Instance_t* Debug_IF_Create(const Debug_IF_Config_t* config)
{
    const Debug_IF_Ops_t* ops;
    Debug_IF_Instance_t* instance;
    
    /* 参数检查 */
    if (config == NULL) {
        return NULL;
    }
    
    /* 获取接口操作函数 */
    ops = Debug_IF_Get(config->type);
    if (ops == NULL) {
        return NULL;
    }
    
    /* 分配实例内存 */
    instance = (Debug_IF_Instance_t*)malloc(sizeof(Debug_IF_Instance_t));
    if (instance == NULL) {
        return NULL;
    }
    
    /* 初始化实例 */
    memset(instance, 0, sizeof(Debug_IF_Instance_t));
    instance->type = config->type;
    instance->ops = ops;
    instance->speed_hz = config->speed_hz;
    instance->is_connected = false;
    instance->is_initialized = false;
    
    /* 调用接口初始化 */
    if (ops->Init != NULL) {
        if (!ops->Init(config)) {
            free(instance);
            return NULL;
        }
        instance->is_initialized = true;
    }
    
    /* 设置为活动实例 */
    g_debug_if_manager.active_instance = instance;
    
    return instance;
}

/**
 * @brief 销毁调试接口实例
 */
bool Debug_IF_Destroy(Debug_IF_Instance_t* instance)
{
    if (instance == NULL) {
        return false;
    }
    
    /* 断开连接 */
    if (instance->is_connected && instance->ops->Disconnect != NULL) {
        instance->ops->Disconnect();
        instance->is_connected = false;
    }
    
    /* 关闭接口 */
    if (instance->is_initialized && instance->ops->Close != NULL) {
        instance->ops->Close();
        instance->is_initialized = false;
    }
    
    /* 清除活动实例引用 */
    if (g_debug_if_manager.active_instance == instance) {
        g_debug_if_manager.active_instance = NULL;
    }
    
    /* 释放内存 */
    free(instance);
    
    return true;
}

/**
 * @brief 获取接口类型名称
 */
const char* Debug_IF_GetTypeName(Chip_Debug_Interface_t type)
{
    switch (type) {
        case DEBUG_IF_SWD:    return "SWD";
        case DEBUG_IF_JTAG:   return "JTAG";
        case DEBUG_IF_BDM:    return "BDM";
        case DEBUG_IF_MON8:   return "MON8";
        case DEBUG_IF_SBW:    return "SBW";
        case DEBUG_IF_FINE:   return "FINE";
        case DEBUG_IF_ICSP:   return "ICSP";
        case DEBUG_IF_ISP:    return "ISP";
        case DEBUG_IF_USB:    return "USB";
        case DEBUG_IF_UART:   return "UART";
        case DEBUG_IF_SPI:    return "SPI";
        case DEBUG_IF_I2C:    return "I2C";
        case DEBUG_IF_CAN:    return "CAN";
        case DEBUG_IF_DAP:    return "DAP";
        case DEBUG_IF_SWD_JTAG: return "SWD/JTAG";
        default:              return "UNKNOWN";
    }
}

/**
 * @brief 检查接口类型是否已注册
 */
bool Debug_IF_IsRegistered(Chip_Debug_Interface_t type)
{
    uint32_t i;
    
    for (i = 0; i < DEBUG_IF_MAX_TYPES; i++) {
        if (g_debug_if_manager.registry[i].type == type &&
            g_debug_if_manager.registry[i].is_registered) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 获取已注册接口数量
 */
uint32_t Debug_IF_GetRegisteredCount(void)
{
    return g_debug_if_manager.registered_count;
}

/**
 * @brief 获取所有已注册接口类型列表
 */
uint32_t Debug_IF_GetRegisteredTypes(Chip_Debug_Interface_t* types, uint32_t max_count)
{
    uint32_t i;
    uint32_t count = 0;
    
    if (types == NULL || max_count == 0) {
        return 0;
    }
    
    for (i = 0; i < DEBUG_IF_MAX_TYPES && count < max_count; i++) {
        if (g_debug_if_manager.registry[i].is_registered) {
            types[count++] = g_debug_if_manager.registry[i].type;
        }
    }
    
    return count;
}

/**
 * @brief 打印已注册接口列表（调试用）
 */
void Debug_IF_PrintRegisteredList(void)
{
    uint32_t i;
    
    printf("=== Registered Debug Interfaces (%u) ===\r\n", 
           (unsigned int)g_debug_if_manager.registered_count);
    
    for (i = 0; i < DEBUG_IF_MAX_TYPES; i++) {
        if (g_debug_if_manager.registry[i].is_registered) {
            printf("  [%02u] %s\r\n", 
                   (unsigned int)i,
                   g_debug_if_manager.registry[i].name);
        }
    }
}

/* ==================== 便捷API函数实现 ==================== */

/**
 * @brief 快速创建并初始化调试接口
 */
Debug_IF_Instance_t* Debug_IF_QuickCreate(Chip_Debug_Interface_t type, uint32_t speed_hz)
{
    Debug_IF_Config_t config = {
        .type = type,
        .speed_hz = speed_hz,
        .custom_config = NULL
    };
    
    return Debug_IF_Create(&config);
}

/**
 * @brief 快速销毁调试接口
 */
bool Debug_IF_QuickDestroy(Debug_IF_Instance_t* instance)
{
    return Debug_IF_Destroy(instance);
}