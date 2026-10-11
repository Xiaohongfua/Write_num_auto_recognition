idf_component_register(SRCS "main.cpp"
                     "main_fun.cpp"
                     "model.c"
                     "process.cpp"
                    REQUIRES MPU6050
                    REQUIRES BLE
                    REQUIRES nvs_flash
                    INCLUDE_DIRS ".")