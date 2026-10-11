# TinyML-Based Air-Writing Digit Recognition System (Non-Vision Approach)

A TinyML-based air-writing digit recognition system built with ESP32-S3 and MPU6050.

## Overview

This project implements a non-vision-based air-writing digit recognition system using ESP32-S3 and MPU6050. By leveraging IMU motion data and a CNN model deployed with TensorFlow Lite Micro, the system performs real-time recognition of handwritten digits from 0 to 9 directly on the microcontroller.

The system supports continuous multi-digit input and uses Bluetooth Low Energy (BLE) to control page navigation in Microsoft Word and PowerPoint.

## Demo Video

Watch the project demonstration on Bilibili:

[**▶ Watch the Demo Video on Bilibili**](https://www.bilibili.com/video/BV1V7g56WEuA/?share_source=copy_web&vd_source=af7f3486c564ddec02165006054fb817)

## Repository Contents

| Directory | Description |
| --- | --- |
| `write_num_auto_recognition` | Complete ESP32-S3 firmware project built with ESP-IDF v5.2, including data acquisition, preprocessing, inference, and BLE communication. |
| `write_num_cnn_tf_auto` | Code for receiving training data via serial communication, training the model, and quantizing the trained model. Supports retraining with newly collected IMU data and deploying the updated model. |
| `receive_from_esp` | Windows host-side application for receiving BLE data and controlling page navigation in PowerPoint, Word, and other supported applications. |

## Directory Structure

```text
.
├── write_num_auto_recognition/
├── write_num_cnn_tf_auto/
└── receive_from_esp/
```

## Tech Stack

- **Hardware:** ESP32-S3, MPU6050
- **Development Framework:** ESP-IDF v5.2
- **Machine Learning:** CNN, TensorFlow Lite Micro, INT8 quantization
- **Communication:** Bluetooth Low Energy (BLE)
- **Host Platform:** Windows

## Performance

- **Recognition Accuracy:** 97%
- **Inference Latency:** 13 ms per inference
- **Tensor Arena Usage:** 10 KB
- **Deployment Platform:** ESP32-S3
- **Model Optimization:** INT8 quantization with TensorFlow Lite Micro

The CNN model achieves 97% recognition accuracy with an inference latency of 13 ms and a Tensor Arena memory footprint of 10 KB on the ESP32-S3, enabling efficient real-time digit recognition on a resource-constrained microcontroller.

## Notes

- Make sure ESP-IDF v5.2 is installed and properly configured before building the ESP32-S3 firmware.
- After retraining the model, copy the generated quantized model into the ESP32-S3 firmware project.
- Ensure that the normalization parameters used during training are consistent with those used during on-device inference. Update the corresponding parameters in the firmware whenever necessary.
- BLE connection settings and the host-side runtime environment may need to be configured according to your setup.
