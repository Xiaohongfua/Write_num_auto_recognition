import keras
import tensorflow as tf
import numpy as np

#
real_data = np.load('calib_and_test_data/x_calib.npy')

def representative_dataset():
    for i in range(70):
        x = real_data[i]
        x = np.expand_dims(x, axis=0)  # batch=1
        yield [x.astype(np.float32)]


model_path = f"best_model.keras"
model = keras.models.load_model(model_path)
model.input.set_shape([1, 150, 8])  # 修改输入形状, batch=1

converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_dataset

converter.target_spec.supported_ops = [
    tf.lite.OpsSet.TFLITE_BUILTINS_INT8
]
converter.target_spec.supported_types = [tf.int8]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8

tflite_model = converter.convert()

with open(f"model.tflite", "wb") as f:
    f.write(tflite_model)

tflite_model_path = f"model.tflite"
interpreter = tf.lite.Interpreter(model_path=tflite_model_path)


interpreter.allocate_tensors()

ops = interpreter._get_ops_details()
print("一共有 {} 个算子:".format(len(ops)))
for op in ops:
    print(op['op_name'])

# ================= 修改部分：提取算子名称并去重 =================
# 提取所有的算子名称
op_names = [op['op_name'] for op in ops]

# 对算子名称去重，并保持一定的顺序（Python 3.7+ dict 保持插入顺序）
unique_op_names = list(dict.fromkeys(op_names))

print("\n去重后一共有 {} 种算子:".format(len(unique_op_names)))
for name in unique_op_names:
    count = op_names.count(name)  # 统计该算子在模型中出现的次数
    print(f"  - {name}: 出现 {count} 次")
# ============================================================


input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()



# print("=== Input Details ===")
# for i, detail in enumerate(input_details):
#     print(f"Input {i}:")
#     print(f"  Name: {detail['name']}")
#     print(f"  Shape: {detail['shape']}")
#     print(f"  Data Type: {detail['dtype']}")
#
# print("\n=== Output Details ===")
# for i, detail in enumerate(output_details):
#     print(f"Output {i}:")
#     print(f"  Name: {detail['name']}")
#     print(f"  Shape: {detail['shape']}")
#     print(f"  Data Type: {detail['dtype']}")
#
# # Get all layer details (Optional)
# print("\n=== Tensor Details ===")
# for tensor in interpreter.get_tensor_details():
#     print(f"Tensor Name: {tensor['name']}")
#     print(f"  Index: {tensor['index']}")
#     print(f"  Shape: {tensor['shape']}")
#     print(f"  Data Type: {tensor['dtype']}")
#     print()

tensor_details = interpreter.get_tensor_details()
total_memory = 0

for tensor in tensor_details:
    tensor_shape = tensor['shape']
    tensor_type = tensor['dtype']
    tensor_size = tf.dtypes.as_dtype(tensor_type).size * tf.reduce_prod(tensor_shape)
    total_memory += tensor_size.numpy()

print(f"Total Memory Required: {total_memory} bytes")


## -------------evaluate tflie on the test_data ------------

from tflite_evaluate import test_tflite
from dataload import make_tf_dataset
x_test = np.load('calib_and_test_data/x_test.npy')
y_test = np.load('calib_and_test_data/y_test.npy')
test_dataset = make_tf_dataset(x_test, y_test, batch_size=1, shuffle=False)

test_tflite('model.tflite', test_dataset)