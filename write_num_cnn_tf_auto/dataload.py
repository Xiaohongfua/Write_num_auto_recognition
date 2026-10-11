import numpy as np
import pandas as pd
import os

def load_seg_dataset(folder):
    samples = []

    files = sorted(os.listdir(folder))


    for file in files:
        path = os.path.join(folder, file)
        data = pd.read_csv(path, header=None).values

        label = int(file[0])  # "3_xxx.csv" → 3

        # 按64切片
        for i in range(0, len(data) - 150 + 1, 150):
            x = data[i:i+150, :]
            samples.append((x.astype(np.float32), label))

    return samples


def build_dataset(samples):
    x = np.array([s[0] for s in samples], dtype=np.float32)
    y = np.array([s[1] for s in samples], dtype=np.int32)
    return x, y

import tensorflow as tf

def make_tf_dataset(x, y, batch_size=8, shuffle=True):
    ds = tf.data.Dataset.from_tensor_slices((x, y))

    if shuffle:
        ds = ds.shuffle(buffer_size=len(x))

    ds = ds.batch(batch_size)
    return ds