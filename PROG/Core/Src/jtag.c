/**
 ******************************************************************************
 * @file    jtag.c
 * @brief   JTAG (Joint Test Action Group) 协议实现
 *          IEEE 1149.1 边界扫描标准
 * 
 * @author  AI_PROG项目
 * @date    2026-06-05
 * @version v2.0
 * 
 * @details JTAG是IEEE 1149.1标准定义的边界扫描接口，用于芯片测试和调试。
 *          本实现支持以下特性：
 *          - 完整的TAP状态机实现
 *          - 支持IR(指令寄存器)和DR(数据寄存器)操作
 *          - 支持菊花链多设备检测
 *          - 支持IDCODE读取
 *          - 支持位级操作
 *          
 *          JTAG接口信号线：
 *          - TCK: 测试时钟
 *          - TMS: 测试模式选择(控制状态机转移)
 *          - TDI: 测试数据输入
 *          - TDO: 测试数据输出
 *          - nTRST: 测试复位(可选)
 *          - nRESET: 系统复位(可选)
 *          
 *          TAP状态机包含16个状态：
 *          - Test-Logic-Reset
 *          - Run-Test/Idle
 *          - Select-DR-Scan
 *          - Capture-DR
 *          - Shift-DR
 *          - Exit1-DR
 *          - Pause-DR
 *          - Exit2-DR
 *          - Update-DR
 *          - Select-IR-Scan
 *          - Capture-IR
 *          - Shift-IR
 *          - Exit1-IR
 *          - Pause-IR
 *          - Exit2-IR
 *          - Update-IR
 * 
 * @warning 本驱动使用GPIO模拟方式实现JTAG协议，速度有限。
 ******************************************************************************
 */

#include "jtag.h"
#include "gpio_soft.h"
#include <string.h>

/**
 * @brief JTAG全局配置结构体
 *        存储JTAG接口的硬件配置
 */
JTAG_Config_TypeDef g_jtag_config = {
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
    .initialized = 0,
};

/**
 * @brief JTAG全局状态结构体
 *        存储JTAG会话期间的状态信息
 */
JTAG_State_TypeDef g_jtag_state = {
    .tap_state = TAP_STATE_RESET,
    .ir_length = 4,
    .current_ir = 0,
    .idcode = 0,
    .tap_count = 0,
};

/**
 * @brief 简单延时函数
 *        用于JTAG时序控制，提供最小延时保证
 */
#define JTAG_DELAY()     do { __NOP(); __NOP(); __NOP(); __NOP(); } while(0)

/* ==================== 内部函数声明 ==================== */

/**
 * @brief 输出TCK信号
 * @param bit: 要输出的位(1或0)
 */
static void JTAG_TCK_Out(uint8_t bit);

/**
 * @brief 输出TMS信号
 * @param bit: 要输出的位(1或0)
 */
static void JTAG_TMS_Out(uint8_t bit);

/**
 * @brief 输出TDI信号
 * @param bit: 要输出的位(1或0)
 */
static void JTAG_TDI_Out(uint8_t bit);

/**
 * @brief 读取TDO信号
 * @return 读取到的位(1或0)
 */
static uint8_t JTAG_TDO_In(void);

/**
 * @brief 产生一个TCK时钟周期
 */
static void JTAG_Clock(void);

/**
 * @brief 执行TAP状态转移
 * @param tms: TMS信号值(0或1)
 */
static void JTAG_TapTransition(uint8_t tms);

/* ==================== 内部函数实现 ==================== */

/**
 * @brief 输出TCK信号
 * @param bit: 要输出的位(1或0)
 */
static void JTAG_TCK_Out(uint8_t bit)
{
    if (bit) {
        HAL_GPIO_WritePin(g_jtag_config.tck_port, g_jtag_config.tck_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(g_jtag_config.tck_port, g_jtag_config.tck_pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief 输出TMS信号
 * @param bit: 要输出的位(1或0)
 */
static void JTAG_TMS_Out(uint8_t bit)
{
    if (bit) {
        HAL_GPIO_WritePin(g_jtag_config.tms_port, g_jtag_config.tms_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(g_jtag_config.tms_port, g_jtag_config.tms_pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief 输出TDI信号
 * @param bit: 要输出的位(1或0)
 */
static void JTAG_TDI_Out(uint8_t bit)
{
    if (bit) {
        HAL_GPIO_WritePin(g_jtag_config.tdi_port, g_jtag_config.tdi_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(g_jtag_config.tdi_port, g_jtag_config.tdi_pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief 读取TDO信号
 * @return 读取到的位(1或0)
 */
static uint8_t JTAG_TDO_In(void)
{
    return (HAL_GPIO_ReadPin(g_jtag_config.tdo_port, g_jtag_config.tdo_pin) == GPIO_PIN_SET) ? 1 : 0;
}

/**
 * @brief 产生一个TCK时钟周期
 * 
 * @note JTAG数据在TCK上升沿被采样，所以先设置数据再产生上升沿
 */
static void JTAG_Clock(void)
{
    JTAG_DELAY();
    JTAG_TCK_Out(1);   /* TCK上升沿 - 数据被采样 */
    JTAG_DELAY();
    JTAG_TCK_Out(0);   /* TCK下降沿 */
}

/**
 * @brief 执行TAP状态转移
 * @param tms: TMS信号值(0或1)
 * 
 * @details 根据TMS信号值执行状态转移，同时更新状态机状态。
 *          TMS=0: 前进到下一状态
 *          TMS=1: 回到Test-Logic-Reset路径
 */
static void JTAG_TapTransition(uint8_t tms)
{
    JTAG_TMS_Out(tms);
    JTAG_Clock();
}

/* ==================== GPIO操作函数 ==================== */

/**
 * @brief 初始化JTAG GPIO引脚
 * 
 * @details 配置TCK、TMS、TDI、nRST、nTRST为输出模式，TDO为输入模式。
 *          设置初始状态：TCK=0, TMS=1, TDI=1, nRST=1, nTRST=1
 */
void JTAG_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 配置输出引脚 */
    GPIO_InitStruct.Pin = g_jtag_config.tck_pin | g_jtag_config.tms_pin | 
                          g_jtag_config.tdi_pin | g_jtag_config.nrst_pin | 
                          g_jtag_config.ntrst_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(g_jtag_config.tck_port, &GPIO_InitStruct);

    /* 配置输入引脚 */
    GPIO_InitStruct.Pin = g_jtag_config.tdo_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(g_jtag_config.tdo_port, &GPIO_InitStruct);

    /* 设置初始状态 */
    HAL_GPIO_WritePin(g_jtag_config.tck_port, g_jtag_config.tck_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(g_jtag_config.tms_port, g_jtag_config.tms_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(g_jtag_config.tdi_port, g_jtag_config.tdi_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(g_jtag_config.nrst_port, g_jtag_config.nrst_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(g_jtag_config.ntrst_port, g_jtag_config.ntrst_pin, GPIO_PIN_SET);
}

/**
 * @brief 反初始化JTAG GPIO引脚
 * 
 * @details 将所有JTAG引脚设置为输入模式，释放硬件资源。
 */
void JTAG_GPIO_DeInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = g_jtag_config.tck_pin | g_jtag_config.tms_pin | 
                          g_jtag_config.tdi_pin | g_jtag_config.tdo_pin |
                          g_jtag_config.nrst_pin | g_jtag_config.ntrst_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(g_jtag_config.tck_port, &GPIO_InitStruct);
}

/* ==================== 初始化与反初始化 ==================== */

/**
 * @brief 初始化JTAG接口
 * @param config: JTAG配置结构体指针
 * @return HAL状态
 * 
 * @details 初始化JTAG接口，包括GPIO配置和状态机初始化。
 *          如果传入config参数，则使用用户配置；否则使用默认配置。
 */
HAL_StatusTypeDef JTAG_Init(JTAG_Config_TypeDef *config)
{
    if (config != NULL) {
        g_jtag_config.tck_port = config->tck_port;
        g_jtag_config.tck_pin = config->tck_pin;
        g_jtag_config.tms_port = config->tms_port;
        g_jtag_config.tms_pin = config->tms_pin;
        g_jtag_config.tdi_port = config->tdi_port;
        g_jtag_config.tdi_pin = config->tdi_pin;
        g_jtag_config.tdo_port = config->tdo_port;
        g_jtag_config.tdo_pin = config->tdo_pin;
        g_jtag_config.nrst_port = config->nrst_port;
        g_jtag_config.nrst_pin = config->nrst_pin;
        g_jtag_config.ntrst_port = config->ntrst_port;
        g_jtag_config.ntrst_pin = config->ntrst_pin;
        g_jtag_config.clock = config->clock;
    }

    JTAG_GPIO_Init();

    g_jtag_config.initialized = 1;

    /* 执行TAP复位，确保状态机进入已知状态 */
    JTAG_TAP_Reset();

    return HAL_OK;
}

/**
 * @brief 反初始化JTAG接口
 * @return HAL状态
 * 
 * @details 关闭JTAG接口，释放GPIO资源。
 */
HAL_StatusTypeDef JTAG_DeInit(void)
{
    JTAG_GPIO_DeInit();
    g_jtag_config.initialized = 0;
    return HAL_OK;
}

/* ==================== TAP状态机控制函数 ==================== */

/**
 * @brief 执行TAP复位
 * @return HAL状态
 * 
 * @details 连续发送5个TMS=1的时钟周期，使TAP状态机进入Test-Logic-Reset状态。
 *          根据IEEE 1149.1规范，需要至少5个TMS=1的周期。
 */
HAL_StatusTypeDef JTAG_TAP_Reset(void)
{
    for (uint8_t i = 0; i < 5; i++) {
        JTAG_TapTransition(1);
    }

    g_jtag_state.tap_state = TAP_STATE_IDLE;
    g_jtag_state.current_ir = 0;

    return HAL_OK;
}

/**
 * @brief 执行系统复位
 * @return HAL状态
 * 
 * @details 执行TAP复位，是JTAG_TAP_Reset的别名。
 */
HAL_StatusTypeDef JTAG_Reset(void)
{
    JTAG_TAP_Reset();
    return HAL_OK;
}

/**
 * @brief 跳转到指定的TAP状态
 * @param state: 目标状态
 * @return HAL状态
 * 
 * @details 根据当前状态和目标状态，计算并执行必要的状态转移。
 */
HAL_StatusTypeDef JTAG_Goto_State(JTAG_TAP_State_TypeDef state)
{
    switch (state) {
        case TAP_STATE_RESET:
            /* 直接复位 */
            JTAG_TAP_Reset();
            break;

        case TAP_STATE_IDLE:
            /* 从任意状态回到IDLE */
            while (g_jtag_state.tap_state != TAP_STATE_IDLE) {
                switch (g_jtag_state.tap_state) {
                    case TAP_STATE_RESET:
                        JTAG_TapTransition(0);
                        g_jtag_state.tap_state = TAP_STATE_IDLE;
                        break;
                    case TAP_STATE_SELECT_DR:
                    case TAP_STATE_SELECT_IR:
                    case TAP_STATE_CAPTURE_DR:
                    case TAP_STATE_CAPTURE_IR:
                    case TAP_STATE_SHIFT_DR:
                    case TAP_STATE_SHIFT_IR:
                    case TAP_STATE_EXIT1_DR:
                    case TAP_STATE_EXIT1_IR:
                    case TAP_STATE_PAUSE_DR:
                    case TAP_STATE_PAUSE_IR:
                    case TAP_STATE_EXIT2_DR:
                    case TAP_STATE_EXIT2_IR:
                    case TAP_STATE_UPDATE_DR:
                    case TAP_STATE_UPDATE_IR:
                        JTAG_TapTransition(1);
                        if (g_jtag_state.tap_state == TAP_STATE_UPDATE_DR || 
                            g_jtag_state.tap_state == TAP_STATE_UPDATE_IR) {
                            g_jtag_state.tap_state = TAP_STATE_IDLE;
                        } else {
                            g_jtag_state.tap_state = TAP_STATE_SELECT_DR;
                        }
                        break;
                    default:
                        break;
                }
            }
            break;

        case TAP_STATE_SHIFT_IR:
            /* 从IDLE进入Shift-IR状态 */
            JTAG_Goto_State(TAP_STATE_IDLE);
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_SELECT_DR;
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_SELECT_IR;
            JTAG_TapTransition(0);
            g_jtag_state.tap_state = TAP_STATE_CAPTURE_IR;
            JTAG_TapTransition(0);
            g_jtag_state.tap_state = TAP_STATE_SHIFT_IR;
            break;

        case TAP_STATE_SHIFT_DR:
            /* 从IDLE进入Shift-DR状态 */
            JTAG_Goto_State(TAP_STATE_IDLE);
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_SELECT_DR;
            JTAG_TapTransition(0);
            g_jtag_state.tap_state = TAP_STATE_CAPTURE_DR;
            JTAG_TapTransition(0);
            g_jtag_state.tap_state = TAP_STATE_SHIFT_DR;
            break;

        case TAP_STATE_UPDATE_IR:
            /* 从Shift-IR进入Update-IR状态 */
            JTAG_Goto_State(TAP_STATE_SHIFT_IR);
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_EXIT1_IR;
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_UPDATE_IR;
            break;

        case TAP_STATE_UPDATE_DR:
            /* 从Shift-DR进入Update-DR状态 */
            JTAG_Goto_State(TAP_STATE_SHIFT_DR);
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_EXIT1_DR;
            JTAG_TapTransition(1);
            g_jtag_state.tap_state = TAP_STATE_UPDATE_DR;
            break;

        default:
            break;
    }

    return HAL_OK;
}

/* ==================== IR(指令寄存器)操作 ==================== */

/**
 * @brief 写入指令寄存器
 * @param ir: 指令值
 * @param length: 指令长度(位)
 * @return HAL状态
 * 
 * @details 将指令写入IR寄存器，LSB优先传输。
 *          在最后一位时设置TMS=1以退出Shift-IR状态。
 */
HAL_StatusTypeDef JTAG_Write_IR(uint32_t ir, uint32_t length)
{
    JTAG_Goto_State(TAP_STATE_SHIFT_IR);

    for (uint32_t i = 0; i < length; i++) {
        JTAG_TDI_Out((ir >> i) & 0x01);
        if (i == length - 1) {
            JTAG_TMS_Out(1);  /* 最后一位时退出 */
        }
        JTAG_Clock();
    }

    g_jtag_state.current_ir = ir;
    g_jtag_state.tap_state = TAP_STATE_EXIT1_IR;

    JTAG_TapTransition(1);
    g_jtag_state.tap_state = TAP_STATE_UPDATE_IR;

    JTAG_TapTransition(0);
    g_jtag_state.tap_state = TAP_STATE_IDLE;

    return HAL_OK;
}

/**
 * @brief 读取指令寄存器
 * @param length: 指令长度(位)
 * @return 读取的指令值
 * 
 * @details 从IR寄存器读取指令，LSB优先传输。
 */
uint32_t JTAG_Read_IR(uint32_t length)
{
    uint32_t ir = 0;

    JTAG_Goto_State(TAP_STATE_SHIFT_IR);

    for (uint32_t i = 0; i < length; i++) {
        JTAG_TDI_Out(0);  /* 发送0 */
        if (i == length - 1) {
            JTAG_TMS_Out(1);  /* 最后一位时退出 */
        }
        ir |= (JTAG_TDO_In() << i);
        JTAG_Clock();
    }

    g_jtag_state.tap_state = TAP_STATE_EXIT1_IR;

    JTAG_TapTransition(1);
    g_jtag_state.tap_state = TAP_STATE_UPDATE_IR;

    JTAG_TapTransition(0);
    g_jtag_state.tap_state = TAP_STATE_IDLE;

    return ir;
}

/* ==================== DR(数据寄存器)操作 ==================== */

/**
 * @brief 写入数据寄存器
 * @param dr: 数据值
 * @param length: 数据长度(位)
 * @return HAL状态
 * 
 * @details 将数据写入DR寄存器，LSB优先传输。
 *          在最后一位时设置TMS=1以退出Shift-DR状态。
 */
HAL_StatusTypeDef JTAG_Write_DR(uint32_t dr, uint32_t length)
{
    JTAG_Goto_State(TAP_STATE_SHIFT_DR);

    for (uint32_t i = 0; i < length; i++) {
        JTAG_TDI_Out((dr >> i) & 0x01);
        if (i == length - 1) {
            JTAG_TMS_Out(1);  /* 最后一位时退出 */
        }
        JTAG_Clock();
    }

    g_jtag_state.tap_state = TAP_STATE_EXIT1_DR;

    JTAG_TapTransition(1);
    g_jtag_state.tap_state = TAP_STATE_UPDATE_DR;

    JTAG_TapTransition(0);
    g_jtag_state.tap_state = TAP_STATE_IDLE;

    return HAL_OK;
}

/**
 * @brief 读取数据寄存器
 * @param length: 数据长度(位)
 * @return 读取的数据值
 * 
 * @details 从DR寄存器读取数据，LSB优先传输。
 */
uint32_t JTAG_Read_DR(uint32_t length)
{
    uint32_t dr = 0;

    JTAG_Goto_State(TAP_STATE_SHIFT_DR);

    for (uint32_t i = 0; i < length; i++) {
        JTAG_TDI_Out(0);  /* 发送0 */
        if (i == length - 1) {
            JTAG_TMS_Out(1);  /* 最后一位时退出 */
        }
        dr |= (JTAG_TDO_In() << i);
        JTAG_Clock();
    }

    g_jtag_state.tap_state = TAP_STATE_EXIT1_DR;

    JTAG_TapTransition(1);
    g_jtag_state.tap_state = TAP_STATE_UPDATE_DR;

    JTAG_TapTransition(0);
    g_jtag_state.tap_state = TAP_STATE_IDLE;

    return dr;
}

/* ==================== 组合操作 ==================== */

/**
 * @brief 写入IR和DR
 * @param ir: 指令值
 * @param ir_length: 指令长度
 * @param dr: 数据值
 * @param dr_length: 数据长度
 * @return HAL状态
 * 
 * @details 先写入IR，然后写入DR，是常用的组合操作。
 */
HAL_StatusTypeDef JTAG_Write_IR_DR(uint32_t ir, uint32_t ir_length, uint32_t dr, uint32_t dr_length)
{
    JTAG_Write_IR(ir, ir_length);
    return JTAG_Write_DR(dr, dr_length);
}

/**
 * @brief 写入IR并读取DR
 * @param ir: 指令值
 * @param ir_length: 指令长度
 * @param dr_length: 数据长度
 * @return 读取的数据值
 * 
 * @details 先写入IR，然后读取DR，是常用的组合操作。
 */
uint32_t JTAG_Read_IR_DR(uint32_t ir, uint32_t ir_length, uint32_t dr_length)
{
    JTAG_Write_IR(ir, ir_length);
    return JTAG_Read_DR(dr_length);
}

/* ==================== IDCODE读取 ==================== */

/**
 * @brief 获取IDCODE
 * @return IDCODE值
 * 
 * @details 写入IDCODE指令(通常为0x02)，然后读取32位IDCODE。
 *          IDCODE格式: [31:28]版本, [27:12]部件号, [11:1]制造商ID, [0]始终为1
 */
uint32_t JTAG_GetIDCode(void)
{
    JTAG_Write_IR(0x02, 4);  /* IDCODE指令 */
    g_jtag_state.idcode = JTAG_Read_DR(32);
    return g_jtag_state.idcode;
}

/**
 * @brief 检测JTAG链中的设备数量
 * @return HAL状态
 * 
 * @details 通过连续读取IDCODE来检测菊花链中的TAP设备数量。
 *          最多检测10个设备。
 */
HAL_StatusTypeDef JTAG_DetectChain(void)
{
    uint32_t idcode = 0;
    uint8_t tap_count = 0;

    JTAG_TAP_Reset();

    JTAG_Write_IR(0x02, 4);  /* IDCODE指令 */

    /* 连续读取IDCODE直到无效 */
    do {
        idcode = JTAG_Read_DR(32);
        if (idcode != 0xFFFFFFFF && idcode != 0x00000000) {
            tap_count++;
        } else {
            break;
        }
    } while (tap_count < 10);

    g_jtag_state.tap_count = tap_count;

    if (tap_count == 0) {
        return HAL_ERROR;
    }

    g_jtag_state.idcode = idcode;

    return HAL_OK;
}

/* ==================== 时钟控制函数 ==================== */

/**
 * @brief 设置JTAG时钟频率
 * @param clock: 时钟频率(Hz)
 * @return HAL状态
 */
HAL_StatusTypeDef JTAG_SetClock(uint32_t clock)
{
    g_jtag_config.clock = clock;
    return HAL_OK;
}

/**
 * @brief 获取当前JTAG时钟频率
 * @return 当前时钟频率(Hz)
 */
uint32_t JTAG_GetClock(void)
{
    return g_jtag_config.clock;
}

/* ==================== 复位控制函数 ==================== */

/**
 * @brief 断言系统复位
 * @return HAL状态
 * 
 * @details 将nRST引脚拉低，复位目标系统。
 */
HAL_StatusTypeDef JTAG_AssertReset(void)
{
    HAL_GPIO_WritePin(g_jtag_config.nrst_port, g_jtag_config.nrst_pin, GPIO_PIN_RESET);
    return HAL_OK;
}

/**
 * @brief 取消断言系统复位
 * @return HAL状态
 * 
 * @details 将nRST引脚拉高，释放复位。
 */
HAL_StatusTypeDef JTAG_DeassertReset(void)
{
    HAL_GPIO_WritePin(g_jtag_config.nrst_port, g_jtag_config.nrst_pin, GPIO_PIN_SET);
    return HAL_OK;
}

/* ==================== 位级操作函数 ==================== */

/**
 * @brief 写入任意长度的位序列
 * @param data: 数据缓冲区
 * @param length: 位长度
 * @return HAL状态
 * 
 * @details 从数据缓冲区中逐位写入DR，LSB优先。
 */
HAL_StatusTypeDef JTAG_WriteBits(uint8_t *data, uint32_t length)
{
    JTAG_Goto_State(TAP_STATE_SHIFT_DR);
    
    for (uint32_t i = 0; i < length; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        uint8_t bit = (data[byte_idx] >> bit_idx) & 0x01;
        
        /* 最后一位时置TMS为1 */
        if (i == length - 1) {
            JTAG_TMS_Out(1);
        } else {
            JTAG_TMS_Out(0);
        }
        
        JTAG_TDI_Out(bit);
        JTAG_Clock();
    }
    
    JTAG_Goto_State(TAP_STATE_UPDATE_DR);
    JTAG_Goto_State(TAP_STATE_IDLE);
    
    return HAL_OK;
}

/**
 * @brief 读取任意长度的位序列
 * @param data_out: 输出数据缓冲区
 * @param length: 位长度
 * @return HAL状态
 * 
 * @details 从DR中逐位读取，LSB优先，存储到数据缓冲区。
 */
HAL_StatusTypeDef JTAG_ReadBits(uint8_t *data_out, uint32_t length)
{
    memset(data_out, 0, (length + 7) / 8);
    
    JTAG_Goto_State(TAP_STATE_SHIFT_DR);
    
    for (uint32_t i = 0; i < length; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        
        /* 最后一位时置TMS为1 */
        if (i == length - 1) {
            JTAG_TMS_Out(1);
        } else {
            JTAG_TMS_Out(0);
        }
        
        JTAG_TDI_Out(0);  /* 发送0 */
        uint8_t bit = JTAG_TDO_In();
        JTAG_Clock();
        
        if (bit) {
            data_out[byte_idx] |= (1 << bit_idx);
        }
    }
    
    JTAG_Goto_State(TAP_STATE_UPDATE_DR);
    JTAG_Goto_State(TAP_STATE_IDLE);
    
    return HAL_OK;
}

/**
 * @brief 写入IR和DR(位级操作)
 * @param ir: IR数据缓冲区
 * @param ir_length: IR位长度
 * @param dr: DR数据缓冲区
 * @param dr_length: DR位长度
 * @param dr_out: DR输出数据缓冲区(可选，用于同时读写)
 * @return HAL状态
 * 
 * @details 支持任意长度的IR和DR操作，可同时进行写入和读取。
 */
HAL_StatusTypeDef JTAG_Write_IR_Bits(uint8_t *ir, uint32_t ir_length, uint8_t *dr, uint32_t dr_length, uint8_t *dr_out)
{
    /* 写入IR */
    JTAG_Goto_State(TAP_STATE_SHIFT_IR);
    
    for (uint32_t i = 0; i < ir_length; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        uint8_t bit = (ir[byte_idx] >> bit_idx) & 0x01;
        
        if (i == ir_length - 1) {
            JTAG_TMS_Out(1);
        } else {
            JTAG_TMS_Out(0);
        }
        
        JTAG_TDI_Out(bit);
        JTAG_Clock();
    }
    
    JTAG_Goto_State(TAP_STATE_UPDATE_IR);
    
    /* 写入/读取DR */
    JTAG_Goto_State(TAP_STATE_SHIFT_DR);
    
    if (dr_out == NULL) {
        /* 仅写入 */
        for (uint32_t i = 0; i < dr_length; i++) {
            uint32_t byte_idx = i / 8;
            uint32_t bit_idx = i % 8;
            uint8_t bit = (dr[byte_idx] >> bit_idx) & 0x01;
            
            if (i == dr_length - 1) {
                JTAG_TMS_Out(1);
            } else {
                JTAG_TMS_Out(0);
            }
            
            JTAG_TDI_Out(bit);
            JTAG_Clock();
        }
    } else {
        /* 写入并读取 */
        memset(dr_out, 0, (dr_length + 7) / 8);
        
        for (uint32_t i = 0; i < dr_length; i++) {
            uint32_t byte_idx = i / 8;
            uint32_t bit_idx = i % 8;
            uint8_t bit = (dr[byte_idx] >> bit_idx) & 0x01;
            
            if (i == dr_length - 1) {
                JTAG_TMS_Out(1);
            } else {
                JTAG_TMS_Out(0);
            }
            
            JTAG_TDI_Out(bit);
            uint8_t read_bit = JTAG_TDO_In();
            JTAG_Clock();
            
            if (read_bit) {
                dr_out[byte_idx] |= (1 << bit_idx);
            }
        }
    }
    
    JTAG_Goto_State(TAP_STATE_UPDATE_DR);
    JTAG_Goto_State(TAP_STATE_IDLE);
    
    return HAL_OK;
}