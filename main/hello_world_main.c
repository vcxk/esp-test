/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "common.h"
#include "cmd_handler.hpp"

#include "hello/hello.h"
#include "bluetooth/blue_server.h"
#include "wifi/wifi_task.h"

M_State s_m_state = {0};

bool checkButtonPressed() {

    gpio_set_direction(GPIO_INPUT_IO_0,GPIO_MODE_INPUT);

    vdelay(200);

    int l = 1;

    l = gpio_get_level(GPIO_INPUT_IO_0);
    if (l > 0) { return false; }
    vdelay(1000);
    l = gpio_get_level(GPIO_INPUT_IO_0);
    if (l > 0) { return false; }

    return true;
} 


void blinkTask(void *arg) {
    gpio_set_direction(GPIO_OUTPUT_LED_0,GPIO_MODE_OUTPUT);

    while (true) {

        int loop = 1;
        if (s_m_state.blue) { loop = 2; }
        if (s_m_state.wifi_connected) { loop = 3; }
        if (s_m_state.wifi_connected && s_m_state.mqtt_connected) { loop = 4; }

        for (int i = 0; i < loop; i++ ) {
            gpio_set_level(GPIO_OUTPUT_LED_0,1);
            vdelay(200);
            gpio_set_level(GPIO_OUTPUT_LED_0,0);
            vdelay(200);
        }

        vdelay(1000);
    }
    
}

void app_main(void) {
    esp_err_t ret;
    /* Initialize NVS. */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK( ret );

    // uint8_t cmd1[] = "wifi_ssid:2=cxk_wifi\n";
    // uint8_t cmd2[] = "wifi_ssid:3\n";


    // CmdRet cmd_ret = runCmd(cmd1,strlen((const char *)cmd1));
    // if (cmd_ret.value) {
    //     ESP_LOGI("cmd test 1","%s",(char *)cmd_ret.value);
    //     free(cmd_ret.value);
    // }
    // cmd_ret = runCmd(cmd2,strlen((const char *)cmd2));
    // if (cmd_ret.value) {
    //     ESP_LOGI("cmd test 2","%s",cmd_ret.value);
    //     free(cmd_ret.value);
    // }

    xTaskCreate(blinkTask,"blink",2048,NULL,10,NULL);

    if (checkButtonPressed()) {
        ESP_LOGI(__func__,"start blue task");
        blue_server_start();    
    } else {
        ESP_LOGI(__func__,"start wifi task");
        xTaskCreate(wifiTask,"wifi_task",20480,NULL,10,NULL);
    }
    

}
