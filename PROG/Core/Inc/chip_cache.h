/**
 ******************************************************************************
 * @file    chip_cache.h
 * @brief   芯片驱动框架缓存模块头文件
 ******************************************************************************
 */

#ifndef __CHIP_CACHE_H__
#define __CHIP_CACHE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "chip_driver_framework.h"

/**
 * @brief 缓存大小配置
 * @note  配置缓存条目数量，默认32个条目
 */
#define CHIP_CACHE_SIZE    32

/**
 * @brief 初始化缓存模块
 */
void Chip_Cache_Init(void);

/**
 * @brief 添加缓存条目
 * @param chip_id: 芯片ID
 * @param jtag_id: JTAG ID
 * @param flash_id: Flash ID
 * @param device_id: 设备ID
 * @param signature: 芯片签名
 * @param chip_info: 芯片信息指针
 * @param driver: 驱动操作指针
 * @return HAL状态
 */
HAL_StatusTypeDef Chip_Cache_Add(uint32_t chip_id, uint32_t jtag_id,
                                  uint32_t flash_id, uint32_t device_id,
                                  const uint8_t* signature,
                                  const Chip_Info_t* chip_info,
                                  const Chip_Driver_Ops_t* driver);

/**
 * @brief 查找缓存中的芯片信息
 * @param chip_id: 芯片ID
 * @param jtag_id: JTAG ID
 * @param flash_id: Flash ID
 * @param device_id: 设备ID
 * @param signature: 芯片签名
 * @param chip_info: 输出芯片信息指针
 * @param driver: 输出驱动操作指针
 * @return HAL状态
 */
HAL_StatusTypeDef Chip_Cache_Lookup(uint32_t chip_id, uint32_t jtag_id,
                                     uint32_t flash_id, uint32_t device_id,
                                     const uint8_t* signature,
                                     const Chip_Info_t** chip_info,
                                     const Chip_Driver_Ops_t** driver);

/**
 * @brief 移除缓存条目
 * @param chip_id: 芯片ID
 * @return HAL状态
 */
HAL_StatusTypeDef Chip_Cache_Remove(uint32_t chip_id);

/**
 * @brief 清空缓存
 */
void Chip_Cache_Clear(void);

/**
 * @brief 获取缓存统计信息
 * @param hits: 命中次数
 * @param misses: 未命中次数
 * @param hit_rate: 命中率(百分比)
 */
void Chip_Cache_GetStats(uint32_t* hits, uint32_t* misses, float* hit_rate);

#ifdef __cplusplus
}
#endif

#endif /* __CHIP_CACHE_H__ */
