#ifndef __FFT_PROCESS_H__
#define __FFT_PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

void fft_Init(void);
void fft_fun(const float* data, float* acc_std, float* energy_fft);

#ifdef __cplusplus
}
#endif

#endif