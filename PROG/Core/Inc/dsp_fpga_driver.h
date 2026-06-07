/**
 ******************************************************************************
 * @file    dsp_fpga_driver.h
 * @brief   DSP/FPGA/CPLD驱动头文件
 *          支持TI TMS320、ADI Blackfin DSP、Xilinx/Altera/Lattice FPGA/CPLD
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#ifndef __DSP_FPGA_DRIVER_H__
#define __DSP_FPGA_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* ==================== DSP类型定义 ==================== */
typedef enum {
    DSP_TYPE_TMS320C2000 = 0,       /* TI C2000系列(DSP+MCU) */
    DSP_TYPE_TMS320C5000,           /* TI C5000系列(低功耗) */
    DSP_TYPE_TMS320C6000,           /* TI C6000系列(高性能) */
    DSP_TYPE_TMS320DM,              /* TI DaVinci系列(视频) */
    DSP_TYPE_TMS320OMAP,            /* TI OMAP系列 */
    DSP_TYPE_BLACKFIN,              /* ADI Blackfin系列 */
    DSP_TYPE_SHARC,                 /* ADI SHARC系列 */
    DSP_TYPE_ADSP_21XX,             /* ADI 21xx系列 */
} DSP_Type_t;

/* ==================== FPGA类型定义 ==================== */
typedef enum {
    FPGA_TYPE_XILINX_SERIES7 = 0,   /* Xilinx 7系列(Artix/Kintex/Virtex) */
    FPGA_TYPE_XILINX_ULTRASCALE,    /* Xilinx UltraScale系列 */
    FPGA_TYPE_XILINX_SPARTAN,       /* Xilinx Spartan系列 */
    FPGA_TYPE_XILINX_COOLRUNNER,    /* Xilinx CoolRunner CPLD */
    FPGA_TYPE_INTEL_CYCLONE,        /* Intel/Altera Cyclone系列 */
    FPGA_TYPE_INTEL_STRATIX,        /* Intel/Altera Stratix系列 */
    FPGA_TYPE_INTEL_MAX,            /* Intel/Altera MAX CPLD系列 */
    FPGA_TYPE_LATTICE_ICE40,        /* Lattice iCE40系列 */
    FPGA_TYPE_LATTICE_ECP5,         /* Lattice ECP5系列 */
    FPGA_TYPE_LATTICE_MACHXO,       /* Lattice MachXO系列 */
    FPGA_TYPE_GOWIN_GW1N,           /* 高云GW1N系列 */
    FPGA_TYPE_GOWIN_GW2A,           /* 高云GW2A系列 */
    FPGA_TYPE_ANLOGIC_EG4,          /* 安路EG4系列 */
} FPGA_Type_t;

/* ==================== JTAG配置结构体 ==================== */
typedef struct {
    GPIO_TypeDef* tck_port;         /* TCK引脚端口 */
    uint16_t      tck_pin;          /* TCK引脚 */
    GPIO_TypeDef* tms_port;         /* TMS引脚端口 */
    uint16_t      tms_pin;          /* TMS引脚 */
    GPIO_TypeDef* tdi_port;         /* TDI引脚端口 */
    uint16_t      tdi_pin;          /* TDI引脚 */
    GPIO_TypeDef* tdo_port;         /* TDO引脚端口 */
    uint16_t      tdo_pin;          /* TDO引脚 */
    uint32_t      clock_hz;         /* JTAG时钟频率 */
} JTAG_Config_t;

/* ==================== DSP句柄结构体 ==================== */
typedef struct {
    DSP_Type_t    type;             /* DSP类型 */
    uint32_t      device_id;        /* 设备ID */
    JTAG_Config_t jtag;             /* JTAG配置 */
    uint32_t      flash_base;       /* Flash基地址 */
    uint32_t      flash_size;       /* Flash大小 */
    uint32_t      ram_base;         /* RAM基地址 */
    uint32_t      ram_size;         /* RAM大小 */
    char          part_number[32];  /* 型号 */
    uint8_t       initialized;      /* 初始化标志 */
} DSP_HandleTypeDef;

/* ==================== FPGA句柄结构体 ==================== */
typedef struct {
    FPGA_Type_t   type;             /* FPGA类型 */
    uint32_t      device_id;        /* 设备ID */
    JTAG_Config_t jtag;             /* JTAG配置 */
    uint32_t      flash_size;       /* 配置Flash大小 */
    char          part_number[32];  /* 型号 */
    uint8_t       initialized;      /* 初始化标志 */
    uint8_t       is_cpld;          /* 是否为CPLD */
} FPGA_HandleTypeDef;

/* ==================== DSP函数声明 ==================== */

/* 初始化 */
HAL_StatusTypeDef DSP_Init(DSP_HandleTypeDef* hdsp);
HAL_StatusTypeDef DSP_DeInit(DSP_HandleTypeDef* hdsp);

/* 检测 */
HAL_StatusTypeDef DSP_Detect(DSP_HandleTypeDef* hdsp);

/* Flash操作 */
HAL_StatusTypeDef DSP_EraseFlash(DSP_HandleTypeDef* hdsp, uint32_t addr, uint32_t size);
HAL_StatusTypeDef DSP_ProgramFlash(DSP_HandleTypeDef* hdsp, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef DSP_ReadFlash(DSP_HandleTypeDef* hdsp, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef DSP_VerifyFlash(DSP_HandleTypeDef* hdsp, uint32_t addr, uint8_t* data, uint32_t size);

/* 内存操作 */
HAL_StatusTypeDef DSP_WriteMem(DSP_HandleTypeDef* hdsp, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef DSP_ReadMem(DSP_HandleTypeDef* hdsp, uint32_t addr, uint8_t* data, uint32_t size);

/* TMS320特殊操作 */
HAL_StatusTypeDef DSP_TMS320_Reset(DSP_HandleTypeDef* hdsp);
HAL_StatusTypeDef DSP_TMS320_Halt(DSP_HandleTypeDef* hdsp);
HAL_StatusTypeDef DSP_TMS320_Run(DSP_HandleTypeDef* hdsp);

/* ==================== FPGA函数声明 ==================== */

/* 初始化 */
HAL_StatusTypeDef FPGA_Init(FPGA_HandleTypeDef* hfpga);
HAL_StatusTypeDef FPGA_DeInit(FPGA_HandleTypeDef* hfpga);

/* 检测 */
HAL_StatusTypeDef FPGA_Detect(FPGA_HandleTypeDef* hfpga);

/* 配置操作 */
HAL_StatusTypeDef FPGA_Configure(FPGA_HandleTypeDef* hfpga, uint8_t* bitstream, uint32_t size);
HAL_StatusTypeDef FPGA_VerifyConfiguration(FPGA_HandleTypeDef* hfpga, uint8_t* bitstream, uint32_t size);
HAL_StatusTypeDef FPGA_ReadBack(FPGA_HandleTypeDef* hfpga, uint8_t* data, uint32_t* size);

/* Flash操作(配置Flash) */
HAL_StatusTypeDef FPGA_EraseFlash(FPGA_HandleTypeDef* hfpga, uint32_t addr, uint32_t size);
HAL_StatusTypeDef FPGA_ProgramFlash(FPGA_HandleTypeDef* hfpga, uint32_t addr, uint8_t* data, uint32_t size);
HAL_StatusTypeDef FPGA_ReadFlash(FPGA_HandleTypeDef* hfpga, uint32_t addr, uint8_t* data, uint32_t size);

/* Xilinx专用操作 */
HAL_StatusTypeDef FPGA_Xilinx_JTAG_Program(FPGA_HandleTypeDef* hfpga, uint8_t* bitstream, uint32_t size);
HAL_StatusTypeDef FPGA_Xilinx_SlaveSerial_Program(FPGA_HandleTypeDef* hfpga, uint8_t* bitstream, uint32_t size);
HAL_StatusTypeDef FPGA_Xilinx_SelectMAP_Program(FPGA_HandleTypeDef* hfpga, uint8_t* data, uint32_t size);

/* Intel/Altera专用操作 */
HAL_StatusTypeDef FPGA_Intel_JTAG_Program(FPGA_HandleTypeDef* hfpga, uint8_t* data, uint32_t size);
HAL_StatusTypeDef FPGA_Intel_PS_Program(FPGA_HandleTypeDef* hfpga, uint8_t* data, uint32_t size);
HAL_StatusTypeDef FPGA_Intel_FPP_Program(FPGA_HandleTypeDef* hfpga, uint8_t* data, uint32_t size);

/* Lattice专用操作 */
HAL_StatusTypeDef FPGA_Lattice_JTAG_Program(FPGA_HandleTypeDef* hfpga, uint8_t* data, uint32_t size);
HAL_StatusTypeDef FPGA_Lattice_SS_Program(FPGA_HandleTypeDef* hfpga, uint8_t* bitstream, uint32_t size);

/* 状态查询 */
uint8_t FPGA_GetInitStatus(FPGA_HandleTypeDef* hfpga);
uint8_t FPGA_GetDoneStatus(FPGA_HandleTypeDef* hfpga);

#ifdef __cplusplus
}
#endif

#endif /* __DSP_FPGA_DRIVER_H__ */