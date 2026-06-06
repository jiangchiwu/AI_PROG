/**
 * @file chip_driver_framework.h
 * @brief 可扩展芯片驱动框架 - 支持百万级芯片
 * 
 * 设计目标:
 *   - 支持1,000,000种芯片
 *   - 插件式驱动架构
 *   - 自动ID识别
 *   - 模板化驱动生成
 *   - 分层分类体系
 * 
 * 架构:
 *   - 调试接口层 (SWD/JTAG/BDM/SBW等)
 *   - 驱动抽象层 (统一API)
 *   - 驱动模板层 (按系列生成)
 *   - ID匹配引擎 (自动识别)
 * 
 * 作者: AI_PROG项目
 * 日期: 2026-06-03
 * 版本: v2.0
 */

#ifndef __CHIP_DRIVER_FRAMEWORK_H
#define __CHIP_DRIVER_FRAMEWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ==================== 配置宏 ==================== */

#define CHIP_FRAMEWORK_VERSION      "2.0.0"
#define MAX_CHIP_COUNT              1000000    /* 目标芯片数量 */
#define MAX_VENDOR_NAME_LEN         32
#define MAX_FAMILY_NAME_LEN         32
#define MAX_SERIES_NAME_LEN         32
#define MAX_PART_NUMBER_LEN         48
#define MAX_DRIVER_COUNT            1000       /* 最大驱动数量 */
#define MAX_ID_MAP_ENTRIES          50000      /* 最大ID映射条目 */

/* ==================== 类型定义 ==================== */

/**
 * 厂商ID枚举 - 扩展至所有主流厂商
 */
typedef enum {
    /* 国际主流厂商 */
    VENDOR_ST              = 0x0020,   /* STMicroelectronics */
    VENDOR_NXP             = 0x0015,   /* NXP Semiconductors */
    VENDOR_TI              = 0x0017,   /* Texas Instruments */
    VENDOR_INFINEON        = 0x0005,   /* Infineon Technologies */
    VENDOR_RENESAS         = 0x0023,   /* Renesas Electronics */
    VENDOR_MICROCHIP       = 0x0003,   /* Microchip Technology */
    VENDOR_ANALOG_DEVICES  = 0x0041,   /* Analog Devices */
    VENDOR_SILICON_LABS    = 0x0053,   /* Silicon Labs */
    VENDOR_MAXIM           = 0x001C,   /* Maxim Integrated */
    VENDOR_ONSEMI          = 0x004F,   /* ON Semiconductor */
    VENDOR_CYPRESS         = 0x0004,   /* Cypress Semiconductor */
    VENDOR_NORDIC          = 0x004E,   /* Nordic Semiconductor */
    VENDOR_ZILOG           = 0x005A,   /* Zilog */
    VENDOR_ATMEL           = 0x0003,   /* Atmel (已并入Microchip) */
    
    /* 日本厂商 */
    VENDOR_FUJITSU         = 0x0006,   /* Fujitsu */
    VENDOR_TOSHIBA         = 0x0010,   /* Toshiba */
    VENDOR_ROHM            = 0x0012,   /* ROHM */
    VENDOR_EPSON           = 0x0013,   /* Epson */
    VENDOR_OKI             = 0x0014,   /* OKI */
    VENDOR_NEC             = 0x0016,   /* NEC */
    VENDOR_HITACHI         = 0x0018,   /* Hitachi */
    VENDOR_MITSUBISHI      = 0x0019,   /* Mitsubishi */
    VENDOR_SANYO           = 0x001A,   /* Sanyo */
    VENDOR_SHARP           = 0x001B,   /* Sharp */
    
    /* 韩国厂商 */
    VENDOR_SAMSUNG         = 0x0053,   /* Samsung */
    VENDOR_SK_HYNIX        = 0x0054,   /* SK Hynix */
    
    /* 中国大陆厂商 */
    VENDOR_GIGADEVICE      = 0x0048,   /* 兆易创新 */
    VENDOR_NATIONSTECH     = 0x0049,   /* 国民技术 */
    VENDOR_HDSC            = 0x004A,   /* 华大半导体 */
    VENDOR_HKMCU           = 0x004B,   /* 航顺芯片 */
    VENDOR_MINDMOTION      = 0x004C,   /* 灵动微电子 */
    VENDOR_GEEHY           = 0x004D,   /* 极海半导体 */
    VENDOR_ARTERY          = 0x004E,   /* 雅特力 */
    VENDOR_EASTSOFT        = 0x004F,   /* 东软载波 */
    VENDOR_WCH             = 0x0050,   /* 沁恒微电子 */
    VENDOR_SINOMCU         = 0x0051,   /* 赛元微电子 */
    VENDOR_CHIPONE         = 0x0052,   /* 中微爱芯 */
    VENDOR_FMD             = 0x0053,   /* 辉芒微 */
    VENDOR_FUDANMICRO      = 0x0054,   /* 复旦微电子 */
    VENDOR_BOUFFALO        = 0x0055,   /* 博流智能 */
    VENDOR_CW              = 0x0056,   /* 武汉芯源 */
    VENDOR_SYNWIT          = 0x0057,   /* 华芯微 */
    VENDOR_MEGAWIN         = 0x0058,   /* Megawin */
    VENDOR_ESPRESSIF       = 0x0059,   /* 乐鑫信息 */
    VENDOR_REALTEK         = 0x005B,   /* 瑞昱半导体 */
    VENDOR_ALLWINNER       = 0x005C,   /* 全志科技 */
    VENDOR_ROCKCHIP        = 0x005D,   /* 瑞芯微 */
    VENDOR_CSKY            = 0x005E,   /* 中天微 */
    VENDOR_GOWIN           = 0x005F,   /* 高云半导体 */
    VENDOR_THEAD           = 0x0060,   /* 平头哥 */
    VENDOR_NUCLEI          = 0x0061,   /* 芯来科技 */
    
    /* 台湾厂商 */
    VENDOR_MEDIATEK        = 0x0062,   /* MediaTek */
    VENDOR_ITE             = 0x0063,   /* ITE */
    VENDOR_GENESYS         = 0x0064,   /* Genesys Logic */
    VENDOR_ALI             = 0x0065,   /* ALi */
    VENDOR_WINBOND         = 0x0066,   /* Winbond */
    VENDOR_ETRON           = 0x0067,   /* Etron */
    VENDOR_FARADAY         = 0x0068,   /* Faraday */
    VENDOR_NUVOTON         = 0x0069,   /* Nuvoton */
    
    /* 其他厂商 */
    VENDOR_XILINX          = 0x0070,   /* Xilinx */
    VENDOR_INTEL_FPGA      = 0x0071,   /* Intel FPGA */
    VENDOR_LATTICE         = 0x0072,   /* Lattice */
    VENDOR_SIFIVE          = 0x0073,   /* SiFive */
    VENDOR_KENDRYTE        = 0x0074,   /* Kendryte */
    VENDOR_AMS             = 0x0075,   /* AMS */
    VENDOR_MELEXIS         = 0x0076,   /* Melexis */
    VENDOR_DIALOG          = 0x0077,   /* Dialog */
    VENDOR_XMOS            = 0x0078,   /* XMOS */
    VENDOR_QUICKLOGIC      = 0x0079,   /* QuickLogic */
    VENDOR_POWERINT        = 0x007A,   /* Power Integrations */
    VENDOR_MPS             = 0x007B,   /* Monolithic Power */
    VENDOR_LINEAR          = 0x007C,   /* Linear Technology */
    
    VENDOR_UNKNOWN         = 0xFFFF,   /* 未知厂商 */
    
    /* 厂商ID最大值 - 用于数组大小 */
    VENDOR_MAX             = 256,
} Chip_Vendor_ID_t;

/**
 * 内核架构枚举 - 扩展至所有主流内核
 */
typedef enum {
    /* ARM Cortex系列 */
    CORE_ARM_CORTEX_M0     = 0x1000,
    CORE_ARM_CORTEX_M0P    = 0x1001,   /* Cortex-M0+ */
    CORE_ARM_CORTEX_M1     = 0x1002,
    CORE_ARM_CORTEX_M3     = 0x1003,
    CORE_ARM_CORTEX_M4     = 0x1004,
    CORE_ARM_CORTEX_M4F    = 0x1005,   /* Cortex-M4F (带FPU) */
    CORE_ARM_CORTEX_M7     = 0x1006,
    CORE_ARM_CORTEX_M7F    = 0x1007,   /* Cortex-M7F (带FPU) */
    CORE_ARM_CORTEX_M23    = 0x1008,
    CORE_ARM_CORTEX_M33    = 0x1009,
    CORE_ARM_CORTEX_M35P   = 0x100A,
    CORE_ARM_CORTEX_M55    = 0x100B,
    CORE_ARM_CORTEX_M85    = 0x100C,
    CORE_ARM_CORTEX_A5     = 0x1100,
    CORE_ARM_CORTEX_A7     = 0x1101,
    CORE_ARM_CORTEX_A8     = 0x1102,
    CORE_ARM_CORTEX_A9     = 0x1103,
    CORE_ARM_CORTEX_A15    = 0x1104,
    CORE_ARM_CORTEX_A17    = 0x1105,
    CORE_ARM_CORTEX_A35    = 0x1106,
    CORE_ARM_CORTEX_A53    = 0x1107,
    CORE_ARM_CORTEX_A55    = 0x1108,
    CORE_ARM_CORTEX_A57    = 0x1109,
    CORE_ARM_CORTEX_A72    = 0x110A,
    CORE_ARM_CORTEX_A73    = 0x110B,
    CORE_ARM_CORTEX_A75    = 0x110C,
    CORE_ARM_CORTEX_A76    = 0x110D,
    CORE_ARM_CORTEX_R4     = 0x1200,
    CORE_ARM_CORTEX_R5     = 0x1201,
    CORE_ARM_CORTEX_R7     = 0x1202,
    CORE_ARM_CORTEX_R8     = 0x1203,
    
    /* RISC-V系列 */
    CORE_RISCV_RV32I       = 0x2000,
    CORE_RISCV_RV32IM      = 0x2001,
    CORE_RISCV_RV32IMC     = 0x2002,
    CORE_RISCV_RV32IMA     = 0x2003,
    CORE_RISCV_RV32IMAC    = 0x2004,
    CORE_RISCV_RV32IMAF    = 0x2005,
    CORE_RISCV_RV32GC      = 0x2006,
    CORE_RISCV_RV64I       = 0x2007,
    CORE_RISCV_RV64IM      = 0x2008,
    CORE_RISCV_RV64GC      = 0x2009,
    CORE_RISCV_NULCLEI_N   = 0x2010,
    CORE_RISCV_SIFIVE_E    = 0x2011,
    CORE_RISCV_SIFIVE_U    = 0x2012,
    CORE_RISCV_CK802       = 0x2013,   /* C-SKY CK802 */
    CORE_RISCV_CK803       = 0x2014,   /* C-SKY CK803 */
    CORE_RISCV_CK804       = 0x2015,   /* C-SKY CK804 */
    
    /* MIPS系列 */
    CORE_MIPS_M4K          = 0x3000,
    CORE_MIPS_M14K         = 0x3001,
    CORE_MIPS_MICROAPTIV   = 0x3002,
    CORE_MIPS_INTERAPTIV   = 0x3003,
    CORE_MIPS_PROAPTIV     = 0x3004,
    CORE_MIPS_I7200        = 0x3005,
    CORE_MIPS_P5600        = 0x3006,
    
    /* 8位内核 */
    CORE_8051_CLASSIC      = 0x4000,
    CORE_8051_1T           = 0x4001,   /* 1T增强型8051 */
    CORE_8051_2T           = 0x4002,   /* 2T增强型8051 */
    CORE_8051_4T           = 0x4003,   /* 4T增强型8051 */
    CORE_8051_E8051        = 0x4004,   /* WCH E8051 */
    CORE_PIC_CORE          = 0x4010,
    CORE_PIC14             = 0x4011,
    CORE_PIC16             = 0x4012,
    CORE_PIC18             = 0x4013,
    CORE_AVR               = 0x4020,
    CORE_AVR_XMEGA         = 0x4021,
    CORE_ST7               = 0x4030,
    CORE_STM8              = 0x4031,
    CORE_HC08              = 0x4040,
    CORE_HCS08             = 0x4041,
    CORE_HC05              = 0x4042,
    
    /* 16位内核 */
    CORE_HCS12             = 0x5000,
    CORE_HCS12X            = 0x5001,
    CORE_MSP430            = 0x5010,
    CORE_MSP430X           = 0x5011,
    CORE_PIC24             = 0x5020,
    CORE_DSPIC             = 0x5021,   /* dsPIC DSC */
    CORE_78K0R             = 0x5030,
    CORE_RL78              = 0x5031,
    CORE_XA                = 0x5040,
    
    /* 32位专用内核 */
    CORE_TRICORE_TC1       = 0x6000,
    CORE_TRICORE_TC1P      = 0x6001,
    CORE_TRICORE_TC2       = 0x6002,
    CORE_TRICORE_TC2P      = 0x6003,
    CORE_TRICORE_TC3       = 0x6004,
    CORE_V850              = 0x6010,
    CORE_V850ES            = 0x6011,
    CORE_V850E             = 0x6012,
    CORE_V850E2            = 0x6013,
    CORE_RH850             = 0x6014,
    CORE_RX_V1             = 0x6020,
    CORE_RX_V2             = 0x6021,
    CORE_RX_V3             = 0x6022,
    CORE_SH2               = 0x6030,
    CORE_SH2A              = 0x6031,
    CORE_SH3               = 0x6032,
    CORE_SH4               = 0x6033,
    CORE_C28X              = 0x6040,   /* TI C28x DSP */
    CORE_C55X              = 0x6041,   /* TI C55x DSP */
    CORE_C64X              = 0x6042,   /* TI C64x DSP */
    CORE_C66X              = 0x6043,   /* TI C66x DSP */
    CORE_COLDFIRE          = 0x6050,
    CORE_DSP56K            = 0x6060,
    CORE_POWERPC_E200      = 0x6070,
    CORE_POWERPC_E300      = 0x6071,
    CORE_POWERPC_E500      = 0x6072,
    CORE_POWERPC_E5500     = 0x6073,
    CORE_POWERPC_NXP       = 0x6074,
    
    /* 其他内核 */
    CORE_X86               = 0x7000,
    CORE_X64               = 0x7001,
    CORE_DSPIC33           = 0x8000,
    CORE_STM32WBA          = 0x8001,
    
    CORE_UNKNOWN           = 0xFFFF,
} Chip_Core_Type_t;

/**
 * 调试接口类型枚举
 */
typedef enum {
    DEBUG_IF_SWD           = 0x01,     /* Serial Wire Debug */
    DEBUG_IF_JTAG          = 0x02,     /* JTAG */
    DEBUG_IF_BDM           = 0x03,     /* Background Debug Mode */
    DEBUG_IF_MON8          = 0x04,     /* Monitor Mode 8-bit */
    DEBUG_IF_SBW           = 0x05,     /* Spy-Bi-Wire */
    DEBUG_IF_FINE          = 0x06,     /* Renesas FINE */
    DEBUG_IF_ICSP          = 0x07,     /* In-Circuit Serial Programming */
    DEBUG_IF_ISP           = 0x08,     /* In-System Programming */
    DEBUG_IF_USB           = 0x09,     /* USB Programming */
    DEBUG_IF_UART          = 0x0A,     /* UART Bootloader */
    DEBUG_IF_SPI           = 0x0B,     /* SPI Flash */
    DEBUG_IF_I2C           = 0x0C,     /* I2C EEPROM */
    DEBUG_IF_CAN           = 0x0D,     /* CAN Bus */
    DEBUG_IF_DAP           = 0x0E,     /* Debug Access Port */
    DEBUG_IF_SWD_JTAG      = 0x0F,     /* SWD or JTAG */
    
    DEBUG_IF_UNKNOWN       = 0xFF,
} Chip_Debug_Interface_t;

/**
 * Flash擦除类型
 */
typedef enum {
    ERASE_TYPE_CHIP        = 0x00,     /* 全片擦除 */
    ERASE_TYPE_SECTOR      = 0x01,     /* 扇区擦除 */
    ERASE_TYPE_PAGE        = 0x02,     /* 页擦除 */
    ERASE_TYPE_BLOCK       = 0x03,     /* 块擦除 */
} Chip_Erase_Type_t;

/**
 * 芯片状态
 */
typedef enum {
    CHIP_STATUS_ACTIVE     = 0x00,     /* 活跃/量产中 */
    CHIP_STATUS_EOL        = 0x01,     /* 停产 */
    CHIP_STATUS_PREVIEW    = 0x02,     /* 预览/测试版 */
    CHIP_STATUS_NRE        = 0x03,     /* 不推荐新设计 */
    CHIP_STATUS_UNKNOWN    = 0xFF,
} Chip_Status_t;

/**
 * 芯片等级
 */
typedef enum {
    CHIP_GRADE_COMMERCIAL  = 0x00,     /* 商业级 (-40~85°C) */
    CHIP_GRADE_INDUSTRIAL  = 0x01,     /* 工业级 (-40~105°C) */
    CHIP_GRADE_AUTOMOTIVE  = 0x02,     /* 汽车级 (-40~125°C) */
    CHIP_GRADE_EXTENDED    = 0x03,     /* 扩展级 (-40~150°C) */
} Chip_Grade_t;

/* ==================== 数据结构 ==================== */

/**
 * 厂商信息结构
 */
typedef struct {
    Chip_Vendor_ID_t id;                       /* 厂商ID */
    const char* name;                          /* 厂商名称 */
    const char* short_name;                    /* 简称 */
    const char* country;                       /* 国家 */
    const char* website;                       /* 网站 */
} Chip_Vendor_Info_t;

/**
 * 系列信息结构
 */
typedef struct {
    Chip_Vendor_ID_t vendor_id;                /* 厂商ID */
    Chip_Core_Type_t core_type;                /* 内核类型 */
    const char* name;                          /* 系列名称 */
    const char* description;                   /* 描述 */
} Chip_Family_Info_t;

/**
 * 芯片详细信息结构 - 核心数据结构
 */
typedef struct {
    /* 基本信息 */
    uint32_t chip_id;                          /* 内部芯片ID */
    Chip_Vendor_ID_t vendor_id;                /* 厂商ID */
    Chip_Core_Type_t core_type;                /* 内核类型 */
    const char* family_name;                   /* 系列名称 */
    const char* series_name;                   /* 子系列名称 */
    const char* part_number;                   /* 完整型号 */
    const char* full_name;                     /* 全名 */
    
    /* 存储信息 */
    uint32_t flash_size;                       /* Flash大小 (字节) */
    uint32_t ram_size;                         /* RAM大小 (字节) */
    uint32_t eeprom_size;                      /* EEPROM大小 (字节) */
    uint32_t flash_sector_size;                /* Flash扇区大小 */
    uint32_t flash_page_size;                  /* Flash页大小 */
    
    /* 封装信息 */
    const char* package_type;                  /* 封装类型 */
    uint16_t pin_count;                        /* 引脚数 */
    
    /* 工作参数 */
    uint32_t max_freq_hz;                      /* 最大频率 (Hz) */
    uint16_t voltage_min_mv;                   /* 最小电压 (mV) */
    uint16_t voltage_max_mv;                   /* 最大电压 (mV) */
    int16_t temp_min_c;                        /* 最低温度 (°C) */
    int16_t temp_max_c;                        /* 最高温度 (°C) */
    Chip_Grade_t grade;                        /* 芯片等级 */
    
    /* 状态信息 */
    Chip_Status_t status;                      /* 芯片状态 */
    
    /* ID识别信息 */
    uint32_t jtag_id;                          /* JTAG ID */
    uint32_t flash_id;                         /* Flash ID */
    uint32_t device_id;                        /* Device ID */
    uint32_t signature[3];                     /* 签名字节 (AVR等) */
    
    /* 调试接口支持 */
    Chip_Debug_Interface_t primary_debug;      /* 主调试接口 */
    uint8_t supported_debug_count;             /* 支持的调试接口数量 */
    Chip_Debug_Interface_t supported_debug[4]; /* 支持的调试接口列表 */
    
    /* 文档链接 */
    const char* datasheet_url;                 /* 数据手册URL */
    const char* reference_manual_url;          /* 参考手册URL */
} Chip_Info_t;

/**
 * 驱动操作函数结构 - 统一API
 */
typedef struct {
    /* 初始化 */
    bool (*Init)(const Chip_Info_t* chip, void* config);
    bool (*Close)(void);
    
    /* 检测 */
    bool (*Detect)(Chip_Info_t* chip);
    bool (*ReadID)(uint32_t* id);
    bool (*GetInfo)(Chip_Info_t* chip);
    
    /* Flash操作 */
    bool (*Erase)(uint32_t addr, uint32_t size, Chip_Erase_Type_t type);
    bool (*Write)(uint32_t addr, const uint8_t* data, uint32_t size);
    bool (*Read)(uint32_t addr, uint8_t* data, uint32_t size);
    bool (*Verify)(uint32_t addr, const uint8_t* data, uint32_t size);
    
    /* 内存操作 */
    bool (*MemWrite)(uint32_t addr, const uint8_t* data, uint32_t size);
    bool (*MemRead)(uint32_t addr, uint8_t* data, uint32_t size);
    
    /* 寄存器操作 */
    bool (*RegWrite)(uint32_t addr, uint32_t value);
    bool (*RegRead)(uint32_t addr, uint32_t* value);
    
    /* 复位 */
    bool (*Reset)(void);
    bool (*Halt)(void);
    bool (*Run)(void);
    
    /* 特殊功能 */
    bool (*OptionBytesWrite)(const uint8_t* data, uint32_t size);
    bool (*OptionBytesRead)(uint8_t* data, uint32_t size);
    bool (*SecurityUnlock)(const uint8_t* key, uint32_t size);
    bool (*SecurityLock)(void);
    
    /* 驱动信息 */
    const char* driver_name;
    const char* driver_version;
    uint32_t supported_chip_count;
} Chip_Driver_Ops_t;

/**
 * 驱动注册结构
 */
typedef struct {
    uint32_t driver_id;                        /* 驱动ID */
    const char* driver_name;                   /* 驱动名称 */
    Chip_Core_Type_t core_type;                /* 支持的内核 */
    Chip_Debug_Interface_t debug_interface;    /* 调试接口 */
    const Chip_Driver_Ops_t* ops;              /* 操作函数 */
    uint32_t chip_count;                       /* 支持的芯片数量 */
    const uint32_t* supported_chips;           /* 支持的芯片ID列表 */
} Chip_Driver_Entry_t;

/**
 * ID映射条目结构
 */
typedef struct {
    uint32_t chip_id;                          /* 芯片内部ID */
    uint32_t id_value;                         /* ID值 */
    uint32_t id_mask;                          /* ID掩码 */
    Chip_Debug_Interface_t detection_method;   /* 检测方法 */
    uint8_t priority;                          /* 优先级 */
} Chip_ID_Map_Entry_t;

/* ==================== 外部变量 ==================== */

/* 厂商信息表 */
extern const Chip_Vendor_Info_t Chip_Vendors_Table[];
extern const uint32_t Chip_Vendors_Count;

/* 驱动注册表 */
extern const Chip_Driver_Entry_t Chip_Drivers_Table[];
extern const uint32_t Chip_Drivers_Count;

/* ID映射表 */
extern const Chip_ID_Map_Entry_t Chip_ID_Map_Table[];
extern const uint32_t Chip_ID_Map_Count;

/* ==================== API函数 ==================== */

/**
 * 初始化驱动框架
 */
bool Chip_Framework_Init(void);

/**
 * 关闭驱动框架
 */
bool Chip_Framework_Close(void);

/**
 * 获取厂商信息
 */
const Chip_Vendor_Info_t* Chip_GetVendorInfo(Chip_Vendor_ID_t vendor_id);

/**
 * 根据厂商名称获取厂商ID
 */
Chip_Vendor_ID_t Chip_GetVendorIDByName(const char* name);

/**
 * 获取芯片信息
 */
const Chip_Info_t* Chip_GetChipInfo(uint32_t chip_id);

/**
 * 根据型号获取芯片信息
 */
const Chip_Info_t* Chip_GetChipByPartNumber(const char* part_number);

/**
 * 根据ID值识别芯片
 */
const Chip_Info_t* Chip_IdentifyByID(uint32_t id_value, 
                                      Chip_Debug_Interface_t detection_method);

/**
 * 搜索芯片
 */
uint32_t Chip_Search(const char* query, Chip_Info_t* results, uint32_t max_results);

/**
 * 匹配驱动
 */
const Chip_Driver_Ops_t* Chip_MatchDriver(const Chip_Info_t* chip);

/**
 * 注册驱动
 */
bool Chip_RegisterDriver(const Chip_Driver_Entry_t* driver);

/**
 * 获取统计信息
 */
uint32_t Chip_GetTotalChipCount(void);
uint32_t Chip_GetTotalDriverCount(void);
uint32_t Chip_GetTotalVendorCount(void);

/**
 * 打印芯片信息
 */
void Chip_PrintInfo(const Chip_Info_t* chip);

/**
 * 获取调试接口名称
 */
const char* Chip_GetDebugInterfaceName(Chip_Debug_Interface_t debug_if);

/**
 * 获取内核类型名称
 */
const char* Chip_GetCoreTypeName(Chip_Core_Type_t core);

/**
 * 获取厂商名称
 */
const char* Chip_GetVendorName(Chip_Vendor_ID_t vendor_id);

/* ==================== 调试接口抽象层 ==================== */

/**
 * 调试接口操作结构
 */
typedef struct {
    bool (*Init)(uint32_t speed_hz);
    bool (*Close)(void);
    
    bool (*Connect)(void);
    bool (*Disconnect)(void);
    
    bool (*ReadID)(uint32_t* id);
    bool (*GetCapabilities)(uint32_t* caps);
    
    bool (*MemWrite)(uint32_t addr, const uint8_t* data, uint32_t size);
    bool (*MemRead)(uint32_t addr, uint8_t* data, uint32_t size);
    
    bool (*RegWrite)(uint32_t addr, uint32_t value);
    bool (*RegRead)(uint32_t addr, uint32_t* value);
    
    bool (*Reset)(void);
    bool (*Halt)(void);
    bool (*Run)(void);
    bool (*Step)(void);
    
    bool (*SetSpeed)(uint32_t speed_hz);
    uint32_t (*GetSpeed)(void);
    
    const char* name;
    Chip_Debug_Interface_t type;
} Chip_Debug_Interface_Ops_t;

/**
 * 创建调试接口
 */
const Chip_Debug_Interface_Ops_t* Chip_CreateDebugInterface(Chip_Debug_Interface_t type);

/* ==================== 驱动模板宏 ==================== */

/**
 * 定义驱动模板宏 - 用于简化驱动创建
 */
#define CHIP_DRIVER_DECLARE(name, version, core, debug, ops) \
    static const Chip_Driver_Entry_t Chip_Driver_##name = { \
        .driver_id = CHIP_DRIVER_ID_##name, \
        .driver_name = #name, \
        .core_type = core, \
        .debug_interface = debug, \
        .ops = ops, \
        .chip_count = 0, \
        .supported_chips = NULL, \
    };

#define CHIP_DRIVER_REGISTER(name) \
    Chip_RegisterDriver(&Chip_Driver_##name);

/* ==================== 预定义驱动ID ==================== */

#define CHIP_DRIVER_ID_BASE          0x0000

/* ARM Cortex-M系列驱动 */
#define CHIP_DRIVER_ID_ARM_CORTEX_M  0x0001
#define CHIP_DRIVER_ID_STM32         0x0002
#define CHIP_DRIVER_ID_GD32          0x0003
#define CHIP_DRIVER_ID_NXP_LPC       0x0004
#define CHIP_DRIVER_ID_NXP_S32K      0x0005
#define CHIP_DRIVER_ID_NXP_KINETIS   0x0006
#define CHIP_DRIVER_ID_SILICON_LABS  0x0007
#define CHIP_DRIVER_ID_NUVOTON_ARM   0x0008
#define CHIP_DRIVER_ID_ATMEL_SAM     0x0009

/* 8位单片机驱动 */
#define CHIP_DRIVER_ID_PIC           0x0100
#define CHIP_DRIVER_ID_AVR           0x0101
#define CHIP_DRIVER_ID_8051          0x0102
#define CHIP_DRIVER_ID_STC           0x0103
#define CHIP_DRIVER_ID_NUVOTON_51    0x0104
#define CHIP_DRIVER_ID_WCH_51        0x0105

/* 16位单片机驱动 */
#define CHIP_DRIVER_ID_HCS12         0x0200
#define CHIP_DRIVER_ID_HCS08         0x0201
#define CHIP_DRIVER_ID_MSP430        0x0202
#define CHIP_DRIVER_ID_DSPIC         0x0203
#define CHIP_DRIVER_ID_78K0R         0x0204
#define CHIP_DRIVER_ID_RL78          0x0205

/* 32位专用内核驱动 */
#define CHIP_DRIVER_ID_TRICORE       0x0300
#define CHIP_DRIVER_ID_V850          0x0301
#define CHIP_DRIVER_ID_RH850         0x0302
#define CHIP_DRIVER_ID_RX            0x0303
#define CHIP_DRIVER_ID_C28X          0x0304
#define CHIP_DRIVER_ID_DSP56K        0x0305
#define CHIP_DRIVER_ID_POWERPC       0x0306

/* RISC-V驱动 */
#define CHIP_DRIVER_ID_RISCV         0x0400
#define CHIP_DRIVER_ID_CH32V         0x0401
#define CHIP_DRIVER_ID_BL602         0x0402
#define CHIP_DRIVER_ID_K210          0x0403
#define CHIP_DRIVER_ID_GD32VF103     0x0404

/* 其他驱动 */
#define CHIP_DRIVER_ID_X86           0x0500
#define CHIP_DRIVER_ID_CUSTOM        0x0600

#endif /* __CHIP_DRIVER_FRAMEWORK_H */