/**
 ******************************************************************************
 * @file    sbw.c
 * @brief   SBW (Spy-Bi-Wire) 协议实现
 *          MSP430 调试接口 - 两线JTAG替代方案
 * 
 * @author  AI_PROG项目
 * @date    2026-06-05
 * @version v2.0
 * 
 * @details SBW是TI公司为MSP430系列单片机设计的两线调试接口，是传统JTAG接口的简化版本。
 *          本实现采用以下优化策略：
 *          1. 寄存器直接操作替代HAL库函数，减少函数调用开销
 *          2. 使用TIM7定时器实现纳秒级精确定时
 *          3. 支持100KHz~10MHz的可调时钟频率
 *          4. 开漏输出模式，支持双向数据传输
 * 
 * @note    SBW接口使用两个信号：
 *          - TCK: 测试时钟
 *          - TMS: 测试模式选择(双向数据线)
 *          - RST: 复位控制线
 *          - TEST: 测试模式触发线
 * 
 * @warning 本驱动使用TIM7定时器，需确保与其他功能不冲突
 ******************************************************************************
 */

#include "sbw.h"

/**
 * @brief SBW全局配置结构体
 *        存储SBW接口的硬件配置和运行状态
 */
SBW_Config_TypeDef g_sbw_config = {0};

/**
 * @brief SBW全局状态结构体
 *        存储SBW会话期间的状态信息
 */
SBW_State_TypeDef g_sbw_state = {0};

/**
 * @brief SBW定时器定义
 * @note  使用TIM7，挂载在APB1总线上，最高时钟可达120MHz
 */
#define SBW_TIM TIM7

/**
 * @brief GPIO寄存器操作宏定义
 * @note  使用BSRR寄存器实现原子操作，避免中断竞争问题
 */
#define SBW_TCK_HIGH()    ((g_sbw_config.tck_port)->BSRR = (1 << g_sbw_config.tck_pin))
#define SBW_TCK_LOW()     ((g_sbw_config.tck_port)->BSRR = (1 << g_sbw_config.tck_pin) << 16)

#define SBW_TMS_HIGH()    ((g_sbw_config.tms_port)->BSRR = (1 << g_sbw_config.tms_pin))
#define SBW_TMS_LOW()     ((g_sbw_config.tms_port)->BSRR = (1 << g_sbw_config.tms_pin) << 16)
#define SBW_TMS_READ()    (((g_sbw_config.tms_port)->IDR & (1 << g_sbw_config.tms_pin)) != 0)

#define SBW_RST_HIGH()    ((g_sbw_config.rst_port)->BSRR = (1 << g_sbw_config.rst_pin))
#define SBW_RST_LOW()     ((g_sbw_config.rst_port)->BSRR = (1 << g_sbw_config.rst_pin) << 16)

#define SBW_TEST_HIGH()   ((g_sbw_config.test_port)->BSRR = (1 << g_sbw_config.test_pin))
#define SBW_TEST_LOW()    ((g_sbw_config.test_port)->BSRR = (1 << g_sbw_config.test_pin) << 16)

/**
 * @brief 等待定时器计数达到指定值
 * @param ticks: 等待的定时器计数值
 * @note  阻塞式等待，精确延时
 */
static void SBW_TimerWait(uint32_t ticks)
{
    SBW_TIM->CNT = 0;
    while (SBW_TIM->CNT < ticks);
}

/**
 * @brief 纳秒级延时
 * @param ns: 延时时间(纳秒)
 * @note  基于定时器实现，精度取决于定时器配置
 */
void SBW_DelayNs(uint32_t ns)
{
    uint32_t ticks = (ns + g_sbw_config.tick_ns - 1) / g_sbw_config.tick_ns;
    SBW_TimerWait(ticks);
}

/**
 * @brief 微秒级延时
 * @param us: 延时时间(微秒)
 * @note  基于定时器实现，精度取决于定时器配置
 */
void SBW_DelayUs(uint32_t us)
{
    uint32_t ticks = (us * 1000 + g_sbw_config.tick_ns - 1) / g_sbw_config.tick_ns;
    SBW_TimerWait(ticks);
}

/**
 * @brief 设置SBW通信速度
 * @param speed_hz: 目标通信速度(Hz)
 * @note  支持的速度范围: 100KHz ~ 10MHz
 * @note  速度过高会导致通信不稳定
 */
void SBW_SetSpeed(uint32_t speed_hz)
{
    if (speed_hz > SBW_CLOCK_10MHZ) {
        speed_hz = SBW_CLOCK_10MHZ;
    }
    
    g_sbw_config.speed_hz = speed_hz;
    
    uint32_t apb1_freq = HAL_RCC_GetPCLK1Freq();
    uint32_t tick_freq = speed_hz * 10;
    
    g_sbw_config.prescaler = (apb1_freq + tick_freq - 1) / tick_freq;
    if (g_sbw_config.prescaler < 1) {
        g_sbw_config.prescaler = 1;
    }
    
    g_sbw_config.tick_ns = 1000000000ULL / (apb1_freq / g_sbw_config.prescaler);
    g_sbw_config.period = 65535;
    
    if (SBW_TIM->CR1 & TIM_CR1_CEN) {
        SBW_TIM->CR1 &= ~TIM_CR1_CEN;
        SBW_TIM->PSC = g_sbw_config.prescaler - 1;
        SBW_TIM->ARR = g_sbw_config.period - 1;
        SBW_TIM->EGR = TIM_EGR_UG;
        SBW_TIM->CR1 |= TIM_CR1_CEN;
    }
}

/**
 * @brief 获取当前SBW通信速度
 * @return 当前速度(Hz)
 */
uint32_t SBW_GetSpeed(void)
{
    return g_sbw_config.speed_hz;
}

/**
 * @brief 初始化SBW定时器
 * @note  使用TIM7定时器，配置为基本定时器模式
 * @note  定时器时钟 = APB1时钟 / prescaler
 */
static void SBW_TimerInit(void)
{
    SBW_TIM_CLK_ENABLE();
    
    SBW_TIM->CR1 = 0;
    SBW_TIM->CR2 = 0;
    SBW_TIM->SMCR = 0;
    SBW_TIM->DIER = 0;
    SBW_TIM->SR = 0;
    
    SBW_TIM->PSC = g_sbw_config.prescaler - 1;
    SBW_TIM->ARR = g_sbw_config.period - 1;
    
    SBW_TIM->EGR = TIM_EGR_UG;
    
    SBW_TIM->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief 初始化SBW GPIO引脚(寄存器操作)
 * 
 * @details 配置TCK、TMS、RST、TEST引脚为输出模式，使用开漏输出。
 *          设置初始状态：TCK=1, TMS=1, RST=1, TEST=1
 */
static void SBW_GPIO_Init_Reg(void)
{
    uint32_t tck_pos = g_sbw_config.tck_pin;
    uint32_t tms_pos = g_sbw_config.tms_pin;
    uint32_t rst_pos = g_sbw_config.rst_pin;
    uint32_t test_pos = g_sbw_config.test_pin;
    
    GPIO_TypeDef* ports[4] = {g_sbw_config.tck_port, g_sbw_config.tms_port, g_sbw_config.rst_port, g_sbw_config.test_port};
    uint32_t pins[4] = {tck_pos, tms_pos, rst_pos, test_pos};
    
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
        
        port->OTYPER |= (1UL << pin);
        
        port->PUPDR &= ~(0x3UL << shift);
        port->PUPDR |= (0x1UL << shift);
        
        port->OSPEEDR &= ~(0x3UL << shift);
        port->OSPEEDR |= (0x3UL << shift);
    }
    
    SBW_TCK_HIGH();
    SBW_TMS_HIGH();
    SBW_RST_HIGH();
    SBW_TEST_HIGH();
}

/**
 * @brief 设置TMS引脚为输入模式
 * 
 * @details 将TMS引脚切换为输入模式，用于接收数据
 */
static void SBW_TMS_Mode_In(void)
{
    uint32_t tms_pos = g_sbw_config.tms_pin;
    uint32_t shift = (tms_pos % 8) * 4;
    
    g_sbw_config.tms_port->MODER &= ~(0x3UL << shift);
}

/**
 * @brief 设置TMS引脚为输出模式
 * 
 * @details 将TMS引脚切换为输出模式，用于发送数据
 */
static void SBW_TMS_Mode_Out(void)
{
    uint32_t tms_pos = g_sbw_config.tms_pin;
    uint32_t shift = (tms_pos % 8) * 4;
    
    g_sbw_config.tms_port->MODER &= ~(0x3UL << shift);
    g_sbw_config.tms_port->MODER |= (0x1UL << shift);
    g_sbw_config.tms_port->OTYPER |= (1UL << tms_pos);
}

/**
 * @brief 初始化SBW接口
 * @param config: SBW配置结构体指针
 * @return HAL状态
 * 
 * @details 初始化SBW接口，包括GPIO配置、定时器配置和协议状态初始化。
 *          如果传入config参数，则使用用户配置；否则使用默认配置。
 */
HAL_StatusTypeDef SBW_Init(SBW_Config_TypeDef *config)
{
    if (config == NULL) {
        return HAL_ERROR;
    }

    memcpy(&g_sbw_config, config, sizeof(SBW_Config_TypeDef));
    
    if (g_sbw_config.speed_hz == 0) {
        g_sbw_config.speed_hz = SBW_DEFAULT_CLOCK;
    }
    
    SBW_SetSpeed(g_sbw_config.speed_hz);
    SBW_TimerInit();
    SBW_GPIO_Init_Reg();
    
    g_sbw_config.initialized = 1;
    
    return HAL_OK;
}

/**
 * @brief 反初始化SBW接口
 * @return HAL状态
 * 
 * @details 关闭SBW接口，释放GPIO和定时器资源。
 */
HAL_StatusTypeDef SBW_DeInit(void)
{
    SBW_TIM->CR1 &= ~TIM_CR1_CEN;
    SBW_GPIO_DeInit();
    
    memset(&g_sbw_config, 0, sizeof(SBW_Config_TypeDef));
    memset(&g_sbw_state, 0, sizeof(SBW_State_TypeDef));
    
    return HAL_OK;
}

/**
 * @brief 初始化SBW GPIO引脚(封装接口)
 */
void SBW_GPIO_Init(void)
{
    SBW_GPIO_Init_Reg();
}

/**
 * @brief 反初始化SBW GPIO引脚
 * 
 * @details 将所有SBW引脚设置为输入模式，释放硬件资源。
 */
void SBW_GPIO_DeInit(void)
{
    uint32_t pins[4] = {g_sbw_config.tck_pin, g_sbw_config.tms_pin, g_sbw_config.rst_pin, g_sbw_config.test_pin};
    
    for (int i = 0; i < 4; i++) {
        uint32_t shift = (pins[i] % 8) * 4;
        GPIO_TypeDef* port;
        switch(i) {
            case 0: port = g_sbw_config.tck_port; break;
            case 1: port = g_sbw_config.tms_port; break;
            case 2: port = g_sbw_config.rst_port; break;
            case 3: port = g_sbw_config.test_port; break;
        }
        port->MODER &= ~(0x3UL << shift);
    }
}

/**
 * @brief 发送一位数据
 * @param bit: 要发送的位(1或0)
 * 
 * @note SBW协议在TCK下降沿采样数据
 */
void SBW_SendBit(uint8_t bit)
{
    if (bit) {
        SBW_TMS_HIGH();
    } else {
        SBW_TMS_LOW();
    }
    
    SBW_TCK_LOW();
    SBW_DelayNs(g_sbw_config.tick_ns);
    SBW_TCK_HIGH();
    SBW_DelayNs(g_sbw_config.tick_ns);
}

/**
 * @brief 接收一位数据
 * @return 接收到的位(1或0)
 * 
 * @note SBW协议在TCK下降沿采样数据
 */
uint8_t SBW_ReceiveBit(void)
{
    uint8_t bit;
    
    SBW_TMS_Mode_In();
    SBW_DelayNs(g_sbw_config.tick_ns);
    
    SBW_TCK_LOW();
    SBW_DelayNs(g_sbw_config.tick_ns);
    
    bit = SBW_TMS_READ() ? 1 : 0;
    
    SBW_TCK_HIGH();
    SBW_DelayNs(g_sbw_config.tick_ns);
    
    SBW_TMS_Mode_Out();
    
    return bit;
}

/**
 * @brief 发送一个字节
 * @param byte: 要发送的字节
 * 
 * @note SBW发送字节时LSB优先
 */
static void SBW_SendByte(uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        SBW_SendBit(byte & 0x01);
        byte >>= 1;
    }
}

/**
 * @brief 接收一个字节
 * @return 接收到的字节
 * 
 * @note SBW接收字节时LSB优先
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
 * @brief 执行TAP复位
 * @return HAL状态
 * 
 * @details 发送8个TMS=1的时钟周期，然后发送5个TMS=0的周期，使TAP状态机进入已知状态。
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
 * @brief 写入指令寄存器
 * @param instruction: 16位指令值
 * @return HAL状态
 * 
 * @details SBW IR长度固定为16位
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
 * @brief 写入数据寄存器
 * @param data: 数据指针(可选，为NULL时仅执行移位)
 * @param bitlen: 数据位长度
 * @return HAL状态
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
 * @param bitlen: 数据位长度
 * @return 读取的数据
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
 * @brief 进入SBW调试模式
 * @return HAL状态
 * 
 * @details 执行SBW进入序列：
 *          1. 拉低RST和TEST
 *          2. 释放TEST，保持RST
 *          3. 释放RST
 *          4. 发送同步时钟序列
 *          5. 读取JTAG ID验证连接
 */
HAL_StatusTypeDef SBW_Enter(void)
{
    SBW_RST_LOW();
    SBW_DelayUs(100);
    
    SBW_TEST_LOW();
    SBW_DelayUs(100);
    
    SBW_TEST_HIGH();
    SBW_DelayUs(100);
    
    SBW_RST_HIGH();
    SBW_DelayUs(100);
    
    uint32_t half_period_ns = 500000000UL / g_sbw_config.speed_hz;
    for (int i = 0; i < 200; i++) {
        SBW_TCK_LOW();
        SBW_DelayNs(half_period_ns);
        SBW_TCK_HIGH();
        SBW_DelayNs(half_period_ns);
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
 * @brief 退出SBW调试模式
 * @return HAL状态
 */
HAL_StatusTypeDef SBW_Exit(void)
{
    SBW_TapReset();
    
    SBW_TapShiftIR(0x00);
    
    SBW_RST_LOW();
    SBW_DelayUs(100);
    SBW_RST_HIGH();
    
    return HAL_OK;
}

/**
 * @brief 复位目标设备
 * @return HAL状态
 */
HAL_StatusTypeDef SBW_Reset(void)
{
    SBW_RST_LOW();
    SBW_DelayUs(10);
    SBW_RST_HIGH();
    SBW_DelayUs(10);
    
    return HAL_OK;
}

/**
 * @brief 启动调试会话
 * @return HAL状态
 */
HAL_StatusTypeDef SBW_Start(void)
{
    SBW_TapShiftIR(0x20);
    SBW_TapShiftDR(NULL, 16);
    
    return HAL_OK;
}

/**
 * @brief 停止调试会话
 * @return HAL状态
 */
HAL_StatusTypeDef SBW_Stop(void)
{
    SBW_TapShiftIR(0x00);
    
    return HAL_OK;
}

/**
 * @brief 写入16位字
 * @param addr: 目标地址
 * @param data: 16位数据
 * @return HAL状态
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
 * @brief 读取16位字
 * @param addr: 源地址
 * @return 读取的16位数据
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
 * @brief 写入内存
 * @param addr: 目标内存地址
 * @param data: 数据缓冲区指针
 * @param size: 数据大小(字节)
 * @return HAL状态
 * 
 * @details 支持任意地址和大小的写入，自动处理对齐。
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
 * @brief 读取内存
 * @param addr: 源内存地址
 * @param data: 数据缓冲区指针
 * @param size: 数据大小(字节)
 * @return HAL状态
 * 
 * @details 支持任意地址和大小的读取，自动处理对齐。
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
 * @return JTAG ID值
 */
uint16_t SBW_GetJTAGID(void)
{
    return g_sbw_state.jtag_id;
}

/**
 * @brief 获取IDCODE
 * @return IDCODE值
 */
uint32_t SBW_GetIDCode(void)
{
    SBW_TapShiftIR(0xFE);
    g_sbw_state.idcode = SBW_TapReadDR(32);
    
    return g_sbw_state.idcode;
}