/**
 ******************************************************************************
 * @file    chip_driver.c
 * @brief   芯片驱动抽象层实现
 ******************************************************************************
 */

#include "chip_driver.h"
#include "dap.h"
#include <string.h>
#include <stdlib.h>

// 全局变量
Chip_Info_TypeDef g_chip_info;
Chip_Ops_TypeDef g_chip_ops;
Chip_Flash_Ops_TypeDef g_chip_flash_ops;

// S32K1芯片ID映射表
typedef struct {
    uint32_t id;
    char name[32];
    uint32_t flash_size;
    uint32_t ram_size;
    uint32_t sector_size;
} S32K1_Chip_Entry;

static const S32K1_Chip_Entry s32k1_chips[] = {
    {0x01480040, "S32K148", 1024 * 1024, 176 * 1024, 4 * 1024},
    {0x01460040, "S32K146", 512 * 1024, 128 * 1024, 4 * 1024},
    {0x01440040, "S32K144", 256 * 1024, 64 * 1024, 4 * 1024},
    {0x01420040, "S32K142", 128 * 1024, 32 * 1024, 4 * 1024},
    {0x01180040, "S32K118", 256 * 1024, 32 * 1024, 2 * 1024},
    {0x01160040, "S32K116", 128 * 1024, 24 * 1024, 2 * 1024},
    {0, "", 0, 0, 0}
};

// S32K3芯片ID映射表
typedef struct {
    uint32_t id;
    char name[32];
    uint32_t flash_size;
    uint32_t ram_size;
    uint32_t sector_size;
} S32K3_Chip_Entry;

static const S32K3_Chip_Entry s32k3_chips[] = {
    {0x38420040, "S32K388", 8192 * 1024, 768 * 1024, 32 * 1024},
    {0x38410040, "S32K348", 4096 * 1024, 512 * 1024, 32 * 1024},
    {0x38400040, "S32K344", 2048 * 1024, 320 * 1024, 32 * 1024},
    {0x383F0040, "S32K342", 1536 * 1024, 256 * 1024, 32 * 1024},
    {0x383E0040, "S32K324", 1536 * 1024, 256 * 1024, 32 * 1024},
    {0x383D0040, "S32K314", 1024 * 1024, 192 * 1024, 32 * 1024},
    {0, "", 0, 0, 0}
};

// S32K1内部辅助函数
static HAL_StatusTypeDef S32K1_Wait_FSTAT_CCIF(void)
{
    uint8_t fstat;
    uint32_t timeout = 100000;
    
    do {
        DAP_ReadWord(FTFC_FSTAT_ADDR, (uint32_t*)&fstat);
        if (fstat & FSTAT_CCIF) {
            return HAL_OK;
        }
        timeout--;
    } while (timeout > 0);
    
    return HAL_TIMEOUT;
}

static HAL_StatusTypeDef S32K1_Clear_Error_Flags(void)
{
    uint8_t fstat = FSTAT_ACCERR | FSTAT_FPVIOL;
    return DAP_WriteWord(FTFC_FSTAT_ADDR, (uint32_t)fstat);
}

// S32K1检测函数
HAL_StatusTypeDef S32K1_Detect(Chip_Info_TypeDef *info)
{
    uint32_t chip_id;
    uint32_t i;

    // 读取芯片ID
    DAP_ReadWord(S32K1_CHIP_ID_ADDR, &chip_id);

    // 查找芯片表
    for (i = 0; s32k1_chips[i].id != 0; i++) {
        if (s32k1_chips[i].id == chip_id) {
            // 找到匹配的芯片
            strcpy(info->name, s32k1_chips[i].name);
            strcpy(info->vendor, "NXP");
            strcpy(info->family, "S32K1");
            info->chip_id = chip_id;
            info->flash_size = s32k1_chips[i].flash_size;
            info->ram_size = s32k1_chips[i].ram_size;
            info->flash_base = S32K1_FLASH_BASE;
            info->flash_sector_size = s32k1_chips[i].sector_size;
            info->flash_sector_count = info->flash_size / info->flash_sector_size;
            info->ram_base = S32K1_SRAM_BASE;
            info->has_eeprom = 1;
            info->eeprom_size = 0;
            info->eeprom_base = 0;
            info->has_option_bytes = 1;
            info->has_security = 1;
            info->debug_interface = 0;

            return HAL_OK;
        }
    }

    // 未找到匹配芯片
    return HAL_ERROR;
}

// S32K1 Flash初始化
HAL_StatusTypeDef S32K1_Flash_Init(void)
{
    // 清除错误标志
    return S32K1_Clear_Error_Flags();
}

// S32K1 Flash擦除扇区
HAL_StatusTypeDef S32K1_Flash_Erase_Sector(uint32_t addr)
{
    HAL_StatusTypeDef status;
    
    // 等待Flash准备好
    status = S32K1_Wait_FSTAT_CCIF();
    if (status != HAL_OK) {
        return status;
    }
    
    // 清除错误标志
    status = S32K1_Clear_Error_Flags();
    if (status != HAL_OK) {
        return status;
    }
    
    // 设置FCCOB寄存器
    DAP_WriteWord(FTFC_FCCOB0_ADDR, FTFC_CMD_ERS1SEC);
    DAP_WriteWord(FTFC_FCCOB1_ADDR, (addr >> 16) & 0xFF);
    DAP_WriteWord(FTFC_FCCOB2_ADDR, (addr >> 8) & 0xFF);
    DAP_WriteWord(FTFC_FCCOB3_ADDR, addr & 0xFF);
    
    // 启动命令
    DAP_WriteWord(FTFC_FSTAT_ADDR, FSTAT_CCIF);
    
    // 等待完成
    status = S32K1_Wait_FSTAT_CCIF();
    if (status != HAL_OK) {
        return status;
    }
    
    // 检查错误
    uint8_t fstat;
    DAP_ReadWord(FTFC_FSTAT_ADDR, (uint32_t*)&fstat);
    if (fstat & (FSTAT_ACCERR | FSTAT_FPVIOL | FSTAT_MGSTAT0)) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

// S32K1 Flash写入
HAL_StatusTypeDef S32K1_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t i;
    uint32_t write_addr;
    HAL_StatusTypeDef status;
    uint32_t ph_size = 4; // 按4字节处理
    
    for (i = 0; i < size; i += ph_size) {
        write_addr = addr + i;
        
        // 等待Flash准备好
        status = S32K1_Wait_FSTAT_CCIF();
        if (status != HAL_OK) {
            return status;
        }
        
        // 清除错误标志
        status = S32K1_Clear_Error_Flags();
        if (status != HAL_OK) {
            return status;
        }
        
        // 设置FCCOB寄存器
        DAP_WriteWord(FTFC_FCCOB0_ADDR, FTFC_CMD_PGM1WORD);
        DAP_WriteWord(FTFC_FCCOB1_ADDR, (write_addr >> 16) & 0xFF);
        DAP_WriteWord(FTFC_FCCOB2_ADDR, (write_addr >> 8) & 0xFF);
        DAP_WriteWord(FTFC_FCCOB3_ADDR, write_addr & 0xFF);
        
        // 写入数据
        if (i + 0 < size) DAP_WriteWord(FTFC_FCCOB4_ADDR, data[i + 0]);
        if (i + 1 < size) DAP_WriteWord(FTFC_FCCOB5_ADDR, data[i + 1]);
        if (i + 2 < size) DAP_WriteWord(FTFC_FCCOB6_ADDR, data[i + 2]);
        if (i + 3 < size) DAP_WriteWord(FTFC_FCCOB7_ADDR, data[i + 3]);
        
        // 启动命令
        DAP_WriteWord(FTFC_FSTAT_ADDR, FSTAT_CCIF);
        
        // 等待完成
        status = S32K1_Wait_FSTAT_CCIF();
        if (status != HAL_OK) {
            return status;
        }
        
        // 检查错误
        uint8_t fstat;
        DAP_ReadWord(FTFC_FSTAT_ADDR, (uint32_t*)&fstat);
        if (fstat & (FSTAT_ACCERR | FSTAT_FPVIOL | FSTAT_MGSTAT0)) {
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

// S32K3 Flash操作内部常量
#define S32K3_PFC_BASE          0x402A8000UL
#define S32K3_PFC_MCR_ADDR      (S32K3_PFC_BASE + 0x00)
#define S32K3_PFC_MCR_S5V       (1 << 18)
#define S32K3_PFC_MCR_PGM       (1 << 9)
#define S32K3_PFC_MCR_ERS       (1 << 8)
#define S32K3_PFC_MCR_EHV       (1 << 7)
#define S32K3_PFC_MCR_ACK       (1 << 2)
#define S32K3_PFC_MCR_EER       (1 << 1)
#define S32K3_PFC_MCR_PEG       (1 << 0)

#define S32K3_PFC_PEADR_ADDR    (S32K3_PFC_BASE + 0x10)
#define S32K3_PFC_PDATAL_ADDR   (S32K3_PFC_BASE + 0x40)
#define S32K3_PFC_PDATAH_ADDR   (S32K3_PFC_BASE + 0x44)
#define S32K3_PFC_PEID_ADDR     (S32K3_PFC_BASE + 0xD0)

static HAL_StatusTypeDef S32K3_Wait_PEG(void)
{
    uint32_t mcr;
    uint32_t timeout = 1000000;
    
    do {
        DAP_ReadWord(S32K3_PFC_MCR_ADDR, &mcr);
        if (mcr & S32K3_PFC_MCR_PEG) {
            return HAL_OK;
        }
        timeout--;
    } while (timeout > 0);
    
    return HAL_TIMEOUT;
}

// S32K3检测函数
HAL_StatusTypeDef S32K3_Detect(Chip_Info_TypeDef *info)
{
    uint32_t chip_id;
    uint32_t i;

    // 读取芯片ID
    if (DAP_ReadWord(S32K3_CHIP_ID_ADDR, &chip_id) != HAL_OK) {
        return HAL_ERROR;
    }

    // 查找芯片表
    for (i = 0; s32k3_chips[i].id != 0; i++) {
        if (s32k3_chips[i].id == chip_id) {
            // 找到匹配的芯片
            strcpy(info->name, s32k3_chips[i].name);
            strcpy(info->vendor, "NXP");
            strcpy(info->family, "S32K3");
            info->chip_id = chip_id;
            info->flash_size = s32k3_chips[i].flash_size;
            info->ram_size = s32k3_chips[i].ram_size;
            info->flash_base = S32K3_FLASH_BASE;
            info->flash_sector_size = s32k3_chips[i].sector_size;
            info->flash_sector_count = info->flash_size / info->flash_sector_size;
            info->ram_base = S32K3_SRAM_BASE;
            info->has_eeprom = 1;
            info->eeprom_size = 0;
            info->eeprom_base = 0;
            info->has_option_bytes = 1;
            info->has_security = 1;
            info->debug_interface = 0;

            return HAL_OK;
        }
    }

    // 未找到匹配芯片
    return HAL_ERROR;
}

// S32K3 Flash初始化
HAL_StatusTypeDef S32K3_Flash_Init(void)
{
    // S32K3 Flash模块初始化
    // 清除错误标志
    uint32_t mcr = S32K3_PFC_MCR_EER;
    return DAP_WriteWord(S32K3_PFC_MCR_ADDR, mcr);
}

// S32K3 Flash擦除扇区
HAL_StatusTypeDef S32K3_Flash_Erase_Sector(uint32_t addr)
{
    HAL_StatusTypeDef status;
    uint32_t mcr;
    
    // 验证地址在Flash范围内
    if (addr < S32K3_FLASH_BASE || 
        addr >= (S32K3_FLASH_BASE + g_chip_info.flash_size)) {
        return HAL_ERROR;
    }
    
    // 等待Flash准备好
    status = S32K3_Wait_PEG();
    if (status != HAL_OK) {
        return status;
    }
    
    // 清除错误标志
    DAP_WriteWord(S32K3_PFC_MCR_ADDR, S32K3_PFC_MCR_EER);
    
    // 设置擦除地址
    DAP_WriteWord(S32K3_PFC_PEADR_ADDR, addr);
    
    // 设置擦除命令
    mcr = S32K3_PFC_MCR_ERS | S32K3_PFC_MCR_EHV;
    DAP_WriteWord(S32K3_PFC_MCR_ADDR, mcr);
    
    // 等待完成
    status = S32K3_Wait_PEG();
    if (status != HAL_OK) {
        return status;
    }
    
    // 清除高压
    DAP_ReadWord(S32K3_PFC_MCR_ADDR, &mcr);
    mcr &= ~S32K3_PFC_MCR_EHV;
    DAP_WriteWord(S32K3_PFC_MCR_ADDR, mcr);
    
    // 检查错误
    DAP_ReadWord(S32K3_PFC_MCR_ADDR, &mcr);
    if (mcr & S32K3_PFC_MCR_EER) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

// S32K3 Flash写入
HAL_StatusTypeDef S32K3_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t i;
    uint32_t page_addr;
    uint32_t page_offset;
    uint32_t write_size;
    HAL_StatusTypeDef status;
    uint32_t mcr;
    
    // 验证地址在Flash范围内
    if (addr < S32K3_FLASH_BASE || 
        (addr + size) > (S32K3_FLASH_BASE + g_chip_info.flash_size)) {
        return HAL_ERROR;
    }
    
    // 按页写入（假设页大小256字节）
    for (i = 0; i < size; ) {
        page_addr = addr + i;
        page_offset = page_addr % S32K3_PAGE_SIZE;
        write_size = S32K3_PAGE_SIZE - page_offset;
        
        if (write_size > (size - i)) {
            write_size = size - i;
        }
        
        // 等待Flash准备好
        status = S32K3_Wait_PEG();
        if (status != HAL_OK) {
            return status;
        }
        
        // 清除错误标志
        DAP_WriteWord(S32K3_PFC_MCR_ADDR, S32K3_PFC_MCR_EER);
        
        // 写入数据到数据缓冲区（一次写8字节）
        for (uint32_t j = 0; j < write_size; j += 8) {
            uint32_t data_low = 0;
            uint32_t data_high = 0;
            
            for (uint32_t k = 0; k < 4 && (j + k) < write_size; k++) {
                data_low |= (uint32_t)data[i + j + k] << (k * 8);
            }
            for (uint32_t k = 4; k < 8 && (j + k) < write_size; k++) {
                data_high |= (uint32_t)data[i + j + k] << ((k - 4) * 8);
            }
            
            DAP_WriteWord(S32K3_PFC_PDATAL_ADDR, data_low);
            DAP_WriteWord(S32K3_PFC_PDATAH_ADDR, data_high);
        }
        
        // 设置编程地址
        DAP_WriteWord(S32K3_PFC_PEADR_ADDR, page_addr);
        
        // 设置编程命令
        mcr = S32K3_PFC_MCR_PGM | S32K3_PFC_MCR_EHV;
        DAP_WriteWord(S32K3_PFC_MCR_ADDR, mcr);
        
        // 等待完成
        status = S32K3_Wait_PEG();
        if (status != HAL_OK) {
            return status;
        }
        
        // 清除高压
        DAP_ReadWord(S32K3_PFC_MCR_ADDR, &mcr);
        mcr &= ~S32K3_PFC_MCR_EHV;
        DAP_WriteWord(S32K3_PFC_MCR_ADDR, mcr);
        
        // 检查错误
        DAP_ReadWord(S32K3_PFC_MCR_ADDR, &mcr);
        if (mcr & S32K3_PFC_MCR_EER) {
            return HAL_ERROR;
        }
        
        i += write_size;
    }
    
    return HAL_OK;
}

// S32K3 Flash读取
HAL_StatusTypeDef S32K3_Flash_Read(uint32_t addr, uint8_t *data, uint32_t size)
{
    // S32K3 Flash可以直接内存映射读取
    return DAP_ReadMem(addr, data, size);
}

// 通用芯片初始化
HAL_StatusTypeDef Chip_Init(void)
{
    HAL_StatusTypeDef status;

    // 初始化内核抽象层
    status = Core_Init();
    if (status != HAL_OK) {
        return status;
    }

    // 检测芯片
    status = Chip_Detect(&g_chip_info);
    if (status != HAL_OK) {
        return status;
    }

    // 初始化Flash操作
    if (g_chip_flash_ops.init) {
        g_chip_flash_ops.init();
    }

    return HAL_OK;
}

HAL_StatusTypeDef Chip_DeInit(void)
{
    if (g_chip_flash_ops.deinit) {
        g_chip_flash_ops.deinit();
    }
    Core_DeInit();
    memset(&g_chip_info, 0, sizeof(g_chip_info));
    return HAL_OK;
}

HAL_StatusTypeDef Chip_Detect(Chip_Info_TypeDef *info)
{
    HAL_StatusTypeDef status;

    // 尝试检测S32K1
    status = S32K1_Detect(info);
    if (status == HAL_OK) {
        // 设置S32K1操作函数
        g_chip_ops.detect = S32K1_Detect;
        g_chip_ops.reset = Core_Reset;
        g_chip_ops.halt = Core_Halt;
        g_chip_ops.resume = Core_Resume;
        g_chip_ops.unlock = NULL;
        g_chip_ops.lock = NULL;
        g_chip_ops.read_chip_id = NULL;

        g_chip_flash_ops.init = S32K1_Flash_Init;
        g_chip_flash_ops.deinit = NULL;
        g_chip_flash_ops.erase_chip = NULL;
        g_chip_flash_ops.erase_sector = S32K1_Flash_Erase_Sector;
        g_chip_flash_ops.erase_range = NULL;
        g_chip_flash_ops.write = S32K1_Flash_Write;
        g_chip_flash_ops.read = DAP_ReadMem;
        g_chip_flash_ops.verify = NULL;

        return HAL_OK;
    }

    // 尝试检测S32K3
    status = S32K3_Detect(info);
    if (status == HAL_OK) {
        // 设置S32K3操作函数
        g_chip_ops.detect = S32K3_Detect;
        g_chip_ops.reset = Core_Reset;
        g_chip_ops.halt = Core_Halt;
        g_chip_ops.resume = Core_Resume;
        g_chip_ops.unlock = NULL;
        g_chip_ops.lock = NULL;
        g_chip_ops.read_chip_id = NULL;

        g_chip_flash_ops.init = S32K3_Flash_Init;
        g_chip_flash_ops.deinit = NULL;
        g_chip_flash_ops.erase_chip = NULL;
        g_chip_flash_ops.erase_sector = S32K3_Flash_Erase_Sector;
        g_chip_flash_ops.erase_range = NULL;
        g_chip_flash_ops.write = S32K3_Flash_Write;
        g_chip_flash_ops.read = S32K3_Flash_Read;
        g_chip_flash_ops.verify = NULL;

        return HAL_OK;
    }

    // 未知芯片
    return HAL_ERROR;
}

HAL_StatusTypeDef Chip_Reset(void)
{
    if (g_chip_ops.reset) {
        return g_chip_ops.reset();
    }
    return Core_Reset();
}

HAL_StatusTypeDef Chip_Halt(void)
{
    if (g_chip_ops.halt) {
        return g_chip_ops.halt();
    }
    return Core_Halt();
}

HAL_StatusTypeDef Chip_Resume(void)
{
    if (g_chip_ops.resume) {
        return g_chip_ops.resume();
    }
    return Core_Resume();
}

HAL_StatusTypeDef Chip_Unlock(void)
{
    if (g_chip_ops.unlock) {
        return g_chip_ops.unlock();
    }
    return HAL_OK;
}

HAL_StatusTypeDef Chip_Lock(void)
{
    if (g_chip_ops.lock) {
        return g_chip_ops.lock();
    }
    return HAL_OK;
}

HAL_StatusTypeDef Chip_Flash_Read(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (g_chip_flash_ops.read) {
        return g_chip_flash_ops.read(addr, data, size);
    }
    return Core_ReadMemory(addr, data, size);
}

HAL_StatusTypeDef Chip_Flash_Write(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (g_chip_flash_ops.write) {
        return g_chip_flash_ops.write(addr, data, size);
    }
    return Core_WriteMemory(addr, data, size);
}

HAL_StatusTypeDef Chip_Flash_Erase(uint32_t addr, uint32_t size)
{
    if (g_chip_flash_ops.erase_range) {
        return g_chip_flash_ops.erase_range(addr, size);
    }

    // 如果没有范围擦除函数，逐个扇区擦除
    uint32_t sector_addr;
    uint32_t end_addr = addr + size;
    HAL_StatusTypeDef status;

    for (sector_addr = addr; sector_addr < end_addr; sector_addr += g_chip_info.flash_sector_size) {
        if (g_chip_flash_ops.erase_sector) {
            status = g_chip_flash_ops.erase_sector(sector_addr);
            if (status != HAL_OK) {
                return status;
            }
        } else {
            // 默认使用S32K1扇区擦除
            status = S32K1_Flash_Erase_Sector(sector_addr);
            if (status != HAL_OK) {
                return status;
            }
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef Chip_Flash_Erase_Chip(void)
{
    if (g_chip_flash_ops.erase_chip) {
        return g_chip_flash_ops.erase_chip();
    }
    return Chip_Flash_Erase(g_chip_info.flash_base, g_chip_info.flash_size);
}

HAL_StatusTypeDef Chip_Flash_Verify(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint8_t *read_buf;
    uint32_t i;
    HAL_StatusTypeDef status;

    // 分配临时缓冲区
    read_buf = (uint8_t *)malloc(size);
    if (read_buf == NULL) {
        return HAL_ERROR;
    }

    // 读取Flash
    status = Chip_Flash_Read(addr, read_buf, size);
    if (status != HAL_OK) {
        free(read_buf);
        return status;
    }

    // 比较数据
    for (i = 0; i < size; i++) {
        if (read_buf[i] != data[i]) {
            free(read_buf);
            return HAL_ERROR;
        }
    }

    free(read_buf);
    return HAL_OK;
}

// 读取芯片ID
HAL_StatusTypeDef Chip_ReadChipID(uint32_t *id)
{
    if (g_chip_ops.read_chip_id) {
        return g_chip_ops.read_chip_id(id);
    }
    
    // 默认实现 - 从芯片信息中获取
    if (g_chip_info.chip_id != 0) {
        *id = g_chip_info.chip_id;
        return HAL_OK;
    }
    
    // 尝试重新检测芯片
    Chip_Info_TypeDef temp_info;
    if (Chip_Detect(&temp_info) == HAL_OK) {
        *id = temp_info.chip_id;
        return HAL_OK;
    }
    
    return HAL_ERROR;
}
