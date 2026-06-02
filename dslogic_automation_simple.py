
#!/usr/bin/env python3
"""
DSlogic DScope 自动化闭环测试框架 (独立版本)

功能特性:
- 自动设备检测和连接
- 实时信号采样模拟
- 自动协议解码和分析
- 时序验证和合规性检查
- 测试报告自动生成
- 连续闭环测试模式
"""

import os
import sys
import time
import json
import threading
from datetime import datetime
from dataclasses import dataclass
from typing import List, Dict, Optional

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# 配置常量
CONFIG = {
    'sample_rate': 100,  # MHz
    'test_duration_ms': 100,
    'loop_interval_ms': 500,
    'output_dir': 'test_results',
    'report_dir': 'reports',
    'log_file': 'dslogic_automation.log',
}

@dataclass
class TestResult:
    test_id: str
    timestamp: str
    duration_ms: float
    samples_count: int
    clk_freq_mhz: float
    valid_frames: int
    protocol_errors: int
    timing_compliant: bool
    avg_frame_duration_ns: float
    min_frame_duration_ns: float
    max_frame_duration_ns: float

class DSlogicAutomation:
    """DSlogic自动化测试类"""
    
    def __init__(self):
        self.is_running = False
        self.test_results: List[TestResult] = []
        self.current_test_id = 0
        
        # 确保目录存在
        os.makedirs(CONFIG['output_dir'], exist_ok=True)
        os.makedirs(CONFIG['report_dir'], exist_ok=True)
        
        # 初始化日志
        self._init_log()
    
    def _init_log(self):
        """初始化日志文件"""
        with open(CONFIG['log_file'], 'w', encoding='utf-8') as f:
            f.write(f"=== DSlogic Automation Log ===\n")
            f.write(f"Start time: {datetime.now().isoformat()}\n")
            f.write(f"Config: {json.dumps(CONFIG, indent=2)}\n")
            f.write("=" * 50 + "\n\n")
    
    def _log(self, message: str, level: str = "INFO"):
        """记录日志"""
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        log_line = f"[{timestamp}] [{level}] {message}"
        
        print(log_line)
        with open(CONFIG['log_file'], 'a', encoding='utf-8') as f:
            f.write(log_line + "\n")
    
    def _generate_simulation_data(self) -> pd.DataFrame:
        """生成模拟SWD数据"""
        self._log("生成模拟SWD协议数据...")
        
        num_samples = int(CONFIG['sample_rate'] * 1000 * CONFIG['test_duration_ms'] / 1000)
        time_ns = np.arange(num_samples) * (1000 / CONFIG['sample_rate'])
        
        # 生成时钟信号
        clk_period_ns = 10  # 100 MHz
        ch0 = (time_ns // clk_period_ns) % 2
        
        # 生成SWD IO信号 (模拟SWD协议帧)
        ch1 = np.zeros(num_samples, dtype=int)
        
        # 模拟SWD帧: 起始位(1) + 读写位 + 地址(2) + ACK(3) + 数据(32)
        bit_time = clk_period_ns
        frame_start = 100  # 从第100ns开始
        
        for frame_idx in range(5):  # 生成5帧
            frame_pos = frame_start + frame_idx * 450  # 每帧约450ns
            bit_idx = 0
            
            # 起始位
            bit_pos = int(frame_pos / bit_time)
            if bit_pos < num_samples:
                ch1[bit_pos] = 1
            
            # 读写位 (交替读写)
            bit_idx += 1
            bit_pos = int((frame_pos + bit_idx * bit_time) / bit_time)
            if bit_pos < num_samples:
                ch1[bit_pos] = frame_idx % 2  # 0=读, 1=写
            
            # 地址位
            bit_idx += 1
            bit_pos = int((frame_pos + bit_idx * bit_time) / bit_time)
            if bit_pos < num_samples:
                ch1[bit_pos] = 0
            
            bit_idx += 1
            bit_pos = int((frame_pos + bit_idx * bit_time) / bit_time)
            if bit_pos < num_samples:
                ch1[bit_pos] = 0  # DPIDR地址
            
            # ACK (OK)
            bit_idx += 1
            for ack_bit in [0, 1, 0]:  # ACK=OK
                bit_pos = int((frame_pos + bit_idx * bit_time) / bit_time)
                if bit_pos < num_samples:
                    ch1[bit_pos] = ack_bit
                bit_idx += 1
            
            # 数据位 (模拟数据)
            data = 0x1BA01477 + frame_idx
            for i in range(32):
                bit_pos = int((frame_pos + bit_idx * bit_time) / bit_time)
                if bit_pos < num_samples:
                    ch1[bit_pos] = (data >> i) & 1
                bit_idx += 1
        
        df = pd.DataFrame({
            'Time(ns)': time_ns,
            'CH0': ch0,
            'CH1': ch1,
            'Decode': [''] * num_samples
        })
        
        return df
    
    def capture_sample(self) -> Optional[pd.DataFrame]:
        """捕获采样数据"""
        try:
            self._log("正在捕获采样数据...")
            df = self._generate_simulation_data()
            self._log(f"采样完成: {len(df)} 个样本")
            return df
        except Exception as e:
            self._log(f"采样失败: {e}", "ERROR")
            return None
    
    def analyze_sample(self, df: pd.DataFrame) -> Optional[TestResult]:
        """分析采样数据"""
        self._log("正在分析采样数据...")
        
        try:
            # 基本统计
            samples_count = len(df)
            duration_ms = (df['Time(ns)'].max() - df['Time(ns)'].min()) / 1_000_000
            
            # 计算时钟频率
            clk_freq = self._calculate_clock_frequency(df)
            
            # 解析协议帧
            frames = self._decode_swd_frames(df)
            valid_frames = len(frames)
            
            # 协议错误检测
            protocol_errors = self._detect_protocol_errors(frames)
            
            # 时序合规性检查
            timing_compliant, frame_durations = self._check_timing_compliance(frames)
            
            # 创建测试结果
            result = TestResult(
                test_id=f"TEST_{self.current_test_id:04d}",
                timestamp=datetime.now().isoformat(),
                duration_ms=duration_ms,
                samples_count=samples_count,
                clk_freq_mhz=clk_freq,
                valid_frames=valid_frames,
                protocol_errors=protocol_errors,
                timing_compliant=timing_compliant,
                avg_frame_duration_ns=np.mean(frame_durations) if frame_durations else 0,
                min_frame_duration_ns=np.min(frame_durations) if frame_durations else 0,
                max_frame_duration_ns=np.max(frame_durations) if frame_durations else 0,
            )
            
            self._log(f"分析完成: {valid_frames}帧, {clk_freq:.2f}MHz, 时序合规: {timing_compliant}")
            return result
            
        except Exception as e:
            self._log(f"分析失败: {e}", "ERROR")
            return None
    
    def _calculate_clock_frequency(self, df: pd.DataFrame) -> float:
        """计算时钟频率"""
        clk_transitions = df[df['CH0'].diff() == 1]
        
        if len(clk_transitions) < 2:
            return 0.0
        
        periods = np.diff(clk_transitions['Time(ns)'])
        
        if len(periods) == 0:
            return 0.0
        
        avg_period_ns = np.mean(periods)
        return 1000.0 / avg_period_ns
    
    def _decode_swd_frames(self, df: pd.DataFrame) -> List[dict]:
        """解析SWD协议帧"""
        frames = []
        frame_start = None
        current_bits = []
        in_frame = False
        
        for i in range(1, len(df)):
            if df.loc[i-1, 'CH0'] == 0 and df.loc[i, 'CH0'] == 1:
                if not in_frame and df.loc[i, 'CH1'] == 1:
                    frame_start = df.loc[i, 'Time(ns)']
                    in_frame = True
                    current_bits = []
                
                if in_frame:
                    current_bits.append(df.loc[i, 'CH1'])
                    
                    if len(current_bits) >= 40:
                        frame_type = 'READ' if current_bits[1] == 0 else 'WRITE'
                        address = current_bits[2] + (current_bits[3] << 1)
                        
                        frames.append({
                            'start_time': frame_start,
                            'end_time': df.loc[i, 'Time(ns)'],
                            'bits': current_bits.copy(),
                            'frame_type': frame_type,
                            'address': address
                        })
                        
                        in_frame = False
                        current_bits = []
        
        return frames
    
    def _detect_protocol_errors(self, frames: List[dict]) -> int:
        """检测协议错误"""
        errors = 0
        
        for frame in frames:
            ack_bits = frame['bits'][4:7]
            ack_value = ack_bits[0] + (ack_bits[1] << 1) + (ack_bits[2] << 2)
            
            if ack_value != 1:
                errors += 1
        
        return errors
    
    def _check_timing_compliance(self, frames: List[dict]) -> tuple:
        """检查时序合规性"""
        durations = []
        
        for frame in frames:
            duration = frame['end_time'] - frame['start_time']
            durations.append(duration)
        
        if not durations:
            return True, []
        
        min_duration = np.min(durations)
        compliant = min_duration >= 350
        
        return compliant, durations
    
    def generate_report(self, results: List[TestResult]) -> str:
        """生成测试报告"""
        if not results:
            return "无测试结果"
        
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        report_path = os.path.join(CONFIG['report_dir'], f"test_report_{timestamp}.md")
        
        summary = {
            'total_tests': len(results),
            'pass_count': sum(1 for r in results if r.timing_compliant and r.protocol_errors == 0),
            'fail_count': sum(1 for r in results if not r.timing_compliant or r.protocol_errors > 0),
            'avg_clk_freq': np.mean([r.clk_freq_mhz for r in results]),
            'avg_frames': np.mean([r.valid_frames for r in results]),
            'total_duration': sum([r.duration_ms for r in results]),
        }
        
        with open(report_path, 'w', encoding='utf-8') as f:
            f.write(f"# DSlogic自动化测试报告\n\n")
            f.write(f"**报告生成时间**: {datetime.now().isoformat()}\n")
            f.write(f"**测试配置**:\n")
            f.write(f"- 采样率: {CONFIG['sample_rate']} MHz\n")
            f.write(f"- 测试时长: {CONFIG['test_duration_ms']} ms\n\n")
            
            f.write(f"## 📊 测试摘要\n\n")
            f.write(f"| 项目 | 数值 |\n")
            f.write(f"|------|------|\n")
            f.write(f"| 测试次数 | {summary['total_tests']} |\n")
            f.write(f"| 通过 | {summary['pass_count']} |\n")
            f.write(f"| 失败 | {summary['fail_count']} |\n")
            f.write(f"| 通过率 | {(summary['pass_count']/summary['total_tests']*100):.1f}% |\n")
            f.write(f"| 平均时钟频率 | {summary['avg_clk_freq']:.2f} MHz |\n")
            f.write(f"| 平均帧数 | {summary['avg_frames']:.1f} |\n\n")
            
            f.write(f"## 📈 测试详情\n\n")
            f.write(f"| 测试ID | 时间 | 时长(ms) | 时钟(MHz) | 帧数 | 错误数 | 时序合规 |\n")
            f.write(f"|--------|------|----------|-----------|------|--------|----------|\n")
            
            for result in results:
                status = '✅' if result.timing_compliant and result.protocol_errors == 0 else '❌'
                f.write(f"| {result.test_id} | {result.timestamp[:19]} | {result.duration_ms:.2f} | {result.clk_freq_mhz:.2f} | {result.valid_frames} | {result.protocol_errors} | {status} |\n")
        
        self._log(f"报告已生成: {report_path}")
        return report_path
    
    def run_single_test(self) -> Optional[TestResult]:
        """运行单次测试"""
        self._log(f"=== 开始测试 #{self.current_test_id} ===")
        
        df = self.capture_sample()
        if df is None:
            return None
        
        result = self.analyze_sample(df)
        
        if result:
            self.test_results.append(result)
            self.current_test_id += 1
            
            status = "通过" if result.timing_compliant and result.protocol_errors == 0 else "失败"
            self._log(f"测试结果: {status}")
        
        return result
    
    def run_loop_test(self, iterations: int = 10):
        """运行循环测试"""
        self._log(f"=== 开始循环测试 ({iterations}次) ===")
        
        self.is_running = True
        success_count = 0
        
        for i in range(iterations):
            if not self.is_running:
                break
            
            self._log(f"\n--- 测试 {i+1}/{iterations} ---")
            
            result = self.run_single_test()
            
            if result and result.timing_compliant and result.protocol_errors == 0:
                success_count += 1
            
            if i < iterations - 1:
                time.sleep(CONFIG['loop_interval_ms'] / 1000.0)
        
        report_path = self.generate_report(self.test_results)
        
        pass_rate = (success_count / iterations) * 100
        self._log(f"\n=== 循环测试完成 ===")
        self._log(f"通过: {success_count}/{iterations} ({pass_rate:.1f}%)")
        self._log(f"报告: {report_path}")
        
        return pass_rate
    
    def run_continuous_test(self, duration_minutes: float = 5):
        """运行连续测试"""
        self._log(f"=== 开始连续测试 ({duration_minutes}分钟) ===")
        
        self.is_running = True
        start_time = time.time()
        test_count = 0
        success_count = 0
        
        while self.is_running:
            elapsed = time.time() - start_time
            if elapsed >= duration_minutes * 60:
                break
            
            result = self.run_single_test()
            
            if result:
                test_count += 1
                if result.timing_compliant and result.protocol_errors == 0:
                    success_count += 1
            
            time.sleep(CONFIG['loop_interval_ms'] / 1000.0)
        
        report_path = self.generate_report(self.test_results)
        
        pass_rate = (success_count / test_count) * 100 if test_count > 0 else 0
        self._log(f"\n=== 连续测试完成 ===")
        self._log(f"总测试次数: {test_count}")
        self._log(f"通过: {success_count}/{test_count} ({pass_rate:.1f}%)")
        self._log(f"报告: {report_path}")
        
        return pass_rate
    
    def stop(self):
        """停止测试"""
        self._log("收到停止信号")
        self.is_running = False

def main():
    """主函数"""
    print("=" * 60)
    print("    DSlogic DScope 自动化闭环测试系统")
    print("=" * 60)
    print("  模式: 模拟测试 (Simulated Mode)")
    print("=" * 60)
    print()
    
    automation = DSlogicAutomation()
    
    while True:
        print("\n请选择测试模式:")
        print("1. 单次测试")
        print("2. 循环测试")
        print("3. 连续测试")
        print("4. 生成报告")
        print("5. 退出")
        
        choice = input("\n请输入选择 (1-5): ")
        
        if choice == '1':
            automation.run_single_test()
            
        elif choice == '2':
            iterations = int(input("请输入测试次数 (默认10): ") or 10)
            automation.run_loop_test(iterations)
            
        elif choice == '3':
            duration = float(input("请输入测试时长(分钟) (默认5): ") or 5)
            automation.run_continuous_test(duration)
            
        elif choice == '4':
            if automation.test_results:
                automation.generate_report(automation.test_results)
                print("✅ 报告已生成")
            else:
                print("❌ 没有测试结果")
                
        elif choice == '5':
            automation.stop()
            print("👋 退出系统")
            break
            
        else:
            print("❌ 无效选择")

if __name__ == "__main__":
    main()
