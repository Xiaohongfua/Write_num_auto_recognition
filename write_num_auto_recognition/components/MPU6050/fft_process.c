#include "fft_process.h"
#include "esp_dsp.h"
#include "math.h"

#define FFT_SIZE 256
#define SAMPLES 150
static const int16_t N = SAMPLES;

static float wind[FFT_SIZE];               // 汉宁窗
static float acc_mag[FFT_SIZE];            // 加速度
static float fft_input[FFT_SIZE*2];        // fft输入, 包含实部和虚部
static float fft_power[FFT_SIZE / 2];      // 功率谱, 对称, 取一半


/*初始化dsp库的fft函数*/
void fft_Init(void)
{
    esp_err_t res;
    res = dsps_fft2r_init_fc32(NULL, FFT_SIZE);
    if(res != ESP_OK)
    {
        perror("fft_Init is not success. ");
    }

    // 生成汉宁窗
    dsps_wind_hann_f32(wind, FFT_SIZE);
}


/*fft*/
void fft_fun(const float* data, float* acc_std, float* energy_fft)
{
    float mean = 0;          // 均值
    float s_2 = 0;           // 方差
    float total_energy = 0;  // 总能量
    float entropy = 0;       // 谱熵


    // 计算加速度大小
    for(uint16_t i = 0; i < N; i++)
    {   
        float ax = data[8*i + 0];  
        float ay = data[8*i + 1];
        float az = data[8*i + 2];
        acc_mag[i] = sqrtf(ax*ax + ay*ay + az*az);
        mean += acc_mag[i];
    }
    mean = mean / N;

    // 求标准差
    for(uint16_t i = 0; i < N; i++)
    {
        float s_2_i = acc_mag[i] - mean;
        s_2 += s_2_i * s_2_i;
    }
    s_2 = s_2 / N;

    // 去均值 + 填充虚部
    for(uint16_t i = 0; i < FFT_SIZE; i++)
    {
        if(i < N)
        {
            acc_mag[i] -= mean;
            fft_input[2*i] = acc_mag[i] * wind[i];  // 实部
        }
        else
        {
            fft_input[2*i] = 0.0f;   // 补零
        }

        fft_input[2*i+1] = 0.0f;                 // 虚部
    }

    // 执行fft
    dsps_fft2r_fc32(fft_input, FFT_SIZE);

    // 位反转
    dsps_bit_rev_fc32(fft_input, FFT_SIZE);

    // 转换为2维矩阵
    // dsps_cplx2reC_fc32(fft_input, FFT_SIZE);

    // 计算功率谱
    for(uint16_t i = 0; i < FFT_SIZE/2; i++)
    {
        float real = fft_input[2*i];
        float imag = fft_input[2*i + 1];
        fft_power[i] = real*real + imag*imag;
    }

    // 计算总能量
    for(uint16_t i = 0; i < FFT_SIZE/2; i++)
    {
        total_energy += fft_power[i];
    }

    if(total_energy < 1e-6f)
    {
        *energy_fft = 0.0f;
        // *entropy_fft = 0.0f;
        return;
    }  // 判断, 如果总能量较小, 返回为0

 
    // 计算谱熵
    for(uint16_t i = 0; i < FFT_SIZE/2; i++)
    {
        float p = fft_power[i] / total_energy;   // 归一化频谱
        entropy += p * logf(p + 1e-12);        
    }
    entropy = -entropy;

    // 返回标准差, 谱熵
    *acc_std = sqrtf(s_2);
    *energy_fft = total_energy;
}