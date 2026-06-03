
import numpy as np

sample_rate = 100  # MHz
test_duration_ms = 100
clk_period_ns = 10

num_samples = int(sample_rate * 1000 * test_duration_ms / 1000)
time_ns = np.arange(num_samples) * (1000 / sample_rate)

# 生成时钟信号
ch0 = (time_ns // clk_period_ns) % 2
ch1 = np.zeros(num_samples, dtype=int)

# 直接在时钟上升沿设置值
for i in range(num_samples):
    # 检查上升沿：i=0时检查初始状态，否则检查前一个样本
    if (i == 0 and ch0[i] == 1) or (i > 0 and ch0[i-1] == 0 and ch0[i] == 1):
        # 这是时钟上升沿
        time_at_edge = time_ns[i]
        
        # 计算属于哪个帧
        frame_start = 100
        frame_interval = 500
        frame_idx = int((time_at_edge - frame_start) // frame_interval)
        
        if frame_idx >= 0 and frame_idx < 5:
            # 计算在帧中的位位置
            pos_in_frame = (time_at_edge - frame_start) % frame_interval
            bit_pos = int(pos_in_frame // clk_period_ns)
            
            if bit_pos < 40:
                # 设置该位的值
                if bit_pos == 0:
                    ch1[i] = 1  # 起始位
                elif bit_pos == 1:
                    ch1[i] = frame_idx % 2  # 读写位
                elif bit_pos == 2 or bit_pos == 3:
                    ch1[i] = 0  # 地址位
                elif bit_pos >= 4 and bit_pos <= 6:
                    ack_pattern = [1, 0, 0]
                    ch1[i] = ack_pattern[bit_pos - 4]
                elif bit_pos >= 7 and bit_pos <= 38:
                    data = 0x1BA01477 + frame_idx
                    ch1[i] = (data >> (bit_pos - 7)) & 1
                else:
                    ch1[i] = 0

# 打印时钟上升沿处的值
print("时钟上升沿采样点:")
for i in range(1, num_samples):
    if ch0[i-1] == 0 and ch0[i] == 1:
        time_at_edge = time_ns[i]
        frame_start = 100
        frame_interval = 500
        frame_idx = int((time_at_edge - frame_start) // frame_interval)
        
        if frame_idx >= 0 and frame_idx < 5:
            pos_in_frame = (time_at_edge - frame_start) % frame_interval
            bit_pos = int(pos_in_frame // clk_period_ns)
            
            if bit_pos < 40:
                print(f"时间={time_at_edge}ns, 帧={frame_idx}, 位={bit_pos}, 值={ch1[i]}")
                if bit_pos >= 4 and bit_pos <= 6:
                    print(f"    -> ACK位 {bit_pos-4}: {ch1[i]}")
