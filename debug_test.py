
import numpy as np
import pandas as pd

# 参数设置
sample_rate = 100  # MHz
test_duration_ms = 100
clk_period_ns = 10

num_samples = int(sample_rate * 1000 * test_duration_ms / 1000)
time_ns = np.arange(num_samples) * (1000 / sample_rate)

# 生成时钟信号
ch0 = (time_ns // clk_period_ns) % 2
ch1 = np.zeros(num_samples, dtype=int)

# 生成SWD帧
frame_start = 90
bit_time = clk_period_ns

for frame_idx in range(2):
    frame_pos = frame_start + frame_idx * 600
    
    for bit_pos_in_frame in range(40):
        bit_pos = int((frame_pos + bit_pos_in_frame * bit_time) / bit_time)
        if bit_pos >= num_samples:
            continue
        
        if bit_pos_in_frame == 0:
            ch1[bit_pos] = 1
        elif bit_pos_in_frame == 1:
            ch1[bit_pos] = frame_idx % 2
        elif bit_pos_in_frame == 2 or bit_pos_in_frame == 3:
            ch1[bit_pos] = 0
        elif bit_pos_in_frame >= 4 and bit_pos_in_frame <= 6:
            ack_pattern = [1, 0, 0]
            ch1[bit_pos] = ack_pattern[bit_pos_in_frame - 4]
        elif bit_pos_in_frame >= 7 and bit_pos_in_frame <= 38:
            data = 0x1BA01477 + frame_idx
            data_bit_idx = bit_pos_in_frame - 7
            ch1[bit_pos] = (data >> data_bit_idx) & 1
        else:
            ch1[bit_pos] = 0

# 打印生成的数据
print("=== 生成的SWD数据 ===")
print(f"帧0起始位置: {frame_start}ns")

# 检查帧0的位
print("\n帧0 (100-500ns):")
for bit_idx in range(40):
    bit_pos = int((frame_start + bit_idx * bit_time) / bit_time)
    if bit_pos < len(ch1):
        print(f"  bit {bit_idx}: {ch1[bit_pos]}", end="")
        if bit_idx in [0, 1, 3, 6, 38, 39]:
            print()

# 解析帧
print("\n=== 解析帧 ===")
df = pd.DataFrame({'Time(ns)': time_ns, 'CH0': ch0, 'CH1': ch1})

frames = []
frame_start = None
current_bits = []
in_frame = False

print("\n时钟上升沿采样点:")
for i in range(1, len(df)):
    if df.loc[i-1, 'CH0'] == 0 and df.loc[i, 'CH0'] == 1:
        print(f"  时间={df.loc[i, 'Time(ns)']}ns, CH1={df.loc[i, 'CH1']}")
        
        if not in_frame and df.loc[i, 'CH1'] == 1:
            frame_start = df.loc[i, 'Time(ns)']
            in_frame = True
            current_bits = []
            print(f"    -> 帧开始")
        
        if in_frame:
            current_bits.append(df.loc[i, 'CH1'])
            
            if len(current_bits) >= 40:
                frames.append({
                    'start_time': frame_start,
                    'bits': current_bits.copy()
                })
                in_frame = False
                current_bits = []

print(f"\n解析到 {len(frames)} 帧")
for i, frame in enumerate(frames):
    print(f"\n帧{i}:")
    print(f"  起始时间: {frame['start_time']}ns")
    print(f"  所有位: {frame['bits'][:10]}...")
    print(f"  ACK位 (bits[4:7]): {frame['bits'][4:7]}")
    ack_value = frame['bits'][4] + (frame['bits'][5] << 1) + (frame['bits'][6] << 2)
    print(f"  ACK值: {ack_value}")
