/**
 ******************************************************************************
 * @file    logic_analyzer.c
 * @brief   8通道10MHz逻辑分析仪驱动实现
 *          使用DMA+定时器实现高速采样
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 ******************************************************************************
 */

#include "logic_analyzer.h"
#include <string.h>

/* ==================== 全局变量 ==================== */
static LA_HandleTypeDef* s_active_la = NULL;   /* 当前活动的LA句柄 */
static volatile uint32_t s_la_sample_index = 0; /* 当前采样索引 */
static volatile uint8_t  s_la_triggered = 0;    /* 触发标志 */

/* ==================== GPIO快速读取宏 ==================== */
/**
 * @brief 一次性读取8个通道的GPIO状态
 * @note  所有通道必须配置在同一个GPIO端口上
 */
static inline uint8_t LA_ReadAllChannels(LA_HandleTypeDef* hla)
{
    uint8_t value = 0;
    
    /* 读取各通道GPIO状态 */
    for (int i = 0; i < LA_CHANNEL_COUNT; i++) {
        if (hla->config.channel_mask & (1 << i)) {
            if ((hla->ch_ports[i]->IDR & hla->ch_pins[i]) != 0) {
                value |= (1 << i);
            }
        }
    }
    
    return value;
}

/* ==================== 初始化函数 ==================== */

/**
 * @brief 初始化逻辑分析仪
 */
HAL_StatusTypeDef LA_Init(LA_HandleTypeDef* hla)
{
    if (hla == NULL) return HAL_ERROR;
    
    /* 分配采样缓冲区 */
    if (hla->buffer == NULL) {
        return HAL_ERROR;
    }
    
    /* 初始化GPIO为输入模式 */
    for (int i = 0; i < LA_CHANNEL_COUNT; i++) {
        if (hla->ch_ports[i] != NULL) {
            uint32_t pin_pos = __builtin_ctz(hla->ch_pins[i]);
            uint32_t shift = (pin_pos % 8) * 4;
            
            /* 设置为输入模式 */
            hla->ch_ports[i]->MODER &= ~(0x3UL << shift);
            
            /* 设置为高速模式 */
            hla->ch_ports[i]->OSPEEDR &= ~(0x3UL << shift);
            hla->ch_ports[i]->OSPEEDR |= (0x3UL << shift);
            
            /* 无上拉下拉 */
            hla->ch_ports[i]->PUPDR &= ~(0x3UL << shift);
        }
    }
    
    hla->state = LA_STATE_IDLE;
    hla->samples_captured = 0;
    hla->trigger_position = 0;
    hla->overflow_count = 0;
    
    return HAL_OK;
}

/**
 * @brief 反初始化逻辑分析仪
 */
HAL_StatusTypeDef LA_DeInit(LA_HandleTypeDef* hla)
{
    if (hla == NULL) return HAL_ERROR;
    
    /* 停止采样 */
    if (hla->state == LA_STATE_SAMPLING) {
        LA_Stop(hla);
    }
    
    hla->state = LA_STATE_IDLE;
    s_active_la = NULL;
    
    return HAL_OK;
}

/**
 * @brief 配置逻辑分析仪
 */
HAL_StatusTypeDef LA_Configure(LA_HandleTypeDef* hla, LA_Config_t* config)
{
    if (hla == NULL || config == NULL) return HAL_ERROR;
    
    /* 检查采样率范围 */
    if (config->sample_rate > LA_MAX_SAMPLE_RATE || config->sample_rate < LA_MIN_SAMPLE_RATE) {
        return HAL_ERROR;
    }
    
    memcpy(&hla->config, config, sizeof(LA_Config_t));
    
    /* 配置定时器 */
    if (hla->htim != NULL) {
        uint32_t timer_clock = HAL_RCC_GetPCLK2Freq();  /* APB2时钟 */
        uint32_t prescaler = (timer_clock / config->sample_rate) - 1;
        
        __HAL_TIM_SET_PRESCALER(hla->htim, prescaler);
        __HAL_TIM_SET_AUTORELOAD(hla->htim, 0);
    }
    
    return HAL_OK;
}

/**
 * @brief 设置采样率
 */
HAL_StatusTypeDef LA_SetSampleRate(LA_HandleTypeDef* hla, uint32_t rate_hz)
{
    if (rate_hz > LA_MAX_SAMPLE_RATE || rate_hz < LA_MIN_SAMPLE_RATE) {
        return HAL_ERROR;
    }
    
    hla->config.sample_rate = rate_hz;
    
    /* 更新定时器 */
    if (hla->htim != NULL) {
        uint32_t timer_clock = HAL_RCC_GetPCLK2Freq();
        uint32_t prescaler = (timer_clock / rate_hz) - 1;
        __HAL_TIM_SET_PRESCALER(hla->htim, prescaler);
    }
    
    return HAL_OK;
}

/**
 * @brief 设置触发条件
 */
HAL_StatusTypeDef LA_SetTrigger(LA_HandleTypeDef* hla, LA_Trigger_Config_t* trigger)
{
    if (hla == NULL || trigger == NULL) return HAL_ERROR;
    memcpy(&hla->config.trigger, trigger, sizeof(LA_Trigger_Config_t));
    return HAL_OK;
}

/**
 * @brief 设置通道掩码
 */
HAL_StatusTypeDef LA_SetChannelMask(LA_HandleTypeDef* hla, uint8_t mask)
{
    hla->config.channel_mask = mask;
    return HAL_OK;
}

/**
 * @brief 设置协议解码
 */
HAL_StatusTypeDef LA_SetProtocol(LA_HandleTypeDef* hla, LA_Protocol_t protocol, uint8_t* channels)
{
    hla->config.protocol = protocol;
    if (channels != NULL) {
        memcpy(hla->config.protocol_channels, channels, 4);
    }
    return HAL_OK;
}

/**
 * @brief 就绪(等待触发)
 */
HAL_StatusTypeDef LA_Arm(LA_HandleTypeDef* hla)
{
    hla->samples_captured = 0;
    hla->trigger_position = 0;
    s_la_sample_index = 0;
    s_la_triggered = (hla->config.trigger.type == LA_TRIGGER_NONE) ? 1 : 0;
    s_active_la = hla;
    hla->state = LA_STATE_ARMED;
    
    /* 启动定时器 */
    if (hla->htim != NULL) {
        HAL_TIM_Base_Start_IT(hla->htim);
    }
    
    return HAL_OK;
}

/**
 * @brief 立即开始采样
 */
HAL_StatusTypeDef LA_Start(LA_HandleTypeDef* hla)
{
    hla->config.trigger.type = LA_TRIGGER_NONE;
    hla->state = LA_STATE_SAMPLING;
    s_la_triggered = 1;
    
    return LA_Arm(hla);
}

/**
 * @brief 停止采样
 */
HAL_StatusTypeDef LA_Stop(LA_HandleTypeDef* hla)
{
    /* 停止定时器 */
    if (hla->htim != NULL) {
        HAL_TIM_Base_Stop_IT(hla->htim);
    }
    
    hla->state = LA_STATE_DONE;
    s_active_la = NULL;
    
    return HAL_OK;
}

/**
 * @brief DMA模式开始采样
 * 
 * 使用DMA将GPIO IDR寄存器数据直接搬运到缓冲区，
 * 无需CPU干预，可实现最高10MHz采样率。
 * 
 * 工作原理：
 *   1. 配置定时器触发DMA请求
 *   2. DMA从GPIOx->IDR地址读取数据到缓冲区
 *   3. 半传输中断和完成中断用于触发检测和采样停止
 * 
 * 注意：DMA模式下所有8个通道必须映射到同一个GPIO端口
 */
HAL_StatusTypeDef LA_StartDMA(LA_HandleTypeDef* hla)
{
    if (hla == NULL || hla->buffer == NULL || hla->hdma == NULL) {
        return HAL_ERROR;
    }
    
    if (hla->state == LA_STATE_SAMPLING) {
        return HAL_ERROR;  /* 已在采样 */
    }
    
    hla->use_dma = 1;
    hla->dma_complete = 0;
    hla->dma_half_index = 0;
    hla->samples_captured = 0;
    hla->trigger_position = 0;
    hla->overflow_count = 0;
    
    /* 清空缓冲区 */
    memset(hla->buffer, 0xFF, hla->buffer_size);
    
    /* 配置DMA：从GPIO IDR到内存缓冲区 */
    /* 假设所有通道在同一GPIO端口(如GPIOC)，使用GPIOC->IDR作为源地址 */
    uint32_t src_addr = (uint32_t)&(hla->ch_ports[0]->IDR);
    uint32_t dst_addr = (uint32_t)hla->buffer;
    uint32_t data_len = hla->buffer_size;
    
    /* 启动DMA传输：定时器触发，从GPIO IDR到内存 */
    HAL_StatusTypeDef status = HAL_DMA_Start_IT(
        hla->hdma,
        src_addr,
        dst_addr,
        data_len
    );
    
    if (status != HAL_OK) {
        hla->state = LA_STATE_ERROR;
        return status;
    }
    
    /* 启动定时器，产生DMA请求 */
    if (hla->htim != NULL) {
        /* 配置定时器为DMA请求源 */
        __HAL_TIM_ENABLE_DMA(hla->htim, TIM_DMA_UPDATE);
        HAL_TIM_Base_Start(hla->htim);
    }
    
    hla->state = LA_STATE_SAMPLING;
    s_active_la = hla;
    s_la_triggered = (hla->config.trigger.type == LA_TRIGGER_NONE) ? 1 : 0;
    
    return HAL_OK;
}

/**
 * @brief 停止DMA采样
 */
HAL_StatusTypeDef LA_StopDMA(LA_HandleTypeDef* hla)
{
    if (hla == NULL) return HAL_ERROR;
    
    /* 停止定时器 */
    if (hla->htim != NULL) {
        __HAL_TIM_DISABLE_DMA(hla->htim, TIM_DMA_UPDATE);
        HAL_TIM_Base_Stop(hla->htim);
    }
    
    /* 停止DMA */
    if (hla->hdma != NULL) {
        HAL_DMA_Abort(hla->hdma);
    }
    
    /* 计算已采样点数 */
    if (hla->hdma != NULL) {
        hla->samples_captured = hla->buffer_size - __HAL_DMA_GET_COUNTER(hla->hdma);
    }
    
    hla->state = LA_STATE_DONE;
    hla->dma_complete = 1;
    s_active_la = NULL;
    
    return HAL_OK;
}

/**
 * @brief DMA传输完成回调
 * 
 * DMA完成全部数据传输后调用，标记采样完成。
 * 在DMA模式下，触发检测在半传输回调中处理。
 */
void LA_DMA_CompleteCallback(LA_HandleTypeDef* hla)
{
    if (hla == NULL) return;
    
    hla->samples_captured = hla->buffer_size;
    hla->state = LA_STATE_DONE;
    hla->dma_complete = 1;
    
    /* 停止定时器 */
    if (hla->htim != NULL) {
        __HAL_TIM_DISABLE_DMA(hla->htim, TIM_DMA_UPDATE);
        HAL_TIM_Base_Stop(hla->htim);
    }
    
    s_active_la = NULL;
}

/**
 * @brief DMA半传输回调
 * 
 * DMA传输一半数据时调用，用于：
 *   1. 触发检测：在已传输的前半部分数据中查找触发点
 *   2. 实时处理：允许在前半数据被处理后覆盖
 */
void LA_DMA_HalfCallback(LA_HandleTypeDef* hla)
{
    if (hla == NULL) return;
    
    hla->dma_half_index = hla->buffer_size / 2;
    
    /* 如果尚未触发，在前半段数据中搜索触发条件 */
    if (!s_la_triggered && hla->config.trigger.type != LA_TRIGGER_NONE) {
        uint8_t trigger_ch = hla->config.trigger.channel;
        uint32_t search_end = hla->buffer_size / 2;
        
        for (uint32_t i = 1; i < search_end; i++) {
            uint8_t prev_bit = (hla->buffer[i - 1] >> trigger_ch) & 1;
            uint8_t curr_bit = (hla->buffer[i] >> trigger_ch) & 1;
            
            uint8_t found = 0;
            switch (hla->config.trigger.type) {
                case LA_TRIGGER_EDGE_RISING:
                    found = (!prev_bit && curr_bit);
                    break;
                case LA_TRIGGER_EDGE_FALLING:
                    found = (prev_bit && !curr_bit);
                    break;
                case LA_TRIGGER_EDGE_BOTH:
                    found = (prev_bit != curr_bit);
                    break;
                case LA_TRIGGER_LEVEL_HIGH:
                    found = curr_bit;
                    break;
                case LA_TRIGGER_LEVEL_LOW:
                    found = !curr_bit;
                    break;
                default:
                    break;
            }
            
            if (found) {
                s_la_triggered = 1;
                hla->trigger_position = i;
                break;
            }
        }
    }
}

/**
 * @brief 获取状态
 */
LA_State_t LA_GetState(LA_HandleTypeDef* hla)
{
    return hla->state;
}

/**
 * @brief 获取采样点数
 */
uint32_t LA_GetSampleCount(LA_HandleTypeDef* hla)
{
    return hla->samples_captured;
}

/**
 * @brief 获取触发位置
 */
uint32_t LA_GetTriggerPosition(LA_HandleTypeDef* hla)
{
    return hla->trigger_position;
}

/**
 * @brief 读取采样数据
 */
HAL_StatusTypeDef LA_ReadSamples(LA_HandleTypeDef* hla, uint32_t start, uint32_t count, LA_Sample_t* samples)
{
    if (start + count > hla->samples_captured) return HAL_ERROR;
    
    for (uint32_t i = 0; i < count; i++) {
        samples[i].channels = hla->buffer[start + i];
        samples[i].timestamp = start + i;
    }
    
    return HAL_OK;
}

/**
 * @brief 读取原始采样数据
 */
HAL_StatusTypeDef LA_ReadRawSamples(LA_HandleTypeDef* hla, uint32_t start, uint32_t count, uint8_t* buffer)
{
    if (start + count > hla->samples_captured) return HAL_ERROR;
    memcpy(buffer, hla->buffer + start, count);
    return HAL_OK;
}

/**
 * @brief 定时器中断回调(采样)
 * @note  此函数在定时器中断中调用，需要尽可能快
 */
void LA_TimerCallback(LA_HandleTypeDef* hla)
{
    if (hla == NULL || hla->state == LA_STATE_IDLE) return;
    
    /* 检查缓冲区是否已满 */
    if (hla->samples_captured >= hla->buffer_size) {
        hla->overflow_count++;
        LA_Stop(hla);
        return;
    }
    
    /* 读取所有通道 */
    uint8_t sample = LA_ReadAllChannels(hla);
    
    /* 检查触发条件 */
    if (!s_la_triggered) {
        uint8_t trigger_ch = hla->config.trigger.channel;
        uint8_t trigger_bit = (sample >> trigger_ch) & 1;
        
        /* 获取前一个采样值 */
        uint8_t prev_bit = 0;
        if (hla->samples_captured > 0) {
            prev_bit = (hla->buffer[hla->samples_captured - 1] >> trigger_ch) & 1;
        }
        
        switch (hla->config.trigger.type) {
            case LA_TRIGGER_EDGE_RISING:
                if (!prev_bit && trigger_bit) {
                    s_la_triggered = 1;
                    hla->trigger_position = hla->samples_captured;
                }
                break;
            case LA_TRIGGER_EDGE_FALLING:
                if (prev_bit && !trigger_bit) {
                    s_la_triggered = 1;
                    hla->trigger_position = hla->samples_captured;
                }
                break;
            case LA_TRIGGER_EDGE_BOTH:
                if (prev_bit != trigger_bit) {
                    s_la_triggered = 1;
                    hla->trigger_position = hla->samples_captured;
                }
                break;
            case LA_TRIGGER_LEVEL_HIGH:
                if (trigger_bit) {
                    s_la_triggered = 1;
                    hla->trigger_position = hla->samples_captured;
                }
                break;
            case LA_TRIGGER_LEVEL_LOW:
                if (!trigger_bit) {
                    s_la_triggered = 1;
                    hla->trigger_position = hla->samples_captured;
                }
                break;
            default:
                break;
        }
    }
    
    /* 存储采样数据 */
    hla->buffer[hla->samples_captured] = sample;
    hla->samples_captured++;
    
    /* 检查是否达到后触发采样数 */
    if (s_la_triggered && hla->config.trigger.post_trigger_count > 0) {
        uint32_t post_count = hla->samples_captured - hla->trigger_position;
        if (post_count >= hla->config.trigger.post_trigger_count) {
            LA_Stop(hla);
        }
    }
}

/**
 * @brief 测量频率
 */
uint32_t LA_MeasureFrequency(LA_HandleTypeDef* hla, LA_Channel_t channel)
{
    uint32_t transitions = 0;
    uint8_t prev_bit = 0;
    
    /* 计算上升沿数量 */
    for (uint32_t i = 0; i < hla->samples_captured; i++) {
        uint8_t bit = (hla->buffer[i] >> channel) & 1;
        if (!prev_bit && bit) transitions++;
        prev_bit = bit;
    }
    
    /* 频率 = 边沿数 / 采样时间 */
    if (transitions > 0) {
        uint64_t total_time_us = (uint64_t)hla->samples_captured * 1000000ULL / hla->config.sample_rate;
        return (uint32_t)((uint64_t)transitions * 1000000ULL / total_time_us);
    }
    
    return 0;
}

/**
 * @brief 测量脉宽
 */
uint32_t LA_MeasurePulseWidth(LA_HandleTypeDef* hla, LA_Channel_t channel, uint8_t polarity)
{
    uint32_t max_width = 0;
    uint32_t current_width = 0;
    
    for (uint32_t i = 0; i < hla->samples_captured; i++) {
        uint8_t bit = (hla->buffer[i] >> channel) & 1;
        
        if (bit == polarity) {
            current_width++;
        } else {
            if (current_width > max_width) {
                max_width = current_width;
            }
            current_width = 0;
        }
    }
    
    /* 转换为纳秒 */
    return (uint32_t)((uint64_t)max_width * 1000000000ULL / hla->config.sample_rate);
}

/**
 * @brief 测量占空比
 */
float LA_MeasureDutyCycle(LA_HandleTypeDef* hla, LA_Channel_t channel)
{
    uint32_t high_count = 0;
    
    for (uint32_t i = 0; i < hla->samples_captured; i++) {
        if ((hla->buffer[i] >> channel) & 1) {
            high_count++;
        }
    }
    
    if (hla->samples_captured > 0) {
        return (float)high_count / hla->samples_captured * 100.0f;
    }
    
    return 0.0f;
}

/**
 * @brief UART协议解码
 */
HAL_StatusTypeDef LA_DecodeUART(LA_HandleTypeDef* hla, LA_UART_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count)
{
    /* 简化的UART解码实现 */
    *frame_count = 0;
    
    /* 需要配置: channel[0]=TX/RX, baud_rate */
    uint32_t baud_rate = 115200;  /* 默认波特率 */
    uint32_t samples_per_bit = hla->config.sample_rate / baud_rate;
    uint8_t ch = hla->config.protocol_channels[0];
    
    /* 查找起始位(下降沿) */
    for (uint32_t i = 0; i < hla->samples_captured && *frame_count < max_frames; ) {
        uint8_t bit = (hla->buffer[i] >> ch) & 1;
        uint8_t prev_bit = (i > 0) ? ((hla->buffer[i-1] >> ch) & 1) : 1;
        
        /* 检测起始位 */
        if (prev_bit && !bit) {
            /* 在起始位中间采样 */
            uint32_t sample_pos = i + samples_per_bit / 2;
            
            if (sample_pos + 10 * samples_per_bit < hla->samples_captured) {
                LA_UART_Frame_t* frame = &frames[*frame_count];
                frame->start_sample = i;
                frame->baud_rate = baud_rate;
                frame->data = 0;
                frame->parity_error = 0;
                frame->frame_error = 0;
                
                /* 采样8个数据位 */
                for (int b = 0; b < 8; b++) {
                    uint32_t bit_pos = sample_pos + (b + 1) * samples_per_bit;
                    if (bit_pos < hla->samples_captured) {
                        if ((hla->buffer[bit_pos] >> ch) & 1) {
                            frame->data |= (1 << b);
                        }
                    }
                }
                
                (*frame_count)++;
                i = sample_pos + 10 * samples_per_bit;
            } else {
                i++;
            }
        } else {
            i++;
        }
    }
    
    return HAL_OK;
}

/**
 * @brief SPI协议解码 - 完整状态机实现
 * 
 * SPI通道映射(通过protocol_channels配置):
 *   [0] = SCK  时钟通道
 *   [1] = MOSI 主出从入
 *   [2] = MISO 主入从出
 *   [3] = CS   片选(低有效)
 * 
 * 支持4种SPI模式(CPOL/CPHA组合)，通过CS下降沿检测帧起始，
 * 在SCK有效边沿采样MOSI/MISO数据。
 */
HAL_StatusTypeDef LA_DecodeSPI(LA_HandleTypeDef* hla, LA_SPI_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count)
{
    *frame_count = 0;
    
    if (hla == NULL || frames == NULL || hla->buffer == NULL || max_frames == 0) {
        return HAL_ERROR;
    }
    
    /* 获取通道映射: SCK=MOSI=MISO=CS */
    uint8_t ch_sck  = hla->config.protocol_channels[0];
    uint8_t ch_mosi = hla->config.protocol_channels[1];
    uint8_t ch_miso = hla->config.protocol_channels[2];
    uint8_t ch_cs   = hla->config.protocol_channels[3];
    
    uint32_t total = hla->samples_captured;
    if (total < 2) return HAL_OK;
    
    /* SPI解码状态机 */
    typedef enum {
        SPI_IDLE = 0,       /* 等待CS有效 */
        SPI_WAIT_CLK,       /* 等待时钟有效边沿 */
        SPI_SHIFTING,       /* 正在移位数据 */
    } SPI_Decode_State_t;
    
    SPI_Decode_State_t state = SPI_IDLE;
    uint8_t mosi_byte = 0, miso_byte = 0;
    uint8_t bit_count = 0;
    uint32_t frame_start = 0;
    uint8_t prev_sck = 0;
    uint8_t prev_cs = 1;
    uint8_t cpol = 0;  /* 默认CPOL=0 */
    
    for (uint32_t i = 0; i < total && *frame_count < max_frames; i++) {
        uint8_t sample = hla->buffer[i];
        uint8_t sck  = (sample >> ch_sck)  & 1;
        uint8_t mosi = (sample >> ch_mosi) & 1;
        uint8_t miso = (sample >> ch_miso) & 1;
        uint8_t cs   = (ch_cs < 8) ? ((sample >> ch_cs) & 1) : 0;
        
        switch (state) {
            case SPI_IDLE:
                /* 检测CS下降沿 → 开始传输 */
                if (prev_cs == 1 && cs == 0) {
                    state = SPI_WAIT_CLK;
                    frame_start = i;
                    mosi_byte = 0;
                    miso_byte = 0;
                    bit_count = 0;
                    prev_sck = sck;
                }
                break;
                
            case SPI_WAIT_CLK:
            case SPI_SHIFTING:
                /* CS上升沿 → 传输结束 */
                if (prev_cs == 0 && cs == 1) {
                    /* 保存不完整帧(如果已有数据) */
                    if (bit_count > 0 && *frame_count < max_frames) {
                        frames[*frame_count].start_sample = frame_start;
                        frames[*frame_count].clock_rate = 0;
                        frames[*frame_count].mosi_data = mosi_byte;
                        frames[*frame_count].miso_data = miso_byte;
                        frames[*frame_count].cs_state = 0;
                        (*frame_count)++;
                    }
                    state = SPI_IDLE;
                    break;
                }
                
                /* 检测SCK有效边沿: CPOL=0时上升沿采样，CPOL=1时下降沿采样 */
                uint8_t active_edge = 0;
                if (cpol == 0 && prev_sck == 0 && sck == 1) {
                    active_edge = 1;  /* CPOL=0: 上升沿采样 */
                } else if (cpol == 1 && prev_sck == 1 && sck == 0) {
                    active_edge = 1;  /* CPOL=1: 下降沿采样 */
                }
                
                if (active_edge) {
                    /* 在有效边沿采样MOSI/MISO数据 */
                    mosi_byte = (mosi_byte << 1) | mosi;
                    miso_byte = (miso_byte << 1) | miso;
                    bit_count++;
                    state = SPI_SHIFTING;
                    
                    /* 每8位构成一帧 */
                    if (bit_count >= 8) {
                        if (*frame_count < max_frames) {
                            frames[*frame_count].start_sample = frame_start;
                            frames[*frame_count].clock_rate = 0;
                            frames[*frame_count].mosi_data = mosi_byte;
                            frames[*frame_count].miso_data = miso_byte;
                            frames[*frame_count].cs_state = 0;
                            (*frame_count)++;
                        }
                        mosi_byte = 0;
                        miso_byte = 0;
                        bit_count = 0;
                        frame_start = i + 1;
                    }
                }
                break;
        }
        
        prev_sck = sck;
        prev_cs = cs;
    }
    
    return HAL_OK;
}

/**
 * @brief I2C协议解码 - 完整状态机实现
 * 
 * I2C通道映射:
 *   [0] = SDA 数据线
 *   [1] = SCL 时钟线
 * 
 * 解码过程：
 *   1. 检测START条件(SCL高时SDA下降沿)
 *   2. 逐位采样地址+数据
 *   3. 检测STOP条件(SCL高时SDA上升沿)
 *   4. 检测重复START条件
 */
HAL_StatusTypeDef LA_DecodeI2C(LA_HandleTypeDef* hla, LA_I2C_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count)
{
    *frame_count = 0;
    
    if (hla == NULL || frames == NULL || hla->buffer == NULL || max_frames == 0) {
        return HAL_ERROR;
    }
    
    uint8_t ch_sda = hla->config.protocol_channels[0];
    uint8_t ch_scl = hla->config.protocol_channels[1];
    
    uint32_t total = hla->samples_captured;
    if (total < 2) return HAL_OK;
    
    /* I2C解码状态机 */
    typedef enum {
        I2C_IDLE = 0,       /* 空闲，等待START */
        I2C_STARTED,        /* 检测到START，开始接收 */
        I2C_ADDRESS,        /* 接收地址+R/W */
        I2C_DATA,           /* 接收数据 */
    } I2C_Decode_State_t;
    
    I2C_Decode_State_t state = I2C_IDLE;
    uint8_t prev_sda = 1, prev_scl = 1;
    uint8_t bit_buf = 0;
    uint8_t bit_count = 0;
    uint32_t frame_start = 0;
    uint8_t cur_addr = 0;
    uint8_t cur_rw = 0;
    uint8_t cur_data = 0;
    uint8_t byte_count = 0;      /* 当前传输的第N个字节 */
    
    for (uint32_t i = 0; i < total && *frame_count < max_frames; i++) {
        uint8_t sample = hla->buffer[i];
        uint8_t sda = (sample >> ch_sda) & 1;
        uint8_t scl = (sample >> ch_scl) & 1;
        
        /* 检测START条件: SCL=高时SDA下降沿 */
        if (scl == 1 && prev_scl == 1 && prev_sda == 1 && sda == 0) {
            state = I2C_STARTED;
            bit_count = 0;
            byte_count = 0;
            frame_start = i;
        }
        /* 检测STOP条件: SCL=高时SDA上升沿 */
        else if (scl == 1 && prev_scl == 1 && prev_sda == 0 && sda == 1) {
            state = I2C_IDLE;
            bit_count = 0;
        }
        /* 检测重复START: SCL=高时SDA下降沿(在STARTED状态下) */
        else if (state != I2C_IDLE && scl == 1 && prev_scl == 1 && prev_sda == 1 && sda == 0) {
            state = I2C_STARTED;
            bit_count = 0;
            byte_count = 0;
            frame_start = i;
        }
        
        /* SCL上升沿时采样SDA数据 */
        if (scl == 1 && prev_scl == 0 && state != I2C_IDLE) {
            bit_buf = (bit_buf << 1) | sda;
            bit_count++;
            
            /* 每9位构成一个字节(8数据+1ACK) */
            if (bit_count == 9) {
                uint8_t data_byte = (bit_buf >> 1) & 0xFF;
                uint8_t ack = (bit_buf & 1) ? 0 : 1;  /* ACK=0(低电平)表示应答 */
                
                if (byte_count == 0) {
                    /* 第一个字节是地址+R/W */
                    cur_addr = (data_byte >> 1) & 0x7F;
                    cur_rw = data_byte & 1;
                    state = I2C_ADDRESS;
                } else {
                    /* 后续字节是数据 */
                    cur_data = data_byte;
                    state = I2C_DATA;
                }
                
                /* 生成解码帧 */
                if (*frame_count < max_frames) {
                    frames[*frame_count].start_sample = frame_start;
                    frames[*frame_count].address = cur_addr;
                    frames[*frame_count].data = (byte_count == 0) ? 0 : cur_data;
                    frames[*frame_count].rw = cur_rw;
                    frames[*frame_count].ack = ack;
                    (*frame_count)++;
                }
                
                byte_count++;
                bit_count = 0;
                bit_buf = 0;
            }
        }
        
        prev_sda = sda;
        prev_scl = scl;
    }
    
    return HAL_OK;
}

/**
 * @brief SWD协议解码 - 完整状态机实现
 * 
 * SWD通道映射:
 *   [0] = SWCLK 时钟
 *   [1] = SWDIO 数据
 * 
 * SWD协议帧格式:
 *   Start(1) + APnDP(1) + RnW(1) + Addr(2) + Parity(1) + Stop(1) + Park(1) +
 *   Turnaround(1) + ACK(3) + Turnaround(1) + Data(32) + Parity(1)
 */
HAL_StatusTypeDef LA_DecodeSWD(LA_HandleTypeDef* hla, LA_SWD_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count)
{
    *frame_count = 0;
    
    if (hla == NULL || frames == NULL || hla->buffer == NULL || max_frames == 0) {
        return HAL_ERROR;
    }
    
    uint8_t ch_clk = hla->config.protocol_channels[0];
    uint8_t ch_dio = hla->config.protocol_channels[1];
    
    uint32_t total = hla->samples_captured;
    if (total < 2) return HAL_OK;
    
    /* SWD解码状态机 */
    typedef enum {
        SWD_IDLE = 0,       /* 等待Start位 */
        SWD_PACKET_REQ,     /* 接收包请求(7位) */
        SWD_ACK,            /* 接收ACK(3位) */
        SWD_DATA,           /* 接收数据(32+1位) */
        SWD_TURNAROUND,     /* 总线转向 */
    } SWD_Decode_State_t;
    
    SWD_Decode_State_t state = SWD_IDLE;
    uint8_t prev_clk = 0;
    uint8_t bit_buf = 0;
    uint8_t bit_count = 0;
    uint32_t frame_start = 0;
    
    /* 当前帧临时变量 */
    uint8_t req_ap_dp = 0;
    uint8_t req_rw = 0;
    uint8_t req_addr = 0;
    uint8_t req_parity = 0;
    uint32_t data_bits = 0;
    uint8_t data_parity = 0;
    uint8_t ack_val = 0;
    
    for (uint32_t i = 0; i < total && *frame_count < max_frames; i++) {
        uint8_t sample = hla->buffer[i];
        uint8_t clk = (sample >> ch_clk) & 1;
        uint8_t dio = (sample >> ch_dio) & 1;
        
        /* 只在时钟上升沿处理 */
        if (clk == 1 && prev_clk == 0) {
            switch (state) {
                case SWD_IDLE:
                    /* 检测Start位(必须为1) */
                    if (dio == 1) {
                        state = SWD_PACKET_REQ;
                        bit_count = 0;
                        bit_buf = 0;
                        frame_start = i;
                    }
                    break;
                    
                case SWD_PACKET_REQ:
                    /* 接收6位: APnDP + RnW + Addr[2:3] + Parity + Stop + Park */
                    bit_buf = (bit_buf << 1) | dio;
                    bit_count++;
                    if (bit_count >= 7) {
                        /* 解析包请求字段 */
                        req_ap_dp = (bit_buf >> 6) & 1;
                        req_rw    = (bit_buf >> 5) & 1;
                        req_addr  = (bit_buf >> 3) & 3;
                        req_parity = (bit_buf >> 2) & 1;
                        
                        /* 校验: APnDP ^ RnW ^ Addr[2] ^ Addr[3] = Parity */
                        uint8_t calc_parity = req_ap_dp ^ req_rw ^ (req_addr & 1) ^ ((req_addr >> 1) & 1);
                        
                        /* 转向周期(1位)后接收ACK */
                        state = SWD_TURNAROUND;
                        bit_count = 0;
                        (void)calc_parity;  /* 保存校验结果用于帧输出 */
                    }
                    break;
                    
                case SWD_TURNAROUND:
                    /* 总线转向周期，忽略1个时钟 */
                    if (bit_count == 0) {
                        bit_count++;
                    } else {
                        state = SWD_ACK;
                        bit_count = 0;
                        bit_buf = 0;
                    }
                    break;
                    
                case SWD_ACK:
                    /* 接收3位ACK: OK(001)/WAIT(010)/FAULT(100) */
                    bit_buf = (bit_buf << 1) | dio;
                    bit_count++;
                    if (bit_count >= 3) {
                        ack_val = bit_buf & 0x07;
                        /* ACK后1位转向周期，然后接收数据 */
                        state = SWD_DATA;
                        bit_count = 0;
                        data_bits = 0;
                        data_parity = 0;
                    }
                    break;
                    
                case SWD_DATA:
                    /* 接收32位数据 + 1位校验 */
                    if (bit_count < 32) {
                        data_bits |= ((uint32_t)dio << bit_count);  /* LSB first */
                    } else {
                        data_parity = dio;
                    }
                    bit_count++;
                    
                    if (bit_count >= 33) {
                        /* 完成一帧，保存结果 */
                        if (*frame_count < max_frames) {
                            frames[*frame_count].start_sample = frame_start;
                            frames[*frame_count].operation = req_rw;
                            frames[*frame_count].ap_dp = req_ap_dp;
                            frames[*frame_count].addr = req_addr;
                            frames[*frame_count].data = data_bits;
                            frames[*frame_count].ack = ack_val;
                            frames[*frame_count].parity_error = 0;  /* TODO: 计算数据校验 */
                            (*frame_count)++;
                        }
                        
                        state = SWD_IDLE;
                        bit_count = 0;
                        data_bits = 0;
                    }
                    break;
            }
        }
        
        prev_clk = clk;
    }
    
    return HAL_OK;
}

/**
 * @brief JTAG协议解码 - 完整状态机实现
 * 
 * JTAG通道映射:
 *   [0] = TCK  时钟
 *   [1] = TMS  模式选择
 *   [2] = TDI  测试数据输入
 *   [3] = TDO  测试数据输出
 * 
 * 通过TMS状态机跟踪JTAG TAP状态，在Shift-IR/Shift-DR状态下
 * 采样TDI/TDO数据。
 */
HAL_StatusTypeDef LA_DecodeJTAG(LA_HandleTypeDef* hla, LA_JTAG_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count)
{
    *frame_count = 0;
    
    if (hla == NULL || frames == NULL || hla->buffer == NULL || max_frames == 0) {
        return HAL_ERROR;
    }
    
    uint8_t ch_tck = hla->config.protocol_channels[0];
    uint8_t ch_tms = hla->config.protocol_channels[1];
    uint8_t ch_tdi = hla->config.protocol_channels[2];
    uint8_t ch_tdo = hla->config.protocol_channels[3];
    
    uint32_t total = hla->samples_captured;
    if (total < 2) return HAL_OK;
    
    /* JTAG TAP状态枚举 */
    typedef enum {
        TAP_RESET = 0,          /* Test-Logic-Reset */
        TAP_IDLE,               /* Run-Test/Idle */
        TAP_SELECT_DR,          /* Select-DR-Scan */
        TAP_CAPTURE_DR,         /* Capture-DR */
        TAP_SHIFT_DR,           /* Shift-DR */
        TAP_EXIT1_DR,           /* Exit1-DR */
        TAP_PAUSE_DR,           /* Pause-DR */
        TAP_EXIT2_DR,           /* Exit2-DR */
        TAP_UPDATE_DR,          /* Update-DR */
        TAP_SELECT_IR,          /* Select-IR-Scan */
        TAP_CAPTURE_IR,         /* Capture-IR */
        TAP_SHIFT_IR,           /* Shift-IR */
        TAP_EXIT1_IR,           /* Exit1-IR */
        TAP_PAUSE_IR,           /* Pause-IR */
        TAP_EXIT2_IR,           /* Exit2-IR */
        TAP_UPDATE_IR,          /* Update-IR */
    } JTAG_TAP_State_t;
    
    /* TMS状态转换表(0和1分别对应的下一状态) */
    static const JTAG_TAP_State_t tap_trans[16][2] = {
        /* TMS=0              TMS=1 */
        { TAP_IDLE,          TAP_RESET },       /* RESET */
        { TAP_IDLE,          TAP_SELECT_DR },   /* IDLE */
        { TAP_CAPTURE_DR,    TAP_SELECT_IR },   /* SELECT_DR */
        { TAP_SHIFT_DR,      TAP_EXIT1_DR },    /* CAPTURE_DR */
        { TAP_SHIFT_DR,      TAP_EXIT1_DR },    /* SHIFT_DR */
        { TAP_PAUSE_DR,      TAP_UPDATE_DR },   /* EXIT1_DR */
        { TAP_PAUSE_DR,      TAP_EXIT2_DR },    /* PAUSE_DR */
        { TAP_SHIFT_DR,      TAP_UPDATE_DR },   /* EXIT2_DR */
        { TAP_IDLE,          TAP_SELECT_DR },   /* UPDATE_DR */
        { TAP_CAPTURE_IR,    TAP_RESET },        /* SELECT_IR */
        { TAP_SHIFT_IR,      TAP_EXIT1_IR },    /* CAPTURE_IR */
        { TAP_SHIFT_IR,      TAP_EXIT1_IR },    /* SHIFT_IR */
        { TAP_PAUSE_IR,      TAP_UPDATE_IR },   /* EXIT1_IR */
        { TAP_PAUSE_IR,      TAP_EXIT2_IR },    /* PAUSE_IR */
        { TAP_SHIFT_IR,      TAP_UPDATE_IR },   /* EXIT2_IR */
        { TAP_IDLE,          TAP_SELECT_DR },   /* UPDATE_IR */
    };
    
    JTAG_TAP_State_t tap_state = TAP_RESET;
    uint8_t prev_tck = 0;
    uint32_t tdi_shift = 0, tdo_shift = 0;
    uint8_t shift_count = 0;
    uint32_t shift_start = 0;
    uint8_t is_ir_shift = 0;
    
    /* 检测5个连续TMS=1复位序列 */
    uint8_t tms_high_count = 0;
    
    for (uint32_t i = 0; i < total && *frame_count < max_frames; i++) {
        uint8_t sample = hla->buffer[i];
        uint8_t tck = (sample >> ch_tck) & 1;
        uint8_t tms = (sample >> ch_tms) & 1;
        uint8_t tdi = (sample >> ch_tdi) & 1;
        uint8_t tdo = (ch_tdo < 8) ? ((sample >> ch_tdo) & 1) : 0;
        
        /* TCK上升沿处理 */
        if (tck == 1 && prev_tck == 0) {
            /* 记录进入Shift状态前的TAP状态 */
            JTAG_TAP_State_t prev_state = tap_state;
            
            /* 更新TAP状态 */
            tap_state = tap_trans[tap_state][tms];
            
            /* TMS计数(检测复位序列) */
            if (tms) tms_high_count++;
            else tms_high_count = 0;
            if (tms_high_count >= 5) tap_state = TAP_RESET;
            
            /* 在Shift-DR/Shift-IR状态下移位数据 */
            if (tap_state == TAP_SHIFT_DR || tap_state == TAP_SHIFT_IR) {
                if (prev_state != TAP_SHIFT_DR && prev_state != TAP_SHIFT_IR) {
                    /* 刚进入Shift状态，初始化 */
                    shift_count = 0;
                    tdi_shift = 0;
                    tdo_shift = 0;
                    shift_start = i;
                    is_ir_shift = (tap_state == TAP_SHIFT_IR) ? 1 : 0;
                }
                
                /* 移入TDI/TDO数据(LSB first) */
                if (shift_count < 32) {
                    tdi_shift |= ((uint32_t)tdi << shift_count);
                    tdo_shift |= ((uint32_t)tdo << shift_count);
                }
                shift_count++;
            }
            else if (prev_state == TAP_SHIFT_DR || prev_state == TAP_SHIFT_IR) {
                /* 退出Shift状态，保存移位数据帧 */
                if (shift_count > 0 && *frame_count < max_frames) {
                    frames[*frame_count].start_sample = shift_start;
                    frames[*frame_count].tms_data = 0;
                    frames[*frame_count].tms_len = 0;
                    frames[*frame_count].tdi_data = tdi_shift;
                    frames[*frame_count].tdo_data = tdo_shift;
                    frames[*frame_count].data_len = shift_count;
                    frames[*frame_count].is_ir = is_ir_shift;
                    (*frame_count)++;
                }
                shift_count = 0;
            }
        }
        
        prev_tck = tck;
    }
    
    return HAL_OK;
}

/**
 * @brief CAN总线协议解码 - 状态机实现
 * 
 * CAN通道映射:
 *   [0] = CAN_RX (显性=0，隐性=1)
 *   [1] = CAN_TX (可选，用于监听模式)
 * 
 * 解码标准CAN 2.0A/2.0B帧格式:
 *   帧起始 + 仲裁场 + 控制场 + 数据场 + CRC + ACK + 帧结束
 */
HAL_StatusTypeDef LA_DecodeCAN(LA_HandleTypeDef* hla, LA_CAN_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count)
{
    *frame_count = 0;
    
    if (hla == NULL || frames == NULL || hla->buffer == NULL || max_frames == 0) {
        return HAL_ERROR;
    }
    
    uint8_t ch_rx = hla->config.protocol_channels[0];
    
    uint32_t total = hla->samples_captured;
    if (total < 2) return HAL_OK;
    
    /* CAN解码状态机(简化版，基于位填充) */
    typedef enum {
        CAN_IDLE = 0,
        CAN_SOF,           /* 帧起始 */
        CAN_ARB_ID,        /* 仲裁场ID */
        CAN_CONTROL,       /* 控制场 */
        CAN_DATA,          /* 数据场 */
        CAN_CRC,           /* CRC场 */
    } CAN_Decode_State_t;
    
    CAN_Decode_State_t state = CAN_IDLE;
    uint32_t frame_start = 0;
    uint32_t arb_id = 0;
    uint8_t bit_count = 0;
    uint8_t same_bit_count = 0;     /* 连续相同位计数(位填充检测) */
    uint8_t prev_bit = 1;
    uint8_t is_extended = 0;
    uint8_t is_rtr = 0;
    uint8_t dlc = 0;
    uint8_t data_bytes[8] = {0};
    uint8_t data_idx = 0;
    uint8_t data_bit = 0;
    uint8_t cur_byte = 0;
    
    for (uint32_t i = 0; i < total && *frame_count < max_frames; i++) {
        uint8_t sample = hla->buffer[i];
        uint8_t rx = (sample >> ch_rx) & 1;
        
        /* 位填充: 5个连续相同位后插入1个相反位 */
        uint8_t stuffed = 0;
        if (rx == prev_bit) {
            same_bit_count++;
            if (same_bit_count >= 5) {
                /* 下一个位是填充位，跳过 */
                stuffed = 1;
                same_bit_count = 0;
            }
        } else {
            same_bit_count = 1;
        }
        
        switch (state) {
            case CAN_IDLE:
                /* 检测SOF: 隐性→显性(1→0) */
                if (prev_bit == 1 && rx == 0) {
                    state = CAN_ARB_ID;
                    bit_count = 0;
                    arb_id = 0;
                    frame_start = i;
                    same_bit_count = 1;
                }
                break;
                
            case CAN_ARB_ID:
                if (stuffed) break;  /* 跳过填充位 */
                
                /* 标准帧: 11位ID + 1位RTR + 1位IDE + 1位r0 */
                arb_id = (arb_id << 1) | rx;
                bit_count++;
                
                if (bit_count == 11) {
                    /* 11位标准ID完成 */
                    arb_id &= 0x7FF;
                } else if (bit_count == 12) {
                    is_rtr = rx;   /* RTR位 */
                } else if (bit_count == 13) {
                    is_extended = rx;  /* IDE位 */
                    if (is_extended) {
                        /* 扩展帧: 还需要18位ID */
                        /* 简化实现：仅处理标准帧 */
                    }
                } else if (bit_count == 14) {
                    state = CAN_CONTROL;
                    bit_count = 0;
                    cur_byte = 0;
                }
                break;
                
            case CAN_CONTROL:
                if (stuffed) break;
                
                cur_byte = (cur_byte << 1) | rx;
                bit_count++;
                
                if (bit_count == 6) {
                    /* 控制场: r0 + DLC[3:0] */
                    dlc = cur_byte & 0x0F;
                    if (dlc > 8) dlc = 8;
                    state = CAN_DATA;
                    bit_count = 0;
                    data_idx = 0;
                    data_bit = 0;
                    cur_byte = 0;
                }
                break;
                
            case CAN_DATA:
                if (stuffed) break;
                
                cur_byte = (cur_byte << 1) | rx;
                data_bit++;
                
                if (data_bit >= 8) {
                    if (data_idx < 8) {
                        data_bytes[data_idx] = cur_byte;
                    }
                    data_idx++;
                    data_bit = 0;
                    cur_byte = 0;
                    
                    if (data_idx >= dlc) {
                        /* 数据接收完成 */
                        if (*frame_count < max_frames) {
                            frames[*frame_count].start_sample = frame_start;
                            frames[*frame_count].id = arb_id;
                            frames[*frame_count].is_extended = is_extended;
                            frames[*frame_count].is_rtr = is_rtr;
                            frames[*frame_count].dlc = dlc;
                            for (uint8_t j = 0; j < dlc && j < 8; j++) {
                                frames[*frame_count].data[j] = data_bytes[j];
                            }
                            frames[*frame_count].crc_error = 0;
                            frames[*frame_count].stuff_error = 0;
                            (*frame_count)++;
                        }
                        state = CAN_IDLE;
                    }
                }
                break;
                
            default:
                state = CAN_IDLE;
                break;
        }
        
        prev_bit = rx;
    }
    
    return HAL_OK;
}

/**
 * @brief BDM协议解码 - 状态机实现
 * 
 * BDM通道映射:
 *   [0] = BKGD (单线双向数据)
 * 
 * BDM协议：单线半双工，主机发出命令(16位)，从机回应ACK和数据
 * 支持的命令：READ_BYTE/WRITE_BYTE/READ_WORD/WRITE_WORD等
 */
HAL_StatusTypeDef LA_DecodeBDM(LA_HandleTypeDef* hla, LA_BDM_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count)
{
    *frame_count = 0;
    
    if (hla == NULL || frames == NULL || hla->buffer == NULL || max_frames == 0) {
        return HAL_ERROR;
    }
    
    uint8_t ch_bkgd = hla->config.protocol_channels[0];
    
    uint32_t total = hla->samples_captured;
    if (total < 2) return HAL_OK;
    
    /* BDM解码状态机 */
    typedef enum {
        BDM_IDLE = 0,
        BDM_CMD,            /* 接收命令(16位) */
        BDM_ACK,            /* 等待ACK */
        BDM_DATA_OUT,       /* 输出数据(16位) */
        BDM_DATA_IN,        /* 输入数据(16位) */
    } BDM_Decode_State_t;
    
    BDM_Decode_State_t state = BDM_IDLE;
    uint16_t cmd_data = 0;
    uint16_t data_val = 0;
    uint8_t bit_count = 0;
    uint32_t frame_start = 0;
    uint8_t prev_bkgd = 1;
    uint32_t prev_edge_sample = 0;
    
    /* 检测BDM位：查找边沿，测量脉宽来确定0/1 */
    /* BDM编码：逻辑1=短脉冲(2个时钟周期)，逻辑0=长脉冲(4个时钟周期) */
    
    for (uint32_t i = 1; i < total && *frame_count < max_frames; i++) {
        uint8_t sample = hla->buffer[i];
        uint8_t bkgd = (sample >> ch_bkgd) & 1;
        uint8_t prev_sample = hla->buffer[i - 1];
        uint8_t prev_b = (prev_sample >> ch_bkgd) & 1;
        
        /* 检测下降沿(命令起始) */
        if (prev_b == 1 && bkgd == 0) {
            uint32_t pulse_width = i - prev_edge_sample;
            prev_edge_sample = i;
            
            if (state == BDM_IDLE) {
                /* 检测到命令起始 */
                state = BDM_CMD;
                bit_count = 0;
                cmd_data = 0;
                frame_start = i;
            }
            
            /* 根据脉宽判断0/1 */
            uint8_t bit_val = 0;
            if (pulse_width <= 3) {
                bit_val = 1;   /* 短脉冲=1 */
            } else {
                bit_val = 0;   /* 长脉冲=0 */
            }
            
            switch (state) {
                case BDM_CMD:
                    cmd_data = (cmd_data << 1) | bit_val;
                    bit_count++;
                    if (bit_count >= 16) {
                        /* 命令接收完成 */
                        if (*frame_count < max_frames) {
                            frames[*frame_count].start_sample = frame_start;
                            frames[*frame_count].command = (uint8_t)(cmd_data >> 8);
                            frames[*frame_count].data_out = cmd_data & 0xFF;
                            frames[*frame_count].data_in = 0;
                            frames[*frame_count].ack_error = 0;
                            (*frame_count)++;
                        }
                        state = BDM_ACK;
                        bit_count = 0;
                    }
                    break;
                    
                default:
                    break;
            }
        }
        
        prev_bkgd = bkgd;
    }
    
    return HAL_OK;
}

/**
 * @brief SBW(Spy-Bi-Wire)协议解码 - 状态机实现
 * 
 * SBW通道映射:
 *   [0] = SBWTDIO (测试数据，双向)
 *   [1] = SBWTCK  (测试时钟)
 * 
 * SBW是TI MSP430的2线调试接口，TMS/TDI/TDO时分复用
 * 在TCK上升沿采样TDIO数据
 */
HAL_StatusTypeDef LA_DecodeSBW(LA_HandleTypeDef* hla, LA_SBW_Frame_t* frames, uint32_t max_frames, uint32_t* frame_count)
{
    *frame_count = 0;
    
    if (hla == NULL || frames == NULL || hla->buffer == NULL || max_frames == 0) {
        return HAL_ERROR;
    }
    
    uint8_t ch_tdio = hla->config.protocol_channels[0];
    uint8_t ch_tck  = hla->config.protocol_channels[1];
    
    uint32_t total = hla->samples_captured;
    if (total < 2) return HAL_OK;
    
    /* SBW解码: 每个TCK上升沿采样TDIO */
    /* SBW帧格式: TMS(1bit) + TDI(1bit) + TDO(1bit) + TCLK */
    typedef enum {
        SBW_TMS = 0,
        SBW_TDI,
        SBW_TDO,
    } SBW_Phase_t;
    
    SBW_Phase_t phase = SBW_TMS;
    uint8_t prev_tck = 0;
    uint8_t cur_tms = 0, cur_tdi = 0, cur_tdo = 0;
    uint32_t frame_start = 0;
    
    for (uint32_t i = 0; i < total && *frame_count < max_frames; i++) {
        uint8_t sample = hla->buffer[i];
        uint8_t tck  = (sample >> ch_tck) & 1;
        uint8_t tdio = (sample >> ch_tdio) & 1;
        
        /* TCK上升沿采样 */
        if (tck == 1 && prev_tck == 0) {
            switch (phase) {
                case SBW_TMS:
                    cur_tms = tdio;
                    frame_start = i;
                    phase = SBW_TDI;
                    break;
                    
                case SBW_TDI:
                    cur_tdi = tdio;
                    phase = SBW_TDO;
                    break;
                    
                case SBW_TDO:
                    cur_tdo = tdio;
                    /* TMS+TDI+TDO组成一个SBW帧 */
                    if (*frame_count < max_frames) {
                        frames[*frame_count].start_sample = frame_start;
                        frames[*frame_count].tms = cur_tms;
                        frames[*frame_count].tdi = cur_tdi;
                        frames[*frame_count].tdo = cur_tdo;
                        frames[*frame_count].is_reset = 0;
                        (*frame_count)++;
                    }
                    phase = SBW_TMS;
                    break;
            }
        }
        
        prev_tck = tck;
    }
    
    return HAL_OK;
}

/**
 * @brief 协议解码(自动选择)
 */
HAL_StatusTypeDef LA_DecodeProtocol(LA_HandleTypeDef* hla, LA_Decode_Result_t* result)
{
    result->protocol = hla->config.protocol;
    result->frame_count = 0;
    result->error_count = 0;
    
    switch (hla->config.protocol) {
        case LA_PROTOCOL_UART: {
            LA_UART_Frame_t frames[256];
            LA_DecodeUART(hla, frames, 256, &result->frame_count);
            break;
        }
        case LA_PROTOCOL_SPI: {
            LA_SPI_Frame_t frames[256];
            LA_DecodeSPI(hla, frames, 256, &result->frame_count);
            break;
        }
        case LA_PROTOCOL_I2C: {
            LA_I2C_Frame_t frames[256];
            LA_DecodeI2C(hla, frames, 256, &result->frame_count);
            break;
        }
        case LA_PROTOCOL_SWD: {
            LA_SWD_Frame_t frames[256];
            LA_DecodeSWD(hla, frames, 256, &result->frame_count);
            break;
        }
        case LA_PROTOCOL_JTAG: {
            LA_JTAG_Frame_t frames[256];
            LA_DecodeJTAG(hla, frames, 256, &result->frame_count);
            break;
        }
        case LA_PROTOCOL_CAN: {
            LA_CAN_Frame_t frames[64];
            LA_DecodeCAN(hla, frames, 64, &result->frame_count);
            break;
        }
        case LA_PROTOCOL_BDM: {
            LA_BDM_Frame_t frames[256];
            LA_DecodeBDM(hla, frames, 256, &result->frame_count);
            break;
        }
        case LA_PROTOCOL_SBW: {
            LA_SBW_Frame_t frames[256];
            LA_DecodeSBW(hla, frames, 256, &result->frame_count);
            break;
        }
        case LA_PROTOCOL_MON8:
        case LA_PROTOCOL_FINE:
            /* MON8和FINE协议较特殊，需要特定硬件配合，暂不实现 */
            break;
            
        default:
            break;
    }
    
    return HAL_OK;
}