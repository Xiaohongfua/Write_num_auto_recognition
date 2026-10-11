idf_component_register(SRCS "fft_process.c" "MPU6050.c" 
                    "collect_data.c"
                    "kalman.c"
                    "mean_std.txt"
                    INCLUDE_DIRS "include"
                    REQUIRES driver
                    REQUIRES esp-dsp)
