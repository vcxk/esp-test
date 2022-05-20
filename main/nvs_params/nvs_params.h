
#ifndef NVS_PARAMS_H
#define NVS_PARAMS_H

#include "common.h"
#include "nvs_flash.h"

#define KEY_WIFI_SSID_NAME "wifi_ssid"
#define KEY_WIFI_PWD_NAME "wifi_pwd"

#define KEY_MQTT_BROKER_URI_NAME "mqtt_uri"
#define KEY_MQTT_USERNAME_NAME "mqtt_username"
#define KEY_MQTT_PASSWORD_NAME "mqtt_password"

#define KEY_DEVICE_SN_NAME "device_sn"

nvs_handle nvsGetStorageHandle();

esp_err_t nvsSetBlob(const char *key,void *buffer,size_t len);
size_t nvsGetBlob(const char *key,void *buffer,size_t len);


#endif
