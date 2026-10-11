#ifndef _MPU6050_H
#define _MPU6050_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>
void mpu6050_Init(void);
void mpu6050_Update(void);
void mpu6050_Geta_(float *x, float *y, float *z);
void mpu6050_Gett_(float *t);
void mpu6050_Getg_(float *x, float *y, float *z);
void iic_read_data(uint8_t *recv_data, uint8_t data_size, uint8_t reg);

#define mpu6050_addr 0x68

#ifdef __cplusplus
}
#endif

#endif