/**
 ******************************************************************************
 * @file    core_cal.c
 * @brief   ARM 内核抽象层 (Core Abstraction Layer) 实现
 ******************************************************************************
 */

#include "core_cal.h"
#include "dap.h"
#include <string.h>

// 全局变量
Core_Info_TypeDef g_core_info;
Core_Ops_TypeDef g_core_ops;

// 内部函数声明
static HAL_StatusTypeDef Core_M_Init(void);
static HAL_StatusTypeDef Core_M_Detect(Core_Info_TypeDef *info);
static HAL_StatusTypeDef Core_M_Reset(void);
static Core_State_TypeDef Core_M_GetState(void);
static HAL_StatusTypeDef Core_M_Halt(void);
static HAL_StatusTypeDef Core_M_Resume(void);
static HAL_StatusTypeDef Core_M_Step(void);
static HAL_StatusTypeDef Core_M_SetPC(uint32_t pc);
static uint32_t Core_M_GetPC(void);
static uint32_t Core_M_GetReg(uint8_t reg_index);
static HAL_StatusTypeDef Core_M_SetReg(uint8_t reg_index, uint32_t value);

// 读取CPUID
uint32_t Core_ReadCPUID(void)
{
    uint32_t cpuid = 0;
    DAP_ReadWord(CPUID_ADDR, &cpuid);
    return cpuid;
}

// Cortex-M初始化
static HAL_StatusTypeDef Core_M_Init(void)
{
    // 通过DAP使能调试 - 写入目标芯片的CoreDebug_DHCSR寄存器
    return DAP_WriteWord(COREDEBUG_DHCSR_ADDR, DHCSR_DBGKEY | DHCSR_C_DEBUGEN);
}

// Cortex-M内核检测
static HAL_StatusTypeDef Core_M_Detect(Core_Info_TypeDef *info)
{
    uint32_t cpuid = Core_ReadCPUID();
    uint32_t implementor = (cpuid >> 24) & 0x7F;
    uint32_t variant = (cpuid >> 20) & 0x0F;
    uint32_t architecture = (cpuid >> 16) & 0x0F;
    uint32_t partno = (cpuid >> 4) & 0xFFF;
    uint32_t revision = cpuid & 0x0F;

    info->idcode = cpuid;

    if (architecture == 0xC) {
        // Cortex-M系列
        switch (partno) {
            case 0xC60:
                info->type = CORE_TYPE_CORTEX_M0;
                strcpy(info->name, "Cortex-M0");
                info->has_fpu = 0;
                info->has_dsp = 0;
                break;
            case 0xC61:
                info->type = CORE_TYPE_CORTEX_M0P;
                strcpy(info->name, "Cortex-M0+");
                info->has_fpu = 0;
                info->has_dsp = 0;
                break;
            case 0xC20:
                info->type = CORE_TYPE_CORTEX_M1;
                strcpy(info->name, "Cortex-M1");
                info->has_fpu = 0;
                info->has_dsp = 0;
                break;
            case 0xC23:
                info->type = CORE_TYPE_CORTEX_M3;
                strcpy(info->name, "Cortex-M3");
                info->has_fpu = 0;
                info->has_dsp = 0;
                break;
            case 0xC24:
                info->type = CORE_TYPE_CORTEX_M4;
                strcpy(info->name, "Cortex-M4");
                info->has_dsp = 1;
                // 检查FPU
                {
                    uint32_t cpacr = 0;
                    DAP_ReadWord(0xE000ED88, &cpacr);
                    info->has_fpu = ((cpacr >> 20) & 0xF) != 0;
                }
                break;
            case 0xC27:
                info->type = CORE_TYPE_CORTEX_M7;
                strcpy(info->name, "Cortex-M7");
                info->has_fpu = 1;
                info->has_dsp = 1;
                break;
            case 0xC28:
                info->type = CORE_TYPE_CORTEX_M33;
                strcpy(info->name, "Cortex-M33");
                info->has_fpu = 1;
                info->has_dsp = 1;
                break;
            default:
                info->type = CORE_TYPE_UNKNOWN;
                strcpy(info->name, "Unknown");
                break;
        }
    } else if (architecture == 0xF) {
        // Cortex-A系列
        switch (partno) {
            case 0xC07:
                info->type = CORE_TYPE_CORTEX_A7;
                strcpy(info->name, "Cortex-A7");
                break;
            case 0xC09:
                info->type = CORE_TYPE_CORTEX_A9;
                strcpy(info->name, "Cortex-A9");
                break;
            case 0xC0D:
                info->type = CORE_TYPE_CORTEX_A53;
                strcpy(info->name, "Cortex-A53");
                break;
            case 0xC0F:
                info->type = CORE_TYPE_CORTEX_A72;
                strcpy(info->name, "Cortex-A72");
                break;
            default:
                info->type = CORE_TYPE_UNKNOWN;
                strcpy(info->name, "Unknown");
                break;
        }
    } else {
        info->type = CORE_TYPE_UNKNOWN;
        strcpy(info->name, "Unknown");
    }

    info->has_mpu = 1;  // 假设都有MPU
    info->has_mmu = (architecture == 0xF);  // Cortex-A有MMU

    // 默认内存映射（可由芯片驱动覆盖）
    info->rom_base = 0x00000000;
    info->rom_size = 0x00100000;  // 1MB
    info->ram_base = 0x20000000;
    info->ram_size = 0x00020000;  // 128KB

    return HAL_OK;
}

// Cortex-M复位
static HAL_StatusTypeDef Core_M_Reset(void)
{
    // 通过DAP写入目标芯片的SCB_AIRCR寄存器
    return DAP_WriteWord(SCB_AIRCR_ADDR, SCB_AIRCR_VECTKEY | SCB_AIRCR_SYSRESETREQ);
}

// Cortex-M获取状态
static Core_State_TypeDef Core_M_GetState(void)
{
    uint32_t dhcsr = 0;
    DAP_ReadWord(COREDEBUG_DHCSR_ADDR, &dhcsr);

    if (dhcsr & DHCSR_S_RESET_ST) {
        return CORE_STATE_RESET;
    } else if (dhcsr & DHCSR_S_HALT) {
        return CORE_STATE_HALTED;
    } else if (!(dhcsr & DHCSR_S_SLEEP)) {
        return CORE_STATE_RUNNING;
    } else {
        return CORE_STATE_UNKNOWN;
    }
}

// Cortex-M暂停
static HAL_StatusTypeDef Core_M_Halt(void)
{
    return DAP_WriteWord(COREDEBUG_DHCSR_ADDR, DHCSR_DBGKEY | DHCSR_C_DEBUGEN | DHCSR_C_HALT);
}

// Cortex-M恢复
static HAL_StatusTypeDef Core_M_Resume(void)
{
    return DAP_WriteWord(COREDEBUG_DHCSR_ADDR, DHCSR_DBGKEY | DHCSR_C_DEBUGEN);
}

// Cortex-M单步
static HAL_StatusTypeDef Core_M_Step(void)
{
    return DAP_WriteWord(COREDEBUG_DHCSR_ADDR, DHCSR_DBGKEY | DHCSR_C_DEBUGEN | DHCSR_C_STEP);
}

// Cortex-M设置PC
static HAL_StatusTypeDef Core_M_SetPC(uint32_t pc)
{
    return Core_M_SetReg(CORE_REG_PC, pc);
}

// Cortex-M获取PC
static uint32_t Core_M_GetPC(void)
{
    return Core_M_GetReg(CORE_REG_PC);
}

// 等待寄存器就绪 - 通过DAP轮询
static HAL_StatusTypeDef Core_Wait_RegReady(uint32_t timeout_us)
{
    uint32_t dhcsr;
    uint32_t start_time = 0; // TODO: 实现超时机制
    
    (void)timeout_us;
    
    do {
        DAP_ReadWord(COREDEBUG_DHCSR_ADDR, &dhcsr);
        if (dhcsr & DHCSR_S_REGRDY) {
            return HAL_OK;
        }
    } while (1);
    
    return HAL_TIMEOUT;
}

// Cortex-M获取寄存器
static uint32_t Core_M_GetReg(uint8_t reg_index)
{
    uint32_t value = 0;
    
    // 等待寄存器就绪
    Core_Wait_RegReady(10000);
    
    // 选择寄存器并触发读取
    DAP_WriteWord(COREDEBUG_DCRSR_ADDR, reg_index & DCRSR_REGSEL);
    
    // 等待寄存器就绪
    Core_Wait_RegReady(10000);
    
    // 读取数据
    DAP_ReadWord(COREDEBUG_DCRDR_ADDR, &value);
    
    return value;
}

// Cortex-M设置寄存器
static HAL_StatusTypeDef Core_M_SetReg(uint8_t reg_index, uint32_t value)
{
    // 等待寄存器就绪
    if (Core_Wait_RegReady(10000) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // 写入数据
    DAP_WriteWord(COREDEBUG_DCRDR_ADDR, value);
    
    // 等待寄存器就绪
    if (Core_Wait_RegReady(10000) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // 选择寄存器并触发写入
    DAP_WriteWord(COREDEBUG_DCRSR_ADDR, (reg_index & DCRSR_REGSEL) | DCRSR_REGWnR);
    
    // 等待操作完成
    if (Core_Wait_RegReady(10000) != HAL_OK) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

// 通用初始化
HAL_StatusTypeDef Core_Init(void)
{
    HAL_StatusTypeDef status;

    // 初始化DAP
    status = DAP_Init(DAP_PROTOCOL_SWD);
    if (status != HAL_OK) {
        return status;
    }

    // 连接DAP
    status = DAP_Connect();
    if (status != HAL_OK) {
        return status;
    }

    // 检测内核
    status = Core_Detect(&g_core_info);
    if (status != HAL_OK) {
        return status;
    }

    // 设置操作函数指针（默认Cortex-M）
    g_core_ops.init = Core_M_Init;
    g_core_ops.detect = Core_M_Detect;
    g_core_ops.reset = Core_M_Reset;
    g_core_ops.get_state = Core_M_GetState;
    g_core_ops.halt = Core_M_Halt;
    g_core_ops.resume = Core_M_Resume;
    g_core_ops.step = Core_M_Step;
    g_core_ops.set_pc = Core_M_SetPC;
    g_core_ops.get_pc = Core_M_GetPC;
    g_core_ops.get_reg = Core_M_GetReg;
    g_core_ops.set_reg = Core_M_SetReg;
    g_core_ops.read_memory = DAP_ReadMem;
    g_core_ops.write_memory = DAP_WriteMem;

    // 初始化内核
    if (g_core_ops.init) {
        g_core_ops.init();
    }

    return HAL_OK;
}

HAL_StatusTypeDef Core_DeInit(void)
{
    DAP_Disconnect();
    DAP_DeInit();
    memset(&g_core_info, 0, sizeof(g_core_info));
    return HAL_OK;
}

HAL_StatusTypeDef Core_Detect(Core_Info_TypeDef *info)
{
    if (g_core_ops.detect) {
        return g_core_ops.detect(info);
    }
    return Core_M_Detect(info);
}

HAL_StatusTypeDef Core_Reset(void)
{
    if (g_core_ops.reset) {
        return g_core_ops.reset();
    }
    return Core_M_Reset();
}

Core_State_TypeDef Core_GetState(void)
{
    if (g_core_ops.get_state) {
        return g_core_ops.get_state();
    }
    return Core_M_GetState();
}

HAL_StatusTypeDef Core_Halt(void)
{
    if (g_core_ops.halt) {
        return g_core_ops.halt();
    }
    return Core_M_Halt();
}

HAL_StatusTypeDef Core_Resume(void)
{
    if (g_core_ops.resume) {
        return g_core_ops.resume();
    }
    return Core_M_Resume();
}

HAL_StatusTypeDef Core_Step(void)
{
    if (g_core_ops.step) {
        return g_core_ops.step();
    }
    return Core_M_Step();
}

uint32_t Core_GetPC(void)
{
    if (g_core_ops.get_pc) {
        return g_core_ops.get_pc();
    }
    return Core_M_GetPC();
}

HAL_StatusTypeDef Core_SetPC(uint32_t pc)
{
    if (g_core_ops.set_pc) {
        return g_core_ops.set_pc(pc);
    }
    return Core_M_SetPC(pc);
}

uint32_t Core_GetRegister(uint8_t reg_index)
{
    if (g_core_ops.get_reg) {
        return g_core_ops.get_reg(reg_index);
    }
    return Core_M_GetReg(reg_index);
}

HAL_StatusTypeDef Core_SetRegister(uint8_t reg_index, uint32_t value)
{
    if (g_core_ops.set_reg) {
        return g_core_ops.set_reg(reg_index, value);
    }
    return Core_M_SetReg(reg_index, value);
}

HAL_StatusTypeDef Core_ReadMemory(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (g_core_ops.read_memory) {
        return g_core_ops.read_memory(addr, data, size);
    }
    return DAP_ReadMem(addr, data, size);
}

HAL_StatusTypeDef Core_WriteMemory(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (g_core_ops.write_memory) {
        return g_core_ops.write_memory(addr, data, size);
    }
    return DAP_WriteMem(addr, data, size);
}
