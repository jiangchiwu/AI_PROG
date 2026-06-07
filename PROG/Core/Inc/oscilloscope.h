/**
 ******************************************************************************
 * @file    oscilloscope.h
 * @brief   8通道1MHz ADC示波器驱动头文件
 *          支持实时采样、触发、波形显示等功能
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#ifndef __OSCILLOSCOPE_H__
#define __OSCILLOSCOPE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* ==================== 示波器参数定义 ==================== */
#define SCO_CHANNEL_COUNT           8           /* 8通道 */
#define SCO_MAX_SAMPLE_RATE         1000000     /* 最高1MHz采样率 */
#define SCO_MIN_SAMPLE_RATE         1000        /* 最低1kHz采样率 */
#define SCO_BUFFER_SIZE             (256*1024)  /* 256KB采样缓冲区 */
#define SCO_MAX_CHANNELS            8           /* 最大8通道 */

/* ==================== ADC参数定义 ==================== */
#define SCO_ADC_RESOLUTION          12          /* 12位ADC */
#define SCO_ADC_VOLTAGE_RANGE_MV    3300        /* 电压范围3300mV */
#define SCO_MAX_VOLTAGE_MV          10000       /* 最大测量10V(分压后) */

/* ==================== 通道定义 ==================== */
typedef enum {
    SCO_CH0 = 0,       /* 通道0 */
    SCO_CH1,           /* 通道1 */
    SCO_CH2,           /* 通道2 */
    SCO_CH3,           /* 通道3 */
    SCO_CH4,           /* 通道4 */
    SCO_CH5,           /* 通道5 */
    SCO_CH6,           /* 通道6 */
    SCO_CH7,           /* 通道7 */
} SCO_Channel_t;

/* ==================== 触发类型定义 ==================== */
typedef enum {
    SCO_TRIGGER_NONE = 0,       /* 无触发(立即采样) */
    SCO_TRIGGER_EDGE_RISING,    /* 上升沿触发 */
    SCO_TRIGGER_EDGE_FALLING,   /* 下降沿触发 */
    SCO_TRIGGER_EDGE_BOTH,      /* 双边沿触发 */
    SCO_TRIGGER_LEVEL_HIGH,     /* 高电平触发 */
    SCO_TRIGGER_LEVEL_LOW,      /* 低电平触发 */
    SCO_TRIGGER_PULSE_WIDTH,    /* 脉宽触发 */
    SCO_TRIGGER_WINDOW,         /* 窗口触发 */
    SCO_TRIGGER_TIMEOUT,        /* 超时触发 */
} SCO_Trigger_Type_t;

/* ==================== 触发源定义 ==================== */
typedef enum {
    SCO_TRIGGER_SRC_CH0 = 0,    /* 通道0触发 */
    SCO_TRIGGER_SRC_CH1,        /* 通道1触发 */
    SCO_TRIGGER_SRC_CH2,        /* 通道2触发 */
    SCO_TRIGGER_SRC_CH3,        /* 通道3触发 */
    SCO_TRIGGER_SRC_CH4,        /* 通道4触发 */
    SCO_TRIGGER_SRC_CH5,        /* 通道5触发 */
    SCO_TRIGGER_SRC_CH6,        /* 通道6触发 */
    SCO_TRIGGER_SRC_CH7,        /* 通道7触发 */
    SCO_TRIGGER_SRC_EXT,        /* 外部触发 */
} SCO_Trigger_Source_t;

/* ==================== 垂直刻度定义 ==================== */
typedef enum {
    SCO_SCALE_1MV_DIV = 1,      /* 1mV/div */
    SCO_SCALE_10MV_DIV,         /* 10mV/div */
    SCO_SCALE_100MV_DIV,        /* 100mV/div */
    SCO_SCALE_1V_DIV,           /* 1V/div */
    SCO_SCALE_2V_DIV,           /* 2V/div */
    SCO_SCALE_5V_DIV,           /* 5V/div */
    SCO_SCALE_10V_DIV,          /* 10V/div */
} SCO_VScale_t;

/* ==================== 时间刻度定义 ==================== */
typedef enum {
    SCO_TIME_1US_DIV = 1,       /* 1μs/div */
    SCO_TIME_10US_DIV,          /* 10μs/div */
    SCO_TIME_100US_DIV,         /* 100μs/div */
    SCO_TIME_1MS_DIV,           /* 1ms/div */
    SCO_TIME_10MS_DIV,          /* 10ms/div */
    SCO_TIME_100MS_DIV,         /* 100ms/div */
    SCO_TIME_1S_DIV,            /* 1s/div */
} SCO_TScale_t;

/* ==================== 采样数据结构 ==================== */
typedef struct {
    uint16_t              value;             /* ADC采样值(12位) */
    uint32_t              timestamp;         /* 时间戳(采样点序号) */
} SCO_Sample_t;

/* ==================== 通道配置结构体 ==================== */
typedef struct {
    SCO_Channel_t         channel;           /* 通道号 */
    uint8_t               enable;            /* 启用标志 */
    SCO_VScale_t          v_scale;           /* 垂直刻度 */
    int16_t               v_offset;          /* 垂直偏移(mV) */
    uint8_t               coupling;          /* 耦合方式(DC/AC) */
    uint8_t               probe_atten;       /* 探头衰减(1x/10x) */
    uint8_t               bandwidth_limit;   /* 带宽限制 */
} SCO_Channel_Config_t;

/* ==================== 触发配置结构体 ==================== */
typedef struct {
    SCO_Trigger_Type_t    type;              /* 触发类型 */
    SCO_Trigger_Source_t  source;            /* 触发源 */
    uint32_t              level_mv;          /* 触发电平(mV) */
    uint32_t              hysteresis_mv;     /* 触发迟滞(mV) */
    uint32_t              pre_trigger_count; /* 前触发采样数 */
    uint32_t              post_trigger_count;/* 后触发采样数 */
    uint32_t              pulse_width_min;   /* 最小脉宽(采样周期数) */
    uint32_t              pulse_width_max;   /* 最大脉宽(采样周期数) */
    uint32_t              timeout_ms;        /* 超时时间(ms) */
} SCO_Trigger_Config_t;

/* ==================== 采样配置结构体 ==================== */
typedef struct {
    uint32_t              sample_rate;       /* 采样率(Hz) */
    uint32_t              sample_count;      /* 采样点数 */
    SCO_TScale_t          time_scale;        /* 时间刻度 */
    SCO_Channel_Config_t  channels[8];       /* 通道配置 */
    SCO_Trigger_Config_t  trigger;           /* 触发配置 */
} SCO_Config_t;

/* ==================== 示波器状态 ==================== */
typedef enum {
    SCO_STATE_IDLE = 0,       /* 空闲 */
    SCO_STATE_ARMED,          /* 已就绪(等待触发) */
    SCO_STATE_SAMPLING,       /* 正在采样 */
    SCO_STATE_DONE,           /* 采样完成 */
    SCO_STATE_ERROR,          /* 错误 */
} SCO_State_t;

/* ==================== 示波器句柄 ==================== */
typedef struct {
    SCO_Config_t           config;            /* 配置 */
    SCO_State_t            state;             /* 当前状态 */
    SCO_Sample_t**         buffers;           /* 采样缓冲区(每通道一个) */
    uint32_t               buffer_size;       /* 缓冲区大小 */
    uint32_t               samples_captured;  /* 已采样点数 */
    uint32_t               trigger_position;  /* 触发位置 */
    uint32_t               overflow_count;    /* 溢出计数 */
    
    /* ADC配置 */
    ADC_HandleTypeDef*     hadc;              /* ADC句柄 */
    uint32_t               adc_resolution;    /* ADC分辨率 */
    uint32_t               adc_voltage_range; /* ADC电压范围 */
    
    /* 定时器 */
    TIM_HandleTypeDef*     htim;              /* 采样定时器 */
    
    /* DMA配置 */
    DMA_HandleTypeDef*     hdma_adc;          /* DMA句柄 */
    uint8_t                use_dma;           /* 是否使用DMA */
    volatile uint8_t       dma_complete;      /* DMA完成标志 */
    uint32_t               dma_half_index;    /* 半传输索引 */
} SCO_HandleTypeDef;

/* ==================== 测量结果结构体 ==================== */
typedef struct {
    uint32_t              min_value;         /* 最小值(mV) */
    uint32_t              max_value;         /* 最大值(mV) */
    uint32_t              avg_value;         /* 平均值(mV) */
    uint32_t              pk_pk;             /* 峰峰值(mV) */
    uint32_t              rms;               /* RMS值(mV) */
    uint32_t              frequency_hz;      /* 频率(Hz) */
    float                 duty_cycle;        /* 占空比(%) */
    float                 rise_time_us;      /* 上升时间(μs) */
    float                 fall_time_us;      /* 下降时间(μs) */
    uint32_t              pulse_width_pos_us;/* 正脉宽(μs) */
    uint32_t              pulse_width_neg_us;/* 负脉宽(μs) */
} SCO_Measurement_t;

/* ==================== 数学运算类型 ==================== */
typedef enum {
    SCO_MATH_ADD = 0,         /* 加法(CH1+CH2) */
    SCO_MATH_SUB,             /* 减法(CH1-CH2) */
    SCO_MATH_MUL,             /*乘法(CH1*CH2) */
    SCO_MATH_DIV,             /* 除法(CH1/CH2) */
    SCO_MATH_FFT,             /* FFT分析 */
    SCO_MATH_DERIVATIVE,      /* 导数 */
    SCO_MATH_INTEGRAL,        /* 积分 */
    SCO_MATH_ABS,             /* 绝对值 */
    SCO_MATH_INVERT,          /* 反相 */
    SCO_MATH_FILTER,          /* 滤波 */
} SCO_Math_Type_t;

/* ==================== 函数声明 ==================== */

/* 初始化 */
HAL_StatusTypeDef SCO_Init(SCO_HandleTypeDef* hscope);
HAL_StatusTypeDef SCO_DeInit(SCO_HandleTypeDef* hscope);

/* 配置 */
HAL_StatusTypeDef SCO_Configure(SCO_HandleTypeDef* hscope, SCO_Config_t* config);
HAL_StatusTypeDef SCO_SetSampleRate(SCO_HandleTypeDef* hscope, uint32_t rate_hz);
HAL_StatusTypeDef SCO_SetTimeScale(SCO_HandleTypeDef* hscope, SCO_TScale_t scale);
HAL_StatusTypeDef SCO_SetChannelConfig(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, SCO_Channel_Config_t* config);
HAL_StatusTypeDef SCO_SetTrigger(SCO_HandleTypeDef* hscope, SCO_Trigger_Config_t* trigger);
HAL_StatusTypeDef SCO_EnableChannel(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, uint8_t enable);

/* 采样 */
HAL_StatusTypeDef SCO_Arm(SCO_HandleTypeDef* hscope);      /* 就绪(等待触发) */
HAL_StatusTypeDef SCO_Start(SCO_HandleTypeDef* hscope);    /* 立即开始采样 */
HAL_StatusTypeDef SCO_Stop(SCO_HandleTypeDef* hscope);     /* 停止采样 */
HAL_StatusTypeDef SCO_SingleShot(SCO_HandleTypeDef* hscope);/* 单次采样 */
HAL_StatusTypeDef SCO_RunContinuous(SCO_HandleTypeDef* hscope); /* 连续运行 */
SCO_State_t       SCO_GetState(SCO_HandleTypeDef* hscope); /* 获取状态 */

/* DMA采样 */
HAL_StatusTypeDef SCO_StartDMA(SCO_HandleTypeDef* hscope); /* DMA模式开始采样 */
HAL_StatusTypeDef SCO_StopDMA(SCO_HandleTypeDef* hscope);  /* DMA模式停止采样 */
void              SCO_DMA_CompleteCallback(SCO_HandleTypeDef* hscope); /* DMA完成回调 */
void              SCO_DMA_HalfCallback(SCO_HandleTypeDef* hscope);      /* DMA半传输回调 */

/* 数据读取 */
uint32_t          SCO_GetSampleCount(SCO_HandleTypeDef* hscope);
uint32_t          SCO_GetTriggerPosition(SCO_HandleTypeDef* hscope);
HAL_StatusTypeDef SCO_ReadChannelData(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, uint32_t start, uint32_t count, SCO_Sample_t* samples);
HAL_StatusTypeDef SCO_ReadRawData(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, uint32_t start, uint32_t count, uint16_t* buffer);

/* 测量 */
HAL_StatusTypeDef SCO_MeasureChannel(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, SCO_Measurement_t* measurement);
uint32_t          SCO_MeasureVoltage(SCO_HandleTypeDef* hscope, SCO_Channel_t channel);
uint32_t          SCO_MeasureFrequency(SCO_HandleTypeDef* hscope, SCO_Channel_t channel);
float             SCO_MeasureDutyCycle(SCO_HandleTypeDef* hscope, SCO_Channel_t channel);
float             SCO_MeasureRiseTime(SCO_HandleTypeDef* hscope, SCO_Channel_t channel);
float             SCO_MeasureFallTime(SCO_HandleTypeDef* hscope, SCO_Channel_t channel);

/* 数学运算 */
HAL_StatusTypeDef SCO_MathOperation(SCO_HandleTypeDef* hscope, SCO_Math_Type_t type, SCO_Channel_t ch1, SCO_Channel_t ch2, SCO_Sample_t* result);
HAL_StatusTypeDef SCO_FFT(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, float* magnitude, uint32_t size);
HAL_StatusTypeDef SCO_Filter(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, SCO_Sample_t* output, uint8_t filter_type, uint32_t cutoff_hz);

/* 校准 */
HAL_StatusTypeDef SCO_AutoCalibrate(SCO_HandleTypeDef* hscope);
HAL_StatusTypeDef SCO_SetCalibration(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, int16_t offset_mv, float gain);

/* 辅助功能 */
HAL_StatusTypeDef SCO_AutoSet(SCO_HandleTypeDef* hscope);  /* 自动设置 */
HAL_StatusTypeDef SCO_SaveWaveform(SCO_HandleTypeDef* hscope, const char* filepath);
HAL_StatusTypeDef SCO_LoadWaveform(SCO_HandleTypeDef* hscope, const char* filepath);
HAL_StatusTypeDef SCO_SaveConfig(SCO_HandleTypeDef* hscope, const char* filepath);
HAL_StatusTypeDef SCO_LoadConfig(SCO_HandleTypeDef* hscope, const char* filepath);

#ifdef __cplusplus
}
#endif

#endif /* __OSCILLOSCOPE_H__ */