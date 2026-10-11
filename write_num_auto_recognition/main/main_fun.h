#ifndef __MAIN_FUN_H__
#define __MAIN_FUN_H__

#ifdef __cplusplus
extern "C" {
#endif

void setup(void);
void loop(float* MPU6050_data, int* result);

#ifdef __cplusplus
}
#endif

#endif