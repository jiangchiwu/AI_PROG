/**
 ******************************************************************************
 * @file    bdm.c
 * @brief   BDM (Background Debug Mode) 接口实现
 *          NXP/Freescale HC08/HCS08/HCS12系列调试接口
 *          支持最高10MHz时钟频率，使用寄存器操作和定时器精确定时
 ******************************************************************************
 */

#include "bdm.h"

BDM_HandleTypeDef g_bdm_handle = {0};

#define BDM_TIM TIM8

#define BDM_BKPT_PIN_MASK(hbdm)    (1 << ((hbdm)->bkpt_pin))
#define BDM_RESET_PIN_MASK(hbdm)   (1 << ((hbdm)->reset_pin))

#define BDM_BKPT_HIGH_REG(hbdm)    ((hbdm)->bkpt_port->BSRR = BDM_BKPT_PIN_MASK(hbdm))
#define BDM_BKPT_LOW_REG(hbdm)     ((hbdm)->bkpt_port->BSRR = BDM_BKPT_PIN_MASK(hbdm) << 16)
#define BDM_RESET_HIGH_REG(hbdm)   ((hbdm)->reset_port->BSRR = BDM_RESET_PIN_MASK(hbdm))
#define BDM_RESET_LOW_REG(hbdm)    ((hbdm)->reset_port->BSRR = BDM_RESET_PIN_MASK(hbdm) << 16)
#define BDM_BKPT_READ_REG(hbdm)    (((hbdm)->bkpt_port->IDR & BDM_BKPT_PIN_MASK(hbdm)) != 0)

static inline void BDM_TimerStart(BDM_HandleTypeDef* hbdm)
{
    BDM_TIM->CNT = 0;
    BDM_TIM->CR1 |= TIM_CR1_CEN;
}

static inline void BDM_TimerStop(BDM_HandleTypeDef* hbdm)
{
    BDM_TIM->CR1 &= ~TIM_CR1_CEN;
}

static inline void BDM_TimerWait(BDM_HandleTypeDef* hbdm, uint32_t ticks)
{
    BDM_TIM->CNT = 0;
    while (BDM_TIM->CNT < ticks);
}

void BDM_TimerDelayNs(BDM_HandleTypeDef* hbdm, uint32_t ns)
{
    uint32_t ticks = (ns + hbdm->tick_ns - 1) / hbdm->tick_ns;
    BDM_TimerWait(hbdm, ticks);
}

void BDM_TimerDelayUs(BDM_HandleTypeDef* hbdm, uint32_t us)
{
    uint32_t ticks = (us * 1000 + hbdm->tick_ns - 1) / hbdm->tick_ns;
    BDM_TimerWait(hbdm, ticks);
}

static void BDM_TimerInit(BDM_HandleTypeDef* hbdm)
{
    BDM_TIM_CLK_ENABLE();
    
    BDM_TIM->CR1 = 0;
    BDM_TIM->CR2 = 0;
    BDM_TIM->SMCR = 0;
    BDM_TIM->DIER = 0;
    BDM_TIM->SR = 0;
    
    BDM_TIM->PSC = hbdm->prescaler - 1;
    BDM_TIM->ARR = hbdm->period - 1;
    
    BDM_TIM->EGR = TIM_EGR_UG;
    
    BDM_TIM->CR1 |= TIM_CR1_CEN;
}

void BDM_SetSpeed(BDM_HandleTypeDef* hbdm, uint32_t speed_hz)
{
    if (speed_hz > BDM_CLOCK_10MHZ) {
        speed_hz = BDM_CLOCK_10MHZ;
    }
    
    hbdm->speed_hz = speed_hz;
    
    uint32_t apb2_freq = HAL_RCC_GetPCLK2Freq();
    uint32_t tick_freq = speed_hz * 10;
    
    hbdm->prescaler = (apb2_freq + tick_freq - 1) / tick_freq;
    if (hbdm->prescaler < 1) {
        hbdm->prescaler = 1;
    }
    
    hbdm->tick_ns = 1000000000ULL / (apb2_freq / hbdm->prescaler);
    hbdm->period = 65535;
    
    if (BDM_TIM->CR1 & TIM_CR1_CEN) {
        BDM_TIM->CR1 &= ~TIM_CR1_CEN;
        BDM_TIM->PSC = hbdm->prescaler - 1;
        BDM_TIM->ARR = hbdm->period - 1;
        BDM_TIM->EGR = TIM_EGR_UG;
        BDM_TIM->CR1 |= TIM_CR1_CEN;
    }
}

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
