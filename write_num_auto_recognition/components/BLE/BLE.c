#include <stdio.h>
#include "BLE.h"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"


#define DEVICE_NAME "ESP32S3-NimBLE"
#define TAG "NimBLE"

static bool ble_adv_active = false;
static uint16_t rx_value_handle;
static uint16_t tx_value_handle;
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;


/*定义回调函数*/
static int gatt_event_handler(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if(ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
    {
        if(attr_handle == rx_value_handle)
        {
            // if(ctxt->om->om_data[0] == 0x00)
            // {
            ESP_LOGI(TAG, "接收到数据: %s", (char *)ctxt->om->om_data);
            // }
        };
    }

    return 0;
};


void ble_send_page(int page)
{
    if(conn_handle == BLE_HS_CONN_HANDLE_NONE)
    {
        ESP_LOGW(TAG,"BLE未连接");
        return;
    }
    
    char buf[16];

    sprintf(buf,"PAGE:%d",page);  // 最多发送16-1-5 = 10位数字

    struct os_mbuf *om =
        ble_hs_mbuf_from_flat(
            buf,
            strlen(buf));

    int rc = ble_gatts_notify_custom(
        conn_handle,
        tx_value_handle,
        om);

    ESP_LOGI(TAG,
            "发送页面:%s rc=%d",
            buf,
            rc);
}


static int gap_event_handler(struct ble_gap_event *event, void *arg)
{
    if(event->type == BLE_GAP_EVENT_CONNECT)
    {
        if(event->connect.status == 0)
        {
            conn_handle = event->connect.conn_handle;
            ESP_LOGI(TAG, "连接成功");
            ble_adv_active = false;
        }
        else
        {
            ESP_LOGI(TAG, "连接失败, 重新广播");
            if(!ble_adv_active)
            {
                start_advertising();
            };
        }
    }
    else if(event->type == BLE_GAP_EVENT_DISCONNECT)
    {
        conn_handle = BLE_HS_CONN_HANDLE_NONE;
        ESP_LOGI(TAG, "设备断开连接, 重新广播");
        if(!ble_adv_active)
        {
            start_advertising();
        };
    };
    return 0;
};


/*定义服务和特征*/
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x00FF),
        .characteristics = (struct ble_gatt_chr_def[])
        {
            // ESP32接收手机数据
           {
             .uuid = BLE_UUID16_DECLARE(0xFF01),
             .access_cb = gatt_event_handler,
             .flags = BLE_GATT_CHR_F_WRITE,
             .val_handle = &rx_value_handle,
             .arg = NULL,

           },
           // ESP32发送手机数据
           {
             .uuid = BLE_UUID16_DECLARE(0xFF02),
             .access_cb = gatt_event_handler,
             .flags = BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_READ,
             .val_handle = &tx_value_handle,
             .arg = NULL,
           },
           { 0 },  
        }
    },
    { 0 }
};



/*开启广播*/
void start_advertising(void)
{
    struct ble_hs_adv_fields adv_fields = {0};
    adv_fields.name =(uint8_t *)DEVICE_NAME;
    adv_fields.name_len = strlen(DEVICE_NAME);
    adv_fields.name_is_complete = 1;
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv_fields.tx_pwr_lvl_is_present = 1;
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    int rc = ble_gap_adv_set_fields(&adv_fields); // 配置广播数据
    if(rc != 0)
    {
        ESP_LOGE(TAG, "广播内容设置失败");
        return;
    }

    struct ble_gap_adv_params adv_para= {0};
    adv_para.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_para.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_para.itvl_min = BLE_GAP_ADV_ITVL_MS(200);
    adv_para.itvl_max = BLE_GAP_ADV_ITVL_MS(500);
    
    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_para, gap_event_handler, NULL);   // 开始广播
    if(rc != 0)
    {
        ESP_LOGE(TAG, "广播启动失败");
        return;
    }
    else
    {
        ESP_LOGI(TAG, "广播启动成功, 等待连接...");
        ble_adv_active = true;
    }
    
};


/*回调函数*/
static void on_sync(void)
{
    ESP_LOGI(TAG, "设备初始化完毕");
    start_advertising();

};


void host_task(void * arg)
{
    nimble_port_run();
}


/*初始化nimble*/
void ble_Init(void)
{
    // // 1.初始化NVS
    // nvs_flash_init();

    // 2.初始化NimBLE蓝牙协议栈
    nimble_port_init();

    // 3.初始化GAP服务
    ble_svc_gap_init();

    // 4.初始化GATT服务
    ble_svc_gatt_init();

    // 5.设置设备名称
    ble_svc_gap_device_name_set(DEVICE_NAME);
    
    // 6.注册服务和特征列表
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    // 7.设置同步回调函数
    ble_hs_cfg.sync_cb = on_sync;

    // 8.启动监听任务
    nimble_port_freertos_init(host_task);
}





