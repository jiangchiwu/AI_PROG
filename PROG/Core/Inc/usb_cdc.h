/*
 * usb_cdc.h
 * USB CDC Device Firmware Header
 */

#ifndef __USB_CDC_H
#define __USB_CDC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* USB CDC Configuration */
#define USB_CDC_TX_BUF_SIZE    2048
#define USB_CDC_RX_BUF_SIZE    2048
#define USB_CDC_MAX_PACKET_SIZE 64

/* Protocol Frame Definitions */
#define PROTOCOL_FRAME_HEADER  0xAA
#define PROTOCOL_FRAME_TRAILER 0x55

/* Command IDs - Matching PC端 */
#define CMD_CONNECT            0x01
#define CMD_DISCONNECT         0x02
#define CMD_READ_FLASH         0x10
#define CMD_WRITE_FLASH        0x11
#define CMD_ERASE_FLASH        0x12
#define CMD_ERASE_CHIP         0x13
#define CMD_READ_MEM           0x20
#define CMD_WRITE_MEM          0x21
#define CMD_RESET              0x30
#define CMD_HALT               0x31
#define CMD_RESUME             0x32
#define CMD_GET_CHIP_ID        0x40
#define CMD_GET_CHIP_INFO      0x41

/* Response IDs */
#define RESP_OK                0x00
#define RESP_ERROR             0xFF
#define RESP_DATA              0x01
#define RESP_INFO              0x02

/* USB CDC State */
typedef enum {
    USB_CDC_STATE_DISCONNECTED = 0,
    USB_CDC_STATE_CONNECTING,
    USB_CDC_STATE_CONNECTED,
    USB_CDC_STATE_ERROR
} USB_CDC_State_t;

/* Protocol Frame Structure */
typedef struct {
    uint8_t header;
    uint8_t length;
    uint8_t cmd_id;
    uint8_t data[256];
    uint8_t checksum;
    uint8_t trailer;
} Protocol_Frame_t;

/* Function Prototypes */
HAL_StatusTypeDef USB_CDC_Init(void);
HAL_StatusTypeDef USB_CDC_DeInit(void);
HAL_StatusTypeDef USB_CDC_SendData(uint8_t *data, uint16_t len);
HAL_StatusTypeDef USB_CDC_ReceiveData(uint8_t *data, uint16_t len, uint32_t timeout);
HAL_StatusTypeDef USB_CDC_SendResponse(uint8_t resp_id, uint8_t *data, uint16_t len);
HAL_StatusTypeDef USB_CDC_ProcessFrame(void);
USB_CDC_State_t USB_CDC_GetState(void);
void USB_CDC_Task(void);

/* Frame Building Helpers */
uint8_t Protocol_CalcChecksum(uint8_t *data, uint16_t len);
HAL_StatusTypeDef Protocol_BuildFrame(uint8_t cmd_id, uint8_t *data, uint16_t len, uint8_t *out_buf, uint16_t *out_len);
HAL_StatusTypeDef Protocol_ParseFrame(uint8_t *in_buf, uint16_t in_len, Protocol_Frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* __USB_CDC_H */
