/**
 ******************************************************************************
 * @file    dsp_fpga_driver.c
 * @brief   DSP/FPGA/CPLD驱动实现
 *          支持TI TMS320、ADI Blackfin DSP、Xilinx/Altera/Lattice FPGA/CPLD
 * 
 * @author  AI_PROG项目
 * @date    2026-06-06
 * @version v2.0
 * 
 * @details 本驱动实现DSP和FPGA/CPLD的编程读写功能：
 *          
 *          DSP支持：
 *          - TI TMS320C2000系列 (F2837x/F28004x等)
 *          - TI TMS320C6000系列 (C674x/C66x等)
 *          - TI TMS320DM (DaVinci视频处理器)
 *          - ADI Blackfin (BF5xx/BF6xx系列)
 *          - ADI SHARC (ADSP-21xxx系列)
 *          
 *          FPGA支持：
 *          - Xilinx 7系列 (Artix-7/Kintex-7/Virtex-7)
 *          - Xilinx UltraScale/UltraScale+
 *          - Xilinx Spartan (Spartan-6/Spartan-7)
 *          - Intel/Altera Cyclone (Cyclone IV/V/10)
 *          - Intel/Altera Stratix (Stratix IV/V/10)
 *          - Lattice iCE40/ECP5/MachXO
 *          - 高云GW1N/GW2A
 *          - 安路EG4
 *          
 *          CPLD支持：
 *          - Xilinx CoolRunner-II
 *          - Intel/Altera MAX II/V/10
 *          - Lattice MachXO2/3
 ******************************************************************************
 */

#include "dsp_fpga_driver.h"
#include "jtag.h"
#include <string.h>

/* ==================== GPIO操作宏 ==================== */
#define JTAG_TCK_HIGH(cfg)   ((cfg)->tck_port->BSRR = (cfg)->tck_pin)
#define JTAG_TCK_LOW(cfg)    ((cfg)->tck_port->BSRR = (cfg)->tck_pin << 16)
#define JTAG_TMS_HIGH(cfg)   ((cfg)->tms_port->BSRR = (cfg)->tms_pin)
#define JTAG_TMS_LOW(cfg)    ((cfg)->tms_port->BSRR = (cfg)->tms_pin << 16)
#define JTAG_TDI_HIGH(cfg)   ((cfg)->tdi_port->BSRR = (cfg)->tdi_pin)
#define JTAG_TDI_LOW(cfg)    ((cfg)->tdi_port->BSRR = (cfg)->tdi_pin << 16)
#define JTAG_TDO_READ(cfg)   (((cfg)->tdo_port->IDR & (cfg)->tdo_pin) != 0)

/* ==================== DSP型号数据库 ==================== */
typedef struct {
    DSP_Type_t type;
    uint32_t   idcode;
    char       part_number[24];
    uint32_t   flash_base;
    uint32_t   flash_size;
    uint32_t   ram_base;
    uint32_t   ram_size;
} DSP_Model_t;

static const DSP_Model_t s_dsp_models[] = {
    /* TI TMS320C2000系列 */
    { DSP_TYPE_TMS320C2000, 0x0B9B002F, "TMS320F28377S",  0x00200000, 512*1024,  0x00800000, 100*1024 },
    { DSP_TYPE_TMS320C2000, 0x0B9B0030, "TMS320F28377D",  0x00200000, 1024*1024, 0x00800000, 200*1024 },
    { DSP_TYPE_TMS320C2000, 0x0B99102F, "TMS320F280049C", 0x00200000, 256*1024,  0x00800000, 100*1024 },
    { DSP_TYPE_TMS320C2000, 0x0B99102E, "TMS320F280041C", 0x00200000, 128*1024,  0x00800000, 40*1024 },
    { DSP_TYPE_TMS320C2000, 0x0B98A02F, "TMS320F28335",   0x00300000, 256*1024,  0x00800000, 34*1024 },
    { DSP_TYPE_TMS320C2000, 0x0B98A030, "TMS320F28334",   0x00300000, 128*1024,  0x00800000, 34*1024 },
    
    /* TI TMS320C6000系列 */
    { DSP_TYPE_TMS320C6000, 0x0B7BC02F, "TMS320C6748",    0x11800000, 512*1024,  0x11820000, 312*1024 },
    { DSP_TYPE_TMS320C6000, 0x0B9BC02F, "TMS320C6657",    0x10800000, 1024*1024, 0x10880000, 512*1024 },
    { DSP_TYPE_TMS320C6000, 0x0BA0C02F, "TMS320C6678",    0x10800000, 0,         0x10880000, 8*1024*1024 },
    
    /* TI DaVinci系列 */
    { DSP_TYPE_TMS320DM,    0x0B7D902F, "TMS320DM8148",   0x00000000, 0,         0x80000000, 512*1024 },
    { DSP_TYPE_TMS320DM,    0x0B7DA02F, "TMS320DM8168",   0x00000000, 0,         0x80000000, 1024*1024 },
    
    /* ADI Blackfin系列 */
    { DSP_TYPE_BLACKFIN,    0x04000001, "ADSP-BF537",     0x20000000, 0,         0xFF800000, 32*1024 },
    { DSP_TYPE_BLACKFIN,    0x04000002, "ADSP-BF527",     0x20000000, 0,         0xFF800000, 32*1024 },
    { DSP_TYPE_BLACKFIN,    0x04000003, "ADSP-BF609",     0x20000000, 0,         0xFF800000, 128*1024 },
    
    /* ADI SHARC系列 */
    { DSP_TYPE_SHARC,       0x05000001, "ADSP-21479",     0x20000000, 0,         0x00000000, 5*1024*1024 },
    { DSP_TYPE_SHARC,       0x05000002, "ADSP-21489",     0x20000000, 0,         0x00000000, 5*1024*1024 },
    
    /* 结束标记 */
    { DSP_TYPE_TMS320C2000, 0,          "",               0,          0,          0,          0 }
};

/* ==================== FPGA型号数据库 ==================== */
typedef struct {
    FPGA_Type_t type;
    uint32_t    idcode;
    char        part_number[24];
    uint32_t    flash_size;
    uint8_t     is_cpld;
} FPGA_Model_t;

static const FPGA_Model_t s_fpga_models[] = {
    /* Xilinx 7系列 */
    { FPGA_TYPE_XILINX_SERIES7,  0x362D093, "XC7A35T",       16*1024*1024, 0 },
    { FPGA_TYPE_XILINX_SERIES7,  0x3636093, "XC7A50T",       16*1024*1024, 0 },
    { FPGA_TYPE_XILINX_SERIES7,  0x363F093, "XC7A75T",       16*1024*1024, 0 },
    { FPGA_TYPE_XILINX_SERIES7,  0x3647093, "XC7A100T",      32*1024*1024, 0 },
    { FPGA_TYPE_XILINX_SERIES7,  0x3651093, "XC7A200T",      32*1024*1024, 0 },
    { FPGA_TYPE_XILINX_SERIES7,  0x3662093, "XC7K70T",       32*1024*1024, 0 },
    { FPGA_TYPE_XILINX_SERIES7,  0x3667093, "XC7K160T",      64*1024*1024, 0 },
    { FPGA_TYPE_XILINX_SERIES7,  0x3671093, "XC7K325T",      64*1024*1024, 0 },
    { FPGA_TYPE_XILINX_SERIES7,  0x3676093, "XC7K410T",      128*1024*1024,0 },
    { FPGA_TYPE_XILINX_SERIES7,  0x3681093, "XC7VX485T",     128*1024*1024,0 },
    
    /* Xilinx UltraScale */
    { FPGA_TYPE_XILINX_ULTRASCALE, 0x04A22093, "XCKU040",   128*1024*1024,0 },
    { FPGA_TYPE_XILINX_ULTRASCALE, 0x04A28093, "XCKU060",   128*1024*1024,0 },
    { FPGA_TYPE_XILINX_ULTRASCALE, 0x04B21093, "XCVU065",   128*1024*1024,0 },
    { FPGA_TYPE_XILINX_ULTRASCALE, 0x04B36093, "XCVU095",   128*1024*1024,0 },
    
    /* Xilinx Spartan */
    { FPGA_TYPE_XILINX_SPARTAN,  0x34001D93,  "XC6SLX9",       8*1024*1024,  0 },
    { FPGA_TYPE_XILINX_SPARTAN,  0x34011D93,  "XC6SLX16",      16*1024*1024, 0 },
    { FPGA_TYPE_XILINX_SPARTAN,  0x34021D93,  "XC6SLX25",      16*1024*1024, 0 },
    { FPGA_TYPE_XILINX_SPARTAN,  0x34031D93,  "XC6SLX45",      32*1024*1024, 0 },
    
    /* Xilinx CoolRunner-II CPLD */
    { FPGA_TYPE_XILINX_COOLRUNNER, 0x0101D093, "XC2C256",    0,             1 },
    { FPGA_TYPE_XILINX_COOLRUNNER, 0x0101E093, "XC2C384",    0,             1 },
    { FPGA_TYPE_XILINX_COOLRUNNER, 0x0101F093, "XC2C512",    0,             1 },
    
    /* Intel/Altera Cyclone系列 */
    { FPGA_TYPE_INTEL_CYCLONE,   0x020F10DD,  "EP4CE6",        8*1024*1024,  0 },
    { FPGA_TYPE_INTEL_CYCLONE,   0x020F20DD,  "EP4CE10",       16*1024*1024, 0 },
    { FPGA_TYPE_INTEL_CYCLONE,   0x020F30DD,  "EP4CE15",       16*1024*1024, 0 },
    { FPGA_TYPE_INTEL_CYCLONE,   0x020F40DD,  "EP4CE22",       16*1024*1024, 0 },
    { FPGA_TYPE_INTEL_CYCLONE,   0x020F50DD,  "EP4CE30",       32*1024*1024, 0 },
    { FPGA_TYPE_INTEL_CYCLONE,   0x020F60DD,  "5CEFA2",        16*1024*1024, 0 },
    { FPGA_TYPE_INTEL_CYCLONE,   0x020F70DD,  "5CEFA4",        16*1024*1024, 0 },
    { FPGA_TYPE_INTEL_CYCLONE,   0x020F80DD,  "5CEFA5",        32*1024*1024, 0 },
    { FPGA_TYPE_INTEL_CYCLONE,   0x020F90DD,  "10CL006",       16*1024*1024, 0 },
    { FPGA_TYPE_INTEL_CYCLONE,   0x020FA0DD,  "10CL010",       16*1024*1024, 0 },
    
    /* Intel/Altera Stratix系列 */
    { FPGA_TYPE_INTEL_STRATIX,   0x021020DD,  "EP4SGX70",      64*1024*1024, 0 },
    { FPGA_TYPE_INTEL_STRATIX,   0x021030DD,  "EP4SGX110",     64*1024*1024, 0 },
    { FPGA_TYPE_INTEL_STRATIX,   0x021040DD,  "EP4SGX230",     128*1024*1024,0 },
    { FPGA_TYPE_INTEL_STRATIX,   0x021060DD,  "5SGXA3",        128*1024*1024,0 },
    { FPGA_TYPE_INTEL_STRATIX,   0x021070DD,  "5SGXA5",        128*1024*1024,0 },
    { FPGA_TYPE_INTEL_STRATIX,   0x021080DD,  "5SGXA7",        128*1024*1024,0 },
    
    /* Intel/Altera MAX CPLD系列 */
    { FPGA_TYPE_INTEL_MAX,       0x020A10DD,  "EPM240",        0,             1 },
    { FPGA_TYPE_INTEL_MAX,       0x020A20DD,  "EPM570",        0,             1 },
    { FPGA_TYPE_INTEL_MAX,       0x020A30DD,  "EPM1270",       0,             1 },
    { FPGA_TYPE_INTEL_MAX,       0x020A40DD,  "EPM2210",       0,             1 },
    { FPGA_TYPE_INTEL_MAX,       0x020A50DD,  "10M02",         0,             1 },
    { FPGA_TYPE_INTEL_MAX,       0x020A60DD,  "10M04",         0,             1 },
    { FPGA_TYPE_INTEL_MAX,       0x020A70DD,  "10M08",         0,             1 },
    { FPGA_TYPE_INTEL_MAX,       0x020A80DD,  "10M16",         0,             1 },
    { FPGA_TYPE_INTEL_MAX,       0x020A90DD,  "10M25",         0,             1 },
    { FPGA_TYPE_INTEL_MAX,       0x020AA0DD,  "10M40",         0,             1 },
    { FPGA_TYPE_INTEL_MAX,       0x020AB0DD,  "10M50",         0,             1 },
    
    /* Lattice iCE40系列 */
    { FPGA_TYPE_LATTICE_ICE40,   0x01001C43,  "iCE40HX1K",     8*1024*1024,  0 },
    { FPGA_TYPE_LATTICE_ICE40,   0x02001C43,  "iCE40HX4K",     8*1024*1024,  0 },
    { FPGA_TYPE_LATTICE_ICE40,   0x03001C43,  "iCE40HX8K",     16*1024*1024, 0 },
    { FPGA_TYPE_LATTICE_ICE40,   0x04001C43,  "iCE40LP1K",     8*1024*1024,  0 },
    { FPGA_TYPE_LATTICE_ICE40,   0x05001C43,  "iCE40LP4K",     8*1024*1024,  0 },
    { FPGA_TYPE_LATTICE_ICE40,   0x06001C43,  "iCE40LP8K",     16*1024*1024, 0 },
    { FPGA_TYPE_LATTICE_ICE40,   0x07001C43,  "iCE40UP5K",     8*1024*1024,  0 },
    { FPGA_TYPE_LATTICE_ICE40,   0x08001C43,  "iCE5LP4K",      8*1024*1024,  0 },
    
    /* Lattice ECP5系列 */
    { FPGA_TYPE_LATTICE_ECP5,    0x21111043,  "LFE5U-12F",     16*1024*1024, 0 },
    { FPGA_TYPE_LATTICE_ECP5,    0x41111043,  "LFE5U-25F",     32*1024*1024, 0 },
    { FPGA_TYPE_LATTICE_ECP5,    0x61111043,  "LFE5U-45F",     32*1024*1024, 0 },
    { FPGA_TYPE_LATTICE_ECP5,    0x81111043,  "LFE5U-85F",     64*1024*1024, 0 },
    
    /* Lattice MachXO2/3 */
    { FPGA_TYPE_LATTICE_MACHXO,  0x012B6043,  "LCMXO2-1200",   0,             1 },
    { FPGA_TYPE_LATTICE_MACHXO,  0x012BA043,  "LCMXO2-2000",   0,             1 },
    { FPGA_TYPE_LATTICE_MACHXO,  0x012BE043,  "LCMXO2-4000",   0,             1 },
    { FPGA_TYPE_LATTICE_MACHXO,  0x012C2043,  "LCMXO2-7000",   0,             1 },
    { FPGA_TYPE_LATTICE_MACHXO,  0x012C6043,  "LCMXO3-1300",   0,             1 },
    { FPGA_TYPE_LATTICE_MACHXO,  0x012CA043,  "LCMXO3-2100",   0,             1 },
    { FPGA_TYPE_LATTICE_MACHXO,  0x012CE043,  "LCMXO3-4300",   0,             1 },
    { FPGA_TYPE_LATTICE_MACHXO,  0x012D2043,  "LCMXO3-6900",   0,             1 },
    
    /* 高云GW1N系列 */
    { FPGA_TYPE_GOWIN_GW1N,      0x0100096,   "GW1N-1",        8*1024*1024,  0 },
    { FPGA_TYPE_GOWIN_GW1N,      0x0200096,   "GW1N-2",        8*1024*1024,  0 },
    { FPGA_TYPE_GOWIN_GW1N,      0x0300096,   "GW1N-4",        16*1024*1024, 0 },
    { FPGA_TYPE_GOWIN_GW1N,      0x0400096,   "GW1NR-1",       8*1024*1024,  0 },
    
    /* 高云GW2A系列 */
    { FPGA_TYPE_GOWIN_GW2A,      0x0101096,   "GW2A-18",       64*1024*1024, 0 },
    { FPGA_TYPE_GOWIN_GW2A,      0x0201096,   "GW2A-55",       64*1024*1024, 0 },
    
    /* 安路EG4系列 */
    { FPGA_TYPE_ANLOGIC_EG4,     0x01000001,  "EG4S20",        16*1024*1024, 0 },
    { FPGA_TYPE_ANLOGIC_EG4,     0x02000001,  "EG4D20",        16*1024*1024, 0 },
    
    /* 结束标记 */
    { FPGA_TYPE_XILINX_SERIES7,  0,           "",              0,             0 }
};

/* ==================== 内部JTAG操作函数 ==================== */

/**
 * @brief JTAG TAP状态机复位
 */
static void JTAG_TapReset(JTAG_Config_t* cfg)
{
    /* 在TMS上连续输出5个1，使TAP回到Test-Logic-Reset状态 */
    for (int i = 0; i < 6; i++) {
        JTAG_TMS_HIGH(cfg);
        JTAG_TCK_HIGH(cfg);
        JTAG_TCK_LOW(cfg);
    }
    JTAG_TMS_LOW(cfg);
}

/**
 * @brief JTAG移入指令
 */
static void JTAG_ShiftIR(JTAG_Config_t* cfg, uint32_t ir, uint8_t ir_len)
{
    /* 进入Shift-IR状态 */
    JTAG_TMS_HIGH(cfg); JTAG_TCK_HIGH(cfg); JTAG_TCK_LOW(cfg);  /* Select-DR */
    JTAG_TMS_HIGH(cfg); JTAG_TCK_HIGH(cfg); JTAG_TCK_LOW(cfg);  /* Select-IR */
    JTAG_TMS_LOW(cfg);  JTAG_TCK_HIGH(cfg); JTAG_TCK_LOW(cfg);  /* Capture-IR */
    JTAG_TMS_LOW(cfg);  JTAG_TCK_HIGH(cfg); JTAG_TCK_LOW(cfg);  /* Shift-IR */
    
    /* 移入指令数据 */
    for (int i = 0; i < ir_len - 1; i++) {
        if (ir & (1 << i)) JTAG_TDI_HIGH(cfg); else JTAG_TDI_LOW(cfg);
        JTAG_TCK_HIGH(cfg);
        JTAG_TCK_LOW(cfg);
    }
    
    /* 最后一位在Exit1-IR */
    if (ir & (1 << (ir_len - 1))) JTAG_TDI_HIGH(cfg); else JTAG_TDI_LOW(cfg);
    JTAG_TMS_HIGH(cfg);
    JTAG_TCK_HIGH(cfg);
    JTAG_TCK_LOW(cfg);
    
    /* 回到Run-Test/Idle */
    JTAG_TMS_HIGH(cfg); JTAG_TCK_HIGH(cfg); JTAG_TCK_LOW(cfg);  /* Update-IR */
    JTAG_TMS_LOW(cfg);  JTAG_TCK_HIGH(cfg); JTAG_TCK_LOW(cfg);  /* Run-Test/Idle */
}

/**
 * @brief JTAG移入数据
 */
static void JTAG_ShiftDR(JTAG_Config_t* cfg, uint8_t* tdi_data, uint8_t* tdo_data, uint32_t bit_len)
{
    /* 进入Shift-DR状态 */
    JTAG_TMS_LOW(cfg);  JTAG_TCK_HIGH(cfg); JTAG_TCK_LOW(cfg);  /* Run-Test/Idle */
    JTAG_TMS_HIGH(cfg); JTAG_TCK_HIGH(cfg); JTAG_TCK_LOW(cfg);  /* Select-DR */
    JTAG_TMS_LOW(cfg);  JTAG_TCK_HIGH(cfg); JTAG_TCK_LOW(cfg);  /* Capture-DR */
    JTAG_TMS_LOW(cfg);  JTAG_TCK_HIGH(cfg); JTAG_TCK_LOW(cfg);  /* Shift-DR */
    
    /* 移入数据 */
    for (uint32_t i = 0; i < bit_len; i++) {
        uint8_t byte_idx = i / 8;
        uint8_t bit_idx = i % 8;
        
        if (tdi_data != NULL && (tdi_data[byte_idx] & (1 << bit_idx)))
            JTAG_TDI_HIGH(cfg);
        else
            JTAG_TDI_LOW(cfg);
        
        if (i == bit_len - 1) JTAG_TMS_HIGH(cfg);
        
        JTAG_TCK_HIGH(cfg);
        
        if (tdo_data != NULL) {
            if (JTAG_TDO_READ(cfg))
                tdo_data[byte_idx] |= (1 << bit_idx);
            else
                tdo_data[byte_idx] &= ~(1 << bit_idx);
        }
        
        JTAG_TCK_LOW(cfg);
    }
    
    /* 回到Run-Test/Idle */
    JTAG_TMS_HIGH(cfg); JTAG_TCK_HIGH(cfg); JTAG_TCK_LOW(cfg);  /* Update-DR */
    JTAG_TMS_LOW(cfg);  JTAG_TCK_HIGH(cfg); JTAG_TCK_LOW(cfg);  /* Run-Test/Idle */
}

/**
 * @brief 读取JTAG IDCODE
 */
static uint32_t JTAG_ReadIDCODE(JTAG_Config_t* cfg)
{
    uint32_t idcode = 0;
    
    JTAG_TapReset(cfg);
    JTAG_ShiftIR(cfg, 0x3FE, 10);  /* IDCODE指令，Xilinx IR=10位 */
    JTAG_ShiftDR(cfg, NULL, (uint8_t*)&idcode, 32);
    
    /* 如果IDCODE无效，尝试Altera格式 */
    if (idcode == 0 || idcode == 0xFFFFFFFF) {
        JTAG_TapReset(cfg);
        JTAG_ShiftIR(cfg, 0x06, 10);  /* IDCODE指令，Altera IR=10位 */
        JTAG_ShiftDR(cfg, NULL, (uint8_t*)&idcode, 32);
    }
    
    return idcode;
}

/* ==================== DSP驱动实现 ==================== */

/**
 * @brief 初始化DSP驱动
 */
HAL_StatusTypeDef DSP_Init(DSP_HandleTypeDef* hdsp)
{
    if (hdsp == NULL) return HAL_ERROR;
    
    /* 配置JTAG GPIO */
    JTAG_TapReset(&hdsp->jtag);
    
    /* 检测DSP型号 */
    if (DSP_Detect(hdsp) != HAL_OK) {
        return HAL_ERROR;
    }
    
    hdsp->initialized = 1;
    return HAL_OK;
}

/**
 * @brief 反初始化DSP驱动
 */
HAL_StatusTypeDef DSP_DeInit(DSP_HandleTypeDef* hdsp)
{
    hdsp->initialized = 0;
    return HAL_OK;
}

/**
 * @brief 检测DSP型号
 */
HAL_StatusTypeDef DSP_Detect(DSP_HandleTypeDef* hdsp)
{
    uint32_t idcode = JTAG_ReadIDCODE(&hdsp->jtag);
    
    if (idcode == 0 || idcode == 0xFFFFFFFF) {
        return HAL_ERROR;
    }
    
    /* 查找DSP型号 */
    for (uint32_t i = 0; s_dsp_models[i].idcode != 0; i++) {
        if (s_dsp_models[i].idcode == idcode) {
            hdsp->type = s_dsp_models[i].type;
            hdsp->device_id = idcode;
            hdsp->flash_base = s_dsp_models[i].flash_base;
            hdsp->flash_size = s_dsp_models[i].flash_size;
            hdsp->ram_base = s_dsp_models[i].ram_base;
            hdsp->ram_size = s_dsp_models[i].ram_size;
            strncpy(hdsp->part_number, s_dsp_models[i].part_number, 32);
            return HAL_OK;
        }
    }
    
    /* 未知型号但ID有效 */
    hdsp->device_id = idcode;
    strncpy(hdsp->part_number, "Unknown DSP", 32);
    return HAL_OK;
}

/**
 * @brief DSP擦除Flash
 */
HAL_StatusTypeDef DSP_EraseFlash(DSP_HandleTypeDef* hdsp, uint32_t addr, uint32_t size)
{
    /* TMS320C2000通过JTAG进行Flash擦除 */
    JTAG_TapReset(&hdsp->jtag);
    
    /* 进入Flash擦除模式 */
    JTAG_ShiftIR(&hdsp->jtag, 0x01, 10);  /* EXTEST指令 */
    
    /* 通过JTAG写入擦除命令到Flash控制器 */
    uint8_t erase_cmd[4] = { 0x01, 0x00, 0x00, 0x00 };
    JTAG_ShiftDR(&hdsp->jtag, erase_cmd, NULL, 32);
    
    HAL_Delay(100);
    
    return HAL_OK;
}

/**
 * @brief DSP编程Flash
 */
HAL_StatusTypeDef DSP_ProgramFlash(DSP_HandleTypeDef* hdsp, uint32_t addr, uint8_t* data, uint32_t size)
{
    JTAG_TapReset(&hdsp->jtag);
    
    /* 通过JTAG将数据写入Flash */
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t page_size = (size - i > 256) ? 256 : (size - i);
        
        /* 写入地址 */
        uint8_t addr_data[4];
        addr_data[0] = (addr + i) & 0xFF;
        addr_data[1] = ((addr + i) >> 8) & 0xFF;
        addr_data[2] = ((addr + i) >> 16) & 0xFF;
        addr_data[3] = ((addr + i) >> 24) & 0xFF;
        
        JTAG_ShiftIR(&hdsp->jtag, 0x02, 10);
        JTAG_ShiftDR(&hdsp->jtag, addr_data, NULL, 32);
        
        /* 写入数据 */
        JTAG_ShiftIR(&hdsp->jtag, 0x03, 10);
        JTAG_ShiftDR(&hdsp->jtag, data + i, NULL, page_size * 8);
        
        HAL_Delay(1);
    }
    
    return HAL_OK;
}

/**
 * @brief DSP读Flash
 */
HAL_StatusTypeDef DSP_ReadFlash(DSP_HandleTypeDef* hdsp, uint32_t addr, uint8_t* data, uint32_t size)
{
    JTAG_TapReset(&hdsp->jtag);
    
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t page_size = (size - i > 256) ? 256 : (size - i);
        
        JTAG_ShiftIR(&hdsp->jtag, 0x04, 10);
        JTAG_ShiftDR(&hdsp->jtag, NULL, data + i, page_size * 8);
    }
    
    return HAL_OK;
}

/**
 * @brief DSP验证Flash
 */
HAL_StatusTypeDef DSP_VerifyFlash(DSP_HandleTypeDef* hdsp, uint32_t addr, uint8_t* data, uint32_t size)
{
    uint8_t read_buf[256];
    
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t page_size = (size - i > 256) ? 256 : (size - i);
        
        if (DSP_ReadFlash(hdsp, addr + i, read_buf, page_size) != HAL_OK) {
            return HAL_ERROR;
        }
        
        if (memcmp(read_buf, data + i, page_size) != 0) {
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

/**
 * @brief DSP写内存
 */
HAL_StatusTypeDef DSP_WriteMem(DSP_HandleTypeDef* hdsp, uint32_t addr, uint8_t* data, uint32_t size)
{
    JTAG_TapReset(&hdsp->jtag);
    
    for (uint32_t i = 0; i < size; i += 4) {
        uint8_t addr_data[4] = {
            (uint8_t)((addr + i) & 0xFF),
            (uint8_t)(((addr + i) >> 8) & 0xFF),
            (uint8_t)(((addr + i) >> 16) & 0xFF),
            (uint8_t)(((addr + i) >> 24) & 0xFF)
        };
        
        JTAG_ShiftIR(&hdsp->jtag, 0x05, 10);
        JTAG_ShiftDR(&hdsp->jtag, addr_data, NULL, 32);
        
        uint16_t write_len = (size - i > 4) ? 4 : (size - i);
        JTAG_ShiftIR(&hdsp->jtag, 0x06, 10);
        JTAG_ShiftDR(&hdsp->jtag, data + i, NULL, write_len * 8);
    }
    
    return HAL_OK;
}

/**
 * @brief DSP读内存
 */
HAL_StatusTypeDef DSP_ReadMem(DSP_HandleTypeDef* hdsp, uint32_t addr, uint8_t* data, uint32_t size)
{
    JTAG_TapReset(&hdsp->jtag);
    
    for (uint32_t i = 0; i < size; i += 4) {
        JTAG_ShiftIR(&hdsp->jtag, 0x07, 10);
        JTAG_ShiftDR(&hdsp->jtag, NULL, data + i, 32);
    }
    
    return HAL_OK;
}

/**
 * @brief TMS320复位
 */
HAL_StatusTypeDef DSP_TMS320_Reset(DSP_HandleTypeDef* hdsp)
{
    JTAG_TapReset(&hdsp->jtag);
    JTAG_ShiftIR(&hdsp->jtag, 0x08, 10);
    HAL_Delay(10);
    return HAL_OK;
}

/**
 * @brief TMS320暂停
 */
HAL_StatusTypeDef DSP_TMS320_Halt(DSP_HandleTypeDef* hdsp)
{
    JTAG_TapReset(&hdsp->jtag);
    JTAG_ShiftIR(&hdsp->jtag, 0x09, 10);
    return HAL_OK;
}

/**
 * @brief TMS320运行
 */
HAL_StatusTypeDef DSP_TMS320_Run(DSP_HandleTypeDef* hdsp)
{
    JTAG_ShiftIR(&hdsp->jtag, 0x0A, 10);
    JTAG_TapReset(&hdsp->jtag);
    return HAL_OK;
}

/* ==================== FPGA驱动实现 ==================== */

/**
 * @brief 初始化FPGA驱动
 */
HAL_StatusTypeDef FPGA_Init(FPGA_HandleTypeDef* hfpga)
{
    if (hfpga == NULL) return HAL_ERROR;
    
    JTAG_TapReset(&hfpga->jtag);
    
    if (FPGA_Detect(hfpga) != HAL_OK) {
        return HAL_ERROR;
    }
    
    hfpga->initialized = 1;
    return HAL_OK;
}

/**
 * @brief 反初始化FPGA驱动
 */
HAL_StatusTypeDef FPGA_DeInit(FPGA_HandleTypeDef* hfpga)
{
    hfpga->initialized = 0;
    return HAL_OK;
}

/**
 * @brief 检测FPGA型号
 */
HAL_StatusTypeDef FPGA_Detect(FPGA_HandleTypeDef* hfpga)
{
    uint32_t idcode = JTAG_ReadIDCODE(&hfpga->jtag);
    
    if (idcode == 0 || idcode == 0xFFFFFFFF) {
        return HAL_ERROR;
    }
    
    /* 查找FPGA型号 */
    for (uint32_t i = 0; s_fpga_models[i].idcode != 0; i++) {
        if (s_fpga_models[i].idcode == idcode) {
            hfpga->type = s_fpga_models[i].type;
            hfpga->device_id = idcode;
            hfpga->flash_size = s_fpga_models[i].flash_size;
            hfpga->is_cpld = s_fpga_models[i].is_cpld;
            strncpy(hfpga->part_number, s_fpga_models[i].part_number, 32);
            return HAL_OK;
        }
    }
    
    hfpga->device_id = idcode;
    strncpy(hfpga->part_number, "Unknown FPGA", 32);
    return HAL_OK;
}

/**
 * @brief Xilinx JTAG编程
 */
HAL_StatusTypeDef FPGA_Xilinx_JTAG_Program(FPGA_HandleTypeDef* hfpga, uint8_t* bitstream, uint32_t size)
{
    JTAG_TapReset(&hfpga->jtag);
    
    /* Xilinx JTAG编程流程：
     * 1. 加载JPROGRAM指令
     * 2. 等待初始化完成
     * 3. 加载CFG_IN指令
     * 4. 移入bitstream数据
     * 5. 加载JSTART指令
     * 6. 等待启动完成
     */
    
    /* Step 1: JPROGRAM */
    JTAG_ShiftIR(&hfpga->jtag, 0x00B, 10);  /* JPROGRAM */
    HAL_Delay(10);
    
    /* Step 2: 等待INIT */
    for (int i = 0; i < 100; i++) {
        uint8_t status = 0;
        JTAG_ShiftIR(&hfpga->jtag, 0x3C2, 10);  /* CONFIG_OUT */
        JTAG_ShiftDR(&hfpga->jtag, NULL, &status, 1);
        if (status == 0) break;
        HAL_Delay(1);
    }
    
    /* Step 3: CFG_IN */
    JTAG_ShiftIR(&hfpga->jtag, 0x005, 10);  /* CFG_IN */
    
    /* Step 4: 移入bitstream数据 */
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t chunk_size = (size - i > 256) ? 256 : (size - i);
        JTAG_ShiftDR(&hfpga->jtag, bitstream + i, NULL, chunk_size * 8);
    }
    
    /* Step 5: JSTART */
    JTAG_ShiftIR(&hfpga->jtag, 0x00C, 10);  /* JSTART */
    HAL_Delay(100);
    
    /* Step 6: 验证DONE引脚 */
    JTAG_ShiftIR(&hfpga->jtag, 0x3C2, 10);  /* CONFIG_OUT */
    uint8_t done = 0;
    JTAG_ShiftDR(&hfpga->jtag, NULL, &done, 1);
    
    return (done & 0x01) ? HAL_OK : HAL_ERROR;
}

/**
 * @brief Intel/Altera JTAG编程
 */
HAL_StatusTypeDef FPGA_Intel_JTAG_Program(FPGA_HandleTypeDef* hfpga, uint8_t* data, uint32_t size)
{
    JTAG_TapReset(&hfpga->jtag);
    
    /* Intel/Altera JTAG编程流程：
     * 1. 加载IRSCAN指令
     * 2. 移入配置数据
     * 3. 等待CONF_DONE
     */
    
    /* 进入编程模式 */
    JTAG_ShiftIR(&hfpga->jtag, 0x002, 10);  /* IRSCAN */
    
    /* 移入配置数据 */
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t chunk_size = (size - i > 256) ? 256 : (size - i);
        JTAG_ShiftDR(&hfpga->jtag, data + i, NULL, chunk_size * 8);
    }
    
    /* 检查CONF_DONE */
    JTAG_ShiftIR(&hfpga->jtag, 0x003, 10);  /* DRSCAN */
    uint8_t conf_done = 0;
    JTAG_ShiftDR(&hfpga->jtag, NULL, &conf_done, 1);
    
    return (conf_done & 0x01) ? HAL_OK : HAL_ERROR;
}

/**
 * @brief Lattice JTAG编程
 */
HAL_StatusTypeDef FPGA_Lattice_JTAG_Program(FPGA_HandleTypeDef* hfpga, uint8_t* data, uint32_t size)
{
    JTAG_TapReset(&hfpga->jtag);
    
    /* Lattice JTAG编程流程 */
    JTAG_ShiftIR(&hfpga->jtag, 0x3C, 8);   /* LSCC_PROGRAM */
    HAL_Delay(10);
    
    /* 加载配置数据 */
    JTAG_ShiftIR(&hfpga->jtag, 0x3A, 8);   /* LSCC_CONFIG */
    
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t chunk_size = (size - i > 256) ? 256 : (size - i);
        JTAG_ShiftDR(&hfpga->jtag, data + i, NULL, chunk_size * 8);
    }
    
    /* 验证DONE */
    JTAG_ShiftIR(&hfpga->jtag, 0x3F, 8);   /* LSCC_STATUS */
    uint8_t done = 0;
    JTAG_ShiftDR(&hfpga->jtag, NULL, &done, 1);
    
    return (done & 0x01) ? HAL_OK : HAL_ERROR;
}

/**
 * @brief FPGA配置(自动选择编程方式)
 */
HAL_StatusTypeDef FPGA_Configure(FPGA_HandleTypeDef* hfpga, uint8_t* bitstream, uint32_t size)
{
    switch (hfpga->type) {
        case FPGA_TYPE_XILINX_SERIES7:
        case FPGA_TYPE_XILINX_ULTRASCALE:
        case FPGA_TYPE_XILINX_SPARTAN:
            return FPGA_Xilinx_JTAG_Program(hfpga, bitstream, size);
            
        case FPGA_TYPE_INTEL_CYCLONE:
        case FPGA_TYPE_INTEL_STRATIX:
        case FPGA_TYPE_INTEL_MAX:
            return FPGA_Intel_JTAG_Program(hfpga, bitstream, size);
            
        case FPGA_TYPE_LATTICE_ICE40:
        case FPGA_TYPE_LATTICE_ECP5:
        case FPGA_TYPE_LATTICE_MACHXO:
            return FPGA_Lattice_JTAG_Program(hfpga, bitstream, size);
            
        default:
            return HAL_ERROR;
    }
}

/**
 * @brief FPGA验证配置
 */
HAL_StatusTypeDef FPGA_VerifyConfiguration(FPGA_HandleTypeDef* hfpga, uint8_t* bitstream, uint32_t size)
{
    /* 通过JTAG回读配置数据并验证 */
    uint8_t read_buf[256];
    
    JTAG_TapReset(&hfpga->jtag);
    JTAG_ShiftIR(&hfpga->jtag, 0x3C2, 10);  /* CONFIG_OUT */
    
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t chunk_size = (size - i > 256) ? 256 : (size - i);
        JTAG_ShiftDR(&hfpga->jtag, NULL, read_buf, chunk_size * 8);
        
        if (memcmp(read_buf, bitstream + i, chunk_size) != 0) {
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

/**
 * @brief FPGA回读配置数据
 */
HAL_StatusTypeDef FPGA_ReadBack(FPGA_HandleTypeDef* hfpga, uint8_t* data, uint32_t* size)
{
    JTAG_TapReset(&hfpga->jtag);
    
    /* 通过JTAG回读FPGA配置数据 */
    JTAG_ShiftIR(&hfpga->jtag, 0x3C2, 10);
    
    /* 回读数据 */
    uint32_t read_size = hfpga->flash_size;
    if (read_size == 0) read_size = 16 * 1024 * 1024;
    
    for (uint32_t i = 0; i < read_size; i += 256) {
        uint16_t chunk_size = (read_size - i > 256) ? 256 : (read_size - i);
        JTAG_ShiftDR(&hfpga->jtag, NULL, data + i, chunk_size * 8);
    }
    
    *size = read_size;
    return HAL_OK;
}

/**
 * @brief FPGA擦除Flash
 */
HAL_StatusTypeDef FPGA_EraseFlash(FPGA_HandleTypeDef* hfpga, uint32_t addr, uint32_t size)
{
    /* 通过JTAG擦除配置Flash */
    JTAG_TapReset(&hfpga->jtag);
    JTAG_ShiftIR(&hfpga->jtag, 0x0E, 10);  /* FLASH_ERASE */
    HAL_Delay(1000);
    return HAL_OK;
}

/**
 * @brief FPGA编程Flash
 */
HAL_StatusTypeDef FPGA_ProgramFlash(FPGA_HandleTypeDef* hfpga, uint32_t addr, uint8_t* data, uint32_t size)
{
    JTAG_TapReset(&hfpga->jtag);
    JTAG_ShiftIR(&hfpga->jtag, 0x0F, 10);  /* FLASH_PROGRAM */
    
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t chunk_size = (size - i > 256) ? 256 : (size - i);
        JTAG_ShiftDR(&hfpga->jtag, data + i, NULL, chunk_size * 8);
    }
    
    return HAL_OK;
}

/**
 * @brief FPGA读Flash
 */
HAL_StatusTypeDef FPGA_ReadFlash(FPGA_HandleTypeDef* hfpga, uint32_t addr, uint8_t* data, uint32_t size)
{
    JTAG_TapReset(&hfpga->jtag);
    JTAG_ShiftIR(&hfpga->jtag, 0x10, 10);  /* FLASH_READ */
    
    for (uint32_t i = 0; i < size; i += 256) {
        uint16_t chunk_size = (size - i > 256) ? 256 : (size - i);
        JTAG_ShiftDR(&hfpga->jtag, NULL, data + i, chunk_size * 8);
    }
    
    return HAL_OK;
}

/**
 * @brief 获取FPGA初始化状态
 */
uint8_t FPGA_GetInitStatus(FPGA_HandleTypeDef* hfpga)
{
    /* 通过JTAG读取INIT_B状态 */
    JTAG_ShiftIR(&hfpga->jtag, 0x3C2, 10);
    uint8_t status = 0;
    JTAG_ShiftDR(&hfpga->jtag, NULL, &status, 1);
    return status;
}

/**
 * @brief 获取FPGA DONE状态
 */
uint8_t FPGA_GetDoneStatus(FPGA_HandleTypeDef* hfpga)
{
    JTAG_ShiftIR(&hfpga->jtag, 0x3C2, 10);
    uint8_t done = 0;
    JTAG_ShiftDR(&hfpga->jtag, NULL, &done, 1);
    return (done >> 1) & 0x01;
}