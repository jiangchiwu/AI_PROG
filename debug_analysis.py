import numpy as np
import pandas as pd

sample_rate = 100  # MHz
test_duration_ms = 1
num_samples = int(sample_rate * 1000 * test_duration_ms / 1000)
time_ns = np.arange(num_samples) * (1000 / sample_rate)

clk_period_ns = 10
ch0 = (time_ns // clk_period_ns) % 2
ch1 = np.zeros(num_samples, dtype=int)

all_frames = []
for frame_idx in range(3):
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

print("帧数据定义:")
for i, f in enumerate(all_frames):
    print(f"帧{i}: 起始={f[0]}, 读写={f[1]}, 地址={f[2:4]}, ACK={f[4:7]}, 数据前4位={f[7:11]}")

frame_start_sample = 9
bits_per_frame = 40
frame_gap_samples = 20
frame_interval_samples = bits_per_frame + frame_gap_samples

for frame_idx, frame_data in enumerate(all_frames):
    start_sample = frame_start_sample + frame_idx * frame_interval_samples
    print(f"\n帧{frame_idx} 起始样本: {start_sample}, 起始时间: {start_sample*10}ns")
    for bit_idx, bit_value in enumerate(frame_data):
        sample_pos = start_sample + bit_idx
        if sample_pos < num_samples:
            ch1[sample_pos] = bit_value
            if sample_pos + 1 < num_samples:
                ch1[sample_pos + 1] = bit_value

print("\n时钟上升沿位置（样本索引，对应时间）:")
rising_edges = []
for i in range(1, len(ch0)):
    if ch0[i-1] == 0 and ch0[i] == 1:
        rising_edges.append((i, time_ns[i]))
        if len(rising_edges) <= 50:
            print(f"  样本{i}: {time_ns[i]}ns")

print("\n帧解析测试:")
for frame_idx in range(len(all_frames)):
    start_sample = frame_start_sample + frame_idx * frame_interval_samples
    print(f"\n帧{frame_idx} 采样点:")
    for bit_idx in range(min(10, bits_per_frame)):
        sample_pos = start_sample + bit_idx
        if sample_pos < len(ch0):
            clk_val = ch0[sample_pos]
            data_val = ch1[sample_pos]
            print(f"  位{bit_idx}: 样本={sample_pos}, 时间={time_ns[sample_pos]}ns, CLK={clk_val}, DATA={data_val}")

df = pd.DataFrame({
    'Time(ns)': time_ns,
    'CH0': ch0,
    'CH1': ch1
})

df.to_csv('debug_data.csv', index=False)
print("\n调试数据已保存到 debug_data.csv")