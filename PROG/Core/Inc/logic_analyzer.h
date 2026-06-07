/**
 ******************************************************************************
 * @file    logic_analyzer.h
 * @brief   8通道10MHz逻辑分析仪驱动头文件
 *          支持实时采样、触发、协议解码等功能
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#ifndef __LOGIC_ANALYZER_H__
#define __LOGIC_ANALYZER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* ==================== 逻辑分析仪参数定义 ==================== */
#define LA_CHANNEL_COUNT            8           /* 8通道 */
#define LA_MAX_SAMPLE_RATE          10000000    /* 最高10MHz采样率 */
#define LA_MIN_SAMPLE_RATE          1000        /* 最低1kHz采样率 */
#define LA_BUFFER_SIZE              (512*1024)  /* 512KB采样缓冲区 */
#define LA_MAX_TRIGGER_DEPTH        256         /* 最大触发深度 */

/* ==================== 通道定义 ==================== */
typedef enum {
    LA_CH0 = 0,        /* 通道0 */
    LA_CH1,            /* 通道1 */
    LA_CH2,            /* 通道2 */
    LA_CH3,            /* 通道3 */
    LA_CH4,            /* 通道4 */
    LA_CH5,            /* 通道5 */
    LA_CH6,            /* 通道6 */
    LA_CH7,            /* 通道7 */
} LA_Channel_t;

/* ==================== 触发类型定义 ==================== */
typedef enum {
    LA_TRIGGER_NONE = 0,        /* 无触发(立即采样) */
    LA_TRIGGER_EDGE_RISING,     /* 上升沿触发 */
    LA_TRIGGER_EDGE_FALLING,    /* 下降沿触发 */
    LA_TRIGGER_EDGE_BOTH,       /* 双边沿触发 */
    LA_TRIGGER_LEVEL_HIGH,      /* 高电平触发 */
    LA_TRIGGER_LEVEL_LOW,       /* 低电平触发 */
    LA_TRIGGER_PATTERN,         /* 模式触发(多通道) */
    LA_TRIGGER_PULSE_WIDTH,     /* 脉宽触发 */
    LA_TRIGGER_PROTOCOL,        /* 协议触发 */
} LA_Trigger_Type_t;

/* ==================== 协议解码类型 ==================== */
typedef enum {
    LA_PROTOCOL_NONE = 0,       /* 不解码 */
    LA_PROTOCOL_UART,           /* UART协议解码 */
    LA_PROTOCOL_SPI,            /* SPI协议解码 */
    LA_PROTOCOL_I2C,            /* I2C协议解码 */
    LA_PROTOCOL_SWD,            /* SWD协议解码 */
    LA_PROTOCOL_JTAG,           /* JTAG协议解码 */
    LA_PROTOCOL_CAN,            /* CAN协议解码 */
    LA_PROTOCOL_BDM,           /* BDM协议解码 */
    LA_PROTOCOL_SBW,            /* SBW协议解码 */
    LA_PROTOCOL_MON8,           /* MON8协议解码 */
    LA_PROTOCOL_FINE,           /* FINE协议解码 */
} LA_Protocol_t;

/* ==================== 采样数据结构 ==================== */
typedef struct {
    uint8_t  channels;           /* 通道数据(每通道1位，共8位) */
    uint32_t timestamp;          /* 时间戳(采样点序号) */
} LA_Sample_t;

/* ==================== 触发配置结构体 ==================== */
typedef struct {
    LA_Trigger_Type_t    type;               /* 触发类型 */
    LA_Channel_t         channel;            /* 触发通道 */
    uint8_t              trigger_pattern;    /* 触发模式(8位对应8通道) */
    uint32_t             pre_trigger_count;  /* 前触发采样数 */
    uint32_t             post_trigger_count; /* 后触发采样数 */
    uint32_t             pulse_width_min;    /* 最小脉宽(采样周期数) */
    uint32_t             pulse_width_max;    /* 最大脉宽(采样周期数) */
} LA_Trigger_Config_t;

/* ==================== 采样配置结构体 ==================== */
typedef struct {
    uint32_t             sample_rate;        /* 采样率(Hz) */
    uint32_t             sample_count;       /* 采样点数 */
    uint32_t             sample_depth;       /* 采样深度 */
    uint8_t              channel_mask;       /* 通道掩码(8位) */
    LA_Trigger_Config_t  trigger;            /* 触发配置 */
    LA_Protocol_t        protocol;           /* 协议解码类型 */
    uint8_t              protocol_channels[6];/* 协议通道映射(扩展到6通道支持JTAG) */
} LA_Config_t;

/* ==================== 逻辑分析仪状态 ==================== */
typedef enum {
    LA_STATE_IDLE = 0,         /* 空闲 */
    LA_STATE_ARMED,            /* 已就绪(等待触发) */
    LA_STATE_SAMPLING,         /* 正在采样 */
    LA_STATE_DONE,             /* 采样完成 */
    LA_STATE_ERROR,            /* 错误 */
} LA_State_t;

/* ==================== 逻辑分析仪句柄 ==================== */
typedef struct {
    LA_Config_t          config;             /* 配置 */
    LA_State_t           state;              /* 当前状态 */
    uint8_t*             buffer;             /* 采样缓冲区 */
    uint32_t             buffer_size;        /* 缓冲区大小 */
    uint32_t             samples_captured;   /* 已采样点数 */
    uint32_t             trigger_position;   /* 触发位置 */
    uint32_t             overflow_count;     /* 溢出计数 */
    
    /* GPIO引脚配置(8通道) */
    GPIO_TypeDef*        ch_ports[8];        /* 各通道端口 */
    uint16_t             ch_pins[8];         /* 各通道引脚 */
    
    /* 定时器 */
    TIM_HandleTypeDef*   htim;               /* 采样定时器 */
    
    /* DMA模式 */
    DMA_HandleTypeDef*   hdma;               /* DMA句柄(DMA模式采样) */
    uint8_t              use_dma;            /* 是否使用DMA模式(1=DMA 0=中断) */
    volatile uint8_t     dma_complete;       /* DMA传输完成标志 */
    uint32_t             dma_half_index;     /* DMA半传输中断索引 */
} LA_HandleTypeDef;

/* ==================== 协议解码结果 ==================== */
typedef struct {
    LA_Protocol_t        protocol;           /* 协议类型 */
    uint32_t             frame_count;        /* 帧数 */
    uint32_t             error_count;        /* 错误数 */
    uint8_t*             decoded_data;       /* 解码数据 */
    uint32_t             decoded_size;       /* 解码数据大小 */
} LA_Decode_Result_t;

/* ==================== UART解码帧 ==================== */
typedef struct {
    uint32_t             start_sample;       /* 起始采样点 */
    uint32_t             baud_rate;          /* 波特率 */
    uint8_t              data;               /* 数据 */
    uint8_t              parity_error;       /* 偶校验错误 */
    uint8_t              frame_error;        /* 帧错误 */
} LA_UART_Frame_t;

/* ==================== SPI解码帧 ==================== */
typedef struct {
    uint32_t             start_sample;       /* 起始采样点 */
    uint32_t             clock_rate;         /* 时钟频率 */
    uint8_t              mosi_data;          /* MOSI数据 */
    uint8_t              miso_data;          /* MISO数据 */
    uint8_t              cs_state;           /* CS状态 */
} LA_SPI_Frame_t;

/* ==================== I2C解码帧 ==================== */
typedef struct {
    uint32_t             start_sample;       /* 起始采样点 */
    uint32_t             address;            /* 地址 */
    uint8_t              data;               /* 数据 */
    uint8_t              rw;                 /* 读/写 */
    uint8_t              ack;                /* ACK/NACK */
} LA_I2C_Frame_t;

/* ==================== SWD解码帧 ==================== */
typedef struct {
    uint32_t             start_sample;       /* 起始采样点 */
    uint8_t              operation;          /* 0=读 1=写 */
    uint8_t              ap_dp;              /* 0=DP 1=AP */
    uint8_t              addr;               /* A[2:3]地址 */
    uint32_t             data;               /* 32位数据 */
    uint8_t              ack;                /* ACK响应(OK/FAULT/WAIT) */
    uint8_t              parity_error;       /* 校验错误标志 */
} LA_SWD_Frame_t;

/* ==================== JTAG解码帧 ==================== */
typedef struct {
    uint32_t             start_sample;       /* 起始采样点 */
    uint8_t              tms_data;           /* TMS序列数据 */
    uint8_t              tms_len;            /* TMS序列长度 */
    uint32_t             tdi_data;           /* TDI数据 */
    uint32_t             tdo_data;           /* TDO数据 */
    uint8_t              data_len;           /* 数据位长度 */
    uint8_t              is_ir;              /* 1=IR移位 0=DR移位 */
} LA_JTAG_Frame_t;

/* ==================== CAN解码帧 ==================== */
typedef struct {
    uint32_t             start_sample;       /* 起始采样点 */
    uint32_t             id;                 /* CAN ID(11位标准/29位扩展) */
    uint8_t              is_extended;        /* 扩展帧标志 */
    uint8_t              is_rtr;             /* 远程帧标志 */
    uint8_t              dlc;                /* 数据长度 */
    uint8_t              data[8];            /* 数据 */
    uint8_t              crc_error;          /* CRC错误标志 */
    uint8_t              stuff_error;        /* 填充错误标志 */
} LA_CAN_Frame_t;

/* ==================== BDM解码帧 ==================== */
typedef struct {
    uint32_t             start_sample;       /* 起始采样点 */
    uint8_t              command;            /* BDM命令 */
    uint16_t             data_out;           /* 输出数据 */
    uint16_t             data_in;            /* 输入数据 */
    uint8_t              ack_error;          /* ACK错误标志 */
} LA_BDM_Frame_t;

/* ==================== SBW解码帧 ==================== */
typedef struct {
    uint32_t             start_sample;       /* 起始采样点 */
    uint8_t              tms;                /* TMS位 */
    uint8_t              tdi;                /* TDI位 */
    uint8_t              tdo;                /* TDO位 */
    uint8_t              is_reset;           /* 复位序列标志 */
} LA_SBW_Frame_t;

/* ==================== 协议通道映射扩展 ==================== */
#define LA_MAX_PROTOCOL_CHANNELS 6             /* 最大协议通道数(JTAG需5线) */

/* ==================== 函数声明 ==================== */

/* 初始化 */
HAL_StatusTypeDef LA_Init(LA_HandleTypeDef* hla);
HAL_StatusTypeDef LA_DeInit(LA_HandleTypeDef* hla);

/* 配置 */
HAL_StatusTypeDef LA_Configure(LA_HandleTypeDef* hla, LA_Config_t* config);
HAL_StatusTypeDef LA_SetSampleRate(LA_HandleTypeDef* hla, uint32_t rate_hz);
HAL_StatusTypeDef LA_SetTrigger(LA_HandleTypeDef* hla, LA_Trigger_Config_t* trigger);
HAL_StatusTypeDef LA_SetChannelMask(LA_HandleTypeDef* hla, uint8_t mask);
HAL_StatusTypeDef LA_SetProtocol(LA_HandleTypeDef* hla, LA_Protocol_t protocol, uint8_t* channels);

/* 采样 */
HAL_StatusTypeDef LA_Arm(LA_HandleTypeDef* hla);       /* 就绪(等待触发) */
HAL_StatusTypeDef LA_Start(LA_HandleTypeDef* hla);     /* 立即开始采样 */
HAL_StatusTypeDef LA_Stop(LA_HandleTypeDef* hla);      /* 停止采样 */
LA_State_t        LA_GetState(LA_HandleTypeDef* hla);  /* 获取状态 */

/* DMA采样模式 */
HAL_StatusTypeDef LA_StartDMA(LA_HandleTypeDef* hla);  /* DMA模式开始采样 */
HAL_StatusTypeDef LA_StopDMA(LA_HandleTypeDef* hla);   /* 停止DMA采样 */
void              LA_DMA_CompleteCallback(LA_HandleTypeDef* hla);  /* DMA完成回调 */
void              LA_DMA_HalfCallback(LA_HandleTypeDef* hla);      /* DMA半传输回调 */

/* 数据读取 */
uint32_t          LA_GetSampleCount(LA_HandleTypeDef* hla);
uint32_t          LA_GetTriggerPosition(LA_HandleTypeDef* hla);
HAL_StatusTypeDef LA_ReadSamples(LA_HandleTypeDef* hla, uint32_t start, uint32_t count, LA_Sample_t* samples);
HAL_StatusTypeDef LA_ReadRawSamples(LA_HandleTypeDef* hla, uint32_t start, uint32_t count, uint8_t* buffer);

/* 协议解码 */
HAL_StatusTypeDef LA_DecodeProtocol(LA_HandleTypeDef* hla, LA_Decode_Result_t* result);
HAL_StatusTypeDef LA_DecodeUART(LA_HandleTypeDef* hla, LA_UART_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count);
HAL_StatusTypeDef LA_DecodeSPI(LA_HandleTypeDef* hla, LA_SPI_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count);
HAL_StatusTypeDef LA_DecodeI2C(LA_HandleTypeDef* hla, LA_I2C_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count);
HAL_StatusTypeDef LA_DecodeSWD(LA_HandleTypeDef* hla, LA_SWD_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count);
HAL_StatusTypeDef LA_DecodeJTAG(LA_HandleTypeDef* hla, LA_JTAG_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count);
HAL_StatusTypeDef LA_DecodeCAN(LA_HandleTypeDef* hla, LA_CAN_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count);
HAL_StatusTypeDef LA_DecodeBDM(LA_HandleTypeDef* hla, LA_BDM_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count);
HAL_StatusTypeDef LA_DecodeSBW(LA_HandleTypeDef* hla, LA_SBW_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count);

/* 通道测量 */
uint32_t          LA_MeasureFrequency(LA_HandleTypeDef* hla, LA_Channel_t channel);
uint32_t          LA_MeasurePulseWidth(LA_HandleTypeDef* hla, LA_Channel_t channel, uint8_t polarity);
float             LA_MeasureDutyCycle(LA_HandleTypeDef* hla, LA_Channel_t channel);

#ifdef __cplusplus
}
#endif

#endif /* __LOGIC_ANALYZER_H__ */