import numpy as np
import pandas as pd
from typing import List, Dict
import os
import time

class DSLogicAutomation:
    def __init__(self):
        self.sample_rate = 100  # MHz
        self.test_duration_ms = 100
        
    def _log(self, message: str, level: str = "INFO"):
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{timestamp}] [{level}] {message}")
    
    def capture_sample_data(self) -> pd.DataFrame:
        """捕获采样数据（模拟）"""
        self._log("生成模拟SWD协议数据...")
        
        num_samples = int(self.sample_rate * 1000 * self.test_duration_ms / 1000)
        time_ns = np.arange(num_samples) * (1000 / self.sample_rate)
        
        # 生成时钟信号
        clk_period_ns = 10
        ch0 = (time_ns // clk_period_ns) % 2
        ch1 = np.zeros(num_samples, dtype=int)
        
        # 定义完整的帧数据（每帧40位）
        all_frames = []
        for frame_idx in range(5):
            frame = []
            frame.append(1)  # 起始位
            frame.append(frame_idx % 2)  # 读写位
            frame.append(0)  # 地址位0
            frame.append(0)  # 地址位1
            frame.extend([1, 0, 0])  # ACK位: OK
            data = 0x1BA01477 + frame_idx
            for i in range(32):
                frame.append((data >> i) & 1)
            all_frames.append(frame)
        
        # 将帧数据写入ch1，每个位占用2个样本（一个完整时钟周期）
        # 时钟上升沿发生在样本1, 3, 5, 7, 9, 11,... 对应时间10, 30, 50, 70, 90, 110ns
        # 帧起始于90ns（样本9）的上升沿
        frame_start_sample = 9  # 从第9个样本开始（90ns，上升沿位置）
        bits_per_frame = 40  # 每帧40位
        samples_per_bit = 2  # 每个位占用2个样本
        frame_gap_samples = 20  # 帧间隙20个样本
        frame_interval_samples = bits_per_frame * samples_per_bit + frame_gap_samples  # 帧间隔100个样本
        
        for frame_idx, frame_data in enumerate(all_frames):
            start_sample = frame_start_sample + frame_idx * frame_interval_samples
            for bit_idx, bit_value in enumerate(frame_data):
                sample_pos = start_sample + bit_idx * samples_per_bit
                if sample_pos < num_samples:
                    # 每个位占用2个样本（在上升沿和下一个下降沿之间）
                    ch1[sample_pos] = bit_value
                    if sample_pos + 1 < num_samples:
                        ch1[sample_pos + 1] = bit_value
        
        df = pd.DataFrame({
            'Time(ns)': time_ns,
            'CH0': ch0,
            'CH1': ch1
        })
        
        self._log(f"采样完成: {len(df)} 个样本")
        return df
    
    def analyze_protocol(self, df: pd.DataFrame) -> Dict:
        """分析协议数据"""
        self._log("正在分析采样数据...")
        
        frames = self._decode_swd_frames(df)
        protocol_errors = self._detect_protocol_errors(frames)
        timing_ok = self._verify_timing(df)
        freq_mhz = self._calculate_frequency(df)
        
        result = {
            'frame_count': len(frames),
            'protocol_errors': protocol_errors,
            'timing_ok': timing_ok,
            'frequency_mhz': freq_mhz,
            'frames': frames
        }
        
        self._log(f"分析完成: {len(frames)}帧, {freq_mhz:.2f}MHz, 时序合规: {timing_ok}")
        return result
    
    def _decode_swd_frames(self, df: pd.DataFrame) -> List[dict]:
        """解析SWD协议帧"""
        frames = []
        frame_start = None
        current_bits = []
        in_frame = False
        
        for i in range(1, len(df)):
            if df.loc[i-1, 'CH0'] == 0 and df.loc[i, 'CH0'] == 1:
                # 时钟上升沿
                if not in_frame and df.loc[i, 'CH1'] == 1:
                    frame_start = df.loc[i, 'Time(ns)']
                    in_frame = True
                    current_bits = [df.loc[i, 'CH1']]
                elif in_frame:
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
        
        for idx, frame in enumerate(frames):
            ack_bits = frame['bits'][4:7]
            ack_value = ack_bits[0] + (ack_bits[1] << 1) + (ack_bits[2] << 2)
            
            if ack_value != 1:
                errors += 1
                self._log(f"帧{idx}: ACK错误 - 期望值=1, 实际值={ack_value}, ACK位={ack_bits}", "DEBUG")
        
        return errors
    
    def _verify_timing(self, df: pd.DataFrame) -> bool:
        """验证时序是否合规"""
        rising_edges = []
        for i in range(1, len(df)):
            if df.loc[i-1, 'CH0'] == 0 and df.loc[i, 'CH0'] == 1:
                rising_edges.append(df.loc[i, 'Time(ns)'])
        
        if len(rising_edges) < 2:
            return False
        
        periods = np.diff(rising_edges)
        avg_period = np.mean(periods)
        max_deviation = np.max(np.abs(periods - avg_period))
        
        return max_deviation < 1.0
    
    def _calculate_frequency(self, df: pd.DataFrame) -> float:
        """计算频率"""
        rising_edges = []
        for i in range(1, len(df)):
            if df.loc[i-1, 'CH0'] == 0 and df.loc[i, 'CH0'] == 1:
                rising_edges.append(df.loc[i, 'Time(ns)'])
        
        if len(rising_edges) < 2:
            return 0.0
        
        periods = np.diff(rising_edges)
        avg_period_ns = np.mean(periods)
        
        if avg_period_ns == 0:
            return 0.0
        return 1000.0 / avg_period_ns
    
    def generate_report(self, results: List[Dict]) -> str:
        """生成测试报告"""
        report_lines = [
            "# DSlogic 自动化测试报告",
            "",
            f"生成时间: {time.strftime('%Y-%m-%d %H:%M:%S')}",
            "",
            "## 测试摘要",
            f"- 测试次数: {len(results)}",
            f"- 平均帧率: {np.mean([r['frame_count'] for r in results]):.1f} 帧/测试",
            f"- 平均协议错误: {np.mean([r['protocol_errors'] for r in results]):.1f}",
            f"- 时序合规率: {sum(1 for r in results if r['timing_ok']) / len(results) * 100:.1f}%",
            "",
            "## 详细结果",
            ""
        ]
        
        for i, result in enumerate(results):
            report_lines.append(f"### 测试 #{i}")
            report_lines.append(f"- 帧数: {result['frame_count']}")
            report_lines.append(f"- 协议错误: {result['protocol_errors']}")
            report_lines.append(f"- 时序合规: {'是' if result['timing_ok'] else '否'}")
            report_lines.append(f"- 频率: {result['frequency_mhz']:.2f} MHz")
            report_lines.append("")
        
        report_content = "\n".join(report_lines)
        
        os.makedirs("reports", exist_ok=True)
        report_filename = f"reports/test_report_{time.strftime('%Y%m%d_%H%M%S')}.md"
        with open(report_filename, 'w', encoding='utf-8') as f:
            f.write(report_content)
        
        self._log(f"报告已生成: {report_filename}")
        return report_filename
    
    def run_test(self) -> bool:
        """运行单次测试"""
        self._log("=== 开始测试 ===")
        
        df = self.capture_sample_data()
        result = self.analyze_protocol(df)
        
        success = (result['protocol_errors'] == 0) and result['timing_ok']
        self._log(f"测试结果: {'通过' if success else '失败'}")
        
        return success
    
    def run_cycle_test(self, cycles: int = 5) -> Dict:
        """运行循环测试"""
        self._log(f"=== 开始循环测试 ({cycles}次) ===")
        
        results = []
        passed = 0
        
        for i in range(cycles):
            self._log(f"\n--- 测试 {i+1}/{cycles} ---")
            self._log(f"=== 开始测试 #{i} ===")
            
            df = self.capture_sample_data()
            result = self.analyze_protocol(df)
            results.append(result)
            
            if result['protocol_errors'] == 0 and result['timing_ok']:
                passed += 1
                self._log("测试结果: 通过")
            else:
                self._log("测试结果: 失败")
        
        report_path = self.generate_report(results)
        
        self._log("\n=== 循环测试完成 ===")
        self._log(f"通过: {passed}/{cycles} ({passed/cycles*100:.1f}%)")
        self._log(f"报告: {report_path}")
        
        return {
            'passed': passed,
            'total': cycles,
            'report_path': report_path,
            'results': results
        }

if __name__ == "__main__":
    automation = DSLogicAutomation()
    result = automation.run_cycle_test(5)
