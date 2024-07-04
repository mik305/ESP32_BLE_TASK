#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> 
#include <time.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "sdkconfig.h"
#include "lwip/def.h"

char *TAG = "ESP32_BLE";
uint8_t ble_addr_type;
uint16_t int_data;
uint16_t control_notif_handle;
void ble_app_advertise(void);

#define ARRAY_SIZE 2000
uint16_t data_array[ARRAY_SIZE];

// Initialize the array with some values 
void initialize_array() {
    for (int i = 0; i < ARRAY_SIZE; i++) {
        data_array[i] = i;
    }
}

// Read data from ESP32 defined as server and convert hex value
static int device_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char decimal_str[10];
    snprintf(decimal_str, sizeof(decimal_str), "%u", data_array[int_data]);

    os_mbuf_append(ctxt->om, decimal_str, strlen(decimal_str));
    return 0;
}

// Handle value of notification
// Send value to user in hex
void notify_handler(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, uint16_t data)
{
    struct os_mbuf *om;
    om = ble_hs_mbuf_from_flat(&data, sizeof(data));
    ble_gattc_notify_custom((uint16_t)conn_handle, control_notif_handle, om);
}

// Notify user device from server 
static int device_notify(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t data = *((uint16_t *)arg);
    notify_handler(conn_handle, attr_handle, ctxt, data);
    return 0;
}

// Write data to ESP32 defined as a server
static int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    printf("Data from the client: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);

    uint16_t received_data;
    char data[ctxt->om->om_len + 1];  
    memcpy(data, ctxt->om->om_data, ctxt->om->om_len);  
    data[ctxt->om->om_len] = '\0';  

    received_data = (uint16_t)atoi(data);  
    
    if (received_data < ARRAY_SIZE) {
        int_data = received_data;
        printf("Table value: %u\n", data_array[int_data]);
        device_read(conn_handle, attr_handle, ctxt, &int_data);
        notify_handler(conn_handle, attr_handle, ctxt, data_array[int_data]);
    } else {
        printf("Data outside table range: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);
    }

    return 0;
}

// Array of pointers to other service definitions
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(0x180),                 	// Define UUID for device type
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID16_DECLARE(0x0002),           // Define UUID for reading
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = device_read},
         {.uuid = BLE_UUID16_DECLARE(0x0008),           // Define UUID for writing
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = device_write},
         {.uuid = BLE_UUID16_DECLARE(0x0010),           // Define UUID for notifying
          .flags = BLE_GATT_CHR_F_NOTIFY,
          .val_handle = &control_notif_handle,
          .access_cb = device_notify},
         {0}}},
    {0}};

// BLE event handling
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    // Advertise if connected
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
    // Advertise again after completion of the event
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT DISCONNECTED");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE GAP EVENT");
        ble_app_advertise();
        break;
    default:
        break;
    }
    return 0;
}

// Define the BLE connection
void ble_app_advertise(void)
{
    // GAP - device name definition
    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = ble_svc_gap_device_name(); 
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    // GAP - device connectivity definition
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // connectable or non-connectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // discoverable or non-discoverable
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

// The application
void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type); // Determines the best address type automatically
    ble_app_advertise();                     // Define the BLE connection
}

// The infinite task
void host_task(void *param)
{
    nimble_port_run(); // This function will return only when nimble_port_stop() is executed
}

// Timer handle, as a software interrupt
TimerHandle_t xTimer;

// Timer callback function
void vTimerCallback(TimerHandle_t xTimer) {
    uint16_t random_value = rand() % 65534; 
    uint16_t random_index = rand() % 1999; 
    data_array[random_index] = random_value;
    printf("data_array[%d] = %u\n", random_index, random_value);
}

void app_main()
{
    nvs_flash_init();                          	  	// NVS flash using
    nimble_port_init();                           	// host stack
    ble_svc_gap_device_name_set("ESP32_BLE"); 	// NimBLE configuration - server name
    ble_svc_gap_init();                          	// configuration - gap service
    ble_svc_gatt_init();                          	// NimBLE configuration - gatt service
    ble_gatts_count_cfg(gatt_svcs);            // NimBLE configuration - config gatt services
    ble_gatts_add_svcs(gatt_svcs);             // NimBLE configuration - queues gatt services.
    ble_hs_cfg.sync_cb = ble_app_on_sync;      		// Initialize application
    nimble_port_freertos_init(host_task);      // Run the thread
    initialize_array();	

    srand(time(NULL));

    // Timer for 1 second
    xTimer = xTimerCreate("Timer", pdMS_TO_TICKS(1000), pdTRUE, (void *)0, vTimerCallback);

    if (xTimer != NULL) {
        if (xTimerStart(xTimer, 0) != pdPASS) {
            ESP_LOGE(TAG, "Failed to start timer");
        }
    } else {
        ESP_LOGE(TAG, "Failed to create timer");
    }
}
