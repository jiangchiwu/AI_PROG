/**
 ******************************************************************************
 * @file    fine.c
 * @brief   FINE (Flash Interface Network for Easy Programming) 接口实现
 *          Renesas 瑞萨单片机调试编程接口
 *          支持最高10MHz时钟频率，使用寄存器操作和定时器精确定时
 ******************************************************************************
 */

#include "fine.h"

FINE_Config_TypeDef g_fine_config = {0};
FINE_State_TypeDef g_fine_state = {0};

#define FINE_TIM TIM13

static void FINE_TimerWait(uint32_t ticks)
{
    FINE_TIM->CNT = 0;
    while (FINE_TIM->CNT < ticks);
}

void FINE_DelayNs(uint32_t ns)
{
    uint32_t ticks = (ns + g_fine_config.tick_ns - 1) / g_fine_config.tick_ns;
    FINE_TimerWait(ticks);
}

void FINE_DelayUs(uint32_t us)
{
    uint32_t ticks = (us * 1000 + g_fine_config.tick_ns - 1) / g_fine_config.tick_ns;
    FINE_TimerWait(ticks);
}

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

uint32_t FINE_GetSpeed(void)
{
    return g_fine_config.speed_hz;
}

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

static void FINE_DATA_Mode_In(void)
{
    uint32_t pin = g_fine_config.flmd0_pin;
    uint32_t shift = (pin % 8) * 4;
    
    g_fine_config.flmd0_port->MODER &= ~(0x3UL << shift);
}

static void FINE_DATA_Mode_Out(void)
{
    uint32_t pin = g_fine_config.flmd0_pin;
    uint32_t shift = (pin % 8) * 4;
    
    g_fine_config.flmd0_port->MODER &= ~(0x3UL << shift);
    g_fine_config.flmd0_port->MODER |= (0x1UL << shift);
    g_fine_config.flmd0_port->OTYPER |= (1UL << pin);
}

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

HAL_StatusTypeDef FINE_DeInit(void)
{
    FINE_TIM->CR1 &= ~TIM_CR1_CEN;
    FINE_GPIO_DeInit();
    
    memset(&g_fine_config, 0, sizeof(FINE_Config_TypeDef));
    memset(&g_fine_state, 0, sizeof(FINE_State_TypeDef));
    
    return HAL_OK;
}

void FINE_GPIO_Init(void)
{
    FINE_GPIO_Init_Reg();
}

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

HAL_StatusTypeDef FINE_WriteByte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        FINE_SendBit(data & 0x01);
        data >>= 1;
    }
    
    return HAL_OK;
}

uint8_t FINE_ReadByte(void)
{
    uint8_t data = 0;
    
    for (int i = 0; i < 8; i++) {
        data |= FINE_ReceiveBit() << i;
    }
    
    return data;
}

HAL_StatusTypeDef FINE_WriteWord(uint16_t data)
{
    FINE_WriteByte(data & 0xFF);
    FINE_WriteByte((data >> 8) & 0xFF);
    
    return HAL_OK;
}

uint16_t FINE_ReadWord(void)
{
    uint16_t data = 0;
    
    data = FINE_ReadByte();
    data |= (uint16_t)FINE_ReadByte() << 8;
    
    return data;
}

HAL_StatusTypeDef FINE_WriteDWord(uint32_t data)
{
    FINE_WriteByte(data & 0xFF);
    FINE_WriteByte((data >> 8) & 0xFF);
    FINE_WriteByte((data >> 16) & 0xFF);
    FINE_WriteByte((data >> 24) & 0xFF);
    
    return HAL_OK;
}

uint32_t FINE_ReadDWord(void)
{
    uint32_t data = 0;
    
    data = FINE_ReadByte();
    data |= (uint32_t)FINE_ReadByte() << 8;
    data |= (uint32_t)FINE_ReadByte() << 16;
    data |= (uint32_t)FINE_ReadByte() << 24;
    
    return data;
}

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

HAL_StatusTypeDef FINE_Reset(void)
{
    FINE_RESET_LOW();
    FINE_DelayUs(10);
    FINE_RESET_HIGH();
    FINE_DelayUs(10);
    
    return HAL_OK;
}

HAL_StatusTypeDef FINE_ReadID(void)
{
    FINE_WriteByte(FINE_CMD_READ_ID);
    
    g_fine_state.device_code = FINE_ReadByte();
    g_fine_state.product_code = FINE_ReadWord();
    g_fine_state.chip_id = FINE_ReadDWord();
    
    return HAL_OK;
}

HAL_StatusTypeDef FINE_ReadMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    FINE_WriteByte(FINE_CMD_READ);
    FINE_WriteDWord(addr);
    
    for (uint32_t i = 0; i < size; i++) {
        data[i] = FINE_ReadByte();
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef FINE_WriteMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    FINE_WriteByte(FINE_CMD_WRITE);
    FINE_WriteDWord(addr);
    
    for (uint32_t i = 0; i < size; i++) {
        FINE_WriteByte(data[i]);
    }
    
    return HAL_OK;
}

HAL_StatusTypeDef FINE_EraseSector(uint32_t addr)
{
    FINE_WriteByte(FINE_CMD_ERASE);
    FINE_WriteDWord(addr);
    
    return HAL_OK;
}

HAL_StatusTypeDef FINE_EraseChip(void)
{
    FINE_WriteByte(FINE_CMD_ERASE);
    FINE_WriteDWord(0x00000000);
    
    return HAL_OK;
}

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

uint8_t FINE_GetDeviceCode(void)
{
    return g_fine_state.device_code;
}

uint16_t FINE_GetProductCode(void)
{
    return g_fine_state.product_code;
}

uint32_t FINE_GetChipID(void)
{
    return g_fine_state.chip_id;
}
