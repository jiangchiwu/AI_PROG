/**
 * @file chip_driver_framework.c
 * @brief 可扩展芯片驱动框架实现
 * 
 * 本文件实现了芯片驱动框架的核心功能，包括：
 *   - 厂商信息管理
 *   - 芯片信息查询
 *   - 驱动注册与匹配
 *   - ID识别引擎
 *   - 调试接口抽象
 * 
 * 作者: AI_PROG项目
 * 日期: 2026-06-05
 * 版本: v2.0
 */

#include "chip_driver_framework.h"
#include <string.h>
#include <stdio.h>

/* ==================== 内部宏定义 ==================== */

/* 最大动态注册驱动数量 */
#define MAX_DYNAMIC_DRIVERS     100

/* 最大动态芯片数量 */
#define MAX_DYNAMIC_CHIPS       1000

/* 最大ID映射条目数量 */
#define MAX_DYNAMIC_ID_MAPS     500

/* 字符串比较宏 - 大小写不敏感 */
#define STR_MATCH_IGNORE_CASE(s1, s2) (strcmpi(s1, s2) == 0)

/* ==================== 内部数据结构 ==================== */

/**
 * 框架状态结构
 */
typedef struct {
    bool initialized;                          /* 是否已初始化 */
    uint32_t dynamic_driver_count;             /* 动态注册的驱动数量 */
    uint32_t dynamic_chip_count;               /* 动态添加的芯片数量 */
    uint32_t dynamic_id_map_count;             /* 动态添加的ID映射数量 */
} Framework_State_t;

/* ==================== 静态数据表 ==================== */

/**
 * 厂商信息表 - 示例数据
 * 包含主流芯片厂商的基本信息
 */
static const Chip_Vendor_Info_t s_Vendor_Table[] = {
    /* 国际主流厂商 */
    { VENDOR_ST,              "STMicroelectronics",   "ST",     "瑞士",     "www.st.com" },
    { VENDOR_NXP,             "NXP Semiconductors",   "NXP",    "荷兰",     "www.nxp.com" },
    { VENDOR_TI,              "Texas Instruments",    "TI",     "美国",     "www.ti.com" },
    { VENDOR_INFINEON,        "Infineon Technologies","Infineon","德国",    "www.infineon.com" },
    { VENDOR_RENESAS,         "Renesas Electronics",  "Renesas","日本",     "www.renesas.com" },
    { VENDOR_MICROCHIP,       "Microchip Technology", "Microchip","美国",   "www.microchip.com" },
    { VENDOR_ANALOG_DEVICES,  "Analog Devices",       "ADI",    "美国",     "www.analog.com" },
    { VENDOR_SILICON_LABS,    "Silicon Labs",         "Silabs", "美国",     "www.silabs.com" },
    { VENDOR_MAXIM,           "Maxim Integrated",     "Maxim",  "美国",     "www.maximintegrated.com" },
    { VENDOR_ONSEMI,          "ON Semiconductor",     "ONsemi", "美国",     "www.onsemi.com" },
    { VENDOR_CYPRESS,         "Cypress Semiconductor","Cypress","美国",     "www.cypress.com" },
    { VENDOR_NORDIC,          "Nordic Semiconductor", "Nordic", "挪威",     "www.nordicsemi.com" },
    
    /* 中国大陆厂商 */
    { VENDOR_GIGADEVICE,      "GigaDevice",           "GD",     "中国",     "www.gigadevice.com" },
    { VENDOR_NATIONSTECH,     "Nationstech",          "Nations","中国",     "www.nationstech.com" },
    { VENDOR_HDSC,            "HDSC",                 "HDSC",   "中国",     "www.hdsc.com.cn" },
    { VENDOR_MINDMOTION,      "MindMotion",           "MM",     "中国",     "www.mindmotion.com.cn" },
    { VENDOR_GEEHY,           "Geehy Semiconductor",  "Geehy",  "中国",     "www.geehy.com" },
    { VENDOR_ARTERY,          "ArteryTek",            "AT",     "中国",     "www.arterytek.com" },
    { VENDOR_WCH,             "WCH",                  "WCH",    "中国",     "www.wch.cn" },
    { VENDOR_ESPRESSIF,       "Espressif Systems",    "ESP",    "中国",     "www.espressif.com" },
    { VENDOR_REALTEK,         "Realtek Semiconductor","Realtek","中国台湾", "www.realtek.com" },
    { VENDOR_NUVOTON,         "Nuvoton Technology",   "Nuvoton","中国台湾", "www.nuvoton.com" },
    
    /* 其他厂商 */
    { VENDOR_XILINX,          "Xilinx",               "Xilinx", "美国",     "www.xilinx.com" },
    { VENDOR_LATTICE,         "Lattice Semiconductor","Lattice","美国",    "www.latticesemi.com" },
    { VENDOR_SIFIVE,          "SiFive",               "SiFive", "美国",     "www.sifive.com" },
    { VENDOR_KENDRYTE,        "Kendryte",             "Kendryte","中国",    "www.kendryte.com" },
    
    /* 结束标记 */
    { VENDOR_UNKNOWN,         NULL,                   NULL,     NULL,       NULL }
};

/* 厂商数量 */
static const uint32_t s_Vendor_Count = sizeof(s_Vendor_Table) / sizeof(s_Vendor_Table[0]) - 1;

/**
 * 示例芯片信息表
 * 包含部分常见芯片的详细信息
 */
static const Chip_Info_t s_Chip_Table[] = {
    /* STM32F103C8T6 - 经典ARM Cortex-M3芯片 */
    {
        .chip_id = 0x00010001,
        .vendor_id = VENDOR_ST,
        .core_type = CORE_ARM_CORTEX_M3,
        .family_name = "STM32F1",
        .series_name = "STM32F103",
        .part_number = "STM32F103C8T6",
        .full_name = "STM32F103C8T6 - Mainstream MCU",
        .flash_size = 64 * 1024,          /* 64KB Flash */
        .ram_size = 20 * 1024,            /* 20KB RAM */
        .eeprom_size = 0,
        .flash_sector_size = 1024,        /* 1KB扇区 */
        .flash_page_size = 2048,          /* 2KB页 */
        .package_type = "LQFP48",
        .pin_count = 48,
        .max_freq_hz = 72000000,          /* 72MHz */
        .voltage_min_mv = 2000,           /* 2.0V */
        .voltage_max_mv = 3600,           /* 3.6V */
        .temp_min_c = -40,
        .temp_max_c = 85,
        .grade = CHIP_GRADE_COMMERCIAL,
        .status = CHIP_STATUS_ACTIVE,
        .jtag_id = 0x4BA00477,
        .flash_id = 0,
        .device_id = 0x410,
        .signature = {0, 0, 0},
        .primary_debug = DEBUG_IF_SWD,
        .supported_debug_count = 2,
        .supported_debug = { DEBUG_IF_SWD, DEBUG_IF_JTAG, DEBUG_IF_UNKNOWN, DEBUG_IF_UNKNOWN },
        .datasheet_url = "www.st.com/stm32f103",
        .reference_manual_url = "www.st.com/rm0008"
    },
    
    /* GD32F103C8T6 - 兆易创新兼容芯片 */
    {
        .chip_id = 0x00020001,
        .vendor_id = VENDOR_GIGADEVICE,
        .core_type = CORE_ARM_CORTEX_M3,
        .family_name = "GD32F1",
        .series_name = "GD32F103",
        .part_number = "GD32F103C8T6",
        .full_name = "GD32F103C8T6 - High Performance MCU",
        .flash_size = 64 * 1024,
        .ram_size = 20 * 1024,
        .eeprom_size = 0,
        .flash_sector_size = 1024,
        .flash_page_size = 2048,
        .package_type = "LQFP48",
        .pin_count = 48,
        .max_freq_hz = 108000000,         /* 108MHz */
        .voltage_min_mv = 2700,
        .voltage_max_mv = 3600,
        .temp_min_c = -40,
        .temp_max_c = 85,
        .grade = CHIP_GRADE_INDUSTRIAL,
        .status = CHIP_STATUS_ACTIVE,
        .jtag_id = 0x1BA01477,
        .flash_id = 0,
        .device_id = 0x410,
        .signature = {0, 0, 0},
        .primary_debug = DEBUG_IF_SWD,
        .supported_debug_count = 2,
        .supported_debug = { DEBUG_IF_SWD, DEBUG_IF_JTAG, DEBUG_IF_UNKNOWN, DEBUG_IF_UNKNOWN },
        .datasheet_url = "www.gigadevice.com/gd32f103",
        .reference_manual_url = NULL
    },
    
    /* CH32V103C8T6 - 沁恒RISC-V芯片 */
    {
        .chip_id = 0x00030001,
        .vendor_id = VENDOR_WCH,
        .core_type = CORE_RISCV_RV32IMAC,
        .family_name = "CH32V1",
        .series_name = "CH32V103",
        .part_number = "CH32V103C8T6",
        .full_name = "CH32V103C8T6 - RISC-V MCU",
        .flash_size = 64 * 1024,
        .ram_size = 20 * 1024,
        .eeprom_size = 0,
        .flash_sector_size = 256,
        .flash_page_size = 256,
        .package_type = "LQFP48",
        .pin_count = 48,
        .max_freq_hz = 80000000,          /* 80MHz */
        .voltage_min_mv = 2700,
        .voltage_max_mv = 3600,
        .temp_min_c = -40,
        .temp_max_c = 85,
        .grade = CHIP_GRADE_INDUSTRIAL,
        .status = CHIP_STATUS_ACTIVE,
        .jtag_id = 0,
        .flash_id = 0,
        .device_id = 0x103,
        .signature = {0, 0, 0},
        .primary_debug = DEBUG_IF_SWD,
        .supported_debug_count = 1,
        .supported_debug = { DEBUG_IF_SWD, DEBUG_IF_UNKNOWN, DEBUG_IF_UNKNOWN, DEBUG_IF_UNKNOWN },
        .datasheet_url = "www.wch.cn/ch32v103",
        .reference_manual_url = NULL
    },
    
    /* ESP32-C3 - 乐鑫RISC-V芯片 */
    {
        .chip_id = 0x00040001,
        .vendor_id = VENDOR_ESPRESSIF,
        .core_type = CORE_RISCV_RV32IMC,
        .family_name = "ESP32-C3",
        .series_name = "ESP32-C3",
        .part_number = "ESP32-C3-WROOM-02",
        .full_name = "ESP32-C3 - WiFi + BLE RISC-V SoC",
        .flash_size = 4 * 1024 * 1024,     /* 外部Flash 4MB */
        .ram_size = 400 * 1024,            /* 400KB SRAM */
        .eeprom_size = 0,
        .flash_sector_size = 4096,
        .flash_page_size = 256,
        .package_type = "QFN32",
        .pin_count = 32,
        .max_freq_hz = 160000000,         /* 160MHz */
        .voltage_min_mv = 3000,
        .voltage_max_mv = 3600,
        .temp_min_c = -40,
        .temp_max_c = 85,
        .grade = CHIP_GRADE_INDUSTRIAL,
        .status = CHIP_STATUS_ACTIVE,
        .jtag_id = 0,
        .flash_id = 0,
        .device_id = 0xC3,
        .signature = {0, 0, 0},
        .primary_debug = DEBUG_IF_JTAG,
        .supported_debug_count = 2,
        .supported_debug = { DEBUG_IF_JTAG, DEBUG_IF_UART, DEBUG_IF_UNKNOWN, DEBUG_IF_UNKNOWN },
        .datasheet_url = "www.espressif.com/esp32-c3",
        .reference_manual_url = NULL
    },
    
    /* ATmega328P - 经典AVR芯片 */
    {
        .chip_id = 0x00050001,
        .vendor_id = VENDOR_MICROCHIP,
        .core_type = CORE_AVR,
        .family_name = "ATmega",
        .series_name = "ATmega328P",
        .part_number = "ATmega328P-PU",
        .full_name = "ATmega328P - 8-bit AVR MCU",
        .flash_size = 32 * 1024,           /* 32KB Flash */
        .ram_size = 2 * 1024,              /* 2KB SRAM */
        .eeprom_size = 1024,               /* 1KB EEPROM */
        .flash_sector_size = 128,
        .flash_page_size = 128,
        .package_type = "DIP28",
        .pin_count = 28,
        .max_freq_hz = 20000000,           /* 20MHz */
        .voltage_min_mv = 1800,
        .voltage_max_mv = 5500,
        .temp_min_c = -40,
        .temp_max_c = 85,
        .grade = CHIP_GRADE_INDUSTRIAL,
        .status = CHIP_STATUS_ACTIVE,
        .jtag_id = 0,
        .flash_id = 0,
        .device_id = 0x1E950F,
        .signature = {0x1E, 0x95, 0x0F},   /* AVR签名 */
        .primary_debug = DEBUG_ISP,
        .supported_debug_count = 2,
        .supported_debug = { DEBUG_ISP, DEBUG_IF_JTAG, DEBUG_IF_UNKNOWN, DEBUG_IF_UNKNOWN },
        .datasheet_url = "www.microchip.com/atmega328p",
        .reference_manual_url = NULL
    },
    
    /* 结束标记 */
    {
        .chip_id = 0,
        .vendor_id = VENDOR_UNKNOWN,
        .core_type = CORE_UNKNOWN,
        .family_name = NULL,
        .series_name = NULL,
        .part_number = NULL,
        .full_name = NULL,
        .flash_size = 0,
        .ram_size = 0,
        .eeprom_size = 0,
        .flash_sector_size = 0,
        .flash_page_size = 0,
        .package_type = NULL,
        .pin_count = 0,
        .max_freq_hz = 0,
        .voltage_min_mv = 0,
        .voltage_max_mv = 0,
        .temp_min_c = 0,
        .temp_max_c = 0,
        .grade = CHIP_GRADE_COMMERCIAL,
        .status = CHIP_STATUS_UNKNOWN,
        .jtag_id = 0,
        .flash_id = 0,
        .device_id = 0,
        .signature = {0, 0, 0},
        .primary_debug = DEBUG_IF_UNKNOWN,
        .supported_debug_count = 0,
        .supported_debug = { DEBUG_IF_UNKNOWN, DEBUG_IF_UNKNOWN, DEBUG_IF_UNKNOWN, DEBUG_IF_UNKNOWN },
        .datasheet_url = NULL,
        .reference_manual_url = NULL
    }
};

/* 静态芯片数量 */
static const uint32_t s_Chip_Count = sizeof(s_Chip_Table) / sizeof(s_Chip_Table[0]) - 1;

/**
 * ID映射表 - 用于自动识别芯片
 * 通过JTAG ID、Device ID等识别芯片
 */
static const Chip_ID_Map_Entry_t s_ID_Map_Table[] = {
    /* STM32F1系列 - 通过JTAG ID识别 */
    { 0x00010001, 0x4BA00477, 0xFFFFFFFF, DEBUG_IF_SWD, 10 },
    
    /* GD32F1系列 - 通过JTAG ID识别 */
    { 0x00020001, 0x1BA01477, 0xFFFFFFFF, DEBUG_IF_SWD, 10 },
    
    /* STM32F1系列 - 通过Device ID识别 */
    { 0x00010001, 0x410, 0xFFF, DEBUG_IF_SWD, 5 },
    
    /* CH32V系列 - 通过Device ID识别 */
    { 0x00030001, 0x103, 0xFFF, DEBUG_IF_SWD, 5 },
    
    /* ESP32-C3 - 通过Device ID识别 */
    { 0x00040001, 0xC3, 0xFF, DEBUG_IF_JTAG, 5 },
    
    /* ATmega328P - 通过签名识别 */
    { 0x00050001, 0x1E950F, 0xFFFFFF, DEBUG_ISP, 10 },
    
    /* 结束标记 */
    { 0, 0, 0, DEBUG_IF_UNKNOWN, 0 }
};

/* 静态ID映射数量 */
static const uint32_t s_ID_Map_Count = sizeof(s_ID_Map_Table) / sizeof(s_ID_Map_Table[0]) - 1;

/**
 * 调试接口名称表
 */
static const struct {
    Chip_Debug_Interface_t type;
    const char* name;
} s_Debug_Interface_Names[] = {
    { DEBUG_IF_SWD,      "SWD (Serial Wire Debug)" },
    { DEBUG_IF_JTAG,     "JTAG (IEEE 1149.1)" },
    { DEBUG_IF_BDM,      "BDM (Background Debug Mode)" },
    { DEBUG_IF_MON8,     "MON8 (Monitor Mode 8-bit)" },
    { DEBUG_IF_SBW,      "SBW (Spy-Bi-Wire)" },
    { DEBUG_IF_FINE,     "FINE (Renesas)" },
    { DEBUG_IF_ICSP,     "ICSP (In-Circuit Serial Programming)" },
    { DEBUG_ISP,         "ISP (In-System Programming)" },
    { DEBUG_IF_USB,      "USB Programming" },
    { DEBUG_IF_UART,     "UART Bootloader" },
    { DEBUG_IF_SPI,      "SPI Flash" },
    { DEBUG_IF_I2C,      "I2C EEPROM" },
    { DEBUG_IF_CAN,      "CAN Bus" },
    { DEBUG_IF_DAP,      "DAP (Debug Access Port)" },
    { DEBUG_IF_SWD_JTAG, "SWD/JTAG" },
    { DEBUG_IF_UNKNOWN,  "Unknown" }
};

/**
 * 内核类型名称表
 */
static const struct {
    Chip_Core_Type_t type;
    const char* name;
} s_Core_Type_Names[] = {
    /* ARM Cortex-M系列 */
    { CORE_ARM_CORTEX_M0,   "ARM Cortex-M0" },
    { CORE_ARM_CORTEX_M0P,  "ARM Cortex-M0+" },
    { CORE_ARM_CORTEX_M1,   "ARM Cortex-M1" },
    { CORE_ARM_CORTEX_M3,   "ARM Cortex-M3" },
    { CORE_ARM_CORTEX_M4,   "ARM Cortex-M4" },
    { CORE_ARM_CORTEX_M4F,  "ARM Cortex-M4F" },
    { CORE_ARM_CORTEX_M7,   "ARM Cortex-M7" },
    { CORE_ARM_CORTEX_M7F,  "ARM Cortex-M7F" },
    { CORE_ARM_CORTEX_M23,  "ARM Cortex-M23" },
    { CORE_ARM_CORTEX_M33,  "ARM Cortex-M33" },
    { CORE_ARM_CORTEX_M35P, "ARM Cortex-M35P" },
    { CORE_ARM_CORTEX_M55,  "ARM Cortex-M55" },
    { CORE_ARM_CORTEX_M85,  "ARM Cortex-M85" },
    
    /* ARM Cortex-A系列 */
    { CORE_ARM_CORTEX_A5,   "ARM Cortex-A5" },
    { CORE_ARM_CORTEX_A7,   "ARM Cortex-A7" },
    { CORE_ARM_CORTEX_A53,  "ARM Cortex-A53" },
    { CORE_ARM_CORTEX_A55,  "ARM Cortex-A55" },
    { CORE_ARM_CORTEX_A72,  "ARM Cortex-A72" },
    
    /* RISC-V系列 */
    { CORE_RISCV_RV32I,     "RISC-V RV32I" },
    { CORE_RISCV_RV32IM,    "RISC-V RV32IM" },
    { CORE_RISCV_RV32IMC,   "RISC-V RV32IMC" },
    { CORE_RISCV_RV32IMAC,  "RISC-V RV32IMAC" },
    { CORE_RISCV_RV32GC,    "RISC-V RV32GC" },
    { CORE_RISCV_RV64I,     "RISC-V RV64I" },
    { CORE_RISCV_RV64GC,    "RISC-V RV64GC" },
    
    /* 8位内核 */
    { CORE_8051_CLASSIC,    "8051 Classic" },
    { CORE_8051_1T,         "8051 1T Enhanced" },
    { CORE_AVR,             "AVR" },
    { CORE_AVR_XMEGA,       "AVR XMEGA" },
    { CORE_STM8,            "STM8" },
    
    /* 16位内核 */
    { CORE_MSP430,          "MSP430" },
    { CORE_MSP430X,         "MSP430X" },
    { CORE_HCS12,           "HCS12" },
    { CORE_RL78,            "RL78" },
    
    /* 32位专用内核 */
    { CORE_TRICORE_TC1,     "TriCore TC1" },
    { CORE_TRICORE_TC2,     "TriCore TC2" },
    { CORE_TRICORE_TC3,     "TriCore TC3" },
    { CORE_RH850,           "RH850" },
    { CORE_RX_V1,           "RX V1" },
    { CORE_RX_V2,           "RX V2" },
    { CORE_RX_V3,           "RX V3" },
    { CORE_V850,            "V850" },
    { CORE_C28X,            "TI C28x DSP" },
    { CORE_POWERPC_E200,    "PowerPC e200" },
    
    /* 其他 */
    { CORE_UNKNOWN,         "Unknown" }
};

/* ==================== 动态数据存储 ==================== */

/* 动态驱动注册表 */
static Chip_Driver_Entry_t s_Dynamic_Drivers[MAX_DYNAMIC_DRIVERS];

/* 动态芯片信息表 */
static Chip_Info_t s_Dynamic_Chips[MAX_DYNAMIC_CHIPS];

/* 动态ID映射表 */
static Chip_ID_Map_Entry_t s_Dynamic_ID_Maps[MAX_DYNAMIC_ID_MAPS];

/* 框架状态 */
static Framework_State_t s_Framework_State = { false, 0, 0, 0 };

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 大小写不敏感的字符串比较
 * @param s1 字符串1
 * @param s2 字符串2
 * @return 0表示相等，非0表示不等
 */
static int strcmpi(const char* s1, const char* s2)
{
    if (s1 == NULL || s2 == NULL) {
        return (s1 == s2) ? 0 : ((s1 == NULL) ? -1 : 1);
    }
    
    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        
        /* 转换为小写 */
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        
        if (c1 != c2) {
            return c1 - c2;
        }
        
        s1++;
        s2++;
    }
    
    return *s1 - *s2;
}

/**
 * @brief 简单哈希函数 - 用于快速查找
 * @param str 输入字符串
 * @return 哈希值
 */
static uint32_t simple_hash(const char* str)
{
    uint32_t hash = 5381;
    int c;
    
    if (str == NULL) return 0;
    
    while ((c = *str++)) {
        /* hash * 33 + c */
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash;
}

/**
 * @brief 检查字符串是否包含子串（大小写不敏感）
 * @param haystack 主字符串
 * @param needle 子串
 * @return true表示包含，false表示不包含
 */
static bool str_contains_ignore_case(const char* haystack, const char* needle)
{
    if (haystack == NULL || needle == NULL) {
        return false;
    }
    
    /* 空子串认为包含 */
    if (*needle == '\0') {
        return true;
    }
    
    const char* h = haystack;
    const char* n = needle;
    
    while (*h) {
        /* 找到可能的起始位置 */
        char h_char = *h;
        char n_char = *n;
        
        if (h_char >= 'A' && h_char <= 'Z') h_char += 32;
        if (n_char >= 'A' && n_char <= 'Z') n_char += 32;
        
        if (h_char == n_char) {
            const char* h_temp = h;
            const char* n_temp = n;
            
            /* 继续匹配 */
            while (*h_temp && *n_temp) {
                h_char = *h_temp;
                n_char = *n_temp;
                
                if (h_char >= 'A' && h_char <= 'Z') h_char += 32;
                if (n_char >= 'A' && n_char <= 'Z') n_char += 32;
                
                if (h_char != n_char) {
                    break;
                }
                
                h_temp++;
                n_temp++;
            }
            
            /* 完全匹配 */
            if (*n_temp == '\0') {
                return true;
            }
        }
        
        h++;
    }
    
    return false;
}

/* ==================== API函数实现 ==================== */

/**
 * @brief 初始化驱动框架
 * 
 * 初始化框架内部状态，准备驱动注册表和ID映射表。
 * 此函数应在使用任何其他框架功能之前调用。
 * 
 * @return true表示初始化成功，false表示失败
 */
bool Chip_Framework_Init(void)
{
    /* 检查是否已初始化 */
    if (s_Framework_State.initialized) {
        return true;  /* 已初始化，直接返回成功 */
    }
    
    /* 清零动态数据区 */
    memset(s_Dynamic_Drivers, 0, sizeof(s_Dynamic_Drivers));
    memset(s_Dynamic_Chips, 0, sizeof(s_Dynamic_Chips));
    memset(s_Dynamic_ID_Maps, 0, sizeof(s_Dynamic_ID_Maps));
    
    /* 初始化框架状态 */
    s_Framework_State.initialized = true;
    s_Framework_State.dynamic_driver_count = 0;
    s_Framework_State.dynamic_chip_count = 0;
    s_Framework_State.dynamic_id_map_count = 0;
    
    return true;
}

/**
 * @brief 关闭驱动框架
 * 
 * 释放框架资源，清理动态注册的驱动和数据。
 * 调用此函数后，需要重新初始化才能使用框架功能。
 * 
 * @return true表示关闭成功，false表示失败
 */
bool Chip_Framework_Close(void)
{
    /* 检查是否已初始化 */
    if (!s_Framework_State.initialized) {
        return true;  /* 未初始化，直接返回成功 */
    }
    
    /* 清零动态数据区 */
    memset(s_Dynamic_Drivers, 0, sizeof(s_Dynamic_Drivers));
    memset(s_Dynamic_Chips, 0, sizeof(s_Dynamic_Chips));
    memset(s_Dynamic_ID_Maps, 0, sizeof(s_Dynamic_ID_Maps));
    
    /* 重置框架状态 */
    s_Framework_State.initialized = false;
    s_Framework_State.dynamic_driver_count = 0;
    s_Framework_State.dynamic_chip_count = 0;
    s_Framework_State.dynamic_id_map_count = 0;
    
    return true;
}

/**
 * @brief 获取厂商信息
 * 
 * 根据厂商ID获取厂商的详细信息，包括名称、简称、国家和网站。
 * 
 * @param vendor_id 厂商ID
 * @return 厂商信息结构指针，未找到返回NULL
 */
const Chip_Vendor_Info_t* Chip_GetVendorInfo(Chip_Vendor_ID_t vendor_id)
{
    /* 遍历静态厂商表 */
    for (uint32_t i = 0; i < s_Vendor_Count; i++) {
        if (s_Vendor_Table[i].id == vendor_id) {
            return &s_Vendor_Table[i];
        }
    }
    
    return NULL;  /* 未找到 */
}

/**
 * @brief 根据厂商名称获取厂商ID
 * 
 * 通过厂商名称（全名或简称）查找对应的厂商ID。
 * 查找时忽略大小写。
 * 
 * @param name 厂商名称（全名或简称）
 * @return 厂商ID，未找到返回VENDOR_UNKNOWN
 */
Chip_Vendor_ID_t Chip_GetVendorIDByName(const char* name)
{
    if (name == NULL) {
        return VENDOR_UNKNOWN;
    }
    
    /* 遍历厂商表进行匹配 */
    for (uint32_t i = 0; i < s_Vendor_Count; i++) {
        /* 匹配全名 */
        if (s_Vendor_Table[i].name != NULL && 
            strcmpi(s_Vendor_Table[i].name, name) == 0) {
            return s_Vendor_Table[i].id;
        }
        
        /* 匹配简称 */
        if (s_Vendor_Table[i].short_name != NULL && 
            strcmpi(s_Vendor_Table[i].short_name, name) == 0) {
            return s_Vendor_Table[i].id;
        }
    }
    
    return VENDOR_UNKNOWN;  /* 未找到 */
}

/**
 * @brief 获取芯片信息
 * 
 * 根据芯片内部ID获取芯片的详细信息。
 * 
 * @param chip_id 芯片内部ID
 * @return 芯片信息结构指针，未找到返回NULL
 */
const Chip_Info_t* Chip_GetChipInfo(uint32_t chip_id)
{
    /* 首先搜索静态芯片表 */
    for (uint32_t i = 0; i < s_Chip_Count; i++) {
        if (s_Chip_Table[i].chip_id == chip_id) {
            return &s_Chip_Table[i];
        }
    }
    
    /* 然后搜索动态芯片表 */
    for (uint32_t i = 0; i < s_Framework_State.dynamic_chip_count; i++) {
        if (s_Dynamic_Chips[i].chip_id == chip_id) {
            return &s_Dynamic_Chips[i];
        }
    }
    
    return NULL;  /* 未找到 */
}

/**
 * @brief 根据型号获取芯片信息
 * 
 * 通过芯片的完整型号（如"STM32F103C8T6"）查找芯片信息。
 * 查找时忽略大小写。
 * 
 * @param part_number 芯片型号
 * @return 芯片信息结构指针，未找到返回NULL
 */
const Chip_Info_t* Chip_GetChipByPartNumber(const char* part_number)
{
    if (part_number == NULL) {
        return NULL;
    }
    
    /* 搜索静态芯片表 */
    for (uint32_t i = 0; i < s_Chip_Count; i++) {
        if (s_Chip_Table[i].part_number != NULL &&
            strcmpi(s_Chip_Table[i].part_number, part_number) == 0) {
            return &s_Chip_Table[i];
        }
    }
    
    /* 搜索动态芯片表 */
    for (uint32_t i = 0; i < s_Framework_State.dynamic_chip_count; i++) {
        if (s_Dynamic_Chips[i].part_number != NULL &&
            strcmpi(s_Dynamic_Chips[i].part_number, part_number) == 0) {
            return &s_Dynamic_Chips[i];
        }
    }
    
    return NULL;  /* 未找到 */
}

/**
 * @brief 根据ID值识别芯片
 * 
 * 通过JTAG ID、Device ID或其他硬件ID识别芯片。
 * 使用ID掩码进行匹配，支持部分匹配。
 * 
 * @param id_value ID值（如JTAG ID或Device ID）
 * @param detection_method 检测方法（调试接口类型）
 * @return 芯片信息结构指针，未找到返回NULL
 */
const Chip_Info_t* Chip_IdentifyByID(uint32_t id_value, 
                                      Chip_Debug_Interface_t detection_method)
{
    const Chip_ID_Map_Entry_t* best_match = NULL;
    uint8_t best_priority = 0;
    
    /* 搜索静态ID映射表 */
    for (uint32_t i = 0; i < s_ID_Map_Count; i++) {
        const Chip_ID_Map_Entry_t* entry = &s_ID_Map_Table[i];
        
        /* 检查检测方法是否匹配（如果指定了具体方法） */
        if (detection_method != DEBUG_IF_UNKNOWN && 
            entry->detection_method != detection_method) {
            continue;
        }
        
        /* 使用掩码进行ID匹配 */
        if ((id_value & entry->id_mask) == (entry->id_value & entry->id_mask)) {
            /* 选择优先级最高的匹配 */
            if (best_match == NULL || entry->priority > best_priority) {
                best_match = entry;
                best_priority = entry->priority;
            }
        }
    }
    
    /* 搜索动态ID映射表 */
    for (uint32_t i = 0; i < s_Framework_State.dynamic_id_map_count; i++) {
        const Chip_ID_Map_Entry_t* entry = &s_Dynamic_ID_Maps[i];
        
        /* 检查检测方法是否匹配 */
        if (detection_method != DEBUG_IF_UNKNOWN && 
            entry->detection_method != detection_method) {
            continue;
        }
        
        /* 使用掩码进行ID匹配 */
        if ((id_value & entry->id_mask) == (entry->id_value & entry->id_mask)) {
            if (best_match == NULL || entry->priority > best_priority) {
                best_match = entry;
                best_priority = entry->priority;
            }
        }
    }
    
    /* 如果找到匹配，返回对应的芯片信息 */
    if (best_match != NULL) {
        return Chip_GetChipInfo(best_match->chip_id);
    }
    
    return NULL;  /* 未找到 */
}

/**
 * @brief 搜索芯片
 * 
 * 根据查询字符串搜索芯片，支持模糊匹配。
 * 搜索范围包括型号、系列名、厂商名等。
 * 
 * @param query 查询字符串
 * @param results 结果数组
 * @param max_results 最大结果数量
 * @return 实际找到的芯片数量
 */
uint32_t Chip_Search(const char* query, Chip_Info_t* results, uint32_t max_results)
{
    uint32_t count = 0;
    
    if (query == NULL || results == NULL || max_results == 0) {
        return 0;
    }
    
    /* 搜索静态芯片表 */
    for (uint32_t i = 0; i < s_Chip_Count && count < max_results; i++) {
        const Chip_Info_t* chip = &s_Chip_Table[i];
        
        /* 在各字段中搜索匹配 */
        if ((chip->part_number && str_contains_ignore_case(chip->part_number, query)) ||
            (chip->family_name && str_contains_ignore_case(chip->family_name, query)) ||
            (chip->series_name && str_contains_ignore_case(chip->series_name, query)) ||
            (chip->full_name && str_contains_ignore_case(chip->full_name, query))) {
            
            /* 复制芯片信息到结果数组 */
            results[count] = *chip;
            count++;
        }
    }
    
    /* 搜索动态芯片表 */
    for (uint32_t i = 0; i < s_Framework_State.dynamic_chip_count && count < max_results; i++) {
        const Chip_Info_t* chip = &s_Dynamic_Chips[i];
        
        if ((chip->part_number && str_contains_ignore_case(chip->part_number, query)) ||
            (chip->family_name && str_contains_ignore_case(chip->family_name, query)) ||
            (chip->series_name && str_contains_ignore_case(chip->series_name, query)) ||
            (chip->full_name && str_contains_ignore_case(chip->full_name, query))) {
            
            results[count] = *chip;
            count++;
        }
    }
    
    return count;
}

/**
 * @brief 匹配驱动
 * 
 * 根据芯片信息匹配合适的驱动程序。
 * 匹配规则：内核类型 + 调试接口类型
 * 
 * @param chip 芯片信息
 * @return 驱动操作函数结构指针，未找到返回NULL
 */
const Chip_Driver_Ops_t* Chip_MatchDriver(const Chip_Info_t* chip)
{
    if (chip == NULL) {
        return NULL;
    }
    
    /* 搜索动态驱动表 */
    for (uint32_t i = 0; i < s_Framework_State.dynamic_driver_count; i++) {
        const Chip_Driver_Entry_t* driver = &s_Dynamic_Drivers[i];
        
        /* 检查内核类型是否匹配 */
        if (driver->core_type != chip->core_type) {
            continue;
        }
        
        /* 检查调试接口是否匹配 */
        if (driver->debug_interface == chip->primary_debug ||
            driver->debug_interface == DEBUG_IF_SWD_JTAG) {
            return driver->ops;
        }
        
        /* 检查是否在支持的调试接口列表中 */
        for (uint8_t j = 0; j < chip->supported_debug_count; j++) {
            if (driver->debug_interface == chip->supported_debug[j]) {
                return driver->ops;
            }
        }
    }
    
    return NULL;  /* 未找到匹配的驱动 */
}

/**
 * @brief 注册驱动
 * 
 * 动态注册一个芯片驱动到框架中。
 * 注册后，该驱动可用于芯片匹配。
 * 
 * @param driver 驱动注册信息
 * @return true表示注册成功，false表示失败
 */
bool Chip_RegisterDriver(const Chip_Driver_Entry_t* driver)
{
    /* 参数检查 */
    if (driver == NULL || driver->ops == NULL) {
        return false;
    }
    
    /* 检查框架是否已初始化 */
    if (!s_Framework_State.initialized) {
        return false;
    }
    
    /* 检查是否还有空间 */
    if (s_Framework_State.dynamic_driver_count >= MAX_DYNAMIC_DRIVERS) {
        return false;
    }
    
    /* 检查驱动是否已存在（通过驱动ID或名称） */
    for (uint32_t i = 0; i < s_Framework_State.dynamic_driver_count; i++) {
        if (s_Dynamic_Drivers[i].driver_id == driver->driver_id) {
            return false;  /* 驱动ID已存在 */
        }
        if (s_Dynamic_Drivers[i].driver_name != NULL && 
            driver->driver_name != NULL &&
            strcmpi(s_Dynamic_Drivers[i].driver_name, driver->driver_name) == 0) {
            return false;  /* 驱动名称已存在 */
        }
    }
    
    /* 添加驱动到动态表 */
    s_Dynamic_Drivers[s_Framework_State.dynamic_driver_count] = *driver;
    s_Framework_State.dynamic_driver_count++;
    
    return true;
}

/**
 * @brief 获取芯片总数
 * 
 * 返回框架中注册的芯片总数（静态 + 动态）。
 * 
 * @return 芯片总数
 */
uint32_t Chip_GetTotalChipCount(void)
{
    return s_Chip_Count + s_Framework_State.dynamic_chip_count;
}

/**
 * @brief 获取驱动总数
 * 
 * 返回框架中注册的驱动总数（动态注册）。
 * 
 * @return 驱动总数
 */
uint32_t Chip_GetTotalDriverCount(void)
{
    return s_Framework_State.dynamic_driver_count;
}

/**
 * @brief 获取厂商总数
 * 
 * 返回框架中支持的厂商总数。
 * 
 * @return 厂商总数
 */
uint32_t Chip_GetTotalVendorCount(void)
{
    return s_Vendor_Count;
}

/**
 * @brief 打印芯片信息
 * 
 * 将芯片的详细信息打印输出，用于调试和显示。
 * 
 * @param chip 芯片信息结构指针
 */
void Chip_PrintInfo(const Chip_Info_t* chip)
{
    if (chip == NULL) {
        printf("芯片信息: NULL\r\n");
        return;
    }
    
    printf("\r\n========== 芯片信息 ==========\r\n");
    printf("型号: %s\r\n", chip->part_number ? chip->part_number : "N/A");
    printf("全名: %s\r\n", chip->full_name ? chip->full_name : "N/A");
    printf("厂商: %s\r\n", Chip_GetVendorName(chip->vendor_id));
    printf("内核: %s\r\n", Chip_GetCoreTypeName(chip->core_type));
    printf("系列: %s\r\n", chip->family_name ? chip->family_name : "N/A");
    printf("子系列: %s\r\n", chip->series_name ? chip->series_name : "N/A");
    
    printf("\r\n--- 存储信息 ---\r\n");
    printf("Flash: %lu KB\r\n", chip->flash_size / 1024);
    printf("RAM: %lu KB\r\n", chip->ram_size / 1024);
    printf("EEPROM: %lu KB\r\n", chip->eeprom_size / 1024);
    printf("扇区大小: %lu 字节\r\n", chip->flash_sector_size);
    printf("页大小: %lu 字节\r\n", chip->flash_page_size);
    
    printf("\r\n--- 封装信息 ---\r\n");
    printf("封装: %s\r\n", chip->package_type ? chip->package_type : "N/A");
    printf("引脚数: %d\r\n", chip->pin_count);
    
    printf("\r\n--- 工作参数 ---\r\n");
    printf("最大频率: %lu MHz\r\n", chip->max_freq_hz / 1000000);
    printf("电压范围: %d.%dV ~ %d.%dV\r\n", 
           chip->voltage_min_mv / 1000, (chip->voltage_min_mv % 1000) / 100,
           chip->voltage_max_mv / 1000, (chip->voltage_max_mv % 1000) / 100);
    printf("温度范围: %d°C ~ %d°C\r\n", chip->temp_min_c, chip->temp_max_c);
    
    printf("\r\n--- 调试接口 ---\r\n");
    printf("主接口: %s\r\n", Chip_GetDebugInterfaceName(chip->primary_debug));
    printf("支持接口: ");
    for (uint8_t i = 0; i < chip->supported_debug_count; i++) {
        printf("%s", Chip_GetDebugInterfaceName(chip->supported_debug[i]));
        if (i < chip->supported_debug_count - 1) {
            printf(", ");
        }
    }
    printf("\r\n");
    
    printf("\r\n--- ID信息 ---\r\n");
    printf("芯片ID: 0x%08lX\r\n", chip->chip_id);
    printf("JTAG ID: 0x%08lX\r\n", chip->jtag_id);
    printf("Device ID: 0x%08lX\r\n", chip->device_id);
    
    printf("==============================\r\n\r\n");
}

/**
 * @brief 获取调试接口名称
 * 
 * 将调试接口类型转换为可读的名称字符串。
 * 
 * @param debug_if 调试接口类型
 * @return 调试接口名称字符串
 */
const char* Chip_GetDebugInterfaceName(Chip_Debug_Interface_t debug_if)
{
    /* 遍历调试接口名称表 */
    for (uint32_t i = 0; i < sizeof(s_Debug_Interface_Names) / sizeof(s_Debug_Interface_Names[0]); i++) {
        if (s_Debug_Interface_Names[i].type == debug_if) {
            return s_Debug_Interface_Names[i].name;
        }
    }
    
    return "Unknown";
}

/**
 * @brief 获取内核类型名称
 * 
 * 将内核类型枚举转换为可读的名称字符串。
 * 
 * @param core 内核类型
 * @return 内核名称字符串
 */
const char* Chip_GetCoreTypeName(Chip_Core_Type_t core)
{
    /* 遍历内核类型名称表 */
    for (uint32_t i = 0; i < sizeof(s_Core_Type_Names) / sizeof(s_Core_Type_Names[0]); i++) {
        if (s_Core_Type_Names[i].type == core) {
            return s_Core_Type_Names[i].name;
        }
    }
    
    return "Unknown";
}

/**
 * @brief 获取厂商名称
 * 
 * 根据厂商ID获取厂商的简称。
 * 
 * @param vendor_id 厂商ID
 * @return 厂商名称字符串
 */
const char* Chip_GetVendorName(Chip_Vendor_ID_t vendor_id)
{
    const Chip_Vendor_Info_t* info = Chip_GetVendorInfo(vendor_id);
    
    if (info != NULL && info->short_name != NULL) {
        return info->short_name;
    }
    
    return "Unknown";
}

/* ==================== 调试接口抽象层实现 ==================== */

/* SWD调试接口操作函数 - 占位实现 */
static bool SWD_Init(uint32_t speed_hz) { (void)speed_hz; return true; }
static bool SWD_Close(void) { return true; }
static bool SWD_Connect(void) { return true; }
static bool SWD_Disconnect(void) { return true; }
static bool SWD_ReadID(uint32_t* id) { *id = 0; return true; }
static bool SWD_GetCapabilities(uint32_t* caps) { *caps = 0; return true; }
static bool SWD_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size) { 
    (void)addr; (void)data; (void)size; return true; 
}
static bool SWD_MemRead(uint32_t addr, uint8_t* data, uint32_t size) { 
    (void)addr; (void)data; (void)size; return true; 
}
static bool SWD_RegWrite(uint32_t addr, uint32_t value) { (void)addr; (void)value; return true; }
static bool SWD_RegRead(uint32_t addr, uint32_t* value) { (void)addr; *value = 0; return true; }
static bool SWD_Reset(void) { return true; }
static bool SWD_Halt(void) { return true; }
static bool SWD_Run(void) { return true; }
static bool SWD_Step(void) { return true; }
static bool SWD_SetSpeed(uint32_t speed_hz) { (void)speed_hz; return true; }
static uint32_t SWD_GetSpeed(void) { return 1000000; }

/* SWD调试接口操作结构 */
static const Chip_Debug_Interface_Ops_t s_SWD_Ops = {
    .Init = SWD_Init,
    .Close = SWD_Close,
    .Connect = SWD_Connect,
    .Disconnect = SWD_Disconnect,
    .ReadID = SWD_ReadID,
    .GetCapabilities = SWD_GetCapabilities,
    .MemWrite = SWD_MemWrite,
    .MemRead = SWD_MemRead,
    .RegWrite = SWD_RegWrite,
    .RegRead = SWD_RegRead,
    .Reset = SWD_Reset,
    .Halt = SWD_Halt,
    .Run = SWD_Run,
    .Step = SWD_Step,
    .SetSpeed = SWD_SetSpeed,
    .GetSpeed = SWD_GetSpeed,
    .name = "SWD",
    .type = DEBUG_IF_SWD
};

/* JTAG调试接口操作函数 - 占位实现 */
static bool JTAG_Init(uint32_t speed_hz) { (void)speed_hz; return true; }
static bool JTAG_Close(void) { return true; }
static bool JTAG_Connect(void) { return true; }
static bool JTAG_Disconnect(void) { return true; }
static bool JTAG_ReadID(uint32_t* id) { *id = 0; return true; }
static bool JTAG_GetCapabilities(uint32_t* caps) { *caps = 0; return true; }
static bool JTAG_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size) { 
    (void)addr; (void)data; (void)size; return true; 
}
static bool JTAG_MemRead(uint32_t addr, uint8_t* data, uint32_t size) { 
    (void)addr; (void)data; (void)size; return true; 
}
static bool JTAG_RegWrite(uint32_t addr, uint32_t value) { (void)addr; (void)value; return true; }
static bool JTAG_RegRead(uint32_t addr, uint32_t* value) { (void)addr; *value = 0; return true; }
static bool JTAG_Reset(void) { return true; }
static bool JTAG_Halt(void) { return true; }
static bool JTAG_Run(void) { return true; }
static bool JTAG_Step(void) { return true; }
static bool JTAG_SetSpeed(uint32_t speed_hz) { (void)speed_hz; return true; }
static uint32_t JTAG_GetSpeed(void) { return 1000000; }

/* JTAG调试接口操作结构 */
static const Chip_Debug_Interface_Ops_t s_JTAG_Ops = {
    .Init = JTAG_Init,
    .Close = JTAG_Close,
    .Connect = JTAG_Connect,
    .Disconnect = JTAG_Disconnect,
    .ReadID = JTAG_ReadID,
    .GetCapabilities = JTAG_GetCapabilities,
    .MemWrite = JTAG_MemWrite,
    .MemRead = JTAG_MemRead,
    .RegWrite = JTAG_RegWrite,
    .RegRead = JTAG_RegRead,
    .Reset = JTAG_Reset,
    .Halt = JTAG_Halt,
    .Run = JTAG_Run,
    .Step = JTAG_Step,
    .SetSpeed = JTAG_SetSpeed,
    .GetSpeed = JTAG_GetSpeed,
    .name = "JTAG",
    .type = DEBUG_IF_JTAG
};

/* UART调试接口操作函数 - 占位实现 */
static bool UART_Init(uint32_t speed_hz) { (void)speed_hz; return true; }
static bool UART_Close(void) { return true; }
static bool UART_Connect(void) { return true; }
static bool UART_Disconnect(void) { return true; }
static bool UART_ReadID(uint32_t* id) { *id = 0; return true; }
static bool UART_GetCapabilities(uint32_t* caps) { *caps = 0; return true; }
static bool UART_MemWrite(uint32_t addr, const uint8_t* data, uint32_t size) { 
    (void)addr; (void)data; (void)size; return true; 
}
static bool UART_MemRead(uint32_t addr, uint8_t* data, uint32_t size) { 
    (void)addr; (void)data; (void)size; return true; 
}
static bool UART_RegWrite(uint32_t addr, uint32_t value) { (void)addr; (void)value; return true; }
static bool UART_RegRead(uint32_t addr, uint32_t* value) { (void)addr; *value = 0; return true; }
static bool UART_Reset(void) { return true; }
static bool UART_Halt(void) { return true; }
static bool UART_Run(void) { return true; }
static bool UART_Step(void) { return true; }
static bool UART_SetSpeed(uint32_t speed_hz) { (void)speed_hz; return true; }
static uint32_t UART_GetSpeed(void) { return 115200; }

/* UART调试接口操作结构 */
static const Chip_Debug_Interface_Ops_t s_UART_Ops = {
    .Init = UART_Init,
    .Close = UART_Close,
    .Connect = UART_Connect,
    .Disconnect = UART_Disconnect,
    .ReadID = UART_ReadID,
    .GetCapabilities = UART_GetCapabilities,
    .MemWrite = UART_MemWrite,
    .MemRead = UART_MemRead,
    .RegWrite = UART_RegWrite,
    .RegRead = UART_RegRead,
    .Reset = UART_Reset,
    .Halt = UART_Halt,
    .Run = UART_Run,
    .Step = UART_Step,
    .SetSpeed = UART_SetSpeed,
    .GetSpeed = UART_GetSpeed,
    .name = "UART",
    .type = DEBUG_IF_UART
};

/**
 * @brief 创建调试接口
 * 
 * 根据调试接口类型创建对应的调试接口操作对象。
 * 返回的对象包含该接口的所有操作函数。
 * 
 * @param type 调试接口类型
 * @return 调试接口操作结构指针，不支持返回NULL
 */
const Chip_Debug_Interface_Ops_t* Chip_CreateDebugInterface(Chip_Debug_Interface_t type)
{
    switch (type) {
        case DEBUG_IF_SWD:
            return &s_SWD_Ops;
            
        case DEBUG_IF_JTAG:
            return &s_JTAG_Ops;
            
        case DEBUG_IF_UART:
            return &s_UART_Ops;
            
        case DEBUG_IF_SWD_JTAG:
            /* SWD/JTAG复用，默认返回SWD */
            return &s_SWD_Ops;
            
        default:
            return NULL;  /* 不支持的调试接口类型 */
    }
}

/* ==================== 外部变量定义 ==================== */

/* 导出厂商信息表 */
const Chip_Vendor_Info_t Chip_Vendors_Table[] = {
    { VENDOR_ST, "STMicroelectronics", "ST", "瑞士", "www.st.com" },
    { VENDOR_NXP, "NXP Semiconductors", "NXP", "荷兰", "www.nxp.com" },
    { VENDOR_GIGADEVICE, "GigaDevice", "GD", "中国", "www.gigadevice.com" },
    { VENDOR_UNKNOWN, NULL, NULL, NULL, NULL }
};
const uint32_t Chip_Vendors_Count = 3;

/* 导出驱动注册表 - 空表，由动态注册填充 */
const Chip_Driver_Entry_t Chip_Drivers_Table[] = {
    { 0, NULL, CORE_UNKNOWN, DEBUG_IF_UNKNOWN, NULL, 0, NULL }
};
const uint32_t Chip_Drivers_Count = 0;

/* 导出ID映射表 */
const Chip_ID_Map_Entry_t Chip_ID_Map_Table[] = {
    { 0x00010001, 0x4BA00477, 0xFFFFFFFF, DEBUG_IF_SWD, 10 },
    { 0, 0, 0, DEBUG_IF_UNKNOWN, 0 }
};
const uint32_t Chip_ID_Map_Count = 1;
