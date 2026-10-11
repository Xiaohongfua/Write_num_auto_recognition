#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "esp_heap_caps.h"
#include "main_fun.h"
#include "model.h"
#include "driver/gpio.h"


namespace{
    const tflite::Model* model = nullptr;  // 指向模型
    tflite::MicroInterpreter* interpreter = nullptr; // 推理引擎
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;
    int inference_count = 0;

    constexpr int kTensorArenaSize = 10000; // 内存分配20kb -> SPIRAM 10kb
    // uint8_t* tensor_arena = (uint8_t*) heap_caps_malloc(
    //                         kTensorArenaSize,
    //                         MALLOC_CAP_SPIRAM
    //                         );

    // alignas(16) static uint8_t tensor_arena[kTensorArenaSize];
    EXT_RAM_BSS_ATTR  alignas(16) static uint8_t tensor_arena[kTensorArenaSize];

    
} // namespace


// 初始化模型
void setup(void)
{   

    // if (tensor_arena)
    // {
    //     printf("tensor_arena malloc is ok!\n");
    // }
    // else
    // {
    //     printf("tensor_arena falied malloc!\n");
    // }
    
    // 加载模型
    model = tflite::GetModel(cnn_model);
    if(model->version() != TFLITE_SCHEMA_VERSION)
    {
        MicroPrintf("Model provided is schema version %d not equal to supported"
            "version %d.", model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::MicroMutableOpResolver<10> resolver;

    resolver.AddExpandDims();
    resolver.AddConv2D();
    resolver.AddReshape();
    resolver.AddMaxPool2D();
    resolver.AddMean();
    resolver.AddRelu();
    resolver.AddFullyConnected();
    resolver.AddQuantize();
    resolver.AddDequantize();
    resolver.AddSoftmax();

    
    // 创建解释器，绑定模型+算子+内存
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize); 
    interpreter = &static_interpreter;
    
    // 分配内存
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if(allocate_status != kTfLiteOk)
    {
        MicroPrintf("AllocateTensors() failed.");
        return;
    }

    // 获取输入输出
    input = interpreter->input(0);
    output = interpreter->output(0);

    // 初始化推理次数
    inference_count = 0;

    printf("setup is ok.\n");

}


// 模型推理
void loop(float* MPU6050_data, int* result)
{
    // 获取输入
    int data_index = 0;
    for(int t = 0; t < 150; t++)
    {
        for(int f = 0; f < 8; f++)
        {
            data_index = t * 8 + f;
            float val = MPU6050_data[data_index];
            // 量化输入 float ->>>> int8, 并写入张量
            int32_t quantized = val / input->params.scale + input->params.zero_point;   
            if(quantized > 127) quantized = 127;
            if(quantized < -128) quantized = -128;
            input->data.int8[data_index] = (int8_t)quantized;
        }
    }

    // printf("Getting Input is ok.\n");

    // 执行推理
    TfLiteStatus invoke_status = interpreter->Invoke();
    if(invoke_status != kTfLiteOk)
    {
        MicroPrintf("Invoke failed on x: %f\n", MPU6050_data);
        return;
    }

    // printf("Invoking is ok.\n");

    // 获取模型的输出
    int max_output_index = 0;
    int8_t max_val = output->data.int8[0];

    for(int i = 1; i < 10; i++)
    {
        if(output->data.int8[i] > max_val)
        {
            max_val = output->data.int8[i];
            max_output_index = i;
        }
    }

    // 反量化输出结果 int8 ->>>  float
    // float y = (max_val - output->params.zero_point) * output->params.scale;

    //  -----对结果进行处理---------
    // if(max_output_index == 10)
    //     max_output_index = 404;
    // if(y > 0.0f)
    // {
    //     printf("Predict number: %d, val: %f\n", max_output_index, y);
    // }
    // else
    // {
    //     printf("404, val: %f\n", y);
    // }

    *result = max_output_index;  

}





