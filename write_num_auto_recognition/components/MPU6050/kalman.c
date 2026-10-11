#include "kalman.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"
#include "math.h"


#define PI 3.141592653589793f
TickType_t pre_time;                          // 初始tick
TickType_t cur_time;                          // 现在tick
float acc_roll, acc_pitch;                    // 加速度计解算出的roll和pitch
float roll_v, pitch_v, yaw_v;                 // 陀螺仪映射到x,y轴上的角速度, yaw仅作为测试
float gyro_roll, gyro_pitch, gyro_yaw;        // 陀螺仪解算出的roll和pitch, yaw仅作为测试
float k_roll, k_pitch;                        // 卡尔曼滤波后得出的最优roll和pitch
static float e_P[2][2];                              // 先验协方差矩阵/更新后的协方差矩阵
static float k_k[2][2];                              // 卡尔曼增益



/*卡尔曼滤波,解算Roll和Pitch*/
void kalman_update(float ax, float ay, float az, float gx, float gy, float gz, float *re_roll, float *re_pitch, float *re_yaw)
{
    cur_time = xTaskGetTickCount(); // 获取当前tick
    float delta_t = (float)(cur_time - pre_time) / 1000;  // 换算Δt,单位为s
       
    // 1.计算先验估计X^{hat-}
    float Gx = gx / 180 * PI; 
    float Gy = gy / 180 * PI;
    float Gz = gz / 180 * PI;   // 切换陀螺仪输出值为弧度制
    roll_v = Gx + (sin(k_pitch)*sin(k_roll)/cos(k_pitch))*Gy + (cos(k_roll)*sin(k_pitch)/cos(k_pitch))*Gz;  // 计算x轴上roll角速度
    pitch_v = 0 + cos(k_roll)*Gy - sin(k_roll)*Gz;  // 计算pitch角速度
    yaw_v = 0 + (sin(k_roll)/cos(k_pitch))*Gy + (cos(k_roll)/cos(k_pitch))*Gz;  // 测试yaw

    gyro_roll = k_roll + delta_t * roll_v;      // 先验roll ->> 陀螺仪解算
    gyro_pitch = k_pitch + delta_t * pitch_v;   // 先验pitch ->> 陀螺仪解算
    gyro_yaw = gyro_yaw + delta_t * yaw_v;  // 测试

    // 2.计算先验协方差矩阵
    e_P[0][0] = e_P[0][0]  + 0.0025;
    e_P[0][1] = e_P[0][1] + 0;
    e_P[1][0] = e_P[1][0] + 0;
    e_P[1][1] = e_P[1][1] + 0.0025;

    // 3.计算卡尔曼增益k_k
    k_k[0][0] = e_P[0][0] / (e_P[0][0] + 0.3);
    k_k[0][1] = 0;
    k_k[1][0] = 0;
    k_k[1][1] = e_P[1][1] / (e_P[1][1] + 0.3);

    // 4.数据融合，获得卡尔曼滤波后最优roll和pitch
    acc_roll = atan2(ay, az);                          // 加速度计解算出的roll
    acc_pitch = -1 * atan2(ax, sqrt(ay*ay + az*az));   // 加速度计解算出的pitch

    k_roll = gyro_roll + k_k[0][0]*(acc_roll - gyro_roll);        // 卡尔曼滤波得到最优roll
    k_pitch = gyro_pitch + k_k[1][1]*(acc_pitch - gyro_pitch);    // 卡尔曼滤波得到最优pitch

    // 5.更新协方差e_P
    e_P[0][0] = (1 - k_k[0][0])*e_P[0][0];
    e_P[0][1] = 0;
    e_P[1][0] = 0;
    e_P[1][1] = (1 - k_k[1][1])*e_P[1][1];

    // 返回滤波后结果
    *re_roll = k_roll;
    *re_pitch = k_pitch;
    *re_yaw = gyro_yaw;  // 测试

    pre_time = cur_time;

}

/*初始化kalman滤波参数*/
void Init_kalman(void)
{
    // 设定协方差矩阵初值
    e_P[0][0] = 1;
    e_P[0][1] = 0;
    e_P[1][0] = 0;
    e_P[1][1] = 1;

    // 设定卡尔曼增益k_k初值
    k_k[0][0] = 0;
    k_k[0][1] = 0;
    k_k[1][0] = 0;
    k_k[1][1] = 0;

    // 设定yaw初值,测试用
    gyro_yaw = 0;
    
    pre_time = xTaskGetTickCount();
}


