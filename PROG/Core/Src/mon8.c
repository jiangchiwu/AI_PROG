/**
 ******************************************************************************
 * @file    mon8.c
 * @brief   MON8 接口协议实现
 *          Freescale HC08/HC05 8位单片机调试接口
 * 
 * @author  AI_PROG项目
 * @date    2026-06-05
 * @version v2.0
 * 
 * @details MON8是Freescale/NXP为HC08系列8位单片机设计的监控模式调试接口。
 *          本实现采用以下优化策略：
 *          1. 寄存器直接操作替代HAL库函数，减少函数调用开销
 *          2. 使用TIM12定时器实现纳秒级精确定时
 *          3. 支持100KHz~10MHz的可调时钟频率
 *          4. 开漏输出模式，支持双向数据传输
 * 
 * @note    MON8接口使用以下信号：
 *          - BKPT: 断点/双向数据通信线
 *          - RST: 复位控制线
 *          - PTX: 编程/测试数据输出
 *          - PRX: 编程/测试数据输入
 * 
 * @warning 本驱动使用TIM12定时器，需确保与其他功能不冲突
 ******************************************************************************
 */

#include "mon8.h"

/**
 * @brief MON8全局配置结构体
 *        存储MON8接口的硬件配置和运行状态
 */
MON8_Config_TypeDef g_mon8_config = {0};

/**
 * @brief MON8全局状态结构体
 *        存储MON8会话期间的状态信息
 */
MON8_State_TypeDef g_mon8_state = {0};

/**
 * @brief MON8定时器定义
 * @note  使用TIM12，挂载在APB1总线上，最高时钟可达120MHz
 */
#define MON8_TIM TIM12

/**
 * @brief GPIO寄存器操作宏定义
 * @note  使用BSRR寄存器实现原子操作，避免中断竞争问题
 */
#define MON8_BKPT_HIGH()    ((g_mon8_config.bkpt_port)->BSRR = (1 << g_mon8_config.bkpt_pin))
#define MON8_BKPT_LOW()     ((g_mon8_config.bkpt_port)->BSRR = (1 << g_mon8_config.bkpt_pin) << 16)
#define MON8_BKPT_READ()    (((g_mon8_config.bkpt_port)->IDR & (1 << g_mon8_config.bkpt_pin)) != 0)

#define MON8_RST_HIGH()     ((g_mon8_config.rst_port)->BSRR = (1 << g_mon8_config.rst_pin))
#define MON8_RST_LOW()      ((g_mon8_config.rst_port)->BSRR = (1 << g_mon8_config.rst_pin) << 16)

#define MON8_TX_HIGH()      ((g_mon8_config.ptx_port)->BSRR = (1 << g_mon8_config.ptx_pin))
#define MON8_TX_LOW()       ((g_mon8_config.ptx_port)->BSRR = (1 << g_mon8_config.ptx_pin) << 16)
#define MON8_RX_READ()      (((g_mon8_config.prx_port)->IDR & (1 << g_mon8_config.prx_pin)) != 0)

/**
 * @brief 等待定时器计数达到指定值
 * @param ticks: 等待的定时器计数值
 * @note  阻塞式等待，精确延时
 */
static void MON8_TimerWait(uint32_t ticks)
{
    MON8_TIM->CNT = 0;
    while (MON8_TIM->CNT < ticks);
}

/**
 * @brief 纳秒级延时
 * @param ns: 延时时间(纳秒)
 * @note  基于定时器实现，精度取决于定时器配置
 */
void MON8_DelayNs(uint32_t ns)
{
    uint32_t ticks = (ns + g_mon8_config.tick_ns - 1) / g_mon8_config.tick_ns;
    MON8_TimerWait(ticks);
}

/**
 * @brief 微秒级延时
 * @param us: 延时时间(微秒)
 * @note  基于定时器实现，精度取决于定时器配置
 */
void MON8_DelayUs(uint32_t us)
{
    uint32_t ticks = (us * 1000 + g_mon8_config.tick_ns - 1) / g_mon8_config.tick_ns;
    MON8_TimerWait(ticks);
}

/**
 * @brief 设置MON8通信速度
 * @param speed_hz: 目标通信速度(Hz)
 * @note  支持的速度范围: 100KHz ~ 10MHz
 * @note  速度过高会导致通信不稳定
 */
void MON8_SetSpeed(uint32_t speed_hz)
{
    if (speed_hz > MON8_CLOCK_10MHZ) {
        speed_hz = MON8_CLOCK_10MHZ;
    }
    
    g_mon8_config.speed_hz = speed_hz;
    
    uint32_t apb1_freq = HAL_RCC_GetPCLK1Freq();
    uint32_t tick_freq = speed_hz * 10;
    
    g_mon8_config.prescaler = (apb1_freq + tick_freq - 1) / tick_freq;
    if (g_mon8_config.prescaler < 1) {
        g_mon8_config.prescaler = 1;
    }
    
    g_mon8_config.tick_ns = 1000000000ULL / (apb1_freq / g_mon8_config.prescaler);
    g_mon8_config.period = 65535;
    
    if (MON8_TIM->CR1 & TIM_CR1_CEN) {
        MON8_TIM->CR1 &= ~TIM_CR1_CEN;
        MON8_TIM->PSC = g_mon8_config.prescaler - 1;
        MON8_TIM->ARR = g_mon8_config.period - 1;
        MON8_TIM->EGR = TIM_EGR_UG;
        MON8_TIM->CR1 |= TIM_CR1_CEN;
    }
}

/**
 * @brief 获取当前MON8通信速度
 * @return 当前速度(Hz)
 */
uint32_t MON8_GetSpeed(void)
{
    return g_mon8_config.speed_hz;
}

/**
 * @brief 初始化MON8定时器
 * @note  使用TIM12定时器，配置为基本定时器模式
 * @note  定时器时钟 = APB1时钟 / prescaler
 */
static void MON8_TimerInit(void)
{
    MON8_TIM_CLK_ENABLE();
    
    MON8_TIM->CR1 = 0;
    MON8_TIM->CR2 = 0;
    MON8_TIM->SMCR = 0;
    MON8_TIM->DIER = 0;
    MON8_TIM->SR = 0;
    
    MON8_TIM->PSC = g_mon8_config.prescaler - 1;
    MON8_TIM->ARR = g_mon8_config.period - 1;
    
    MON8_TIM->EGR = TIM_EGR_UG;
    
    MON8_TIM->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief 初始化MON8 GPIO引脚(寄存器操作)
 * 
 * @details 配置BKPT、RST、PTX、PRX引脚。
 *          BKPT和PTX使用开漏输出模式，RST使用推挽输出，PRX使用输入模式。
 *          设置初始状态：BKPT=1, TX=1, RST=1
 */
static void MON8_GPIO_Init_Reg(void)
{
    GPIO_TypeDef* ports[4] = {g_mon8_config.bkpt_port, g_mon8_config.rst_port, g_mon8_config.ptx_port, g_mon8_config.prx_port};
    uint16_t pins[4] = {g_mon8_config.bkpt_pin, g_mon8_config.rst_pin, g_mon8_config.ptx_pin, g_mon8_config.prx_pin};
    uint8_t is_open_drain[4] = {1, 0, 1, 0};
    
    for (int i = 0; i < 4; i++) {
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
        
        if (is_open_drain[i]) {
            port->OTYPER |= (1UL << pin);
            port->PUPDR &= ~(0x3UL << shift);
            port->PUPDR |= (0x1UL << shift);
        } else {
            port->OTYPER &= ~(1UL << pin);
        }
        
        port->OSPEEDR &= ~(0x3UL << shift);
        port->OSPEEDR |= (0x3UL << shift);
    }
    
    MON8_BKPT_HIGH();
    MON8_TX_HIGH();
    MON8_RST_HIGH();
}

/**
 * @brief 设置BKPT引脚为输入模式
 * 
 * @details 将BKPT引脚切换为输入模式，用于接收数据
 */
static void MON8_BKPT_Mode_In(void)
{
    uint32_t pin = g_mon8_config.bkpt_pin;
    uint32_t shift = (pin % 8) * 4;
    
    g_mon8_config.bkpt_port->MODER &= ~(0x3UL << shift);
}

/**
 * @brief 设置BKPT引脚为输出模式
 * 
 * @details 将BKPT引脚切换为输出模式，用于发送数据
 */
static void MON8_BKPT_Mode_Out(void)
{
    uint32_t pin = g_mon8_config.bkpt_pin;
    uint32_t shift = (pin % 8) * 4;
    
    g_mon8_config.bkpt_port->MODER &= ~(0x3UL << shift);
    g_mon8_config.bkpt_port->MODER |= (0x1UL << shift);
    g_mon8_config.bkpt_port->OTYPER |= (1UL << pin);
}

/**
 * @brief 初始化MON8接口
 * @param config: MON8配置结构体指针
 * @return HAL状态
 * 
 * @details 初始化MON8接口，包括GPIO配置、定时器配置和协议状态初始化。
 *          如果传入config参数，则使用用户配置；否则使用默认配置。
 */
HAL_StatusTypeDef MON8_Init(MON8_Config_TypeDef *config)
{
    if (config == NULL) {
        return HAL_ERROR;
    }

    memcpy(&g_mon8_config, config, sizeof(MON8_Config_TypeDef));
    
    if (g_mon8_config.speed_hz == 0) {
        g_mon8_config.speed_hz = MON8_DEFAULT_CLOCK;
    }
    
    MON8_SetSpeed(g_mon8_config.speed_hz);
    MON8_TimerInit();
    MON8_GPIO_Init_Reg();
    
    g_mon8_config.initialized = 1;
    
    return HAL_OK;
}

/**
 * @brief 反初始化MON8接口
 * @return HAL状态
 * 
 * @details 关闭MON8接口，释放GPIO和定时器资源。
 */
HAL_StatusTypeDef MON8_DeInit(void)
{
    MON8_TIM->CR1 &= ~TIM_CR1_CEN;
    MON8_GPIO_DeInit();
    
    memset(&g_mon8_config, 0, sizeof(MON8_Config_TypeDef));
    memset(&g_mon8_state, 0, sizeof(MON8_State_TypeDef));
    
    return HAL_OK;
}

/**
 * @brief 初始化MON8 GPIO引脚(封装接口)
 */
void MON8_GPIO_Init(void)
{
    MON8_GPIO_Init_Reg();
}

/**
 * @brief 反初始化MON8 GPIO引脚
 * 
 * @details 将所有MON8引脚设置为输入模式，释放硬件资源。
 */
void MON8_GPIO_DeInit(void)
{
    uint16_t pins[4] = {g_mon8_config.bkpt_pin, g_mon8_config.rst_pin, g_mon8_config.ptx_pin, g_mon8_config.prx_pin};
    
    for (int i = 0; i < 4; i++) {
        uint32_t shift = (pins[i] % 8) * 4;
        GPIO_TypeDef* port;
        switch(i) {
            case 0: port = g_mon8_config.bkpt_port; break;
            case 1: port = g_mon8_config.rst_port; break;
            case 2: port = g_mon8_config.ptx_port; break;
            case 3: port = g_mon8_config.prx_port; break;
        }
        port->MODER &= ~(0x3UL << shift);
    }
}

/**
 * @brief 发送一位数据
 * @param bit: 要发送的位(1或0)
 * 
 * @note MON8协议在RST下降沿采样数据
 */
void MON8_SendBit(uint8_t bit)
{
    if (bit) {
        MON8_TX_HIGH();
    } else {
        MON8_TX_LOW();
    }
    
    MON8_DelayNs(g_mon8_config.tick_ns);
    
    MON8_RST_LOW();
    MON8_DelayNs(g_mon8_config.tick_ns);
    
    MON8_RST_HIGH();
    MON8_DelayNs(g_mon8_config.tick_ns);
}

/**
 * @brief 接收一位数据
 * @return 接收到的位(1或0)
 * 
 * @note MON8协议在RST下降沿采样数据
 */
uint8_t MON8_ReceiveBit(void)
{
    uint8_t bit;
    
    MON8_RST_LOW();
    MON8_DelayNs(g_mon8_config.tick_ns);
    
    bit = MON8_RX_READ() ? 1 : 0;
    
    MON8_RST_HIGH();
    MON8_DelayNs(g_mon8_config.tick_ns);
    
    return bit;
}

/**
 * @brief 写入一个字节
 * @param data: 要写入的字节
 * @return HAL状态
 * 
 * @note MON8发送字节时MSB优先
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
 * @brief 读取一个字节
 * @return 读取的字节
 * 
 * @note MON8接收字节时MSB优先
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
 * @brief 进入MON8监控模式
 * @return HAL状态
 * 
 * @details 执行MON8进入序列：
 *          1. 拉低RST和BKPT
 *          2. 释放BKPT，保持RST
 *          3. 释放RST
 *          4. 发送读版本命令验证连接
 */
HAL_StatusTypeDef MON8_Enter(void)
{
    MON8_RST_LOW();
    MON8_BKPT_LOW();
    MON8_DelayUs(100);
    
    MON8_BKPT_HIGH();
    MON8_DelayUs(100);
    
    MON8_RST_HIGH();
    MON8_DelayUs(100);
    
    MON8_WriteByte(MON8_CMD_READ_VER);
    
    g_mon8_state.version = MON8_ReadByte();
    
    if (g_mon8_state.version == 0) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief 退出MON8监控模式
 * @return HAL状态
 */
HAL_StatusTypeDef MON8_Exit(void)
{
    MON8_RST_LOW();
    MON8_DelayUs(100);
    MON8_RST_HIGH();
    
    return HAL_OK;
}

/**
 * @brief 复位目标设备
 * @return HAL状态
 */
HAL_StatusTypeDef MON8_Reset(void)
{
    MON8_RST_LOW();
    MON8_DelayUs(10);
    MON8_RST_HIGH();
    MON8_DelayUs(10);
    
    return HAL_OK;
}

/**
 * @brief 运行目标程序
 * @param addr: 起始地址
 * @return HAL状态
 */
HAL_StatusTypeDef MON8_Run(uint16_t addr)
{
    MON8_WriteByte(MON8_CMD_RUN);
    MON8_WriteByte((addr >> 8) & 0xFF);
    MON8_WriteByte(addr & 0xFF);
    
    return HAL_OK;
}

/**
 * @brief 停止目标程序
 * @return HAL状态
 */
HAL_StatusTypeDef MON8_Stop(void)
{
    MON8_WriteByte(MON8_CMD_STOP);
    
    return HAL_OK;
}

/**
 * @brief 读取内存
 * @param addr: 源内存地址
 * @param data: 数据缓冲区指针
 * @param size: 数据大小(字节)
 * @return HAL状态
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
 * @brief 写入内存
 * @param addr: 目标内存地址
 * @param data: 数据缓冲区指针
 * @param size: 数据大小(字节)
 * @return HAL状态
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
 * @brief 擦除内存
 * @param addr: 起始地址
 * @param size: 大小(字节)
 * @return HAL状态
 */
HAL_StatusTypeDef MON8_Erase(uint16_t addr, uint16_t size)
{
    MON8_WriteByte(MON8_CMD_ERASE);
    MON8_WriteByte((addr >> 8) & 0xFF);
    MON8_WriteByte(addr & 0xFF);
    
    return HAL_OK;
}

/**
 * @brief 加密设备
 * @return HAL状态
 */
HAL_StatusTypeDef MON8_Secure(void)
{
    MON8_WriteByte(MON8_CMD_SECURE);
    
    return HAL_OK;
}

/**
 * @brief 获取版本号
 * @return MON8版本号
 */
uint8_t MON8_GetVersion(void)
{
    return g_mon8_state.version;
}

/**
 * @brief 获取设备ID
 * @return 设备ID
 */
uint8_t MON8_GetDeviceID(void)
{
    MON8_ReadMem(0xFFFE, &g_mon8_state.device_id, 1);
    return g_mon8_state.device_id;
}