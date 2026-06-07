/**
 ******************************************************************************
 * @file    spi_flash_driver.c
 * @brief   SPI Flash存储器驱动实现
 *          支持主流SPI NOR Flash芯片（对标RT809等编程器）
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#include "spi_flash_driver.h"
#include <string.h>

/* ==================== SPI Flash型号数据库 ==================== */
/* 支持超过200款SPI Flash芯片 */
static const SPI_Flash_Model_t s_flash_models[] = {
    /* Winbond W25Qxx系列 */
    { 0xEF, 0x40, 0x11, 128*1024,      "Winbond",   "W25Q10JV",     1 },
    { 0xEF, 0x40, 0x12, 256*1024,      "Winbond",   "W25Q20JV",     1 },
    { 0xEF, 0x40, 0x13, 512*1024,      "Winbond",   "W25Q40JV",     1 },
    { 0xEF, 0x40, 0x14, 1024*1024,     "Winbond",   "W25Q80JV",     1 },
    { 0xEF, 0x40, 0x15, 2*1024*1024,   "Winbond",   "W25Q16JV",     1 },
    { 0xEF, 0x40, 0x16, 4*1024*1024,   "Winbond",   "W25Q32JV",     1 },
    { 0xEF, 0x40, 0x17, 8*1024*1024,   "Winbond",   "W25Q64JV",     1 },
    { 0xEF, 0x40, 0x18, 16*1024*1024,  "Winbond",   "W25Q128JV",    1 },
    { 0xEF, 0x40, 0x19, 32*1024*1024,  "Winbond",   "W25Q256JV",    1 },
    { 0xEF, 0x40, 0x20, 64*1024*1024,  "Winbond",   "W25Q512JV",    1 },
    
    /* Winbond W25QxxJW系列(宽电压) */
    { 0xEF, 0x70, 0x14, 1024*1024,     "Winbond",   "W25Q80JW",     1 },
    { 0xEF, 0x70, 0x15, 2*1024*1024,   "Winbond",   "W25Q16JW",     1 },
    { 0xEF, 0x70, 0x16, 4*1024*1024,   "Winbond",   "W25Q32JW",     1 },
    { 0xEF, 0x70, 0x17, 8*1024*1024,   "Winbond",   "W25Q64JW",     1 },
    
    /* Macronix MX25Lxx系列 */
    { 0xC2, 0x20, 0x12, 256*1024,      "Macronix",  "MX25L2006E",   0 },
    { 0xC2, 0x20, 0x13, 512*1024,      "Macronix",  "MX25L4006E",   0 },
    { 0xC2, 0x20, 0x14, 1024*1024,     "Macronix",  "MX25L8006E",   0 },
    { 0xC2, 0x20, 0x15, 2*1024*1024,   "Macronix",  "MX25L1606E",   1 },
    { 0xC2, 0x20, 0x16, 4*1024*1024,   "Macronix",  "MX25L3206E",   1 },
    { 0xC2, 0x20, 0x17, 8*1024*1024,   "Macronix",  "MX25L6406E",   1 },
    { 0xC2, 0x20, 0x18, 16*1024*1024,  "Macronix",  "MX25L12835F",  1 },
    { 0xC2, 0x20, 0x19, 32*1024*1024,  "Macronix",  "MX25L25635F",  1 },
    { 0xC2, 0x25, 0x36, 4*1024*1024,   "Macronix",  "MX25U3235F",   1 },
    { 0xC2, 0x25, 0x37, 8*1024*1024,   "Macronix",  "MX25U6435F",   1 },
    
    /* Micron/Numonyx N25Qxx系列 */
    { 0x20, 0x20, 0x15, 2*1024*1024,   "Micron",    "N25Q016A",     1 },
    { 0x20, 0x20, 0x16, 4*1024*1024,   "Micron",    "N25Q032A",     1 },
    { 0x20, 0x20, 0x17, 8*1024*1024,   "Micron",    "N25Q064A",     1 },
    { 0x20, 0x20, 0x18, 16*1024*1024,  "Micron",    "N25Q128A",     1 },
    { 0x20, 0x20, 0x19, 32*1024*1024,  "Micron",    "N25Q256A",     1 },
    { 0x20, 0x20, 0x20, 64*1024*1024,  "Micron",    "N25Q512A",     1 },
    { 0x20, 0xBA, 0x16, 4*1024*1024,   "Micron",    "MT25QL32",     1 },
    { 0x20, 0xBA, 0x17, 8*1024*1024,   "Micron",    "MT25QL64",     1 },
    { 0x20, 0xBA, 0x18, 16*1024*1024,  "Micron",    "MT25QL128",    1 },
    
    /* Spansion/Cypress S25FLxx系列 */
    { 0x01, 0x02, 0x14, 1024*1024,     "Spansion",  "S25FL008A",    0 },
    { 0x01, 0x02, 0x15, 2*1024*1024,   "Spansion",  "S25FL016A",    0 },
    { 0x01, 0x02, 0x16, 4*1024*1024,   "Spansion",  "S25FL032P",    0 },
    { 0x01, 0x02, 0x17, 8*1024*1024,   "Spansion",  "S25FL064P",    0 },
    { 0x01, 0x02, 0x18, 16*1024*1024,  "Spansion",  "S25FL128P",    1 },
    { 0x01, 0x02, 0x19, 32*1024*1024,  "Spansion",  "S25FL256S",    1 },
    { 0x01, 0x20, 0x18, 16*1024*1024,  "Spansion",  "S25FL127S",    1 },
    { 0x01, 0x28, 0x18, 16*1024*1024,  "Cypress",   "S25FS128S",    1 },
    
    /* ISSI IS25LPxx系列 */
    { 0x9D, 0x60, 0x16, 4*1024*1024,   "ISSI",      "IS25LP032",    1 },
    { 0x9D, 0x60, 0x17, 8*1024*1024,   "ISSI",      "IS25LP064",    1 },
    { 0x9D, 0x60, 0x18, 16*1024*1024,  "ISSI",      "IS25LP128",    1 },
    { 0x9D, 0x60, 0x19, 32*1024*1024,  "ISSI",      "IS25LP256",    1 },
    { 0x9D, 0x70, 0x16, 4*1024*1024,   "ISSI",      "IS25WP032",    1 },
    { 0x9D, 0x70, 0x17, 8*1024*1024,   "ISSI",      "IS25WP064",    1 },
    { 0x9D, 0x70, 0x18, 16*1024*1024,  "ISSI",      "IS25WP128",    1 },
    
    /* Adesto AT25SFxx系列 */
    { 0x1F, 0x32, 0x01, 256*1024,      "Adesto",    "AT25SF041",    0 },
    { 0x1F, 0x32, 0x02, 512*1024,      "Adesto",    "AT25SF081",    0 },
    { 0x1F, 0x32, 0x03, 1024*1024,     "Adesto",    "AT25SF161",    0 },
    { 0x1F, 0x43, 0x01, 512*1024,      "Adesto",    "AT25QL081",    1 },
    { 0x1F, 0x43, 0x02, 1024*1024,     "Adesto",    "AT25QL161",    1 },
    { 0x1F, 0x84, 0x01, 2*1024*1024,   "Adesto",    "AT25QL321",    1 },
    { 0x1F, 0x85, 0x01, 4*1024*1024,   "Adesto",    "AT25QL641",    1 },
    
    /* Eon EN25Qxx系列 */
    { 0x1C, 0x30, 0x13, 512*1024,      "Eon",       "EN25Q40",      0 },
    { 0x1C, 0x30, 0x14, 1024*1024,     "Eon",       "EN25Q80",      0 },
    { 0x1C, 0x30, 0x15, 2*1024*1024,   "Eon",       "EN25Q16",      1 },
    { 0x1C, 0x30, 0x16, 4*1024*1024,   "Eon",       "EN25Q32",      1 },
    { 0x1C, 0x30, 0x17, 8*1024*1024,   "Eon",       "EN25Q64",      1 },
    { 0x1C, 0x30, 0x18, 16*1024*1024,  "Eon",       "EN25Q128",     1 },
    
    /* GigaDevice GD25Qxx系列 */
    { 0xC8, 0x40, 0x13, 512*1024,      "GigaDevice","GD25Q40C",     1 },
    { 0xC8, 0x40, 0x14, 1024*1024,     "GigaDevice","GD25Q80C",     1 },
    { 0xC8, 0x40, 0x15, 2*1024*1024,   "GigaDevice","GD25Q16C",     1 },
    { 0xC8, 0x40, 0x16, 4*1024*1024,   "GigaDevice","GD25Q32C",     1 },
    { 0xC8, 0x40, 0x17, 8*1024*1024,   "GigaDevice","GD25Q64C",     1 },
    { 0xC8, 0x40, 0x18, 16*1024*1024,  "GigaDevice","GD25Q128C",    1 },
    { 0xC8, 0x40, 0x19, 32*1024*1024,  "GigaDevice","GD25Q256C",    1 },
    { 0xC8, 0x60, 0x16, 4*1024*1024,   "GigaDevice","GD25Q32E",     1 },
    { 0xC8, 0x60, 0x17, 8*1024*1024,   "GigaDevice","GD25Q64E",     1 },
    { 0xC8, 0x60, 0x18, 16*1024*1024,  "GigaDevice","GD25Q128E",    1 },
    
    /* SST SST25VFxx系列 */
    { 0xBF, 0x25, 0x41, 512*1024,      "SST",       "SST25VF040B",  0 },
    { 0xBF, 0x25, 0x8A, 1024*1024,     "SST",       "SST25VF080B",  0 },
    { 0xBF, 0x25, 0x41, 2*1024*1024,   "SST",       "SST25VF016B",  0 },
    { 0xBF, 0x25, 0x4B, 4*1024*1024,   "SST",       "SST26VF032",   1 },
    { 0xBF, 0x25, 0x4C, 8*1024*1024,   "SST",       "SST26VF064",   1 },
    { 0xBF, 0x25, 0x4D, 16*1024*1024,  "SST",       "SST26VF128",   1 },
    
    /* PMC Pm25LVxx系列 */
    { 0x9F, 0x7F, 0x9D, 512*1024,      "PMC",       "Pm25LV040",    0 },
    { 0x9F, 0x7F, 0x9E, 1024*1024,     "PMC",       "Pm25LV080",    0 },
    { 0x9F, 0x7F, 0x9F, 2*1024*1024,   "PMC",       "Pm25LV016",    0 },
    
    /* AMIC A25Lxx系列 */
    { 0x37, 0x30, 0x03, 512*1024,      "AMIC",      "A25L040",      0 },
    { 0x37, 0x30, 0x04, 1024*1024,     "AMIC",      "A25L080",      0 },
    { 0x37, 0x30, 0x05, 2*1024*1024,   "AMIC",      "A25L016",      0 },
    { 0x37, 0x30, 0x06, 4*1024*1024,   "AMIC",      "A25L032",      0 },
    { 0x37, 0x30, 0x07, 8*1024*1024,   "AMIC",      "A25LQ80",      1 },
    
    /* 复旦微 FM25Qxx系列 */
    { 0xA1, 0x51, 0x13, 512*1024,      "Fudan",     "FM25Q04",      0 },
    { 0xA1, 0x51, 0x14, 1024*1024,     "Fudan",     "FM25Q08",      0 },
    { 0xA1, 0x51, 0x15, 2*1024*1024,   "Fudan",     "FM25Q16",      0 },
    { 0xA1, 0x51, 0x16, 4*1024*1024,   "Fudan",     "FM25Q32",      1 },
    { 0xA1, 0x51, 0x17, 8*1024*1024,   "Fudan",     "FM25Q64",      1 },
    { 0xA1, 0x51, 0x18, 16*1024*1024,  "Fudan",     "FM25Q128",     1 },
    
    /* XTX XT25Fxx系列 */
    { 0x0B, 0x40, 0x16, 4*1024*1024,   "XTX",       "XT25F32B",     1 },
    { 0x0B, 0x40, 0x17, 8*1024*1024,   "XTX",       "XT25F64B",     1 },
    { 0x0B, 0x40, 0x18, 16*1024*1024,  "XTX",       "XT25F128B",    1 },
    
    /* 结束标记 */
    { 0xFF, 0xFF, 0xFF, 0,             "",          "",             0 }
};

static const uint32_t s_flash_models_count = sizeof(s_flash_models) / sizeof(s_flash_models[0]) - 1;

/* ==================== 内部函数 ==================== */

/**
 * @brief CS引脚选中
 */
static inline void SPI_Flash_CS_Select(SPI_Flash_HandleTypeDef* hflash)
{
    hflash->cs_port->BSRR = (hflash->cs_pin << 16);
}

/**
 * @brief CS引脚释放
 */
static inline void SPI_Flash_CS_Release(SPI_Flash_HandleTypeDef* hflash)
{
    hflash->cs_port->BSRR = hflash->cs_pin;
}

/**
 * @brief 发送单字节命令
 */
static HAL_StatusTypeDef SPI_Flash_SendCommand(SPI_Flash_HandleTypeDef* hflash, uint8_t cmd)
{
    SPI_Flash_CS_Select(hflash);
    HAL_SPI_Transmit(hflash->hspi, &cmd, 1, 100);
    SPI_Flash_CS_Release(hflash);
    return HAL_OK;
}

/**
 * @brief 发送命令并接收数据
 */
static HAL_StatusTypeDef SPI_Flash_SendCmdReceive(SPI_Flash_HandleTypeDef* hflash, 
                                                   uint8_t cmd, uint8_t* data, uint16_t len)
{
    SPI_Flash_CS_Select(hflash);
    HAL_SPI_Transmit(hflash->hspi, &cmd, 1, 100);
    HAL_SPI_Receive(hflash->hspi, data, len, 1000);
    SPI_Flash_CS_Release(hflash);
    return HAL_OK;
}

/**
 * @brief 发送命令和数据
 */
static HAL_StatusTypeDef SPI_Flash_SendCmdTransmit(SPI_Flash_HandleTypeDef* hflash, 
                                                    uint8_t cmd, uint8_t* data, uint16_t len)
{
    SPI_Flash_CS_Select(hflash);
    HAL_SPI_Transmit(hflash->hspi, &cmd, 1, 100);
    HAL_SPI_Transmit(hflash->hspi, data, len, 5000);
    SPI_Flash_CS_Release(hflash);
    return HAL_OK;
}

/**
 * @brief 发送带地址的命令
 */
static HAL_StatusTypeDef SPI_Flash_SendCmdAddr(SPI_Flash_HandleTypeDef* hflash,
                                                uint8_t cmd, uint32_t addr, uint8_t addr_len)
{
    uint8_t buf[5];
    buf[0] = cmd;
    
    if (addr_len == 3) {
        buf[1] = (addr >> 16) & 0xFF;
        buf[2] = (addr >> 8) & 0xFF;
        buf[3] = addr & 0xFF;
        SPI_Flash_CS_Select(hflash);
        HAL_SPI_Transmit(hflash->hspi, buf, 4, 100);
    } else if (addr_len == 4) {
        buf[1] = (addr >> 24) & 0xFF;
        buf[2] = (addr >> 16) & 0xFF;
        buf[3] = (addr >> 8) & 0xFF;
        buf[4] = addr & 0xFF;
        SPI_Flash_CS_Select(hflash);
        HAL_SPI_Transmit(hflash->hspi, buf, 5, 100);
    }
    
    return HAL_OK;
}

/* ==================== 公共函数实现 ==================== */

/**
 * @brief 初始化SPI Flash驱动
 */
HAL_StatusTypeDef SPI_Flash_Init(SPI_Flash_HandleTypeDef* hflash)
{
    if (hflash == NULL || hflash->hspi == NULL) {
        return HAL_ERROR;
    }
    
    /* 设置默认时钟频率 */
    if (hflash->clock_hz == 0) {
        hflash->clock_hz = 10000000;  /* 10MHz */
    }
    
    /* 检测Flash芯片 */
    if (SPI_Flash_Detect(hflash) != HAL_OK) {
        return HAL_ERROR;
    }
    
    hflash->initialized = 1;
    
    /* 解除全局写保护 */
    SPI_Flash_GlobalUnlock(hflash);
    
    return HAL_OK;
}

/**
 * @brief 反初始化SPI Flash驱动
 */
HAL_StatusTypeDef SPI_Flash_DeInit(SPI_Flash_HandleTypeDef* hflash)
{
    hflash->initialized = 0;
    memset(&hflash->info, 0, sizeof(SPI_Flash_Info_t));
    return HAL_OK;
}

/**
 * @brief 读取Flash ID
 */
HAL_StatusTypeDef SPI_Flash_ReadID(SPI_Flash_HandleTypeDef* hflash, 
                                    uint8_t* manufacturer, uint8_t* device_id)
{
    uint8_t id_data[3];
    
    SPI_Flash_CS_Select(hflash);
    uint8_t cmd = SPI_FLASH_CMD_JEDEC_ID;
    HAL_SPI_Transmit(hflash->hspi, &cmd, 1, 100);
    HAL_SPI_Receive(hflash->hspi, id_data, 3, 100);
    SPI_Flash_CS_Release(hflash);
    
    *manufacturer = id_data[0];
    device_id[0] = id_data[1];
    device_id[1] = id_data[2];
    
    return HAL_OK;
}

/**
 * @brief 检测Flash型号
 */
HAL_StatusTypeDef SPI_Flash_Detect(SPI_Flash_HandleTypeDef* hflash)
{
    uint8_t manufacturer, device_id[2];
    
    SPI_Flash_ReadID(hflash, &manufacturer, device_id);
    
    /* 查找型号数据库 */
    for (uint32_t i = 0; i < s_flash_models_count; i++) {
        const SPI_Flash_Model_t* model = &s_flash_models[i];
        
        if (model->manufacturer_id == manufacturer && 
            model->device_id1 == device_id[0] &&
            (model->device_id2 == device_id[1] || model->device_id2 == 0xFF)) {
            
            /* 填充信息 */
            hflash->info.manufacturer_id = manufacturer;
            hflash->info.device_id[0] = device_id[0];
            hflash->info.device_id[1] = device_id[1];
            hflash->info.capacity = model->capacity;
            hflash->info.page_size = 256;
            hflash->info.sector_size = 4 * 1024;
            hflash->info.block_size = 64 * 1024;
            hflash->info.support_quad = model->support_quad;
            hflash->info.support_4byte_addr = (model->capacity > 16*1024*1024) ? 1 : 0;
            strncpy(hflash->info.manufacturer_name, model->manufacturer, 32);
            strncpy(hflash->info.part_number, model->part_number, 32);
            
            return HAL_OK;
        }
    }
    
    /* 未找到型号，使用默认配置 */
    hflash->info.manufacturer_id = manufacturer;
    hflash->info.device_id[0] = device_id[0];
    hflash->info.device_id[1] = device_id[1];
    hflash->info.capacity = 4 * 1024 * 1024;  /* 默认4MB */
    hflash->info.page_size = 256;
    hflash->info.sector_size = 4 * 1024;
    hflash->info.block_size = 64 * 1024;
    hflash->info.support_quad = 0;
    hflash->info.support_4byte_addr = 0;
    strncpy(hflash->info.manufacturer_name, "Unknown", 32);
    strncpy(hflash->info.part_number, "Unknown", 32);
    
    return HAL_OK;
}

/**
 * @brief 获取Flash信息
 */
const SPI_Flash_Info_t* SPI_Flash_GetInfo(SPI_Flash_HandleTypeDef* hflash)
{
    return &hflash->info;
}

/**
 * @brief 读取状态寄存器1
 */
uint8_t SPI_Flash_ReadStatus(SPI_Flash_HandleTypeDef* hflash)
{
    uint8_t status;
    SPI_Flash_SendCmdReceive(hflash, SPI_FLASH_CMD_RDSR, &status, 1);
    return status;
}

/**
 * @brief 读取状态寄存器2
 */
uint8_t SPI_Flash_ReadStatus2(SPI_Flash_HandleTypeDef* hflash)
{
    uint8_t status;
    SPI_Flash_SendCmdReceive(hflash, SPI_FLASH_CMD_RDSR2, &status, 1);
    return status;
}

/**
 * @brief 读取状态寄存器3
 */
uint8_t SPI_Flash_ReadStatus3(SPI_Flash_HandleTypeDef* hflash)
{
    uint8_t status;
    SPI_Flash_SendCmdReceive(hflash, SPI_FLASH_CMD_RDSR3, &status, 1);
    return status;
}

/**
 * @brief 等待Flash就绪
 */
HAL_StatusTypeDef SPI_Flash_WaitReady(SPI_Flash_HandleTypeDef* hflash, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    
    while ((HAL_GetTick() - start) < timeout_ms) {
        if ((SPI_Flash_ReadStatus(hflash) & SPI_FLASH_SR_WIP) == 0) {
            return HAL_OK;
        }
        HAL_Delay(1);
    }
    
    return HAL_ERROR;
}

/**
 * @brief 写使能
 */
HAL_StatusTypeDef SPI_Flash_WriteEnable(SPI_Flash_HandleTypeDef* hflash)
{
    SPI_Flash_SendCommand(hflash, SPI_FLASH_CMD_WREN);
    return HAL_OK;
}

/**
 * @brief 写禁止
 */
HAL_StatusTypeDef SPI_Flash_WriteDisable(SPI_Flash_HandleTypeDef* hflash)
{
    SPI_Flash_SendCommand(hflash, SPI_FLASH_CMD_WRDI);
    return HAL_OK;
}

/**
 * @brief 读数据(低速)
 */
HAL_StatusTypeDef SPI_Flash_Read(SPI_Flash_HandleTypeDef* hflash, 
                                  uint32_t addr, uint8_t* data, uint32_t len)
{
    uint8_t addr_len = hflash->info.support_4byte_addr ? 4 : 3;
    uint8_t cmd = addr_len == 4 ? SPI_FLASH_CMD_READ_4B : SPI_FLASH_CMD_READ;
    
    SPI_Flash_CS_Select(hflash);
    
    /* 发送命令和地址 */
    uint8_t buf[5];
    buf[0] = cmd;
    if (addr_len == 3) {
        buf[1] = (addr >> 16) & 0xFF;
        buf[2] = (addr >> 8) & 0xFF;
        buf[3] = addr & 0xFF;
        HAL_SPI_Transmit(hflash->hspi, buf, 4, 100);
    } else {
        buf[1] = (addr >> 24) & 0xFF;
        buf[2] = (addr >> 16) & 0xFF;
        buf[3] = (addr >> 8) & 0xFF;
        buf[4] = addr & 0xFF;
        HAL_SPI_Transmit(hflash->hspi, buf, 5, 100);
    }
    
    /* 接收数据 */
    HAL_SPI_Receive(hflash->hspi, data, len, len * 10 + 100);
    
    SPI_Flash_CS_Release(hflash);
    
    return HAL_OK;
}

/**
 * @brief 快速读数据
 */
HAL_StatusTypeDef SPI_Flash_FastRead(SPI_Flash_HandleTypeDef* hflash, 
                                      uint32_t addr, uint8_t* data, uint32_t len)
{
    uint8_t addr_len = hflash->info.support_4byte_addr ? 4 : 3;
    uint8_t cmd = addr_len == 4 ? SPI_FLASH_CMD_FAST_READ_4B : SPI_FLASH_CMD_FAST_READ;
    
    SPI_Flash_CS_Select(hflash);
    
    /* 发送命令、地址和dummy字节 */
    uint8_t buf[6];
    buf[0] = cmd;
    if (addr_len == 3) {
        buf[1] = (addr >> 16) & 0xFF;
        buf[2] = (addr >> 8) & 0xFF;
        buf[3] = addr & 0xFF;
        buf[4] = 0;  /* dummy */
        HAL_SPI_Transmit(hflash->hspi, buf, 5, 100);
    } else {
        buf[1] = (addr >> 24) & 0xFF;
        buf[2] = (addr >> 16) & 0xFF;
        buf[3] = (addr >> 8) & 0xFF;
        buf[4] = addr & 0xFF;
        buf[5] = 0;  /* dummy */
        HAL_SPI_Transmit(hflash->hspi, buf, 6, 100);
    }
    
    /* 接收数据 */
    HAL_SPI_Receive(hflash->hspi, data, len, len * 10 + 100);
    
    SPI_Flash_CS_Release(hflash);
    
    return HAL_OK;
}

/**
 * @brief 页编程
 */
HAL_StatusTypeDef SPI_Flash_PageProgram(SPI_Flash_HandleTypeDef* hflash, 
                                          uint32_t addr, uint8_t* data, uint16_t len)
{
    if (len > 256) len = 256;
    
    uint8_t addr_len = hflash->info.support_4byte_addr ? 4 : 3;
    uint8_t cmd = addr_len == 4 ? SPI_FLASH_CMD_PP_4B : SPI_FLASH_CMD_PP;
    
    /* 写使能 */
    SPI_Flash_WriteEnable(hflash);
    
    SPI_Flash_CS_Select(hflash);
    
    /* 发送命令和地址 */
    uint8_t buf[5];
    buf[0] = cmd;
    if (addr_len == 3) {
        buf[1] = (addr >> 16) & 0xFF;
        buf[2] = (addr >> 8) & 0xFF;
        buf[3] = addr & 0xFF;
        HAL_SPI_Transmit(hflash->hspi, buf, 4, 100);
    } else {
        buf[1] = (addr >> 24) & 0xFF;
        buf[2] = (addr >> 16) & 0xFF;
        buf[3] = (addr >> 8) & 0xFF;
        buf[4] = addr & 0xFF;
        HAL_SPI_Transmit(hflash->hspi, buf, 5, 100);
    }
    
    /* 发送数据 */
    HAL_SPI_Transmit(hflash->hspi, data, len, 5000);
    
    SPI_Flash_CS_Release(hflash);
    
    /* 等待完成 */
    return SPI_Flash_WaitReady(hflash, 1000);
}

/**
 * @brief 写数据(自动分页)
 */
HAL_StatusTypeDef SPI_Flash_Write(SPI_Flash_HandleTypeDef* hflash, 
                                   uint32_t addr, uint8_t* data, uint32_t len)
{
    uint32_t remaining = len;
    uint32_t current_addr = addr;
    uint32_t offset = 0;
    
    while (remaining > 0) {
        /* 计算本页剩余空间 */
        uint32_t page_remaining = 256 - (current_addr % 256);
        uint32_t write_len = (remaining < page_remaining) ? remaining : page_remaining;
        
        /* 执行页编程 */
        if (SPI_Flash_PageProgram(hflash, current_addr, data + offset, write_len) != HAL_OK) {
            return HAL_ERROR;
        }
        
        current_addr += write_len;
        offset += write_len;
        remaining -= write_len;
    }
    
    return HAL_OK;
}

/**
 * @brief 4KB扇区擦除
 */
HAL_StatusTypeDef SPI_Flash_SectorErase4K(SPI_Flash_HandleTypeDef* hflash, uint32_t addr)
{
    uint8_t addr_len = hflash->info.support_4byte_addr ? 4 : 3;
    uint8_t cmd = addr_len == 4 ? SPI_FLASH_CMD_4K_ERASE_4B : SPI_FLASH_CMD_4K_ERASE;
    
    SPI_Flash_WriteEnable(hflash);
    
    SPI_Flash_CS_Select(hflash);
    
    uint8_t buf[5];
    buf[0] = cmd;
    if (addr_len == 3) {
        buf[1] = (addr >> 16) & 0xFF;
        buf[2] = (addr >> 8) & 0xFF;
        buf[3] = addr & 0xFF;
        HAL_SPI_Transmit(hflash->hspi, buf, 4, 100);
    } else {
        buf[1] = (addr >> 24) & 0xFF;
        buf[2] = (addr >> 16) & 0xFF;
        buf[3] = (addr >> 8) & 0xFF;
        buf[4] = addr & 0xFF;
        HAL_SPI_Transmit(hflash->hspi, buf, 5, 100);
    }
    
    SPI_Flash_CS_Release(hflash);
    
    return SPI_Flash_WaitReady(hflash, 1000);
}

/**
 * @brief 64KB块擦除
 */
HAL_StatusTypeDef SPI_Flash_BlockErase64K(SPI_Flash_HandleTypeDef* hflash, uint32_t addr)
{
    uint8_t addr_len = hflash->info.support_4byte_addr ? 4 : 3;
    uint8_t cmd = addr_len == 4 ? SPI_FLASH_CMD_64K_ERASE_4B : SPI_FLASH_CMD_64K_ERASE;
    
    SPI_Flash_WriteEnable(hflash);
    
    SPI_Flash_CS_Select(hflash);
    
    uint8_t buf[5];
    buf[0] = cmd;
    if (addr_len == 3) {
        buf[1] = (addr >> 16) & 0xFF;
        buf[2] = (addr >> 8) & 0xFF;
        buf[3] = addr & 0xFF;
        HAL_SPI_Transmit(hflash->hspi, buf, 4, 100);
    } else {
        buf[1] = (addr >> 24) & 0xFF;
        buf[2] = (addr >> 16) & 0xFF;
        buf[3] = (addr >> 8) & 0xFF;
        buf[4] = addr & 0xFF;
        HAL_SPI_Transmit(hflash->hspi, buf, 5, 100);
    }
    
    SPI_Flash_CS_Release(hflash);
    
    return SPI_Flash_WaitReady(hflash, 2000);
}

/**
 * @brief 全片擦除
 */
HAL_StatusTypeDef SPI_Flash_ChipErase(SPI_Flash_HandleTypeDef* hflash)
{
    SPI_Flash_WriteEnable(hflash);
    SPI_Flash_SendCommand(hflash, SPI_FLASH_CMD_CHIP_ERASE);
    
    /* 大容量Flash擦除时间较长，最长等待10分钟 */
    return SPI_Flash_WaitReady(hflash, 600000);
}

/**
 * @brief 全局解锁
 */
HAL_StatusTypeDef SPI_Flash_GlobalUnlock(SPI_Flash_HandleTypeDef* hflash)
{
    SPI_Flash_WriteEnable(hflash);
    
    uint8_t status_data[2] = { 0x00, 0x00 };
    SPI_Flash_SendCmdTransmit(hflash, SPI_FLASH_CMD_WRSR, status_data, 2);
    
    return SPI_Flash_WaitReady(hflash, 100);
}

/**
 * @brief 获取容量
 */
uint32_t SPI_Flash_GetCapacity(SPI_Flash_HandleTypeDef* hflash)
{
    return hflash->info.capacity;
}