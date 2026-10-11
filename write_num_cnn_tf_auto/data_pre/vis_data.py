
import os
import pandas as pd
import matplotlib.pyplot as plt

# 数据目录
path = r'C:\Users\x\Desktop\write_num_cnn_tf_auto\basic_data'

# 获取所有csv
files = sorted(os.listdir(path))

# 创建2x5子图
fig, axes = plt.subplots(2, 6 ,figsize=(18, 8))

axes = axes.flatten()

# 遍历文件

for idx, file in enumerate(files):

    if file.endswith(".csv"):

        file_path = os.path.join(path, file)

        # 读取数据
        data = pd.read_csv(file_path, header=None).values

        # 前150个点
        ax = data[0:3000, 0]
        ay = data[0:3000, 1]
        az = data[0:3000, 2]

        # gx = data[0:3000, 3]
        # gy = data[0:3000, 4]
        # gz = data[0:3000, 5]


        t = range(3000)

        agx = axes[idx]

        # 三轴角速度
        agx.plot(t, ax, label='ax')
        agx.plot(t, ay, label='ay')
        agx.plot(t, az, label='az')

        # agx.plot(t, gx, label='gx')
        # agx.plot(t, gy, label='gy')
        # agx.plot(t, gz, label='gz')

        # 标题
        agx.set_title(file.replace(".csv", ""))

        # 网格
        agx.grid(True)

        # 图例
        agx.legend(fontsize=8)

# 自动布局
plt.tight_layout()

# 显示
plt.show()
