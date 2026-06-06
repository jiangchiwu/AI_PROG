/**
 ******************************************************************************
 * @file    mon8.c
 * @brief   MON8 接口协议实现
 *          Freescale HC08/HC05 8位单片机调试接口
 *          支持最高10MHz时钟频率，使用寄存器操作和定时器精确定时
 ******************************************************************************
 */

#include "mon8.h"

MON8_Config_TypeDef g_mon8_config = {0};
MON8_State_TypeDef g_mon8_state = {0};

#define MON8_TIM TIM12

#define MON8_BKPT_HIGH()    ((g_mon8_config.bkpt_port)->BSRR = (1 << g_mon8_config.bkpt_pin))
#define MON8_BKPT_LOW()     ((g_mon8_config.bkpt_port)->BSRR = (1 << g_mon8_config.bkpt_pin) << 16)
#define MON8_BKPT_READ()     (((g_mon8_config.bkpt_port)->IDR & (1 << g_mon8_config.bkpt_pin)) != 0)

#define MON8_RST_HIGH()     ((g_mon8_config.rst_port)->BSRR = (1 << g_mon8_config.rst_pin))
#define MON8_RST_LOW()      ((g_mon8_config.rst_port)->BSRR = (1 << g_mon8_config.rst_pin) << 16)

#define MON8_TX_HIGH()       ((g_mon8_config.ptx_port)->BSRR = (1 << g_mon8_config.ptx_pin))
#define MON8_TX_LOW()       ((g_mon8_config.ptx_port)->BSRR = (1 << g_mon8_config.ptx_pin) << 16)
#define MON8_RX_READ()       (((g_mon8_config.prx_port)->IDR & (1 << g_mon8_config.prx_pin)) != 0)

static void MON8_TimerWait(uint32_t ticks)
{
    MON8_TIM->CNT = 0;
    while (MON8_TIM->CNT < ticks);
}

void MON8_DelayNs(uint32_t ns)
{
    uint32_t ticks = (ns + g_mon8_config.tick_ns - 1) / g_mon8_config.tick_ns;
    MON8_TimerWait(ticks);
}

void MON8_DelayUs(uint32_t us)
{
    uint32_t ticks = (us * 1000 + g_mon8_config.tick_ns - 1) / g_mon8_config.tick_ns;
    MON8_TimerWait(ticks);
}

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

uint32_t MON8_GetSpeed(void)
{
    return g_mon8_config.speed_hz;
}

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

static void MON8_BKPT_Mode_In(void)
{
    uint32_t pin = g_mon8_config.bkpt_pin;
    uint32_t shift = (pin % 8) * 4;
    
    g_mon8_config.bkpt_port->MODER &= ~(0x3UL << shift);
}

static void MON8_BKPT_Mode_Out(void)
{
    uint32_t pin = g_mon8_config.bkpt_pin;
    uint32_t shift = (pin % 8) * 4;
    
    g_mon8_config.bkpt_port->MODER &= ~(0x3UL << shift);
    g_mon8_config.bkpt_port->MODER |= (0x1UL << shift);
    g_mon8_config.bkpt_port->OTYPER |= (1UL << pin);
}

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

HAL_StatusTypeDef MON8_DeInit(void)
{
    MON8_TIM->CR1 &= ~TIM_CR1_CEN;
    MON8_GPIO_DeInit();
    
    memset(&g_mon8_config, 0, sizeof(MON8_Config_TypeDef));
    memset(&g_mon8_state, 0, sizeof(MON8_State_TypeDef));
    
    return HAL_OK;
}

void MON8_GPIO_Init(void)
{
    MON8_GPIO_Init_Reg();
}

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

HAL_StatusTypeDef MON8_WriteByte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        MON8_SendBit(data & 0x80);
        data <<= 1;
    }
    
    return HAL_OK;
}

uint8_t MON8_ReadByte(void)
{
    uint8_t data = 0;
    
    for (int i = 0; i < 8; i++) {
        data <<= 1;
        data |= MON8_ReceiveBit();
    }
    
    return data;
}

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

HAL_StatusTypeDef MON8_Exit(void)
{
    MON8_RST_LOW();
    MON8_DelayUs(100);
    MON8_RST_HIGH();
    
    return HAL_OK;
}

HAL_StatusTypeDef MON8_Reset(void)
{
    MON8_RST_LOW();
    MON8_DelayUs(10);
    MON8_RST_HIGH();
    MON8_DelayUs(10);
    
    return HAL_OK;
}

HAL_StatusTypeDef MON8_Run(uint16_t addr)
{
    MON8_WriteByte(MON8_CMD_RUN);
    MON8_WriteByte((addr >> 8) & 0xFF);
    MON8_WriteByte(addr & 0xFF);
    
    return HAL_OK;
}

HAL_StatusTypeDef MON8_Stop(void)
{
    MON8_WriteByte(MON8_CMD_STOP);
    
    return HAL_OK;
}

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

HAL_StatusTypeDef MON8_Erase(uint16_t addr, uint16_t size)
{
    MON8_WriteByte(MON8_CMD_ERASE);
    MON8_WriteByte((addr >> 8) & 0xFF);
    MON8_WriteByte(addr & 0xFF);
    
    return HAL_OK;
}

HAL_StatusTypeDef MON8_Secure(void)
{
    MON8_WriteByte(MON8_CMD_SECURE);
    
    return HAL_OK;
}

uint8_t MON8_GetVersion(void)
{
    return g_mon8_state.version;
}

uint8_t MON8_GetDeviceID(void)
{
    MON8_ReadMem(0xFFFE, &g_mon8_state.device_id, 1);
    return g_mon8_state.device_id;
}
