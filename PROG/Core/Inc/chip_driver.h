/**
 ******************************************************************************
 * @file    chip_driver.h
 * @brief   芯片驱动抽象层头文件
 ******************************************************************************
 */

#ifndef __CHIP_DRIVER_H__
#define __CHIP_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "core_cal.h"

// 芯片信息结构
typedef struct {
    char name[64];
    char vendor[32];
    char family[32];
    uint32_t chip_id;
    uint32_t flash_size;
    uint32_t ram_size;
    uint32_t flash_base;
    uint32_t flash_sector_size;
    uint32_t flash_sector_count;
    uint32_t ram_base;
    uint8_t has_eeprom;
    uint32_t eeprom_size;
    uint32_t eeprom_base;
    uint8_t has_option_bytes;
    uint8_t has_security;
    uint8_t debug_interface;
} Chip_Info_TypeDef;

// Flash操作结构
typedef struct {
    HAL_StatusTypeDef (*init)(void);
    HAL_StatusTypeDef (*deinit)(void);
    HAL_StatusTypeDef (*erase_chip)(void);
    HAL_StatusTypeDef (*erase_sector)(uint32_t sector);
    HAL_StatusTypeDef (*erase_range)(uint32_t addr, uint32_t size);
    HAL_StatusTypeDef (*write)(uint32_t addr, uint8_t *data, uint32_t size);
    HAL_StatusTypeDef (*read)(uint32_t addr, uint8_t *data, uint32_t size);
    HAL_StatusTypeDef (*verify)(uint32_t addr, uint8_t *data, uint32_t size);
} Chip_Flash_Ops_TypeDef;

// 芯片操作结构
typedef struct {
    HAL_StatusTypeDef (*detect)(Chip_Info_TypeDef *info);
    HAL_StatusTypeDef (*reset)(void);
    HAL_StatusTypeDef (*halt)(void);
    HAL_StatusTypeDef (*resume)(void);
    HAL_StatusTypeDef (*unlock)(void);
    HAL_StatusTypeDef (*lock)(void);
    HAL_StatusTypeDef (*read_chip_id)(uint32_t *id);
} Chip_Ops_TypeDef;

// S32K1系列定义
#define S32K1_CHIP_ID_ADDR    0x40048004UL
#define S32K1_FTFE_BASE       0x40020000UL
#define S32K1_FLASH_BASE      0x00000000UL
#define S32K1_SRAM_BASE       0x1FFFC000UL

// FTFC寄存器地址（S32K1，通过DAP访问目标芯片）
#define FTFC_FSTAT_ADDR      (S32K1_FTFE_BASE + 0x00)
#define FTFC_FCNFG_ADDR      (S32K1_FTFE_BASE + 0x01)
#define FTFC_FSEC_ADDR       (S32K1_FTFE_BASE + 0x02)
#define FTFC_FCLKDIV_ADDR    (S32K1_FTFE_BASE + 0x03)
#define FTFC_FCCOB0_ADDR     (S32K1_FTFE_BASE + 0x04)
#define FTFC_FCCOB1_ADDR     (S32K1_FTFE_BASE + 0x05)
#define FTFC_FCCOB2_ADDR     (S32K1_FTFE_BASE + 0x06)
#define FTFC_FCCOB3_ADDR     (S32K1_FTFE_BASE + 0x07)
#define FTFC_FCCOB4_ADDR     (S32K1_FTFE_BASE + 0x08)
#define FTFC_FCCOB5_ADDR     (S32K1_FTFE_BASE + 0x09)
#define FTFC_FCCOB6_ADDR     (S32K1_FTFE_BASE + 0x0A)
#define FTFC_FCCOB7_ADDR     (S32K1_FTFE_BASE + 0x0B)
#define FTFC_FCCOB8_ADDR     (S32K1_FTFE_BASE + 0x0C)
#define FTFC_FCCOB9_ADDR     (S32K1_FTFE_BASE + 0x0D)
#define FTFC_FCCOBA_ADDR     (S32K1_FTFE_BASE + 0x0E)
#define FTFC_FCCOBB_ADDR     (S32K1_FTFE_BASE + 0x0F)

// FTFC命令
#define FTFC_CMD_RD1BLK      0x00
#define FTFC_CMD_PGM1PHR     0x07
#define FTFC_CMD_ERS1SEC     0x09
#define FTFC_CMD_READ1S      0x01
#define FTFC_CMD_READALLS    0x80
#define FTFC_CMD_PGM1SEC     0x0B
#define FTFC_CMD_CHKERS      0x16
#define FTFC_CMD_PGM1PHS     0x0F
#define FTFC_CMD_PGM1WORD    0x37
#define FTFC_CMD_ERSALL      0x44
#define FTFC_CMD_VFY1SEC     0x30
#define FTFC_CMD_PGM1PHS1    0x63
#define FTFC_CMD_PGM1PHS2    0x65
#define FTFC_CMD_PGM1PHS3    0x67

// FTFC状态位
#define FSTAT_CCIF           (1 << 7)
#define FSTAT_ACCERR         (1 << 5)
#define FSTAT_FPVIOL         (1 << 4)
#define FSTAT_MGSTAT0        (1 << 0)

// S32K3系列定义
#define S32K3_CHIP_ID_ADDR    0x403AC004UL
#define S32K3_FLASH_BASE      0x00400000UL
#define S32K3_SRAM_BASE       0x20400000UL
#define S32K3_PFLASH_BASE     0x00400000UL
#define S32K3_DFLASH_BASE     0x10000000UL

// S32K3 Flash命令
#define S32K3_CMD_ERASE_SECTOR  0x01
#define S32K3_CMD_PROGRAM       0x02
#define S32K3_CMD_READ          0x03
#define S32K3_CMD_ERASE_ALL     0x04
#define S32K3_CMD_SET_BLOCK_LOCK 0x05
#define S32K3_CMD_GET_BLOCK_LOCK 0x06

// S32K3 Flash配置
#define S32K3_SECTOR_SIZE_8K    0x00002000UL
#define S32K3_SECTOR_SIZE_32K   0x00008000UL
#define S32K3_PAGE_SIZE        0x00000200UL

// STM32系列定义
#define STM32_FLASH_BASE        0x08000000UL
#define STM32_SRAM_BASE         0x20000000UL
#define STM32H7_SRAM_BASE       0x24000000UL

// STM32 F1系列扇区大小
#define STM32F1_SECTOR_SIZE_1K  0x00000400UL
#define STM32F1_SECTOR_SIZE_2K  0x00000800UL
#define STM32F1_SECTOR_SIZE_4K  0x00001000UL
#define STM32F1_SECTOR_SIZE_8K  0x00002000UL
#define STM32F1_SECTOR_SIZE_16K 0x00004000UL

// STM32 F4系列扇区大小
#define STM32F4_SECTOR_SIZE_16K 0x00004000UL
#define STM32F4_SECTOR_SIZE_64K 0x00010000UL
#define STM32F4_SECTOR_SIZE_128K 0x00020000UL

// STM32 H7系列扇区大小
#define STM32H7_SECTOR_SIZE_128K 0x00020000UL
#define STM32H7_SECTOR_SIZE_256K 0x00040000UL

// STM32 Flash寄存器地址（通过DAP访问目标芯片）
#define STM32_FLASH_KEYR_ADDR   0x40023C04UL
#define STM32_FLASH_OPTKEYR_ADDR 0x40023C08UL
#define STM32_FLASH_SR_ADDR     0x40023C0CUL
#define STM32_FLASH_CR_ADDR     0x40023C10UL
#define STM32_FLASH_AR_ADDR     0x40023C14UL

// STM32H7 Flash寄存器地址
#define STM32H7_FLASH_CR1_ADDR  0x52002000UL
#define STM32H7_FLASH_CR2_ADDR  0x52002004UL
#define STM32H7_FLASH_CR3_ADDR  0x52002008UL
#define STM32H7_FLASH_SR1_ADDR  0x5200200CUL
#define STM32H7_FLASH_SR2_ADDR  0x52002010UL
#define STM32H7_FLASH_PAR_ADDR  0x52002014UL

// STM32 Flash密钥
#define STM32_FLASH_KEY1        0x45670123UL
#define STM32_FLASH_KEY2        0xCDEF89ABUL

// STM32 Flash状态位
#define STM32_FLASH_SR_BSY      (1 << 16)
#define STM32_FLASH_SR_EOP      (1 << 0)
#define STM32_FLASH_SR_WRPERR   (1 << 4)
#define STM32_FLASH_SR_PGAERR   (1 << 5)
#define STM32_FLASH_SR_PGPERR   (1 << 6)
#define STM32_FLASH_SR_PGSERR   (1 << 7)

// STM32 Flash控制位
#define STM32_FLASH_CR_PG       (1 << 0)
#define STM32_FLASH_CR_PER      (1 << 1)
#define STM32_FLASH_CR_MER      (1 << 2)
#define STM32_FLASH_CR_SER      (1 << 3)
#define STM32_FLASH_CR_BKER     (1 << 11)
#define STM32_FLASH_CR_EOPIE    (1 << 12)
#define STM32_FLASH_CR_LOCK     (1 << 31)

// STM32H7 Flash控制位
#define STM32H7_FLASH_CR1_PG    (1 << 0)
#define STM32H7_FLASH_CR1_SER   (1 << 1)
#define STM32H7_FLASH_CR1_MER   (1 << 2)
#define STM32H7_FLASH_CR1_BSY   (1 << 16)
#define STM32H7_FLASH_CR1_EOP   (1 << 0)

// 函数声明
HAL_StatusTypeDef Chip_Init(void);
HAL_StatusTypeDef Chip_DeInit(void);
HAL_StatusTypeDef Chip_Detect(Chip_Info_TypeDef *info);

HAL_StatusTypeDef Chip_Reset(void);
HAL_StatusTypeDef Chip_Halt(void);
HAL_StatusTypeDef Chip_Resume(void);
HAL_StatusTypeDef Chip_Unlock(void);
HAL_StatusTypeDef Chip_Lock(void);

HAL_StatusTypeDef Chip_Flash_Read(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef Chip_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef Chip_Flash_Erase(uint32_t addr, uint32_t size);
HAL_StatusTypeDef Chip_Flash_Erase_Chip(void);
HAL_StatusTypeDef Chip_Flash_Verify(uint32_t addr, uint8_t *data, uint32_t size);

// S32K系列特定函数
HAL_StatusTypeDef S32K1_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef S32K1_Flash_Init(void);
HAL_StatusTypeDef S32K1_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef S32K1_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef S32K3_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef S32K3_Flash_Init(void);
HAL_StatusTypeDef S32K3_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef S32K3_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef S32K3_Flash_Read(uint32_t addr, uint8_t *data, uint32_t size);

// STM32系列特定函数
HAL_StatusTypeDef STM32F1_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32F1_Flash_Init(void);
HAL_StatusTypeDef STM32F1_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32F1_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef STM32F4_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32F4_Flash_Init(void);
HAL_StatusTypeDef STM32F4_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32F4_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef STM32H7_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32H7_Flash_Init(void);
HAL_StatusTypeDef STM32H7_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32H7_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef STM32F0_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32F0_Flash_Init(void);
HAL_StatusTypeDef STM32F0_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32F0_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef STM32F2_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32F2_Flash_Init(void);
HAL_StatusTypeDef STM32F2_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32F2_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef STM32F3_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32F3_Flash_Init(void);
HAL_StatusTypeDef STM32F3_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32F3_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef STM32F7_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32F7_Flash_Init(void);
HAL_StatusTypeDef STM32F7_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32F7_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef STM32L0_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32L0_Flash_Init(void);
HAL_StatusTypeDef STM32L0_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32L0_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef STM32L1_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32L1_Flash_Init(void);
HAL_StatusTypeDef STM32L1_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32L1_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef STM32L4_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32L4_Flash_Init(void);
HAL_StatusTypeDef STM32L4_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32L4_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef STM32L5_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32L5_Flash_Init(void);
HAL_StatusTypeDef STM32L5_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32L5_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef STM32G0_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32G0_Flash_Init(void);
HAL_StatusTypeDef STM32G0_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32G0_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef STM32G4_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32G4_Flash_Init(void);
HAL_StatusTypeDef STM32G4_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32G4_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef STM32WB_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef STM32WB_Flash_Init(void);
HAL_StatusTypeDef STM32WB_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef STM32WB_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef GD32F1_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef GD32F1_Flash_Init(void);
HAL_StatusTypeDef GD32F1_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef GD32F1_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef GD32F3_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef GD32F3_Flash_Init(void);
HAL_StatusTypeDef GD32F3_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef GD32F3_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef GD32F4_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef GD32F4_Flash_Init(void);
HAL_StatusTypeDef GD32F4_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef GD32F4_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef GD32E2_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef GD32E2_Flash_Init(void);
HAL_StatusTypeDef GD32E2_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef GD32E2_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef GD32E5_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef GD32E5_Flash_Init(void);
HAL_StatusTypeDef GD32E5_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef GD32E5_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef GD32L2_Detect(Chip_Info_TypeDef *info);
HAL_StatusTypeDef GD32L2_Flash_Init(void);
HAL_StatusTypeDef GD32L2_Flash_Erase_Sector(uint32_t addr);
HAL_StatusTypeDef GD32L2_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef Chip_ReadChipID(uint32_t *id);

extern Chip_Info_TypeDef g_chip_info;
extern Chip_Ops_TypeDef g_chip_ops;
extern Chip_Flash_Ops_TypeDef g_chip_flash_ops;

#ifdef __cplusplus
}
#endif

#endif
