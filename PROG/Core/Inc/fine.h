/**
 ******************************************************************************
 * @file    fine.h
 * @brief   FINE (Flash Interface Network for Easy Programming) 接口实现
 *          Renesas 瑞萨单片机调试编程接口
 ******************************************************************************
 */

#ifndef __FINE_H__
#define __FINE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

#define FINE_OK              0x00
#define FINE_ERR             0x01
#define FINE_ERR_FAULT       0x02
#define FINE_ERR_TIMEOUT     0x03
#define FINE_ERR_PARITY      0x04
#define FINE_ERR_NO_ACK      0x05

#define FINE_CLOCK_100KHZ    100000
#define FINE_CLOCK_200KHZ    200000
#define FINE_CLOCK_400KHZ    400000
#define FINE_CLOCK_1MHZ      1000000

#define FINE_DEFAULT_CLOCK   FINE_CLOCK_200KHZ

#define FINE_CMD_RESET       0x01
#define FINE_CMD_READ_ID     0x02
#define FINE_CMD_READ        0x03
#define FINE_CMD_WRITE       0x04
#define FINE_CMD_ERASE       0x05
#define FINE_CMD_VERIFY      0x06
#define FINE_CMD_ENTER       0x07
#define FINE_CMD_EXIT        0x08

typedef struct {
    GPIO_TypeDef *flmd0_port;
    uint16_t flmd0_pin;
    GPIO_TypeDef *flmd1_port;
    uint16_t flmd1_pin;
    GPIO_TypeDef *flmd2_port;
    uint16_t flmd2_pin;
    GPIO_TypeDef *flmd3_port;
    uint16_t flmd3_pin;
    GPIO_TypeDef *flclk_port;
    uint16_t flclk_pin;
    GPIO_TypeDef *reset_port;
    uint16_t reset_pin;

    uint32_t clock;
    uint8_t initialized;
} FINE_Config_TypeDef;

typedef struct {
    uint8_t device_code;
    uint16_t product_code;
    uint32_t chip_id;
    uint8_t status;
} FINE_State_TypeDef;

extern FINE_Config_TypeDef g_fine_config;
extern FINE_State_TypeDef g_fine_state;

HAL_StatusTypeDef FINE_Init(FINE_Config_TypeDef *config);
HAL_StatusTypeDef FINE_DeInit(void);

HAL_StatusTypeDef FINE_Enter(void);
HAL_StatusTypeDef FINE_Exit(void);

HAL_StatusTypeDef FINE_Reset(void);
HAL_StatusTypeDef FINE_ReadID(void);

HAL_StatusTypeDef FINE_WriteByte(uint8_t data);
uint8_t FINE_ReadByte(void);

HAL_StatusTypeDef FINE_WriteWord(uint16_t data);
uint16_t FINE_ReadWord(void);

HAL_StatusTypeDef FINE_WriteDWord(uint32_t data);
uint32_t FINE_ReadDWord(void);

HAL_StatusTypeDef FINE_ReadMem(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef FINE_WriteMem(uint32_t addr, uint8_t *data, uint32_t size);

HAL_StatusTypeDef FINE_EraseSector(uint32_t addr);
HAL_StatusTypeDef FINE_EraseChip(void);

HAL_StatusTypeDef FINE_Verify(uint32_t addr, uint8_t *data, uint32_t size);

uint8_t FINE_GetDeviceCode(void);
uint16_t FINE_GetProductCode(void);
uint32_t FINE_GetChipID(void);

void FINE_DelayUs(uint32_t us);
void FINE_SendBit(uint8_t bit);
uint8_t FINE_ReceiveBit(void);

void FINE_GPIO_Init(void);
void FINE_GPIO_DeInit(void);

#define FINE_Enable()       HAL_GPIO_WritePin(g_fine_config.reset_port, g_fine_config.reset_pin, GPIO_PIN_SET)
#define FINE_Disable()      HAL_GPIO_WritePin(g_fine_config.reset_port, g_fine_config.reset_pin, GPIO_PIN_RESET)

#define FINE_FLMD0_HIGH()   HAL_GPIO_WritePin(g_fine_config.flmd0_port, g_fine_config.flmd0_pin, GPIO_PIN_SET)
#define FINE_FLMD0_LOW()    HAL_GPIO_WritePin(g_fine_config.flmd0_port, g_fine_config.flmd0_pin, GPIO_PIN_RESET)
#define FINE_FLMD1_HIGH()   HAL_GPIO_WritePin(g_fine_config.flmd1_port, g_fine_config.flmd1_pin, GPIO_PIN_SET)
#define FINE_FLMD1_LOW()    HAL_GPIO_WritePin(g_fine_config.flmd1_port, g_fine_config.flmd1_pin, GPIO_PIN_RESET)
#define FINE_FLMD2_HIGH()   HAL_GPIO_WritePin(g_fine_config.flmd2_port, g_fine_config.flmd2_pin, GPIO_PIN_SET)
#define FINE_FLMD2_LOW()    HAL_GPIO_WritePin(g_fine_config.flmd2_port, g_fine_config.flmd2_pin, GPIO_PIN_RESET)
#define FINE_FLMD3_HIGH()   HAL_GPIO_WritePin(g_fine_config.flmd3_port, g_fine_config.flmd3_pin, GPIO_PIN_SET)
#define FINE_FLMD3_LOW()    HAL_GPIO_WritePin(g_fine_config.flmd3_port, g_fine_config.flmd3_pin, GPIO_PIN_RESET)

#define FINE_CLK_HIGH()     HAL_GPIO_WritePin(g_fine_config.flclk_port, g_fine_config.flclk_pin, GPIO_PIN_SET)
#define FINE_CLK_LOW()      HAL_GPIO_WritePin(g_fine_config.flclk_port, g_fine_config.flclk_pin, GPIO_PIN_RESET)

#define FINE_RESET_HIGH()   HAL_GPIO_WritePin(g_fine_config.reset_port, g_fine_config.reset_pin, GPIO_PIN_SET)
#define FINE_RESET_LOW()    HAL_GPIO_WritePin(g_fine_config.reset_port, g_fine_config.reset_pin, GPIO_PIN_RESET)

#ifdef __cplusplus
}
#endif

#endif /* __FINE_H__ */
