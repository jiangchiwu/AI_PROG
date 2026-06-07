/**
 ******************************************************************************
 * @file    swim.c
 * @brief   SWIM (Single Wire Interface Module) 接口实现
 *          STMicroelectronics STM8系列单片机调试接口
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 * 
 * @details SWIM是STM8系列单片机的单总线调试接口，使用一根线实现双向通信。
 *          本实现采用以下优化策略：
 *          1. 使用HAL库函数操作GPIO，简化代码
 *          2. 支持多档速度配置(100KHz~1MHz)
 *          3. 开漏输出模式，支持双向数据传输
 * 
 * @note    SWIM接口仅使用一根信号线：
 *          - SWIM: 双向数据线(带上拉电阻)
 *          - RESET: 复位控制线(可选)
 * 
 * @warning SWIM接口需要上拉电阻(约10KΩ)
 ******************************************************************************
 */

#include "swim.h"

/**
 * @brief SWIM延时函数
 * @param hswim: SWIM句柄
 * @note 根据通信速度选择不同的延时长度
 */
static void SWIM_Delay(SWIM_HandleTypeDef* hswim)
{
    if (hswim->speed_hz >= 1000000) {
        for (volatile uint32_t i = 0; i < 5; i++);
    } else if (hswim->speed_hz >= 100000) {
        for (volatile uint32_t i = 0; i < 50; i++);
    } else {
        for (volatile uint32_t i = 0; i < 500; i++);
    }
}

/**
 * @brief 设置SWIM引脚为低电平
 * @param hswim: SWIM句柄
 */
static void SWIM_LOW(SWIM_HandleTypeDef* hswim)
{
    HAL_GPIO_WritePin(hswim->swim_port, hswim->swim_pin, GPIO_PIN_RESET);
}

/**
 * @brief 设置SWIM引脚为高电平
 * @param hswim: SWIM句柄
 */
static void SWIM_HIGH(SWIM_HandleTypeDef* hswim)
{
    HAL_GPIO_WritePin(hswim->swim_port, hswim->swim_pin, GPIO_PIN_SET);
}

/**
 * @brief 设置SWIM引脚为输入模式
 * @param hswim: SWIM句柄
 * @note 使用上拉电阻，使引脚空闲时为高电平
 */
static void SWIM_SetInput(SWIM_HandleTypeDef* hswim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = hswim->swim_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(hswim->swim_port, &GPIO_InitStruct);
}

/**
 * @brief 设置SWIM引脚为输出模式
 * @param hswim: SWIM句柄
 * @note 使用开漏输出模式，支持双向通信
 */
static void SWIM_SetOutput(SWIM_HandleTypeDef* hswim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = hswim->swim_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(hswim->swim_port, &GPIO_InitStruct);
}

/**
 * @brief 读取SWIM引脚状态
 * @param hswim: SWIM句柄
 * @return 引脚状态
 */
static GPIO_PinState SWIM_Read(SWIM_HandleTypeDef* hswim)
{
    return HAL_GPIO_ReadPin(hswim->swim_port, hswim->swim_pin);
}

/**
 * @brief 发送一位数据
 * @param hswim: SWIM句柄
 * @param bit: 要发送的位(1或0)
 * @note SWIM协议在下降沿采样数据
 */
static void SWIM_SendBit(SWIM_HandleTypeDef* hswim, uint8_t bit)
{
    if (bit) {
        SWIM_HIGH(hswim);
    } else {
        SWIM_LOW(hswim);
    }
    SWIM_Delay(hswim);
    
    SWIM_LOW(hswim);
    SWIM_Delay(hswim);
    
    SWIM_HIGH(hswim);
    SWIM_Delay(hswim);
}

/**
 * @brief 接收一位数据
 * @param hswim: SWIM句柄
 * @return 接收到的位(1或0)
 */
static uint8_t SWIM_ReceiveBit(SWIM_HandleTypeDef* hswim)
{
    uint8_t bit;
    
    SWIM_SetInput(hswim);
    SWIM_Delay(hswim);
    
    SWIM_LOW(hswim);
    SWIM_SetOutput(hswim);
    SWIM_Delay(hswim);
    
    SWIM_SetInput(hswim);
    SWIM_Delay(hswim);
    
    bit = (SWIM_Read(hswim) == GPIO_PIN_SET) ? 1 : 0;
    
    SWIM_SetOutput(hswim);
    SWIM_HIGH(hswim);
    SWIM_Delay(hswim);
    
    return bit;
}

/**
 * @brief 发送一个字节
 * @param hswim: SWIM句柄
 * @param data: 要发送的字节
 * @note SWIM发送字节时LSB优先
 */
static void SWIM_SendByte(SWIM_HandleTypeDef* hswim, uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++) {
        SWIM_SendBit(hswim, data & 0x01);
        data >>= 1;
    }
}

/**
 * @brief 接收一个字节
 * @param hswim: SWIM句柄
 * @return 接收到的字节
 * @note SWIM接收字节时LSB优先
 */
static uint8_t SWIM_ReceiveByte(SWIM_HandleTypeDef* hswim)
{
    uint8_t data = 0;
    
    for (uint8_t i = 0; i < 8; i++) {
        data |= (SWIM_ReceiveBit(hswim) << i);
    }
    
    return data;
}

/**
 * @brief 初始化SWIM接口
 * @param hswim: SWIM句柄
 * @return HAL状态
 */
HAL_StatusTypeDef SWIM_Init(SWIM_HandleTypeDef* hswim)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    
    SWIM_SetOutput(hswim);
    SWIM_HIGH(hswim);
    
    return HAL_OK;
}

/**
 * @brief 进入SWIM模式
 * @param hswim: SWIM句柄
 * @return HAL状态
 * @note 执行SWIM进入序列：0xA0, 0x00, 0x00, 0x00
 */
HAL_StatusTypeDef SWIM_Entry(SWIM_HandleTypeDef* hswim)
{
    const uint8_t entry_seq[SWIM_ENTRY_SEQ_LEN] = {0xA0, 0x00, 0x00, 0x00};
    
    SWIM_HIGH(hswim);
    for (volatile uint32_t i = 0; i < 1000; i++);
    
    SWIM_LOW(hswim);
    for (volatile uint32_t i = 0; i < 100; i++);
    
    SWIM_HIGH(hswim);
    for (volatile uint32_t i = 0; i < 10; i++);
    
    for (uint8_t i = 0; i < SWIM_ENTRY_SEQ_LEN; i++) {
        SWIM_SendByte(hswim, entry_seq[i]);
    }
    
    return HAL_OK;
}

/**
 * @brief 退出SWIM模式
 * @param hswim: SWIM句柄
 * @return HAL状态
 */
HAL_StatusTypeDef SWIM_Exit(SWIM_HandleTypeDef* hswim)
{
    SWIM_HIGH(hswim);
    
    return HAL_OK;
}

/**
 * @brief 读取内存
 * @param hswim: SWIM句柄
 * @param addr: 内存地址
 * @param data: 数据缓冲区指针
 * @param len: 数据长度
 * @return HAL状态
 */
HAL_StatusTypeDef SWIM_ReadMemory(SWIM_HandleTypeDef* hswim, uint16_t addr, uint8_t* data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        SWIM_SendByte(hswim, 0x80);
        SWIM_SendByte(hswim, addr & 0xFF);
        SWIM_SendByte(hswim, (addr >> 8) & 0xFF);
        data[i] = SWIM_ReceiveByte(hswim);
        addr++;
    }
    
    return HAL_OK;
}

/**
 * @brief 写入内存
 * @param hswim: SWIM句柄
 * @param addr: 内存地址
 * @param data: 数据缓冲区指针
 * @param len: 数据长度
 * @return HAL状态
 */
HAL_StatusTypeDef SWIM_WriteMemory(SWIM_HandleTypeDef* hswim, uint16_t addr, uint8_t* data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        SWIM_SendByte(hswim, 0x40);
        SWIM_SendByte(hswim, addr & 0xFF);
        SWIM_SendByte(hswim, (addr >> 8) & 0xFF);
        SWIM_SendByte(hswim, data[i]);
        addr++;
    }
    
    return HAL_OK;
}

/**
 * @brief 擦除Flash
 * @param hswim: SWIM句柄
 * @return HAL状态
 */
HAL_StatusTypeDef SWIM_EraseFlash(SWIM_HandleTypeDef* hswim)
{
    SWIM_SendByte(hswim, 0x20);
    
    for (volatile uint32_t i = 0; i < 10000; i++);
    
    return HAL_OK;
}