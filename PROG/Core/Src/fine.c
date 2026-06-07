/**
 ******************************************************************************
 * @file    fine.c
 * @brief   FINE (Flash Interface Network for Easy Programming) 接口实现
 *          Renesas 瑞萨单片机调试编程接口
 * 
 * @author  AI_PROG项目
 * @date    2026-06-05
 * @version v2.0
 * 
 * @details FINE是Renesas为其RL78系列等单片机设计的专用编程接口。
 *          本实现采用以下优化策略：
 *          1. 寄存器直接操作替代HAL库函数，减少函数调用开销
 *          2. 使用TIM13定时器实现纳秒级精确定时
 *          3. 支持100KHz~10MHz的可调时钟频率
 *          4. 开漏输出模式，支持双向数据传输
 * 
 * @note    FINE接口使用以下信号：
 *          - FLMD[3:0]: 多功能数据/地址线
 *          - FLCLK: 时钟线
 *          - RESET: 复位控制线
 * 
 * @warning 本驱动使用TIM13定时器，需确保与其他功能不冲突
 ******************************************************************************
 */

#include "fine.h"

/**
 * @brief FINE全局配置结构体
 *        存储FINE接口的硬件配置和运行状态
 */
FINE_Config_TypeDef g_fine_config = {0};

/**
 * @brief FINE全局状态结构体
 *        存储FINE会话期间的状态信息
 */
FINE_State_TypeDef g_fine_state = {0};

/**
 * @brief FINE定时器定义
 * @note  使用TIM13，挂载在APB1总线上，最高时钟可达120MHz
 */
#define FINE_TIM TIM13

/**
 * @brief 等待定时器计数达到指定值
 * @param ticks: 等待的定时器计数值
 * @note  阻塞式等待，精确延时
 */
static void FINE_TimerWait(uint32_t ticks)
{
    FINE_TIM->CNT = 0;
    while (FINE_TIM->CNT < ticks);
}

/**
 * @brief 纳秒级延时
 * @param ns: 延时时间(纳秒)
 * @note  基于定时器实现，精度取决于定时器配置
 */
void FINE_DelayNs(uint32_t ns)
{
    uint32_t ticks = (ns + g_fine_config.tick_ns - 1) / g_fine_config.tick_ns;
    FINE_TimerWait(ticks);
}

/**
 * @brief 微秒级延时
 * @param us: 延时时间(微秒)
 * @note  基于定时器实现，精度取决于定时器配置
 */
void FINE_DelayUs(uint32_t us)
{
    uint32_t ticks = (us * 1000 + g_fine_config.tick_ns - 1) / g_fine_config.tick_ns;
    FINE_TimerWait(ticks);
}

/**
 * @brief 设置FINE通信速度
 * @param speed_hz: 目标通信速度(Hz)
 * @note  支持的速度范围: 100KHz ~ 10MHz
 * @note  速度过高会导致通信不稳定
 */
void FINE_SetSpeed(uint32_t speed_hz)
{
    if (speed_hz > FINE_CLOCK_10MHZ) {
        speed_hz = FINE_CLOCK_10MHZ;
    }
    
    g_fine_config.speed_hz = speed_hz;
    
    uint32_t apb1_freq = HAL_RCC_GetPCLK1Freq();
    uint32_t tick_freq = speed_hz * 10;
    
    g_fine_config.prescaler = (apb1_freq + tick_freq - 1) / tick_freq;
    if (g_fine_config.prescaler < 1) {
        g_fine_config.prescaler = 1;
    }
    
    g_fine_config.tick_ns = 1000000000ULL / (apb1_freq / g_fine_config.prescaler);
    g_fine_config.period = 65535;
    
    if (FINE_TIM->CR1 & TIM_CR1_CEN) {
        FINE_TIM->CR1 &= ~TIM_CR1_CEN;
        FINE_TIM->PSC = g_fine_config.prescaler - 1;
        FINE_TIM->ARR = g_fine_config.period - 1;
        FINE_TIM->EGR = TIM_EGR_UG;
        FINE_TIM->CR1 |= TIM_CR1_CEN;
    }
}

/**
 * @brief 获取当前FINE通信速度
 * @return 当前速度(Hz)
 */
uint32_t FINE_GetSpeed(void)
{
    return g_fine_config.speed_hz;
}

/**
 * @brief 初始化FINE定时器
 * @note  使用TIM13定时器，配置为基本定时器模式
 * @note  定时器时钟 = APB1时钟 / prescaler
 */
static void FINE_TimerInit(void)
{
    FINE_TIM_CLK_ENABLE();
    
    FINE_TIM->CR1 = 0;
    FINE_TIM->CR2 = 0;
    FINE_TIM->SMCR = 0;
    FINE_TIM->DIER = 0;
    FINE_TIM->SR = 0;
    
    FINE_TIM->PSC = g_fine_config.prescaler - 1;
    FINE_TIM->ARR = g_fine_config.period - 1;
    
    FINE_TIM->EGR = TIM_EGR_UG;
    
    FINE_TIM->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief 初始化FINE GPIO引脚(寄存器操作)
 * 
 * @details 配置FLMD[3:0]、FLCLK、RESET引脚。
 *          FLMD引脚使用开漏输出模式，RESET使用推挽输出。
 *          设置初始状态：FLMD[3:0]=0, FLCLK=0, RESET=1
 */
static void FINE_GPIO_Init_Reg(void)
{
    GPIO_TypeDef* ports[6] = {
        g_fine_config.flmd0_port, g_fine_config.flmd1_port,
        g_fine_config.flmd2_port, g_fine_config.flmd3_port,
        g_fine_config.flclk_port, g_fine_config.reset_port
    };
    uint16_t pins[6] = {
        g_fine_config.flmd0_pin, g_fine_config.flmd1_pin,
        g_fine_config.flmd2_pin, g_fine_config.flmd3_pin,
        g_fine_config.flclk_pin, g_fine_config.reset_pin
    };
    
    for (int i = 0; i < 6; i++) {
        GPIO_TypeDef* port = ports[i];
        uint32_t pin = pins[i];
        
        if (port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
        else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
        else if (port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
        else if (port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
        else if (port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
        else if (port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
        else if (port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
        else if (port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
        else if (port == GPIOI) __HAL_RCC_GPIOI_CLK_ENABLE();
        
        uint32_t shift = (pin % 8) * 4;
        
        port->MODER &= ~(0x3UL << shift);
        port->MODER |= (0x1UL << shift);
        
        port->OTYPER |= (1UL << pin);
        
        port->PUPDR &= ~(0x3UL << shift);
        port->PUPDR |= (0x1UL << shift);
        
        port->OSPEEDR &= ~(0x3UL << shift);
        port->OSPEEDR |= (0x3UL << shift);
    }
    
    FINE_FLMD0_LOW();
    FINE_FLMD1_LOW();
    FINE_FLMD2_LOW();
    FINE_FLMD3_LOW();
    FINE_CLK_LOW();
    FINE_RESET_HIGH();
}

/**
 * @brief 设置数据引脚为输入模式
 * 
 * @details 将FLMD0引脚切换为输入模式，用于接收数据
 */
static void FINE_DATA_Mode_In(void)
{
    uint32_t pin = g_fine_config.flmd0_pin;
    uint32_t shift = (pin % 8) * 4;
    
    g_fine_config.flmd0_port->MODER &= ~(0x3UL << shift);
}

/**
 * @brief 设置数据引脚为输出模式
 * 
 * @details 将FLMD0引脚切换为输出模式，用于发送数据
 */
static void FINE_DATA_Mode_Out(void)
{
    uint32_t pin = g_fine_config.flmd0_pin;
    uint32_t shift = (pin % 8) * 4;
    
    g_fine_config.flmd0_port->MODER &= ~(0x3UL << shift);
    g_fine_config.flmd0_port->MODER |= (0x1UL << shift);
    g_fine_config.flmd0_port->OTYPER |= (1UL << pin);
}

/**
 * @brief 初始化FINE接口
 * @param config: FINE配置结构体指针
 * @return HAL状态
 * 
 * @details 初始化FINE接口，包括GPIO配置、定时器配置和协议状态初始化。
 *          如果传入config参数，则使用用户配置；否则使用默认配置。
 */
HAL_StatusTypeDef FINE_Init(FINE_Config_TypeDef *config)
{
    if (config == NULL) {
        return HAL_ERROR;
    }

    memcpy(&g_fine_config, config, sizeof(FINE_Config_TypeDef));
    
    if (g_fine_config.speed_hz == 0) {
        g_fine_config.speed_hz = FINE_DEFAULT_CLOCK;
    }
    
    FINE_SetSpeed(g_fine_config.speed_hz);
    FINE_TimerInit();
    FINE_GPIO_Init_Reg();
    
    g_fine_config.initialized = 1;
    
    return HAL_OK;
}

/**
 * @brief 反初始化FINE接口
 * @return HAL状态
 * 
 * @details 关闭FINE接口，释放GPIO和定时器资源。
 */
HAL_StatusTypeDef FINE_DeInit(void)
{
    FINE_TIM->CR1 &= ~TIM_CR1_CEN;
    FINE_GPIO_DeInit();
    
    memset(&g_fine_config, 0, sizeof(FINE_Config_TypeDef));
    memset(&g_fine_state, 0, sizeof(FINE_State_TypeDef));
    
    return HAL_OK;
}

/**
 * @brief 初始化FINE GPIO引脚(封装接口)
 */
void FINE_GPIO_Init(void)
{
    FINE_GPIO_Init_Reg();
}

/**
 * @brief 反初始化FINE GPIO引脚
 * 
 * @details 将所有FINE引脚设置为输入模式，释放硬件资源。
 */
void FINE_GPIO_DeInit(void)
{
    uint16_t pins[6] = {
        g_fine_config.flmd0_pin, g_fine_config.flmd1_pin,
        g_fine_config.flmd2_pin, g_fine_config.flmd3_pin,
        g_fine_config.flclk_pin, g_fine_config.reset_pin
    };
    
    for (int i = 0; i < 6; i++) {
        uint32_t shift = (pins[i] % 8) * 4;
        GPIO_TypeDef* port;
        switch(i) {
            case 0: port = g_fine_config.flmd0_port; break;
            case 1: port = g_fine_config.flmd1_port; break;
            case 2: port = g_fine_config.flmd2_port; break;
            case 3: port = g_fine_config.flmd3_port; break;
            case 4: port = g_fine_config.flclk_port; break;
            case 5: port = g_fine_config.reset_port; break;
        }
        port->MODER &= ~(0x3UL << shift);
    }
}

/**
 * @brief 发送一位数据
 * @param bit: 要发送的位(1或0)
 * 
 * @note FINE协议在FLCLK上升沿采样数据
 */
void FINE_SendBit(uint8_t bit)
{
    if (bit) {
        FINE_FLMD0_HIGH();
    } else {
        FINE_FLMD0_LOW();
    }
    
    FINE_DelayNs(g_fine_config.tick_ns);
    
    FINE_CLK_HIGH();
    FINE_DelayNs(g_fine_config.tick_ns);
    
    FINE_CLK_LOW();
    FINE_DelayNs(g_fine_config.tick_ns);
}

/**
 * @brief 接收一位数据
 * @return 接收到的位(1或0)
 * 
 * @note FINE协议在FLCLK上升沿采样数据
 */
uint8_t FINE_ReceiveBit(void)
{
    uint8_t bit;
    
    FINE_DATA_Mode_In();
    FINE_DelayNs(g_fine_config.tick_ns);
    
    FINE_CLK_HIGH();
    FINE_DelayNs(g_fine_config.tick_ns);
    
    bit = FINE_DATA_READ() ? 1 : 0;
    
    FINE_CLK_LOW();
    FINE_DelayNs(g_fine_config.tick_ns);
    
    FINE_DATA_Mode_Out();
    
    return bit;
}

/**
 * @brief 写入一个字节
 * @param data: 要写入的字节
 * @return HAL状态
 * 
 * @note FINE发送字节时LSB优先
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
 * @brief 读取一个字节
 * @return 读取的字节
 * 
 * @note FINE接收字节时LSB优先
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
 * @brief 写入一个16位字
 * @param data: 要写入的16位数据
 * @return HAL状态
 * 
 * @note FINE发送字时LSB优先，先发送低字节
 */
HAL_StatusTypeDef FINE_WriteWord(uint16_t data)
{
    FINE_WriteByte(data & 0xFF);
    FINE_WriteByte((data >> 8) & 0xFF);
    
    return HAL_OK;
}

/**
 * @brief 读取一个16位字
 * @return 读取的16位数据
 * 
 * @note FINE接收字时LSB优先，先接收低字节
 */
uint16_t FINE_ReadWord(void)
{
    uint16_t data = 0;
    
    data = FINE_ReadByte();
    data |= (uint16_t)FINE_ReadByte() << 8;
    
    return data;
}

/**
 * @brief 写入一个32位双字
 * @param data: 要写入的32位数据
 * @return HAL状态
 * 
 * @note FINE发送双字时LSB优先，先发送低字节
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
 * @brief 读取一个32位双字
 * @return 读取的32位数据
 * 
 * @note FINE接收双字时LSB优先，先接收低字节
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
 * @brief 进入FINE编程模式
 * @return HAL状态
 * 
 * @details 执行FINE进入序列：
 *          1. 拉低RESET
 *          2. 设置FLMD[3:0]为高电平
 *          3. 释放RESET
 *          4. 发送同步时钟序列
 *          5. 发送读ID命令验证连接
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
        FINE_CLK_HIGH();
        FINE_DelayNs(g_fine_config.tick_ns);
        FINE_CLK_LOW();
        FINE_DelayNs(g_fine_config.tick_ns);
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
 * @brief 退出FINE编程模式
 * @return HAL状态
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
 * @brief 复位目标设备
 * @return HAL状态
 */
HAL_StatusTypeDef FINE_Reset(void)
{
    FINE_RESET_LOW();
    FINE_DelayUs(10);
    FINE_RESET_HIGH();
    FINE_DelayUs(10);
    
    return HAL_OK;
}

/**
 * @brief 读取设备ID信息
 * @return HAL状态
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
 * @brief 读取内存
 * @param addr: 源内存地址
 * @param data: 数据缓冲区指针
 * @param size: 数据大小(字节)
 * @return HAL状态
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
 * @brief 写入内存
 * @param addr: 目标内存地址
 * @param data: 数据缓冲区指针
 * @param size: 数据大小(字节)
 * @return HAL状态
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
 * @param addr: 扇区地址
 * @return HAL状态
 */
HAL_StatusTypeDef FINE_EraseSector(uint32_t addr)
{
    FINE_WriteByte(FINE_CMD_ERASE);
    FINE_WriteDWord(addr);
    
    return HAL_OK;
}

/**
 * @brief 擦除整片芯片
 * @return HAL状态
 */
HAL_StatusTypeDef FINE_EraseChip(void)
{
    FINE_WriteByte(FINE_CMD_ERASE);
    FINE_WriteDWord(0x00000000);
    
    return HAL_OK;
}

/**
 * @brief 验证内存数据
 * @param addr: 起始地址
 * @param data: 预期数据缓冲区
 * @param size: 数据大小(字节)
 * @return HAL状态(HAL_OK表示验证通过，HAL_ERROR表示验证失败)
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
 * @return 设备代码
 */
uint8_t FINE_GetDeviceCode(void)
{
    return g_fine_state.device_code;
}

/**
 * @brief 获取产品代码
 * @return 产品代码
 */
uint16_t FINE_GetProductCode(void)
{
    return g_fine_state.product_code;
}

/**
 * @brief 获取芯片ID
 * @return 芯片ID
 */
uint32_t FINE_GetChipID(void)
{
    return g_fine_state.chip_id;
}