/**
  ******************************************************************************
  * @file    spi_flash.h
  * @brief   SPI Flash/NOR Flash 编程驱动头文件
  * @details 该模块实现了对主流 SPI Flash 芯片的编程驱动，支持多种厂商和型号。
  *          对标 RT809 等专业编程器产品功能。
  *          
  *          支持的厂商包括：
  *          - Winbond (华邦): W25Q 系列
  *          - Macronix (旺宏): MX25L 系列
  *          - Micron (美光): M25P/N25Q 系列
  *          - ISSI (芯成): IS25LP 系列
  *          - GigaDevice (兆易创新): GD25Q 系列
  *          - Cypress/Spansion (赛普拉斯): S25FL 系列
  *          
  *          支持的操作模式：
  *          - 标准 SPI 模式 (Mode 0 和 Mode 3)
  *          - 双输出 SPI (Dual Output)
  *          - 四输出 SPI (Quad SPI / QSPI)
  ******************************************************************************
  * @attention
  *
  * 本驱动用于 STM32H7 平台嵌入式编程器项目
  * 使用 STM32 HAL 库进行 SPI 通信
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Exported constants --------------------------------------------------------*/

/**
 * @defgroup SPI_Flash_Commands SPI Flash 命令集定义
 * @brief 定义所有 SPI Flash 标准命令和扩展命令
 * @{
 */

/* ========== 基本读取命令 ========== */
#define SPI_FLASH_CMD_READ              0x03U   /**< 读取数据（标准读取，最大33MHz） */
#define SPI_FLASH_CMD_FAST_READ         0x0BU   /**< 快速读取（支持最高133MHz，需等待8个dummy时钟） */
#define SPI_FLASH_CMD_DUAL_READ         0x3BU   /**< 双输出快速读取 */
#define SPI_FLASH_CMD_QUAD_READ         0x6BU   /**< 四输出快速读取 */
#define SPI_FLASH_CMD_DUAL_IO_READ      0xBBU   /**< 双IO快速读取 */
#define SPI_FLASH_CMD_QUAD_IO_READ      0xEBU   /**< 四IO快速读取 */

/* ========== 编程命令 ========== */
#define SPI_FLASH_CMD_PAGE_PROGRAM      0x02U   /**< 页编程（每次最多256字节） */
#define SPI_FLASH_CMD_DUAL_PAGE_PROGRAM 0xA2U   /**< 双输出页编程 */
#define SPI_FLASH_CMD_QUAD_PAGE_PROGRAM 0x32U   /**< 四输出页编程 */

/* ========== 擦除命令 ========== */
#define SPI_FLASH_CMD_SECTOR_ERASE_4K   0x20U   /**< 4KB扇区擦除 */
#define SPI_FLASH_CMD_BLOCK_ERASE_32K   0x52U   /**< 32KB块擦除 */
#define SPI_FLASH_CMD_BLOCK_ERASE_64K   0xD8U   /**< 64KB块擦除 */
#define SPI_FLASH_CMD_CHIP_ERASE        0xC7U   /**< 全片擦除 */
#define SPI_FLASH_CMD_CHIP_ERASE_ALT     0x60U   /**< 全片擦除（备用命令） */

/* ========== ID 读取命令 ========== */
#define SPI_FLASH_CMD_JEDEC_ID          0x9FU   /**< 读取 JEDEC ID（厂商ID+设备ID） */
#define SPI_FLASH_CMD_MANUFACTURER_ID   0x90U   /**< 读取厂商ID */
#define SPI_FLASH_CMD_DEVICE_ID         0xABU   /**< 读取设备ID/释放掉电 */
#define SPI_FLASH_CMD_UNIQUE_ID         0x4BU   /**< 读取唯一序列号 */

/* ========== 写保护命令 ========== */
#define SPI_FLASH_CMD_WRITE_ENABLE      0x06U   /**< 写使能 */
#define SPI_FLASH_CMD_WRITE_DISABLE     0x04U   /**< 写禁止 */
#define SPI_FLASH_CMD_READ_STATUS_REG1  0x05U   /**< 读取状态寄存器1 */
#define SPI_FLASH_CMD_READ_STATUS_REG2  0x35U   /**< 读取状态寄存器2 */
#define SPI_FLASH_CMD_READ_STATUS_REG3  0x15U   /**< 读取状态寄存器3 */
#define SPI_FLASH_CMD_WRITE_STATUS_REG1 0x01U   /**< 写状态寄存器1 */
#define SPI_FLASH_CMD_WRITE_STATUS_REG2 0x31U   /**< 写状态寄存器2 */
#define SPI_FLASH_CMD_WRITE_STATUS_REG3 0x11U   /**< 写状态寄存器3 */

/* ========== 安全/保护命令 ========== */
#define SPI_FLASH_CMD_READ_SFDP         0x5AU   /**< 读取 SFDP (Serial Flash Discoverable Parameters) */
#define SPI_FLASH_CMD_READ_SECURITY     0x48U   /**< 读取安全寄存器 */
#define SPI_FLASH_CMD_PROGRAM_SECURITY  0x42U   /**< 编程安全寄存器 */
#define SPI_FLASH_CMD_ERASE_SECURITY    0x44U   /**< 擦除安全寄存器 */

/* ========== 电源管理命令 ========== */
#define SPI_FLASH_CMD_POWER_DOWN        0xB9U   /**< 进入掉电模式 */
#define SPI_FLASH_CMD_RELEASE_POWER_DOWN 0xABU  /**< 释放掉电模式/读取设备ID */

/* ========== 复位命令 ========== */
#define SPI_FLASH_CMD_ENABLE_RESET      0x66U   /**< 复位使能 */
#define SPI_FLASH_CMD_RESET             0x99U   /**< 执行软件复位 */

/* ========== 暂停/恢复命令 ========== */
#define SPI_FLASH_CMD_PROGRAM_SUSPEND   0x75U   /**< 暂停编程/擦除 */
#define SPI_FLASH_CMD_PROGRAM_RESUME    0x7AU   /**< 恢复编程/擦除 */

/* ========== 4字节地址模式命令 ========== */
#define SPI_FLASH_CMD_ENTER_4BYTE_MODE  0xB7U   /**< 进入4字节地址模式（支持大于128Mb容量） */
#define SPI_FLASH_CMD_EXIT_4BYTE_MODE   0xE9U   /**< 退出4字节地址模式 */

/** @} */

/**
 * @defgroup SPI_Flash_Status_Bits SPI Flash 状态寄存器位定义
 * @brief 状态寄存器各位的定义
 * @{
 */
#define SPI_FLASH_STATUS_BUSY           0x01U   /**< 忙标志位（bit0）：1=正在操作，0=空闲 */
#define SPI_FLASH_STATUS_WEL            0x02U   /**< 写使能标志位（bit1）：1=已使能，0=未使能 */
#define SPI_FLASH_STATUS_BP0            0x04U   /**< 块保护位0（bit2） */
#define SPI_FLASH_STATUS_BP1            0x08U   /**< 块保护位1（bit3） */
#define SPI_FLASH_STATUS_BP2            0x10U   /**< 块保护位2（bit4） */
#define SPI_FLASH_STATUS_TB             0x20U   /**< 顶部/底部保护位（bit5） */
#define SPI_FLASH_STATUS_SRP0           0x80U   /**< 状态寄存器保护位0（bit7） */

/* 状态寄存器2位定义 */
#define SPI_FLASH_STATUS2_SRP1          0x01U   /**< 状态寄存器保护位1（bit0） */
#define SPI_FLASH_STATUS2_QE            0x02U   /**< 四线使能位（bit1） */
#define SPI_FLASH_STATUS2_LB1           0x08U   /**< 锁定位1（bit3） */
#define SPI_FLASH_STATUS2_LB2           0x10U   /**< 锁定位2（bit4） */
#define SPI_FLASH_STATUS2_LB3           0x20U   /**< 锁定位3（bit5） */
#define SPI_FLASH_STATUS2_CMP           0x40U   /**< 比较保护位（bit6） */
#define SPI_FLASH_STATUS2_SUS           0x80U   /**< 暂停状态位（bit7） */

/** @} */

/**
 * @defgroup SPI_Flash_Vendor_ID SPI Flash 厂商ID定义
 * @brief 各厂商的 JEDEC 厂商ID
 * @{
 */
#define SPI_FLASH_VENDOR_WINBOND        0xEFU   /**< Winbond (华邦) 厂商ID */
#define SPI_FLASH_VENDOR_MACRONIX       0xC2U   /**< Macronix (旺宏) 厂商ID */
#define SPI_FLASH_VENDOR_MICRON         0x20U   /**< Micron (美光) 厂商ID */
#define SPI_FLASH_VENDOR_ISSI           0x9DU   /**< ISSI (芯成) 厂商ID */
#define SPI_FLASH_VENDOR_GIGADEVICE     0xC8U   /**< GigaDevice (兆易创新) 厂商ID */
#define SPI_FLASH_VENDOR_CYPRESS        0x01U   /**< Cypress/Spansion (赛普拉斯) 厂商ID */
#define SPI_FLASH_VENDOR_SPANSION       0x01U   /**< Spansion (已被Cypress收购) 厂商ID */
#define SPI_FLASH_VENDOR_SST            0xBFU   /**< SST 厂商ID */
#define SPI_FLASH_VENDOR_ATMEL          0x1FU   /**< Atmel (已被Microchip收购) 厂商ID */
#define SPI_FLASH_VENDOR_AMIC           0x37U   /**< AMIC 厂商ID */
#define SPI_FLASH_VENDOR_EON            0x1CU   /**< EON 厂商ID */
#define SPI_FLASH_VENDOR_PUYA           0x85U   /**< PUYA (普冉) 厂商ID */
#define SPI_FLASH_VENDOR_XMC            0x20U   /**< XMC (武汉新芯) 厂商ID */

/** @} */

/**
 * @defgroup SPI_Flash_Error_Codes SPI Flash 错误码定义
 * @brief 函数返回的错误码定义
 * @{
 */
#define SPI_FLASH_OK                    HAL_OK          /**< 操作成功 */
#define SPI_FLASH_ERROR                 HAL_ERROR       /**< 通用错误 */
#define SPI_FLASH_BUSY                  HAL_BUSY        /**< 设备忙 */
#define SPI_FLASH_TIMEOUT               HAL_TIMEOUT     /**< 操作超时 */
#define SPI_FLASH_NOT_DETECTED          0xF0U           /**< 未检测到芯片 */
#define SPI_FLASH_NOT_SUPPORTED         0xF1U           /**< 不支持的芯片型号 */
#define SPI_FLASH_WRITE_PROTECTED       0xF2U           /**< 写保护状态 */
#define SPI_FLASH_ERASE_FAILED          0xF3U           /**< 擦除失败 */
#define SPI_FLASH_PROGRAM_FAILED        0xF4U           /**< 编程失败 */

/** @} */

/**
 * @defgroup SPI_Flash_Speed SPI Flash 速度等级定义
 * @brief SPI Flash 支持的最高时钟频率
 * @{
 */
#define SPI_FLASH_SPEED_33MHZ           33000000U       /**< 33 MHz */
#define SPI_FLASH_SPEED_50MHZ           50000000U       /**< 50 MHz */
#define SPI_FLASH_SPEED_66MHZ           66000000U       /**< 66 MHz */
#define SPI_FLASH_SPEED_80MHZ           80000000U       /**< 80 MHz */
#define SPI_FLASH_SPEED_104MHZ          104000000U      /**< 104 MHz */
#define SPI_FLASH_SPEED_133MHZ          133000000U      /**< 133 MHz */
#define SPI_FLASH_SPEED_166MHZ          166000000U      /**< 166 MHz (需使用快速读取) */

/** @} */

/**
 * @defgroup SPI_Flash_Operation_Mode SPI Flash 操作模式定义
 * @brief SPI Flash 支持的操作模式
 * @{
 */
#define SPI_FLASH_MODE_STANDARD         0x00U   /**< 标准SPI模式（单线） */
#define SPI_FLASH_MODE_DUAL_OUTPUT      0x01U   /**< 双输出模式 */
#define SPI_FLASH_MODE_DUAL_IO          0x02U   /**< 双IO模式 */
#define SPI_FLASH_MODE_QUAD_OUTPUT      0x03U   /**< 四输出模式 */
#define SPI_FLASH_MODE_QUAD_IO          0x04U   /**< 四IO模式 */

/** @} */

/**
 * @defgroup SPI_Flash_Flags SPI Flash 状态标志定义
 * @brief SPI Flash 句柄中的状态标志
 * @{
 */
#define SPI_FLASH_FLAG_INITIALIZED      0x0001U     /**< 已初始化标志 */
#define SPI_FLASH_FLAG_DETECTED         0x0002U     /**< 已检测到芯片 */
#define SPI_FLASH_FLAG_BUSY             0x0004U     /**< 操作忙标志 */
#define SPI_FLASH_FLAG_WRITE_ENABLED    0x0008U     /**< 写使能状态 */
#define SPI_FLASH_FLAG_4BYTE_MODE       0x0010U     /**< 4字节地址模式 */
#define SPI_FLASH_FLAG_QUAD_ENABLED     0x0020U     /**< 四线模式使能 */
#define SPI_FLASH_FLAG_POWER_DOWN       0x0040U     /**< 掉电模式 */
#define SPI_FLASH_FLAG_PROTECTED        0x0080U     /**< 写保护状态 */

/** @} */

/* Exported types ------------------------------------------------------------*/

/**
 * @brief SPI Flash 芯片信息结构体
 * @details 存储已知的 SPI Flash 芯片型号信息，用于自动识别
 */
typedef struct {
    uint8_t manufacturer_id;    /**< 厂商ID (JEDEC Manufacturer ID) */
    uint8_t device_id;          /**< 设备ID (Device ID, 第一个字节) */
    uint8_t capacity_id;        /**< 容量ID (Capacity ID, 第二个字节) */
    uint32_t capacity;          /**< 芯片容量（字节） */
    uint32_t sector_size;       /**< 扇区大小（字节，通常为4KB） */
    uint32_t page_size;         /**< 页大小（字节，通常为256字节） */
    uint32_t block_size;        /**< 块大小（字节，通常为64KB） */
    const char *manufacturer;   /**< 厂商名称字符串 */
    const char *model_name;     /**< 型号名称字符串 */
    uint32_t max_speed;         /**< 最高时钟频率 (Hz) */
    uint8_t support_quad;       /**< 是否支持四线模式 */
} SPI_Flash_ChipInfo_t;

/**
 * @brief SPI Flash 操作句柄结构体
 * @details 定义 SPI Flash 驱动的完整句柄，包含所有必要的状态和配置信息
 */
typedef struct {
    SPI_HandleTypeDef *hspi;            /**< HAL SPI 句柄指针 */
    
    /* GPIO 引脚配置 */
    GPIO_TypeDef *cs_port;              /**< CS 片选端口 */
    uint16_t cs_pin;                   /**< CS 片选引脚 */
    GPIO_TypeDef *wp_port;              /**< WP 写保护端口（可选） */
    uint16_t wp_pin;                   /**< WP 写保护引脚（可选，设为0表示不使用） */
    GPIO_TypeDef *hold_port;            /**< HOLD 端口（可选） */
    uint16_t hold_pin;                 /**< HOLD 引脚（可选，设为0表示不使用） */
    GPIO_TypeDef *rst_port;             /**< RST 复位端口（可选） */
    uint16_t rst_pin;                  /**< RST 复位引脚（可选，设为0表示不使用） */
    
    /* 芯片参数 */
    uint32_t capacity;                 /**< 芯片容量（字节） */
    uint32_t sector_size;               /**< 扇区大小（字节） */
    uint32_t page_size;                 /**< 页大小（字节） */
    uint32_t block_size;                /**< 块大小（字节） */
    
    /* 芯片识别信息 */
    uint8_t manufacturer_id;            /**< 厂商ID */
    uint16_t device_id;                 /**< 设备ID（16位） */
    uint32_t jedec_id;                  /**< JEDEC ID（24位） */
    
    /* 配置参数 */
    uint32_t spi_speed;                 /**< SPI 通信速度 (Hz) */
    uint8_t spi_mode;                   /**< SPI 模式 (0 或 3) */
    uint8_t operation_mode;             /**< 操作模式（标准/双线/四线） */
    uint32_t timeout;                   /**< 操作超时时间 (ms) */
    
    /* 状态标志 */
    volatile uint16_t flags;            /**< 状态标志位 */
    
    /* 芯片信息指针 */
    const SPI_Flash_ChipInfo_t *chip_info;  /**< 当前芯片信息指针 */
    
} SPI_Flash_HandleTypeDef;

/* Exported variables --------------------------------------------------------*/

/**
 * @brief SPI Flash 芯片信息表
 * @details 包含所有支持的 SPI Flash 芯片型号信息
 * @note 表格以 manufacturer_id = 0xFF 结尾，用于遍历检测
 */
extern const SPI_Flash_ChipInfo_t SPI_Flash_ChipTable[];

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @defgroup SPI_Flash_Init_DeInit 初始化和反初始化函数
 * @{
 */

/**
 * @brief 初始化 SPI Flash 接口
 * @param hflash SPI Flash 句柄指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_Init(SPI_Flash_HandleTypeDef *hflash);

/**
 * @brief 反初始化 SPI Flash 接口
 * @param hflash SPI Flash 句柄指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_DeInit(SPI_Flash_HandleTypeDef *hflash);

/** @} */

/**
 * @defgroup SPI_Flash_ID ID 读取函数
 * @{
 */

/**
 * @brief 读取 JEDEC ID
 * @details 发送 0x9F 命令，读取 3 字节 JEDEC ID
 *          格式：[厂商ID(1字节)][设备ID(2字节)]
 * @param hflash SPI Flash 句柄指针
 * @param jedec_id 用于存储 JEDEC ID 的指针（24位）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_ReadJEDEC_ID(SPI_Flash_HandleTypeDef *hflash, uint32_t *jedec_id);

/**
 * @brief 读取厂商 ID
 * @details 发送 0x90 命令，读取厂商 ID
 *          格式：[厂商ID][设备ID]
 * @param hflash SPI Flash 句柄指针
 * @param manufacturer_id 用于存储厂商 ID 的指针
 * @param device_id 用于存储设备 ID 的指针（可选，设为NULL忽略）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_ReadManufacturerID(SPI_Flash_HandleTypeDef *hflash, 
                                                uint8_t *manufacturer_id, 
                                                uint16_t *device_id);

/**
 * @brief 读取设备 ID
 * @details 发送 0xAB 命令，读取设备 ID
 *          该命令也可用于从掉电模式唤醒
 * @param hflash SPI Flash 句柄指针
 * @param device_id 用于存储设备 ID 的指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_ReadDeviceID(SPI_Flash_HandleTypeDef *hflash, uint16_t *device_id);

/**
 * @brief 自动识别 SPI Flash 芯片
 * @details 通过 JEDEC ID 查询芯片信息表，自动识别芯片型号
 * @param hflash SPI Flash 句柄指针
 * @retval HAL_OK 识别成功
 * @retval SPI_FLASH_NOT_DETECTED 未检测到芯片
 * @retval SPI_FLASH_NOT_SUPPORTED 不支持的芯片型号
 */
HAL_StatusTypeDef SPI_Flash_AutoDetect(SPI_Flash_HandleTypeDef *hflash);

/** @} */

/**
 * @defgroup SPI_Flash_Read 读取函数
 * @{
 */

/**
 * @brief 读取数据（标准读取）
 * @details 使用 0x03 命令，支持任意地址和长度的读取
 *          最大时钟频率约 33 MHz
 * @param hflash SPI Flash 句柄指针
 * @param addr 起始地址
 * @param data 数据缓冲区指针
 * @param size 读取长度（字节）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_Read(SPI_Flash_HandleTypeDef *hflash, 
                                   uint32_t addr, 
                                   uint8_t *data, 
                                   uint32_t size);

/**
 * @brief 快速读取数据
 * @details 使用 0x0B 命令，支持最高 133 MHz 时钟频率
 *          需要额外的 8 个 dummy 时钟周期
 * @param hflash SPI Flash 句柄指针
 * @param addr 起始地址
 * @param data 数据缓冲区指针
 * @param size 读取长度（字节）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_FastRead(SPI_Flash_HandleTypeDef *hflash, 
                                       uint32_t addr, 
                                       uint8_t *data, 
                                       uint32_t size);

/**
 * @brief 双输出快速读取
 * @details 使用 0x3B 命令，使用双线数据输出，提高读取速度
 * @param hflash SPI Flash 句柄指针
 * @param addr 起始地址
 * @param data 数据缓冲区指针
 * @param size 读取长度（字节）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_DualRead(SPI_Flash_HandleTypeDef *hflash, 
                                       uint32_t addr, 
                                       uint8_t *data, 
                                       uint32_t size);

/**
 * @brief 四输出快速读取
 * @details 使用 0x6B 命令，使用四线数据输出，最大化读取速度
 * @param hflash SPI Flash 句柄指针
 * @param addr 起始地址
 * @param data 数据缓冲区指针
 * @param size 读取长度（字节）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_QuadRead(SPI_Flash_HandleTypeDef *hflash, 
                                        uint32_t addr, 
                                        uint8_t *data, 
                                        uint32_t size);

/** @} */

/**
 * @defgroup SPI_Flash_Write 写入和编程函数
 * @{
 */

/**
 * @brief 页编程
 * @details 使用 0x02 命令，每次最多写入一页（256字节）
 *          写入地址必须在同一页内，跨页部分将被忽略
 * @param hflash SPI Flash 句柄指针
 * @param addr 起始地址（必须页对齐效果最好）
 * @param data 待写入的数据指针
 * @param size 写入长度（字节，最大256）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_PageProgram(SPI_Flash_HandleTypeDef *hflash, 
                                          uint32_t addr, 
                                          const uint8_t *data, 
                                          uint16_t size);

/**
 * @brief 写入数据
 * @details 自动分页写入，内部调用 PageProgram 实现
 *          支持任意地址和长度，自动处理跨页和页对齐
 * @param hflash SPI Flash 句柄指针
 * @param addr 起始地址
 * @param data 待写入的数据指针
 * @param size 写入长度（字节）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_Write(SPI_Flash_HandleTypeDef *hflash, 
                                    uint32_t addr, 
                                    const uint8_t *data, 
                                    uint32_t size);

/**
 * @brief 四线页编程
 * @details 使用 0x32 命令，使用四线数据输入，提高编程速度
 * @param hflash SPI Flash 句柄指针
 * @param addr 起始地址
 * @param data 待写入的数据指针
 * @param size 写入长度（字节，最大256）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_QuadPageProgram(SPI_Flash_HandleTypeDef *hflash, 
                                              uint32_t addr, 
                                              const uint8_t *data, 
                                              uint16_t size);

/** @} */

/**
 * @defgroup SPI_Flash_Erase 擦除函数
 * @{
 */

/**
 * @brief 扇区擦除（4KB）
 * @details 使用 0x20 命令，擦除指定地址所在的 4KB 扇区
 *          擦除后所有位变为 1
 * @param hflash SPI Flash 句柄指针
 * @param addr 扇区起始地址（建议扇区对齐，即4KB对齐）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_SectorErase(SPI_Flash_HandleTypeDef *hflash, uint32_t addr);

/**
 * @brief 块擦除（32KB）
 * @details 使用 0x52 命令，擦除指定地址所在的 32KB 块
 * @param hflash SPI Flash 句柄指针
 * @param addr 块起始地址（建议32KB对齐）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_BlockErase32K(SPI_Flash_HandleTypeDef *hflash, uint32_t addr);

/**
 * @brief 块擦除（64KB）
 * @details 使用 0xD8 命令，擦除指定地址所在的 64KB 块
 * @param hflash SPI Flash 句柄指针
 * @param addr 块起始地址（建议64KB对齐）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_BlockErase64K(SPI_Flash_HandleTypeDef *hflash, uint32_t addr);

/**
 * @brief 全片擦除
 * @details 使用 0xC7 命令，擦除整个芯片
 *          擦除时间可能长达数分钟，需要等待操作完成
 * @param hflash SPI Flash 句柄指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_ChipErase(SPI_Flash_HandleTypeDef *hflash);

/**
 * @brief 按地址范围擦除
 * @details 根据地址范围自动选择最优的擦除方式
 *          自动处理扇区/块对齐问题
 * @param hflash SPI Flash 句柄指针
 * @param addr 起始地址
 * @param size 擦除大小（字节）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_EraseRange(SPI_Flash_HandleTypeDef *hflash, 
                                         uint32_t addr, 
                                         uint32_t size);

/** @} */

/**
 * @defgroup SPI_Flash_Protection 写保护和状态寄存器函数
 * @{
 */

/**
 * @brief 写使能
 * @details 发送 0x06 命令，使能写操作
 *          每次页编程、擦除、写状态寄存器操作后自动复位
 * @param hflash SPI Flash 句柄指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_WriteEnable(SPI_Flash_HandleTypeDef *hflash);

/**
 * @brief 写禁止
 * @details 发送 0x04 命令，禁止写操作
 * @param hflash SPI Flash 句柄指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_WriteDisable(SPI_Flash_HandleTypeDef *hflash);

/**
 * @brief 读取状态寄存器
 * @details 发送 0x05 命令，读取状态寄存器
 * @param hflash SPI Flash 句柄指针
 * @param status 用于存储状态寄存器值的指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_ReadStatusReg(SPI_Flash_HandleTypeDef *hflash, uint8_t *status);

/**
 * @brief 读取状态寄存器2
 * @details 发送 0x35 命令，读取状态寄存器2
 * @param hflash SPI Flash 句柄指针
 * @param status 用于存储状态寄存器2值的指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_ReadStatusReg2(SPI_Flash_HandleTypeDef *hflash, uint8_t *status);

/**
 * @brief 读取状态寄存器3
 * @details 发送 0x15 命令，读取状态寄存器3
 * @param hflash SPI Flash 句柄指针
 * @param status 用于存储状态寄存器3值的指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_ReadStatusReg3(SPI_Flash_HandleTypeDef *hflash, uint8_t *status);

/**
 * @brief 写状态寄存器
 * @details 发送 0x01 命令，写入状态寄存器
 *          需要先执行写使能命令
 * @param hflash SPI Flash 句柄指针
 * @param status 待写入的状态寄存器值
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_WriteStatusReg(SPI_Flash_HandleTypeDef *hflash, uint8_t status);

/**
 * @brief 写状态寄存器（支持多字节）
 * @details 写入多个状态寄存器，用于配置四线模式等
 * @param hflash SPI Flash 句柄指针
 * @param status 寄存器值数组指针
 * @param len 写入字节数（1-3）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_WriteStatusRegEx(SPI_Flash_HandleTypeDef *hflash, 
                                               const uint8_t *status, 
                                               uint8_t len);

/**
 * @brief 等待操作完成
 * @details 轮询状态寄存器的 BUSY 位，直到操作完成或超时
 * @param hflash SPI Flash 句柄指针
 * @param timeout 超时时间（毫秒）
 * @retval HAL_OK 成功
 * @retval HAL_TIMEOUT 超时
 */
HAL_StatusTypeDef SPI_Flash_WaitBusy(SPI_Flash_HandleTypeDef *hflash, uint32_t timeout);

/**
 * @brief 扇区保护
 * @details 设置指定扇区的写保护
 * @param hflash SPI Flash 句柄指针
 * @param addr 扇区地址
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_ProtectSector(SPI_Flash_HandleTypeDef *hflash, uint32_t addr);

/**
 * @brief 解除所有保护
 * @details 清除所有扇区的写保护
 * @param hflash SPI Flash 句柄指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_UnprotectAll(SPI_Flash_HandleTypeDef *hflash);

/** @} */

/**
 * @defgroup SPI_Flash_Power 电源管理函数
 * @{
 */

/**
 * @brief 进入掉电模式
 * @details 发送 0xB9 命令，进入深度掉电模式
 *          功耗可降至微安级别
 *          需要 ReleasePowerDown 唤醒
 * @param hflash SPI Flash 句柄指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_PowerDown(SPI_Flash_HandleTypeDef *hflash);

/**
 * @brief 释放掉电模式
 * @details 发送 0xAB 命令，从掉电模式唤醒
 *          唤醒后需要等待一定时间才能正常操作
 * @param hflash SPI Flash 句柄指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_ReleasePowerDown(SPI_Flash_HandleTypeDef *hflash);

/** @} */

/**
 * @defgroup SPI_Flash_Reset 复位函数
 * @{
 */

/**
 * @brief 软件复位
 * @details 发送 0x66+0x99 命令序列，执行软件复位
 *          复位后需要等待一定时间才能正常操作
 * @param hflash SPI Flash 句柄指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_Reset(SPI_Flash_HandleTypeDef *hflash);

/** @} */

/**
 * @defgroup SPI_Flash_Config 配置函数
 * @{
 */

/**
 * @brief 设置 SPI 通信速度
 * @details 动态调整 SPI 时钟频率
 * @param hflash SPI Flash 句柄指针
 * @param speed 目标速度 (Hz)
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_SetSpeed(SPI_Flash_HandleTypeDef *hflash, uint32_t speed);

/**
 * @brief 设置 SPI 模式
 * @details 设置 SPI 模式（Mode 0 或 Mode 3）
 * @param hflash SPI Flash 句柄指针
 * @param mode SPI 模式（0 或 3）
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_SetMode(SPI_Flash_HandleTypeDef *hflash, uint8_t mode);

/**
 * @brief 使能四线模式
 * @details 配置状态寄存器使能四线输出模式
 * @param hflash SPI Flash 句柄指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_EnableQuadMode(SPI_Flash_HandleTypeDef *hflash);

/**
 * @brief 进入4字节地址模式
 * @details 对于大于128Mb（16MB）的芯片，进入4字节地址模式
 * @param hflash SPI Flash 句柄指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_Enter4ByteMode(SPI_Flash_HandleTypeDef *hflash);

/**
 * @brief 退出4字节地址模式
 * @details 退出4字节地址模式，恢复3字节地址模式
 * @param hflash SPI Flash 句柄指针
 * @retval HAL_OK 成功
 * @retval HAL_ERROR 失败
 */
HAL_StatusTypeDef SPI_Flash_Exit4ByteMode(SPI_Flash_HandleTypeDef *hflash);

/** @} */

/**
 * @defgroup SPI_Flash_Utility 工具函数
 * @{
 */

/**
 * @brief 获取芯片容量字符串
 * @details 将容量转换为可读的字符串（如 "16 MB"）
 * @param capacity 容量值（字节）
 * @param buf 输出缓冲区
 * @param buf_size 缓冲区大小
 */
void SPI_Flash_GetCapacityString(uint32_t capacity, char *buf, uint32_t buf_size);

/**
 * @brief 查找芯片信息
 * @details 根据JEDEC ID在芯片信息表中查找
 * @param jedec_id JEDEC ID
 * @return 芯片信息指针，未找到返回NULL
 */
const SPI_Flash_ChipInfo_t* SPI_Flash_FindChipInfo(uint32_t jedec_id);

/**
 * @brief 计算地址所在的扇区号
 * @param addr 地址
 * @param sector_size 扇区大小
 * @return 扇区号
 */
uint32_t SPI_Flash_AddrToSector(uint32_t addr, uint32_t sector_size);

/**
 * @brief 计算扇区起始地址
 * @param sector 扇区号
 * @param sector_size 扇区大小
 * @return 扇区起始地址
 */
uint32_t SPI_Flash_SectorToAddr(uint32_t sector, uint32_t sector_size);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SPI_FLASH_H__ */