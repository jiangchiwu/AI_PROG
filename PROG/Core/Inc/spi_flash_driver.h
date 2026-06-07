/**
 ******************************************************************************
 * @file    spi_flash_driver.h
 * @brief   SPI Flash存储器驱动头文件
 *          支持主流SPI NOR Flash芯片（对标RT809等编程器）
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 * 
 * @details 本驱动支持以下SPI Flash厂商和系列：
 *          - Winbond (W25Qxx系列)
 *          - Macronix (MX25Lxx系列)
 *          - Micron/Numonyx (M25Pxx/N25Qxx系列)
 *          - Spansion/Cypress (S25FLxx系列)
 *          - ISSI (IS25LPxx系列)
 *          - Adesto (AT25SFxx系列)
 *          - Eon (EN25Qxx系列)
 *          - GigaDevice (GD25Qxx系列)
 *          - PMC (Pm25LVxx系列)
 *          - SST (SST25VFxx系列)
 ******************************************************************************
 */

#ifndef __SPI_FLASH_DRIVER_H__
#define __SPI_FLASH_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* ==================== SPI Flash厂商定义 ==================== */
#define SPI_FLASH_VENDOR_WINBOND      0xEF
#define SPI_FLASH_VENDOR_MACRONIX     0xC2
#define SPI_FLASH_VENDOR_MICRON       0x20
#define SPI_FLASH_VENDOR_SPANSION     0x01
#define SPI_FLASH_VENDOR_ISSI         0x9D
#define SPI_FLASH_VENDOR_ADESTO       0x1F
#define SPI_FLASH_VENDOR_EON          0x1C
#define SPI_FLASH_VENDOR_GIGADEVICE   0xC8
#define SPI_FLASH_VENDOR_PMC          0x9F
#define SPI_FLASH_VENDOR_SST          0xBF
#define SPI_FLASH_VENDOR_AMIC         0x37
#define SPI_FLASH_VENDOR_FUDAN        0xA1
#define SPI_FLASH_VENDOR_XTX          0x0B

/* ==================== SPI Flash命令定义 ==================== */
/* 标准命令 */
#define SPI_FLASH_CMD_WREN            0x06    /* 写使能 */
#define SPI_FLASH_CMD_WRDI            0x04    /* 写禁止 */
#define SPI_FLASH_CMD_RDSR            0x05    /* 读状态寄存器1 */
#define SPI_FLASH_CMD_RDSR2           0x35    /* 读状态寄存器2 */
#define SPI_FLASH_CMD_RDSR3           0x15    /* 读状态寄存器3 */
#define SPI_FLASH_CMD_WRSR            0x01    /* 写状态寄存器 */
#define SPI_FLASH_CMD_WRSR2           0x31    /* 写状态寄存器2 */
#define SPI_FLASH_CMD_WRSR3           0x11    /* 写状态寄存器3 */
#define SPI_FLASH_CMD_READ            0x03    /* 读数据(低速) */
#define SPI_FLASH_CMD_FAST_READ       0x0B    /* 快速读 */
#define SPI_FLASH_CMD_FAST_READ_DUAL  0x3B    /* 双输出快速读 */
#define SPI_FLASH_CMD_FAST_READ_QUAD  0x6B    /* 四输出快速读 */
#define SPI_FLASH_CMD_RDID            0x9F    /* 读JEDEC ID */
#define SPI_FLASH_CMD_REMS            0x90    /* 读厂商/设备ID */
#define SPI_FLASH_CMD_JEDEC_ID        0x9F    /* 读JEDEC ID */
#define SPI_FLASH_CMD_P4E             0x20    /* 4KB扇区擦除 */
#define SPI_FLASH_CMD_P8E             0x40    /* 8KB块擦除 */
#define SPI_FLASH_CMD_BE              0xD8    /* 64KB块擦除 */
#define SPI_FLASH_CMD_CE              0xC7    /* 全片擦除 */
#define SPI_FLASH_CMD_PP              0x02    /* 页编程 */
#define SPI_FLASH_CMD_QPP             0x32    /* 四输入页编程 */
#define SPI_FLASH_CMD_DP              0xB9    /* 深度掉电 */
#define SPI_FLASH_CMD_RDP             0xAB    /* 释放深度掉电 */
#define SPI_FLASH_CMD_RES             0xAB    /* 读电子签名 */

/* 扩展命令 */
#define SPI_FLASH_CMD_4K_ERASE        0x20    /* 4KB扇区擦除 */
#define SPI_FLASH_CMD_32K_ERASE       0x52    /* 32KB块擦除 */
#define SPI_FLASH_CMD_64K_ERASE       0xD8    /* 64KB块擦除 */
#define SPI_FLASH_CMD_CHIP_ERASE      0xC7    /* 全片擦除 */

/* 4字节地址模式命令(大容量Flash) */
#define SPI_FLASH_CMD_READ_4B         0x13    /* 4字节地址读 */
#define SPI_FLASH_CMD_FAST_READ_4B    0x0C    /* 4字节地址快速读 */
#define SPI_FLASH_CMD_PP_4B           0x12    /* 4字节地址页编程 */
#define SPI_FLASH_CMD_4K_ERASE_4B     0x21    /* 4字节地址4KB擦除 */
#define SPI_FLASH_CMD_64K_ERASE_4B    0xDC    /* 4字节地址64KB擦除 */

/* 状态寄存器位定义 */
#define SPI_FLASH_SR_WIP              0x01    /* 写进行中 */
#define SPI_FLASH_SR_WEL              0x02    /* 写使能锁存 */
#define SPI_FLASH_SR_BP0              0x04    /* 块保护位0 */
#define SPI_FLASH_SR_BP1              0x08    /* 块保护位1 */
#define SPI_FLASH_SR_BP2              0x10    /* 块保护位2 */
#define SPI_FLASH_SR_BP3              0x20    /* 块保护位3 */
#define SPI_FLASH_SR_QE               0x40    /* 四通道使能 */
#define SPI_FLASH_SR_SRWD             0x80    /* 状态寄存器写保护 */

/* ==================== SPI Flash信息结构体 ==================== */
typedef struct {
    uint8_t  manufacturer_id;          /* 厂商ID */
    uint8_t  device_id[2];             /* 设备ID(2字节) */
    uint32_t capacity;                 /* 容量(字节) */
    uint32_t page_size;                /* 页大小 */
    uint32_t sector_size;              /* 扇区大小 */
    uint32_t block_size;               /* 块大小 */
    uint8_t  support_quad;             /* 是否支持四通道 */
    uint8_t  support_4byte_addr;       /* 是否支持4字节地址 */
    char     manufacturer_name[32];    /* 厂商名称 */
    char     part_number[32];          /* 型号 */
} SPI_Flash_Info_t;

/* ==================== SPI Flash句柄结构体 ==================== */
typedef struct {
    SPI_HandleTypeDef* hspi;           /* SPI句柄 */
    GPIO_TypeDef*      cs_port;        /* CS引脚端口 */
    uint16_t           cs_pin;         /* CS引脚 */
    SPI_Flash_Info_t   info;           /* Flash信息 */
    uint8_t            initialized;    /* 初始化标志 */
    uint8_t            quad_enabled;   /* 四通道使能标志 */
    uint32_t           clock_hz;       /* 时钟频率 */
} SPI_Flash_HandleTypeDef;

/* ==================== SPI Flash型号表 ==================== */
typedef struct {
    uint8_t  manufacturer_id;
    uint8_t  device_id1;
    uint8_t  device_id2;
    uint32_t capacity;
    char     manufacturer[16];
    char     part_number[24];
    uint8_t  support_quad;
} SPI_Flash_Model_t;

/* ==================== 函数声明 ==================== */

/* 初始化函数 */
HAL_StatusTypeDef SPI_Flash_Init(SPI_Flash_HandleTypeDef* hflash);
HAL_StatusTypeDef SPI_Flash_DeInit(SPI_Flash_HandleTypeDef* hflash);

/* 识别函数 */
HAL_StatusTypeDef SPI_Flash_ReadID(SPI_Flash_HandleTypeDef* hflash, uint8_t* manufacturer, uint8_t* device_id);
HAL_StatusTypeDef SPI_Flash_Detect(SPI_Flash_HandleTypeDef* hflash);
const SPI_Flash_Info_t* SPI_Flash_GetInfo(SPI_Flash_HandleTypeDef* hflash);

/* 状态函数 */
uint8_t SPI_Flash_ReadStatus(SPI_Flash_HandleTypeDef* hflash);
uint8_t SPI_Flash_ReadStatus2(SPI_Flash_HandleTypeDef* hflash);
uint8_t SPI_Flash_ReadStatus3(SPI_Flash_HandleTypeDef* hflash);
HAL_StatusTypeDef SPI_Flash_WriteStatus(SPI_Flash_HandleTypeDef* hflash, uint8_t sr1, uint8_t sr2);
HAL_StatusTypeDef SPI_Flash_WaitReady(SPI_Flash_HandleTypeDef* hflash, uint32_t timeout_ms);
HAL_StatusTypeDef SPI_Flash_WriteEnable(SPI_Flash_HandleTypeDef* hflash);
HAL_StatusTypeDef SPI_Flash_WriteDisable(SPI_Flash_HandleTypeDef* hflash);

/* 读函数 */
HAL_StatusTypeDef SPI_Flash_Read(SPI_Flash_HandleTypeDef* hflash, uint32_t addr, uint8_t* data, uint32_t len);
HAL_StatusTypeDef SPI_Flash_FastRead(SPI_Flash_HandleTypeDef* hflash, uint32_t addr, uint8_t* data, uint32_t len);
HAL_StatusTypeDef SPI_Flash_DualRead(SPI_Flash_HandleTypeDef* hflash, uint32_t addr, uint8_t* data, uint32_t len);
HAL_StatusTypeDef SPI_Flash_QuadRead(SPI_Flash_HandleTypeDef* hflash, uint32_t addr, uint8_t* data, uint32_t len);

/* 写函数 */
HAL_StatusTypeDef SPI_Flash_PageProgram(SPI_Flash_HandleTypeDef* hflash, uint32_t addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef SPI_Flash_Write(SPI_Flash_HandleTypeDef* hflash, uint32_t addr, uint8_t* data, uint32_t len);
HAL_StatusTypeDef SPI_Flash_QuadPageProgram(SPI_Flash_HandleTypeDef* hflash, uint32_t addr, uint8_t* data, uint16_t len);

/* 擦除函数 */
HAL_StatusTypeDef SPI_Flash_SectorErase4K(SPI_Flash_HandleTypeDef* hflash, uint32_t addr);
HAL_StatusTypeDef SPI_Flash_BlockErase32K(SPI_Flash_HandleTypeDef* hflash, uint32_t addr);
HAL_StatusTypeDef SPI_Flash_BlockErase64K(SPI_Flash_HandleTypeDef* hflash, uint32_t addr);
HAL_StatusTypeDef SPI_Flash_ChipErase(SPI_Flash_HandleTypeDef* hflash);

/* 保护函数 */
HAL_StatusTypeDef SPI_Flash_GlobalUnlock(SPI_Flash_HandleTypeDef* hflash);
HAL_StatusTypeDef SPI_Flash_GlobalLock(SPI_Flash_HandleTypeDef* hflash);
HAL_StatusTypeDef SPI_Flash_EnableQuad(SPI_Flash_HandleTypeDef* hflash);

/* 其他函数 */
HAL_StatusTypeDef SPI_Flash_Reset(SPI_Flash_HandleTypeDef* hflash);
HAL_StatusTypeDef SPI_Flash_DeepPowerDown(SPI_Flash_HandleTypeDef* hflash);
HAL_StatusTypeDef SPI_Flash_ReleasePowerDown(SPI_Flash_HandleTypeDef* hflash);
uint32_t SPI_Flash_GetCapacity(SPI_Flash_HandleTypeDef* hflash);

#ifdef __cplusplus
}
#endif

#endif /* __SPI_FLASH_DRIVER_H__ */