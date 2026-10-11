#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "collect_data.h"
#include "process.h"
#include <string.h>
#include "kalman.h"
#include "fft_process.h"
#include "main_fun.h"
#include "BLE.h"


/*参数配置*/
#define FEATURE_DIM 8
#define SAMPLES 150
#define PRESAMOLES 60        // 环形缓冲区大小
#define HIST_READ_COUNT 20   // 触发录制时回溯历史帧数


/*存储数据*/
static uint16_t k = SAMPLES;
static float MPU6050_data[FEATURE_DIM * 300];
static float MPU6050_data_inter[FEATURE_DIM * SAMPLES];
static float infer_buf[2][FEATURE_DIM * SAMPLES];
static float maf_data[SAMPLES * FEATURE_DIM];

/*状态机*/
enum State{
    IDLE,
    RECOGNITION,
    COOLDOWN
}; 

/*结构体*/
typedef struct
{
    float pre_buffer[FEATURE_DIM * PRESAMOLES];
    uint16_t read;
    uint16_t write;

} ring_buffer;

static ring_buffer pre;

typedef struct 
{
    float *buf;
} infer_msg_t;  // 推理双缓存


/*全局变量*/
volatile float E = 0;
volatile State state = IDLE;
static int8_t start_count = 0;
static int8_t end_count = 0, cool_count = 0;
static uint16_t data_index = 0;
static float acc_std = 0;
static float energy_fft = 0;
static bool recog_enter = false;


static float peak_E = 0;
static float E_smooth = 0;

static float baseline_E = 0;
static float last_E = 0;

QueueHandle_t infer_queue;
static uint8_t active_buf = 0;

// ble发送相关变量
static uint32_t input_number = 0;
static bool has_input = false;
static uint16_t wait_send_count = 0;

/*函数声明*/
void data_exam(void);
void print_data(const float* data);
void sample_task(void *arg);
void infer_task(void *arg);
void write_ring_buffer(void);
void read_ring_buffer(void);
void move_av_filter(float *data);

// 声明任务句柄
TaskHandle_t samp_task_handle = NULL;
TaskHandle_t infer_task_handle = NULL;

/*运行主逻辑*/
void process_logic(void)
{
    Init_kalman();
    fft_Init();

    infer_queue = xQueueCreate(2, sizeof(infer_msg_t));
    xTaskCreatePinnedToCore(sample_task,
                            "samp_task",
                            4096,
                            NULL,
                            5,
                            &samp_task_handle,
                            1);

    xTaskCreatePinnedToCore(infer_task,
                            "infer_task",
                            4096,
                            NULL,
                            3,
                            &infer_task_handle,
                            0);

}


/*数据采集task*/
void sample_task(void* arg)
{
    pre.read = 0;
    pre.write = 0;
    TickType_t last = xTaskGetTickCount();
    TickType_t task_begin;
    TickType_t task_end;
    while(1)
    {
        get_data();
        cal_energy((float*)&E);

        float delta_E = E - last_E;
        last_E = E;

        switch(state)
        {
            case IDLE:
            {
                write_ring_buffer();
                
                if(has_input)
                {
                
                    wait_send_count++;

                    if(wait_send_count > 20)
                    {
                        ble_send_page(input_number); // 发送延时4ms
                        // printf("ble send time = %ld\n", pdTICKS_TO_MS(task_end - task_begin));
                        input_number = 0;
                        wait_send_count = 0;
                        has_input = false;
                    }
                }


                baseline_E = 0.99f * baseline_E + 0.01f * E;
                float dynamic_start = baseline_E * 2.0f + 2.0f;

                if(E > dynamic_start)
                {
                    if(delta_E > 2.0f) 
                    { 
                        start_count += 3; 
                    } else 
                    { 
                        start_count++; 
                    } 
                    
                    if(start_count > 10) 
                    { 
                        state = RECOGNITION; 
                        start_count = 0; 
                    }
                }
                else
                    start_count = 0;
            
            break;
            }

            case RECOGNITION:
            {
                if(!recog_enter)
                {
                    read_ring_buffer(); // 放前HIST_READ_COUNT帧
                    data_index = HIST_READ_COUNT;
                    peak_E = E;
                    recog_enter = true;
                }

                if(data_index < 300)
                {
                    fill_mpu_data(MPU6050_data, data_index++);
                    E_smooth = 0.8f * E_smooth + 0.2f * E;

                    if(E_smooth > peak_E)
                    {
                        peak_E = E_smooth;
                    }
                }

                
                // printf("Start recording, data_index = %d, E = %f\n",data_index, E);
                if(data_index > 40)
                {
                    float dynamic_end = peak_E * 0.25f;
                    if (E_smooth < dynamic_end)
                    {
                        end_count++;
                        if (end_count > 15)
                        {
                            state = COOLDOWN;
                            end_count = 0;
                            recog_enter = false;
                        }
                    }
                    else
                        end_count = 0;
                }

                if(data_index >= 300)
                {
                    state = COOLDOWN;
                    recog_enter = false;
                }

            break;
            }
            
            case COOLDOWN:
            {
                if(cool_count == 0)
                {   
                    data_exam();
                    data_index = 0;
                    move_av_filter(MPU6050_data_inter); 

                    // 数据传递
                    float *cur_buf = infer_buf[active_buf];
                    memcpy(cur_buf, MPU6050_data_inter, sizeof(MPU6050_data_inter));
                    infer_msg_t msg = {
                        .buf = cur_buf
                    };

                    xQueueSend(infer_queue, &msg, 0);
                    active_buf ^= 1; // 切换buffer


                    // printf("End recording. deltaE = %f\n", E - END_THRESHOLD);
                }

                cool_count++;
                if (cool_count > 20)
                {
                    state = IDLE;
                    cool_count = 0;
                }
            break; 
            }
            
        }
        

        vTaskDelayUntil(&last, pdMS_TO_TICKS(10));  // 1/0.01 = 100Hz
    }
}


/*推理task*/
void infer_task(void *arg)
{
    infer_msg_t msg;
    static int result = 0;

    while (1)
    {
        if(xQueueReceive(infer_queue, &msg, portMAX_DELAY))
        {
            fft_fun(msg.buf, &acc_std, &energy_fft);  //判断标准差和谱能量, 防止误触情况进行推理
            // printf("acc_std = %f, energy_fft = %f\n", acc_std, energy_fft);

            if(acc_std > 0.15f && energy_fft > 300.0f)
            {
                // print_data(MPU6050_data_inter);
                norm(msg.buf, SAMPLES, FEATURE_DIM);

                loop(msg.buf, &result);

                input_number = input_number*10 + result;
                has_input = true;
                wait_send_count = 0; // 重置发送计数


            }
        }

    }
    
}


/*对未采集到k个时刻的数据进行插值补充, 以及超过的数据进行下采样*/
void data_exam(void)
{
    // 1.当采样点数等于有效点数时, 直接复制
    if(data_index == k)
    {
        memcpy(MPU6050_data_inter, MPU6050_data, sizeof(MPU6050_data_inter));
        return;
    }

    // 2. 当采样点数小于2时, 填充0
    if(data_index < 2)
    {
        memset(MPU6050_data_inter, 0, sizeof(MPU6050_data_inter));
        return;
    }

    //3. 对未采集到k个时刻的数据进行插值 or 对超过samples的数据进行下采样 
    float r = (float)(data_index - 1) / (k-1);    

    for(uint16_t j = 0; j < k; j++)
    {
        float index = j * r;
        int pre_index = (int)index;   // 原数据上一个整点索引

        uint16_t target_offset = 8 * j;
        if(pre_index >= data_index - 1)
        {
            // 处理边界, 直接复制最后一个点数据
            uint16_t src_offset = 8 * (data_index - 1);
            for (uint8_t i = 0; i < 8; i++)
            {
                MPU6050_data_inter[i + target_offset] = MPU6050_data[i + src_offset];  
            }
        }
        else
        {
            float delta_x = index - pre_index;       // x差值
            uint16_t src_offset_1 = 8 * pre_index;      // 对应原数据下一个索引
            uint16_t src_offset_2 = 8 * (pre_index + 1);
            
            for (uint8_t i = 0; i < 8; i++)
            {
                float y2 = MPU6050_data[i + src_offset_2];  //对应原数据的下一个整点值
                float y1 = MPU6050_data[i + src_offset_1];        //对应原数据的上一个整点值
                float inter_data = delta_x * (y2 - y1) + y1;  // 所需插入值, 线性插值
                MPU6050_data_inter[i + target_offset] = inter_data;  // 存储
            }
        }
    }
}



/*打印输出, 用于导出数据*/
void print_data(const float* data)
{
    for(uint16_t i = 0; i < k; i++)
    {
        int base = 8*i;
        printf("%f,%f,%f,%f,%f,%f,%f,%f\n", 
            data[base+0], data[base+1], data[base+2],
            data[base+3], data[base+4], data[base+5],
            data[base+6], data[base+7]);
    }
}


/*写入环形缓冲区*/
void write_ring_buffer(void)
{
    uint16_t size = FEATURE_DIM * PRESAMOLES;
    fill_mpu_data(pre.pre_buffer, pre.write / FEATURE_DIM);
    pre.write = (pre.write + FEATURE_DIM) % size;
}


/*读取环形缓冲区*/
void read_ring_buffer(void)
{
    uint16_t size = FEATURE_DIM * PRESAMOLES;
    pre.read = (pre.write - FEATURE_DIM * HIST_READ_COUNT + size) % size; // 最近HIST_READ_COUNT个数据点的起始位置
    
    for(uint16_t i = 0; i < HIST_READ_COUNT; i++)
    {
        for(uint16_t j = 0; j < FEATURE_DIM; j++)
        {
            MPU6050_data[i*FEATURE_DIM + j] = pre.pre_buffer[(pre.read + j) % size];
        }
        pre.read = (pre.read + FEATURE_DIM) % size;
    }
    
}


/*移动平均滤波*/
void move_av_filter(float *data)
{
    const uint16_t N = 5;     // 滤波窗口大小

    for(uint16_t j = 0; j < FEATURE_DIM; j++)
    {
        float sum = 0.0f;
        for(uint16_t i = 0; i < SAMPLES; i++)
        {
            float x_n = data[i*FEATURE_DIM + j];

            if(i < N - 1)
            {
                sum += x_n;
                maf_data[i*FEATURE_DIM + j] = x_n;
            }
            else if(i == N - 1)
            {
                sum += x_n;
                maf_data[i*FEATURE_DIM + j] = sum / N;
            }
            else
            {
                sum += x_n;
                sum -= data[(i - N) * FEATURE_DIM + j];
                maf_data[i*FEATURE_DIM + j] = sum / N;  // y[n] = (x[n] + x[n-1] +...+ x[n-N+1]) / N;
            }
        }
    }

    memcpy(data, maf_data, sizeof(maf_data));
}