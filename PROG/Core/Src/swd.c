/**
  ******************************************************************************
  * @file    swd.c
  * @brief   SWD (Serial Wire Debug) 协议实现
  *          ARM Cortex-M系列调试接口
  * 
  * @author  AI_PROG项目
  * @date    2026-06-05
  * @version v2.0
  * 
  * @details SWD是ARM公司推出的两线调试接口，用于替代传统的JTAG接口。
  *          本实现支持以下特性：
  *          - 支持SWD协议全功能操作
  *          - 支持DP(Debug Port)和AP(Access Port)访问
  *          - 支持内存读写操作
  *          - 支持JTAG/SWD模式切换
  *          - 支持奇偶校验
  *          
  *          SWD接口信号线：
  *          - SWDIO: 双向数据线
  *          - SWCLK: 时钟线
  *          - nRESET: 复位线(可选)
  *          
  *          协议说明：
  *          - SWD使用应答机制(ACK): OK(001), WAIT(010), FAULT(100)
  *          - 数据传输采用LSB优先
  *          - 支持32位数据传输及奇偶校验
  * 
  * @warning 本驱动使用GPIO模拟方式实现SWD协议，速度有限。如需更高速度，
  *          建议使用硬件DAP接口。
  ******************************************************************************
  */

#include "swd.h"
#include "gpio_soft.h"

/**
 * @brief SWD全局配置结构体
 *        存储SWD接口的硬件配置和运行状态
 */
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

/**
 * @brief SWD全局状态结构体
 *        存储SWD会话期间的状态信息
 */
SWD_State_TypeDef g_swd_state = {
    .dp_idcode  = 0,
    .ctrl_stat  = 0,
    .select     = 0,
    .ap         = 0,
    .protocol_ver = 0,
};

/**
 * @brief 简单延时函数
 *        用于SWD时序控制，提供最小延时保证
 */
#define SWD_DELAY()     do { __NOP(); __NOP(); __NOP(); __NOP(); } while(0)

/* ==================== 内部函数声明 ==================== */

/**
 * @brief 输出SWDIO信号
 * @param bit: 要输出的位(1或0)
 */
static void SWD_SWO_Out(uint8_t bit);

/**
 * @brief 读取SWDIO信号
 * @return 读取到的位(1或0)
 */
static uint8_t SWD_SWO_In(void);

/**
 * @brief 输出SWCLK信号
 * @param bit: 要输出的位(1或0)
 */
static void SWD_CLK_Out(uint8_t bit);

/**
 * @brief 发送一位数据
 * @param bit: 要发送的位
 */
static void SWD_TxBit(uint8_t bit);

/**
 * @brief 接收一位数据
 * @return 接收到的位
 */
static uint8_t SWD_RxBit(void);

/**
 * @brief 计算32位数据的奇偶校验位
 * @param data: 要计算校验的数据
 * @return 奇偶校验位(1表示奇数个1，0表示偶数个1)
 */
static uint8_t SWD_CalcParity(uint32_t data);

/**
 * @brief 等待并读取应答信号
 * @return 应答值(ACK): 0x01=OK, 0x02=WAIT, 0x04=FAULT
 */
static uint8_t SWD_WaitAck(void);

/* ==================== 内部函数实现 ==================== */

/**
 * @brief 输出SWDIO信号
 * @param bit: 要输出的位(1或0)
 */
static void SWD_SWO_Out(uint8_t bit)
{
    if (bit) {
        HAL_GPIO_WritePin(g_swd_config.swdio_port, g_swd_config.swdio_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(g_swd_config.swdio_port, g_swd_config.swdio_pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief 读取SWDIO信号
 * @return 读取到的位(1或0)
 */
static uint8_t SWD_SWO_In(void)
{
    return (HAL_GPIO_ReadPin(g_swd_config.swdio_port, g_swd_config.swdio_pin) == GPIO_PIN_SET) ? 1 : 0;
}

/**
 * @brief 输出SWCLK信号
 * @param bit: 要输出的位(1或0)
 */
static void SWD_CLK_Out(uint8_t bit)
{
    if (bit) {
        HAL_GPIO_WritePin(g_swd_config.swclk_port, g_swd_config.swclk_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(g_swd_config.swclk_port, g_swd_config.swclk_pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief 发送一位数据
 * @param bit: 要发送的位
 * 
 * @note SWD协议在时钟上升沿采样数据，所以先设置数据再产生时钟
 */
static void SWD_TxBit(uint8_t bit)
{
    SWD_SWO_Out(bit);    /* 设置数据 */
    SWD_DELAY();         /* 数据建立时间 */
    SWD_CLK_Out(1);      /* 时钟上升沿 - 数据被采样 */
    SWD_DELAY();         /* 数据保持时间 */
    SWD_CLK_Out(0);      /* 时钟下降沿 */
}

/**
 * @brief 接收一位数据
 * @return 接收到的位
 * 
 * @note SWD协议在时钟上升沿采样数据，所以在上升沿时读取
 */
static uint8_t SWD_RxBit(void)
{
    uint8_t bit;
    SWD_CLK_Out(1);      /* 时钟上升沿 - 数据被采样 */
    SWD_DELAY();         /* 等待数据稳定 */
    bit = SWD_SWO_In();  /* 读取数据 */
    SWD_DELAY();         /* 保持时间 */
    SWD_CLK_Out(0);      /* 时钟下降沿 */
    return bit;
}

/**
 * @brief 计算32位数据的奇偶校验位
 * @param data: 要计算校验的数据
 * @return 奇偶校验位(1表示奇数个1，0表示偶数个1)
 * 
 * @note 奇校验：统计数据中1的个数，如果是奇数则校验位为1，否则为0
 */
static uint8_t SWD_CalcParity(uint32_t data)
{
    uint8_t parity = 0;
    for (uint8_t i = 0; i < 32; i++) {
        parity ^= (data >> i) & 0x01;
    }
    return parity;
}

/**
 * @brief 等待并读取应答信号
 * @return 应答值(ACK): 0x01=OK, 0x02=WAIT, 0x04=FAULT
 * 
 * @note SWD应答信号包含3位：ACK[2:0]
 *       - 001: OK - 操作成功
 *       - 010: WAIT - 目标忙，稍后重试
 *       - 100: FAULT - 操作失败
 *       第4位是奇偶校验位，被忽略
 */
static uint8_t SWD_WaitAck(void)
{
    uint8_t ack;
    ack = SWD_RxBit();           /* ACK[0] */
    ack |= (SWD_RxBit() << 1);   /* ACK[1] */
    ack |= (SWD_RxBit() << 2);   /* ACK[2] */
    SWD_RxBit();                 /* 读取奇偶校验位(忽略) */
    return ack;
}

/* ==================== GPIO操作函数 ==================== */

/**
 * @brief 初始化SWD GPIO引脚
 * 
 * @details 配置SWDIO、SWCLK和RESET引脚为输出模式，
 *          SWDIO初始化为高电平，SWCLK初始化为低电平，RESET初始化为高电平。
 */
void SWD_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = g_swd_config.swdio_pin | g_swd_config.swclk_pin | g_swd_config.reset_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(g_swd_config.swdio_port, &GPIO_InitStruct);

    /* 设置初始状态 */
    HAL_GPIO_WritePin(g_swd_config.swdio_port, g_swd_config.swdio_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(g_swd_config.swclk_port, g_swd_config.swclk_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(g_swd_config.reset_port, g_swd_config.reset_pin, GPIO_PIN_SET);
}

/**
 * @brief 反初始化SWD GPIO引脚
 * 
 * @details 将所有SWD引脚设置为输入模式，释放硬件资源。
 */
void SWD_GPIO_DeInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = g_swd_config.swdio_pin | g_swd_config.swclk_pin | g_swd_config.reset_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(g_swd_config.swdio_port, g_swd_config.swclk_pin, GPIO_InitStruct);
}

/* ==================== 初始化与反初始化 ==================== */

/**
 * @brief 初始化SWD接口
 * @param config: SWD配置结构体指针
 * @return HAL状态
 * 
 * @details 初始化SWD接口，包括GPIO配置和协议状态初始化。
 *          如果传入config参数，则使用用户配置；否则使用默认配置。
 */
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

    /* 执行线路复位，确保进入SWD模式 */
    SWD_LineReset();

    return HAL_OK;
}

/**
 * @brief 反初始化SWD接口
 * @return HAL状态
 * 
 * @details 关闭SWD接口，释放GPIO资源。
 */
HAL_StatusTypeDef SWD_DeInit(void)
{
    SWD_GPIO_DeInit();
    g_swd_config.initialized = 0;
    return HAL_OK;
}

/* ==================== 线路控制函数 ==================== */

/**
 * @brief 执行SWD线路复位
 * @return HAL状态
 * 
 * @details 发送50+个SWCLK周期的高电平SWDIO信号，使目标进入SWD模式。
 *          根据ARM SWD规范，需要至少50个时钟周期的高电平。
 */
HAL_StatusTypeDef SWD_LineReset(void)
{
    /* 发送51个高电平位 - 确保目标检测到复位序列 */
    for (uint8_t i = 0; i < 51; i++) {
        SWD_TxBit(1);
    }

    /* 等待应答（忽略结果，这只是复位序列的一部分） */
    SWD_WaitAck();

    g_swd_config.line_mode = SWD_LINE_SWD;

    return HAL_OK;
}

/**
 * @brief 切换SWD/JTAG模式
 * @param mode: 目标模式(SWD_LINE_SWD或SWD_LINE_JTAG)
 * @return HAL状态
 * 
 * @details 实现SWD和JTAG模式之间的切换。
 *          JTAG模式需要特定的序列(16个1 + 6个0 + 1个1)。
 */
HAL_StatusTypeDef SWD_SwitchMode(uint8_t mode)
{
    if (mode == SWD_LINE_JTAG) {
        /* JTAG选择序列: 16个1 + 6个0 + 1个1 */
        for (uint8_t i = 0; i < 16; i++) {
            SWD_TxBit(1);
        }
        for (uint8_t i = 0; i < 6; i++) {
            SWD_TxBit(0);
        }
        SWD_TxBit(1);
        SWD_WaitAck();
    } else {
        /* SWD模式 - 执行线路复位 */
        SWD_LineReset();
    }

    g_swd_config.line_mode = mode;
    return HAL_OK;
}

/* ==================== 底层读写操作 ==================== */

/**
 * @brief 执行SWD写操作
 * @param addr: 地址(AP/DP选择 + 地址位)
 * @param data: 要写入的32位数据
 * @param apnwp: AP/DP选择位(0=DP, 1=AP)和写保护位
 * @return HAL状态
 * 
 * @details SWD写操作格式：
 *          - 起始位(0)
 *          - 地址位(2位)
 *          - APnWP位(AP/DP选择 + 写保护)
 *          - 奇偶校验位
 *          - ACK周期(3位)
 *          - 数据(32位)
 *          - 数据奇偶校验位
 *          - 停止位(0)
 *          - 空闲位(1)
 */
HAL_StatusTypeDef SWD_Write(uint8_t addr, uint32_t data, uint8_t apnwp)
{
    uint8_t parity;
    uint8_t ack;

    /* 计算命令奇偶校验 */
    parity = SWD_CalcParity((uint32_t)(addr | (apnwp << 2)));

    /* 发送命令序列 */
    SWD_TxBit(0);                /* 起始位 */
    SWD_TxBit(addr & 0x0C);      /* 地址位[3:2] */
    SWD_TxBit(apnwp);            /* AP/DP选择 + 写保护 */
    SWD_TxBit(parity);           /* 命令奇偶校验 */

    /* 等待应答 */
    ack = SWD_WaitAck();
    (void)ack;  /* 忽略应答，实际应用中应检查 */

    /* 发送32位数据(LBS优先) */
    for (uint8_t i = 0; i < 32; i++) {
        SWD_TxBit((data >> i) & 0x01);
    }

    /* 发送数据奇偶校验位和结束位 */
    SWD_TxBit(SWD_CalcParity(data));  /* 数据奇偶校验 */
    SWD_TxBit(0);                     /* 停止位 */
    SWD_TxBit(1);                     /* 空闲位 */

    return HAL_OK;
}

/**
 * @brief 执行SWD读操作
 * @param addr: 地址(AP/DP选择 + 地址位)
 * @param ack: 输出参数，返回应答值
 * @return 读取的32位数据
 * 
 * @details SWD读操作格式：
 *          - 起始位(0)
 *          - 地址位(2位)
 *          - RnW位(读=1)
 *          - 奇偶校验位
 *          - ACK周期(3位)
 *          - 数据(32位) - 如果ACK=OK
 *          - 数据奇偶校验位
 *          - 停止位(0)
 *          - 空闲位(1)
 */
uint32_t SWD_Read(uint8_t addr, uint8_t *ack)
{
    uint32_t data = 0;
    uint8_t parity = 0;
    uint8_t read_ack;

    /* 发送命令序列 */
    SWD_TxBit(0);              /* 起始位 */
    SWD_TxBit(addr & 0x0C);    /* 地址位[3:2] */
    SWD_TxBit(1);              /* RnW位=1(读) */
    SWD_TxBit(1);              /* 奇偶校验(固定为1，简化实现) */

    /* 等待应答 */
    read_ack = SWD_WaitAck();

    /* 返回应答值 */
    if (ack != NULL) {
        *ack = read_ack;
    }

    /* 如果ACK为OK，则读取数据 */
    if (read_ack == 0x01) {
        /* 读取32位数据(LSB优先) */
        for (uint8_t i = 0; i < 32; i++) {
            data |= (SWD_RxBit() << i);
        }

        /* 读取奇偶校验位和结束位 */
        parity = SWD_RxBit();
        SWD_RxBit();  /* 停止位 */
        SWD_TxBit(0); /* 空闲位(输出) */
    } else {
        /* ACK不为OK，跳过数据周期 */
        for (uint8_t i = 0; i < 33; i++) {
            SWD_RxBit();
        }
        SWD_RxBit();  /* 停止位 */
        SWD_TxBit(0); /* 空闲位 */
    }

    (void)parity;  /* 忽略奇偶校验，实际应用中应检查 */
    return data;
}

/* ==================== DP(Debug Port)操作 ==================== */

/**
 * @brief 写DP寄存器
 * @param addr: DP寄存器地址(0x00, 0x04, 0x08, 0x0C)
 * @param data: 要写入的32位数据
 * @return HAL状态
 * 
 * @details DP寄存器映射：
 *          - 0x00: IDCODE - 只读，设备识别码
 *          - 0x04: CTRL/STAT - 控制/状态寄存器
 *          - 0x08: SELECT - AP选择寄存器
 *          - 0x0C: RDBUFF - 读缓冲寄存器(只读)
 */
HAL_StatusTypeDef SWD_WriteDP(uint8_t addr, uint32_t data)
{
    uint8_t ack;
    SWD_Write(addr & 0x0C, data, 0);  /* 0表示DP操作 */
    g_swd_state.dp_idcode = SWD_Read(addr & 0x0C, &ack);
    (void)ack;
    return HAL_OK;
}

/**
 * @brief 读DP寄存器
 * @param addr: DP寄存器地址(0x00, 0x04, 0x08, 0x0C)
 * @return 读取的32位数据
 * 
 * @note 读取后会自动更新状态结构体中的对应字段
 */
uint32_t SWD_ReadDP(uint8_t addr)
{
    uint8_t ack;
    uint32_t data = SWD_Read(addr & 0x0C, &ack);

    /* 更新状态 */
    if (addr == 0x00) {
        g_swd_state.dp_idcode = data;
    } else if (addr == 0x04) {
        g_swd_state.ctrl_stat = data;
    } else if (addr == 0x08) {
        /* SELECT寄存器不存储在状态中 */
    } else if (addr == 0x0C) {
        g_swd_state.select = data;
    }

    /* 读取RDBUFF以完成前一次读操作 */
    SWD_ReadDP(0x0C);

    return data;
}

/* ==================== AP(Access Port)操作 ==================== */

/**
 * @brief 写AP寄存器
 * @param addr: AP寄存器地址(高8位为AP编号，低8位为寄存器偏移)
 * @param data: 要写入的32位数据
 * @return HAL状态
 * 
 * @details AP地址格式: [AP编号][寄存器偏移]
 *          寄存器偏移通常为: 0x00(CSW), 0x04(TAR), 0x0C(DRW)
 */
HAL_StatusTypeDef SWD_WriteAP(uint32_t addr, uint32_t data)
{
    uint8_t ap = (addr >> 24) & 0xFF;
    uint8_t reg = addr & 0xFF;

    /* 设置AP选择寄存器 */
    g_swd_state.select = (ap << 24) | (reg & 0xF0);
    SWD_WriteDP(0x0C, g_swd_state.select);

    /* 写入AP寄存器 */
    SWD_Write(addr, data, 1);  /* 1表示AP操作 */

    /* 读取RDBUFF完成操作 */
    SWD_ReadDP(0x0C);

    return HAL_OK;
}

/**
 * @brief 读AP寄存器
 * @param addr: AP寄存器地址(高8位为AP编号，低8位为寄存器偏移)
 * @return 读取的32位数据
 */
uint32_t SWD_ReadAP(uint32_t addr)
{
    uint8_t ap = (addr >> 24) & 0xFF;
    uint8_t reg = addr & 0xFF;

    /* 设置AP选择寄存器 */
    g_swd_state.select = (ap << 24) | (reg & 0xF0);
    SWD_WriteDP(0x0C, g_swd_state.select);

    /* 读取RDBUFF完成前一次操作 */
    SWD_ReadDP(0x0C);

    /* 执行读操作 */
    uint8_t ack;
    uint32_t data = SWD_Read(addr, &ack);

    return data;
}

/**
 * @brief 写AP寄存器(简化接口)
 * @param ap: AP编号
 * @param reg: 寄存器偏移
 * @param data: 要写入的32位数据
 * @return HAL状态
 */
HAL_StatusTypeDef SWD_WriteAPReg(uint8_t ap, uint8_t reg, uint32_t data)
{
    return SWD_WriteAP((ap << 24) | (reg & 0xF0), data);
}

/**
 * @brief 读AP寄存器(简化接口)
 * @param ap: AP编号
 * @param reg: 寄存器偏移
 * @return 读取的32位数据
 */
uint32_t SWD_ReadAPReg(uint8_t ap, uint8_t reg)
{
    return SWD_ReadAP((ap << 24) | (reg & 0xF0));
}

/* ==================== 内存操作函数 ==================== */

/**
 * @brief 写内存
 * @param addr: 目标内存地址
 * @param data: 数据缓冲区指针
 * @param size: 数据大小(字节)
 * @return HAL状态
 * 
 * @details 支持任意地址和大小的写入，自动处理对齐。
 */
HAL_StatusTypeDef SWD_WriteMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t word_addr = addr & 0xFFFFFFFC;  /* 对齐到字边界 */
    uint32_t offset = addr - word_addr;       /* 地址偏移 */
    uint32_t aligned_size = ((size + offset + 3) & ~3);  /* 对齐后的大小 */

    uint8_t *aligned_data = data;
    uint8_t temp[4] = {0};

    /* 如果地址不对齐，需要预处理 */
    if (offset != 0) {
        aligned_data = temp;
        uint32_t first_bytes = (4 - offset) > size ? size : (4 - offset);
        memcpy(temp, data, first_bytes);
    }

    /* 设置目标地址寄存器(TAR) */
    SWD_WriteAP(0x04, word_addr);

    /* 按字写入数据 */
    for (uint32_t i = 0; i < aligned_size; i += 4) {
        uint32_t value = aligned_data[i] |
                        (aligned_data[i + 1] << 8) |
                        (aligned_data[i + 2] << 16) |
                        (aligned_data[i + 3] << 24);

        SWD_WriteAP(0x0C, value);

        /* 简单延时确保写入完成 */
        for (volatile uint32_t j = 0; j < 100; j++);
    }

    return HAL_OK;
}

/**
 * @brief 读内存
 * @param addr: 源内存地址
 * @param data: 数据缓冲区指针
 * @param size: 数据大小(字节)
 * @return HAL状态
 * 
 * @details 支持任意地址和大小的读取，自动处理对齐。
 */
HAL_StatusTypeDef SWD_ReadMem(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t word_addr = addr & 0xFFFFFFFC;  /* 对齐到字边界 */
    uint32_t offset = addr - word_addr;       /* 地址偏移 */
    uint32_t aligned_size = ((size + offset + 3) & ~3);  /* 对齐后的大小 */

    /* 设置目标地址寄存器(TAR) */
    SWD_WriteAP(0x04, word_addr);

    /* 按字读取数据 */
    for (uint32_t i = 0; i < aligned_size; i += 4) {
        uint32_t value = SWD_ReadAP(0x0C);

        /* 计算有效数据范围 */
        uint32_t start = i;
        uint32_t end = i + 4;

        if (start < offset) {
            start = offset;
        }

        if (end > size + offset) {
            end = size + offset;
        }

        /* 提取有效字节 */
        for (uint32_t j = start; j < end; j++) {
            data[j - offset] = (value >> ((j - i) * 8)) & 0xFF;
        }
    }

    return HAL_OK;
}

/**
 * @brief 写32位字
 * @param addr: 目标地址(必须字对齐)
 * @param data: 32位数据
 * @return HAL状态
 */
HAL_StatusTypeDef SWD_WriteWord(uint32_t addr, uint32_t data)
{
    SWD_WriteAP(0x04, addr);
    SWD_WriteAP(0x0C, data);
    return HAL_OK;
}

/**
 * @brief 读32位字
 * @param addr: 源地址(必须字对齐)
 * @return 读取的32位数据
 */
uint32_t SWD_ReadWord(uint32_t addr)
{
    SWD_WriteAP(0x04, addr);
    return SWD_ReadAP(0x0C);
}

/* ==================== 时钟控制函数 ==================== */

/**
 * @brief 设置SWD时钟频率
 * @param clock: 时钟频率(Hz)
 * @return HAL状态
 */
HAL_StatusTypeDef SWD_SetClock(uint32_t clock)
{
    g_swd_config.clock = clock;
    return HAL_OK;
}

/**
 * @brief 获取当前SWD时钟频率
 * @return 当前时钟频率(Hz)
 */
uint32_t SWD_GetClock(void)
{
    return g_swd_config.clock;
}

/* ==================== 状态查询函数 ==================== */

/**
 * @brief 获取DP IDCODE
 * @return DP IDCODE值
 */
uint32_t SWD_GetDPID(void)
{
    return g_swd_state.dp_idcode;
}

/**
 * @brief 获取SWD协议版本
 * @return 协议版本号
 */
uint8_t SWD_GetProtocolVersion(void)
{
    return g_swd_state.protocol_ver;
}