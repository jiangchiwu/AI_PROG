/**
 ******************************************************************************
 * @file    icsp.c
 * @brief   ICSP (In-Circuit Serial Programming) 和 ISP (In-System Programming) 接口实现
 *
 * @author  AI_PROG项目
 * @date    2026-06-05
 * @version v2.0
 *
 * @details 本文件实现了两种串行编程接口：
 *          1. ICSP (In-Circuit Serial Programming): 用于Microchip PIC系列微控制器
 *             - 支持PIC10/PIC12/PIC16/PIC18等多个系列
 *             - 使用PGC(时钟)、PGD(数据)、MCLR(复位)三线接口
 *          2. ISP (In-System Programming): 用于Atmel AVR系列微控制器
 *             - 支持ATmega/ATtiny等多个系列
 *             - 使用SPI协议进行通信(SCK/MOSI/MISO/RST)
 *
 *          本实现采用以下优化策略：
 *          1. 使用GPIO_Soft框架进行IO口软件模拟，灵活性高
 *          2. 使用TIM7定时器实现纳秒级精确定时
 *          3. 支持100KHz~8MHz的可调时钟频率
 *          4. 同时支持硬件SPI和软件模拟SPI模式
 *
 * @note    ICSP接口使用TIM7定时器，需确保与其他功能不冲突
 *
 * @warning 编程操作会擦除芯片Flash内容，请谨慎操作
 ******************************************************************************
 */

#include "icsp.h"

/*============================================================================*
 * ICSP (PIC系列) 全局变量和宏定义
 *============================================================================*/

/**
 * @brief ICSP全局句柄
 *        存储ICSP接口的硬件配置和运行状态
 */
ICSP_HandleTypeDef g_icsp_handle = {0};

/**
 * @brief ICSP定时器定义
 * @note  使用TIM7，挂载在APB1总线上，最高时钟可达120MHz
 */
#define ICSP_TIM TIM7

/**
 * @brief ICSP GPIO宏定义 - 使用GPIO_Soft框架
 * @note  ICSP接口信号说明：
 *        - PGC: Program Clock - 编程时钟线
 *        - PGD: Program Data - 编程数据线(双向)
 *        - MCLR: Master Clear - 主复位线(低有效)
 */
#define ICSP_PGC_HIGH()      GPIO_Soft_WriteBit(g_icsp_handle.pgc_port, g_icsp_handle.pgc_pin, SOFT_GPIO_HIGH)
#define ICSP_PGC_LOW()       GPIO_Soft_WriteBit(g_icsp_handle.pgc_port, g_icsp_handle.pgc_pin, SOFT_GPIO_LOW)
#define ICSP_PGD_HIGH()      GPIO_Soft_WriteBit(g_icsp_handle.pgd_port, g_icsp_handle.pgd_pin, SOFT_GPIO_HIGH)
#define ICSP_PGD_LOW()       GPIO_Soft_WriteBit(g_icsp_handle.pgd_port, g_icsp_handle.pgd_pin, SOFT_GPIO_LOW)
#define ICSP_PGD_READ()      GPIO_Soft_ReadBit(g_icsp_handle.pgd_port, g_icsp_handle.pgd_pin)
#define ICSP_MCLR_HIGH()     GPIO_Soft_WriteBit(g_icsp_handle.mclr_port, g_icsp_handle.mclr_pin, SOFT_GPIO_HIGH)
#define ICSP_MCLR_LOW()      GPIO_Soft_WriteBit(g_icsp_handle.mclr_port, g_icsp_handle.mclr_pin, SOFT_GPIO_LOW)

/*============================================================================*
 * ISP (AVR系列) 全局变量和宏定义
 *============================================================================*/

/**
 * @brief ISP全局句柄
 *        存储ISP接口的硬件配置和运行状态
 */
ISP_HandleTypeDef g_isp_handle = {0};

/**
 * @brief ISP GPIO宏定义
 * @note  ISP接口信号说明(基于SPI协议)：
 *        - SCK: Serial Clock - 串行时钟
 *        - MOSI: Master Output Slave Input - 主机输出/从机输入
 *        - MISO: Master Input Slave Output - 主机输入/从机输出
 *        - RST: Reset - 复位线(低有效)
 */
#define ISP_SCK_HIGH()       GPIO_Soft_WriteBit(g_isp_handle.sck_port, g_isp_handle.sck_pin, SOFT_GPIO_HIGH)
#define ISP_SCK_LOW()        GPIO_Soft_WriteBit(g_isp_handle.sck_port, g_isp_handle.sck_pin, SOFT_GPIO_LOW)
#define ISP_MOSI_HIGH()      GPIO_Soft_WriteBit(g_isp_handle.mosi_port, g_isp_handle.mosi_pin, SOFT_GPIO_HIGH)
#define ISP_MOSI_LOW()       GPIO_Soft_WriteBit(g_isp_handle.mosi_port, g_isp_handle.mosi_pin, SOFT_GPIO_LOW)
#define ISP_MISO_READ()      GPIO_Soft_ReadBit(g_isp_handle.miso_port, g_isp_handle.miso_pin)
#define ISP_RST_HIGH()       GPIO_Soft_WriteBit(g_isp_handle.rst_port, g_isp_handle.rst_pin, SOFT_GPIO_HIGH)
#define ISP_RST_LOW()        GPIO_Soft_WriteBit(g_isp_handle.rst_port, g_isp_handle.rst_pin, SOFT_GPIO_LOW)

/*============================================================================*
 * ICSP (PIC) 定时器延时函数
 *============================================================================*/

/**
 * @brief  ICSP定时器等待指定tick数
 * @param  ticks: 等待的tick数
 * @note   阻塞式等待，基于TIM7定时器实现精确定时
 */
static void ICSP_TimerWait(uint32_t ticks)
{
    ICSP_TIM->CNT = 0;
    while (ICSP_TIM->CNT < ticks);
}

/**
 * @brief  ICSP纳秒级延时
 * @param  hicsp: ICSP句柄
 * @param  ns: 延时纳秒数
 * @note   基于定时器实现，精度取决于定时器配置的tick_ns
 */
void ICSP_DelayNs(ICSP_HandleTypeDef* hicsp, uint32_t ns)
{
    uint32_t ticks = (ns + hicsp->tick_ns - 1) / hicsp->tick_ns;
    if (ticks < 1) ticks = 1;
    ICSP_TimerWait(ticks);
}

/**
 * @brief  ICSP微秒级延时
 * @param  hicsp: ICSP句柄
 * @param  us: 延时微秒数
 * @note   基于定时器实现，精度取决于定时器配置的tick_ns
 */
void ICSP_DelayUs(ICSP_HandleTypeDef* hicsp, uint32_t us)
{
    uint32_t ticks = (us * 1000 + hicsp->tick_ns - 1) / hicsp->tick_ns;
    if (ticks < 1) ticks = 1;
    ICSP_TimerWait(ticks);
}

/**
 * @brief  设置ICSP通信速度
 * @param  hicsp: ICSP句柄
 * @param  speed_hz: 期望的速度(Hz)
 * @note   支持的速度范围: 100KHz ~ 8MHz
 * @note   使用4倍过采样以提高定时精度
 */
void ICSP_SetSpeed(ICSP_HandleTypeDef* hicsp, uint32_t speed_hz)
{
    if (speed_hz > ICSP_CLOCK_8MHZ) {
        speed_hz = ICSP_CLOCK_8MHZ;
    }

    hicsp->speed_hz = speed_hz;

    uint32_t apb1_freq = HAL_RCC_GetPCLK1Freq();
    uint32_t tick_freq = speed_hz * 4;

    hicsp->prescaler = (apb1_freq + tick_freq - 1) / tick_freq;
    if (hicsp->prescaler < 1) {
        hicsp->prescaler = 1;
    }

    hicsp->tick_ns = 1000000000ULL / (apb1_freq / hicsp->prescaler);
    hicsp->period = 65535;

    if (ICSP_TIM->CR1 & TIM_CR1_CEN) {
        ICSP_TIM->CR1 &= ~TIM_CR1_CEN;
        ICSP_TIM->PSC = hicsp->prescaler - 1;
        ICSP_TIM->ARR = hicsp->period - 1;
        ICSP_TIM->EGR = TIM_EGR_UG;
        ICSP_TIM->CR1 |= TIM_CR1_CEN;
    }
}

/**
 * @brief  获取ICSP当前通信速度
 * @param  hicsp: ICSP句柄
 * @retval 当前速度(Hz)
 */
uint32_t ICSP_GetSpeed(ICSP_HandleTypeDef* hicsp)
{
    return hicsp->speed_hz;
}

/**
 * @brief  ICSP定时器初始化
 * @param  hicsp: ICSP句柄
 */
static void ICSP_TimerInit(ICSP_HandleTypeDef* hicsp)
{
    ICSP_TIM_CLK_ENABLE();

    ICSP_TIM->CR1 = 0;
    ICSP_TIM->CR2 = 0;
    ICSP_TIM->SMCR = 0;
    ICSP_TIM->DIER = 0;
    ICSP_TIM->SR = 0;

    ICSP_TIM->PSC = hicsp->prescaler - 1;
    ICSP_TIM->ARR = hicsp->period - 1;

    ICSP_TIM->EGR = TIM_EGR_UG;

    ICSP_TIM->CR1 |= TIM_CR1_CEN;
}

/*============================================================================*
 * ICSP (PIC) GPIO初始化 - 使用GPIO_Soft框架
 *============================================================================*/

/**
 * @brief  ICSP GPIO初始化
 * @param  hicsp: ICSP句柄
 */
void ICSP_GPIO_Init(ICSP_HandleTypeDef* hicsp)
{
    /* 确保GPIO时钟已使能 */
    GPIO_Soft_SetMode(hicsp->pgc_port, hicsp->pgc_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(hicsp->pgd_port, hicsp->pgd_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(hicsp->mclr_port, hicsp->mclr_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_NONE);

    /* 设置初始状态 */
    ICSP_PGC_HIGH();
    ICSP_PGD_HIGH();
    ICSP_MCLR_HIGH();
}

/**
 * @brief  ICSP GPIO反初始化
 * @param  hicsp: ICSP句柄
 */
void ICSP_GPIO_DeInit(ICSP_HandleTypeDef* hicsp)
{
    /* 设置为输入模式 */
    GPIO_Soft_SetMode(hicsp->pgc_port, hicsp->pgc_pin, SOFT_GPIO_MODE_INPUT, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(hicsp->pgd_port, hicsp->pgd_pin, SOFT_GPIO_MODE_INPUT, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(hicsp->mclr_port, hicsp->mclr_pin, SOFT_GPIO_MODE_INPUT, SOFT_GPIO_PULL_NONE);
}

/*============================================================================*
 * ICSP (PIC) 核心通信函数
 *============================================================================*/

/**
 * @brief  ICSP发送一位数据
 * @param  hicsp: ICSP句柄
 * @param  bit: 要发送的位(0或1)
 */
static void ICSP_SendBit(ICSP_HandleTypeDef* hicsp, uint8_t bit)
{
    if (bit) {
        ICSP_PGD_HIGH();
    } else {
        ICSP_PGD_LOW();
    }

    ICSP_DelayNs(hicsp, hicsp->tick_ns);

    ICSP_PGC_HIGH();
    ICSP_DelayNs(hicsp, hicsp->tick_ns);

    ICSP_PGC_LOW();
    ICSP_DelayNs(hicsp, hicsp->tick_ns);
}

/**
 * @brief  ICSP接收一位数据
 * @param  hicsp: ICSP句柄
 * @retval 接收到的位
 */
static uint8_t ICSP_ReceiveBit(ICSP_HandleTypeDef* hicsp)
{
    uint8_t bit;

    /* 设置PGD为输入 */
    GPIO_Soft_SetMode(hicsp->pgd_port, hicsp->pgd_pin, SOFT_GPIO_MODE_INPUT, SOFT_GPIO_PULL_UP);

    ICSP_DelayNs(hicsp, hicsp->tick_ns);

    ICSP_PGC_HIGH();
    ICSP_DelayNs(hicsp, hicsp->tick_ns);

    bit = (ICSP_PGD_READ() == SOFT_GPIO_HIGH) ? 1 : 0;

    ICSP_PGC_LOW();

    /* 恢复PGD为输出 */
    GPIO_Soft_SetMode(hicsp->pgd_port, hicsp->pgd_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_NONE);

    return bit;
}

/**
 * @brief  ICSP发送一个字节(MSB优先)
 * @param  hicsp: ICSP句柄
 * @param  byte: 要发送的字节
 */
static void ICSP_SendByte(ICSP_HandleTypeDef* hicsp, uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++) {
        ICSP_SendBit(hicsp, (byte >> 7) & 0x01);
        byte <<= 1;
    }
}

/**
 * @brief  ICSP接收一个字节(MSB优先)
 * @param  hicsp: ICSP句柄
 * @retval 接收到的字节
 */
static uint8_t ICSP_ReceiveByte(ICSP_HandleTypeDef* hicsp)
{
    uint8_t byte = 0;

    for (uint8_t i = 0; i < 8; i++) {
        byte <<= 1;
        byte |= ICSP_ReceiveBit(hicsp);
    }

    return byte;
}

/**
 * @brief  ICSP发送命令
 * @param  hicsp: ICSP句柄
 * @param  cmd: 命令字节
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_SendCommand(ICSP_HandleTypeDef* hicsp, uint8_t cmd)
{
    ICSP_SendByte(hicsp, cmd);
    return HAL_OK;
}

/**
 * @brief  ICSP发送命令并延时
 * @param  hicsp: ICSP句柄
 * @param  cmd: 命令字节
 * @param  delay_us: 延时微秒数
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_SendCommandWithDelay(ICSP_HandleTypeDef* hicsp, uint8_t cmd, uint32_t delay_us)
{
    ICSP_SendByte(hicsp, cmd);
    ICSP_DelayUs(hicsp, delay_us);
    return HAL_OK;
}

/**
 * @brief  ICSP写入16位数据
 * @param  hicsp: ICSP句柄
 * @param  data: 要写入的16位数据
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_WriteData(ICSP_HandleTypeDef* hicsp, uint16_t data)
{
    for (uint8_t i = 0; i < 16; i++) {
        ICSP_SendBit(hicsp, (data >> 15) & 0x01);
        data <<= 1;
    }
    return HAL_OK;
}

/**
 * @brief  ICSP读取16位数据
 * @param  hicsp: ICSP句柄
 * @retval 读取的16位数据
 */
uint16_t ICSP_ReadData(ICSP_HandleTypeDef* hicsp)
{
    uint16_t data = 0;

    /* 设置PGD为输入 */
    GPIO_Soft_SetMode(hicsp->pgd_port, hicsp->pgd_pin, SOFT_GPIO_MODE_INPUT, SOFT_GPIO_PULL_UP);

    for (uint8_t i = 0; i < 16; i++) {
        data <<= 1;

        ICSP_PGC_HIGH();
        ICSP_DelayNs(hicsp, hicsp->tick_ns);

        if (ICSP_PGD_READ() == SOFT_GPIO_HIGH) {
            data |= 0x01;
        }

        ICSP_PGC_LOW();
        ICSP_DelayNs(hicsp, hicsp->tick_ns);
    }

    /* 恢复PGD为输出 */
    GPIO_Soft_SetMode(hicsp->pgd_port, hicsp->pgd_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_NONE);

    return data;
}

/*============================================================================*
 * ICSP (PIC) 地址操作
 *============================================================================*/

/**
 * @brief  ICSP递增地址
 * @param  hicsp: ICSP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_IncrementAddress(ICSP_HandleTypeDef* hicsp)
{
    ICSP_SendCommand(hicsp, ICSP_CMD_INCREMENT_ADDR);
    /* 需要4个时钟脉冲来完成 */
    for (uint8_t i = 0; i < 4; i++) {
        ICSP_PGC_HIGH();
        ICSP_DelayNs(hicsp, hicsp->tick_ns);
        ICSP_PGC_LOW();
        ICSP_DelayNs(hicsp, hicsp->tick_ns);
    }
    return HAL_OK;
}

/**
 * @brief  ICSP复位地址
 * @param  hicsp: ICSP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_ResetAddress(ICSP_HandleTypeDef* hicsp)
{
    ICSP_SendCommand(hicsp, ICSP_CMD_RESET_ADDR);
    /* 需要4个时钟脉冲来完成 */
    for (uint8_t i = 0; i < 4; i++) {
        ICSP_PGC_HIGH();
        ICSP_DelayNs(hicsp, hicsp->tick_ns);
        ICSP_PGC_LOW();
        ICSP_DelayNs(hicsp, hicsp->tick_ns);
    }
    return HAL_OK;
}

/**
 * @brief  ICSP设置地址
 * @param  hicsp: ICSP句柄
 * @param  addr: 要设置的地址
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_SetAddress(ICSP_HandleTypeDef* hicsp, uint32_t addr)
{
    ICSP_ResetAddress(hicsp);

    for (uint32_t i = 0; i < addr; i++) {
        ICSP_IncrementAddress(hicsp);
    }

    return HAL_OK;
}

/*============================================================================*
 * ICSP (PIC) 编程模式进入/退出
 *============================================================================*/

/**
 * @brief  ICSP进入编程模式
 *          按照Microchip ICSP协议发送进入编程序列
 * @param  hicsp: ICSP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_EnterProgramming(ICSP_HandleTypeDef* hicsp)
{
    hicsp->state = ICSP_STATE_ENTERING;

    /* 确保MCLR为高 */
    ICSP_MCLR_HIGH();
    ICSP_DelayUs(hicsp, 100);

    /* 进入编程序列:
     * 1. MCLR拉低
     * 2. PGC拉低
     * 3. PGD拉低
     * 4. 延时
     * 5. PGD拉高(进入编程模式信号)
     * 6. 延时
     * 7. PGD再次拉低
     * 8. 延时
     * 9. PGD拉高(确认信号)
     */
    ICSP_MCLR_LOW();
    ICSP_PGC_LOW();
    ICSP_PGD_LOW();
    ICSP_DelayUs(hicsp, 1);

    ICSP_PGD_HIGH();  /* 进入编程模式信号 */
    ICSP_DelayUs(hicsp, 1);

    ICSP_PGD_LOW();   /* 确认信号前 */
    ICSP_DelayUs(hicsp, 1);

    ICSP_PGD_HIGH();  /* 确认信号 */
    ICSP_DelayUs(hicsp, 1);

    /* 发送CLK指令(6个时钟脉冲) - 使PIC进入编程模式 */
    for (uint8_t i = 0; i < 6; i++) {
        ICSP_PGC_HIGH();
        ICSP_DelayNs(hicsp, hicsp->tick_ns);
        ICSP_PGC_LOW();
        ICSP_DelayNs(hicsp, hicsp->tick_ns);
    }

    /* 等待芯片准备好 */
    ICSP_DelayUs(hicsp, 100);

    hicsp->state = ICSP_STATE_PROGRAMMING;

    return HAL_OK;
}

/**
 * @brief  ICSP退出编程模式
 * @param  hicsp: ICSP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_ExitProgramming(ICSP_HandleTypeDef* hicsp)
{
    hicsp->state = ICSP_STATE_EXITING;

    /* MCLR拉高退出编程模式 */
    ICSP_MCLR_HIGH();

    ICSP_PGC_HIGH();
    ICSP_PGD_HIGH();

    ICSP_DelayUs(hicsp, 100);

    hicsp->state = ICSP_STATE_IDLE;

    return HAL_OK;
}

/*============================================================================*
 * ICSP (PIC) 芯片操作
 *============================================================================*/

/**
 * @brief  ICSP整片擦除
 * @param  hicsp: ICSP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_BulkErase(ICSP_HandleTypeDef* hicsp)
{
    hicsp->state = ICSP_STATE_ERASING;

    /* 发送Bulk Erase命令 */
    ICSP_SendCommand(hicsp, ICSP_CMD_BULK_ERASE);
    ICSP_WriteData(hicsp, 0x0000);
    ICSP_WriteData(hicsp, 0x0000);
    ICSP_WriteData(hicsp, 0x0000);

    /* 等待擦除完成 */
    ICSP_DelayUs(hicsp, 10000);

    hicsp->state = ICSP_STATE_PROGRAMMING;

    return HAL_OK;
}

/**
 * @brief  ICSP擦除一行
 * @param  hicsp: ICSP句柄
 * @param  addr: 行地址
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_RowErase(ICSP_HandleTypeDef* hicsp, uint32_t addr)
{
    ICSP_SendCommand(hicsp, ICSP_CMD_ROW_ERASE);
    ICSP_WriteData(hicsp, (addr >> 16) & 0xFFFF);
    ICSP_WriteData(hicsp, addr & 0xFFFF);
    ICSP_WriteData(hicsp, 0x0000);

    /* 等待擦除完成 */
    ICSP_DelayUs(hicsp, 5000);

    return HAL_OK;
}

/**
 * @brief  ICSP擦除Flash
 * @param  hicsp: ICSP句柄
 * @param  addr: 起始地址
 * @param  len: 擦除长度(字节)
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_EraseFlash(ICSP_HandleTypeDef* hicsp, uint32_t addr, uint32_t len)
{
    uint32_t row_size = 64;  /* 典型行大小(字) */

    for (uint32_t i = 0; i < len; i += row_size) {
        ICSP_RowErase(hicsp, addr + i);
    }

    return HAL_OK;
}

/**
 * @brief  ICSP读取设备ID
 * @param  hicsp: ICSP句柄
 * @retval 设备ID
 */
uint32_t ICSP_ReadDeviceID(ICSP_HandleTypeDef* hicsp)
{
    uint32_t device_id = 0;

    /* 读取设备ID命令 */
    ICSP_SendCommand(hicsp, ICSP_CMD_READ_ID);

    /* 读取4个字 */
    for (uint8_t i = 0; i < 4; i++) {
        uint16_t word = ICSP_ReadData(hicsp);
        device_id = (device_id << 16) | word;
        ICSP_IncrementAddress(hicsp);
    }

    hicsp->device_id = device_id;

    return device_id;
}

/**
 * @brief  ICSP读取BandGap校准值
 * @param  hicsp: ICSP句柄
 * @retval BandGap值
 */
uint16_t ICSP_ReadBandGap(ICSP_HandleTypeDef* hicsp)
{
    uint16_t bandgap = 0;

    /* 加载配置字 */
    ICSP_SendCommand(hicsp, ICSP_CMD_LOAD_CONFIG);
    ICSP_WriteData(hicsp, 0x0000);

    /* 读取数据 */
    ICSP_SendCommand(hicsp, ICSP_CMD_READ_DATA);
    bandgap = ICSP_ReadData(hicsp);

    return bandgap;
}

/*============================================================================*
 * ICSP (PIC) 读写操作
 *============================================================================*/

/**
 * @brief  ICSP写Flash
 * @param  hicsp: ICSP句柄
 * @param  addr: 起始地址(字地址)
 * @param  data: 数据缓冲区
 * @param  len: 数据长度(字节)
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_WriteFlash(ICSP_HandleTypeDef* hicsp, uint32_t addr, uint8_t* data, uint32_t len)
{
    uint32_t word_addr = addr / 2;

    for (uint32_t i = 0; i < len; i += 2) {
        uint16_t word = data[i];
        if (i + 1 < len) {
            word |= (data[i + 1] << 8);
        }

        /* 加载数据 */
        ICSP_SendCommand(hicsp, ICSP_CMD_LOAD_DATA);
        ICSP_WriteData(hicsp, word);

        /* 设置地址 */
        ICSP_SetAddress(hicsp, word_addr + i / 2);

        /* 开始编程 */
        ICSP_SendCommand(hicsp, ICSP_CMD_BEGIN_ERASE);
        ICSP_DelayUs(hicsp, 3000);  /* 等待编程完成 */
    }

    return HAL_OK;
}

/**
 * @brief  ICSP读Flash
 * @param  hicsp: ICSP句柄
 * @param  addr: 起始地址(字地址)
 * @param  data: 数据缓冲区
 * @param  len: 数据长度(字节)
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_ReadFlash(ICSP_HandleTypeDef* hicsp, uint32_t addr, uint8_t* data, uint32_t len)
{
    uint32_t word_addr = addr / 2;

    ICSP_SetAddress(hicsp, word_addr);

    for (uint32_t i = 0; i < len; i += 2) {
        ICSP_SendCommand(hicsp, ICSP_CMD_READ_DATA);
        uint16_t word = ICSP_ReadData(hicsp);

        data[i] = word & 0xFF;
        if (i + 1 < len) {
            data[i + 1] = (word >> 8) & 0xFF;
        }

        ICSP_IncrementAddress(hicsp);
    }

    return HAL_OK;
}

/**
 * @brief  ICSP写EEPROM
 * @param  hicsp: ICSP句柄
 * @param  addr: EEPROM地址
 * @param  data: 数据缓冲区
 * @param  len: 数据长度
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_WriteEEPROM(ICSP_HandleTypeDef* hicsp, uint16_t addr, uint8_t* data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        /* 加载数据到EEADR */
        ICSP_SendCommand(hicsp, 0x82);  /* 加载EE数据寄存器 */
        ICSP_WriteData(hicsp, data[i]);

        /* 设置地址 */
        ICSP_SendCommand(hicsp, 0x80);  /* 加载EE地址寄存器 */
        ICSP_WriteData(hicsp, addr + i);

        /* 开始EE写 */
        ICSP_SendCommand(hicsp, 0x84);
        ICSP_DelayUs(hicsp, 4000);  /* EEPROM写入较慢 */
    }

    return HAL_OK;
}

/**
 * @brief  ICSP读EEPROM
 * @param  hicsp: ICSP句柄
 * @param  addr: EEPROM地址
 * @param  data: 数据缓冲区
 * @param  len: 数据长度
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_ReadEEPROM(ICSP_HandleTypeDef* hicsp, uint16_t addr, uint8_t* data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        /* 设置地址 */
        ICSP_SendCommand(hicsp, 0x80);  /* 加载EE地址寄存器 */
        ICSP_WriteData(hicsp, addr + i);

        /* 读取数据 */
        ICSP_SendCommand(hicsp, 0x81);  /* 读EE数据寄存器 */
        uint16_t word = ICSP_ReadData(hicsp);
        data[i] = word & 0xFF;
    }

    return HAL_OK;
}

/**
 * @brief  ICSP写配置字
 * @param  hicsp: ICSP句柄
 * @param  addr: 配置字地址
 * @param  data: 配置字数据
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_WriteConfig(ICSP_HandleTypeDef* hicsp, uint16_t addr, uint16_t data)
{
    /* 加载配置字 */
    ICSP_SendCommand(hicsp, ICSP_CMD_LOAD_CONFIG);
    ICSP_WriteData(hicsp, data);

    /* 设置地址 */
    ICSP_SetAddress(hicsp, addr);

    /* 写配置字 */
    ICSP_SendCommand(hicsp, ICSP_CMD_WRITE_CONFIG);
    ICSP_WriteData(hicsp, 0x0000);

    ICSP_DelayUs(hicsp, 3000);

    return HAL_OK;
}

/**
 * @brief  ICSP读配置字
 * @param  hicsp: ICSP句柄
 * @param  addr: 配置字地址
 * @param  data: 配置字数据指针
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_ReadConfig(ICSP_HandleTypeDef* hicsp, uint16_t addr, uint16_t* data)
{
    ICSP_SetAddress(hicsp, addr);

    ICSP_SendCommand(hicsp, ICSP_CMD_READ_DATA);
    *data = ICSP_ReadData(hicsp);

    return HAL_OK;
}

/**
 * @brief  ICSP验证内存
 * @param  hicsp: ICSP句柄
 * @param  addr: 起始地址
 * @param  data: 期望的数据
 * @param  len: 数据长度
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_VerifyMemory(ICSP_HandleTypeDef* hicsp, uint32_t addr, uint8_t* data, uint32_t len)
{
    uint8_t* read_buf = (uint8_t*)malloc(len);

    if (read_buf == NULL) {
        return HAL_ERROR;
    }

    ICSP_ReadFlash(hicsp, addr, read_buf, len);

    for (uint32_t i = 0; i < len; i++) {
        if (read_buf[i] != data[i]) {
            free(read_buf);
            hicsp->state = ICSP_STATE_ERROR;
            return HAL_ERROR;
        }
    }

    free(read_buf);
    return HAL_OK;
}

/*============================================================================*
 * ICSP (PIC) 初始化/反初始化
 *============================================================================*/

/**
 * @brief  ICSP初始化
 * @param  hicsp: ICSP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_Init(ICSP_HandleTypeDef* hicsp)
{
    if (hicsp == NULL) {
        return HAL_ERROR;
    }

    /* 复制配置到全局句柄 */
    memcpy(&g_icsp_handle, hicsp, sizeof(ICSP_HandleTypeDef));

    /* 设置默认速度 */
    if (g_icsp_handle.speed_hz == 0) {
        g_icsp_handle.speed_hz = ICSP_DEFAULT_CLOCK;
    }

    /* 初始化GPIO */
    ICSP_GPIO_Init(&g_icsp_handle);

    /* 初始化定时器 */
    ICSP_SetSpeed(&g_icsp_handle, g_icsp_handle.speed_hz);
    ICSP_TimerInit(&g_icsp_handle);

    g_icsp_handle.initialized = 1;
    g_icsp_handle.state = ICSP_STATE_IDLE;

    return HAL_OK;
}

/**
 * @brief  ICSP反初始化
 * @param  hicsp: ICSP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ICSP_DeInit(ICSP_HandleTypeDef* hicsp)
{
    /* 停止定时器 */
    ICSP_TIM->CR1 &= ~TIM_CR1_CEN;

    /* 反初始化GPIO */
    ICSP_GPIO_DeInit(&g_icsp_handle);

    /* 清除状态 */
    memset(&g_icsp_handle, 0, sizeof(ICSP_HandleTypeDef));

    return HAL_OK;
}

/*============================================================================*
 * ISP (AVR) 定时器和延时函数
 *============================================================================*/

/**
 * @brief  ISP定时器等待指定tick数
 * @param  ticks: 等待的tick数
 */
static void ISP_TimerWait(uint32_t ticks)
{
    ICSP_TIM->CNT = 0;
    while (ICSP_TIM->CNT < ticks);
}

/**
 * @brief  ISP微秒级延时
 * @param  hisp: ISP句柄
 * @param  us: 延时微秒数
 */
static void ISP_DelayUs(ISP_HandleTypeDef* hisp, uint32_t us)
{
    uint32_t ticks = (us * 1000 + 100 - 1) / 100;  /* 假设100ns分辨率 */
    if (ticks < 1) ticks = 1;
    ISP_TimerWait(ticks);
}

/**
 * @brief  ISP设置SPI速度
 * @param  hisp: ISP句柄
 * @param  speed_hz: 期望的速度(Hz)
 */
void ISP_SetSpeed(ISP_HandleTypeDef* hisp, uint32_t speed_hz)
{
    if (speed_hz > ISP_CLOCK_8MHZ) {
        speed_hz = ISP_CLOCK_8MHZ;
    }

    hisp->speed_hz = speed_hz;
}

/**
 * @brief  ISP获取当前SPI速度
 * @param  hisp: ISP句柄
 * @retval 当前速度(Hz)
 */
uint32_t ISP_GetSpeed(ISP_HandleTypeDef* hisp)
{
    return hisp->speed_hz;
}

/*============================================================================*
 * ISP (AVR) GPIO初始化
 *============================================================================*/

/**
 * @brief  ISP GPIO初始化
 * @param  hisp: ISP句柄
 */
void ISP_GPIO_Init(ISP_HandleTypeDef* hisp)
{
    /* 使用GPIO_Soft框架初始化GPIO */
    GPIO_Soft_SetMode(hisp->sck_port, hisp->sck_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(hisp->mosi_port, hisp->mosi_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(hisp->miso_port, hisp->miso_pin, SOFT_GPIO_MODE_INPUT, SOFT_GPIO_PULL_UP);
    GPIO_Soft_SetMode(hisp->rst_port, hisp->rst_pin, SOFT_GPIO_MODE_OUT_PP, SOFT_GPIO_PULL_NONE);

    /* 初始状态: SCK=0, MOSI=0, RST=1(不复位) */
    ISP_SCK_LOW();
    ISP_MOSI_LOW();
    ISP_RST_HIGH();
}

/**
 * @brief  ISP GPIO反初始化
 * @param  hisp: ISP句柄
 */
void ISP_GPIO_DeInit(ISP_HandleTypeDef* hisp)
{
    GPIO_Soft_SetMode(hisp->sck_port, hisp->sck_pin, SOFT_GPIO_MODE_INPUT, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(hisp->mosi_port, hisp->mosi_pin, SOFT_GPIO_MODE_INPUT, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(hisp->miso_port, hisp->miso_pin, SOFT_GPIO_MODE_INPUT, SOFT_GPIO_PULL_NONE);
    GPIO_Soft_SetMode(hisp->rst_port, hisp->rst_pin, SOFT_GPIO_MODE_INPUT, SOFT_GPIO_PULL_NONE);
}

/*============================================================================*
 * ISP (AVR) 软件模拟SPI通信
 *============================================================================*/

/**
 * @brief  ISP软件模拟SPI字节传输
 * 
 * @details AVR ISP使用SPI模式0进行通信：
 *          - CPOL=0: 时钟空闲时为低电平
 *          - CPHA=0: 数据在SCK上升沿被采样
 *          - 数据传输顺序: MSB(最高有效位)优先
 * 
 *          时序流程：
 *          1. 设置MOSI输出位(MSB先)
 *          2. 等待半周期
 *          3. SCK上升沿 - 从机采样MOSI，主机采样MISO
 *          4. 等待半周期
 *          5. SCK下降沿
 *          6. 重复8次完成一个字节
 * 
 * @param  hisp: ISP句柄
 * @param  data: 要发送的字节
 * @retval 接收到的字节
 */
uint8_t ISP_SoftTransferByte(ISP_HandleTypeDef* hisp, uint8_t data)
{
    uint8_t received = 0;
    uint32_t half_period_us = 500000 / hisp->speed_hz;
    if (half_period_us < 1) half_period_us = 1;

    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80) {
            ISP_MOSI_HIGH();
        } else {
            ISP_MOSI_LOW();
        }
        data <<= 1;

        ISP_DelayUs(hisp, half_period_us);

        ISP_SCK_HIGH();

        if (ISP_MISO_READ() == SOFT_GPIO_HIGH) {
            received |= 0x01;
        }

        ISP_DelayUs(hisp, half_period_us);

        ISP_SCK_LOW();

        received <<= 1;
    }

    received >>= 1;

    return received;
}

/**
 * @brief  ISP软件模拟SPI初始化
 * @param  hisp: ISP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ISP_SoftInit(ISP_HandleTypeDef* hisp)
{
    if (hisp == NULL) {
        return HAL_ERROR;
    }

    /* 初始化GPIO */
    ISP_GPIO_Init(hisp);

    /* 设置默认速度 */
    if (hisp->speed_hz == 0) {
        hisp->speed_hz = ISP_DEFAULT_CLOCK;
    }

    /* 初始化定时器(如果需要精确延时) */

    hisp->initialized = 1;

    return HAL_OK;
}

/*============================================================================*
 * ISP (AVR) 核心通信函数
 *============================================================================*/

/**
 * @brief  ISP传输一个字节
 * @param  hisp: ISP句柄
 * @param  data: 要发送的字节
 * @retval 接收到的字节
 */
uint8_t ISP_TransferByte(ISP_HandleTypeDef* hisp, uint8_t data)
{
    if (hisp->use_hardware_spi && hisp->hspi != NULL) {
        /* 使用硬件SPI */
        uint8_t rx;
        HAL_SPI_TransmitReceive(hisp->hspi, &data, &rx, 1, 100);
        return rx;
    } else {
        /* 使用软件模拟SPI */
        return ISP_SoftTransferByte(hisp, data);
    }
}

/**
 * @brief  ISP等待目标准备好
 * @param  hisp: ISP句柄
 * @param  timeout_ms: 超时时间(毫秒)
 * @retval 状态
 */
HAL_StatusTypeDef ISP_WaitReady(ISP_HandleTypeDef* hisp, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t status;

    while (HAL_GetTick() - start < timeout_ms) {
        status = ISP_TransferByte(hisp, 0xF0);  /* 空命令 */
        if ((status & 0x01) == 0) {
            return HAL_OK;  /* RDY/BSY位为0,表示准备好 */
        }
        ISP_DelayUs(hisp, 100);
    }

    return HAL_TIMEOUT;
}

/*============================================================================*
 * ISP (AVR) 编程使能
 *============================================================================*/

/**
 * @brief  ISP使能编程模式
 *          发送编程使能序列: 0xAC, 0x53, 0x00, 0x00
 * @param  hisp: ISP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ISP_EnableProgramming(ISP_HandleTypeDef* hisp)
{
    uint8_t response[4];

    /* 发送编程使能命令 */
    response[0] = ISP_TransferByte(hisp, 0xAC);
    response[1] = ISP_TransferByte(hisp, 0x53);
    response[2] = ISP_TransferByte(hisp, 0x00);
    response[3] = ISP_TransferByte(hisp, 0x00);

    /* 检查响应: 第三个字节应为0x53表示成功 */
    if (response[2] != 0x53) {
        hisp->state = ISP_STATE_ERROR;
        return HAL_ERROR;
    }

    hisp->state = ISP_STATE_ENABLED;

    return HAL_OK;
}

/**
 * @brief  ISP禁用编程模式
 * @param  hisp: ISP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ISP_DisableProgramming(ISP_HandleTypeDef* hisp)
{
    /* 发送空命令 */
    ISP_TransferByte(hisp, 0x00);
    ISP_TransferByte(hisp, 0x00);
    ISP_TransferByte(hisp, 0x00);
    ISP_TransferByte(hisp, 0x00);

    hisp->state = ISP_STATE_IDLE;

    return HAL_OK;
}

/*============================================================================*
 * ISP (AVR) 进入/退出编程模式
 *============================================================================*/

/**
 * @brief  ISP进入编程模式
 * @param  hisp: ISP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ISP_EnterProgramming(ISP_HandleTypeDef* hisp)
{
    hisp->state = ISP_STATE_ENABLED;

    /* RST低电平使能 */
    ISP_RST_LOW();
    ISP_DelayUs(hisp, 20);  /* 至少2个SCK周期 */

    /* 使能编程 */
    if (ISP_EnableProgramming(hisp) != HAL_OK) {
        /* 尝试复位后重试 */
        ISP_RST_HIGH();
        ISP_DelayUs(hisp, 100);
        ISP_RST_LOW();
        ISP_DelayUs(hisp, 20);

        if (ISP_EnableProgramming(hisp) != HAL_OK) {
            ISP_RST_HIGH();
            hisp->state = ISP_STATE_ERROR;
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

/**
 * @brief  ISP退出编程模式
 * @param  hisp: ISP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ISP_ExitProgramming(ISP_HandleTypeDef* hisp)
{
    /* 禁用编程 */
    ISP_DisableProgramming(hisp);

    /* 释放RST */
    ISP_RST_HIGH();

    ISP_DelayUs(hisp, 100);

    hisp->state = ISP_STATE_IDLE;

    return HAL_OK;
}

/*============================================================================*
 * ISP (AVR) 擦除操作
 *============================================================================*/

/**
 * @brief  ISP芯片擦除
 *          擦除命令: 0xAC, 0x80, 0x00, 0x00
 * @param  hisp: ISP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ISP_ChipErase(ISP_HandleTypeDef* hisp)
{
    hisp->state = ISP_STATE_ERASING;

    /* 发送芯片擦除命令 */
    ISP_TransferByte(hisp, 0xAC);
    ISP_TransferByte(hisp, 0x80);
    ISP_TransferByte(hisp, 0x00);
    ISP_TransferByte(hisp, 0x00);

    /* 等待擦除完成(最大9ms) */
    ISP_DelayUs(hisp, 10000);

    /* 检查是否擦除成功 */
    if (ISP_WaitReady(hisp, 1000) != HAL_OK) {
        hisp->state = ISP_STATE_ERROR;
        return HAL_ERROR;
    }

    hisp->state = ISP_STATE_ENABLED;

    return HAL_OK;
}

/**
 * @brief  ISP擦除Flash
 * @param  hisp: ISP句柄
 * @param  addr: 起始地址
 * @param  len: 擦除长度(字节)
 * @retval 状态
 */
HAL_StatusTypeDef ISP_EraseFlash(ISP_HandleTypeDef* hisp, uint32_t addr, uint32_t len)
{
    /* AVR Flash以字为单位 */
    uint16_t word_addr = addr / 2;
    uint16_t word_count = len / 2;

    for (uint16_t i = 0; i < word_count; i++) {
        /* 页擦除命令 */
        ISP_TransferByte(hisp, 0xAC);
        ISP_TransferByte(hisp, 0x23);
        ISP_TransferByte(hisp, (word_addr + i) >> 8);
        ISP_TransferByte(hisp, (word_addr + i) & 0xFF);

        ISP_DelayUs(hisp, 4500);  /* 页擦除约4.5ms */
    }

    return HAL_OK;
}

/**
 * @brief  ISP擦除EEPROM
 * @param  hisp: ISP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ISP_EraseEEPROM(ISP_HandleTypeDef* hisp)
{
    /* EEPROM擦除通过写0xFF实现 */
    /* 实际芯片擦除已经包含EEPROM擦除 */

    return HAL_OK;
}

/*============================================================================*
 * ISP (AVR) Flash读写操作
 *============================================================================*/

/**
 * @brief  ISP写Flash字
 * @param  hisp: ISP句柄
 * @param  addr: 字地址
 * @param  data: 16位数据
 * @retval 状态
 */
HAL_StatusTypeDef ISP_WriteFlashWord(ISP_HandleTypeDef* hisp, uint32_t addr, uint16_t data)
{
    uint8_t low_byte = data & 0xFF;
    uint8_t high_byte = (data >> 8) & 0xFF;
    uint16_t word_addr = addr / 2;

    /* 写低字节 */
    ISP_TransferByte(hisp, ISP_CMD_WRITE_PROG_MEM);
    ISP_TransferByte(hisp, 0x00);
    ISP_TransferByte(hisp, low_byte);
    ISP_TransferByte(hisp, high_byte);

    /* 等待写入完成 */
    ISP_WaitReady(hisp, 100);

    return HAL_OK;
}

/**
 * @brief  ISP写Flash页
 *          AVR使用页编程,需要先加载页缓冲区,然后触发页编程
 * @param  hisp: ISP句柄
 * @param  addr: 页起始地址(字地址)
 * @param  data: 数据缓冲区
 * @param  len: 数据长度(字节)
 * @retval 状态
 */
HAL_StatusTypeDef ISP_WriteFlashPage(ISP_HandleTypeDef* hisp, uint32_t addr, uint8_t* data, uint16_t len)
{
    uint16_t word_addr = addr / 2;
    uint16_t page_size = hisp->page_size > 0 ? hisp->page_size : 64;

    /* 加载页缓冲区 */
    for (uint16_t i = 0; i < len; i += 2) {
        uint16_t word = data[i];
        if (i + 1 < len) {
            word |= (data[i + 1] << 8);
        }

        ISP_TransferByte(hisp, 0x40);  /* 加载程序存储器页字 */
        ISP_TransferByte(hisp, 0x00);
        ISP_TransferByte(hisp, word & 0xFF);
        ISP_TransferByte(hisp, (word >> 8) & 0xFF);
    }

    /* 触发页编程 */
    ISP_TransferByte(hisp, 0x4C);
    ISP_TransferByte(hisp, (word_addr >> 8) & 0xFF);
    ISP_TransferByte(hisp, word_addr & 0xFF);
    ISP_TransferByte(hisp, 0x00);

    /* 等待页编程完成 */
    ISP_DelayUs(hisp, page_size * 500);  /* 根据页大小延时 */

    return HAL_OK;
}

/**
 * @brief  ISP读Flash字
 * @param  hisp: ISP句柄
 * @param  addr: 字地址
 * @param  data: 数据指针
 * @retval 状态
 */
HAL_StatusTypeDef ISP_ReadFlashWord(ISP_HandleTypeDef* hisp, uint32_t addr, uint16_t* data)
{
    uint16_t word_addr = addr / 2;
    uint8_t response[4];

    /* 读低字节 */
    response[0] = ISP_TransferByte(hisp, ISP_CMD_READ_PROG_MEM);
    response[1] = ISP_TransferByte(hisp, 0x00);
    response[2] = ISP_TransferByte(hisp, (word_addr >> 8) & 0xFF);
    response[3] = ISP_TransferByte(hisp, word_addr & 0xFF);

    uint8_t low_byte = response[3];

    /* 读高字节 */
    response[0] = ISP_TransferByte(hisp, ISP_CMD_READ_PROG_MEM_HIGH);
    response[1] = ISP_TransferByte(hisp, 0x00);
    response[2] = ISP_TransferByte(hisp, (word_addr >> 8) & 0xFF);
    response[3] = ISP_TransferByte(hisp, word_addr & 0xFF);

    uint8_t high_byte = response[3];

    *data = (high_byte << 8) | low_byte;

    return HAL_OK;
}

/**
 * @brief  ISP读Flash
 * @param  hisp: ISP句柄
 * @param  addr: 起始地址
 * @param  data: 数据缓冲区
 * @param  len: 数据长度(字节)
 * @retval 状态
 */
HAL_StatusTypeDef ISP_ReadFlash(ISP_HandleTypeDef* hisp, uint32_t addr, uint8_t* data, uint32_t len)
{
    uint16_t word_addr = addr / 2;

    for (uint32_t i = 0; i < len; i += 2) {
        uint16_t word;
        ISP_ReadFlashWord(hisp, word_addr + i / 2, &word);

        data[i] = word & 0xFF;
        if (i + 1 < len) {
            data[i + 1] = (word >> 8) & 0xFF;
        }
    }

    return HAL_OK;
}

/**
 * @brief  ISP写Flash
 * @param  hisp: ISP句柄
 * @param  addr: 起始地址
 * @param  data: 数据缓冲区
 * @param  len: 数据长度(字节)
 * @retval 状态
 */
HAL_StatusTypeDef ISP_WriteFlash(ISP_HandleTypeDef* hisp, uint32_t addr, uint8_t* data, uint32_t len)
{
    uint16_t page_size = hisp->page_size > 0 ? hisp->page_size : 64;

    /* 按页编程 */
    for (uint32_t offset = 0; offset < len; offset += page_size * 2) {
        uint32_t page_addr = addr + offset;
        uint16_t chunk_len = page_size * 2;
        if (offset + chunk_len > len) {
            chunk_len = len - offset;
        }

        ISP_WriteFlashPage(hisp, page_addr, data + offset, chunk_len);
    }

    return HAL_OK;
}

/*============================================================================*
 * ISP (AVR) EEPROM读写操作
 *============================================================================*/

/**
 * @brief  ISP写EEPROM字节
 * @param  hisp: ISP句柄
 * @param  addr: EEPROM地址
 * @param  data: 数据
 * @retval 状态
 */
HAL_StatusTypeDef ISP_WriteEEPROM(ISP_HandleTypeDef* hisp, uint16_t addr, uint8_t data)
{
    /* 写EEPROM命令 */
    ISP_TransferByte(hisp, ISP_CMD_WRITE_EEPROM);
    ISP_TransferByte(hisp, (addr >> 8) & 0xFF);
    ISP_TransferByte(hisp, addr & 0xFF);
    ISP_TransferByte(hisp, data);

    /* 等待写入完成 */
    ISP_DelayUs(hisp, 10000);  /* EEPROM写入约8.5ms */

    return HAL_OK;
}

/**
 * @brief  ISP读EEPROM字节
 * @param  hisp: ISP句柄
 * @param  addr: EEPROM地址
 * @param  data: 数据指针
 * @retval 状态
 */
HAL_StatusTypeDef ISP_ReadEEPROM(ISP_HandleTypeDef* hisp, uint16_t addr, uint8_t* data)
{
    uint8_t response;

    /* 读EEPROM命令 */
    response = ISP_TransferByte(hisp, ISP_CMD_READ_EEPROM);
    response = ISP_TransferByte(hisp, (addr >> 8) & 0xFF);
    response = ISP_TransferByte(hisp, addr & 0xFF);
    response = ISP_TransferByte(hisp, 0x00);

    *data = response;

    return HAL_OK;
}

/**
 * @brief  ISP块写EEPROM
 * @param  hisp: ISP句柄
 * @param  addr: EEPROM地址
 * @param  data: 数据缓冲区
 * @param  len: 数据长度
 * @retval 状态
 */
HAL_StatusTypeDef ISP_WriteEEPROMBlock(ISP_HandleTypeDef* hisp, uint16_t addr, uint8_t* data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        ISP_WriteEEPROM(hisp, addr + i, data[i]);
    }

    return HAL_OK;
}

/**
 * @brief  ISP块读EEPROM
 * @param  hisp: ISP句柄
 * @param  addr: EEPROM地址
 * @param  data: 数据缓冲区
 * @param  len: 数据长度
 * @retval 状态
 */
HAL_StatusTypeDef ISP_ReadEEPROMBlock(ISP_HandleTypeDef* hisp, uint16_t addr, uint8_t* data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        ISP_ReadEEPROM(hisp, addr + i, &data[i]);
    }

    return HAL_OK;
}

/*============================================================================*
 * ISP (AVR) 熔丝位和锁定位操作
 *============================================================================*/

/**
 * @brief  ISP读熔丝位
 * @param  hisp: ISP句柄
 * @param  fuses: 熔丝位数据指针 (fuses[0]=低, fuses[1]=高, fuses[2]=扩展)
 * @retval 状态
 */
HAL_StatusTypeDef ISP_ReadFuseBits(ISP_HandleTypeDef* hisp, uint8_t* fuses)
{
    uint8_t response;

    /* 读低熔丝位 */
    response = ISP_TransferByte(hisp, ISP_CMD_READ_FUSE_BITS);
    response = ISP_TransferByte(hisp, 0x00);
    fuses[0] = ISP_TransferByte(hisp, 0x00);
    fuses[0] = ISP_TransferByte(hisp, 0x00);

    /* 读高熔丝位 */
    response = ISP_TransferByte(hisp, ISP_CMD_READ_FUSE_HIGH);
    response = ISP_TransferByte(hisp, 0x00);
    fuses[1] = ISP_TransferByte(hisp, 0x00);
    fuses[1] = ISP_TransferByte(hisp, 0x00);

    /* 读扩展熔丝位 */
    response = ISP_TransferByte(hisp, ISP_CMD_READ_EXT_FUSE);
    response = ISP_TransferByte(hisp, 0x00);
    fuses[2] = ISP_TransferByte(hisp, 0x00);
    fuses[2] = ISP_TransferByte(hisp, 0x00);

    return HAL_OK;
}

/**
 * @brief  ISP写熔丝位
 * @param  hisp: ISP句柄
 * @param  fuses: 熔丝位数据 (仅低字节有效)
 * @retval 状态
 */
HAL_StatusTypeDef ISP_WriteFuseBits(ISP_HandleTypeDef* hisp, uint8_t fuses)
{
    /* 写低熔丝位 */
    ISP_TransferByte(hisp, ISP_CMD_WRITE_FUSE_BITS);
    ISP_TransferByte(hisp, 0xA0);
    ISP_TransferByte(hisp, 0x00);
    ISP_TransferByte(hisp, fuses);

    ISP_DelayUs(hisp, 5000);

    return HAL_OK;
}

/**
 * @brief  ISP读锁定位
 * @param  hisp: ISP句柄
 * @param  locks: 锁定位数据指针
 * @retval 状态
 */
HAL_StatusTypeDef ISP_ReadLockBits(ISP_HandleTypeDef* hisp, uint8_t* locks)
{
    uint8_t response;

    response = ISP_TransferByte(hisp, ISP_CMD_READ_LOCK_BITS);
    response = ISP_TransferByte(hisp, 0x00);
    *locks = ISP_TransferByte(hisp, 0x00);
    *locks = ISP_TransferByte(hisp, 0x00);

    return HAL_OK;
}

/**
 * @brief  ISP写锁定位
 * @param  hisp: ISP句柄
 * @param  locks: 锁定位数据
 * @retval 状态
 */
HAL_StatusTypeDef ISP_WriteLockBits(ISP_HandleTypeDef* hisp, uint8_t locks)
{
    ISP_TransferByte(hisp, ISP_CMD_WRITE_LOCK_BITS);
    ISP_TransferByte(hisp, 0xE0);
    ISP_TransferByte(hisp, 0x00);
    ISP_TransferByte(hisp, locks);

    ISP_DelayUs(hisp, 5000);

    return HAL_OK;
}

/**
 * @brief  ISP读校准值
 * @param  hisp: ISP句柄
 * @param  cal: 校准值数据指针
 * @retval 状态
 */
HAL_StatusTypeDef ISP_ReadCalibration(ISP_HandleTypeDef* hisp, uint8_t* cal)
{
    uint8_t response;

    response = ISP_TransferByte(hisp, ISP_CMD_READ_CALIB);
    response = ISP_TransferByte(hisp, 0x00);
    *cal = ISP_TransferByte(hisp, 0x00);
    *cal = ISP_TransferByte(hisp, 0x00);

    return HAL_OK;
}

/*============================================================================*
 * ISP (AVR) 芯片信息读取
 *============================================================================*/

/**
 * @brief  ISP读芯片签名
 * @param  hisp: ISP句柄
 * @retval 芯片签名(24位)
 */
uint32_t ISP_ReadSignature(ISP_HandleTypeDef* hisp)
{
    uint32_t signature = 0;
    uint8_t sig[3];

    /* 读取三个签名字节 */
    for (uint8_t i = 0; i < 3; i++) {
        ISP_TransferByte(hisp, ISP_CMD_READ_SIGNATURE);
        ISP_TransferByte(hisp, 0x00);
        ISP_TransferByte(hisp, i);
        sig[i] = ISP_TransferByte(hisp, 0x00);
    }

    signature = (sig[2] << 16) | (sig[1] << 8) | sig[0];

    hisp->signature = signature;

    return signature;
}

/**
 * @brief  ISP获取设备ID
 * @param  hisp: ISP句柄
 * @retval 设备ID
 */
uint32_t ISP_GetDeviceID(ISP_HandleTypeDef* hisp)
{
    return hisp->signature;
}

/*============================================================================*
 * ISP (AVR) 初始化/反初始化
 *============================================================================*/

/**
 * @brief  ISP初始化
 * @param  hisp: ISP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ISP_Init(ISP_HandleTypeDef* hisp)
{
    if (hisp == NULL) {
        return HAL_ERROR;
    }

    /* 复制配置到全局句柄 */
    memcpy(&g_isp_handle, hisp, sizeof(ISP_HandleTypeDef));

    /* 初始化GPIO */
    ISP_GPIO_Init(&g_isp_handle);

    /* 初始化软件SPI(如果没有硬件SPI) */
    if (!g_isp_handle.use_hardware_spi) {
        ISP_SoftInit(&g_isp_handle);
    }

    /* 设置默认速度 */
    if (g_isp_handle.speed_hz == 0) {
        g_isp_handle.speed_hz = ISP_DEFAULT_CLOCK;
    }

    g_isp_handle.initialized = 1;
    g_isp_handle.state = ISP_STATE_IDLE;

    return HAL_OK;
}

/**
 * @brief  ISP反初始化
 * @param  hisp: ISP句柄
 * @retval 状态
 */
HAL_StatusTypeDef ISP_DeInit(ISP_HandleTypeDef* hisp)
{
    /* 反初始化GPIO */
    ISP_GPIO_DeInit(&g_isp_handle);

    /* 清除状态 */
    memset(&g_isp_handle, 0, sizeof(ISP_HandleTypeDef));

    return HAL_OK;
}
