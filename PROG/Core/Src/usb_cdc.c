/*
 * usb_cdc.c
 * USB CDC Device Firmware Implementation
 */

#include "usb_cdc.h"
#include "swd.h"
#include "jtag.h"
#include "dap.h"
#include "core_cal.h"
#include "chip_driver.h"
#include <string.h>

/* Static Variables */
static USB_CDC_State_t usb_cdc_state = USB_CDC_STATE_DISCONNECTED;
static uint8_t tx_buffer[USB_CDC_TX_BUF_SIZE];
static uint8_t rx_buffer[USB_CDC_RX_BUF_SIZE];
static uint16_t rx_write_index = 0;
static uint16_t rx_read_index = 0;
static bool chip_connected = false;

/* Local Function Prototypes */
static HAL_StatusTypeDef Process_CMD_Connect(void);
static HAL_StatusTypeDef Process_CMD_Disconnect(void);
static HAL_StatusTypeDef Process_CMD_ReadFlash(Protocol_Frame_t *frame);
static HAL_StatusTypeDef Process_CMD_WriteFlash(Protocol_Frame_t *frame);
static HAL_StatusTypeDef Process_CMD_EraseFlash(Protocol_Frame_t *frame);
static HAL_StatusTypeDef Process_CMD_EraseChip(void);
static HAL_StatusTypeDef Process_CMD_ReadMem(Protocol_Frame_t *frame);
static HAL_StatusTypeDef Process_CMD_WriteMem(Protocol_Frame_t *frame);
static HAL_StatusTypeDef Process_CMD_Reset(void);
static HAL_StatusTypeDef Process_CMD_Halt(void);
static HAL_StatusTypeDef Process_CMD_Resume(void);
static HAL_StatusTypeDef Process_CMD_GetChipID(void);
static HAL_StatusTypeDef Process_CMD_GetChipInfo(void);

/* Calculate Checksum */
uint8_t Protocol_CalcChecksum(uint8_t *data, uint16_t len)
{
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < len; i++) {
        checksum += data[i];
    }
    return checksum;
}

/* Build Protocol Frame */
HAL_StatusTypeDef Protocol_BuildFrame(uint8_t cmd_id, uint8_t *data, uint16_t len, uint8_t *out_buf, uint16_t *out_len)
{
    if (len > 256 || out_buf == NULL || out_len == NULL) {
        return HAL_ERROR;
    }

    uint16_t idx = 0;
    out_buf[idx++] = PROTOCOL_FRAME_HEADER;
    out_buf[idx++] = len + 1; /* cmd_id + data */
    out_buf[idx++] = cmd_id;
    
    if (data != NULL && len > 0) {
        memcpy(&out_buf[idx], data, len);
        idx += len;
    }
    
    out_buf[idx++] = Protocol_CalcChecksum(&out_buf[2], len + 1);
    out_buf[idx++] = PROTOCOL_FRAME_TRAILER;
    
    *out_len = idx;
    return HAL_OK;
}

/* Parse Protocol Frame */
HAL_StatusTypeDef Protocol_ParseFrame(uint8_t *in_buf, uint16_t in_len, Protocol_Frame_t *frame)
{
    if (in_buf == NULL || frame == NULL || in_len < 5) {
        return HAL_ERROR;
    }

    uint16_t idx = 0;
    
    /* Check header */
    if (in_buf[idx++] != PROTOCOL_FRAME_HEADER) {
        return HAL_ERROR;
    }
    
    frame->length = in_buf[idx++];
    frame->cmd_id = in_buf[idx++];
    
    if (frame->length > 0) {
        uint16_t data_len = frame->length - 1;
        if (data_len > 256) data_len = 256;
        memcpy(frame->data, &in_buf[idx], data_len);
        idx += data_len;
    }
    
    frame->checksum = in_buf[idx++];
    
    /* Verify checksum */
    uint8_t calc_checksum = Protocol_CalcChecksum(&in_buf[2], frame->length);
    if (calc_checksum != frame->checksum) {
        return HAL_ERROR;
    }
    
    /* Check trailer */
    if (in_buf[idx++] != PROTOCOL_FRAME_TRAILER) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/* USB CDC Initialization - Simulated for now */
HAL_StatusTypeDef USB_CDC_Init(void)
{
    memset(tx_buffer, 0, sizeof(tx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    rx_write_index = 0;
    rx_read_index = 0;
    chip_connected = false;
    
    usb_cdc_state = USB_CDC_STATE_CONNECTING;
    
    /* In real implementation, configure USB peripherals here */
    /* For now, set to connected state for testing */
    usb_cdc_state = USB_CDC_STATE_CONNECTED;
    
    return HAL_OK;
}

HAL_StatusTypeDef USB_CDC_DeInit(void)
{
    usb_cdc_state = USB_CDC_STATE_DISCONNECTED;
    chip_connected = false;
    return HAL_OK;
}

/* Send Data (Simulated) */
HAL_StatusTypeDef USB_CDC_SendData(uint8_t *data, uint16_t len)
{
    if (usb_cdc_state != USB_CDC_STATE_CONNECTED) {
        return HAL_ERROR;
    }
    
    /* In real implementation, send via USB CDC */
    /* For now, just copy to buffer (simulated) */
    memcpy(tx_buffer, data, (len > USB_CDC_TX_BUF_SIZE) ? USB_CDC_TX_BUF_SIZE : len);
    
    return HAL_OK;
}

/* Receive Data (Simulated) */
HAL_StatusTypeDef USB_CDC_ReceiveData(uint8_t *data, uint16_t len, uint32_t timeout)
{
    if (usb_cdc_state != USB_CDC_STATE_CONNECTED) {
        return HAL_ERROR;
    }
    
    /* In real implementation, receive from USB CDC */
    /* For now, just return timeout (simulated) */
    (void)data;
    (void)len;
    (void)timeout;
    
    return HAL_TIMEOUT;
}

/* Send Response */
HAL_StatusTypeDef USB_CDC_SendResponse(uint8_t resp_id, uint8_t *data, uint16_t len)
{
    uint8_t frame_buf[512];
    uint16_t frame_len = 0;
    
    if (Protocol_BuildFrame(resp_id, data, len, frame_buf, &frame_len) != HAL_OK) {
        return HAL_ERROR;
    }
    
    return USB_CDC_SendData(frame_buf, frame_len);
}

/* Get Current State */
USB_CDC_State_t USB_CDC_GetState(void)
{
    return usb_cdc_state;
}

/* Process Command Connect */
static HAL_StatusTypeDef Process_CMD_Connect(void)
{
    /* Initialize SWD/JTAG interface */
    SWD_Config_TypeDef swd_config = {
        .swdio_port = GPIOA,
        .swdio_pin = GPIO_PIN_13,
        .swclk_port = GPIOA,
        .swclk_pin = GPIO_PIN_14,
        .reset_port = GPIOA,
        .reset_pin = GPIO_PIN_15,
        .clock = SWD_CLOCK_4MHZ
    };
    
    if (SWD_Init(&swd_config) != HAL_OK) {
        return HAL_ERROR;
    }
    
    chip_connected = true;
    return USB_CDC_SendResponse(RESP_OK, NULL, 0);
}

/* Process Command Disconnect */
static HAL_StatusTypeDef Process_CMD_Disconnect(void)
{
    SWD_DeInit();
    chip_connected = false;
    return USB_CDC_SendResponse(RESP_OK, NULL, 0);
}

/* Process Command Read Flash */
static HAL_StatusTypeDef Process_CMD_ReadFlash(Protocol_Frame_t *frame)
{
    if (!chip_connected) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    if (frame->length < 9) { /* addr(4) + size(4) */
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    uint32_t addr = (frame->data[3] << 24) | (frame->data[2] << 16) | 
                    (frame->data[1] << 8) | frame->data[0];
    uint32_t size = (frame->data[7] << 24) | (frame->data[6] << 16) | 
                    (frame->data[5] << 8) | frame->data[4];
    
    /* Read Flash via DAP/Chip Driver */
    uint8_t read_data[256];
    uint16_t chunk_size = (size > 256) ? 256 : size;
    
    if (Chip_Flash_Read(addr, read_data, chunk_size) != HAL_OK) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    return USB_CDC_SendResponse(RESP_DATA, read_data, chunk_size);
}

/* Process Command Write Flash */
static HAL_StatusTypeDef Process_CMD_WriteFlash(Protocol_Frame_t *frame)
{
    if (!chip_connected) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    if (frame->length < 7) { /* addr(4) + len(2) + data(1) */
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    uint32_t addr = (frame->data[3] << 24) | (frame->data[2] << 16) | 
                    (frame->data[1] << 8) | frame->data[0];
    uint16_t data_len = (frame->data[5] << 8) | frame->data[4];
    
    if (Chip_Flash_Write(addr, &frame->data[6], data_len) != HAL_OK) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    return USB_CDC_SendResponse(RESP_OK, NULL, 0);
}

/* Process Command Erase Flash */
static HAL_StatusTypeDef Process_CMD_EraseFlash(Protocol_Frame_t *frame)
{
    if (!chip_connected) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    if (frame->length < 8) { /* addr(4) + size(4) */
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    uint32_t addr = (frame->data[3] << 24) | (frame->data[2] << 16) | 
                    (frame->data[1] << 8) | frame->data[0];
    uint32_t size = (frame->data[7] << 24) | (frame->data[6] << 16) | 
                    (frame->data[5] << 8) | frame->data[4];
    
    if (Chip_Flash_Erase(addr, size) != HAL_OK) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    return USB_CDC_SendResponse(RESP_OK, NULL, 0);
}

/* Process Command Erase Chip */
static HAL_StatusTypeDef Process_CMD_EraseChip(void)
{
    if (!chip_connected) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    if (Chip_Flash_Erase_Chip() != HAL_OK) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    return USB_CDC_SendResponse(RESP_OK, NULL, 0);
}

/* Process Command Read Memory */
static HAL_StatusTypeDef Process_CMD_ReadMem(Protocol_Frame_t *frame)
{
    if (!chip_connected) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    if (frame->length < 8) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    uint32_t addr = (frame->data[3] << 24) | (frame->data[2] << 16) | 
                    (frame->data[1] << 8) | frame->data[0];
    uint32_t size = (frame->data[7] << 24) | (frame->data[6] << 16) | 
                    (frame->data[5] << 8) | frame->data[4];
    
    uint8_t read_data[256];
    uint16_t chunk_size = (size > 256) ? 256 : size;
    
    if (DAP_ReadMem(addr, read_data, chunk_size) != HAL_OK) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    return USB_CDC_SendResponse(RESP_DATA, read_data, chunk_size);
}

/* Process Command Write Memory */
static HAL_StatusTypeDef Process_CMD_WriteMem(Protocol_Frame_t *frame)
{
    if (!chip_connected) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    if (frame->length < 5) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    uint32_t addr = (frame->data[3] << 24) | (frame->data[2] << 16) | 
                    (frame->data[1] << 8) | frame->data[0];
    uint16_t data_len = frame->length - 5;
    
    if (DAP_WriteMem(addr, &frame->data[4], data_len) != HAL_OK) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    return USB_CDC_SendResponse(RESP_OK, NULL, 0);
}

/* Process Command Reset */
static HAL_StatusTypeDef Process_CMD_Reset(void)
{
    if (!chip_connected) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    if (Chip_Reset() != HAL_OK) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    return USB_CDC_SendResponse(RESP_OK, NULL, 0);
}

/* Process Command Halt */
static HAL_StatusTypeDef Process_CMD_Halt(void)
{
    if (!chip_connected) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    if (Chip_Halt() != HAL_OK) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    return USB_CDC_SendResponse(RESP_OK, NULL, 0);
}

/* Process Command Resume */
static HAL_StatusTypeDef Process_CMD_Resume(void)
{
    if (!chip_connected) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    if (Chip_Resume() != HAL_OK) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    return USB_CDC_SendResponse(RESP_OK, NULL, 0);
}

/* Process Command Get Chip ID */
static HAL_StatusTypeDef Process_CMD_GetChipID(void)
{
    if (!chip_connected) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    uint32_t chip_id = 0;
    if (Chip_ReadChipID(&chip_id) != HAL_OK) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    uint8_t resp_data[4];
    resp_data[0] = chip_id & 0xFF;
    resp_data[1] = (chip_id >> 8) & 0xFF;
    resp_data[2] = (chip_id >> 16) & 0xFF;
    resp_data[3] = (chip_id >> 24) & 0xFF;
    
    return USB_CDC_SendResponse(RESP_DATA, resp_data, 4);
}

/* Process Command Get Chip Info */
static HAL_StatusTypeDef Process_CMD_GetChipInfo(void)
{
    if (!chip_connected) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    Chip_Info_TypeDef chip_info;
    if (Chip_Detect(&chip_info) != HAL_OK) {
        return USB_CDC_SendResponse(RESP_ERROR, NULL, 0);
    }
    
    /* Pack chip info into response */
    uint8_t resp_data[64];
    uint16_t idx = 0;
    memcpy(&resp_data[idx], chip_info.name, 32);
    idx += 32;
    resp_data[idx++] = (chip_info.flash_size >> 24) & 0xFF;
    resp_data[idx++] = (chip_info.flash_size >> 16) & 0xFF;
    resp_data[idx++] = (chip_info.flash_size >> 8) & 0xFF;
    resp_data[idx++] = chip_info.flash_size & 0xFF;
    resp_data[idx++] = (chip_info.ram_size >> 24) & 0xFF;
    resp_data[idx++] = (chip_info.ram_size >> 16) & 0xFF;
    resp_data[idx++] = (chip_info.ram_size >> 8) & 0xFF;
    resp_data[idx++] = chip_info.ram_size & 0xFF;
    
    return USB_CDC_SendResponse(RESP_INFO, resp_data, idx);
}

/* Process Received Frame */
HAL_StatusTypeDef USB_CDC_ProcessFrame(void)
{
    Protocol_Frame_t frame;
    
    /* In real implementation, get data from RX buffer */
    /* For now, process simulated frame */
    
    return HAL_OK;
}

/* Main USB CDC Task - Call this in main loop */
void USB_CDC_Task(void)
{
    if (usb_cdc_state != USB_CDC_STATE_CONNECTED) {
        return;
    }
    
    /* Process any pending frames */
    USB_CDC_ProcessFrame();
}
