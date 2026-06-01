# ARM DAP层设计文档

## 1. DAP概述

Debug Access Port (DAP) 是ARM调试架构的核心组件，提供对芯片调试资源的访问接口。DAP通过SW-DP或JTAG-DP连接到调试器，允许访问：

- ARM CoreSight调试组件
- Cortex-M处理器内核寄存器
- 系统内存和外设
- Flash编程接口

## 2. DAP架构

```
┌─────────────────────────────────────────┐
│         Debug Host (PC)                 │
│  ┌─────────────────────────────────┐  │
│  │  Debugger Software              │  │
│  └─────────────────────────────────┘  │
└─────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│     Debug Port (SW-DP / JTAG-DP)       │
│  ┌─────────────────────────────────┐  │
│  │  SWD / JTAG Protocol Layer     │  │
│  └─────────────────────────────────┘  │
└─────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│  Access Port (AP)                       │
│  ┌────────────┐ ┌────────────┐         │
│  │  AHB-AP   │ │  APB-AP   │  ...   │
│  │ (Memory)  │ │ (Periph)  │         │
│  └────────────┘ └────────────┘         │
└─────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────┐
│    System Bus (AHB / APB / AXI)        │
└─────────────────────────────────────────┘
```

## 3. DP寄存器

### 3.1 Debug Port寄存器

| 地址 | 名称 | 描述 |
|------|------|------|
| 0x00 | IDCODE | 器件标识码 |
| 0x04 | CTRL/STAT | 控制和状态寄存器 |
| 0x08 | RESEND | 上次读取结果重发 |
| 0x0C | SELECT | AP选择寄存器 |
| 0x10 | RDBUFF | 读缓冲区 |

### 3.2 DP寄存器详解

#### IDCODE寄存器 (0x00)
```
31-28: REVISION   - 修订版本
27-20: PARTNO     - 器件型号
19-17: MANUFACTURER - 制造商ID
16-1 : UNIQUEID   - 唯一标识
0    : VERSION    - 版本号
```

#### CTRL/STAT寄存器 (0x04)
```
31-30: STICKYORUN - 过量运行标志
29   : STICKYCMP  - 比较标志
28   : STICKYERR  - 错误标志
27   : WAITUP     - 等待up
26   : WDATAERR   - 数据错误
25   : READOK     - 读取OK
24   : OVERUN     - 过量运行
23-12: Txn counter - 事务计数器
11   : TRNNMOD    - 事务模式
10-8 : MASKLANE   - 掩码通道
7    : ORUNDETECT - 过量运行检测
6    : STRESET    - 系统复位
5    : HALTCLK    - 停止时钟
4    : CSYSACKREQ - 系统确认请求
3    : CSYSREQ    - 系统请求
2    : CDBGPWRUPREQ - 调试电源请求
1    : CDBGPWRUPACK - 调试电源确认
0    : KEY        - 必须为1
```

## 4. AP寄存器

### 4.1 AHB-AP (Memory Access Port)

| 地址 | 名称 | 描述 |
|------|------|------|
| 0x00 | CSW | 控制/状态字 |
| 0x04 | TAR | 传输地址寄存器 |
| 0x08 | DRW | 数据读/写寄存器 |
| 0x0C | BD0 | 银行寄存器0 |
| 0x10 | BD1 | 银行寄存器1 |
| 0x14 | BD2 | 银行寄存器2 |
| 0x18 | BD3 | 银行寄存器3 |
| 0xFC | IDR | AP标识寄存器 |

### 4.2 AHB-AP CSW寄存器

```
31-28: PROT      - 保护标识
27-24: DEVICEEN  - 设备使能
23-16: Reserved  - 保留
15-8 : MINORSVCCLKS - 从时钟数
7-6   : MASTERSERVERTY - 主服务器类型
5     : HAIPERFMON - HAI性能监视器
4     : BIGENDIAN - 大端模式
3-2   : PROTOCOL  - 协议类型
1     : ADDINCR    - 地址递增模式
0     : DBGTHRST   - 调试异常请求
```

## 5. DAP实现接口

```c
typedef struct {
    uint32_t (*read_dp)(uint8_t addr);
    void (*write_dp)(uint8_t addr, uint32_t data);
    uint32_t (*read_ap)(uint8_t addr);
    void (*write_ap)(uint8_t addr, uint32_t data);
    void (*write_ap_addr)(uint32_t addr);
    uint32_t (*read_ap_data)(void);
    void (*write_ap_data)(uint32_t data);
} DAP_Ops_TypeDef;

typedef struct {
    uint32_t dp_idcode;
    uint32_t ap_idr;
    uint8_t ap_count;
    uint8_t selected_ap;
    uint8_t protocol;  // 0=SWD, 1=JTAG
} DAP_Info_TypeDef;

HAL_StatusTypeDef DAP_Init(DAP_Info_TypeDef *info);
HAL_StatusTypeDef DAP_Connect(DAP_Info_TypeDef *info);
HAL_StatusTypeDef DAP_Disconnect(void);
uint32_t DAP_ReadDP(uint8_t addr);
void DAP_WriteDP(uint8_t addr, uint32_t data);
uint32_t DAP_ReadAP(uint8_t addr);
void DAP_WriteAP(uint8_t addr, uint32_t data);
HAL_StatusTypeDef DAP_WriteMem(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef DAP_ReadMem(uint32_t addr, uint8_t *data, uint32_t size);
HAL_StatusTypeDef DAP_WriteReg(uint8_t core_reg, uint32_t value);
uint32_t DAP_ReadReg(uint8_t core_reg);
```

## 6. DAP操作流程

### 6.1 连接流程

```
1. 初始化SWD/JTAG接口
2. 读取DP IDCODE，验证连接
3. 配置CTRL/STAT寄存器
4. 选择AP (通常是AHB-AP)
5. 读取AP IDR，验证AP存在
6. 配置AP CSW寄存器
7. 连接成功
```

### 6.2 内存读取流程

```
1. 选择AP
2. 写入AP TAR (目标地址)
3. 读取AP DRW (触发传输)
4. 读取DP RDBUFF (获取数据)
5. 重复步骤2-4进行连续读取
```

### 6.3 Flash编程流程

```
1. 解锁Flash编程接口
2. 擦除Flash扇区/页
3. 写入Flash数据
4. 验证写入
5. 锁定Flash
```

## 7. 支持的ARM芯片

| 系列 | 内核 | 调试接口 | 状态 |
|------|------|---------|------|
| STM32F0 | Cortex-M0 | SW-DP | ⬜ |
| STM32F1 | Cortex-M3 | SW-DP | ⬜ |
| STM32F2 | Cortex-M3 | SW-DP | ⬜ |
| STM32F3 | Cortex-M4 | SW-DP | ⬜ |
| STM32F4 | Cortex-M4 | SW-DP | ⬜ |
| STM32F7 | Cortex-M7 | SW-DP | ⬜ |
| STM32H7 | Cortex-M7 | SW-DP | ⬜ |
| STM32L0 | Cortex-M0+ | SW-DP | ⬜ |
| STM32L4 | Cortex-M4 | SW-DP | ⬜ |
| STM32L5 | Cortex-M33 | SW-DP | ⬜ |
| STM32G0 | Cortex-M0+ | SW-DP | ⬜ |
| STM32G4 | Cortex-M4 | SW-DP | ⬜ |

## 8. 错误处理

### 8.1 常见错误

| 错误码 | 描述 | 处理方式 |
|--------|------|---------|
| DAP_ERR_OK | 无错误 | - |
| DAP_ERR_NOT_CONNECTED | 未连接 | 重新连接 |
| DAP_ERR_TIMEOUT | 超时 | 重试或检查硬件 |
| DAP_ERR_WDATA_ERR | 写数据错误 | 重试 |
| DAP_ERR_STICKY_ERR | 粘性错误 | 清除错误标志 |
| DAP_ERR_FAULT | 总线Fault | 检查地址有效性 |

### 8.2 错误恢复

```c
HAL_StatusTypeDef DAP_ClearErrors(void)
{
    uint32_t ctrl_stat = DAP_ReadDP(0x04);
    DAP_WriteDP(0x04, ctrl_stat | (1 << 28));  // Clear sticky error
    return HAL_OK;
}
```

## 9. 性能优化

### 9.1 批量读取优化

```c
HAL_StatusTypeDef DAP_BurstRead(uint32_t addr, uint8_t *data, uint32_t size)
{
    DAP_WriteAP(0x00, 0x23000012);  // CSW: Auto-increment
    DAP_WriteAP(0x04, addr);        // TAR

    for (uint32_t i = 0; i < size; i += 4) {
        DAP_ReadAP(0x0C);           // DRW triggers transfer
        uint32_t value = DAP_ReadDP(0x0C);  // RDBUFF
        data[i]     = value & 0xFF;
        data[i + 1] = (value >> 8) & 0xFF;
        data[i + 2] = (value >> 16) & 0xFF;
        data[i + 3] = (value >> 24) & 0xFF;
    }

    return HAL_OK;
}
```

### 9.2 速度配置

| SWD频率 | 适用场景 | 稳定性 |
|---------|---------|--------|
| 1MHz | 初始化和调试 | 最高 |
| 4MHz | 通用编程 | 高 |
| 10MHz | 快速编程 | 中 |
| 20MHz | 高速读取 | 需要验证 |

## 10. 测试计划

### 10.1 DAP功能测试

| 测试项目 | 测试内容 | 验收标准 |
|---------|---------|----------|
| 连接测试 | DP/AP识别 | 100%通过 |
| 寄存器读写 | DP/AP寄存器访问 | 100%通过 |
| 内存读写 | SRAM/外设读写 | 100%通过 |
| Flash编程 | STM32全系列测试 | >99%成功率 |

### 10.2 兼容性测试

- STM32全系列 (159款)
- 其他ARM Cortex-M芯片 (待扩展)

## 11. 文档版本

- 版本: v1.0
- 创建日期: 2026-06-01
- 最后更新: 2026-06-01
- 状态: 草稿
