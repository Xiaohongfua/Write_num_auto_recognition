import numpy as np
import tensorflow as tf

def test_tflite(tflite_model_path, test_dataset):
    # 1. 加载模型
    interpreter = tf.lite.Interpreter(model_path=tflite_model_path)
    interpreter.allocate_tensors()

    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    correct = 0
    total = 0
    wrong_samples = []

    for batch_idx, (inputs, targets) in enumerate(test_dataset):
        inputs = inputs.numpy()

        for i in range(inputs.shape[0]):
            x = inputs[i:i+1]  # batch=1

            # =========================
            # 2. 处理 INT8 输入（关键）
            # =========================
            if input_details[0]['dtype'] == np.int8:
                scale, zero_point = input_details[0]['quantization']
                x = x / scale + zero_point
                x = np.clip(x, -128, 127).astype(np.int8)

            # 3. 推理
            interpreter.set_tensor(input_details[0]['index'], x)
            interpreter.invoke()

            output = interpreter.get_tensor(output_details[0]['index'])

            # =========================
            # 4. 反量化（如果是 INT8）
            # =========================
            if output_details[0]['dtype'] == np.int8:
                scale, zero_point = output_details[0]['quantization']
                output = (output.astype(np.float32) - zero_point) * scale

            pred = np.argmax(output, axis=1)[0]
            target = targets[i].numpy()

            if pred == target:
                correct += 1
            else:
                wrong_samples.append({
                    'batch': batch_idx,
                    'index_in_batch': i,
                    'pred': int(pred),
                    'target': int(target)
                })

            total += 1

    acc = 100 * correct / total
    print(f"Test Accuracy (TFLite): {acc:.2f}%")

    print(f"\n错误样本数量: {len(wrong_samples)}")
    for w in wrong_samples[:20]:  # 避免打印太多
        print(f"预测: {w['pred']} | 实际: {w['target']} | batch: {w['batch']} idx: {w['index_in_batch']}")