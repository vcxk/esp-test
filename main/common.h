#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "esp_mac.h"

#define GPIO_OUTPUT_LED_0    2
#define GPIO_INPUT_IO_0     0

#define vdelay(ms) vTaskDelay(ms/portTICK_PERIOD_MS)

typedef struct {
    bool blue;
    bool wifi_connected;
    bool mqtt_connected;
} M_State;

extern M_State s_m_state;

#endif