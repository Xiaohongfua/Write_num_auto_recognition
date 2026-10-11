import serial
import pandas as pd

# 配置串口参数
port = 'COM7'
baud_rate = 115200

path = r'C:\Users\x\Desktop\write_num_cnn_tf_auto\basic_data'
data_x = f'{path}/11.csv'
num = 0

try:
    # 打开串口
    ser = serial.Serial(port, baud_rate)
    print(f"串口 {port} 打开成功，波特率 {baud_rate}")

    # 打开或创建CSV文件
    with open(data_x, mode='a', newline='') as file1:
        while True:
            # 读取串口数据
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()

                try:
                    # 将数据按逗号分割并转换为float32
                    data = [float(x) for x in line.split(',')]

                    # 将数据转换为DataFrame
                    df_x = pd.DataFrame([data])

                    # 追加数据到CSV文件
                    df_x.to_csv(file1, header=False, index=False)

                    # 刷新文件缓冲区
                    file1.flush()
                    print(f"数据追加到CSV文件成功:{num + 1}")
                    num += 1

                except ValueError as e:
                    print(f"数据转换错误: {e}")

except serial.SerialException as e:
    print(f"串口打开失败: {e}")
except Exception as e:
    print(f"发生错误: {e}")