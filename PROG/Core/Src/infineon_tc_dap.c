/**
 ******************************************************************************
 * @file    infineon_tc_dap.c
 * @brief   Infineon TriCore TC系列 DAP (Debug Access Port) 协议实现
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 * 
 * @details 本文件实现了针对英飞凌TriCore架构TC系列微控制器的DAP调试协议。
 *          支持的芯片系列包括：
 *          - TC2xx系列: TC234, TC264, TC274, TC275, TC277, TC297
 *          - TC3xx系列: TC31x, TC33x, TC35x, TC36x, TC37x, TC39x
 * 
 *          TriCore DAP特性：
 *          1. 使用SWD接口进行调试访问
 *          2. 支持APB-AP (Advanced Peripheral Bus Access Port)
 *          3. 支持AHB-AP (Advanced High-performance Bus Access Port)
 *          4. 支持最高10MHz通信频率
 *          5. 使用TIM6定时器实现精确定时
 * 
 * @note    英飞凌TC系列DAP需要先解锁调试端口才能访问
 * 
 * @warning 调试端口解锁需要正确的密码或安全配置
 ******************************************************************************
 */

#include "infineon_tc_dap.h"

/**
 * @brief TC DAP全局句柄
 */
TC_DAP_HandleTypeDef g_tc_dap_handle = {0};

/**
 * @brief TC DAP定时器定义
 * @note 使用TIM6定时器，挂载在APB1总线上
 */
#define TC_DAP_TIM TIM6

/**
 * @brief TC DAP定时器等待函数
 * @param ticks: 等待的tick数
 */
static void TC_DAP_TimerWait(uint32_t ticks)
{
    TC_DAP_TIM->CNT = 0;
    while (TC_DAP_TIM->CNT < ticks);
}

/**
 * @brief TC DAP微秒级延时
 * @param htc_dap: TC DAP句柄
 * @param us: 延时微秒数
 */
static void TC_DAP_DelayUs(TC_DAP_HandleTypeDef* htc_dap, uint32_t us)
{
    uint32_t ticks = (us * 1000 + htc_dap->tick_ns - 1) / htc_dap->tick_ns;
    if (ticks < 1) ticks = 1;
    TC_DAP_TimerWait(ticks);
}

/**
 * @brief 设置TC DAP通信速度
 * @param htc_dap: TC DAP句柄
 * @param speed_hz: 期望的速度(Hz)
 * @note 支持的速度范围: 100KHz ~ 10MHz
 */
void TC_DAP_SetSpeed(TC_DAP_HandleTypeDef* htc_dap, uint32_t speed_hz)
{
    if (speed_hz > TC_DAP_CLOCK_10MHZ) {
        speed_hz = TC_DAP_CLOCK_10MHZ;
    }

    htc_dap->speed_hz = speed_hz;

    uint32_t apb1_freq = HAL_RCC_GetPCLK1Freq();
    uint32_t tick_freq = speed_hz * 4;

    htc_dap->prescaler = (apb1_freq + tick_freq - 1) / tick_freq;
    if (htc_dap->prescaler < 1) {
        htc_dap->prescaler = 1;
    }

    htc_dap->tick_ns = 1000000000ULL / (apb1_freq / htc_dap->prescaler);
    htc_dap->period = 65535;

    if (TC_DAP_TIM->CR1 & TIM_CR1_CEN) {
        TC_DAP_TIM->CR1 &= ~TIM_CR1_CEN;
        TC_DAP_TIM->PSC = htc_dap->prescaler - 1;
        TC_DAP_TIM->ARR = htc_dap->period - 1;
        TC_DAP_TIM->EGR = TIM_EGR_UG;
        TC_DAP_TIM->CR1 |= TIM_CR1_CEN;
    }
}

/**
 * @brief 初始化TC DAP定时器
 * @param htc_dap: TC DAP句柄
 * @return HAL状态
 */
static HAL_StatusTypeDef TC_DAP_InitTimer(TC_DAP_HandleTypeDef* htc_dap)
{
    __HAL_RCC_TIM6_CLK_ENABLE();

    TIM_HandleTypeDef htim = {0};
    htim.Instance = TC_DAP_TIM;
    htim.Init.Prescaler = htc_dap->prescaler - 1;
    htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim.Init.Period = htc_dap->period - 1;
    htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim) != HAL_OK) {
        return HAL_ERROR;
    }

    TC_DAP_TIM->CR1 |= TIM_CR1_CEN;

    return HAL_OK;
}

/**
 * @brief 初始化TC DAP接口
 * @param htc_dap: TC DAP句柄
 * @return HAL状态
 */
HAL_StatusTypeDef TC_DAP_Init(TC_DAP_HandleTypeDef* htc_dap)
{
    HAL_StatusTypeDef status;

    htc_dap->prescaler = 1;
    htc_dap->period = 65535;
    htc_dap->tick_ns = 1000;
    htc_dap->speed_hz = TC_DAP_CLOCK_1MHZ;

    status = TC_DAP_InitTimer(htc_dap);
    if (status != HAL_OK) {
        return status;
    }

    SWD_HandleTypeDef hswd = {0};
    hswd.swdio_port = htc_dap->swdio_port;
    hswd.swdio_pin = htc_dap->swdio_pin;
    hswd.swclk_port = htc_dap->swclk_port;
    hswd.swclk_pin = htc_dap->swclk_pin;
    hswd.nrst_port = htc_dap->nrst_port;
    hswd.nrst_pin = htc_dap->nrst_pin;

    status = SWD_Init(&hswd);
    if (status != HAL_OK) {
        return status;
    }

    htc_dap->connected = 0;

    return HAL_OK;
}

/**
 * @brief 反初始化TC DAP接口
 * @param htc_dap: TC DAP句柄
 * @return HAL状态
 */
HAL_StatusTypeDef TC_DAP_DeInit(TC_DAP_HandleTypeDef* htc_dap)
{
    HAL_TIM_Base_DeInit((TIM_HandleTypeDef*)TC_DAP_TIM);
    __HAL_RCC_TIM6_CLK_DISABLE();

    SWD_DeInit();

    htc_dap->connected = 0;

    return HAL_OK;
}

/**
 * @brief SWD读操作封装
 * @param htc_dap: TC DAP句柄
 * @param addr: 地址
 * @param ap: AP标志 (0=DP, 1=AP)
 * @return 读取的值
 */
static uint32_t TC_DAP_SWDRead(TC_DAP_HandleTypeDef* htc_dap, uint8_t addr, uint8_t ap)
{
    uint32_t value = 0;
    uint8_t ack;
    
    if (ap) {
        value = SWD_ReadAP(addr);
    } else {
        value = SWD_ReadDP(addr);
    }
    
    (void)ack;
    (void)htc_dap;
    
    return value;
}

/**
 * @brief SWD写操作封装
 * @param htc_dap: TC DAP句柄
 * @param addr: 地址
 * @param ap: AP标志 (0=DP, 1=AP)
 * @param data: 要写入的值
 */
static void TC_DAP_SWDWrite(TC_DAP_HandleTypeDef* htc_dap, uint8_t addr, uint8_t ap, uint32_t data)
{
    if (ap) {
        SWD_WriteAP(addr, data);
    } else {
        SWD_WriteDP(addr, data);
    }
    
    (void)htc_dap;
}

/**
 * @brief 读取DP寄存器
 * @param htc_dap: TC DAP句柄
 * @param addr: DP寄存器地址
 * @return 读取的值
 */
uint32_t TC_DAP_ReadDP(TC_DAP_HandleTypeDef* htc_dap, uint8_t addr)
{
    return TC_DAP_SWDRead(htc_dap, addr, 0);
}

/**
 * @brief 写入DP寄存器
 * @param htc_dap: TC DAP句柄
 * @param addr: DP寄存器地址
 * @param data: 要写入的值
 */
void TC_DAP_WriteDP(TC_DAP_HandleTypeDef* htc_dap, uint8_t addr, uint32_t data)
{
    TC_DAP_SWDWrite(htc_dap, addr, 0, data);
}

/**
 * @brief 读取AP寄存器
 * @param htc_dap: TC DAP句柄
 * @param ap_num: AP编号
 * @param addr: AP寄存器地址
 * @return 读取的值
 */
uint32_t TC_DAP_ReadAP(TC_DAP_HandleTypeDef* htc_dap, uint8_t ap_num, uint8_t addr)
{
    TC_DAP_WriteDP(htc_dap, TC_DAP_DP_SELECT, ((uint32_t)ap_num << 24) | (addr & 0xFC));
    return TC_DAP_SWDRead(htc_dap, TC_DAP_AP_DRW, 1);
}

/**
 * @brief 写入AP寄存器
 * @param htc_dap: TC DAP句柄
 * @param ap_num: AP编号
 * @param addr: AP寄存器地址
 * @param data: 要写入的值
 */
void TC_DAP_WriteAP(TC_DAP_HandleTypeDef* htc_dap, uint8_t ap_num, uint8_t addr, uint32_t data)
{
    TC_DAP_WriteDP(htc_dap, TC_DAP_DP_SELECT, ((uint32_t)ap_num << 24) | (addr & 0xFC));
    TC_DAP_SWDWrite(htc_dap, TC_DAP_AP_DRW, 1, data);
}

/**
 * @brief 解锁调试端口
 * @param htc_dap: TC DAP句柄
 * @return HAL状态
 * @note 英飞凌TC系列需要解锁调试端口才能进行访问
 */
HAL_StatusTypeDef TC_DAP_Unlock(TC_DAP_HandleTypeDef* htc_dap)
{
    uint32_t idcode;

    idcode = TC_DAP_ReadDP(htc_dap, TC_DAP_DP_IDCODE);
    htc_dap->dp_idcode = idcode;

    TC_DAP_WriteDP(htc_dap, TC_DAP_DP_CTRL_STAT, TC_DAP_CTRL_STAT_KEY | 
                   TC_DAP_CTRL_STAT_CDBGPWRUPREQ);

    TC_DAP_DelayUs(htc_dap, 100);

    uint32_t ctrl_stat = TC_DAP_ReadDP(htc_dap, TC_DAP_DP_CTRL_STAT);
    if (!(ctrl_stat & TC_DAP_CTRL_STAT_CDBGPWRUPACK)) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief 连接到目标设备
 * @param htc_dap: TC DAP句柄
 * @return HAL状态
 */
HAL_StatusTypeDef TC_DAP_Connect(TC_DAP_HandleTypeDef* htc_dap)
{
    HAL_StatusTypeDef status;

    SWD_LineReset();
    TC_DAP_DelayUs(htc_dap, 100);

    status = TC_DAP_Unlock(htc_dap);
    if (status != HAL_OK) {
        return status;
    }

    htc_dap->ap_idr = TC_DAP_ReadAP(htc_dap, 0, TC_DAP_AP_IDR);
    
    TC_DAP_WriteAP(htc_dap, 0, TC_DAP_AP_CSW, TC_DAP_AP_CSW_DEFAULT);

    htc_dap->connected = 1;

    return HAL_OK;
}

/**
 * @brief 断开连接
 * @param htc_dap: TC DAP句柄
 * @return HAL状态
 */
HAL_StatusTypeDef TC_DAP_Disconnect(TC_DAP_HandleTypeDef* htc_dap)
{
    uint32_t ctrl_stat = TC_DAP_ReadDP(htc_dap, TC_DAP_DP_CTRL_STAT);
    ctrl_stat &= ~TC_DAP_CTRL_STAT_CDBGPWRUPREQ;
    TC_DAP_WriteDP(htc_dap, TC_DAP_DP_CTRL_STAT, ctrl_stat);

    SWD_LineReset();

    htc_dap->connected = 0;

    return HAL_OK;
}

/**
 * @brief 读取内存
 * @param htc_dap: TC DAP句柄
 * @param addr: 内存地址
 * @param data: 数据缓冲区指针
 * @param size: 数据长度(字节)
 * @return HAL状态
 */
HAL_StatusTypeDef TC_DAP_ReadMem(TC_DAP_HandleTypeDef* htc_dap, uint32_t addr, uint8_t* data, uint32_t size)
{
    if (!htc_dap->connected) {
        return HAL_ERROR;
    }

    TC_DAP_WriteAP(htc_dap, 0, TC_DAP_AP_TAR, addr);

    while (size >= 4) {
        uint32_t value = TC_DAP_ReadAP(htc_dap, 0, TC_DAP_AP_DRW);
        data[0] = (uint8_t)(value & 0xFF);
        data[1] = (uint8_t)((value >> 8) & 0xFF);
        data[2] = (uint8_t)((value >> 16) & 0xFF);
        data[3] = (uint8_t)((value >> 24) & 0xFF);
        data += 4;
        size -= 4;
    }

    if (size > 0) {
        uint32_t value = TC_DAP_ReadAP(htc_dap, 0, TC_DAP_AP_DRW);
        for (uint32_t i = 0; i < size; i++) {
            data[i] = (uint8_t)(value >> (8 * i));
        }
    }

    return HAL_OK;
}

/**
 * @brief 写入内存
 * @param htc_dap: TC DAP句柄
 * @param addr: 内存地址
 * @param data: 数据缓冲区指针
 * @param size: 数据长度(字节)
 * @return HAL状态
 */
HAL_StatusTypeDef TC_DAP_WriteMem(TC_DAP_HandleTypeDef* htc_dap, uint32_t addr, uint8_t* data, uint32_t size)
{
    if (!htc_dap->connected) {
        return HAL_ERROR;
    }

    TC_DAP_WriteAP(htc_dap, 0, TC_DAP_AP_TAR, addr);

    while (size >= 4) {
        uint32_t value = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
        TC_DAP_WriteAP(htc_dap, 0, TC_DAP_AP_DRW, value);
        data += 4;
        size -= 4;
        TC_DAP_DelayUs(htc_dap, 1);
    }

    if (size > 0) {
        uint32_t value = 0;
        for (uint32_t i = 0; i < size; i++) {
            value |= ((uint32_t)data[i] << (8 * i));
        }
        TC_DAP_WriteAP(htc_dap, 0, TC_DAP_AP_DRW, value);
    }

    return HAL_OK;
}

/**
 * @brief 读取32位字
 * @param htc_dap: TC DAP句柄
 * @param addr: 内存地址
 * @param value: 读取的值指针
 * @return HAL状态
 */
HAL_StatusTypeDef TC_DAP_ReadWord(TC_DAP_HandleTypeDef* htc_dap, uint32_t addr, uint32_t* value)
{
    if (!htc_dap->connected) {
        return HAL_ERROR;
    }

    TC_DAP_WriteAP(htc_dap, 0, TC_DAP_AP_TAR, addr);
    *value = TC_DAP_ReadAP(htc_dap, 0, TC_DAP_AP_DRW);

    return HAL_OK;
}

/**
 * @brief 写入32位字
 * @param htc_dap: TC DAP句柄
 * @param addr: 内存地址
 * @param value: 要写入的值
 * @return HAL状态
 */
HAL_StatusTypeDef TC_DAP_WriteWord(TC_DAP_HandleTypeDef* htc_dap, uint32_t addr, uint32_t value)
{
    if (!htc_dap->connected) {
        return HAL_ERROR;
    }

    TC_DAP_WriteAP(htc_dap, 0, TC_DAP_AP_TAR, addr);
    TC_DAP_WriteAP(htc_dap, 0, TC_DAP_AP_DRW, value);
    TC_DAP_DelayUs(htc_dap, 1);

    return HAL_OK;
}

/**
 * @brief 擦除Flash
 * @param htc_dap: TC DAP句柄
 * @param sector_addr: 扇区地址
 * @return HAL状态
 * @note TriCore Flash擦除需要通过DFLASH模块操作
 */
HAL_StatusTypeDef TC_DAP_EraseFlash(TC_DAP_HandleTypeDef* htc_dap, uint32_t sector_addr)
{
    uint32_t dflash_addr = 0xF0000000;
    uint32_t erase_cmd;
    uint32_t status;

    TC_DAP_WriteWord(htc_dap, dflash_addr + 0x08, 0x01);
    
    erase_cmd = 0x00000001 | (sector_addr & 0xFFFFFFF0);
    TC_DAP_WriteWord(htc_dap, dflash_addr + 0x0C, erase_cmd);

    TC_DAP_DelayUs(htc_dap, 10000);

    TC_DAP_ReadWord(htc_dap, dflash_addr + 0x08, &status);
    if (status & 0x80000000) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief 编程Flash
 * @param htc_dap: TC DAP句柄
 * @param addr: 目标地址
 * @param data: 数据缓冲区
 * @param size: 数据长度(字节，必须是4的倍数)
 * @return HAL状态
 * @note TriCore Flash编程需要通过DFLASH模块操作
 */
HAL_StatusTypeDef TC_DAP_ProgramFlash(TC_DAP_HandleTypeDef* htc_dap, uint32_t addr, uint8_t* data, uint32_t size)
{
    uint32_t dflash_addr = 0xF0000000;
    uint32_t status;

    TC_DAP_WriteWord(htc_dap, dflash_addr + 0x08, 0x02);

    for (uint32_t i = 0; i < size; i += 4) {
        uint32_t word = (data[i+3] << 24) | (data[i+2] << 16) | (data[i+1] << 8) | data[i];
        
        TC_DAP_WriteWord(htc_dap, dflash_addr + 0x04, addr + i);
        TC_DAP_WriteWord(htc_dap, dflash_addr + 0x0C, word);

        TC_DAP_DelayUs(htc_dap, 100);

        TC_DAP_ReadWord(htc_dap, dflash_addr + 0x08, &status);
        if (status & 0x80000000) {
            return HAL_ERROR;
        }
    }

    TC_DAP_WriteWord(htc_dap, dflash_addr + 0x08, 0x00);

    return HAL_OK;
}