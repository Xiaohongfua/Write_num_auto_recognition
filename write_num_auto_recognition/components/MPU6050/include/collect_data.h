#ifndef __COLLECT_DATA_H__
#define __COLLECT_DATA_H__

#ifdef __cplusplus
extern "C" {
#endif

void key_Init(void);
void norm(float *data, uint16_t row, uint16_t col);
void get_data(void);
void cal_energy(float* E);
void fill_mpu_data(float* MPU6050_data, int i);


#ifdef __cplusplus
}
#endif

#endif