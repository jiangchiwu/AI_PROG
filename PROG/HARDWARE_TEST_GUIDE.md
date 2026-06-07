# 硬件测试准备文档

## 1. 概述

本文档描述AI_PROG项目硬件测试的准备工作，包括硬件连接、测试流程、测试脚本和预期结果。

## 2. 硬件连接说明

### 2.1 测试设备清单

| 设备名称 | 型号 | 数量 | 说明 |
|---------|------|------|------|
| STM32H7开发板 | AI_PROG-PROG | 1 | 编程器主板 |
| 目标板1 | STM32F4Discovery | 1 | ARM Cortex-M4测试 |
| 目标板2 | MSP430 LaunchPad | 1 | MSP430 SBW测试 |
| 目标板3 | NXP S32K144 | 1 | BDM测试 |
| 逻辑分析仪 | DSLogic Plus | 1 | 时序分析 |
| 万用表 | - | 1 | 电压测量 |
| 杜邦线 | - | 若干 | 连接线缆 |

### 2.2 SWD接口连接

| PROG引脚 | 目标板引脚 | 信号名称 |
|---------|-----------|---------|
| PA13 | SWDIO | 串行数据输入/输出 |
| PA14 | SWCLK | 串行时钟 |
| NRST | NRST | 复位信号 |
| GND | GND | 地 |

### 2.3 JTAG接口连接

| PROG引脚 | 目标板引脚 | 信号名称 |
|---------|-----------|---------|
| PA13 | TMS | 测试模式选择 |
| PA14 | TCK | 测试时钟 |
| PB3 | TDI | 测试数据输入 |
| PB4 | TDO | 测试数据输出 |
| NRST | TRST | 测试复位 |
| NRST | NRST | 复位信号 |
| GND | GND | 地 |

### 2.4 BDM接口连接

| PROG引脚 | 目标板引脚 | 信号名称 |
|---------|-----------|---------|
| PB5 | BKPT | 断点/数据信号 |
| PB6 | RESET | 复位信号 |
| GND | GND | 地 |

### 2.5 SBW接口连接

| PROG引脚 | 目标板引脚 | 信号名称 |
|---------|-----------|---------|
| PB7 | TCK | 测试时钟 |
| PB8 | TMS | 测试模式选择 |
| PB9 | RST | 复位信号 |
| PB10 | TEST | 测试触发 |
| GND | GND | 地 |

### 2.6 ICSP接口连接

| PROG引脚 | 目标板引脚 | 信号名称 |
|---------|-----------|---------|
| PB11 | MCLR/VPP | 编程电压 |
| PB12 | CLK | 时钟 |
| PB13 | DATA | 数据 |
| GND | GND | 地 |

### 2.7 ISP接口连接

| PROG引脚 | 目标板引脚 | 信号名称 |
|---------|-----------|---------|
| PA4 | MOSI | 主出从入 |
| PA5 | SCK | 时钟 |
| PA6 | MISO | 主入从出 |
| PA7 | RESET | 复位信号 |
| GND | GND | 地 |

## 3. 测试流程

### 3.1 测试环境搭建

1. 连接PROG板到PC（USB-C接口）
2. 连接目标板到PROG板
3. 连接逻辑分析仪到关键信号
4. 打开串口终端（115200bps, 8N1）
5. 启动测试程序

### 3.2 测试步骤

#### 步骤1: 设备枚举测试
```bash
# 发送命令
> enum
# 预期响应
[OK] Device detected: STM32H743VIT6
[OK] SWD Interface Ready
[OK] JTAG Interface Ready
```

#### 步骤2: 芯片识别测试
```bash
# 发送命令
> detect
# 预期响应
[OK] Detecting chip...
[OK] JTAG ID: 0x4BA00477
[OK] Chip ID: 0x00010001
[OK] Chip detected: STM32F407VGT6
[OK] Flash Size: 1024KB
[OK] RAM Size: 192KB
```

#### 步骤3: Flash擦除测试
```bash
# 发送命令
> erase
# 预期响应
[OK] Erasing Flash...
[OK] Sector 0 erased
[OK] Sector 1 erased
...
[OK] Erase completed
```

#### 步骤4: Flash编程测试
```bash
# 发送命令
> write 0x08000000 test.bin
# 预期响应
[OK] Writing 1024 bytes to 0x08000000...
[OK] Write completed
```

#### 步骤5: Flash验证测试
```bash
# 发送命令
> verify 0x08000000 test.bin
# 预期响应
[OK] Verifying 1024 bytes at 0x08000000...
[OK] Verification passed
```

#### 步骤6: 读回测试
```bash
# 发送命令
> read 0x08000000 1024 output.bin
# 预期响应
[OK] Reading 1024 bytes from 0x08000000...
[OK] Read completed
```

## 4. 测试脚本

### 4.1 自动测试脚本

```python
#!/usr/bin/env python3
"""
AI_PROG 自动测试脚本
"""

import serial
import time
import os

class AIProgTester:
    def __init__(self, port='COM3', baud=115200):
        self.ser = serial.Serial(port, baud, timeout=5)
        time.sleep(1)
    
    def send_command(self, cmd):
        self.ser.write((cmd + '\n').encode())
        time.sleep(0.5)
        return self.ser.read_all().decode()
    
    def test_enum(self):
        print("[TEST] Device Enumeration")
        result = self.send_command('enum')
        if '[OK]' in result:
            print("[PASS] Enumeration test passed")
            return True
        print("[FAIL] Enumeration test failed")
        return False
    
    def test_detect(self):
        print("[TEST] Chip Detection")
        result = self.send_command('detect')
        if '[OK] Chip detected:' in result:
            print("[PASS] Detection test passed")
            return True
        print("[FAIL] Detection test failed")
        return False
    
    def test_erase(self):
        print("[TEST] Flash Erase")
        result = self.send_command('erase')
        if '[OK] Erase completed' in result:
            print("[PASS] Erase test passed")
            return True
        print("[FAIL] Erase test failed")
        return False
    
    def test_write(self):
        print("[TEST] Flash Write")
        result = self.send_command('write 0x08000000 test.bin')
        if '[OK] Write completed' in result:
            print("[PASS] Write test passed")
            return True
        print("[FAIL] Write test failed")
        return False
    
    def test_verify(self):
        print("[TEST] Flash Verify")
        result = self.send_command('verify 0x08000000 test.bin')
        if '[OK] Verification passed' in result:
            print("[PASS] Verify test passed")
            return True
        print("[FAIL] Verify test failed")
        return False
    
    def run_all_tests(self):
        print("="*50)
        print("AI_PROG Automatic Test Suite")
        print("="*50)
        
        tests = [
            self.test_enum,
            self.test_detect,
            self.test_erase,
            self.test_write,
            self.test_verify
        ]
        
        passed = 0
        for test in tests:
            if test():
                passed += 1
            print()
        
        print("="*50)
        print(f"Results: {passed}/{len(tests)} tests passed")
        print("="*50)
        
        self.ser.close()

if __name__ == '__main__':
    tester = AIProgTester()
    tester.run_all_tests()
```

### 4.2 时序测试脚本

```python
#!/usr/bin/env python3
"""
AI_PROG 时序分析脚本
需要连接DSLogic逻辑分析仪
"""

import dslogic as ds

class TimingAnalyzer:
    def __init__(self):
        self.device = ds.DSLogic()
    
    def setup(self):
        self.device.open()
        self.device.set_samplerate(100e6)  # 100MHz采样率
        self.device.set_channels(0b1111)    # 启用4通道
    
    def measure_swd_timing(self):
        """测量SWD接口时序"""
        print("[TEST] SWD Timing Analysis")
        
        # 捕获数据
        data = self.device.capture(10000)
        
        # 分析时钟频率
        clk_periods = []
        last_clk = 0
        last_time = 0
        
        for i, sample in enumerate(data):
            clk = (sample >> 0) & 1
            if clk != last_clk and clk == 1:
                if last_time > 0:
                    clk_periods.append(i - last_time)
                last_time = i
            last_clk = clk
        
        avg_period = sum(clk_periods) / len(clk_periods)
        freq = 100e6 / avg_period
        
        print(f"[INFO] SWD Clock Frequency: {freq/1e6:.2f} MHz")
        print(f"[INFO] Average Period: {avg_period*10:.2f} ns")
        
        if freq > 9e6:
            print("[PASS] SWD frequency > 9MHz")
            return True
        print("[FAIL] SWD frequency too low")
        return False
    
    def measure_bdm_timing(self):
        """测量BDM接口时序"""
        print("[TEST] BDM Timing Analysis")
        
        data = self.device.capture(10000)
        
        # 分析BKPT信号
        periods = []
        last_bkpt = 0
        last_time = 0
        
        for i, sample in enumerate(data):
            bkpt = (sample >> 2) & 1
            if bkpt != last_bkpt:
                if last_time > 0:
                    periods.append(i - last_time)
                last_time = i
            last_bkpt = bkpt
        
        if len(periods) > 0:
            avg_period = sum(periods) / len(periods)
            freq = 100e6 / avg_period
            print(f"[INFO] BDM Frequency: {freq/1e6:.2f} MHz")
            
            if freq > 9e6:
                print("[PASS] BDM frequency > 9MHz")
                return True
        
        print("[FAIL] BDM timing test failed")
        return False
    
    def run(self):
        self.setup()
        
        print("="*50)
        print("AI_PROG Timing Analysis")
        print("="*50)
        
        self.measure_swd_timing()
        print()
        self.measure_bdm_timing()
        
        self.device.close()

if __name__ == '__main__':
    analyzer = TimingAnalyzer()
    analyzer.run()
```

## 5. 预期结果

### 5.1 功能测试预期结果

| 测试项 | 预期结果 | 实际结果 |
|-------|---------|---------|
| 设备枚举 | 正确识别PROG板 | |
| 芯片识别 | 正确识别目标芯片 | |
| Flash擦除 | 成功擦除所有扇区 | |
| Flash编程 | 成功写入数据 | |
| Flash验证 | 验证通过 | |
| Flash读取 | 数据正确 | |

### 5.2 时序测试预期结果

| 接口 | 预期频率 | 实际频率 |
|-----|---------|---------|
| SWD | >= 9MHz | |
| JTAG | >= 9MHz | |
| BDM | >= 9MHz | |
| SBW | >= 1MHz | |
| ICSP | >= 1MHz | |
| ISP | >= 1MHz | |

### 5.3 性能测试预期结果

| 测试项 | 预期指标 | 实际结果 |
|-------|---------|---------|
| 芯片识别时间 | < 100ms | |
| Flash擦除时间(1MB) | < 5s | |
| Flash编程速度 | > 50KB/s | |
| 缓存命中率 | > 80% | |

## 6. 故障排除

### 6.1 常见问题

| 问题 | 可能原因 | 解决方案 |
|-----|---------|---------|
| 无法识别芯片 | 连接错误 | 检查接线 |
| 通信超时 | 时钟频率过高 | 降低频率 |
| 编程失败 | 目标芯片未上电 | 检查电源 |
| 验证失败 | 数据传输错误 | 检查接线质量 |

### 6.2 调试命令

```bash
# 查看版本信息
> version

# 查看接口状态
> status

# 手动发送SWD命令
> swd read 0 0x00

# 手动发送JTAG命令
> jtag shift 0x1234

# 查看缓存统计
> cache stats
```

## 7. 测试报告模板

```
AI_PROG 硬件测试报告
=====================

测试日期: YYYY-MM-DD
测试人员: XXX
测试环境: Windows 10 / Python 3.8

一、设备信息
------------
PROG板型号: AI_PROG-PROG v2.0
固件版本: v2.0.0
目标芯片: STM32F407VGT6

二、测试结果
------------
1. 设备枚举: PASS
2. 芯片识别: PASS
3. Flash擦除: PASS
4. Flash编程: PASS
5. Flash验证: PASS
6. Flash读取: PASS

三、时序测试
------------
SWD频率: 9.8 MHz
JTAG频率: 9.5 MHz
BDM频率: 9.2 MHz

四、性能测试
------------
芯片识别时间: 45ms
Flash擦除时间: 3.2s
Flash编程速度: 65KB/s
缓存命中率: 85%

五、结论
--------
测试通过: 是/否
备注: 无
```

## 8. 安全注意事项

1. 确保所有连接正确无误后再上电
2. 编程电压(VPP)仅在编程时启用
3. 逻辑分析仪探头应使用适当的接地夹
4. 测试过程中避免触摸电路板
5. 遵守ESD防护规范
