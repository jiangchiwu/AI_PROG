/**
 ******************************************************************************
 * @file    oscilloscope.c
 * @brief   8通道1MHz ADC示波器驱动实现
 *          使用DMA+ADC+定时器实现高速采样
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#include "oscilloscope.h"
#include <string.h>
#include <math.h>

/* ==================== 全局变量 ==================== */
static SCO_HandleTypeDef* s_active_scope = NULL;

/* ==================== ADC值转电压 ==================== */
/**
 * @brief 将ADC原始值转换为电压值(mV)
 * @param adc_value: ADC原始值(12位)
 * @param config: 通道配置
 * @return 电压值(mV)
 */
static int32_t SCO_ADCToVoltage(uint16_t adc_value, SCO_Channel_Config_t* config)
{
    int32_t voltage;
    
    /* 12位ADC: 0-4095 -> 0-3300mV */
    voltage = (int32_t)adc_value * SCO_ADC_VOLTAGE_RANGE_MV / 4096;
    
    /* 应用探头衰减 */
    if (config->probe_atten == 10) {
        voltage *= 10;
    }
    
    /* 应用偏移 */
    voltage -= config->v_offset;
    
    return voltage;
}

/* ==================== 初始化函数 ==================== */

/**
 * @brief 初始化示波器
 */
HAL_StatusTypeDef SCO_Init(SCO_HandleTypeDef* hscope)
{
    if (hscope == NULL) return HAL_ERROR;
    
    /* 初始化默认配置 */
    for (int i = 0; i < SCO_MAX_CHANNELS; i++) {
        hscope->config.channels[i].channel = i;
        hscope->config.channels[i].enable = 0;
        hscope->config.channels[i].v_scale = SCO_SCALE_1V_DIV;
        hscope->config.channels[i].v_offset = 0;
        hscope->config.channels[i].coupling = 0;  /* DC */
        hscope->config.channels[i].probe_atten = 1;
        hscope->config.channels[i].bandwidth_limit = 0;
    }
    
    /* 默认启用通道0和1 */
    hscope->config.channels[0].enable = 1;
    hscope->config.channels[1].enable = 1;
    
    /* 默认采样配置 */
    hscope->config.sample_rate = 1000000;  /* 1MHz */
    hscope->config.sample_count = 1024;
    hscope->config.time_scale = SCO_TIME_10US_DIV;
    
    /* 默认触发配置 */
    hscope->config.trigger.type = SCO_TRIGGER_EDGE_RISING;
    hscope->config.trigger.source = SCO_TRIGGER_SRC_CH0;
    hscope->config.trigger.level_mv = 500;  /* 500mV */
    hscope->config.trigger.hysteresis_mv = 50;
    hscope->config.trigger.pre_trigger_count = 256;
    hscope->config.trigger.post_trigger_count = 768;
    
    hscope->state = SCO_STATE_IDLE;
    hscope->samples_captured = 0;
    hscope->trigger_position = 0;
    hscope->overflow_count = 0;
    
    return HAL_OK;
}

/**
 * @brief 反初始化示波器
 */
HAL_StatusTypeDef SCO_DeInit(SCO_HandleTypeDef* hscope)
{
    if (hscope == NULL) return HAL_ERROR;
    
    if (hscope->state == SCO_STATE_SAMPLING) {
        SCO_Stop(hscope);
    }
    
    hscope->state = SCO_STATE_IDLE;
    s_active_scope = NULL;
    
    return HAL_OK;
}

/**
 * @brief 配置示波器
 */
HAL_StatusTypeDef SCO_Configure(SCO_HandleTypeDef* hscope, SCO_Config_t* config)
{
    if (hscope == NULL || config == NULL) return HAL_ERROR;
    
    if (config->sample_rate > SCO_MAX_SAMPLE_RATE || config->sample_rate < SCO_MIN_SAMPLE_RATE) {
        return HAL_ERROR;
    }
    
    memcpy(&hscope->config, config, sizeof(SCO_Config_t));
    return HAL_OK;
}

/**
 * @brief 设置采样率
 */
HAL_StatusTypeDef SCO_SetSampleRate(SCO_HandleTypeDef* hscope, uint32_t rate_hz)
{
    if (rate_hz > SCO_MAX_SAMPLE_RATE || rate_hz < SCO_MIN_SAMPLE_RATE) {
        return HAL_ERROR;
    }
    
    hscope->config.sample_rate = rate_hz;
    
    /* 配置ADC定时器 */
    if (hscope->htim != NULL) {
        uint32_t timer_clock = HAL_RCC_GetPCLK2Freq();
        uint32_t prescaler = (timer_clock / rate_hz) - 1;
        __HAL_TIM_SET_PRESCALER(hscope->htim, prescaler);
    }
    
    return HAL_OK;
}

/**
 * @brief 设置通道配置
 */
HAL_StatusTypeDef SCO_SetChannelConfig(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, SCO_Channel_Config_t* config)
{
    if (channel >= SCO_MAX_CHANNELS) return HAL_ERROR;
    memcpy(&hscope->config.channels[channel], config, sizeof(SCO_Channel_Config_t));
    return HAL_OK;
}

/**
 * @brief 设置触发
 */
HAL_StatusTypeDef SCO_SetTrigger(SCO_HandleTypeDef* hscope, SCO_Trigger_Config_t* trigger)
{
    memcpy(&hscope->config.trigger, trigger, sizeof(SCO_Trigger_Config_t));
    return HAL_OK;
}

/**
 * @brief 启用/禁用通道
 */
HAL_StatusTypeDef SCO_EnableChannel(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, uint8_t enable)
{
    if (channel >= SCO_MAX_CHANNELS) return HAL_ERROR;
    hscope->config.channels[channel].enable = enable;
    return HAL_OK;
}

/**
 * @brief 就绪(等待触发)
 */
HAL_StatusTypeDef SCO_Arm(SCO_HandleTypeDef* hscope)
{
    hscope->samples_captured = 0;
    hscope->trigger_position = 0;
    s_active_scope = hscope;
    hscope->state = SCO_STATE_ARMED;
    
    /* 启动ADC DMA采样 */
    if (hscope->hadc != NULL) {
        HAL_ADC_Start_DMA(hscope->hadc, (uint32_t*)hscope->buffers[0], hscope->config.sample_count);
    }
    
    /* 启动定时器 */
    if (hscope->htim != NULL) {
        HAL_TIM_Base_Start_IT(hscope->htim);
    }
    
    return HAL_OK;
}

/**
 * @brief 立即开始采样
 */
HAL_StatusTypeDef SCO_Start(SCO_HandleTypeDef* hscope)
{
    hscope->config.trigger.type = SCO_TRIGGER_NONE;
    return SCO_Arm(hscope);
}

/**
 * @brief 停止采样
 */
HAL_StatusTypeDef SCO_Stop(SCO_HandleTypeDef* hscope)
{
    /* 停止ADC DMA */
    if (hscope->hadc != NULL) {
        HAL_ADC_Stop_DMA(hscope->hadc);
    }
    
    /* 停止定时器 */
    if (hscope->htim != NULL) {
        HAL_TIM_Base_Stop_IT(hscope->htim);
    }
    
    hscope->state = SCO_STATE_DONE;
    s_active_scope = NULL;
    
    return HAL_OK;
}

/**
 * @brief 单次采样
 */
HAL_StatusTypeDef SCO_SingleShot(SCO_HandleTypeDef* hscope)
{
    hscope->config.trigger.post_trigger_count = hscope->config.sample_count;
    return SCO_Arm(hscope);
}

/**
 * @brief 连续运行
 */
HAL_StatusTypeDef SCO_RunContinuous(SCO_HandleTypeDef* hscope)
{
    hscope->config.trigger.type = SCO_TRIGGER_NONE;
    hscope->config.trigger.post_trigger_count = 0;
    return SCO_Arm(hscope);
}

/**
 * @brief 获取状态
 */
SCO_State_t SCO_GetState(SCO_HandleTypeDef* hscope)
{
    return hscope->state;
}

/**
 * @brief 获取采样点数
 */
uint32_t SCO_GetSampleCount(SCO_HandleTypeDef* hscope)
{
    return hscope->samples_captured;
}

/**
 * @brief 获取触发位置
 */
uint32_t SCO_GetTriggerPosition(SCO_HandleTypeDef* hscope)
{
    return hscope->trigger_position;
}

/**
 * @brief 读取通道数据
 */
HAL_StatusTypeDef SCO_ReadChannelData(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, uint32_t start, uint32_t count, SCO_Sample_t* samples)
{
    if (channel >= SCO_MAX_CHANNELS) return HAL_ERROR;
    if (start + count > hscope->samples_captured) return HAL_ERROR;
    
    for (uint32_t i = 0; i < count; i++) {
        samples[i].value = hscope->buffers[channel][start + i].value;
        samples[i].timestamp = start + i;
    }
    
    return HAL_OK;
}

/**
 * @brief 读取原始数据
 */
HAL_StatusTypeDef SCO_ReadRawData(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, uint32_t start, uint32_t count, uint16_t* buffer)
{
    if (channel >= SCO_MAX_CHANNELS) return HAL_ERROR;
    if (start + count > hscope->samples_captured) return HAL_ERROR;
    
    for (uint32_t i = 0; i < count; i++) {
        buffer[i] = hscope->buffers[channel][start + i].value;
    }
    
    return HAL_OK;
}

/**
 * @brief 测量电压
 */
uint32_t SCO_MeasureVoltage(SCO_HandleTypeDef* hscope, SCO_Channel_t channel)
{
    if (hscope->samples_captured == 0) return 0;
    
    uint32_t avg = 0;
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < hscope->samples_captured; i++) {
        avg += hscope->buffers[channel][i].value;
        count++;
    }
    
    return (avg / count) * SCO_ADC_VOLTAGE_RANGE_MV / 4096;
}

/**
 * @brief 测量频率
 */
uint32_t SCO_MeasureFrequency(SCO_HandleTypeDef* hscope, SCO_Channel_t channel)
{
    uint32_t trigger_level = hscope->config.trigger.level_mv * 4096 / SCO_ADC_VOLTAGE_RANGE_MV;
    uint32_t crossings = 0;
    uint16_t prev_value = 0;
    uint8_t prev_above = 0;
    
    for (uint32_t i = 0; i < hscope->samples_captured; i++) {
        uint16_t value = hscope->buffers[channel][i].value;
        uint8_t above = (value >= trigger_level) ? 1 : 0;
        
        /* 计算上升沿穿越 */
        if (!prev_above && above) crossings++;
        
        prev_value = value;
        prev_above = above;
    }
    
    if (crossings > 0) {
        uint64_t total_time_us = (uint64_t)hscope->samples_captured * 1000000ULL / hscope->config.sample_rate;
        return (uint32_t)((uint64_t)crossings * 1000000ULL / total_time_us);
    }
    
    return 0;
}

/**
 * @brief 测量占空比
 */
float SCO_MeasureDutyCycle(SCO_HandleTypeDef* hscope, SCO_Channel_t channel)
{
    uint32_t trigger_level = hscope->config.trigger.level_mv * 4096 / SCO_ADC_VOLTAGE_RANGE_MV;
    uint32_t high_count = 0;
    
    for (uint32_t i = 0; i < hscope->samples_captured; i++) {
        if (hscope->buffers[channel][i].value >= trigger_level) {
            high_count++;
        }
    }
    
    return (float)high_count / hscope->samples_captured * 100.0f;
}

/**
 * @brief 全面测量
 */
HAL_StatusTypeDef SCO_MeasureChannel(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, SCO_Measurement_t* measurement)
{
    if (hscope->samples_captured == 0) return HAL_ERROR;
    
    uint32_t min_val = 0xFFFF;
    uint32_t max_val = 0;
    uint64_t sum = 0;
    uint64_t sum_sq = 0;
    
    for (uint32_t i = 0; i < hscope->samples_captured; i++) {
        uint16_t val = hscope->buffers[channel][i].value;
        int32_t voltage = SCO_ADCToVoltage(val, &hscope->config.channels[channel]);
        
        if (voltage < min_val) min_val = voltage;
        if (voltage > max_val) max_val = voltage;
        sum += voltage;
        sum_sq += (uint64_t)voltage * voltage;
    }
    
    measurement->min_value = min_val;
    measurement->max_value = max_val;
    measurement->avg_value = (uint32_t)(sum / hscope->samples_captured);
    measurement->pk_pk = max_val - min_val;
    measurement->rms = (uint32_t)sqrtf((float)(sum_sq / hscope->samples_captured));
    measurement->frequency_hz = SCO_MeasureFrequency(hscope, channel);
    measurement->duty_cycle = SCO_MeasureDutyCycle(hscope, channel);
    
    return HAL_OK;
}

/**
 * @brief 自动设置
 */
HAL_StatusTypeDef SCO_AutoSet(SCO_HandleTypeDef* hscope)
{
    /* 先进行一次无触发采样 */
    SCO_Start(hscope);
    HAL_Delay(100);
    SCO_Stop(hscope);
    
    /* 分析采样数据 */
    SCO_Measurement_t meas;
    SCO_MeasureChannel(hscope, SCO_CH0, &meas);
    
    /* 根据测量结果自动设置参数 */
    uint32_t freq = meas.frequency_hz;
    uint32_t pk_pk = meas.pk_pk;
    
    /* 设置时间刻度 */
    if (freq > 0) {
        uint32_t period_us = 1000000 / freq;
        /* 选择合适的时间刻度，使屏幕显示2-5个周期 */
        if (period_us < 10) hscope->config.time_scale = SCO_TIME_1US_DIV;
        else if (period_us < 100) hscope->config.time_scale = SCO_TIME_10US_DIV;
        else if (period_us < 1000) hscope->config.time_scale = SCO_TIME_100US_DIV;
        else if (period_us < 10000) hscope->config.time_scale = SCO_TIME_1MS_DIV;
        else hscope->config.time_scale = SCO_TIME_10MS_DIV;
    }
    
    /* 设置垂直刻度 */
    if (pk_pk < 100) hscope->config.channels[0].v_scale = SCO_SCALE_10MV_DIV;
    else if (pk_pk < 1000) hscope->config.channels[0].v_scale = SCO_SCALE_100MV_DIV;
    else if (pk_pk < 5000) hscope->config.channels[0].v_scale = SCO_SCALE_1V_DIV;
    else hscope->config.channels[0].v_scale = SCO_SCALE_5V_DIV;
    
    /* 设置触发电平 */
    hscope->config.trigger.level_mv = meas.avg_value;
    
    /* 设置采样率(至少10倍于信号频率) */
    if (freq > 0) {
        uint32_t min_rate = freq * 10;
        if (min_rate > SCO_MAX_SAMPLE_RATE) min_rate = SCO_MAX_SAMPLE_RATE;
        SCO_SetSampleRate(hscope, min_rate);
    }
    
    return HAL_OK;
}

/**
 * @brief 设置时间刻度
 */
HAL_StatusTypeDef SCO_SetTimeScale(SCO_HandleTypeDef* hscope, SCO_TScale_t scale)
{
    hscope->config.time_scale = scale;
    
    /* 根据时间刻度调整采样率 */
    /* 10个div, 每div需要至少100个采样点 */
    uint32_t div_time_us;
    switch (scale) {
        case SCO_TIME_1US_DIV:     div_time_us = 1; break;
        case SCO_TIME_10US_DIV:    div_time_us = 10; break;
        case SCO_TIME_100US_DIV:   div_time_us = 100; break;
        case SCO_TIME_1MS_DIV:     div_time_us = 1000; break;
        case SCO_TIME_10MS_DIV:    div_time_us = 10000; break;
        case SCO_TIME_100MS_DIV:   div_time_us = 100000; break;
        case SCO_TIME_1S_DIV:      div_time_us = 1000000; break;
        default:                   div_time_us = 1000; break;
    }
    
    /* 总时间 = 10 * div_time_us, 采样率 = 1000 / div_time_us */
    uint32_t total_time_us = 10 * div_time_us;
    uint32_t sample_rate = hscope->config.sample_count * 1000000 / total_time_us;
    
    if (sample_rate > SCO_MAX_SAMPLE_RATE) sample_rate = SCO_MAX_SAMPLE_RATE;
    if (sample_rate < SCO_MIN_SAMPLE_RATE) sample_rate = SCO_MIN_SAMPLE_RATE;
    
    return SCO_SetSampleRate(hscope, sample_rate);
}

/**
 * @brief 测量上升时间
 */
float SCO_MeasureRiseTime(SCO_HandleTypeDef* hscope, SCO_Channel_t channel)
{
    uint32_t trigger_level = hscope->config.trigger.level_mv * 4096 / SCO_ADC_VOLTAGE_RANGE_MV;
    uint32_t lower_10 = trigger_level - (trigger_level / 10);
    uint32_t upper_90 = trigger_level + (trigger_level * 9 / 10);
    
    uint32_t start_idx = 0;
    uint32_t end_idx = 0;
    uint8_t found_start = 0;
    
    for (uint32_t i = 0; i < hscope->samples_captured; i++) {
        uint16_t val = hscope->buffers[channel][i].value;
        
        if (!found_start && val >= lower_10) {
            start_idx = i;
            found_start = 1;
        }
        
        if (found_start && val >= upper_90) {
            end_idx = i;
            break;
        }
    }
    
    if (found_start && end_idx > start_idx) {
        return (float)(end_idx - start_idx) / hscope->config.sample_rate * 1000000.0f;
    }
    
    return 0.0f;
}

/**
 * @brief 测量下降时间
 */
float SCO_MeasureFallTime(SCO_HandleTypeDef* hscope, SCO_Channel_t channel)
{
    uint32_t trigger_level = hscope->config.trigger.level_mv * 4096 / SCO_ADC_VOLTAGE_RANGE_MV;
    uint32_t upper_90 = trigger_level + (trigger_level * 9 / 10);
    uint32_t lower_10 = trigger_level - (trigger_level / 10);
    
    uint32_t start_idx = 0;
    uint32_t end_idx = 0;
    uint8_t found_start = 0;
    
    for (uint32_t i = 0; i < hscope->samples_captured; i++) {
        uint16_t val = hscope->buffers[channel][i].value;
        
        if (!found_start && val <= upper_90) {
            start_idx = i;
            found_start = 1;
        }
        
        if (found_start && val <= lower_10) {
            end_idx = i;
            break;
        }
    }
    
    if (found_start && end_idx > start_idx) {
        return (float)(end_idx - start_idx) / hscope->config.sample_rate * 1000000.0f;
    }
    
    return 0.0f;
}

/**
 * @brief FFT分析
 */
HAL_StatusTypeDef SCO_FFT(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, float* magnitude, uint32_t size)
{
    /* 简化的FFT实现(实际应使用优化FFT库) */
    for (uint32_t k = 0; k < size / 2; k++) {
        float real = 0.0f;
        float imag = 0.0f;
        
        for (uint32_t n = 0; n < size; n++) {
            float sample = (float)SCO_ADCToVoltage(
                hscope->buffers[channel][n].value,
                &hscope->config.channels[channel]);
            
            float angle = 2.0f * 3.14159265359f * k * n / size;
            real += sample * cosf(angle);
            imag -= sample * sinf(angle);
        }
        
        magnitude[k] = sqrtf(real * real + imag * imag) / size;
    }
    
    return HAL_OK;
}

/**
 * @brief 数学运算
 */
HAL_StatusTypeDef SCO_MathOperation(SCO_HandleTypeDef* hscope, SCO_Math_Type_t type, SCO_Channel_t ch1, SCO_Channel_t ch2, SCO_Sample_t* result)
{
    for (uint32_t i = 0; i < hscope->samples_captured; i++) {
        int32_t v1 = SCO_ADCToVoltage(hscope->buffers[ch1][i].value, &hscope->config.channels[ch1]);
        int32_t v2 = SCO_ADCToVoltage(hscope->buffers[ch2][i].value, &hscope->config.channels[ch2]);
        
        switch (type) {
            case SCO_MATH_ADD:  result[i].value = v1 + v2; break;
            case SCO_MATH_SUB:  result[i].value = v1 - v2; break;
            case SCO_MATH_MUL:  result[i].value = v1 * v2 / 1000; break;
            case SCO_MATH_INVERT: result[i].value = -v1; break;
            case SCO_MATH_ABS:  result[i].value = abs(v1); break;
            default: break;
        }
        
        result[i].timestamp = i;
    }
    
    return HAL_OK;
}

/**
 * @brief 自动校准
 */
HAL_StatusTypeDef SCO_AutoCalibrate(SCO_HandleTypeDef* hscope)
{
    /* 校准流程：测量零点偏移和增益误差 */
    /* 实际实现需要对每个通道进行校准 */
    return HAL_OK;
}

/**
 * @brief 设置校准参数
 */
HAL_StatusTypeDef SCO_SetCalibration(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, int16_t offset_mv, float gain)
{
    hscope->config.channels[channel].v_offset = offset_mv;
    return HAL_OK;
}

/* ==================== DMA采样函数 ==================== */

/**
 * @brief DMA模式开始采样
 * @param hscope: 示波器句柄
 * @return HAL状态
 */
HAL_StatusTypeDef SCO_StartDMA(SCO_HandleTypeDef* hscope)
{
    if (hscope == NULL || hscope->hadc == NULL) {
        return HAL_ERROR;
    }
    
    /* 初始化DMA采样参数 */
    hscope->samples_captured = 0;
    hscope->trigger_position = 0;
    hscope->dma_complete = 0;
    hscope->dma_half_index = 0;
    hscope->use_dma = 1;
    hscope->state = SCO_STATE_SAMPLING;
    
    s_active_scope = hscope;
    
    /* 启动定时器 */
    if (hscope->htim != NULL) {
        HAL_TIM_Base_Start_IT(hscope->htim);
    }
    
    /* 启动ADC DMA采样 - 使用循环模式实现连续采样 */
    /* 注意：这里假设ADC已经配置好了通道序列 */
    uint32_t* buffer_ptr = (uint32_t*)hscope->buffers[0];
    HAL_StatusTypeDef status = HAL_ADC_Start_DMA(hscope->hadc, buffer_ptr, hscope->config.sample_count);
    
    return status;
}

/**
 * @brief DMA模式停止采样
 * @param hscope: 示波器句柄
 * @return HAL状态
 */
HAL_StatusTypeDef SCO_StopDMA(SCO_HandleTypeDef* hscope)
{
    if (hscope == NULL) {
        return HAL_ERROR;
    }
    
    /* 停止ADC DMA */
    if (hscope->hadc != NULL) {
        HAL_ADC_Stop_DMA(hscope->hadc);
    }
    
    /* 停止定时器 */
    if (hscope->htim != NULL) {
        HAL_TIM_Base_Stop_IT(hscope->htim);
    }
    
    hscope->use_dma = 0;
    hscope->dma_complete = 0;
    hscope->state = SCO_STATE_DONE;
    s_active_scope = NULL;
    
    return HAL_OK;
}

/**
 * @brief DMA完成回调
 * @param hscope: 示波器句柄
 */
void SCO_DMA_CompleteCallback(SCO_HandleTypeDef* hscope)
{
    if (hscope == NULL) return;
    
    hscope->dma_complete = 1;
    hscope->samples_captured = hscope->config.sample_count;
    
    /* 标记采样完成 */
    hscope->state = SCO_STATE_DONE;
    
    /* 停止ADC DMA(单次模式) */
    if (hscope->hadc != NULL) {
        HAL_ADC_Stop_DMA(hscope->hadc);
    }
    
    /* 停止定时器 */
    if (hscope->htim != NULL) {
        HAL_TIM_Base_Stop_IT(hscope->htim);
    }
}

/**
 * @brief DMA半传输回调
 * @param hscope: 示波器句柄
 */
void SCO_DMA_HalfCallback(SCO_HandleTypeDef* hscope)
{
    if (hscope == NULL) return;
    
    /* 更新半传输索引 */
    hscope->dma_half_index = hscope->config.sample_count / 2;
    
    /* 可以在此处理前半部分数据，实现乒乓缓冲 */
    /* 例如：处理hscope->buffers[0]到hscope->buffers[hscope->dma_half_index-1]的数据 */
}

/* ==================== FFT辅助函数 ==================== */

/**
 * @brief 位反转函数(Cooley-Tukey FFT)
 * @param index: 原始索引
 * @param bits: 索引位数
 * @return 反转后的索引
 */
static uint32_t SCO_BitReverse(uint32_t index, uint32_t bits)
{
    uint32_t reversed = 0;
    for (uint32_t i = 0; i < bits; i++) {
        reversed = (reversed << 1) | (index & 1);
        index >>= 1;
    }
    return reversed;
}

/**
 * @brief FFT分析 - Cooley-Tukey radix-2实现
 * @param hscope: 示波器句柄
 * @param channel: 通道号
 * @param magnitude: 幅度谱输出缓冲区
 * @param size: FFT点数(必须是2的幂)
 * @return HAL状态
 */
HAL_StatusTypeDef SCO_FFT(SCO_HandleTypeDef* hscope, SCO_Channel_t channel, float* magnitude, uint32_t size)
{
    if (hscope == NULL || magnitude == NULL || size == 0) {
        return HAL_ERROR;
    }
    if (channel >= SCO_MAX_CHANNELS) {
        return HAL_ERROR;
    }
    if (hscope->samples_captured == 0) {
        return HAL_ERROR;
    }
    
    /* 检查size是否为2的幂 */
    if ((size & (size - 1)) != 0) {
        return HAL_ERROR;  /* size必须是2的幂 */
    }
    
    /* 计算log2(size) */
    uint32_t log2_size = 0;
    uint32_t temp = size;
    while (temp > 1) {
        temp >>= 1;
        log2_size++;
    }
    
    /* 分配临时缓冲区(实部和虚部) */
    float* real = (float*)malloc(size * sizeof(float));
    float* imag = (float*)malloc(size * sizeof(float));
    
    if (real == NULL || imag == NULL) {
        if (real) free(real);
        if (imag) free(imag);
        return HAL_ERROR;
    }
    
    /* 初始化数据：复制采样数据到实部，虚部初始化为0 */
    uint32_t copy_count = (size < hscope->samples_captured) ? size : hscope->samples_captured;
    for (uint32_t i = 0; i < copy_count; i++) {
        real[i] = (float)SCO_ADCToVoltage(
            hscope->buffers[channel][i].value,
            &hscope->config.channels[channel]);
        imag[i] = 0.0f;
    }
    /* 剩余部分补零 */
    for (uint32_t i = copy_count; i < size; i++) {
        real[i] = 0.0f;
        imag[i] = 0.0f;
    }
    
    /* 位反转置换 */
    for (uint32_t i = 0; i < size; i++) {
        uint32_t j = SCO_BitReverse(i, log2_size);
        if (i < j) {
            /* 交换实部 */
            float temp_real = real[i];
            real[i] = real[j];
            real[j] = temp_real;
            /* 交换虚部 */
            float temp_imag = imag[i];
            imag[i] = imag[j];
            imag[j] = temp_imag;
        }
    }
    
    /* Cooley-Tukey radix-2 FFT 蝶形运算 */
    const float PI = 3.14159265358979323846f;
    
    for (uint32_t stage = 1; stage <= log2_size; stage++) {
        uint32_t m = 1 << stage;           /* 当前阶段的蝶形大小 */
        uint32_t m2 = m >> 1;              /* 蝶形的一半 */
        
        /* 计算旋转因子 */
        float wm_real = cosf(PI / (float)m2);
        float wm_imag = -sinf(PI / (float)m2);
        
        for (uint32_t k = 0; k < size; k += m) {
            float w_real = 1.0f;
            float w_imag = 0.0f;
            
            for (uint32_t j = 0; j < m2; j++) {
                uint32_t idx1 = k + j;
                uint32_t idx2 = k + j + m2;
                
                /* 蝶形计算 */
                float t_real = w_real * real[idx2] - w_imag * imag[idx2];
                float t_imag = w_real * imag[idx2] + w_imag * real[idx2];
                
                real[idx2] = real[idx1] - t_real;
                imag[idx2] = imag[idx1] - t_imag;
                real[idx1] = real[idx1] + t_real;
                imag[idx1] = imag[idx1] + t_imag;
                
                /* 更新旋转因子 */
                float w_temp_real = w_real * wm_real - w_imag * wm_imag;
                float w_temp_imag = w_real * wm_imag + w_imag * wm_real;
                w_real = w_temp_real;
                w_imag = w_temp_imag;
            }
        }
    }
    
    /* 计算幅度谱(只取前半部分，因为是对称的) */
    for (uint32_t i = 0; i < size / 2; i++) {
        magnitude[i] = sqrtf(real[i] * real[i] + imag[i] * imag[i]) / (float)size;
    }
    
    /* 释放临时缓冲区 */
    free(real);
    free(imag);
    
    return HAL_OK;
}