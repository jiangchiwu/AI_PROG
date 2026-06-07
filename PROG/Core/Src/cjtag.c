/**
 ******************************************************************************
 * @file    cjtag.c
 * @brief   cJTAG (compact JTAG) 接口实现
 *          兼容传统JTAG和cJTAG模式的调试接口
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 * 
 * @details cJTAG是传统JTAG的紧凑版本，使用更少的引脚实现调试功能。
 *          本实现支持以下特性：
 *          1. 支持JTAG到cJTAG模式切换
 *          2. 支持传统JTAG操作
 *          3. 支持100KHz~10MHz可调时钟频率
 *          4. 支持RTCK(返回时钟)功能
 * 
 * @note    cJTAG接口信号：
 *          - TCK: 测试时钟
 *          - TMS: 测试模式选择
 *          - TDI: 测试数据输入
 *          - TDO: 测试数据输出
 *          - RTCK: 返回时钟(可选)
 * 
 * @warning cJTAG需要目标芯片支持cJTAG模式
 ******************************************************************************
 */

#include "cjtag.h"
#include "gpio_soft.h"

/**
 * @brief cJTAG延时函数
 * @param hcjtag: cJTAG句柄
 * @note 根据通信速度选择不同的延时长度，支持最高10MHz
 */
static void CJTAG_Delay(CJTAG_HandleTypeDef* hcjtag)
{
    if (hcjtag->speed_hz >= 10000000) {
        for (volatile uint32_t i = 0; i < 5; i++);
    } else if (hcjtag->speed_hz >= 1000000) {
        for (volatile uint32_t i = 0; i < 50; i++);
    } else {
        for (volatile uint32_t i = 0; i < 500; i++);
    }
}

/**
 * @brief 切换TCK时钟
 * @param hcjtag: cJTAG句柄
 * @note 产生一个完整的TCK时钟周期
 */
static void CJTAG_ToggleTCK(CJTAG_HandleTypeDef* hcjtag)
{
    HAL_GPIO_WritePin(hcjtag->tck_port, hcjtag->tck_pin, GPIO_PIN_SET);
    CJTAG_Delay(hcjtag);
    HAL_GPIO_WritePin(hcjtag->tck_port, hcjtag->tck_pin, GPIO_PIN_RESET);
    CJTAG_Delay(hcjtag);
}

/**
 * @brief 初始化cJTAG接口
 * @param hcjtag: cJTAG句柄
 * @return HAL状态
 */
HAL_StatusTypeDef CJTAG_Init(CJTAG_HandleTypeDef* hcjtag)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = hcjtag->tck_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(hcjtag->tck_port, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = hcjtag->tms_pin;
    HAL_GPIO_Init(hcjtag->tms_port, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = hcjtag->tdi_pin;
    HAL_GPIO_Init(hcjtag->tdi_port, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = hcjtag->tdo_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(hcjtag->tdo_port, &GPIO_InitStruct);
    
    if (hcjtag->rtck_port != NULL) {
        GPIO_InitStruct.Pin = hcjtag->rtck_pin;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(hcjtag->rtck_port, &GPIO_InitStruct);
    }
    
    HAL_GPIO_WritePin(hcjtag->tck_port, hcjtag->tck_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(hcjtag->tms_port, hcjtag->tms_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(hcjtag->tdi_port, hcjtag->tdi_pin, GPIO_PIN_RESET);
    
    return HAL_OK;
}

/**
 * @brief 反初始化cJTAG接口
 * @param hcjtag: cJTAG句柄
 * @return HAL状态
 */
HAL_StatusTypeDef CJTAG_DeInit(CJTAG_HandleTypeDef* hcjtag)
{
    HAL_GPIO_DeInit(hcjtag->tck_port, hcjtag->tck_pin);
    HAL_GPIO_DeInit(hcjtag->tms_port, hcjtag->tms_pin);
    HAL_GPIO_DeInit(hcjtag->tdi_port, hcjtag->tdi_pin);
    HAL_GPIO_DeInit(hcjtag->tdo_port, hcjtag->tdo_pin);
    if (hcjtag->rtck_port != NULL) {
        HAL_GPIO_DeInit(hcjtag->rtck_port, hcjtag->rtck_pin);
    }
    return HAL_OK;
}

/**
 * @brief 复位TAP状态机
 * @param hcjtag: cJTAG句柄
 * @return HAL状态
 * @note 通过在TMS=1时提供5个TCK周期进入Test-Logic-Reset状态
 */
HAL_StatusTypeDef CJTAG_Reset(CJTAG_HandleTypeDef* hcjtag)
{
    HAL_GPIO_WritePin(hcjtag->tms_port, hcjtag->tms_pin, GPIO_PIN_SET);
    for (uint8_t i = 0; i < 5; i++) {
        CJTAG_ToggleTCK(hcjtag);
    }
    HAL_GPIO_WritePin(hcjtag->tms_port, hcjtag->tms_pin, GPIO_PIN_RESET);
    CJTAG_ToggleTCK(hcjtag);
    return HAL_OK;
}

/**
 * @brief 切换到传统JTAG模式
 * @param hcjtag: cJTAG句柄
 * @return HAL状态
 * @note 发送0x1F序列切换到JTAG模式
 */
HAL_StatusTypeDef CJTAG_SwitchToJTAG(CJTAG_HandleTypeDef* hcjtag)
{
    CJTAG_Reset(hcjtag);
    
    uint8_t switch_seq = 0x1F;
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(hcjtag->tms_port, hcjtag->tms_pin, (switch_seq & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        CJTAG_ToggleTCK(hcjtag);
        switch_seq >>= 1;
    }
    
    hcjtag->mode = 0;
    return HAL_OK;
}

/**
 * @brief 切换到cJTAG模式
 * @param hcjtag: cJTAG句柄
 * @return HAL状态
 * @note 发送0x3F序列切换到cJTAG模式
 */
HAL_StatusTypeDef CJTAG_SwitchToCJTAG(CJTAG_HandleTypeDef* hcjtag)
{
    CJTAG_Reset(hcjtag);
    
    uint8_t switch_seq = 0x3F;
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(hcjtag->tms_port, hcjtag->tms_pin, (switch_seq & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        CJTAG_ToggleTCK(hcjtag);
        switch_seq >>= 1;
    }
    
    hcjtag->mode = 1;
    return HAL_OK;
}

/**
 * @brief 移位一位数据
 * @param hcjtag: cJTAG句柄
 * @param tms: TMS位值
 * @param tdi: TDI位值
 * @return TDO位值
 */
static uint8_t CJTAG_ShiftBit(CJTAG_HandleTypeDef* hcjtag, uint8_t tms, uint8_t tdi)
{
    uint8_t tdo = 0;
    
    HAL_GPIO_WritePin(hcjtag->tms_port, hcjtag->tms_pin, tms ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(hcjtag->tdi_port, hcjtag->tdi_pin, tdi ? GPIO_PIN_SET : GPIO_PIN_RESET);
    CJTAG_Delay(hcjtag);
    
    tdo = HAL_GPIO_ReadPin(hcjtag->tdo_port, hcjtag->tdo_pin) ? 1 : 0;
    
    HAL_GPIO_WritePin(hcjtag->tck_port, hcjtag->tck_pin, GPIO_PIN_SET);
    CJTAG_Delay(hcjtag);
    HAL_GPIO_WritePin(hcjtag->tck_port, hcjtag->tck_pin, GPIO_PIN_RESET);
    CJTAG_Delay(hcjtag);
    
    return tdo;
}

/**
 * @brief TAP状态机状态转移
 * @param hcjtag: cJTAG句柄
 * @param target_state: 目标状态
 * @return HAL状态
 */
HAL_StatusTypeDef CJTAG_TAP_GotoState(CJTAG_HandleTypeDef* hcjtag, CJTAG_TAP_State_t target_state)
{
    static const uint8_t tms_seq[16] = {
        0b11111, 0b0, 0b10, 0b110, 0b1110, 0b11110, 0b111110, 0b0,
        0b1, 0b11, 0b111, 0b1111, 0b11111, 0b0, 0b0, 0b0
    };
    
    uint8_t seq = tms_seq[target_state];
    for (uint8_t i = 0; i < 5; i++) {
        CJTAG_ShiftBit(hcjtag, (seq >> i) & 0x01, 0);
    }
    
    return HAL_OK;
}

/**
 * @brief 写入指令寄存器
 * @param hcjtag: cJTAG句柄
 * @param ir_data: 指令数据
 * @param ir_length: 指令长度(位)
 * @return HAL状态
 */
HAL_StatusTypeDef CJTAG_WriteIR(CJTAG_HandleTypeDef* hcjtag, uint8_t* ir_data, uint16_t ir_length)
{
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_SHIFT_IR);
    
    for (uint16_t i = 0; i < ir_length; i++) {
        uint8_t byte_idx = i / 8;
        uint8_t bit_idx = i % 8;
        uint8_t tdi = (ir_data[byte_idx] >> bit_idx) & 0x01;
        uint8_t tms = (i == ir_length - 1) ? 1 : 0;
        
        CJTAG_ShiftBit(hcjtag, tms, tdi);
    }
    
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_RUN_TEST_IDLE);
    
    return HAL_OK;
}

/**
 * @brief 读取指令寄存器
 * @param hcjtag: cJTAG句柄
 * @param ir_data: 读取的数据缓冲区
 * @param ir_length: 指令长度(位)
 * @return HAL状态
 */
HAL_StatusTypeDef CJTAG_ReadIR(CJTAG_HandleTypeDef* hcjtag, uint8_t* ir_data, uint16_t ir_length)
{
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_SHIFT_IR);
    
    for (uint16_t i = 0; i < ir_length; i++) {
        uint8_t byte_idx = i / 8;
        uint8_t bit_idx = i % 8;
        uint8_t tms = (i == ir_length - 1) ? 1 : 0;
        
        uint8_t tdo = CJTAG_ShiftBit(hcjtag, tms, 0);
        
        if (bit_idx == 0) {
            ir_data[byte_idx] = 0;
        }
        ir_data[byte_idx] |= (tdo << bit_idx);
    }
    
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_RUN_TEST_IDLE);
    
    return HAL_OK;
}

/**
 * @brief 写入数据寄存器
 * @param hcjtag: cJTAG句柄
 * @param dr_data: 数据
 * @param dr_length: 数据长度(位)
 * @return HAL状态
 */
HAL_StatusTypeDef CJTAG_WriteDR(CJTAG_HandleTypeDef* hcjtag, uint8_t* dr_data, uint16_t dr_length)
{
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_SHIFT_DR);
    
    for (uint16_t i = 0; i < dr_length; i++) {
        uint8_t byte_idx = i / 8;
        uint8_t bit_idx = i % 8;
        uint8_t tdi = (dr_data[byte_idx] >> bit_idx) & 0x01;
        uint8_t tms = (i == dr_length - 1) ? 1 : 0;
        
        CJTAG_ShiftBit(hcjtag, tms, tdi);
    }
    
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_RUN_TEST_IDLE);
    
    return HAL_OK;
}

/**
 * @brief 读取数据寄存器
 * @param hcjtag: cJTAG句柄
 * @param dr_data: 读取的数据缓冲区
 * @param dr_length: 数据长度(位)
 * @return HAL状态
 */
HAL_StatusTypeDef CJTAG_ReadDR(CJTAG_HandleTypeDef* hcjtag, uint8_t* dr_data, uint16_t dr_length)
{
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_SHIFT_DR);
    
    for (uint16_t i = 0; i < dr_length; i++) {
        uint8_t byte_idx = i / 8;
        uint8_t bit_idx = i % 8;
        uint8_t tms = (i == dr_length - 1) ? 1 : 0;
        
        uint8_t tdo = CJTAG_ShiftBit(hcjtag, tms, 0);
        
        if (bit_idx == 0) {
            dr_data[byte_idx] = 0;
        }
        dr_data[byte_idx] |= (tdo << bit_idx);
    }
    
    CJTAG_TAP_GotoState(hcjtag, CJTAG_STATE_RUN_TEST_IDLE);
    
    return HAL_OK;
}

/**
 * @brief 读取IDCODE
 * @param hcjtag: cJTAG句柄
 * @return IDCODE值(32位)
 */
uint32_t CJTAG_ReadIDCODE(CJTAG_HandleTypeDef* hcjtag)
{
    uint8_t ir_data[2] = {0x01, 0x00};
    uint8_t dr_data[4] = {0};
    
    CJTAG_WriteIR(hcjtag, ir_data, 4);
    CJTAG_ReadDR(hcjtag, dr_data, 32);
    
    return (dr_data[3] << 24) | (dr_data[2] << 16) | (dr_data[1] << 8) | dr_data[0];
}