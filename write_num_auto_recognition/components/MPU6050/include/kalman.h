#ifndef __KALMAN_H__
#define __KALMAN_H__

#ifdef __cplusplus
extern "C" {
#endif

void kalman_update(float ax, float ay, float az, float gx, float gy, float gz, float *re_roll, float *re_pitch, float *re_yaw);
void Init_kalman(void);

#ifdef __cplusplus
}
#endif

#endif