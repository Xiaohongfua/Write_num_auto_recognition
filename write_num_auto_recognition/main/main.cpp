#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "main_fun.h"
#include "MPU6050.h"
#include "collect_data.h"
#include "process.h"
#include "MPU6050.h"
#include "BLE.h"
#include "nvs_flash.h"


extern "C" void app_main(void)
{   
    // 初始化MPU6050模块
    mpu6050_Init();
    setup();
    // 初始化BLE模块
    nvs_flash_init();
    ble_Init();

    process_logic();
    
}