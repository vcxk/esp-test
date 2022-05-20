

#include <stdio.h>
#include "stdlib.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_spi_flash.h"
#include "esp_log.h"

void hello_task(void *arg) {

    ESP_LOGI(__func__,"hello world %d",10);

    for (int i = 0; i < 5; i++) {
        vTaskDelay(1000/portTICK_PERIOD_MS);
        ESP_LOGI(__func__,"hello world restart in %d seconds", 5 - i - 1);
    }

    esp_restart();
}



