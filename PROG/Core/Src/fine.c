/**
 ******************************************************************************
 * @file    fine.c
 * @brief   FINE (Flash Interface Network for Easy Programming) 接口实现
 *          Renesas 瑞萨单片机调试编程接口
 ******************************************************************************
 */

#include "fine.h"
#include "gpio_soft.h"
#include "tim.h"

FINE_Config_TypeDef g_fine_config = {0};
FINE_State_TypeDef g_fine_state = {0};

static uint32_t g_fine_delay = 0;

#define FINE_SET_DATA_HIGH()  GPIO_Soft_WritePin(g_fine_config.flmd0_port, g_fine_config.flmd0_pin, SOFT_GPIO_HIGH)
#define FINE_SET_DATA_LOW()  GPIO_Soft_WritePin(g_fine_config.flmd0_port, g_fine_config.flmd0_pin, SOFT_GPIO_LOW)
#define FINE_READ_DATA()  GPIO_Soft_ReadPin(g_fine_config.flmd0_port, g_fine_config.flmd0_pin)

#define FINE_SET_CLK_HIGH()  GPIO_Soft_WritePin(g_fine_config.flclk_port, g_fine_config.flclk_pin, SOFT_GPIO_HIGH)
#define FINE_SET_CLK_LOW()  GPIO_Soft_WritePin(g_fine_config.flclk_port, g_fine_config.flclk_pin, SOFT_GPIO_LOW)

#define FINE_DATA_MODE_OUT() GPIO_Soft_SetMode(g_fine_config.flmd0_port, g_fine_config.flmd0_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_UP)
#define FINE_DATA_MODE_IN()  GPIO_Soft_SetMode(g_fine_config.flmd0_port, g_fine_config.flmd0_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_UP)

/**
 * @brief 微秒延时函数
 */
void FINE_DelayUs(uint32_t us)
{
    if (us == 0) {
        return;
    }
    
    uint32_t delay = us * g_fine_delay;
    while (delay--) {
        __NOP();
    }
}

/**
 * @brief 设置时钟频率
 */
static void FINE_SetClockFreq(uint32_t freq_hz)
{
    if (freq_hz >= 1000000) {
        g_fine_delay = 1;
    } else if (freq_hz >= 400000) {
        g_fine_delay = 3;
    } else if (freq_hz >= 200000) {
        g_fine_delay = 6;
    } else {
        g_fine_delay = 12;
    }
}

/**
 * @brief FINE 初始化
 */
HAL_StatusTypeDef FINE_Init(FINE_Config_TypeDef *config)
{
    if (config == NULL) {
        return HAL_ERROR;
    }

    memcpy(&g_fine_config, config, sizeof(FINE_Config_TypeDef));
    g_fine_config.clock = (config->clock == 0) ? FINE_DEFAULT_CLOCK : config->clock;

    FINE_GPIO_Init();
    
    FINE_SetClockFreq(g_fine_config.clock);
    
    g_fine_config.initialized = 1;
    
    return HAL_OK;
}

/**
 * @brief FINE 反初始化
 */
HAL_StatusTypeDef FINE_DeInit(void)
{
    FINE_Exit();
    FINE_GPIO_DeInit();
    
    memset(&g_fine_config, 0, sizeof(FINE_Config_TypeDef));
    memset(&g_fine_state, 0, sizeof(FINE_State_TypeDef));
    
    return HAL_OK;
}

/**
 * @brief FINE GPIO 初始化
 */
void FINE_GPIO_Init(void)
{
    FINE_DATA_MODE_OUT();
    GPIO_Soft_SetMode(g_fine_config.flmd1_port, g_fine_config.flmd1_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_UP);
    GPIO_Soft_SetMode(g_fine_config.flmd2_port, g_fine_config.flmd2_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_UP);
    GPIO_Soft_SetMode(g_fine_config.flmd3_port, g_fine_config.flmd3_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_UP);
    GPIO_Soft_SetMode(g_fine_config.flclk_port, g_fine_config.flclk_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(g_fine_config.reset_port, g_fine_config.reset_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_UP);
    
    FINE_SET_DATA_LOW();
    FINE_FLMD1_LOW();
    FINE_FLMD2_LOW();
    FINE_FLMD3_LOW();
    FINE_CLK_LOW();
    FINE_RESET_HIGH();
}

/**
 * @brief FINE GPIO 反初始化
 */
void FINE_GPIO_DeInit(void)
{
    GPIO_Soft_SetMode(g_fine_config.flmd0_port, g_fine_config.flmd0_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(g_fine_config.flmd1_port, g_fine_config.flmd1_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(g_fine_config.flmd2_port, g_fine_config.flmd2_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(g_fine_config.flmd3_port, g_fine_config.flmd3_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(g_fine_config.flclk_port, g_fine_config.flclk_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(g_fine_config.reset_port, g_fine_config.reset_pin, SOFT_GPIO_MODE_IN, SOFT_GPIO_PULL_NONE);
}

/**
 * @brief 发送一位数据
 */
void FINE_SendBit(uint8_t bit)
{
    if (bit) {
        FINE_SET_DATA_HIGH();
    } else {
        FINE_SET_DATA_LOW();
    }
    
    FINE_DelayUs(1);
    
    FINE_SET_CLK_HIGH();
    FINE_DelayUs(1);
    
    FINE_SET_CLK_LOW();
    FINE_DelayUs(1);
}

/**
 * @brief 接收一位数据
 */
uint8_t FINE_ReceiveBit(void)
{
    uint8_t bit;
    
    FINE_DATA_MODE_IN();
    FINE_DelayUs(1);
    
    FINE_SET_CLK_HIGH();
    FINE_DelayUs(1);
    
    bit = FINE_READ_DATA();
    
    FINE_SET_CLK_LOW();
    FINE_DelayUs(1);
    
    FINE_DATA_MODE_OUT();
    
    return bit ? 1 : 0;
}

/**
 * @brief 发送字节
 */
HAL_StatusTypeDef FINE_WriteByte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        FINE_SendBit(data & 0x01);
        data >>= 1;
    }
    
    return HAL_OK;
}

/**
 * @brief 接收字节
 */
uint8_t FINE_ReadByte(void)
{
    uint8_t data = 0;
    
    for (int i = 0; i < 8; i++) {
        data |= FINE_ReceiveBit() << i;
    }
    
    return data;
}

/**
 * @brief 发送字
 */
HAL_StatusTypeDef FINE_WriteWord(uint16_t data)
{
    FINE_WriteByte(data & 0xFF);
    FINE_WriteByte((data >> 8) & 0xFF);
    
    return HAL_OK;
}

/**
 * @brief 接收字
 */
uint16_t FINE_ReadWord(void)
{
    uint16_t data = 0;
    
    data = FINE_ReadByte();
    data |= (uint16_t)FINE_ReadByte() << 8;
    
    return data;
}

/**
 * @brief 发送双字
 */
HAL_StatusTypeDef FINE_WriteDWord(uint32_t data)
{
    FINE_WriteByte(data & 0xFF);
    FINE_WriteByte((data >> 8) & 0xFF);
    FINE_WriteByte((data >> 16) & 0xFF);
    FINE_WriteByte((data >> 24) & 0xFF);
    
    return HAL_OK;
}

/**
 * @brief 接收双字
 */
uint32_t FINE_ReadDWord(void)
{
    uint32_t data = 0;
    
    data = FINE_ReadByte();
    data |= (uint32_t)FINE_ReadByte() << 8;
    data |= (uint32_t)FINE_ReadByte() << 16;
    data |= (uint32_t)FINE_ReadByte() << 24;
    
    return data;
}

/**
 * @brief 进入FINE模式
 */
HAL_StatusTypeDef FINE_Enter(void)
{
    FINE_RESET_LOW();
    FINE_DelayUs(100);
    
    FINE_FLMD0_HIGH();
    FINE_FLMD1_HIGH();
    FINE_FLMD2_HIGH();
    FINE_FLMD3_HIGH();
    FINE_DelayUs(100);
    
    FINE_RESET_HIGH();
    FINE_DelayUs(1000);
    
    for (int i = 0; i < 32; i++) {
        FINE_SET_CLK_HIGH();
        FINE_DelayUs(1);
        FINE_SET_CLK_LOW();
        FINE_DelayUs(1);
    }
    
    FINE_WriteByte(FINE_CMD_READ_ID);
    
    g_fine_state.device_code = FINE_ReadByte();
    g_fine_state.product_code = FINE_ReadWord();
    
    if (g_fine_state.product_code == 0) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief 退出FINE模式
 */
HAL_StatusTypeDef FINE_Exit(void)
{
    FINE_WriteByte(FINE_CMD_EXIT);
    
    FINE_RESET_LOW();
    FINE_DelayUs(100);
    
    FINE_FLMD0_LOW();
    FINE_FLMD1_LOW();
    FINE_FLMD2_LOW();
    FINE_FLMD3_LOW();
    
    FINE_RESET_HIGH();
    FINE_DelayUs(100);
    
    return HAL_OK;
}

/**
 * @brief 复位目标
 */
HAL_StatusTypeDef FINE_Reset(void)
{
    FINE_RESET_LOW();
    FINE_DelayUs(10000);
    FINE_RESET_HIGH();
    FINE_DelayUs(10000);
    
    return HAL_OK;
}

/**
 * @brief 读取ID
 */
HAL_StatusTypeDef FINE_ReadID(void)
{
    FINE_WriteByte(FINE_CMD_READ_ID);
    
    g_fine_state.device_code = FINE_ReadByte();
    g_fine_state.product_code = FINE_ReadWord();
    g_fine_state.chip_id = FINE_ReadDWord();
    
    return HAL_OK;
}

/**
 * @brief 读内存
 */
HAL_StatusTypeDef FINE_ReadMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    FINE_WriteByte(FINE_CMD_READ);
    FINE_WriteDWord(addr);
    
    for (uint32_t i = 0; i < size; i++) {
        data[i] = FINE_ReadByte();
    }
    
    return HAL_OK;
}

/**
 * @brief 写内存
 */
HAL_StatusTypeDef FINE_WriteMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    FINE_WriteByte(FINE_CMD_WRITE);
    FINE_WriteDWord(addr);
    
    for (uint32_t i = 0; i < size; i++) {
        FINE_WriteByte(data[i]);
    }
    
    return HAL_OK;
}

/**
 * @brief 擦除扇区
 */
HAL_StatusTypeDef FINE_EraseSector(uint32_t addr)
{
    FINE_WriteByte(FINE_CMD_ERASE);
    FINE_WriteDWord(addr);
    
    return HAL_OK;
}

/**
 * @brief 擦除芯片
 */
HAL_StatusTypeDef FINE_EraseChip(void)
{
    FINE_WriteByte(FINE_CMD_ERASE);
    FINE_WriteDWord(0x00000000);
    
    return HAL_OK;
}

/**
 * @brief 校验
 */
HAL_StatusTypeDef FINE_Verify(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint8_t temp;
    
    FINE_WriteByte(FINE_CMD_READ);
    FINE_WriteDWord(addr);
    
    for (uint32_t i = 0; i < size; i++) {
        temp = FINE_ReadByte();
        if (temp != data[i]) {
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

/**
 * @brief 获取设备代码
 */
uint8_t FINE_GetDeviceCode(void)
{
    return g_fine_state.device_code;
}

/**
 * @brief 获取产品代码
 */
uint16_t FINE_GetProductCode(void)
{
    return g_fine_state.product_code;
}

/**
 * @brief 获取芯片ID
 */
uint32_t FINE_GetChipID(void)
{
    return g_fine_state.chip_id;
}
