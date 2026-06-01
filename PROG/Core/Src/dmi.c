/**
 ******************************************************************************
 * @file    dmi.c
 * @brief   RISC-V Debug Module Interface (DMI) 实现
 ******************************************************************************
 */

#include "dmi.h"
#include "jtag.h"
#include <string.h>

// 全局状态
static DMI_State_t dmi_state;

// DMI 扫描链长度
#define DMI_SCAN_LENGTH 41  // 6位地址 + 32位数据 + 3位操作 + 预留

// 内部函数声明
static HAL_StatusTypeDef DMI_SendOp(uint8_t op, uint8_t addr, uint32_t data, uint32_t *result);

// DMI 初始化
HAL_StatusTypeDef DMI_Init(void)
{
    HAL_StatusTypeDef status;

    // 初始化JTAG
    status = JTAG_Init(NULL);
    if (status != HAL_OK) {
        return status;
    }

    // 复位DMI
    status = DMI_Reset();
    if (status != HAL_OK) {
        return status;
    }

    // 读取DTMCS寄存器
    status = DMI_ReadReg(DMI_REG_DTMCS, &dmi_state.dtmcs);
    if (status != HAL_OK) {
        return status;
    }

    // 获取空闲周期数
    dmi_state.idle_cycles = (dmi_state.dtmcs >> 12) & 0x07;
    dmi_state.initialized = true;

    return HAL_OK;
}

// DMI 反初始化
HAL_StatusTypeDef DMI_DeInit(void)
{
    memset(&dmi_state, 0, sizeof(dmi_state));
    return JTAG_DeInit();
}

// DMI 复位
HAL_StatusTypeDef DMI_Reset(void)
{
    // JTAG TAP复位
    JTAG_TAP_Reset();
    
    // DMI软复位
    DMI_WriteReg(DMI_REG_DTMCS, DTMCS_DMIRESET);
    
    // 清除复位
    DMI_WriteReg(DMI_REG_DTMCS, 0);
    
    dmi_state.dmi_stat = DMI_STAT_OK;
    
    return HAL_OK;
}

// 激活调试模块
HAL_StatusTypeDef DMI_ActivateDM(void)
{
    uint32_t dmcontrol;
    
    // 读取DMCONTROL
    if (DMI_ReadReg(DM_REG_DMCONTROL, &dmcontrol) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // 设置dmactive
    dmcontrol |= DMCONTROL_DMACTIVE;
    
    // 写回
    if (DMI_WriteReg(DM_REG_DMCONTROL, dmcontrol) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // 确认激活
    if (DMI_ReadReg(DM_REG_DMCONTROL, &dmcontrol) != HAL_OK) {
        return HAL_ERROR;
    }
    
    if (!(dmcontrol & DMCONTROL_DMACTIVE)) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

// 暂停HART
HAL_StatusTypeDef DMI_HaltHart(uint32_t hart_id)
{
    uint32_t dmcontrol;
    
    // 选择HART并设置暂停请求
    dmcontrol = DMCONTROL_DMACTIVE | 
                DMCONTROL_HARTSEL(hart_id) | 
                DMCONTROL_HALTREQ;
    
    if (DMI_WriteReg(DM_REG_DMCONTROL, dmcontrol) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // 等待暂停确认
    // 实际应用中可能需要轮询DMSTATUS
    
    return HAL_OK;
}

// 恢复HART
HAL_StatusTypeDef DMI_ResumeHart(uint32_t hart_id)
{
    uint32_t dmcontrol;
    
    // 选择HART并设置恢复请求
    dmcontrol = DMCONTROL_DMACTIVE | 
                DMCONTROL_HARTSEL(hart_id) | 
                DMCONTROL_RESUMEREQ;
    
    if (DMI_WriteReg(DM_REG_DMCONTROL, dmcontrol) != HAL_OK) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

// 读取DMI寄存器
HAL_StatusTypeDef DMI_ReadReg(uint8_t addr, uint32_t *data)
{
    return DMI_SendOp(DMI_CMD_READ, addr, 0, data);
}

// 写入DMI寄存器
HAL_StatusTypeDef DMI_WriteReg(uint8_t addr, uint32_t data)
{
    return DMI_SendOp(DMI_CMD_WRITE, addr, data, NULL);
}

// 发送DMI操作
static HAL_StatusTypeDef DMI_SendOp(uint8_t op, uint8_t addr, uint32_t data, uint32_t *result)
{
    uint8_t dmi_out[6];
    uint8_t dmi_in[6];
    uint8_t ir_out[1] = {0}; // DMI指令码，通常为0x11
    uint8_t resp;
    
    memset(dmi_out, 0, 6);
    memset(dmi_in, 0, 6);
    
    // 构造DMI数据: 6位addr + 32位data + 3位op
    // 位序: op[0-2], data[3-34], addr[35-40] (小端位序)
    dmi_out[0] = (op & 0x07) | ((data & 0x1F) << 3);
    dmi_out[1] = (data >> 5) & 0xFF;
    dmi_out[2] = (data >> 13) & 0xFF;
    dmi_out[3] = (data >> 21) & 0xFF;
    dmi_out[4] = (data >> 29) & 0x07;
    dmi_out[4] |= ((addr & 0x1F) << 3);
    dmi_out[5] = (addr >> 5) & 0x01;
    
    // 通过JTAG发送DMI操作
    // IR通常为4位，值为0x11 (DMI指令)
    ir_out[0] = 0x11;
    
    if (JTAG_Write_IR_Bits(ir_out, 4, dmi_out, DMI_SCAN_LENGTH, (result != NULL) ? dmi_in : NULL) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // 解析响应
    resp = dmi_in[0] & 0x03;
    dmi_state.dmi_stat = resp;
    
    if (resp != DMI_STAT_OK) {
        return HAL_ERROR;
    }
    
    // 获取读回的数据
    if (result != NULL && op == DMI_CMD_READ) {
        // 数据从第3位开始
        *result = ((uint32_t)(dmi_in[0] >> 3)) | 
                  ((uint32_t)dmi_in[1] << 5) | 
                  ((uint32_t)dmi_in[2] << 13) | 
                  ((uint32_t)dmi_in[3] << 21) |
                  (((uint32_t)dmi_in[4] & 0x07) << 29);
    }
    
    return HAL_OK;
}

// 等待DMI空闲
HAL_StatusTypeDef DMI_WaitIdle(void)
{
    // 发送NOP操作以同步
    return DMI_SendOp(DMI_CMD_NOP, 0, 0, NULL);
}

// 读取内存 (通过系统总线访问)
HAL_StatusTypeDef DMI_ReadMemory(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t i;
    uint32_t word_data;
    uint32_t read_addr;
    
    // 设置地址
    DMI_WriteReg(DM_REG_SBADDRESS0, addr);
    
    // 逐字读取
    for (i = 0; i < size; i += 4) {
        read_addr = addr + i;
        
        // 更新地址 (如果超过一个字)
        if (i > 0) {
            DMI_WriteReg(DM_REG_SBADDRESS0, read_addr);
        }
        
        // 读取数据
        if (DMI_ReadReg(DM_REG_SBDATA0, &word_data) != HAL_OK) {
            return HAL_ERROR;
        }
        
        // 复制到输出缓冲区
        uint32_t copy_size = (size - i) >= 4 ? 4 : (size - i);
        memcpy(&data[i], &word_data, copy_size);
    }
    
    return HAL_OK;
}

// 写入内存 (通过系统总线访问)
HAL_StatusTypeDef DMI_WriteMemory(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t i;
    uint32_t word_data;
    uint32_t write_addr;
    
    // 逐字写入
    for (i = 0; i < size; i += 4) {
        write_addr = addr + i;
        
        // 设置地址
        DMI_WriteReg(DM_REG_SBADDRESS0, write_addr);
        
        // 准备数据
        word_data = 0;
        uint32_t copy_size = (size - i) >= 4 ? 4 : (size - i);
        memcpy(&word_data, &data[i], copy_size);
        
        // 写入数据
        if (DMI_WriteReg(DM_REG_SBDATA0, word_data) != HAL_OK) {
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}
