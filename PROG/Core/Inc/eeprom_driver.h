/**
 ******************************************************************************
 * @file    eeprom_driver.h
 * @brief   EEPROM驱动头文件
 *          支持24Cxx/I2C EEPROM、93Cxx/SPI EEPROM等系列
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#ifndef __EEPROM_DRIVER_H__
#define __EEPROM_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* ==================== EEPROM厂商定义 ==================== */
#define EEPROM_VENDOR_ATMEL    0x01
#define EEPROM_VENDOR_MICROCHIP 0x02
#define EEPROM_VENDOR_ST       0x03
#define EEPROM_VENDOR_ONSEMI   0x04
#define EEPROM_VENDOR_ROHM     0x05
#define EEPROM_VENDOR_FUDAN    0x06
#define EEPROM_VENDOR_GD       0x07

/* ==================== 24Cxx系列定义 ==================== */
#define EEPROM_24C01           128      /* 128 bytes */
#define EEPROM_24C02           256      /* 256 bytes */
#define EEPROM_24C04           512      /* 512 bytes */
#define EEPROM_24C08           1024     /* 1KB */
#define EEPROM_24C16           2048     /* 2KB */
#define EEPROM_24C32           4096     /* 4KB */
#define EEPROM_24C64           8192     /* 8KB */
#define EEPROM_24C128          16384    /* 16KB */
#define EEPROM_24C256          32768    /* 32KB */
#define EEPROM_24C512          65536    /* 64KB */
#define EEPROM_24C1024         131072   /* 128KB */
#define EEPROM_24M01           262144   /* 256KB */
#define EEPROM_24M02           524288   /* 512KB */

/* ==================== 93Cxx系列定义 ==================== */
#define EEPROM_93C46           64       /* 64x16 bits */
#define EEPROM_93C56           128      /* 128x16 bits */
#define EEPROM_EEPROM_93C66    256      /* 256x16 bits */
#define EEPROM_93C76           512      /* 512x16 bits */
#define EEPROM_93C86           1024     /* 1024x16 bits */

/* ==================== EEPROM信息结构体 ==================== */
typedef struct {
    uint8_t  manufacturer;          /* 厂商ID */
    char     part_number[16];       /* 型号 */
    uint32_t capacity;              /* 容量(bytes) */
    uint32_t page_size;             /* 页大小 */
    uint8_t  addr_width;            /* 地址宽度 */
    uint8_t  bus_type;              /* 总线类型(I2C/SPI/MICROWIRE) */
    uint32_t write_cycle_time_us;   /* 写周期时间 */
} EEPROM_Info_t;

/* ==================== I2C EEPROM句柄 ==================== */
typedef struct {
    I2C_HandleTypeDef* hi2c;        /* I2C句柄 */
    uint8_t            dev_addr;    /* 设备地址(7位) */
    EEPROM_Info_t      info;        /* EEPROM信息 */
    uint32_t           timeout_ms;  /* 超时时间 */
    uint8_t            initialized; /* 初始化标志 */
} EEPROM_I2C_HandleTypeDef;

/* ==================== SPI EEPROM句柄 ==================== */
typedef struct {
    SPI_HandleTypeDef* hspi;        /* SPI句柄 */
    GPIO_TypeDef*      cs_port;     /* CS引脚端口 */
    uint16_t           cs_pin;      /* CS引脚 */
    EEPROM_Info_t      info;        /* EEPROM信息 */
    uint8_t            initialized; /* 初始化标志 */
} EEPROM_SPI_HandleTypeDef;

/* ==================== 93Cxx MICROWIRE EEPROM句柄 ==================== */
typedef struct {
    GPIO_TypeDef*      clk_port;    /* CLK引脚端口 */
    uint16_t           clk_pin;     /* CLK引脚 */
    GPIO_TypeDef*      data_in_port;/* DI引脚端口 */
    uint16_t           data_in_pin; /* DI引脚 */
    GPIO_TypeDef*      data_out_port;/* DO引脚端口 */
    uint16_t           data_out_pin;/* DO引脚 */
    GPIO_TypeDef*      cs_port;     /* CS引脚端口 */
    uint16_t           cs_pin;      /* CS引脚 */
    EEPROM_Info_t      info;        /* EEPROM信息 */
    uint8_t            initialized; /* 初始化标志 */
} EEPROM_MICROWIRE_HandleTypeDef;

/* ==================== I2C EEPROM函数 ==================== */

/* 初始化 */
HAL_StatusTypeDef EEPROM_I2C_Init(EEPROM_I2C_HandleTypeDef* heeprom);
HAL_StatusTypeDef EEPROM_I2C_DeInit(EEPROM_I2C_HandleTypeDef* heeprom);

/* 检测 */
HAL_StatusTypeDef EEPROM_I2C_Detect(EEPROM_I2C_HandleTypeDef* heeprom);

/* 读写 */
HAL_StatusTypeDef EEPROM_I2C_ReadByte(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data);
HAL_StatusTypeDef EEPROM_I2C_WriteByte(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t data);
HAL_StatusTypeDef EEPROM_I2C_ReadPage(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef EEPROM_I2C_WritePage(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint16_t len);
HAL_StatusTypeDef EEPROM_I2C_Read(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint32_t len);
HAL_StatusTypeDef EEPROM_I2C_Write(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint32_t len);

/* 擦除 */
HAL_StatusTypeDef EEPROM_I2C_Erase(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint32_t len);
HAL_StatusTypeDef EEPROM_I2C_Fill(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint32_t len, uint8_t value);

/* 验证 */
HAL_StatusTypeDef EEPROM_I2C_Verify(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint32_t len);
HAL_StatusTypeDef EEPROM_I2C_BlankCheck(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint32_t len);

/* ==================== SPI EEPROM函数 ==================== */

/* 初始化 */
HAL_StatusTypeDef EEPROM_SPI_Init(EEPROM_SPI_HandleTypeDef* heeprom);
HAL_StatusTypeDef EEPROM_SPI_DeInit(EEPROM_SPI_HandleTypeDef* heeprom);

/* 检测 */
HAL_StatusTypeDef EEPROM_SPI_Detect(EEPROM_SPI_HandleTypeDef* heeprom);

/* 读写 */
HAL_StatusTypeDef EEPROM_SPI_ReadByte(EEPROM_SPI_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data);
HAL_StatusTypeDef EEPROM_SPI_WriteByte(EEPROM_SPI_HandleTypeDef* heeprom, uint16_t addr, uint8_t data);
HAL_StatusTypeDef EEPROM_SPI_Read(EEPROM_SPI_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint32_t len);
HAL_StatusTypeDef EEPROM_SPI_Write(EEPROM_SPI_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint32_t len);

/* 写保护 */
HAL_StatusTypeDef EEPROM_SPI_WriteEnable(EEPROM_SPI_HandleTypeDef* heeprom);
HAL_StatusTypeDef EEPROM_SPI_WriteDisable(EEPROM_SPI_HandleTypeDef* heeprom);

/* 擦除 */
HAL_StatusTypeDef EEPROM_SPI_Erase(EEPROM_SPI_HandleTypeDef* heeprom, uint16_t addr, uint32_t len);

/* ==================== MICROWIRE EEPROM函数 ==================== */

/* 初始化 */
HAL_StatusTypeDef EEPROM_MICROWIRE_Init(EEPROM_MICROWIRE_HandleTypeDef* heeprom);
HAL_StatusTypeDef EEPROM_MICROWIRE_DeInit(EEPROM_MICROWIRE_HandleTypeDef* heeprom);

/* 读写(16位字) */
HAL_StatusTypeDef EEPROM_MICROWIRE_ReadWord(EEPROM_MICROWIRE_HandleTypeDef* heeprom, uint8_t addr, uint16_t* data);
HAL_StatusTypeDef EEPROM_MICROWIRE_WriteWord(EEPROM_MICROWIRE_HandleTypeDef* heeprom, uint8_t addr, uint16_t data);
HAL_StatusTypeDef EEPROM_MICROWIRE_ReadAll(EEPROM_MICROWIRE_HandleTypeDef* heeprom, uint16_t* data);
HAL_StatusTypeDef EEPROM_MICROWIRE_WriteAll(EEPROM_MICROWIRE_HandleTypeDef* heeprom, uint16_t* data);

/* 擦除 */
HAL_StatusTypeDef EEPROM_MICROWIRE_EraseAll(EEPROM_MICROWIRE_HandleTypeDef* heeprom);

#ifdef __cplusplus
}
#endif

#endif /* __EEPROM_DRIVER_H__ */