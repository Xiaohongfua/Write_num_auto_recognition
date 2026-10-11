from numpy import fft
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# 读取数据
path = rf'C:\Users\x\Desktop\write_num_cnn_tf_auto\basic_data'
df = pd.read_csv(rf'{path}/6.csv', header=None)

array_data = df.values

# 标准化
norm_df = pd.read_csv('mean_std.csv', header=None)

# mean_std = norm_df.values
# mean = mean_std[0,:]
# std = mean_std[1,:]
#
# array_data = (array_data - mean)/std

# 参数
fs = 100
N = 150

num_action = array_data.shape[0] // N
plt.figure(figsize=(10,6))
x = range(num_action)
x_y = np.zeros(num_action)

for k in range(num_action):

    start = k * N
    end = start + N

    ax = array_data[start:end,0]
    ay = array_data[start:end,1]
    az = array_data[start:end,2]

    # 合加速度
    y = np.sqrt(ax**2 + ay**2 + az**2)

    # ===== 时域特征 =====
    std_time = np.std(y)

    # 去均值
    y = y - np.mean(y)

    # FFT
    fft_y = fft.fft(y)

    fft_abs = np.abs(fft_y)

    fft_half = fft_abs[:N//2]

    # ===== 频域特征 =====

    # 总能量
    total_energy = np.sum(fft_half**2)

    # 功率谱
    power = fft_half**2

    # 归一化
    p = power / (np.sum(power) + 1e-12)

    # 谱熵
    entropy = -np.sum(p * np.log(p + 1e-12))

    print(
        "std =", round(std_time,4),
        "energy =", round(total_energy,4),
        "entropy =", round(entropy,4)
    )

    x_y[k] = std_time

plt.plot(x, x_y, alpha=0.5)

plt.xlabel("Samples")
plt.ylabel("std")
plt.grid()
plt.show()