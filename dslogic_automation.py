
#!/usr/bin/env python3
"""
DSlogic DScope 自动化闭环测试系统

功能特性:
- 自动设备检测和连接
- 实时信号采样
- 自动协议解码和分析
- 时序验证和合规性检查
- 测试报告自动生成
- 连续闭环测试模式
"""

import os
import sys
import time
import json
import subprocess
import threading
from datetime import datetime
from dataclasses import dataclass
from typing import List, Dict, Optional, Callable

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# 配置常量
CONFIG = {
    'sample_rate': 100,  # MHz
    'sample_depth': 1024 * 1024,  # 1M
    'trigger_channel': 0,  # CH0
    'output_dir': 'test_results',
    'report_dir': 'reports',
    'log_file': 'dslogic_automation.log',
    'test_duration_ms': 100,
    'loop_interval_ms': 500,
}

@dataclass
class TestResult:
    """测试结果数据类"""
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

@dataclass
class ProtocolFrame:
    """协议帧数据类"""
    start_time: float
    end_time: float
    bits: List[int]
    frame_type: str  # 'READ' or 'WRITE'
    address: int
    data: Optional[int] = None

class DSlogicAutomation:
    """DSlogic自动化测试类"""
    
    def __init__(self):
        self.device_path = None
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
        timestamp = datetime.now().isoformat()
        log_line = f"[{timestamp}] [{level}] {message}\n"
        
        print(log_line.strip())
        with open(CONFIG['log_file'], 'a', encoding='utf-8') as f:
            f.write(log_line)
    
    def detect_device(self) -> bool:
        """检测DSlogic设备"""
        self._log("正在检测DSlogic设备...")
        
        try:
            # 尝试获取设备列表
            result = subprocess.run(
                ['dslogic-cli', '-l'],
                capture_output=True,
                text=True,
                timeout=10
            )
            
            if result.returncode == 0:
                devices = result.stdout.strip().split('\n')
                for device in devices:
                    if 'DSlogic' in device:
                        self.device_path = device.split()[0]
                        self._log(f"找到设备: {self.device_path}")
                        return True
            
            self._log("未找到DSlogic设备", "WARNING")
            return False
            
        except FileNotFoundError:
            self._log("未找到dslogic-cli命令，请确保已安装DSlogic软件", "ERROR")
            return False
        except Exception as e:
            self._log(f"设备检测失败: {e}", "ERROR")
            return False
    
    def capture_sample(self) -> Optional[str]:
        """捕获单次采样"""
        if not self.device_path:
            self._log("设备未连接", "ERROR")
            return None
        
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S_%f')
        filename = f"capture_{timestamp}"
        output_path = os.path.join(CONFIG['output_dir'], filename)
        
        try:
            cmd = [
                'dslogic-cli',
                '-d', self.device_path,
                '-s', str(CONFIG['sample_rate']),
                '-t', str(CONFIG['test_duration_ms']),
                '-o', output_path,
                '-f', 'csv'
            ]
            
            self._log(f"执行采样命令: {' '.join(cmd)}")
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30
            )
            
            if result.returncode == 0:
                csv_path = f"{output_path}.csv"
                if os.path.exists(csv_path):
                    self._log(f"采样成功: {csv_path}")
                    return csv_path
                else:
                    self._log("采样文件未生成", "ERROR")
                    return None
            else:
                self._log(f"采样失败: {result.stderr}", "ERROR")
                return None
                
        except Exception as e:
            self._log(f"采样异常: {e}", "ERROR")
            return None
    
    def analyze_sample(self, csv_path: str) -> Optional[TestResult]:
        """分析采样数据"""
        self._log(f"正在分析: {csv_path}")
        
        try:
            # 加载数据
            df = pd.read_csv(csv_path)
            
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
        # 检测时钟上升沿
        clk_transitions = df[df['CH0'].diff() == 1]
        
        if len(clk_transitions) < 2:
            return 0.0
        
        # 计算周期
        periods = np.diff(clk_transitions['Time(ns)'])
        
        if len(periods) == 0:
            return 0.0
        
        avg_period_ns = np.mean(periods)
        return 1000.0 / avg_period_ns  # MHz
    
    def _decode_swd_frames(self, df: pd.DataFrame) -> List[ProtocolFrame]:
        """解析SWD协议帧"""
        frames = []
        frame_start = None
        current_bits = []
        in_frame = False
        
        for i in range(1, len(df)):
            # 检测时钟上升沿
            if df.loc[i-1, 'CH0'] == 0 and df.loc[i, 'CH0'] == 1:
                # 检测起始位 (SWD_IO=1)
                if not in_frame and df.loc[i, 'CH1'] == 1:
                    frame_start = df.loc[i, 'Time(ns)']
                    in_frame = True
                    current_bits = []
                
                # 收集数据位
                if in_frame:
                    current_bits.append(df.loc[i, 'CH1'])
                    
                    # SWD帧: 1起始 + 4控制 + 3ACK + 32数据 = 40位
                    if len(current_bits) >= 40:
                        # 解析帧类型和地址
                        frame_type = 'READ' if current_bits[1] == 0 else 'WRITE'
                        address = current_bits[2] + (current_bits[3] << 1)
                        
                        frames.append(ProtocolFrame(
                            start_time=frame_start,
                            end_time=df.loc[i, 'Time(ns)'],
                            bits=current_bits.copy(),
                            frame_type=frame_type,
                            address=address
                        ))
                        
                        in_frame = False
                        current_bits = []
        
        return frames
    
    def _detect_protocol_errors(self, frames: List[ProtocolFrame]) -> int:
        """检测协议错误"""
        errors = 0
        
        for frame in frames:
            # 检查ACK位 (位5-7)
            ack_bits = frame.bits[4:7]
            ack_value = ack_bits[0] + (ack_bits[1] << 1) + (ack_bits[2] << 2)
            
            # ACK=0b001 表示OK
            if ack_value != 1:
                errors += 1
        
        return errors
    
    def _check_timing_compliance(self, frames: List[ProtocolFrame]) -> tuple:
        """检查时序合规性"""
        durations = []
        
        for frame in frames:
            duration = frame.end_time - frame.start_time
            durations.append(duration)
        
        if not durations:
            return True, []
        
        # SWD最小周期要求
        min_duration = np.min(durations)
        
        # SWD规范: 每bit最小8.75ns, 40bit帧最小350ns
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
            f.write(f"- 测试时长: {CONFIG['test_duration_ms']} ms\n")
            f.write(f"- 采样深度: {CONFIG['sample_depth']:,} 点\n\n")
            
            f.write(f"## 📊 测试摘要\n\n")
            f.write(f"| 项目 | 数值 |\n")
            f.write(f"|------|------|\n")
            f.write(f"| 测试次数 | {summary['total_tests']} |\n")
            f.write(f"| 通过 | {summary['pass_count']} |\n")
            f.write(f"| 失败 | {summary['fail_count']} |\n")
            f.write(f"| 通过率 | {(summary['pass_count']/summary['total_tests']*100):.1f}% |\n")
            f.write(f"| 平均时钟频率 | {summary['avg_clk_freq']:.2f} MHz |\n")
            f.write(f"| 平均帧数 | {summary['avg_frames']:.1f} |\n")
            f.write(f"| 总测试时长 | {summary['total_duration']:.2f} ms |\n\n")
            
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
        
        # 捕获采样
        csv_path = self.capture_sample()
        if not csv_path:
            return None
        
        # 分析数据
        result = self.analyze_sample(csv_path)
        
        if result:
            self.test_results.append(result)
            self.current_test_id += 1
            
            # 输出测试结果
            status = "通过" if result.timing_compliant and result.protocol_errors == 0 else "失败"
            self._log(f"测试结果: {status}")
        
        return result
    
    def run_loop_test(self, iterations: int = 10, interval_ms: int = 500):
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
            
            # 等待间隔
            if i < iterations - 1:
                time.sleep(interval_ms / 1000.0)
        
        # 生成报告
        report_path = self.generate_report(self.test_results)
        
        # 统计结果
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
            
            # 短暂休息
            time.sleep(CONFIG['loop_interval_ms'] / 1000.0)
        
        # 生成报告
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

class TestMonitor:
    """测试监控器"""
    
    def __init__(self, automation: DSlogicAutomation):
        self.automation = automation
        self.monitor_thread = None
        self.is_monitoring = False
    
    def start_monitoring(self):
        """开始监控"""
        self.is_monitoring = True
        self.monitor_thread = threading.Thread(target=self._monitor_loop)
        self.monitor_thread.daemon = True
        self.monitor_thread.start()
    
    def _monitor_loop(self):
        """监控循环"""
        while self.is_monitoring:
            # 检查最新测试结果
            if self.automation.test_results:
                latest = self.automation.test_results[-1]
                
                # 检查告警条件
                if not latest.timing_compliant:
                    self._alert(f"时序不合规: 最小帧时长 {latest.min_frame_duration_ns:.2f} ns")
                
                if latest.protocol_errors > 0:
                    self._alert(f"协议错误: {latest.protocol_errors} 个")
                
                if latest.clk_freq_mhz < 40 or latest.clk_freq_mhz > 120:
                    self._alert(f"时钟频率异常: {latest.clk_freq_mhz:.2f} MHz")
            
            time.sleep(1.0)
    
    def _alert(self, message: str):
        """发送告警"""
        self.automation._log(f"⚠️ 告警: {message}", "WARNING")
        
        # 可以扩展: 发送邮件、短信、通知等
    
    def stop_monitoring(self):
        """停止监控"""
        self.is_monitoring = False
        if self.monitor_thread:
            self.monitor_thread.join()

def main():
    """主函数"""
    print("=" * 60)
    print("    DSlogic DScope 自动化闭环测试系统")
    print("=" * 60)
    print()
    
    # 创建自动化实例
    automation = DSlogicAutomation()
    
    # 检测设备
    if not automation.detect_device():
        print("❌ 未找到DSlogic设备，请确保设备已连接")
        sys.exit(1)
    
    # 显示菜单
    while True:
        print("\n请选择测试模式:")
        print("1. 单次测试")
        print("2. 循环测试 (10次)")
        print("3. 连续测试 (5分钟)")
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
            print("❌ 无效选择，请重新输入")

if __name__ == "__main__":
    main()
