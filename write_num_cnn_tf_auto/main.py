import keras
import numpy as np
import tensorflow as tf
import time
from model import build_model
from dataload import load_seg_dataset, make_tf_dataset
from sklearn.model_selection import train_test_split


def augment(x):

    x = x.copy()

    # 1. 幅值缩放
    scale = np.random.uniform(0.9, 1.1)
    x *= scale

    # 2. bias 漂移
    bias = np.random.normal(
        0,
        0.02,
        (1, x.shape[1])
    )

    x += bias

    # 3. 高斯噪声
    noise = np.random.normal(
        0,
        0.01,
        x.shape
    )

    x += noise

    return x.astype(np.float32)




def train(model, train_dataset, optimizer, loss_fn, epochs=100):
    for epoch in range(epochs):
        total_loss = 0.0
        correct = 0
        total = 0

        for inputs, targets in train_dataset:
            with tf.GradientTape() as tape:
                outputs = model(inputs, training=True)
                loss = loss_fn(targets, outputs)

            grads = tape.gradient(loss, model.trainable_variables)
            optimizer.apply_gradients(zip(grads, model.trainable_variables))

            total_loss += loss.numpy()

            preds = tf.argmax(outputs, axis=1, output_type=tf.int32)
            correct += tf.reduce_sum(tf.cast(preds == targets, tf.int32)).numpy()
            total += targets.shape[0]

        if (epoch + 1) % 1 == 0:
            print(
                f"Epoch [{epoch+1}/{epochs}] "
                f"Loss: {total_loss/len(train_dataset):.4f} "
                f"Acc: {100*correct/total:.2f}%"
            )

def test(model, test_dataset):
    correct = 0
    total = 0
    wrong_samples = []

    for batch_idx, (inputs, targets) in enumerate(test_dataset):
        outputs = model(inputs, training=False)
        preds = tf.argmax(outputs, axis=1, output_type=tf.int32)

        correct += tf.reduce_sum(tf.cast(preds == targets, tf.int32)).numpy()
        total += targets.shape[0]

        for i in range(len(preds)):
            if preds[i] != targets[i]:
                wrong_samples.append({
                    'batch': batch_idx,
                    'index_in_batch': i,
                    'pred': int(preds[i].numpy()),
                    'target': int(targets[i].numpy())
                })

    acc = 100 * correct / total
    print(f"Test Accuracy: {acc:.2f}%")

    print(f"\n错误样本数量: {len(wrong_samples)}")
    for w in wrong_samples:
        print(f"预测: {w['pred']} | 实际: {w['target']} | batch: {w['batch']} idx: {w['index_in_batch']}")


if __name__ == "__main__":
    start_time = time.time()

    # 1. 读取
    samples = load_seg_dataset("basic_data")

    # 2. 拆分
    x = [s[0] for s in samples]
    y = [s[1] for s in samples]

    x_train, x_test, y_train, y_test = train_test_split(
        x, y, test_size=0.2, random_state=42, stratify=y
    )

    x_calib, _, y_calib, _ = train_test_split(
        x_train, y_train, test_size=0.5, random_state=42, stratify=y_train
    )

    # 3. 标准化
    X_train_np = np.concatenate(x_train, axis=0)
    mean = X_train_np.mean(axis=0, keepdims=True)
    std = X_train_np.std(axis=0, keepdims=True) + 1e-8

    array_m_s = np.zeros((2, mean.shape[1]))
    array_m_s[0] = mean
    array_m_s[1] = std
    np.savetxt('mean_std.txt', array_m_s, delimiter=',', fmt='%.6f')

    x_train = [(x - mean) / std for x in x_train]
    x_test = [(x - mean) / std for x in x_test]

    x_calib = [(x - mean) / std for x in x_calib]  # 校正数据


    #----扩充数据集--------
    # aug_x = []
    # aug_y = []
    #
    # for xx, yy in zip(x_train, y_train):
    #
    #     # 原始数据
    #     aug_x.append(xx)
    #     aug_y.append(yy)
    #
    #     # 增强3次
    #     for _ in range(3):
    #         aug_x.append(
    #             augment(xx)
    #         )
    #
    #         aug_y.append(yy)
    #
    # x_train = aug_x
    # y_train = aug_y

    # 4. 转 numpy
    x_train = np.array(x_train, dtype=np.float32)
    y_train = np.array(y_train, dtype=np.int32)

    x_test = np.array(x_test, dtype=np.float32)
    y_test = np.array(y_test, dtype=np.int32)

    x_calib = np.array(x_calib, dtype=np.float32)
    np.save(fr'calib_and_test_data/x_calib.npy', x_calib)
    np.save(fr'calib_and_test_data/x_test.npy', x_test)
    np.save(fr'calib_and_test_data/y_test.npy', y_test)

    # 5. 转 tf.data
    train_dataset = make_tf_dataset(x_train, y_train, batch_size=8, shuffle=True)
    test_dataset = make_tf_dataset(x_test, y_test, batch_size=8, shuffle=False)

    model = build_model(8,10)

    optimizer = keras.optimizers.Adam(learning_rate=1e-3)
    loss_fn = keras.losses.SparseCategoricalCrossentropy(from_logits=True)

    train(model, train_dataset, optimizer, loss_fn, epochs=30)  #30
    test(model, test_dataset)

    model.save("best_model.keras")



    end_time = time.time()
    print(f"Total time: {end_time - start_time:.2f} s")