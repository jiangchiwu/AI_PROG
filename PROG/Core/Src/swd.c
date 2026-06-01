/**
  ******************************************************************************
  * @file    swd.c
  * @brief   SWD (Serial Wire Debug) 协议实现
  *          ARM Cortex-M调试接口
  ******************************************************************************
  */

#include "swd.h"
#include "gpio_soft.h"

SWD_Config_TypeDef g_swd_config = {
    .swdio_port = GPIOA,
    .swdio_pin  = GPIO_PIN_13,
    .swclk_port = GPIOA,
    .swclk_pin  = GPIO_PIN_14,
    .reset_port = GPIOA,
    .reset_pin  = GPIO_PIN_15,
    .clock      = SWD_DEFAULT_CLOCK,
    .line_mode  = SWD_LINE_RESET,
    .initialized = 0,
};

SWD_State_TypeDef g_swd_state = {
    .dp_idcode  = 0,
    .ctrl_stat  = 0,
    .select     = 0,
    .ap         = 0,
    .protocol_ver = 0,
};

#define SWD_DELAY()     do { __NOP(); __NOP(); __NOP(); __NOP(); } while(0)

static void SWD_SWO_Out(uint8_t bit)
{
    if (bit) {
        HAL_GPIO_WritePin(g_swd_config.swdio_port, g_swd_config.swdio_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(g_swd_config.swdio_port, g_swd_config.swdio_pin, GPIO_PIN_RESET);
    }
}

static uint8_t SWD_SWO_In(void)
{
    return (HAL_GPIO_ReadPin(g_swd_config.swdio_port, g_swd_config.swdio_pin) == GPIO_PIN_SET) ? 1 : 0;
}

static void SWD_CLK_Out(uint8_t bit)
{
    if (bit) {
        HAL_GPIO_WritePin(g_swd_config.swclk_port, g_swd_config.swclk_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(g_swd_config.swclk_port, g_swd_config.swclk_pin, GPIO_PIN_RESET);
    }
}

static void SWD_TxBit(uint8_t bit)
{
    SWD_SWO_Out(bit);
    SWD_DELAY();
    SWD_CLK_Out(1);
    SWD_DELAY();
    SWD_CLK_Out(0);
}

static uint8_t SWD_RxBit(void)
{
    uint8_t bit;
    SWD_CLK_Out(1);
    SWD_DELAY();
    bit = SWD_SWO_In();
    SWD_DELAY();
    SWD_CLK_Out(0);
    return bit;
}

static uint8_t SWD_CalcParity(uint32_t data)
{
    uint8_t parity = 0;
    for (uint8_t i = 0; i < 32; i++) {
        parity ^= (data >> i) & 0x01;
    }
    return parity;
}

static uint8_t SWD_WaitAck(void)
{
    uint8_t ack;
    ack = SWD_RxBit();
    ack |= (SWD_RxBit() << 1);
    ack |= (SWD_RxBit() << 2);
    SWD_RxBit();
    return ack;
}

void SWD_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = g_swd_config.swdio_pin | g_swd_config.swclk_pin | g_swd_config.reset_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(g_swd_config.swdio_port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(g_swd_config.swdio_port, g_swd_config.swdio_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(g_swd_config.swclk_port, g_swd_config.swclk_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(g_swd_config.reset_port, g_swd_config.reset_pin, GPIO_PIN_SET);
}

void SWD_GPIO_DeInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = g_swd_config.swdio_pin | g_swd_config.swclk_pin | g_swd_config.reset_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(g_swd_config.swdio_port, &GPIO_InitStruct);
}

HAL_StatusTypeDef SWD_Init(SWD_Config_TypeDef *config)
{
    if (config != NULL) {
        g_swd_config.swdio_port = config->swdio_port;
        g_swd_config.swdio_pin   = config->swdio_pin;
        g_swd_config.swclk_port = config->swclk_port;
        g_swd_config.swclk_pin   = config->swclk_pin;
        g_swd_config.reset_port  = config->reset_port;
        g_swd_config.reset_pin   = config->reset_pin;
        g_swd_config.clock       = config->clock;
    }

    SWD_GPIO_Init();

    g_swd_config.initialized = 1;
    g_swd_config.line_mode   = SWD_LINE_RESET;

    SWD_LineReset();

    return HAL_OK;
}

HAL_StatusTypeDef SWD_DeInit(void)
{
    SWD_GPIO_DeInit();
    g_swd_config.initialized = 0;
    return HAL_OK;
}

HAL_StatusTypeDef SWD_LineReset(void)
{
    for (uint8_t i = 0; i < 51; i++) {
        SWD_TxBit(1);
    }

    SWD_WaitAck();

    g_swd_config.line_mode = SWD_LINE_SWD;

    return HAL_OK;
}

HAL_StatusTypeDef SWD_SwitchMode(uint8_t mode)
{
    if (mode == SWD_LINE_JTAG) {
        for (uint8_t i = 0; i < 16; i++) {
            SWD_TxBit(1);
        }
        for (uint8_t i = 0; i < 6; i++) {
            SWD_TxBit(0);
        }
        SWD_TxBit(1);
        SWD_WaitAck();
    } else {
        SWD_LineReset();
    }

    g_swd_config.line_mode = mode;
    return HAL_OK;
}

HAL_StatusTypeDef SWD_Write(uint8_t addr, uint32_t data, uint8_t apnwp)
{
    uint8_t parity;
    uint8_t ack;

    parity = SWD_CalcParity(data);

    SWD_TxBit(0);
    SWD_TxBit(addr & 0x0C);
    SWD_TxBit(apnwp);
    SWD_TxBit(1);

    SWD_WaitAck();

    for (uint8_t i = 0; i < 32; i++) {
        SWD_TxBit((data >> i) & 0x01);
    }

    SWD_TxBit(parity);
    SWD_TxBit(0);
    SWD_TxBit(1);

    return HAL_OK;
}

uint32_t SWD_Read(uint8_t addr, uint8_t *ack)
{
    uint32_t data = 0;
    uint8_t parity = 0;
    uint8_t read_ack;

    SWD_TxBit(0);
    SWD_TxBit(addr & 0x0C);
    SWD_TxBit(1);
    SWD_TxBit(1);

    read_ack = SWD_WaitAck();

    if (ack != NULL) {
        *ack = read_ack;
    }

    if (read_ack == 0x01) {
        for (uint8_t i = 0; i < 32; i++) {
            data |= (SWD_RxBit() << i);
        }

        parity = SWD_RxBit();
        SWD_RxBit();
        SWD_TxBit(0);
    } else {
        for (uint8_t i = 0; i < 33; i++) {
            SWD_RxBit();
        }
        SWD_RxBit();
        SWD_TxBit(0);
    }

    return data;
}

HAL_StatusTypeDef SWD_WriteDP(uint8_t addr, uint32_t data)
{
    uint8_t ack;
    SWD_Write(addr & 0x0C, data, 0);
    g_swd_state.dp_idcode = SWD_Read(addr & 0x0C, &ack);
    (void)ack;
    return HAL_OK;
}

uint32_t SWD_ReadDP(uint8_t addr)
{
    uint8_t ack;
    uint32_t data = SWD_Read(addr & 0x0C, &ack);

    if (addr == 0x00) {
        g_swd_state.dp_idcode = data;
    } else if (addr == 0x04) {
        g_swd_state.ctrl_stat = data;
    } else if (addr == 0x08) {
    } else if (addr == 0x0C) {
        g_swd_state.select = data;
    } else if (addr == 0x10) {
    }

    SWD_ReadDP(0x0C);

    return data;
}

HAL_StatusTypeDef SWD_WriteAP(uint32_t addr, uint32_t data)
{
    uint8_t ap = (addr >> 24) & 0xFF;
    uint8_t reg = addr & 0xFF;

    g_swd_state.select = (ap << 24) | (reg & 0xF0);
    SWD_WriteDP(0x0C, g_swd_state.select);

    SWD_Write(addr, data, 1);

    SWD_ReadDP(0x0C);

    return HAL_OK;
}

uint32_t SWD_ReadAP(uint32_t addr)
{
    uint8_t ap = (addr >> 24) & 0xFF;
    uint8_t reg = addr & 0xFF;

    g_swd_state.select = (ap << 24) | (reg & 0xF0);
    SWD_WriteDP(0x0C, g_swd_state.select);

    SWD_ReadDP(0x0C);

    uint8_t ack;
    uint32_t data = SWD_Read(addr, &ack);

    return data;
}

HAL_StatusTypeDef SWD_WriteAPReg(uint8_t ap, uint8_t reg, uint32_t data)
{
    return SWD_WriteAP((ap << 24) | (reg & 0xF0), data);
}

uint32_t SWD_ReadAPReg(uint8_t ap, uint8_t reg)
{
    return SWD_ReadAP((ap << 24) | (reg & 0xF0));
}

HAL_StatusTypeDef SWD_WriteMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t word_addr = addr & 0xFFFFFFFC;
    uint32_t offset = addr - word_addr;
    uint32_t aligned_size = ((size + offset + 3) & ~3);
    uint32_t padded_size = (aligned_size > size) ? aligned_size : size;

    uint8_t *aligned_data = data;
    uint8_t temp[4] = {0};

    if (offset != 0) {
        aligned_data = temp;
        uint32_t first_words = (4 - offset) > size ? size : (4 - offset);
        memcpy(temp, data, first_words);
        padded_size -= first_words;
    }

    SWD_WriteAP(0x04, word_addr);

    for (uint32_t i = 0; i < padded_size; i += 4) {
        uint32_t value = aligned_data[i] |
                        (aligned_data[i + 1] << 8) |
                        (aligned_data[i + 2] << 16) |
                        (aligned_data[i + 3] << 24);

        SWD_WriteAP(0x0C, value);

        for (volatile uint32_t j = 0; j < 100; j++);
    }

    return HAL_OK;
}

HAL_StatusTypeDef SWD_ReadMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t word_addr = addr & 0xFFFFFFFC;
    uint32_t offset = addr - word_addr;
    uint32_t aligned_size = ((size + offset + 3) & ~3);

    SWD_WriteAP(0x04, word_addr);

    for (uint32_t i = 0; i < aligned_size; i += 4) {
        uint32_t value = SWD_ReadAP(0x0C);

        uint32_t start = i;
        uint32_t end = i + 4;

        if (start < offset) {
            start = offset;
        }

        if (end > size + offset) {
            end = size + offset;
        }

        for (uint32_t j = start; j < end; j++) {
            data[j - offset] = (value >> ((j - i) * 8)) & 0xFF;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef SWD_WriteWord(uint32_t addr, uint32_t data)
{
    SWD_WriteAP(0x04, addr);
    SWD_WriteAP(0x0C, data);
    return HAL_OK;
}

uint32_t SWD_ReadWord(uint32_t addr)
{
    SWD_WriteAP(0x04, addr);
    return SWD_ReadAP(0x0C);
}

HAL_StatusTypeDef SWD_SetClock(uint32_t clock)
{
    g_swd_config.clock = clock;
    return HAL_OK;
}

uint32_t SWD_GetClock(void)
{
    return g_swd_config.clock;
}

uint32_t SWD_GetDPID(void)
{
    return g_swd_state.dp_idcode;
}

uint8_t SWD_GetProtocolVersion(void)
{
    return g_swd_state.protocol_ver;
}
