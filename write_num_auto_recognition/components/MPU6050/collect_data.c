#include "MPU6050.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"
#include "kalman.h"

static uint8_t mpu_6050_is_work = 0;
#define DOUBLE_CLICK_TIME 300  // 双击间隔时间

float ax = 0, ay = 0, az = 0;   // 加速度
float gx = 0, gy = 0, gz = 0;   // 陀螺仪角速度
float kal_roll = 0, kal_pitch = 0, yaw = 0;  // 卡尔曼滤波后的最优roll, pitch


typedef struct 
{
    float mean;
    float std;
} Norm_para;


/*标准化参数*/
static Norm_para norm_para[8] = {
    {0.000195f,0.339609f},
    {-0.111546f,0.220902f},
    {0.871474f,0.330190f},
    {-12.164582f,68.241013f},
    {0.962578f,20.021196f},
    {0.691133f,47.621326f},
    {-0.106031f,0.301190f},
    {0.028629f,0.220882f}

};





/*标准化数据*/
void norm(float *data, uint16_t row, uint16_t col)
{   
    for(uint16_t i = 0; i < row; i++)
    {
        for(uint16_t j = 0; j < col; j++)
        {
            data[i*col + j] = (data[i*col +j] - norm_para[j].mean) / norm_para[j].std;
        }
    }

}


/*初始化KEY*/
void key_Init(void)
{
    gpio_config_t cofg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << 6,
        .pull_down_en = 0,
        .pull_up_en = 1
    };
    gpio_config(&cofg);
}

/*检查KEY状态*/
void key_state(void)
{
    static uint8_t click_count = 0;
    static uint8_t pre = 1, cur = 1;
    static TickType_t first_click_tick;
    pre = cur;
    cur = gpio_get_level(GPIO_NUM_6);
  
    if(cur != pre)
    {
        if (pre == 1 && cur == 0)
        {
            mpu_6050_is_work = 1;
        }
    }

}



/*填充 MPU6050 数据（i为第几个样本）*/
void fill_mpu_data(float* MPU6050_data, int i)
{
    int base = i * 8;
    MPU6050_data[base + 0] = ax;
    MPU6050_data[base + 1] = ay;
    MPU6050_data[base + 2] = az;
    MPU6050_data[base + 3] = gx;
    MPU6050_data[base + 4] = gy;
    MPU6050_data[base + 5] = gz;
    MPU6050_data[base + 6] = kal_roll;
    MPU6050_data[base + 7] = kal_pitch;

}



/*从mpu6050传感器中获取角速度、角速度, 并采用卡尔曼滤波对最优姿态进行解算*/
void get_data(void)
{
    mpu6050_Update();
    mpu6050_Geta_(&ax, &ay, &az);
    mpu6050_Getg_(&gx, &gy, &gz);
    kalman_update(ax,ay,az,gx,gy,gz,&kal_roll,&kal_pitch,&yaw);
}


/*计算动作能量, 作为动作触发的判断依据*/
void cal_energy(float* E)
{
    float alpha = 0.95f;  // 一阶IIR系数
    float linear_ax, linear_ay, linear_az;  // 滤除掉静止时重力分量后的加速度
    static float gravity_X=0, gravity_Y=0, gravity_Z=0;

    // IIR低通滤波器提取重力
    gravity_X = alpha * gravity_X + (1.0f - alpha) * ax;
    gravity_Y = alpha * gravity_Y + (1.0f - alpha) * ay;
    gravity_Z = alpha * gravity_Z + (1.0f - alpha) * az;

    // 重力分离
    linear_ax = ax - gravity_X;
    linear_ay = ay - gravity_Y;
    linear_az = az - gravity_Z;

    // 动作能量
    float gyro_norm = sqrtf(gx*gx + gy*gy + gz*gz);
    float linear_acc_norm = sqrtf(linear_ax*linear_ax + linear_ay*linear_ay + linear_az*linear_az);

    *E = 0.8f*gyro_norm + 0.2f*linear_acc_norm;

    // printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",ax, ay, az, gx, gy, gz, kal_roll, kal_pitch, yaw, *E);
    // printf("%f\n", *E);    
}

