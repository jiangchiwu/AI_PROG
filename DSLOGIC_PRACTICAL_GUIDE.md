
# DSlogic DScope 实战指南

## 概述

本文档提供DSlogic DScope逻辑分析仪的完整操作流程，包括设备连接、信号采样、数据保存和数据分析的端到端实践指南。

---

## 目录

1. [设备连接与软件配置](#1-设备连接与软件配置)
2. [逻辑分析仪采样流程](#2-逻辑分析仪采样流程)
3. [SWD协议实战分析](#3-swd协议实战分析)
4. [数据保存与导出](#4-数据保存与导出)
5. [Python数据分析实战](#5-python数据分析实战)
6. [脚本自动化测试](#6-脚本自动化测试)

---

## 1. 设备连接与软件配置

### 1.1 硬件连接

**连接示意图:**

```
DSlogic DScope          目标开发板 (STM32)
─────────────────       ──────────────────
│ CH0 (CLK)    │───────>│ PA14 (SWDCLK) │
│ CH1 (DATA)   │───────>│ PA13 (SWDIO)  │
│ GND          │───────>│ GND           │
│ USB          │───────>│ PC USB        │
─────────────────       ──────────────────
```

**线缆要求:**
- 使用高质量杜邦线或同轴电缆
- 建议线缆长度不超过30cm
- GND必须连接以确保信号稳定

### 1.2 软件配置步骤

```
1. 启动 DSlogic 软件
2. 点击 "设备" → "连接设备"
3. 在弹出窗口中选择您的DSlogic设备
4. 配置采样参数：
   - 采样率: 100 MHz (SWD) / 50 MHz (JTAG)
   - 采样深度: 1M (建议)
   - 触发模式: 上升沿触发
5. 点击 "确定" 完成配置
```

### 1.3 通道设置

| 通道 | 标签 | 颜色 | 阈值 | 说明 |
|------|------|------|------|------|
| CH0 | SWD_CLK | 红色 | 1.4V | 调试时钟 |
| CH1 | SWD_IO | 蓝色 | 1.4V | 数据输入/输出 |
| CH2 | nRESET | 绿色 | 1.4V | 复位信号(可选) |
| CH3 | TRACE | 黄色 | 1.4V | 追踪信号(可选) |

---

## 2. 逻辑分析仪采样流程

### 2.1 触发配置

**步骤1: 设置触发条件**
```
1. 点击 "触发设置" 按钮
2. 选择 "边沿触发"
3. 设置:
   - 触发源: CH0 (SWD_CLK)
   - 触发类型: 上升沿
   - 触发电平: 高
4. 勾选 "使能触发"
5. 点击 "确定"
```

**步骤2: 设置触发延迟**
```
- 预触发深度: 20% (捕获触发前的数据)
- 后触发深度: 80% (捕获触发后的数据)
```

### 2.2 开始采样

```
方法1: 连续采样
┌─────────────────────────────────────┐
│ 1. 点击 "开始" 按钮                 │
│ 2. 设备持续采样直到手动停止          │
│ 3. 适合观察实时信号                 │
└─────────────────────────────────────┘

方法2: 单次触发采样
┌─────────────────────────────────────┐
│ 1. 点击 "单次" 按钮                 │
│ 2. 设备等待触发条件                 │
│ 3. 触发后自动停止并显示数据          │
│ 4. 适合捕获特定事件                 │
└─────────────────────────────────────┘

方法3: 序列触发采样
┌─────────────────────────────────────┐
│ 1. 点击 "触发设置"                  │
│ 2. 选择 "序列触发"                  │
│ 3. 设置触发序列 (如: 0101)          │
│ 4. 点击 "确定" 并开始采样           │
│ 5. 适合捕获特定协议模式             │
└─────────────────────────────────────┘
```

### 2.3 观察信号

**界面布局:**
```
┌─────────────────────────────────────────────┐
│ 时间轴 (Time Scale): 调整显示比例          │
│ 通道标签: CH0, CH1, CH2, CH3              │
│ 波形显示区: 实时信号波形                   │
│ 协议解码区: 自动解码的协议数据             │
└─────────────────────────────────────────────┘
```

**常用操作:**
- **滚轮缩放**: 调整时间轴比例
- **拖拽平移**: 查看不同时间段的数据
- **光标测量**: 点击并拖动测量时间间隔

---

## 3. SWD协议实战分析

### 3.1 协议解码配置

```
1. 点击 "协议解码" → "添加解码器"
2. 选择 "SWD" 协议
3. 配置参数:
   ┌─────────────────────────┐
   │ Clock Channel: CH0     │
   │ Data Channel: CH1      │
   │ Clock Edge: Rising     │
   │ Bit Order: LSB First   │
   │ Enable Decode: ✓       │
   └─────────────────────────┘
4. 点击 "确定"
```

### 3.2 预期解码结果

**SWD协议帧结构:**
```
┌─────────────────────────────────────────────────────────┐
│ SWD Frame Format                                       │
├─────────────┬─────────────┬─────────────┬─────────────┤
│  起始位(1)  │  读写位(1)  │  地址位(2)  │  奇偶校验(1) │
│     1       │    R/W      │   A[1:0]   │    PARITY    │
├─────────────┼─────────────┼─────────────┼─────────────┤
│     响应位(3)           │    数据位(32)              │
│    ACK[2:0]             │    DATA[31:0]              │
└───────────────────────────────────────────────────────┘
```

**解码示例:**
```
Time        SWD_CLK   SWD_IO    Decode
───────────────────────────────────────────────────────
0.000 µs    ↑         1         Start Bit
0.100 µs    ↑         0         Read (R/W=0)
0.200 µs    ↑         0         Addr[0]=0
0.300 µs    ↑         0         Addr[1]=0 → DPIDR
0.400 µs    ↑         1         Parity OK
0.500 µs    ↑         1         ACK=OK
0.600 µs    ↑         1         Data[0]
...         ...       ...       ...
3.500 µs    ↑         0         Data[31]
───────────────────────────────────────────────────────
Result: READ DPIDR → 0x1BA01477
```

### 3.3 时序验证

**SWD时序参数验证:**

| 参数 | 测量方法 | 期望值 |
|------|---------|--------|
| Tclk | 测量两个上升沿之间的时间 | 10-20 ns (50-100 MHz) |
| Tsu | 数据变化到时钟上升沿的时间 | ≥ 10 ns |
| Th | 时钟上升沿后数据保持时间 | ≥ 10 ns |

**使用光标测量:**
```
1. 点击工具栏的 "光标" 按钮
2. 在波形上点击两个点
3. 查看底部状态栏的时间差显示
4. 记录并分析测量结果
```

---

## 4. 数据保存与导出

### 4.1 保存格式对比

| 格式 | 扩展名 | 特点 | 适用场景 |
|------|--------|------|---------|
| DSlogic | .dslog | 原生格式，包含所有信息 | 后续分析和重新打开 |
| CSV | .csv | 通用文本格式 | Excel/Python分析 |
| VCD | .vcd | Verilog仿真格式 | EDA工具分析 |
| BIN | .bin | 二进制格式 | 程序处理 |
| PNG | .png | 图片格式 | 文档报告 |

### 4.2 导出步骤

```
方法1: 导出为CSV
┌─────────────────────────────────────┐
│ 1. 点击 "文件" → "导出"             │
│ 2. 选择 "CSV格式"                   │
│ 3. 设置导出范围:                     │
│    - 全部数据                        │
│    - 当前视图                        │
│    - 自定义范围                      │
│ 4. 设置导出选项:                     │
│    - 包含时间戳                      │
│    - 包含通道标签                    │
│    - 包含协议解码结果                │
│ 5. 选择保存路径和文件名              │
│ 6. 点击 "保存"                      │
└─────────────────────────────────────┘

方法2: 导出波形图片
┌─────────────────────────────────────┐
│ 1. 调整视图到合适的缩放比例          │
│ 2. 点击 "文件" → "导出图片"          │
│ 3. 选择图片格式: PNG/JPG/BMP        │
│ 4. 设置分辨率和质量                  │
│ 5. 点击 "保存"                      │
└─────────────────────────────────────┘
```

### 4.3 导出文件格式说明

**CSV文件结构:**
```csv
Time(ns),CH0,CH1,CH2,CH3,Decode
0,1,0,1,0,Start
100,0,0,1,0,Bit 0
200,1,1,1,0,Bit 1
300,0,1,1,0,Bit 2
...
```

**字段说明:**
- Time(ns): 时间戳，单位纳秒
- CH0-CH3: 各通道电平 (0=低, 1=高)
- Decode: 协议解码结果

---

## 5. Python数据分析实战

### 5.1 环境准备

```bash
# 安装必要的库
pip install pandas matplotlib numpy scipy
```

### 5.2 数据加载与预处理

```python
import pandas as pd
import numpy as np

# 加载CSV数据
df = pd.read_csv('swd_sample.csv')

# 查看数据结构
print("数据形状:", df.shape)
print("列名:", df.columns.tolist())
print("\n前5行数据:")
print(df.head())

# 数据预处理
# 转换时间单位 (ns → us)
df['Time_us'] = df['Time(ns)'] / 1000

# 计算时钟周期
clk_transitions = df[df['CH0'].diff() != 0]['Time(ns)']
clk_periods = np.diff(clk_transitions)

print(f"\n时钟周期统计:")
print(f"  平均周期: {np.mean(clk_periods):.2f} ns")
print(f"  最小周期: {np.min(clk_periods):.2f} ns")
print(f"  最大周期: {np.max(clk_periods):.2f} ns")
print(f"  频率: {1000 / np.mean(clk_periods):.2f} MHz")
```

### 5.3 波形可视化

```python
import matplotlib.pyplot as plt

# 设置图像样式
plt.style.use('seaborn-v0_8-whitegrid')

# 创建子图
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 6), sharex=True)

# 绘制SWD_CLK
ax1.plot(df['Time_us'], df['CH0'], color='#e74c3c', linewidth=2)
ax1.set_ylabel('SWD_CLK')
ax1.set_ylim(-0.5, 1.5)
ax1.set_title('SWD Protocol Waveform')

# 绘制SWD_IO
ax2.plot(df['Time_us'], df['CH1'], color='#3498db', linewidth=2)
ax2.set_ylabel('SWD_IO')
ax2.set_xlabel('Time (us)')
ax2.set_ylim(-0.5, 1.5)

# 显示网格
ax1.grid(True, which='both', axis='x')
ax2.grid(True, which='both', axis='x')

# 保存图像
plt.tight_layout()
plt.savefig('swd_waveform.png', dpi=150)
plt.show()
```

### 5.4 协议解码分析

```python
def decode_swd_frame(df):
    """解析SWD帧数据"""
    frames = []
    frame_start = None
    bit_count = 0
    current_frame = {'bits': []}
    
    for i, row in df.iterrows():
        # 检测起始位 (SWD_IO从0变1且SWD_CLK上升沿)
        if i > 0 and df.loc[i-1, 'CH1'] == 0 and row['CH1'] == 1:
            if df.loc[i-1, 'CH0'] == 0 and row['CH0'] == 1:
                if frame_start is None:
                    frame_start = row['Time(ns)']
                    current_frame['start_time'] = frame_start
                    current_frame['bits'] = []
        
        # 收集数据位 (在时钟上升沿采样)
        if frame_start is not None and i > 0:
            if df.loc[i-1, 'CH0'] == 0 and row['CH0'] == 1:
                current_frame['bits'].append(row['CH1'])
                bit_count += 1
                
                # 一帧包含: 1起始 + 4控制 + 3响应 + 32数据 = 40位
                if bit_count >= 40:
                    current_frame['end_time'] = row['Time(ns)']
                    current_frame['duration'] = current_frame['end_time'] - frame_start
                    frames.append(current_frame)
                    
                    frame_start = None
                    bit_count = 0
                    current_frame = {'bits': []}
    
    return frames

# 解析SWD帧
frames = decode_swd_frame(df)

# 输出解析结果
print(f"\n解析到 {len(frames)} 个SWD帧:")
for i, frame in enumerate(frames):
    bits = ''.join(map(str, frame['bits']))
    rw_bit = 'Read' if frame['bits'][1] == 0 else 'Write'
    addr = frame['bits'][2] + (frame['bits'][3] << 1)
    
    print(f"\n帧 {i+1}:")
    print(f"  时间: {frame['start_time']:.0f} - {frame['end_time']:.0f} ns")
    print(f"  时长: {frame['duration']:.0f} ns")
    print(f"  类型: {rw_bit}")
    print(f"  地址: 0x{addr:02X}")
    print(f"  原始位: {bits}")
```

### 5.5 时序分析报告

```python
def generate_timing_report(frames):
    """生成时序分析报告"""
    report = {
        'total_frames': len(frames),
        'avg_duration': np.mean([f['duration'] for f in frames]),
        'min_duration': np.min([f['duration'] for f in frames]),
        'max_duration': np.max([f['duration'] for f in frames]),
        'read_frames': sum(1 for f in frames if f['bits'][1] == 0),
        'write_frames': sum(1 for f in frames if f['bits'][1] == 1),
    }
    
    print("=" * 50)
    print("          SWD时序分析报告          ")
    print("=" * 50)
    print(f"总帧数: {report['total_frames']}")
    print(f"读操作: {report['read_frames']}")
    print(f"写操作: {report['write_frames']}")
    print(f"\n帧时长统计:")
    print(f"  平均: {report['avg_duration']:.2f} ns")
    print(f"  最小: {report['min_duration']:.2f} ns")
    print(f"  最大: {report['max_duration']:.2f} ns")
    print("\n时序合规性:")
    
    # 检查时序是否符合SWD规范
    if report['min_duration'] >= 350:  # 40位 × 最小8.75ns/位
        print("  ✓ 时序符合SWD规范")
    else:
        print("  ✗ 时序可能不符合规范")
        print(f"    最小帧时长: {report['min_duration']:.2f} ns (建议 ≥ 350 ns)")
    
    print("=" * 50)

# 生成报告
generate_timing_report(frames)
```

---

## 6. 脚本自动化测试

### 6.1 测试脚本框架

```python
import os
import subprocess
import time

class DSlogicTester:
    """DSlogic自动化测试类"""
    
    def __init__(self, device_path='COM3', sample_rate=100):
        self.device_path = device_path
        self.sample_rate = sample_rate
        self.output_dir = 'test_results'
        
        # 创建输出目录
        os.makedirs(self.output_dir, exist_ok=True)
    
    def capture_sample(self, duration_ms=100, filename_prefix='test'):
        """捕获采样数据"""
        timestamp = time.strftime('%Y%m%d_%H%M%S')
        filename = f"{filename_prefix}_{timestamp}"
        
        # 构建命令
        cmd = [
            'dslogic-cli',
            '-d', self.device_path,
            '-s', str(self.sample_rate),
            '-t', str(duration_ms),
            '-o', os.path.join(self.output_dir, filename),
            '-f', 'csv'
        ]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode == 0:
                print(f"✓ 采样成功: {filename}")
                return os.path.join(self.output_dir, f"{filename}.csv")
            else:
                print(f"✗ 采样失败: {result.stderr}")
                return None
        except Exception as e:
            print(f"✗ 执行失败: {e}")
            return None
    
    def analyze_sample(self, csv_path):
        """分析采样数据"""
        df = pd.read_csv(csv_path)
        
        # 基本统计
        stats = {
            'samples': len(df),
            'duration_ns': df['Time(ns)'].max() - df['Time(ns)'].min(),
            'clk_freq': self._calculate_clk_freq(df),
            'valid_frames': self._count_valid_frames(df),
        }
        
        return stats
    
    def _calculate_clk_freq(self, df):
        """计算时钟频率"""
        clk_transitions = df[df['CH0'].diff() != 0]
        if len(clk_transitions) < 2:
            return 0
        
        periods = np.diff(clk_transitions['Time(ns)'])
        avg_period = np.mean(periods)
        return 1000 / avg_period  # MHz
    
    def _count_valid_frames(self, df):
        """统计有效帧数量"""
        frames = decode_swd_frame(df)
        return len(frames)
    
    def run_test_suite(self, iterations=5):
        """运行测试套件"""
        print("=" * 50)
        print("     DSlogic自动化测试套件     ")
        print("=" * 50)
        
        results = []
        for i in range(iterations):
            print(f"\n测试 {i+1}/{iterations}")
            csv_path = self.capture_sample()
            
            if csv_path:
                stats = self.analyze_sample(csv_path)
                results.append(stats)
                
                print(f"  采样数: {stats['samples']}")
                print(f"  时长: {stats['duration_ns']/1000:.2f} us")
                print(f"  时钟频率: {stats['clk_freq']:.2f} MHz")
                print(f"  有效帧: {stats['valid_frames']}")
        
        # 汇总结果
        print("\n" + "=" * 50)
        print("         测试结果汇总         ")
        print("=" * 50)
        print(f"测试次数: {len(results)}")
        print(f"平均时钟频率: {np.mean([r['clk_freq'] for r in results]):.2f} MHz")
        print(f"平均有效帧: {np.mean([r['valid_frames'] for r in results]):.1f}")
        print("=" * 50)

# 使用示例
if __name__ == "__main__":
    tester = DSlogicTester(device_path='COM3', sample_rate=100)
    tester.run_test_suite(iterations=5)
```

### 6.2 测试验证清单

| 测试项 | 预期结果 | 通过条件 |
|--------|---------|---------|
| 设备连接 | 设备被识别 | 无错误提示 |
| 采样功能 | 成功捕获数据 | CSV文件生成 |
| 时钟频率 | 50-100 MHz | 在范围内 |
| 协议解码 | 正确解析SWD帧 | 帧数量 > 0 |
| 时序合规 | 帧时长 ≥ 350 ns | 符合规范 |
| 数据完整性 | 无丢失或错误 | 无错误帧 |

---

## 附录：故障排除

### 常见问题

| 问题 | 可能原因 | 解决方法 |
|------|---------|---------|
| 设备未识别 | 驱动未安装 | 重新安装驱动 |
| 采样无数据 | 探头未连接 | 检查连接和接地 |
| 信号抖动 | 采样率过低 | 提高采样率到100MHz |
| 触发不工作 | 触发条件错误 | 检查触发配置 |
| 解码失败 | 通道映射错误 | 确认CLK/DATA通道 |
| 数据文件过大 | 采样深度过高 | 调整采样深度 |

### 调试技巧

```
1. 检查接地:
   - 确保GND已连接
   - 尝试使用屏蔽线
   - 缩短线缆长度

2. 调整阈值:
   - 默认1.4V适用于3.3V系统
   - 如果信号幅度低，降低阈值

3. 使用参考时钟:
   - 连接已知频率的时钟信号到CH3
   - 用于验证采样率准确性

4. 对比分析:
   - 同时捕获SWD_CLK和SWD_IO
   - 手动验证协议解码是否正确
```

---

## 参考资源

1. **DSlogic官方文档**: https://www.dslogic.org
2. **SWD协议规范**: ARM Debug Interface v5
3. **JTAG规范**: IEEE 1149.1
4. **Python数据分析**: pandas, matplotlib官方文档

---

**文档版本**: v1.0  
**创建日期**: 2026-06-02  
**适用设备**: DSlogic DScope

---

> **提示**: 在实际使用中，建议先使用开发板的LED或已知信号验证设备连接和采样功能，确认正常后再进行SWD/JTAG协议分析。
