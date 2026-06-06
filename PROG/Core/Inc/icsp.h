/**
 ******************************************************************************
 * @file    icsp.h
 * @brief   ICSP (In-Circuit Serial Programming) 和 ISP (In-System Programming) 接口实现
 *
 *          ICSP用于Microchip PIC系列微控制器
 *          - 使用2-3根线：PGC(时钟)、PGD(数据)、MCLR/VPP(编程电压)
 *          - 支持PIC12/PIC16/PIC18系列
 *
 *          ISP用于Atmel AVR系列微控制器
 *          - 使用4根线：MOSI、MISO、SCK、RESET
 *          - 支持ATmega/ATtiny/AT90系列
 *
 *          使用GPIO_Soft框架进行IO口软件模拟
 ******************************************************************************
 */

#ifndef __ICSP_H
#define __ICSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "gpio_soft.h"
#include <stdint.h>

/*============================================================================*
 * ICSP (PIC系列) 定义
 *============================================================================*/

/* ICSP状态码 */
#define ICSP_OK              0x00
#define ICSP_ERR             0x01
#define ICSP_ERR_TIMEOUT     0x02
#define ICSP_ERR_NACK        0x03
#define ICSP_ERR_VERIFY      0x04
#define ICSP_ERR_PARAM       0x05

/* ICSP时钟频率选项 */
#define ICSP_CLOCK_100KHZ    100000
#define ICSP_CLOCK_200KHZ    200000
#define ICSP_CLOCK_500KHZ    500000
#define ICSP_CLOCK_1MHZ      1000000
#define ICSP_CLOCK_2MHZ      2000000
#define ICSP_CLOCK_4MHZ      4000000
#define ICSP_CLOCK_8MHZ      8000000

#define ICSP_DEFAULT_CLOCK   ICSP_CLOCK_1MHZ

/* ICSP定时器配置 */
#define ICSP_TIM_INSTANCE    TIM7
#define ICSP_TIM_CLK_ENABLE() __HAL_RCC_TIM7_CLK_ENABLE()

/* ICSP命令定义 - PICmicro系列 */
#define ICSP_CMD_LOAD_CONFIG         0x00    /* 加载配置字 */
#define ICSP_CMD_LOAD_DATA            0x02    /* 加载数据 */
#define ICSP_CMD_READ_DATA            0x03    /* 读取数据 */
#define ICSP_CMD_INCREMENT_ADDR       0x06    /* 地址递增 */
#define ICSP_CMD_RESET_ADDR           0x16    /* 地址复位 */
#define ICSP_CMD_BEGIN_ERASE          0x08    /* 开始擦除 */
#define ICSP_CMD_BULK_ERASE          0x09    /* 整片擦除 */
#define ICSP_CMD_ROW_ERASE           0x0A    /* 行擦除 */
#define ICSP_CMD_WRITE_DATA           0x0D    /* 写数据 */
#define ICSP_CMD_WRITE_CONFIG         0x00    /* 写配置字 */
#define ICSP_CMD_CHIP_ERASE          0x0F    /* 芯片擦除 */
#define ICSP_CMD_READ_ID              0x04    /* 读取ID */
#define ICSP_CMD_READ_BOOT_ROM       0x05    /* 读取引导ROM */
#define ICSP_CMD_SET_ACTIVE_ROM      0x07    /* 设置激活的ROM */
#define ICSP_CMD_GET_VERSION          0xFF    /* 获取版本 */

/* ICSP状态机状态 */
typedef enum {
    ICSP_STATE_IDLE = 0,
    ICSP_STATE_ENTERING,
    ICSP_STATE_PROGRAMMING,
    ICSP_STATE_VERIFYING,
    ICSP_STATE_READING,
    ICSP_STATE_ERASING,
    ICSP_STATE_EXITING,
    ICSP_STATE_ERROR
} ICSP_State_TypeDef;

/* ICSP配置结构体 */
typedef struct {
    /* GPIO端口和引脚配置 */
    GPIO_TypeDef* pgc_port;          /* PGC时钟端口 */
    uint16_t pgc_pin;               /* PGC时钟引脚 */
    GPIO_TypeDef* pgd_port;         /* PGD数据端口 */
    uint16_t pgd_pin;               /* PGD数据引脚 */
    GPIO_TypeDef* mclr_port;        /* MCLR/VPP编程电压端口 */
    uint16_t mclr_pin;              /* MCLR/VPP编程电压引脚 */

    /* 时钟配置 */
    uint32_t speed_hz;              /* 通信速度 */
    uint32_t tick_ns;               /* 定时器分辨率(ns) */
    uint32_t prescaler;             /* 定时器预分频 */
    uint32_t period;                /* 定时器周期 */

    /* 芯片信息 */
    uint32_t device_id;             /* 设备ID */
    uint16_t flash_size;            /* Flash大小(字) */
    uint16_t eeprom_size;           /* EEPROM大小(字节) */
    uint8_t  family_id;            /* 家族ID */

    /* 状态 */
    uint8_t initialized;             /* 初始化标志 */
    ICSP_State_TypeDef state;       /* 当前状态 */
} ICSP_HandleTypeDef;

/* ICSP外部变量声明 */
extern ICSP_HandleTypeDef g_icsp_handle;

/*============================================================================*
 * ISP (AVR系列) 定义
 *============================================================================*/

/* ISP状态码 */
#define ISP_OK               0x00
#define ISP_ERR              0x01
#define ISP_ERR_TIMEOUT      0x02
#define ISP_ERR_NACK         0x03
#define ISP_ERR_VERIFY       0x04
#define ISP_ERR_PARAM        0x05
#define ISP_ERR_NO_TARGET    0x06

/* ISP时钟频率选项 (SPI时钟,通常为1/4目标频率) */
#define ISP_CLOCK_100KHZ     100000
#define ISP_CLOCK_250KHZ     250000
#define ISP_CLOCK_500KHZ     500000
#define ISP_CLOCK_1MHZ       1000000
#define ISP_CLOCK_2MHZ       2000000
#define ISP_CLOCK_4MHZ       4000000
#define ISP_CLOCK_8MHZ       8000000

#define ISP_DEFAULT_CLOCK    ISP_CLOCK_1MHZ

/* ISP命令定义 - AVR系列 */
#define ISP_CMD_PROGRAM_ENABLE     0xAC    /* 编程使能 */
#define ISP_CMD_CHIP_ERASE         0x80    /* 芯片擦除 */
#define ISP_CMD_READ_LOCK_BITS     0x58    /* 读取锁定位 */
#define ISP_CMD_WRITE_LOCK_BITS    0xAC    /* 写锁定位 */
#define ISP_CMD_READ_SIGNATURE     0x30    /* 读取签名 */
#define ISP_CMD_READ_CALIB         0x38    /* 读取校准值 */
#define ISP_CMD_LOAD_EXT_ADDR      0x4D    /* 加载扩展地址 */
#define ISP_CMD_PROGRAM_EXT_ADDR   0x4D    /* 编程扩展地址 */

#define ISP_CMD_WRITE_PROG_MEM     0x40    /* 写程序存储器(低字节) */
#define ISP_CMD_WRITE_PROG_MEM_LPM 0x44    /* 写程序存储器(低字节,使用LPM) */
#define ISP_CMD_READ_PROG_MEM      0x20    /* 读程序存储器(低字节) */
#define ISP_CMD_READ_PROG_MEM_LPM  0x24    /* 读程序存储器(低字节,使用LPM) */

#define ISP_CMD_WRITE_PROG_MEM_HIGH 0x48   /* 写程序存储器高字节 */
#define ISP_CMD_READ_PROG_MEM_HIGH  0x28   /* 读程序存储器高字节 */

#define ISP_CMD_WRITE_EEPROM       0xC0    /* 写EEPROM */
#define ISP_CMD_READ_EEPROM        0xA0    /* 读EEPROM */

#define ISP_CMD_WRITE_FUSE_BITS    0xAC    /* 写熔丝位 */
#define ISP_CMD_READ_FUSE_BITS     0x50    /* 读熔丝位 */
#define ISP_CMD_WRITE_FUSE_HIGH    0xAC    /* 写高熔丝位 */
#define ISP_CMD_READ_FUSE_HIGH     0x58    /* 读高熔丝位 */
#define ISP_CMD_WRITE_EXT_FUSE     0xAC    /* 写扩展熔丝位 */
#define ISP_CMD_READ_EXT_FUSE      0x50    /* 读扩展熔丝位 */

/* ISP编程模式 */
#define ISP_MODE_PROGRAM     0x00
#define ISP_MODE_VERIFY      0x01
#define ISP_MODE_ERASE       0x02

/* ISP状态机状态 */
typedef enum {
    ISP_STATE_IDLE = 0,
    ISP_STATE_ENABLED,
    ISP_STATE_ERASING,
    ISP_STATE_PROGRAMMING,
    ISP_STATE_VERIFYING,
    ISP_STATE_READING,
    ISP_STATE_ERROR
} ISP_State_TypeDef;

/* ISP配置结构体 - 使用硬件SPI */
typedef struct {
    /* SPI GPIO配置 */
    GPIO_TypeDef* sck_port;         /* SCK时钟端口 */
    uint16_t sck_pin;               /* SCK时钟引脚 */
    GPIO_TypeDef* mosi_port;        /* MOSI数据输出端口 */
    uint16_t mosi_pin;              /* MOSI数据输出引脚 */
    GPIO_TypeDef* miso_port;        /* MISO数据输入端口 */
    uint16_t miso_pin;              /* MISO数据输入引脚 */
    GPIO_TypeDef* rst_port;         /* RESET端口 */
    uint16_t rst_pin;               /* RESET引脚 */

    /* 硬件SPI句柄 (可选,也可以使用软件模拟) */
    SPI_HandleTypeDef* hspi;        /* 硬件SPI句柄 */
    uint8_t use_hardware_spi;       /* 使用硬件SPI标志 */

    /* 时钟配置 */
    uint32_t speed_hz;              /* SPI通信速度 */
    uint32_t tick_ns;               /* 定时器分辨率(ns) */

    /* 芯片信息 */
    uint32_t signature;             /* 芯片签名 */
    uint16_t flash_size;            /* Flash大小(字) */
    uint16_t page_size;              /* 页大小(字) */
    uint16_t eeprom_size;           /* EEPROM大小(字节) */
    uint8_t  lock_bits;             /* 锁定位 */
    uint8_t  fuse_bits;             /* 熔丝位 */

    /* 状态 */
    uint8_t initialized;            /* 初始化标志 */
    ISP_State_TypeDef state;        /* 当前状态 */
} ISP_HandleTypeDef;

/* ISP外部变量声明 */
extern ISP_HandleTypeDef g_isp_handle;

/*============================================================================*
 * ICSP (PIC) 函数声明
 *============================================================================*/

/* 初始化和反初始化 */
HAL_StatusTypeDef ICSP_Init(ICSP_HandleTypeDef* hicsp);
HAL_StatusTypeDef ICSP_DeInit(ICSP_HandleTypeDef* hicsp);

/* 进入/退出编程模式 */
HAL_StatusTypeDef ICSP_EnterProgramming(ICSP_HandleTypeDef* hicsp);
HAL_StatusTypeDef ICSP_ExitProgramming(ICSP_HandleTypeDef* hicsp);

/* 核心通信函数 */
HAL_StatusTypeDef ICSP_SendCommand(ICSP_HandleTypeDef* hicsp, uint8_t cmd);
HAL_StatusTypeDef ICSP_SendCommandWithDelay(ICSP_HandleTypeDef* hicsp, uint8_t cmd, uint32_t delay_us);
HAL_StatusTypeDef ICSP_WriteData(ICSP_HandleTypeDef* hicsp, uint16_t data);
uint16_t ICSP_ReadData(ICSP_HandleTypeDef* hicsp);

/* 地址操作 */
HAL_StatusTypeDef ICSP_IncrementAddress(ICSP_HandleTypeDef* hicsp);
HAL_StatusTypeDef ICSP_ResetAddress(ICSP_HandleTypeDef* hicsp);
HAL_StatusTypeDef ICSP_SetAddress(ICSP_HandleTypeDef* hicsp, uint32_t addr);

/* 编程操作 */
HAL_StatusTypeDef ICSP_BulkErase(ICSP_HandleTypeDef* hicsp);
HAL_StatusTypeDef ICSP_RowErase(ICSP_HandleTypeDef* hicsp, uint32_t addr);
HAL_StatusTypeDef ICSP_EraseFlash(ICSP_HandleTypeDef* hicsp, uint32_t addr, uint32_t len);
HAL_StatusTypeDef ICSP_WriteFlash(ICSP_HandleTypeDef* hicsp, uint32_t addr, uint8_t* data, uint32_t len);
HAL_StatusTypeDef ICSP_ReadFlash(ICSP_HandleTypeDef* hicsp, uint32_t addr, uint8_t* data, uint32_t len);
HAL_StatusTypeDef ICSP_WriteEEPROM(ICSP_HandleTypeDef* hicsp, uint16_t addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef ICSP_ReadEEPROM(ICSP_HandleTypeDef* hicsp, uint16_t addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef ICSP_WriteConfig(ICSP_HandleTypeDef* hicsp, uint16_t addr, uint16_t data);
HAL_StatusTypeDef ICSP_ReadConfig(ICSP_HandleTypeDef* hicsp, uint16_t addr, uint16_t* data);

/* 芯片操作 */
uint32_t ICSP_ReadDeviceID(ICSP_HandleTypeDef* hicsp);
uint16_t ICSP_ReadBandGap(ICSP_HandleTypeDef* hicsp);
HAL_StatusTypeDef ICSP_VerifyMemory(ICSP_HandleTypeDef* hicsp, uint32_t addr, uint8_t* data, uint32_t len);

/* 配置函数 */
void ICSP_SetSpeed(ICSP_HandleTypeDef* hicsp, uint32_t speed_hz);
uint32_t ICSP_GetSpeed(ICSP_HandleTypeDef* hicsp);

/* 延时函数 */
void ICSP_DelayNs(ICSP_HandleTypeDef* hicsp, uint32_t ns);
void ICSP_DelayUs(ICSP_HandleTypeDef* hicsp, uint32_t us);

/* GPIO操作 (使用GPIO_Soft) */
void ICSP_GPIO_Init(ICSP_HandleTypeDef* hicsp);
void ICSP_GPIO_DeInit(ICSP_HandleTypeDef* hicsp);

/*============================================================================*
 * ISP (AVR) 函数声明
 *============================================================================*/

/* 初始化和反初始化 */
HAL_StatusTypeDef ISP_Init(ISP_HandleTypeDef* hisp);
HAL_StatusTypeDef ISP_DeInit(ISP_HandleTypeDef* hisp);

/* 进入/退出编程模式 */
HAL_StatusTypeDef ISP_EnterProgramming(ISP_HandleTypeDef* hisp);
HAL_StatusTypeDef ISP_ExitProgramming(ISP_HandleTypeDef* hisp);

/* 核心SPI通信 */
uint8_t ISP_TransferByte(ISP_HandleTypeDef* hisp, uint8_t data);
HAL_StatusTypeDef ISP_WaitReady(ISP_HandleTypeDef* hisp, uint32_t timeout_ms);

/* 编程使能 */
HAL_StatusTypeDef ISP_EnableProgramming(ISP_HandleTypeDef* hisp);
HAL_StatusTypeDef ISP_DisableProgramming(ISP_HandleTypeDef* hisp);

/* 擦除操作 */
HAL_StatusTypeDef ISP_ChipErase(ISP_HandleTypeDef* hisp);
HAL_StatusTypeDef ISP_EraseFlash(ISP_HandleTypeDef* hisp, uint32_t addr, uint32_t len);
HAL_StatusTypeDef ISP_EraseEEPROM(ISP_HandleTypeDef* hisp);

/* Flash操作 */
HAL_StatusTypeDef ISP_WriteFlashWord(ISP_HandleTypeDef* hisp, uint32_t addr, uint16_t data);
HAL_StatusTypeDef ISP_WriteFlashPage(ISP_HandleTypeDef* hisp, uint32_t addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef ISP_ReadFlashWord(ISP_HandleTypeDef* hisp, uint32_t addr, uint16_t* data);
HAL_StatusTypeDef ISP_ReadFlash(ISP_HandleTypeDef* hisp, uint32_t addr, uint8_t* data, uint32_t len);
HAL_StatusTypeDef ISP_WriteFlash(ISP_HandleTypeDef* hisp, uint32_t addr, uint8_t* data, uint32_t len);

/* EEPROM操作 */
HAL_StatusTypeDef ISP_WriteEEPROM(ISP_HandleTypeDef* hisp, uint16_t addr, uint8_t data);
HAL_StatusTypeDef ISP_ReadEEPROM(ISP_HandleTypeDef* hisp, uint16_t addr, uint8_t* data);
HAL_StatusTypeDef ISP_WriteEEPROMBlock(ISP_HandleTypeDef* hisp, uint16_t addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef ISP_ReadEEPROMBlock(ISP_HandleTypeDef* hisp, uint16_t addr, uint8_t* data, uint16_t len);

/* 熔丝位和锁定位操作 */
HAL_StatusTypeDef ISP_ReadFuseBits(ISP_HandleTypeDef* hisp, uint8_t* fuses);
HAL_StatusTypeDef ISP_WriteFuseBits(ISP_HandleTypeDef* hisp, uint8_t fuses);
HAL_StatusTypeDef ISP_ReadLockBits(ISP_HandleTypeDef* hisp, uint8_t* locks);
HAL_StatusTypeDef ISP_WriteLockBits(ISP_HandleTypeDef* hisp, uint8_t locks);
HAL_StatusTypeDef ISP_ReadCalibration(ISP_HandleTypeDef* hisp, uint8_t* cal);

/* 芯片信息读取 */
uint32_t ISP_ReadSignature(ISP_HandleTypeDef* hisp);
uint32_t ISP_GetDeviceID(ISP_HandleTypeDef* hisp);

/* 配置函数 */
void ISP_SetSpeed(ISP_HandleTypeDef* hisp, uint32_t speed_hz);
uint32_t ISP_GetSpeed(ISP_HandleTypeDef* hisp);

/* GPIO操作 */
void ISP_GPIO_Init(ISP_HandleTypeDef* hisp);
void ISP_GPIO_DeInit(ISP_HandleTypeDef* hisp);

/* 软件模拟SPI函数 (当不使用硬件SPI时) */
uint8_t ISP_SoftTransferByte(ISP_HandleTypeDef* hisp, uint8_t data);
HAL_StatusTypeDef ISP_SoftInit(ISP_HandleTypeDef* hisp);

#ifdef __cplusplus
}
#endif

#endif /* __ICSP_H */
