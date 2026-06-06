/**
 ******************************************************************************
 * @file    bdm.c
 * @brief   BDM (Background Debug Mode) 接口实现
 *          NXP/Freescale HC08/HCS08/HCS12系列调试接口
 *          支持最高10MHz时钟频率，使用寄存器操作和定时器精确定时
 * 
 * @author  AI_PROG项目
 * @date    2026-06-05
 * @version v2.0
 * 
 * @details BDM接口是Freescale/NXP单片机的调试接口，主要用于HC08、HCS08、HCS12等系列芯片。
 *          本实现采用以下优化策略：
 *          1. 寄存器直接操作替代HAL库函数，减少函数调用开销
 *          2. 使用TIM8定时器实现纳秒级精确定时
 *          3. 支持100KHz~10MHz的可调时钟频率
 *          4. 开漏输出模式，支持双向数据传输
 * 
 * @note    BDM接口使用两个信号：BKPT(断点)和RESET(复位)
 *          - BKPT: 双向数据/命令线
 *          - RESET: 复位控制线
 * 
 * @warning 本驱动使用TIM8定时器，需确保与其他功能不冲突
 ******************************************************************************
 */

#include "bdm.h"

/**
 * @brief BDM全局句柄，存储当前BDM配置和状态
 */
BDM_HandleTypeDef g_bdm_handle = {0};

/**
 * @brief BDM定时器定义
 * @note  使用TIM8，挂载在APB2总线上，最高时钟可达480MHz
 */
#define BDM_TIM TIM8

/**
 * @brief GPIO寄存器操作宏定义
 * @note  使用BSRR寄存器实现原子操作，避免中断竞争问题
 */
#define BDM_BKPT_PIN_MASK(hbdm)    (1 << ((hbdm)->bkpt_pin))      /* BKPT引脚位掩码 */
#define BDM_RESET_PIN_MASK(hbdm)   (1 << ((hbdm)->reset_pin))     /* RESET引脚位掩码 */

#define BDM_BKPT_HIGH_REG(hbdm)    ((hbdm)->bkpt_port->BSRR = BDM_BKPT_PIN_MASK(hbdm))        /* BKPT置高 */
#define BDM_BKPT_LOW_REG(hbdm)     ((hbdm)->bkpt_port->BSRR = BDM_BKPT_PIN_MASK(hbdm) << 16)  /* BKPT置低 */
#define BDM_RESET_HIGH_REG(hbdm)   ((hbdm)->reset_port->BSRR = BDM_RESET_PIN_MASK(hbdm))       /* RESET置高 */
#define BDM_RESET_LOW_REG(hbdm)    ((hbdm)->reset_port->BSRR = BDM_RESET_PIN_MASK(hbdm) << 16)  /* RESET置低 */
#define BDM_BKPT_READ_REG(hbdm)    (((hbdm)->bkpt_port->IDR & BDM_BKPT_PIN_MASK(hbdm)) != 0)   /* 读取BKPT引脚状态 */

/**
 * @brief 启动定时器计数
 * @param hbdm: BDM句柄指针
 * @note  定时器必须已初始化
 */
static inline void BDM_TimerStart(BDM_HandleTypeDef* hbdm)
{
    BDM_TIM->CNT = 0;
    BDM_TIM->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief 停止定时器计数
 * @param hbdm: BDM句柄指针
 */
static inline void BDM_TimerStop(BDM_HandleTypeDef* hbdm)
{
    BDM_TIM->CR1 &= ~TIM_CR1_CEN;
}

/**
 * @brief 等待定时器计数达到指定值
 * @param hbdm: BDM句柄指针
 * @param ticks: 等待的定时器计数值
 * @note  阻塞式等待，精确延时
 */
static inline void BDM_TimerWait(BDM_HandleTypeDef* hbdm, uint32_t ticks)
{
    BDM_TIM->CNT = 0;
    while (BDM_TIM->CNT < ticks);
}

/**
 * @brief 纳秒级延时
 * @param hbdm: BDM句柄指针
 * @param ns: 延时时间(纳秒)
 * @note  基于定时器实现，精度取决于定时器配置
 */
void BDM_TimerDelayNs(BDM_HandleTypeDef* hbdm, uint32_t ns)
{
    uint32_t ticks = (ns + hbdm->tick_ns - 1) / hbdm->tick_ns;
    BDM_TimerWait(hbdm, ticks);
}

/**
 * @brief 微秒级延时
 * @param hbdm: BDM句柄指针
 * @param us: 延时时间(微秒)
 * @note  基于定时器实现，精度取决于定时器配置
 */
void BDM_TimerDelayUs(BDM_HandleTypeDef* hbdm, uint32_t us)
{
    uint32_t ticks = (us * 1000 + hbdm->tick_ns - 1) / hbdm->tick_ns;
    BDM_TimerWait(hbdm, ticks);
}

/**
 * @brief 初始化BDM定时器
 * @param hbdm: BDM句柄指针
 * @note  使用TIM8定时器，配置为基本定时器模式
 * @note  定时器时钟 = APB2时钟 / prescaler
 */
static void BDM_TimerInit(BDM_HandleTypeDef* hbdm)
{
    /* 使能TIM8时钟 */
    BDM_TIM_CLK_ENABLE();
    
    /* 复位定时器配置 */
    BDM_TIM->CR1 = 0;
    BDM_TIM->CR2 = 0;
    BDM_TIM->SMCR = 0;
    BDM_TIM->DIER = 0;
    BDM_TIM->SR = 0;
    
    /* 设置预分频器和自动重装载值 */
    BDM_TIM->PSC = hbdm->prescaler - 1;
    BDM_TIM->ARR = hbdm->period - 1;
    
    /* 生成更新事件，应用新配置 */
    BDM_TIM->EGR = TIM_EGR_UG;
    
    /* 启动定时器 */
    BDM_TIM->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief 设置BDM通信速度
 * @param hbdm: BDM句柄指针
 * @param speed_hz: 目标通信速度(Hz)
 * @note  支持的速度范围: 100KHz ~ 10MHz
 * @note  速度过高会导致通信不稳定
 */
void BDM_SetSpeed(BDM_HandleTypeDef* hbdm, uint32_t speed_hz)
{
    /* 限制最大速度为10MHz */
    if (speed_hz > BDM_CLOCK_10MHZ) {
        speed_hz = BDM_CLOCK_10MHZ;
    }
    
    /* 保存当前速度 */
    hbdm->speed_hz = speed_hz;
    
    /* 获取APB2总线频率 */
    uint32_t apb2_freq = HAL_RCC_GetPCLK2Freq();
    
    /* 计算定时器时钟频率 (10倍于通信速度，提高分辨率) */
    uint32_t tick_freq = speed_hz * 10;
    
    /* 计算预分频器值 */
    hbdm->prescaler = (apb2_freq + tick_freq - 1) / tick_freq;
    if (hbdm->prescaler < 1) {
        hbdm->prescaler = 1;
    }
    
    /* 计算定时器tick对应的纳秒数 */
    hbdm->tick_ns = 1000000000ULL / (apb2_freq / hbdm->prescaler);
    hbdm->period = 65535;
    
    /* 如果定时器正在运行，动态更新配置 */
    if (BDM_TIM->CR1 & TIM_CR1_CEN) {
        BDM_TIM->CR1 &= ~TIM_CR1_CEN;
        BDM_TIM->PSC = hbdm->prescaler - 1;
        BDM_TIM->ARR = hbdm->period - 1;
        BDM_TIM->EGR = TIM_EGR_UG;
        BDM_TIM->CR1 |= TIM_CR1_CEN;
    }
}

/**
 * @brief 获取当前BDM通信速度
 * @param hbdm: BDM句柄指针
 * @return 当前速度(Hz)
 */
uint32_t BDM_GetSpeed(BDM_HandleTypeDef* hbdm)
{
    return hbdm->speed_hz;
}

static void BDM_GPIO_Init_Reg(BDM_HandleTypeDef* hbdm)
{
    uint32_t bkpt_pin_pos = hbdm->bkpt_pin;
    uint32_t reset_pin_pos = hbdm->reset_pin;
    
    GPIO_TypeDef* bkpt_port = hbdm->bkpt_port;
    GPIO_TypeDef* reset_port = hbdm->reset_port;
    
    if (bkpt_port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (bkpt_port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (bkpt_port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (bkpt_port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (bkpt_port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (bkpt_port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (bkpt_port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
    else if (bkpt_port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
    else if (bkpt_port == GPIOI) __HAL_RCC_GPIOI_CLK_ENABLE();
    
    if (reset_port != bkpt_port) {
        if (reset_port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
        else if (reset_port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
        else if (reset_port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
        else if (reset_port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
        else if (reset_port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
        else if (reset_port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
        else if (reset_port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
        else if (reset_port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
        else if (reset_port == GPIOI) __HAL_RCC_GPIOI_CLK_ENABLE();
    }
    
    uint32_t bkpt_mode_reg = bkpt_pin_pos / 8;
    uint32_t bkpt_mode_shift = (bkpt_pin_pos % 8) * 4;
    
    uint32_t reset_mode_reg = reset_pin_pos / 8;
    uint32_t reset_mode_shift = (reset_pin_pos % 8) * 4;
    
    bkpt_port->MODER &= ~(0x3UL << bkpt_mode_shift);
    bkpt_port->MODER |= (0x1UL << bkpt_mode_shift);
    
    bkpt_port->OTYPER |= (1UL << bkpt_pin_pos);
    
    bkpt_port->PUPDR &= ~(0x3UL << bkpt_mode_shift);
    bkpt_port->PUPDR |= (0x1UL << bkpt_mode_shift);
    
    bkpt_port->OSPEEDR &= ~(0x3UL << bkpt_mode_shift);
    bkpt_port->OSPEEDR |= (0x3UL << bkpt_mode_shift);
    
    reset_port->MODER &= ~(0x3UL << reset_mode_shift);
    reset_port->MODER |= (0x1UL << reset_mode_shift);
    
    reset_port->OTYPER &= ~(1UL << reset_pin_pos);
    
    reset_port->PUPDR &= ~(0x3UL << reset_mode_shift);
    
    reset_port->OSPEEDR &= ~(0x3UL << reset_mode_shift);
    reset_port->OSPEEDR |= (0x3UL << reset_mode_shift);
    
    BDM_BKPT_HIGH_REG(hbdm);
    BDM_RESET_HIGH_REG(hbdm);
}

static void BDM_GPIO_SetInput_Reg(BDM_HandleTypeDef* hbdm)
{
    uint32_t bkpt_pin_pos = hbdm->bkpt_pin;
    uint32_t bkpt_mode_reg = bkpt_pin_pos / 8;
    uint32_t bkpt_mode_shift = (bkpt_pin_pos % 8) * 4;
    
    hbdm->bkpt_port->MODER &= ~(0x3UL << bkpt_mode_shift);
}

static void BDM_GPIO_SetOutput_Reg(BDM_HandleTypeDef* hbdm)
{
    uint32_t bkpt_pin_pos = hbdm->bkpt_pin;
    uint32_t bkpt_mode_reg = bkpt_pin_pos / 8;
    uint32_t bkpt_mode_shift = (bkpt_pin_pos % 8) * 4;
    
    hbdm->bkpt_port->MODER &= ~(0x3UL << bkpt_mode_shift);
    hbdm->bkpt_port->MODER |= (0x1UL << bkpt_mode_shift);
    
    hbdm->bkpt_port->OTYPER |= (1UL << bkpt_pin_pos);
}

HAL_StatusTypeDef BDM_Init(BDM_HandleTypeDef* hbdm)
{
    if (hbdm == NULL) {
        return HAL_ERROR;
    }
    
    if (hbdm->speed_hz == 0) {
        hbdm->speed_hz = BDM_DEFAULT_CLOCK;
    }
    
    BDM_SetSpeed(hbdm);
    BDM_TimerInit(hbdm);
    BDM_GPIO_Init_Reg(hbdm);
    
    memcpy(&g_bdm_handle, hbdm, sizeof(BDM_HandleTypeDef));
    
    return HAL_OK;
}

HAL_StatusTypeDef BDM_DeInit(BDM_HandleTypeDef* hbdm)
{
    BDM_TIM->CR1 &= ~TIM_CR1_CEN;
    
    uint32_t bkpt_pin_pos = hbdm->bkpt_pin;
    uint32_t reset_pin_pos = hbdm->reset_pin;
    
    uint32_t bkpt_mode_reg = bkpt_pin_pos / 8;
    uint32_t bkpt_mode_shift = (bkpt_pin_pos % 8) * 4;
    
    uint32_t reset_mode_reg = reset_pin_pos / 8;
    uint32_t reset_mode_shift = (reset_pin_pos % 8) * 4;
    
    hbdm->bkpt_port->MODER &= ~(0x3UL << bkpt_mode_shift);
    hbdm->reset_port->MODER &= ~(0x3UL << reset_mode_shift);
    
    memset(&g_bdm_handle, 0, sizeof(BDM_HandleTypeDef));
    
    return HAL_OK;
}

HAL_StatusTypeDef BDM_EnterBDM(BDM_HandleTypeDef* hbdm)
{
    BDM_RESET_LOW_REG(hbdm);
    BDM_TimerDelayUs(hbdm, 10);
    
    BDM_BKPT_LOW_REG(hbdm);
    BDM_TimerDelayUs(hbdm, 1);
    
    BDM_RESET_HIGH_REG(hbdm);
    BDM_TimerDelayUs(hbdm, 10);
    
    return HAL_OK;
}

HAL_StatusTypeDef BDM_ExitBDM(BDM_HandleTypeDef* hbdm)
{
    BDM_BKPT_HIGH_REG(hbdm);
    
    BDM_RESET_LOW_REG(hbdm);
    BDM_TimerDelayUs(hbdm, 10);
    
    BDM_RESET_HIGH_REG(hbdm);
    
    return HAL_OK;
}

HAL_StatusTypeDef BDM_WriteByte(BDM_HandleTypeDef* hbdm, uint8_t data)
{
    uint32_t half_period_ns = 500000000UL / hbdm->speed_hz;
    
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80) {
            BDM_BKPT_HIGH_REG(hbdm);
        } else {
            BDM_BKPT_LOW_REG(hbdm);
        }
        
        BDM_TimerDelayNs(hbdm, half_period_ns);
        
        BDM_RESET_LOW_REG(hbdm);
        BDM_TimerDelayNs(hbdm, half_period_ns);
        
        BDM_RESET_HIGH_REG(hbdm);
        BDM_TimerDelayNs(hbdm, half_period_ns);
        
        data <<= 1;
    }
    
    BDM_BKPT_HIGH_REG(hbdm);
    
    return HAL_OK;
}

uint8_t BDM_ReadByte(BDM_HandleTypeDef* hbdm)
{
    uint8_t data = 0;
    uint32_t half_period_ns = 500000000UL / hbdm->speed_hz;
    
    BDM_GPIO_SetInput_Reg(hbdm);
    
    for (uint8_t i = 0; i < 8; i++) {
        data <<= 1;
        
        BDM_RESET_LOW_REG(hbdm);
        BDM_TimerDelayNs(hbdm, half_period_ns / 2);
        
        if (BDM_BKPT_READ_REG(hbdm)) {
            data |= 0x01;
        }
        
        BDM_TimerDelayNs(hbdm, half_period_ns / 2);
        
        BDM_RESET_HIGH_REG(hbdm);
        BDM_TimerDelayNs(hbdm, half_period_ns);
    }
    
    BDM_GPIO_SetOutput_Reg(hbdm);
    
    return data;
}

HAL_StatusTypeDef BDM_WriteMem(BDM_HandleTypeDef* hbdm, uint32_t addr, uint8_t* data, uint16_t len)
{
    BDM_WriteByte(hbdm, 0x08);
    BDM_WriteByte(hbdm, (addr >> 24) & 0xFF);
    BDM_WriteByte(hbdm, (addr >> 16) & 0xFF);
    BDM_WriteByte(hbdm, (addr >> 8) & 0xFF);
    BDM_WriteByte(hbdm, addr & 0xFF);
    
    for (uint16_t i = 0; i < len; i++) {
        BDM_WriteByte(hbdm, data[i]);
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef BDM_ReadMem(BDM_HandleTypeDef* hbdm, uint32_t addr, uint8_t* data, uint16_t len)
{
    BDM_WriteByte(hbdm, 0x04);
    BDM_WriteByte(hbdm, (addr >> 24) & 0xFF);
    BDM_WriteByte(hbdm, (addr >> 16) & 0xFF);
    BDM_WriteByte(hbdm, (addr >> 8) & 0xFF);
    BDM_WriteByte(hbdm, addr & 0xFF);
    
    for (uint16_t i = 0; i < len; i++) {
        data[i] = BDM_ReadByte(hbdm);
    }
    
    return HAL_OK;
}
