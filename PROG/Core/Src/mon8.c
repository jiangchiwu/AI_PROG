/**
 ******************************************************************************
 * @file    mon8.c
 * @brief   MON8 接口协议实现
 *          Freescale HC08/HC05 8位单片机调试接口
 ******************************************************************************
 */

#include "mon8.h"
#include "gpio_soft.h"
#include "tim.h"

MON8_Config_TypeDef g_mon8_config = {0};
MON8_State_TypeDef g_mon8_state = {0};

static uint32_t g_mon8_delay = 0;

#define MON8_SET_BKPT()  GPIO_Soft_WritePin(g_mon8_config.bkpt_port, g_mon8_config.bkpt_pin, SOFT_GPIO_HIGH)
#define MON8_CLR_BKPT()  GPIO_Soft_WritePin(g_mon8_config.bkpt_port, g_mon8_config.bkpt_pin, SOFT_GPIO_LOW)
#define MON8_READ_BKPT() GPIO_Soft_ReadPin(g_mon8_config.bkpt_port, g_mon8_config.bkpt_pin)

#define MON8_SET_RST()   GPIO_Soft_WritePin(g_mon8_config.rst_port, g_mon8_config.rst_pin, SOFT_GPIO_HIGH)
#define MON8_CLR_RST()   GPIO_Soft_WritePin(g_mon8_config.rst_port, g_mon8_config.rst_pin, SOFT_GPIO_LOW)

#define MON8_SET_TX()    GPIO_Soft_WritePin(g_mon8_config.ptx_port, g_mon8_config.ptx_pin, SOFT_GPIO_HIGH)
#define MON8_CLR_TX()    GPIO_Soft_WritePin(g_mon8_config.ptx_port, g_mon8_config.ptx_pin, SOFT_GPIO_LOW)
#define MON8_READ_RX()   GPIO_Soft_ReadPin(g_mon8_config.prx_port, g_mon8_config.prx_pin)

#define MON8_BKPT_MODE_OUT() GPIO_Soft_SetMode(g_mon8_config.bkpt_port, g_mon8_config.bkpt_pin, SOFT_GPIO_MODE_OUT_OD, SOFT_GPIO_PULL_UP)
#define MON8_BKPT_MODE_IN()  GPIO_Soft_SetMode(g_mon8_config.bkpt_port, g_mon8_config.bkpt_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_UP)

#define MON8_TX_MODE_OUT()   GPIO_Soft_SetMode(g_mon8_config.ptx_port, g_mon8_config.ptx_pin, SOFT_GPIO_MODE_OUT_OD, SOFT_GPIO_PULL_UP)
#define MON8_RX_MODE_IN()    GPIO_Soft_SetMode(g_mon8_config.prx_port, g_mon8_config.prx_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_UP)

/**
 * @brief 微秒延时函数
 */
void MON8_DelayUs(uint32_t us)
{
    if (us == 0) {
        return;
    }
    
    uint32_t delay = us * g_mon8_delay;
    while (delay--) {
        __NOP();
    }
}

/**
 * @brief 设置时钟频率
 */
static void MON8_SetClockFreq(uint32_t freq_hz)
{
    if (freq_hz >= 400000) {
        g_mon8_delay = 2;
    } else if (freq_hz >= 200000) {
        g_mon8_delay = 4;
    } else {
        g_mon8_delay = 8;
    }
}

/**
 * @brief MON8 初始化
 */
HAL_StatusTypeDef MON8_Init(MON8_Config_TypeDef *config)
{
    if (config == NULL) {
        return HAL_ERROR;
    }

    memcpy(&g_mon8_config, config, sizeof(MON8_Config_TypeDef));
    g_mon8_config.clock = (config->clock == 0) ? MON8_DEFAULT_CLOCK : config->clock;

    MON8_GPIO_Init();
    
    MON8_SetClockFreq(g_mon8_config.clock);
    
    g_mon8_config.initialized = 1;
    
    return HAL_OK;
}

/**
 * @brief MON8 反初始化
 */
HAL_StatusTypeDef MON8_DeInit(void)
{
    MON8_Exit();
    MON8_GPIO_DeInit();
    
    memset(&g_mon8_config, 0, sizeof(MON8_Config_TypeDef));
    memset(&g_mon8_state, 0, sizeof(MON8_State_TypeDef));
    
    return HAL_OK;
}

/**
 * @brief MON8 GPIO 初始化
 */
void MON8_GPIO_Init(void)
{
    MON8_BKPT_MODE_OUT();
    MON8_TX_MODE_OUT();
    GPIO_Soft_SetMode(g_mon8_config.rst_port, g_mon8_config.rst_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_UP);
    MON8_RX_MODE_IN();
    
    MON8_SET_BKPT();
    MON8_SET_TX();
    MON8_SET_RST();
}

/**
 * @brief MON8 GPIO 反初始化
 */
void MON8_GPIO_DeInit(void)
{
    GPIO_Soft_SetMode(g_mon8_config.bkpt_port, g_mon8_config.bkpt_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(g_mon8_config.ptx_port, g_mon8_config.ptx_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(g_mon8_config.rst_port, g_mon8_config.rst_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(g_mon8_config.prx_port, g_mon8_config.prx_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
}

/**
 * @brief 发送一位数据
 */
void MON8_SendBit(uint8_t bit)
{
    if (bit) {
        MON8_SET_TX();
    } else {
        MON8_CLR_TX();
    }
    
    MON8_DelayUs(2);
    
    MON8_CLR_RST();
    MON8_DelayUs(1);
    
    MON8_SET_RST();
    MON8_DelayUs(1);
}

/**
 * @brief 接收一位数据
 */
uint8_t MON8_ReceiveBit(void)
{
    uint8_t bit;
    
    MON8_CLR_RST();
    MON8_DelayUs(1);
    
    bit = MON8_READ_RX();
    
    MON8_SET_RST();
    MON8_DelayUs(1);
    
    return bit ? 1 : 0;
}

/**
 * @brief 发送字节
 */
HAL_StatusTypeDef MON8_WriteByte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        MON8_SendBit(data & 0x80);
        data <<= 1;
    }
    
    return HAL_OK;
}

/**
 * @brief 接收字节
 */
uint8_t MON8_ReadByte(void)
{
    uint8_t data = 0;
    
    for (int i = 0; i < 8; i++) {
        data <<= 1;
        data |= MON8_ReceiveBit();
    }
    
    return data;
}

/**
 * @brief 进入MON8模式
 */
HAL_StatusTypeDef MON8_Enter(void)
{
    MON8_CLR_RST();
    MON8_CLR_BKPT();
    MON8_DelayUs(100);
    
    MON8_SET_BKPT();
    MON8_DelayUs(100);
    
    MON8_SET_RST();
    MON8_DelayUs(100);
    
    MON8_WriteByte(MON8_CMD_READ_VER);
    
    g_mon8_state.version = MON8_ReadByte();
    
    if (g_mon8_state.version == 0) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief 退出MON8模式
 */
HAL_StatusTypeDef MON8_Exit(void)
{
    MON8_CLR_RST();
    MON8_DelayUs(100);
    MON8_SET_RST();
    
    return HAL_OK;
}

/**
 * @brief 复位目标
 */
HAL_StatusTypeDef MON8_Reset(void)
{
    MON8_CLR_RST();
    MON8_DelayUs(10000);
    MON8_SET_RST();
    MON8_DelayUs(10000);
    
    return HAL_OK;
}

/**
 * @brief 运行程序
 */
HAL_StatusTypeDef MON8_Run(uint16_t addr)
{
    MON8_WriteByte(MON8_CMD_RUN);
    MON8_WriteByte((addr >> 8) & 0xFF);
    MON8_WriteByte(addr & 0xFF);
    
    return HAL_OK;
}

/**
 * @brief 停止程序
 */
HAL_StatusTypeDef MON8_Stop(void)
{
    MON8_WriteByte(MON8_CMD_STOP);
    
    return HAL_OK;
}

/**
 * @brief 读内存
 */
HAL_StatusTypeDef MON8_ReadMem(uint16_t addr, uint8_t *data, uint16_t size)
{
    MON8_WriteByte(MON8_CMD_READ);
    MON8_WriteByte((addr >> 8) & 0xFF);
    MON8_WriteByte(addr & 0xFF);
    
    for (uint16_t i = 0; i < size; i++) {
        data[i] = MON8_ReadByte();
    }
    
    return HAL_OK;
}

/**
 * @brief 写内存
 */
HAL_StatusTypeDef MON8_WriteMem(uint16_t addr, uint8_t *data, uint16_t size)
{
    MON8_WriteByte(MON8_CMD_WRITE);
    MON8_WriteByte((addr >> 8) & 0xFF);
    MON8_WriteByte(addr & 0xFF);
    
    for (uint16_t i = 0; i < size; i++) {
        MON8_WriteByte(data[i]);
    }
    
    return HAL_OK;
}

/**
 * @brief 擦除
 */
HAL_StatusTypeDef MON8_Erase(uint16_t addr, uint16_t size)
{
    MON8_WriteByte(MON8_CMD_ERASE);
    MON8_WriteByte((addr >> 8) & 0xFF);
    MON8_WriteByte(addr & 0xFF);
    
    return HAL_OK;
}

/**
 * @brief 加密
 */
HAL_StatusTypeDef MON8_Secure(void)
{
    MON8_WriteByte(MON8_CMD_SECURE);
    
    return HAL_OK;
}

/**
 * @brief 获取版本
 */
uint8_t MON8_GetVersion(void)
{
    return g_mon8_state.version;
}

/**
 * @brief 获取设备ID
 */
uint8_t MON8_GetDeviceID(void)
{
    MON8_ReadMem(0xFFFE, &g_mon8_state.device_id, 1);
    return g_mon8_state.device_id;
}
