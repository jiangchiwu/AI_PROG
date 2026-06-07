/**
 ******************************************************************************
 * @file    chip_cache.c
 * @brief   芯片驱动框架缓存模块
 *          提供芯片信息的快速查找和缓存机制，提升芯片识别效率
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 * 
 * @details 本模块实现芯片信息的缓存机制，主要功能包括：
 *          1. 芯片ID查找缓存
 *          2. 驱动匹配结果缓存
 *          3. 芯片信息缓存
 *          4. 缓存过期策略
 * 
 * @note    使用LRU(最近最少使用)算法管理缓存
 * @note    缓存大小可配置，默认支持32个缓存条目
 ******************************************************************************
 */

#include "chip_cache.h"

/**
 * @brief 芯片缓存结构体
 */
typedef struct {
    uint32_t chip_id;
    uint32_t jtag_id;
    uint32_t flash_id;
    uint32_t device_id;
    uint8_t  signature[3];
    const Chip_Info_t* chip_info;
    const Chip_Driver_Ops_t* driver;
    uint32_t access_count;
    uint32_t last_access_time;
} Chip_Cache_Entry_t;

/**
 * @brief 芯片缓存表
 */
static Chip_Cache_Entry_t s_chip_cache[CHIP_CACHE_SIZE] = {0};

/**
 * @brief 缓存命中统计
 */
static uint32_t s_cache_hits = 0;

/**
 * @brief 缓存未命中统计
 */
static uint32_t s_cache_misses = 0;

/**
 * @brief 当前时间戳（模拟）
 */
static uint32_t s_current_time = 0;

/**
 * @brief 获取当前时间戳
 * @return 当前时间戳
 */
static inline uint32_t Chip_Cache_GetTime(void)
{
    return s_current_time++;
}

/**
 * @brief 查找缓存条目
 * @param chip_id: 芯片ID
 * @param jtag_id: JTAG ID
 * @param flash_id: Flash ID
 * @param device_id: 设备ID
 * @param signature: 芯片签名
 * @return 缓存条目索引，未找到返回CHIP_CACHE_SIZE
 */
static uint32_t Chip_Cache_FindEntry(uint32_t chip_id, uint32_t jtag_id, 
                                      uint32_t flash_id, uint32_t device_id,
                                      const uint8_t* signature)
{
    for (uint32_t i = 0; i < CHIP_CACHE_SIZE; i++) {
        if (s_chip_cache[i].chip_id == 0) {
            continue;
        }
        
        /* 精确匹配芯片ID */
        if (chip_id != 0 && s_chip_cache[i].chip_id == chip_id) {
            return i;
        }
        
        /* 匹配JTAG ID */
        if (jtag_id != 0 && s_chip_cache[i].jtag_id == jtag_id) {
            return i;
        }
        
        /* 匹配Flash ID */
        if (flash_id != 0 && s_chip_cache[i].flash_id == flash_id) {
            return i;
        }
        
        /* 匹配设备ID */
        if (device_id != 0 && s_chip_cache[i].device_id == device_id) {
            return i;
        }
        
        /* 匹配签名 */
        if (signature != NULL && 
            s_chip_cache[i].signature[0] != 0 &&
            memcmp(s_chip_cache[i].signature, signature, 3) == 0) {
            return i;
        }
    }
    
    return CHIP_CACHE_SIZE;
}

/**
 * @brief 查找LRU条目（最近最少使用）
 * @return LRU条目索引
 */
static uint32_t Chip_Cache_FindLRUEntry(void)
{
    uint32_t lru_index = 0;
    uint32_t min_access = s_chip_cache[0].access_count;
    uint32_t min_time = s_chip_cache[0].last_access_time;
    
    for (uint32_t i = 1; i < CHIP_CACHE_SIZE; i++) {
        /* 优先使用空闲条目 */
        if (s_chip_cache[i].chip_id == 0) {
            return i;
        }
        
        /* 查找访问次数最少的条目 */
        if (s_chip_cache[i].access_count < min_access) {
            min_access = s_chip_cache[i].access_count;
            min_time = s_chip_cache[i].last_access_time;
            lru_index = i;
        } else if (s_chip_cache[i].access_count == min_access) {
            /* 访问次数相同时，选择最久未访问的 */
            if (s_chip_cache[i].last_access_time < min_time) {
                min_time = s_chip_cache[i].last_access_time;
                lru_index = i;
            }
        }
    }
    
    return lru_index;
}

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
                                  const Chip_Driver_Ops_t* driver)
{
    /* 查找是否已存在 */
    uint32_t index = Chip_Cache_FindEntry(chip_id, jtag_id, flash_id, device_id, signature);
    
    if (index < CHIP_CACHE_SIZE) {
        /* 更新现有条目 */
        s_chip_cache[index].chip_info = chip_info;
        s_chip_cache[index].driver = driver;
        s_chip_cache[index].access_count++;
        s_chip_cache[index].last_access_time = Chip_Cache_GetTime();
        return HAL_OK;
    }
    
    /* 查找LRU条目 */
    index = Chip_Cache_FindLRUEntry();
    
    /* 添加新条目 */
    s_chip_cache[index].chip_id = chip_id;
    s_chip_cache[index].jtag_id = jtag_id;
    s_chip_cache[index].flash_id = flash_id;
    s_chip_cache[index].device_id = device_id;
    if (signature != NULL) {
        memcpy(s_chip_cache[index].signature, signature, 3);
    } else {
        memset(s_chip_cache[index].signature, 0, 3);
    }
    s_chip_cache[index].chip_info = chip_info;
    s_chip_cache[index].driver = driver;
    s_chip_cache[index].access_count = 1;
    s_chip_cache[index].last_access_time = Chip_Cache_GetTime();
    
    return HAL_OK;
}

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
                                     const Chip_Driver_Ops_t** driver)
{
    uint32_t index = Chip_Cache_FindEntry(chip_id, jtag_id, flash_id, device_id, signature);
    
    if (index < CHIP_CACHE_SIZE) {
        /* 更新访问统计 */
        s_chip_cache[index].access_count++;
        s_chip_cache[index].last_access_time = Chip_Cache_GetTime();
        
        /* 返回缓存数据 */
        if (chip_info != NULL) {
            *chip_info = s_chip_cache[index].chip_info;
        }
        if (driver != NULL) {
            *driver = s_chip_cache[index].driver;
        }
        
        s_cache_hits++;
        return HAL_OK;
    }
    
    s_cache_misses++;
    return HAL_ERROR;
}

/**
 * @brief 移除缓存条目
 * @param chip_id: 芯片ID
 * @return HAL状态
 */
HAL_StatusTypeDef Chip_Cache_Remove(uint32_t chip_id)
{
    for (uint32_t i = 0; i < CHIP_CACHE_SIZE; i++) {
        if (s_chip_cache[i].chip_id == chip_id) {
            memset(&s_chip_cache[i], 0, sizeof(Chip_Cache_Entry_t));
            return HAL_OK;
        }
    }
    
    return HAL_ERROR;
}

/**
 * @brief 清空缓存
 */
void Chip_Cache_Clear(void)
{
    memset(s_chip_cache, 0, sizeof(s_chip_cache));
    s_cache_hits = 0;
    s_cache_misses = 0;
    s_current_time = 0;
}

/**
 * @brief 获取缓存统计信息
 * @param hits: 命中次数
 * @param misses: 未命中次数
 * @param hit_rate: 命中率(百分比)
 */
void Chip_Cache_GetStats(uint32_t* hits, uint32_t* misses, float* hit_rate)
{
    *hits = s_cache_hits;
    *misses = s_cache_misses;
    
    uint32_t total = s_cache_hits + s_cache_misses;
    if (total > 0) {
        *hit_rate = ((float)s_cache_hits / total) * 100.0f;
    } else {
        *hit_rate = 0.0f;
    }
}

/**
 * @brief 初始化缓存模块
 */
void Chip_Cache_Init(void)
{
    Chip_Cache_Clear();
}
