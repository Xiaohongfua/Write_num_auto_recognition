#include "MPU6050.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static float ax, ay, az; // 加速度计，单位为g
static float temp; // 温度计，单位为℃
static float gx, gy, gz; // 角速度计，单位为°
static float gx_bias = -2.1530f;
static float gy_bias = -4.5386f;
static float gz_bias = 0.24975f;



void iic_Init(void)
{
    i2c_config_t i2c_cfg = {
        .clk_flags = 0,
        .master.clk_speed = 100000,
        .mode = I2C_MODE_MASTER,
        .scl_io_num = GPIO_NUM_4,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .sda_io_num = GPIO_NUM_5,
        .sda_pullup_en = GPIO_PULLUP_ENABLE
    };

    i2c_param_config(I2C_NUM_0, &i2c_cfg);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);

}

void iic_send_data(uint8_t cmd_data[], uint8_t data_size, uint8_t reg)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (mpu6050_addr << 1) | I2C_MASTER_WRITE, true);  
    i2c_master_write_byte(cmd, reg, true);   

    for (uint8_t i = 0; i < data_size; i++)
    {
        i2c_master_write_byte(cmd, cmd_data[i], true);
    }
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000);
    i2c_cmd_link_delete(cmd);
}


void iic_read_data(uint8_t *recv_data, uint8_t data_size, uint8_t reg)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (mpu6050_addr << 1) | I2C_MASTER_WRITE, true);  // 先写入地址
    i2c_master_write_byte(cmd, reg, true);    

    i2c_master_start(cmd);   
    i2c_master_write_byte(cmd, (mpu6050_addr << 1) | I2C_MASTER_READ, true);  // 切换为读地址

    // for (uint8_t i = 0; i < data_size - 1; i++)
    // {
    //     i2c_master_read_byte(cmd, &recv_data[i], I2C_MASTER_ACK);
    // }
    if(data_size > 1)
    {
        i2c_master_read(cmd, recv_data, data_size - 1, I2C_MASTER_ACK);  // 原子操作
    }
    i2c_master_read_byte(cmd, &recv_data[data_size - 1], I2C_MASTER_NACK);  
    i2c_master_stop(cmd);

    esp_err_t res = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(20));

    if(res != ESP_OK)
    {
        printf("I2C READ ERROR = %d\n", res);
    }
    i2c_cmd_link_delete(cmd);

}

void mpu6050_Init(void)
{
    // 1.初始化I2C
    iic_Init();
    printf("start i2c_init...\n");
    // 2.设置mpu6050参数
    uint8_t send_data[] = {0x80};
    iic_send_data(send_data, 1, 0x6b);  // 复位
    vTaskDelay(pdMS_TO_TICKS(100));
    
    send_data[0] = 0x00;
    iic_send_data(send_data, 1, 0x6b);  // 关闭睡眠模式

    send_data[0] = 0x00;
    iic_send_data(send_data, 1, 0x23);  // 显式关闭所有 FIFO 缓存，防止自动采集时溢出挂死
    
    send_data[0] = 0x07;
    iic_send_data(send_data, 1, 0x6A);  // 重置 MPU6050 的所有数字信号通路和传感器寄存器
    
    send_data[0] = 4; 
    iic_send_data(send_data, 1, 0x19);    // 设置采样率 200Hz, SampleRate = 1000 / (1 + 4) = 200Hz

    send_data[0] = 0x00;
    iic_send_data(send_data, 1, 0x1A);   // 低通滤波

    send_data[0] = 0x18;
    iic_send_data(send_data, 1, 0x1b);  // 陀螺仪量程，设置为+-2000°

    send_data[0] = 0x00;
    iic_send_data(send_data, 1, 0x1c);  // 加速度计量程，设置为+-2g

    printf("mpu6050_Init is successful.\n");

}


void mpu6050_Update(void)
{
    uint8_t recv_data[14] = {0};

    // 一次读完所有数据
    iic_read_data(recv_data, 14, 0x3B);

    int16_t ax_raw = (recv_data[0] << 8) | recv_data[1];
    int16_t ay_raw = (recv_data[2] << 8) | recv_data[3];
    int16_t az_raw = (recv_data[4] << 8) | recv_data[5];

    int16_t temp_raw = (recv_data[6] << 8) | recv_data[7];

    int16_t gx_raw = (recv_data[8] << 8) | recv_data[9];
    int16_t gy_raw = (recv_data[10] << 8) | recv_data[11];
    int16_t gz_raw = (recv_data[12] << 8) | recv_data[13];

    ax = ax_raw * 6.1035e-5f;  // ax_raw / 32768 * 2g
    ay = ay_raw * 6.1035e-5f;
    az = az_raw * 6.1035e-5f;   


    temp = temp_raw / 340.0f + 36.53f;


    gx = gx_raw * 6.1035e-2f; // gx_raw / 32768 * 2000
    gy = gy_raw * 6.1035e-2f; 
    gz = gz_raw * 6.1035e-2f; 

}



void mpu6050_Geta_(float *x, float *y, float *z)
{
    *x = ax;
    *y = ay;
    *z = az;
}
void mpu6050_Gett_(float *t)
{
    *t = temp;
}
void mpu6050_Getg_(float *x, float *y, float *z)
{
    *x = gx - gx_bias;
    *y = gy - gy_bias;
    *z = gz - gz_bias;
}