/**
 ******************************************************************************
 * @file    sbw.c
 * @brief   SBW (Spy-Bi-Wire) 协议实现
 *          MSP430 调试接口 - 两线JTAG替代方案
 ******************************************************************************
 */

#include "sbw.h"
#include "gpio_soft.h"
#include "tim.h"

SBW_Config_TypeDef g_sbw_config = {0};
SBW_State_TypeDef g_sbw_state = {0};

static uint32_t g_sbw_delay = 0;

#define SBW_SET_TMS()  GPIO_Soft_WritePin(g_sbw_config.tms_port, g_sbw_config.tms_pin, SOFT_GPIO_HIGH)
#define SBW_CLR_TMS()  GPIO_Soft_WritePin(g_sbw_config.tms_port, g_sbw_config.tms_pin, SOFT_GPIO_LOW)
#define SBW_READ_TMS() GPIO_Soft_ReadPin(g_sbw_config.tms_port, g_sbw_config.tms_pin)

#define SBW_SET_TCK()  GPIO_Soft_WritePin(g_sbw_config.tck_port, g_sbw_config.tck_pin, SOFT_GPIO_HIGH)
#define SBW_CLR_TCK()  GPIO_Soft_WritePin(g_sbw_config.tck_port, g_sbw_config.tck_pin, SOFT_GPIO_LOW)

#define SBW_TMS_MODE_OUT() GPIO_Soft_SetMode(g_sbw_config.tms_port, g_sbw_config.tms_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_UP)
#define SBW_TMS_MODE_IN()  GPIO_Soft_SetMode(g_sbw_config.tms_port, g_sbw_config.tms_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_UP)

#define SBW_RST_SET()    GPIO_Soft_WritePin(g_sbw_config.rst_port, g_sbw_config.rst_pin, SOFT_GPIO_HIGH)
#define SBW_RST_CLR()    GPIO_Soft_WritePin(g_sbw_config.rst_port, g_sbw_config.rst_pin, SOFT_GPIO_LOW)

#define SBW_TEST_SET()   GPIO_Soft_WritePin(g_sbw_config.test_port, g_sbw_config.test_pin, SOFT_GPIO_HIGH)
#define SBW_TEST_CLR()   GPIO_Soft_WritePin(g_sbw_config.test_port, g_sbw_config.test_pin, SOFT_GPIO_LOW)

/**
 * @brief 微秒延时函数
 */
void SBW_DelayUs(uint32_t us)
{
    if (us == 0) {
        return;
    }
    
    uint32_t delay = us * g_sbw_delay;
    while (delay--) {
        __NOP();
    }
}

/**
 * @brief 设置时钟频率
 */
static void SBW_SetClockFreq(uint32_t freq_hz)
{
    if (freq_hz >= 1000000) {
        g_sbw_delay = 1;
    } else if (freq_hz >= 400000) {
        g_sbw_delay = 3;
    } else if (freq_hz >= 200000) {
        g_sbw_delay = 6;
    } else {
        g_sbw_delay = 12;
    }
}

/**
 * @brief SBW 初始化
 */
HAL_StatusTypeDef SBW_Init(SBW_Config_TypeDef *config)
{
    if (config == NULL) {
        return HAL_ERROR;
    }

    memcpy(&g_sbw_config, config, sizeof(SBW_Config_TypeDef));
    g_sbw_config.clock = (config->clock == 0) ? SBW_DEFAULT_CLOCK : config->clock;

    SBW_GPIO_Init();
    
    SBW_SetClockFreq(g_sbw_config.clock);
    
    g_sbw_config.initialized = 1;
    
    return HAL_OK;
}

/**
 * @brief SBW 反初始化
 */
HAL_StatusTypeDef SBW_DeInit(void)
{
    SBW_Exit();
    SBW_GPIO_DeInit();
    
    memset(&g_sbw_config, 0, sizeof(SBW_Config_TypeDef));
    memset(&g_sbw_state, 0, sizeof(SBW_State_TypeDef));
    
    return HAL_OK;
}

/**
 * @brief SBW GPIO 初始化
 */
void SBW_GPIO_Init(void)
{
    GPIO_Soft_SetMode(g_sbw_config.tck_port, g_sbw_config.tck_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(g_sbw_config.tms_port, g_sbw_config.tms_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_UP);
    GPIO_Soft_SetMode(g_sbw_config.rst_port, g_sbw_config.rst_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_UP);
    GPIO_Soft_SetMode(g_sbw_config.test_port, g_sbw_config.test_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_UP);
    
    SBW_SET_TCK();
    SBW_SET_TMS();
    SBW_RST_SET();
    SBW_TEST_SET();
}

/**
 * @brief SBW GPIO 反初始化
 */
void SBW_GPIO_DeInit(void)
{
    GPIO_Soft_SetMode(g_sbw_config.tck_port, g_sbw_config.tck_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(g_sbw_config.tms_port, g_sbw_config.tms_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(g_sbw_config.rst_port, g_sbw_config.rst_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(g_sbw_config.test_port, g_sbw_config.test_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
}

/**
 * @brief 发送一位数据
 */
void SBW_SendBit(uint8_t bit)
{
    if (bit) {
        SBW_SET_TMS();
    } else {
        SBW_CLR_TMS();
    }
    
    SBW_CLR_TCK();
    SBW_DelayUs(1);
    SBW_SET_TCK();
    SBW_DelayUs(1);
}

/**
 * @brief 接收一位数据
 */
uint8_t SBW_ReceiveBit(void)
{
    uint8_t bit;
    
    SBW_TMS_MODE_IN();
    SBW_DelayUs(1);
    
    SBW_CLR_TCK();
    SBW_DelayUs(1);
    
    bit = SBW_READ_TMS();
    
    SBW_SET_TCK();
    SBW_DelayUs(1);
    
    SBW_TMS_MODE_OUT();
    
    return bit ? 1 : 0;
}

/**
 * @brief 发送字节
 */
static void SBW_SendByte(uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        SBW_SendBit(byte & 0x01);
        byte >>= 1;
    }
}

/**
 * @brief 接收字节
 */
static uint8_t SBW_ReceiveByte(void)
{
    uint8_t byte = 0;
    
    for (int i = 0; i < 8; i++) {
        byte |= SBW_ReceiveBit() << i;
    }
    
    return byte;
}

/**
 * @brief TAP 复位
 */
HAL_StatusTypeDef SBW_TapReset(void)
{
    for (int i = 0; i < 8; i++) {
        SBW_SendBit(1);
    }
    
    for (int i = 0; i < 5; i++) {
        SBW_SendBit(0);
    }
    
    return HAL_OK;
}

/**
 * @brief 移位指令寄存器
 */
HAL_StatusTypeDef SBW_TapShiftIR(uint16_t instruction)
{
    SBW_SendBit(1);
    SBW_SendBit(1);
    SBW_SendBit(0);
    SBW_SendBit(1);
    SBW_SendBit(0);
    
    for (int i = 0; i < 16; i++) {
        SBW_SendBit((instruction >> i) & 0x01);
    }
    
    SBW_SendBit(1);
    SBW_SendBit(1);
    SBW_SendBit(0);
    
    return HAL_OK;
}

/**
 * @brief 移位数据寄存器
 */
HAL_StatusTypeDef SBW_TapShiftDR(uint16_t *data, uint32_t bitlen)
{
    uint16_t temp = 0;
    
    SBW_SendBit(1);
    SBW_SendBit(0);
    SBW_SendBit(1);
    SBW_SendBit(0);
    
    if (data != NULL) {
        temp = *data;
    }
    
    for (uint32_t i = 0; i < bitlen; i++) {
        if (data != NULL) {
            SBW_SendBit((temp >> i) & 0x01);
        } else {
            SBW_ReceiveBit();
        }
    }
    
    SBW_SendBit(1);
    SBW_SendBit(1);
    SBW_SendBit(0);
    
    return HAL_OK;
}

/**
 * @brief 读取数据寄存器
 */
uint16_t SBW_TapReadDR(uint32_t bitlen)
{
    uint16_t data = 0;
    
    SBW_SendBit(1);
    SBW_SendBit(0);
    SBW_SendBit(1);
    SBW_SendBit(0);
    
    for (uint32_t i = 0; i < bitlen; i++) {
        data |= SBW_ReceiveBit() << i;
    }
    
    SBW_SendBit(1);
    SBW_SendBit(1);
    SBW_SendBit(0);
    
    return data;
}

/**
 * @brief 进入SBW模式
 */
HAL_StatusTypeDef SBW_Enter(void)
{
    SBW_RST_CLR();
    SBW_DelayUs(100);
    
    SBW_TEST_CLR();
    SBW_DelayUs(100);
    
    SBW_TEST_SET();
    SBW_DelayUs(100);
    
    SBW_RST_SET();
    SBW_DelayUs(100);
    
    for (int i = 0; i < 200; i++) {
        SBW_CLR_TCK();
        SBW_DelayUs(1);
        SBW_SET_TCK();
        SBW_DelayUs(1);
    }
    
    SBW_TapReset();
    
    SBW_TapShiftIR(0xFF);
    
    g_sbw_state.jtag_id = SBW_TapReadDR(16);
    
    if (g_sbw_state.jtag_id == 0x0000) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief 退出SBW模式
 */
HAL_StatusTypeDef SBW_Exit(void)
{
    SBW_TapReset();
    
    SBW_TapShiftIR(0x00);
    
    SBW_RST_CLR();
    SBW_DelayUs(100);
    SBW_RST_SET();
    
    return HAL_OK;
}

/**
 * @brief 复位目标
 */
HAL_StatusTypeDef SBW_Reset(void)
{
    SBW_RST_CLR();
    SBW_DelayUs(10000);
    SBW_RST_SET();
    SBW_DelayUs(10000);
    
    return HAL_OK;
}

/**
 * @brief 开始调试会话
 */
HAL_StatusTypeDef SBW_Start(void)
{
    SBW_TapShiftIR(0x20);
    SBW_TapShiftDR(NULL, 16);
    
    return HAL_OK;
}

/**
 * @brief 停止调试会话
 */
HAL_StatusTypeDef SBW_Stop(void)
{
    SBW_TapShiftIR(0x00);
    
    return HAL_OK;
}

/**
 * @brief 写字
 */
HAL_StatusTypeDef SBW_WriteWord(uint32_t addr, uint16_t data)
{
    SBW_TapShiftIR(0x22);
    SBW_TapShiftDR(&addr, 32);
    
    SBW_TapShiftIR(0x24);
    SBW_TapShiftDR(&data, 16);
    
    return HAL_OK;
}

/**
 * @brief 读字
 */
uint16_t SBW_ReadWord(uint32_t addr)
{
    uint16_t data;
    
    SBW_TapShiftIR(0x22);
    SBW_TapShiftDR(&addr, 32);
    
    SBW_TapShiftIR(0x21);
    data = SBW_TapReadDR(16);
    
    return data;
}

/**
 * @brief 写内存
 */
HAL_StatusTypeDef SBW_WriteMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint16_t word;
    
    for (uint32_t i = 0; i < size; i += 2) {
        word = data[i];
        if (i + 1 < size) {
            word |= data[i + 1] << 8;
        }
        
        if (SBW_WriteWord(addr + i, word) != HAL_OK) {
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

/**
 * @brief 读内存
 */
HAL_StatusTypeDef SBW_ReadMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint16_t word;
    
    for (uint32_t i = 0; i < size; i += 2) {
        word = SBW_ReadWord(addr + i);
        
        data[i] = word & 0xFF;
        if (i + 1 < size) {
            data[i + 1] = (word >> 8) & 0xFF;
        }
    }
    
    return HAL_OK;
}

/**
 * @brief 获取JTAG ID
 */
uint16_t SBW_GetJTAGID(void)
{
    return g_sbw_state.jtag_id;
}

/**
 * @brief 获取ID Code
 */
uint32_t SBW_GetIDCode(void)
{
    SBW_TapShiftIR(0xFE);
    g_sbw_state.idcode = SBW_TapReadDR(32);
    
    return g_sbw_state.idcode;
}
