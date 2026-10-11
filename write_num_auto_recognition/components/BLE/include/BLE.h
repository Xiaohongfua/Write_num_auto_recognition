#ifndef __BLE_H__
#define __BLE_H__

#ifdef __cplusplus
extern "C" {
#endif

void start_advertising(void);
void ble_Init(void);
void ble_send_page(int page);

#ifdef __cplusplus
}
#endif

#endif 
