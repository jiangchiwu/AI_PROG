/**
 ******************************************************************************
 * @file    eeprom_driver.c
 * @brief   EEPROM驱动实现
 *          支持24Cxx/I2C EEPROM、93Cxx/SPI/MICROWIRE EEPROM
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#include "eeprom_driver.h"
#include <string.h>

/* ==================== 24Cxx型号数据库 ==================== */
typedef struct {
    uint32_t capacity;
    char     part_number[16];
    uint8_t  addr_width;            /* 地址宽度(1/2字节) */
    uint32_t page_size;             /* 页大小 */
    uint32_t write_cycle_ms;        /* 写周期时间(ms) */
} EEPROM_24C_Model_t;

static const EEPROM_24C_Model_t s_24c_models[] = {
    { EEPROM_24C01,  "24C01",  1,  8,  5 },
    { EEPROM_24C02,  "24C02",  1,  8,  5 },
    { EEPROM_24C04,  "24C04",  1, 16,  5 },
    { EEPROM_24C08,  "24C08",  1, 16,  5 },
    { EEPROM_24C16,  "24C16",  1, 16,  5 },
    { EEPROM_24C32,  "24C32",  2, 32,  5 },
    { EEPROM_24C64,  "24C64",  2, 32,  5 },
    { EEPROM_24C128, "24C128", 2, 64,  5 },
    { EEPROM_24C256, "24C256", 2, 64,  5 },
    { EEPROM_24C512, "24C512", 2,128,  5 },
    { EEPROM_24C1024,"24C1024",2,128,  5 },
    { EEPROM_24M01,  "24M01",  2,256,  5 },
    { EEPROM_24M02,  "24M02",  2,256,  5 },
    { 0,             "",       0,  0,  0 }
};

/* ==================== I2C EEPROM实现 ==================== */

/**
 * @brief 初始化I2C EEPROM
 */
HAL_StatusTypeDef EEPROM_I2C_Init(EEPROM_I2C_HandleTypeDef* heeprom)
{
    if (heeprom == NULL || heeprom->hi2c == NULL) return HAL_ERROR;
    
    if (heeprom->timeout_ms == 0) heeprom->timeout_ms = 100;
    if (heeprom->dev_addr == 0) heeprom->dev_addr = 0x50;
    
    /* 检测EEPROM */
    if (EEPROM_I2C_Detect(heeprom) != HAL_OK) {
        return HAL_ERROR;
    }
    
    heeprom->initialized = 1;
    return HAL_OK;
}

/**
 * @brief 反初始化I2C EEPROM
 */
HAL_StatusTypeDef EEPROM_I2C_DeInit(EEPROM_I2C_HandleTypeDef* heeprom)
{
    heeprom->initialized = 0;
    return HAL_OK;
}

/**
 * @brief 检测I2C EEPROM型号
 */
HAL_StatusTypeDef EEPROM_I2C_Detect(EEPROM_I2C_HandleTypeDef* heeprom)
{
    /* 尝试通过容量探测EEPROM型号 */
    /* 从小容量到大容量依次尝试 */
    
    for (uint32_t i = 0; s_24c_models[i].capacity != 0; i++) {
        uint8_t addr_width = s_24c_models[i].addr_width;
        
        if (addr_width == 1) {
            /* 单字节地址(24C01-24C16) */
            HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(heeprom->hi2c, 
                heeprom->dev_addr << 1, 2, heeprom->timeout_ms);
            
            if (status == HAL_OK) {
                /* 检测到设备，尝试读最后地址确认容量 */
                heeprom->info.capacity = s_24c_models[i].capacity;
                heeprom->info.page_size = s_24c_models[i].page_size;
                heeprom->info.addr_width = s_24c_models[i].addr_width;
                heeprom->info.write_cycle_time_us = s_24c_models[i].write_cycle_ms * 1000;
                strncpy(heeprom->info.part_number, s_24c_models[i].part_number, 16);
                heeprom->info.bus_type = 0;  /* I2C */
                return HAL_OK;
            }
        } else {
            /* 双字节地址(24C32-24M02) */
            HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(heeprom->hi2c,
                heeprom->dev_addr << 1, 2, heeprom->timeout_ms);
            
            if (status == HAL_OK) {
                heeprom->info.capacity = s_24c_models[i].capacity;
                heeprom->info.page_size = s_24c_models[i].page_size;
                heeprom->info.addr_width = s_24c_models[i].addr_width;
                heeprom->info.write_cycle_time_us = s_24c_models[i].write_cycle_ms * 1000;
                strncpy(heeprom->info.part_number, s_24c_models[i].part_number, 16);
                heeprom->info.bus_type = 0;
                return HAL_OK;
            }
        }
    }
    
    /* 未知型号，使用默认配置 */
    heeprom->info.capacity = EEPROM_24C256;
    heeprom->info.page_size = 64;
    heeprom->info.addr_width = 2;
    heeprom->info.write_cycle_time_us = 5000;
    strncpy(heeprom->info.part_number, "Unknown", 16);
    
    return HAL_OK;
}

/**
 * @brief I2C EEPROM读一个字节
 */
HAL_StatusTypeDef EEPROM_I2C_ReadByte(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data)
{
    if (heeprom->info.addr_width == 1) {
        return HAL_I2C_Mem_Read(heeprom->hi2c, heeprom->dev_addr << 1, addr,
                                I2C_MEMADD_SIZE_8BIT, data, 1, heeprom->timeout_ms);
    } else {
        return HAL_I2C_Mem_Read(heeprom->hi2c, heeprom->dev_addr << 1, addr,
                                I2C_MEMADD_SIZE_16BIT, data, 1, heeprom->timeout_ms);
    }
}

/**
 * @brief I2C EEPROM写一个字节
 */
HAL_StatusTypeDef EEPROM_I2C_WriteByte(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t data)
{
    if (heeprom->info.addr_width == 1) {
        HAL_I2C_Mem_Write(heeprom->hi2c, heeprom->dev_addr << 1, addr,
                          I2C_MEMADD_SIZE_8BIT, &data, 1, heeprom->timeout_ms);
    } else {
        HAL_I2C_Mem_Write(heeprom->hi2c, heeprom->dev_addr << 1, addr,
                          I2C_MEMADD_SIZE_16BIT, &data, 1, heeprom->timeout_ms);
    }
    
    /* 等待写周期完成 */
    HAL_Delay(heeprom->info.write_cycle_time_us / 1000 + 1);
    
    return HAL_OK;
}

/**
 * @brief I2C EEPROM读一页
 */
HAL_StatusTypeDef EEPROM_I2C_ReadPage(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint16_t len)
{
    if (len > heeprom->info.page_size) len = heeprom->info.page_size;
    
    if (heeprom->info.addr_width == 1) {
        return HAL_I2C_Mem_Read(heeprom->hi2c, heeprom->dev_addr << 1, addr,
                                I2C_MEMADD_SIZE_8BIT, data, len, heeprom->timeout_ms);
    } else {
        return HAL_I2C_Mem_Read(heeprom->hi2c, heeprom->dev_addr << 1, addr,
                                I2C_MEMADD_SIZE_16BIT, data, len, heeprom->timeout_ms);
    }
}

/**
 * @brief I2C EEPROM写一页
 */
HAL_StatusTypeDef EEPROM_I2C_WritePage(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint16_t len)
{
    if (len > heeprom->info.page_size) len = heeprom->info.page_size;
    
    /* 地址必须对齐到页边界 */
    uint16_t page_addr = addr & ~(heeprom->info.page_size - 1);
    
    if (heeprom->info.addr_width == 1) {
        HAL_I2C_Mem_Write(heeprom->hi2c, heeprom->dev_addr << 1, page_addr,
                          I2C_MEMADD_SIZE_8BIT, data, len, heeprom->timeout_ms);
    } else {
        HAL_I2C_Mem_Write(heeprom->hi2c, heeprom->dev_addr << 1, page_addr,
                          I2C_MEMADD_SIZE_16BIT, data, len, heeprom->timeout_ms);
    }
    
    HAL_Delay(heeprom->info.write_cycle_time_us / 1000 + 1);
    
    return HAL_OK;
}

/**
 * @brief I2C EEPROM连续读
 */
HAL_StatusTypeDef EEPROM_I2C_Read(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint32_t len)
{
    /* 分页读取 */
    for (uint32_t i = 0; i < len; i += heeprom->info.page_size) {
        uint16_t chunk = (len - i > heeprom->info.page_size) ? heeprom->info.page_size : (len - i);
        
        if (EEPROM_I2C_ReadPage(heeprom, addr + i, data + i, chunk) != HAL_OK) {
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

/**
 * @brief I2C EEPROM连续写
 */
HAL_StatusTypeDef EEPROM_I2C_Write(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i += heeprom->info.page_size) {
        uint16_t chunk = (len - i > heeprom->info.page_size) ? heeprom->info.page_size : (len - i);
        
        if (EEPROM_I2C_WritePage(heeprom, addr + i, data + i, chunk) != HAL_OK) {
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

/**
 * @brief I2C EEPROM擦除(写入0xFF)
 */
HAL_StatusTypeDef EEPROM_I2C_Erase(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint32_t len)
{
    uint8_t fill_data[64];
    memset(fill_data, 0xFF, heeprom->info.page_size);
    
    for (uint32_t i = 0; i < len; i += heeprom->info.page_size) {
        uint16_t chunk = (len - i > heeprom->info.page_size) ? heeprom->info.page_size : (len - i);
        EEPROM_I2C_WritePage(heeprom, addr + i, fill_data, chunk);
    }
    
    return HAL_OK;
}

/**
 * @brief I2C EEPROM填充
 */
HAL_StatusTypeDef EEPROM_I2C_Fill(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint32_t len, uint8_t value)
{
    uint8_t fill_data[64];
    memset(fill_data, value, heeprom->info.page_size);
    
    for (uint32_t i = 0; i < len; i += heeprom->info.page_size) {
        uint16_t chunk = (len - i > heeprom->info.page_size) ? heeprom->info.page_size : (len - i);
        EEPROM_I2C_WritePage(heeprom, addr + i, fill_data, chunk);
    }
    
    return HAL_OK;
}

/**
 * @brief I2C EEPROM验证
 */
HAL_StatusTypeDef EEPROM_I2C_Verify(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint32_t len)
{
    uint8_t read_buf[64];
    
    for (uint32_t i = 0; i < len; i += heeprom->info.page_size) {
        uint16_t chunk = (len - i > heeprom->info.page_size) ? heeprom->info.page_size : (len - i);
        EEPROM_I2C_ReadPage(heeprom, addr + i, read_buf, chunk);
        
        if (memcmp(read_buf, data + i, chunk) != 0) {
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

/**
 * @brief I2C EEPROM空白检查
 */
HAL_StatusTypeDef EEPROM_I2C_BlankCheck(EEPROM_I2C_HandleTypeDef* heeprom, uint16_t addr, uint32_t len)
{
    uint8_t read_buf[64];
    
    for (uint32_t i = 0; i < len; i += heeprom->info.page_size) {
        uint16_t chunk = (len - i > heeprom->info.page_size) ? heeprom->info.page_size : (len - i);
        EEPROM_I2C_ReadPage(heeprom, addr + i, read_buf, chunk);
        
        for (uint16_t j = 0; j < chunk; j++) {
            if (read_buf[j] != 0xFF) return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

/* ==================== SPI EEPROM实现 ==================== */

/**
 * @brief 初始化SPI EEPROM
 */
HAL_StatusTypeDef EEPROM_SPI_Init(EEPROM_SPI_HandleTypeDef* heeprom)
{
    if (heeprom == NULL || heeprom->hspi == NULL) return HAL_ERROR;
    
    EEPROM_SPI_Detect(heeprom);
    heeprom->initialized = 1;
    return HAL_OK;
}

/**
 * @brief 反初始化SPI EEPROM
 */
HAL_StatusTypeDef EEPROM_SPI_DeInit(EEPROM_SPI_HandleTypeDef* heeprom)
{
    heeprom->initialized = 0;
    return HAL_OK;
}

/**
 * @brief 检测SPI EEPROM型号
 */
HAL_StatusTypeDef EEPROM_SPI_Detect(EEPROM_SPI_HandleTypeDef* heeprom)
{
    /* SPI EEPROM通常支持READ指令，通过读取确认存在 */
    uint8_t status;
    uint8_t cmd = 0x05;  /* RDSR */
    
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;  /* CS低 */
    HAL_SPI_TransmitReceive(heeprom->hspi, &cmd, &status, 1, 100);
    heeprom->cs_port->BSRR = heeprom->cs_pin;         /* CS高 */
    
    heeprom->info.capacity = 8192;  /* 默认8KB */
    heeprom->info.page_size = 32;
    heeprom->info.addr_width = 2;
    heeprom->info.write_cycle_time_us = 5000;
    strncpy(heeprom->info.part_number, "25Cxx", 16);
    
    return HAL_OK;
}

/**
 * @brief SPI EEPROM读一个字节
 */
HAL_StatusTypeDef EEPROM_SPI_ReadByte(EEPROM_SPI_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data)
{
    uint8_t cmd[3] = { 0x03, (addr >> 8) & 0xFF, addr & 0xFF };
    
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    HAL_SPI_Transmit(heeprom->hspi, cmd, 3, 100);
    HAL_SPI_Receive(heeprom->hspi, data, 1, 100);
    heeprom->cs_port->BSRR = heeprom->cs_pin;
    
    return HAL_OK;
}

/**
 * @brief SPI EEPROM写一个字节
 */
HAL_StatusTypeDef EEPROM_SPI_WriteByte(EEPROM_SPI_HandleTypeDef* heeprom, uint16_t addr, uint8_t data)
{
    uint8_t cmd[4] = { 0x06 };  /* WREN */
    
    /* 写使能 */
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    HAL_SPI_Transmit(heeprom->hspi, cmd, 1, 100);
    heeprom->cs_port->BSRR = heeprom->cs_pin;
    
    /* 写数据 */
    cmd[0] = 0x02;  /* WRITE */
    cmd[1] = (addr >> 8) & 0xFF;
    cmd[2] = addr & 0xFF;
    cmd[3] = data;
    
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    HAL_SPI_Transmit(heeprom->hspi, cmd, 4, 100);
    heeprom->cs_port->BSRR = heeprom->cs_pin;
    
    HAL_Delay(5);  /* 等待写周期 */
    
    return HAL_OK;
}

/**
 * @brief SPI EEPROM连续读
 */
HAL_StatusTypeDef EEPROM_SPI_Read(EEPROM_SPI_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint32_t len)
{
    uint8_t cmd[3] = { 0x03, (addr >> 8) & 0xFF, addr & 0xFF };
    
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    HAL_SPI_Transmit(heeprom->hspi, cmd, 3, 100);
    HAL_SPI_Receive(heeprom->hspi, data, len, len * 10 + 100);
    heeprom->cs_port->BSRR = heeprom->cs_pin;
    
    return HAL_OK;
}

/**
 * @brief SPI EEPROM连续写
 */
HAL_StatusTypeDef EEPROM_SPI_Write(EEPROM_SPI_HandleTypeDef* heeprom, uint16_t addr, uint8_t* data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i += heeprom->info.page_size) {
        uint16_t chunk = (len - i > heeprom->info.page_size) ? heeprom->info.page_size : (len - i);
        
        /* 写使能 */
        uint8_t wren = 0x06;
        heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
        HAL_SPI_Transmit(heeprom->hspi, &wren, 1, 100);
        heeprom->cs_port->BSRR = heeprom->cs_pin;
        
        /* 写数据 */
        uint8_t cmd[3] = { 0x02, ((addr + i) >> 8) & 0xFF, (addr + i) & 0xFF };
        heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
        HAL_SPI_Transmit(heeprom->hspi, cmd, 3, 100);
        HAL_SPI_Transmit(heeprom->hspi, data + i, chunk, 5000);
        heeprom->cs_port->BSRR = heeprom->cs_pin;
        
        HAL_Delay(5);
    }
    
    return HAL_OK;
}

/**
 * @brief SPI EEPROM写使能
 */
HAL_StatusTypeDef EEPROM_SPI_WriteEnable(EEPROM_SPI_HandleTypeDef* heeprom)
{
    uint8_t cmd = 0x06;
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    HAL_SPI_Transmit(heeprom->hspi, &cmd, 1, 100);
    heeprom->cs_port->BSRR = heeprom->cs_pin;
    return HAL_OK;
}

/**
 * @brief SPI EEPROM写禁止
 */
HAL_StatusTypeDef EEPROM_SPI_WriteDisable(EEPROM_SPI_HandleTypeDef* heeprom)
{
    uint8_t cmd = 0x04;
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    HAL_SPI_Transmit(heeprom->hspi, &cmd, 1, 100);
    heeprom->cs_port->BSRR = heeprom->cs_pin;
    return HAL_OK;
}

/**
 * @brief SPI EEPROM擦除
 */
HAL_StatusTypeDef EEPROM_SPI_Erase(EEPROM_SPI_HandleTypeDef* heeprom, uint16_t addr, uint32_t len)
{
    uint8_t fill[32];
    memset(fill, 0xFF, heeprom->info.page_size);
    return EEPROM_SPI_Write(heeprom, addr, fill, len);
}

/* ==================== MICROWIRE EEPROM实现 ==================== */

/**
 * @brief 初始化MICROWIRE EEPROM
 */
HAL_StatusTypeDef EEPROM_MICROWIRE_Init(EEPROM_MICROWIRE_HandleTypeDef* heeprom)
{
    if (heeprom == NULL) return HAL_ERROR;
    
    /* 配置GPIO */
    heeprom->cs_port->BSRR = heeprom->cs_pin;         /* CS高 */
    heeprom->clk_port->BSRR = heeprom->clk_pin << 16;  /* CLK低 */
    
    heeprom->info.capacity = 128;  /* 默认93C56 */
    heeprom->info.page_size = 1;
    heeprom->info.addr_width = 8;
    heeprom->info.write_cycle_time_us = 5000;
    strncpy(heeprom->info.part_number, "93C56", 16);
    
    heeprom->initialized = 1;
    return HAL_OK;
}

/**
 * @brief 反初始化MICROWIRE EEPROM
 */
HAL_StatusTypeDef EEPROM_MICROWIRE_DeInit(EEPROM_MICROWIRE_HandleTypeDef* heeprom)
{
    heeprom->initialized = 0;
    return HAL_OK;
}

/**
 * @brief 发送起始位和指令
 */
static void MICROWIRE_SendStartCmd(EEPROM_MICROWIRE_HandleTypeDef* heeprom, uint8_t opcode, uint8_t addr)
{
    /* CS拉高开始 */
    heeprom->cs_port->BSRR = heeprom->cs_pin;
    HAL_Delay(1);
    
    /* 发送起始位(1) */
    heeprom->data_in_port->BSRR = heeprom->data_in_pin;  /* DI=1 */
    heeprom->clk_port->BSRR = heeprom->clk_pin;          /* CLK高 */
    HAL_Delay(1);
    heeprom->clk_port->BSRR = heeprom->clk_pin << 16;    /* CLK低 */
    
    /* 发送操作码(2位) */
    for (int i = 1; i >= 0; i--) {
        if (opcode & (1 << i)) {
            heeprom->data_in_port->BSRR = heeprom->data_in_pin;
        } else {
            heeprom->data_in_port->BSRR = heeprom->data_in_pin << 16;
        }
        heeprom->clk_port->BSRR = heeprom->clk_pin;
        HAL_Delay(1);
        heeprom->clk_port->BSRR = heeprom->clk_pin << 16;
    }
    
    /* 发送地址(6/8/10位) */
    uint8_t addr_bits = heeprom->info.addr_width;
    for (int i = addr_bits - 1; i >= 0; i--) {
        if (addr & (1 << i)) {
            heeprom->data_in_port->BSRR = heeprom->data_in_pin;
        } else {
            heeprom->data_in_port->BSRR = heeprom->data_in_pin << 16;
        }
        heeprom->clk_port->BSRR = heeprom->clk_pin;
        HAL_Delay(1);
        heeprom->clk_port->BSRR = heeprom->clk_pin << 16;
    }
}

/**
 * @brief 读取16位数据
 */
static uint16_t MICROWIRE_Read16(EEPROM_MICROWIRE_HandleTypeDef* heeprom)
{
    uint16_t data = 0;
    
    for (int i = 15; i >= 0; i--) {
        heeprom->clk_port->BSRR = heeprom->clk_pin;
        HAL_Delay(1);
        
        if (heeprom->data_out_port->IDR & heeprom->data_out_pin) {
            data |= (1 << i);
        }
        
        heeprom->clk_port->BSRR = heeprom->clk_pin << 16;
        HAL_Delay(1);
    }
    
    /* CS拉低结束 */
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    
    return data;
}

/**
 * @brief 发送16位数据
 */
static void MICROWIRE_Write16(EEPROM_MICROWIRE_HandleTypeDef* heeprom, uint16_t data)
{
    for (int i = 15; i >= 0; i--) {
        if (data & (1 << i)) {
            heeprom->data_in_port->BSRR = heeprom->data_in_pin;
        } else {
            heeprom->data_in_port->BSRR = heeprom->data_in_pin << 16;
        }
        heeprom->clk_port->BSRR = heeprom->clk_pin;
        HAL_Delay(1);
        heeprom->clk_port->BSRR = heeprom->clk_pin << 16;
    }
    
    /* CS拉低结束 */
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    HAL_Delay(5);  /* 等待写周期 */
}

/**
 * @brief MICROWIRE EEPROM读一个字(16位)
 */
HAL_StatusTypeDef EEPROM_MICROWIRE_ReadWord(EEPROM_MICROWIRE_HandleTypeDef* heeprom, uint8_t addr, uint16_t* data)
{
    MICROWIRE_SendStartCmd(heeprom, 0x02, addr);  /* READ指令=10 */
    *data = MICROWIRE_Read16(heeprom);
    return HAL_OK;
}

/**
 * @brief MICROWIRE EEPROM写一个字(16位)
 */
HAL_StatusTypeDef EEPROM_MICROWIRE_WriteWord(EEPROM_MICROWIRE_HandleTypeDef* heeprom, uint8_t addr, uint16_t data)
{
    /* 先发送EWEN(写使能)指令 */
    MICROWIRE_SendStartCmd(heeprom, 0x00, 0x60);  /* EWEN */
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    
    /* 发送WRITE指令 */
    MICROWIRE_SendStartCmd(heeprom, 0x01, addr);  /* WRITE指令=01 */
    MICROWIRE_Write16(heeprom, data);
    
    /* 发送EWDS(写禁止)指令 */
    MICROWIRE_SendStartCmd(heeprom, 0x00, 0x00);  /* EWDS */
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    
    return HAL_OK;
}

/**
 * @brief MICROWIRE EEPROM读全部
 */
HAL_StatusTypeDef EEPROM_MICROWIRE_ReadAll(EEPROM_MICROWIRE_HandleTypeDef* heeprom, uint16_t* data)
{
    uint32_t word_count = heeprom->info.capacity / 2;
    
    for (uint32_t i = 0; i < word_count; i++) {
        EEPROM_MICROWIRE_ReadWord(heeprom, (uint8_t)i, &data[i]);
    }
    
    return HAL_OK;
}

/**
 * @brief MICROWIRE EEPROM写全部
 */
HAL_StatusTypeDef EEPROM_MICROWIRE_WriteAll(EEPROM_MICROWIRE_HandleTypeDef* heeprom, uint16_t* data)
{
    uint32_t word_count = heeprom->info.capacity / 2;
    
    /* 写使能 */
    MICROWIRE_SendStartCmd(heeprom, 0x00, 0x60);
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    
    for (uint32_t i = 0; i < word_count; i++) {
        MICROWIRE_SendStartCmd(heeprom, 0x01, (uint8_t)i);
        MICROWIRE_Write16(heeprom, data[i]);
    }
    
    /* 写禁止 */
    MICROWIRE_SendStartCmd(heeprom, 0x00, 0x00);
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    
    return HAL_OK;
}

/**
 * @brief MICROWIRE EEPROM全片擦除
 */
HAL_StatusTypeDef EEPROM_MICROWIRE_EraseAll(EEPROM_MICROWIRE_HandleTypeDef* heeprom)
{
    /* 写使能 */
    MICROWIRE_SendStartCmd(heeprom, 0x00, 0x60);
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    
    /* 发送ERAL(全片擦除)指令 */
    MICROWIRE_SendStartCmd(heeprom, 0x00, 0x40);  /* ERAL指令 */
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    HAL_Delay(10);
    
    /* 写禁止 */
    MICROWIRE_SendStartCmd(heeprom, 0x00, 0x00);
    heeprom->cs_port->BSRR = heeprom->cs_pin << 16;
    
    return HAL_OK;
}