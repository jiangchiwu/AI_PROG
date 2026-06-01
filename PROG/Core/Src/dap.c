/**
 ******************************************************************************
 * @file    dap.c
 * @brief   ARM DAP (Debug Access Port) 层实现
 ******************************************************************************
 */

#include "dap.h"
#include <string.h>

DAP_Info_TypeDef g_dap_info = {0};
DAP_Ops_TypeDef g_dap_ops = {0};

static uint32_t DAP_ReadDP_SWD(uint8_t addr)
{
    return SWD_ReadDP(addr);
}

static void DAP_WriteDP_SWD(uint8_t addr, uint32_t data)
{
    SWD_WriteDP(addr, data);
}

static uint32_t DAP_ReadAP_SWD(uint8_t ap_num, uint8_t addr)
{
    DAP_WriteDP(DAP_DP_SELECT, (ap_num << 24) | (addr & 0xF0));
    return SWD_ReadAP(addr);
}

static void DAP_WriteAP_SWD(uint8_t ap_num, uint8_t addr, uint32_t data)
{
    DAP_WriteDP(DAP_DP_SELECT, (ap_num << 24) | (addr & 0xF0));
    SWD_WriteAP(addr, data);
}

static uint32_t DAP_ReadDP_JTAG(uint8_t addr)
{
    return 0;
}

static void DAP_WriteDP_JTAG(uint8_t addr, uint32_t data)
{
}

static uint32_t DAP_ReadAP_JTAG(uint8_t ap_num, uint8_t addr)
{
    return 0;
}

static void DAP_WriteAP_JTAG(uint8_t ap_num, uint8_t addr, uint32_t data)
{
}

HAL_StatusTypeDef DAP_Init(uint8_t protocol)
{
    g_dap_info.protocol = protocol;
    g_dap_info.connected = 0;
    g_dap_info.selected_ap = 0;
    g_dap_info.ap_count = 0;

    if (protocol == DAP_PROTOCOL_SWD) {
        SWD_Config_TypeDef swd_config = {
            .swdio_port = GPIOA,
            .swdio_pin = GPIO_PIN_13,
            .swclk_port = GPIOA,
            .swclk_pin = GPIO_PIN_14,
            .reset_port = GPIOA,
            .reset_pin = GPIO_PIN_15,
            .clock = SWD_DEFAULT_CLOCK,
        };

        if (SWD_Init(&swd_config) != HAL_OK) {
            return HAL_ERROR;
        }

        g_dap_ops.read_dp = DAP_ReadDP_SWD;
        g_dap_ops.write_dp = DAP_WriteDP_SWD;
        g_dap_ops.read_ap = DAP_ReadAP_SWD;
        g_dap_ops.write_ap = DAP_WriteAP_SWD;
    } else if (protocol == DAP_PROTOCOL_JTAG) {
        JTAG_Config_TypeDef jtag_config = {
            .tck_port = GPIOB,
            .tck_pin = GPIO_PIN_0,
            .tms_port = GPIOB,
            .tms_pin = GPIO_PIN_1,
            .tdi_port = GPIOB,
            .tdi_pin = GPIO_PIN_2,
            .tdo_port = GPIOB,
            .tdo_pin = GPIO_PIN_3,
            .nrst_port = GPIOB,
            .nrst_pin = GPIO_PIN_4,
            .ntrst_port = GPIOB,
            .ntrst_pin = GPIO_PIN_5,
            .clock = JTAG_DEFAULT_CLOCK,
        };

        if (JTAG_Init(&jtag_config) != HAL_OK) {
            return HAL_ERROR;
        }

        g_dap_ops.read_dp = DAP_ReadDP_JTAG;
        g_dap_ops.write_dp = DAP_WriteDP_JTAG;
        g_dap_ops.read_ap = DAP_ReadAP_JTAG;
        g_dap_ops.write_ap = DAP_WriteAP_JTAG;
    } else {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef DAP_DeInit(void)
{
    if (g_dap_info.protocol == DAP_PROTOCOL_SWD) {
        SWD_DeInit();
    } else if (g_dap_info.protocol == DAP_PROTOCOL_JTAG) {
        JTAG_DeInit();
    }

    memset(&g_dap_info, 0, sizeof(g_dap_info));
    return HAL_OK;
}

HAL_StatusTypeDef DAP_Connect(void)
{
    uint32_t idcode;

    idcode = DAP_ReadDP(DAP_DP_IDCODE);
    g_dap_info.dp_idcode = idcode;

    if (idcode == 0 || idcode == 0xFFFFFFFF) {
        return HAL_ERROR;
    }

    if (DAP_PowerUpDebug() != HAL_OK) {
        return HAL_ERROR;
    }

    if (DAP_ClearErrors() != HAL_OK) {
        return HAL_ERROR;
    }

    g_dap_info.ap_idr = DAP_ReadAP(0, DAP_AP_IDR);

    g_dap_info.connected = 1;

    return HAL_OK;
}

HAL_StatusTypeDef DAP_Disconnect(void)
{
    g_dap_info.connected = 0;
    return HAL_OK;
}

uint32_t DAP_ReadDP(uint8_t addr)
{
    if (g_dap_ops.read_dp != NULL) {
        return g_dap_ops.read_dp(addr);
    }
    return 0;
}

void DAP_WriteDP(uint8_t addr, uint32_t data)
{
    if (g_dap_ops.write_dp != NULL) {
        g_dap_ops.write_dp(addr, data);
    }
}

uint32_t DAP_ReadAP(uint8_t ap_num, uint8_t addr)
{
    if (g_dap_ops.read_ap != NULL) {
        return g_dap_ops.read_ap(ap_num, addr);
    }
    return 0;
}

void DAP_WriteAP(uint8_t ap_num, uint8_t addr, uint32_t data)
{
    if (g_dap_ops.write_ap != NULL) {
        g_dap_ops.write_ap(ap_num, addr, data);
    }
}

HAL_StatusTypeDef DAP_ReadMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (!g_dap_info.connected) {
        return HAL_ERROR;
    }

    DAP_WriteAP(0, DAP_AP_CSW, DAP_AP_CSW_DEFAULT | DAP_AP_CSW_SIZE32 | DAP_AP_CSW_ADDRINC);
    DAP_WriteAP(0, DAP_AP_TAR, addr);

    for (uint32_t i = 0; i < size; i += 4) {
        uint32_t value = DAP_ReadAP(0, DAP_AP_DRW);
        uint32_t actual = (i + 4 > size) ? size - i : 4;

        for (uint32_t j = 0; j < actual; j++) {
            data[i + j] = (value >> (j * 8)) & 0xFF;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef DAP_WriteMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (!g_dap_info.connected) {
        return HAL_ERROR;
    }

    DAP_WriteAP(0, DAP_AP_CSW, DAP_AP_CSW_DEFAULT | DAP_AP_CSW_SIZE32 | DAP_AP_CSW_ADDRINC);
    DAP_WriteAP(0, DAP_AP_TAR, addr);

    for (uint32_t i = 0; i < size; i += 4) {
        uint32_t value = 0;
        uint32_t actual = (i + 4 > size) ? size - i : 4;

        for (uint32_t j = 0; j < actual; j++) {
            value |= (uint32_t)data[i + j] << (j * 8);
        }

        DAP_WriteAP(0, DAP_AP_DRW, value);
    }

    return HAL_OK;
}

HAL_StatusTypeDef DAP_ReadWord(uint32_t addr, uint32_t *value)
{
    if (!g_dap_info.connected) {
        return HAL_ERROR;
    }

    DAP_WriteAP(0, DAP_AP_CSW, DAP_AP_CSW_DEFAULT | DAP_AP_CSW_SIZE32);
    DAP_WriteAP(0, DAP_AP_TAR, addr);
    *value = DAP_ReadAP(0, DAP_AP_DRW);

    return HAL_OK;
}

HAL_StatusTypeDef DAP_WriteWord(uint32_t addr, uint32_t value)
{
    if (!g_dap_info.connected) {
        return HAL_ERROR;
    }

    DAP_WriteAP(0, DAP_AP_CSW, DAP_AP_CSW_DEFAULT | DAP_AP_CSW_SIZE32);
    DAP_WriteAP(0, DAP_AP_TAR, addr);
    DAP_WriteAP(0, DAP_AP_DRW, value);

    return HAL_OK;
}

HAL_StatusTypeDef DAP_ClearErrors(void)
{
    uint32_t ctrl_stat = DAP_ReadDP(DAP_DP_CTRL_STAT);
    DAP_WriteDP(DAP_DP_CTRL_STAT, ctrl_stat | DAP_CTRL_STAT_STICKYERR | DAP_CTRL_STAT_KEY);
    return HAL_OK;
}

HAL_StatusTypeDef DAP_PowerUpDebug(void)
{
    uint32_t ctrl_stat = DAP_ReadDP(DAP_DP_CTRL_STAT);
    DAP_WriteDP(DAP_DP_CTRL_STAT, ctrl_stat | DAP_CTRL_STAT_CDBGPWRUPREQ | DAP_CTRL_STAT_KEY);

    for (int i = 0; i < 100000; i++) {
        ctrl_stat = DAP_ReadDP(DAP_DP_CTRL_STAT);
        if (ctrl_stat & DAP_CTRL_STAT_CDBGPWRUPACK) {
            return HAL_OK;
        }
    }

    return HAL_ERROR;
}

HAL_StatusTypeDef DAP_SelectAP(uint8_t ap_num)
{
    g_dap_info.selected_ap = ap_num;
    DAP_WriteDP(DAP_DP_SELECT, (ap_num << 24));
    return HAL_OK;
}
