#include "wifi_task.h"
#include "nvs_params/nvs_params.h"
#include "cmd_handler.hpp"

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
#define EXAMPLE_ESP_MAXIMUM_RETRY 5


// #define WIFI_SSID "W1501"
// #define WIFI_PWD "w15011234"

static unsigned char s_wifi_ssid[32] = "";
static unsigned char s_wifi_pwd[64] = "";
static bool s_wifi_connect_fail = false;

esp_mqtt_client_handle_t s_mqtt_handler = NULL;
static char s_mqtt_broker[64] = "";
static char s_mqtt_user[32] = "";
static char s_mqtt_pwd[64] = "";
static char s_mqtt_sn[16] = "";


#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_STA_START_BIT BIT2

static void log_error_if_nonzero(const char *message, int error_code) {
    if (error_code != 0) {
        ESP_LOGE(__func__, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqttEventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(__func__, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    char topic[128] = {};
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(__func__, "MQTT_EVENT_CONNECTED");
        s_m_state.mqtt_connected = true;
        sprintf(topic,"/esp/act/%s",s_mqtt_sn);
        esp_mqtt_client_subscribe(client,topic,1);
        sprintf(topic,"/esp/on/%s",s_mqtt_sn);
        esp_mqtt_client_publish(client,topic,"on",2,1,1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(__func__, "MQTT_EVENT_DISCONNECTED");
        s_m_state.mqtt_connected = false;
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(__func__, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(__func__, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(__func__, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(__func__, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        sprintf(topic,"/esp/act/%s",s_mqtt_sn);
        if (strcmp(topic,event->topic) == 0) {
            ESP_LOGI(__func__,"exec cmd");
            CmdRet ret = runCmd((uint8_t *)event->data,event->data_len);
            if (ret.value) {
                sprintf(topic,"/esp/ack/%s",s_mqtt_sn);
                esp_mqtt_client_publish(client,topic,(const char *)ret.value,ret.value_len,1,false);
                free(ret.value);
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(__func__, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(__func__, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(__func__, "Other event id:%d", event->event_id);
        break;
    }
}

void mqttInit() {
    char lwt_topic[128] = {};
    sprintf(lwt_topic,"/esp/on/%s",s_mqtt_sn);
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = s_mqtt_broker,
        .credentials = {
            .username = s_mqtt_user,
            .authentication = {
                .password = s_mqtt_pwd 
            }
        },
        .session = {
            .last_will = {
                .topic = lwt_topic,
                .msg = "off",
                .qos = 1,
                .retain = 1,
                .msg_len = 3,
            },
            .keepalive = 60
        },
    };
    s_mqtt_handler = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(s_mqtt_handler, ESP_EVENT_ANY_ID, mqttEventHandler, NULL);
    esp_mqtt_client_start(s_mqtt_handler);
}


void wifiEventHandler(
    void* arg, 
    esp_event_base_t event_base,
    int32_t event_id, 
    void* event_data) {

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(__func__, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(__func__,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(__func__, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }

}

void wifiInit() {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifiEventHandler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifiEventHandler,
                                                        NULL,
                                                        &instance_got_ip));                                              

    wifi_config_t wifi_config = {
        .sta = {
	     .threshold.authmode = WIFI_AUTH_OPEN,
        },
    };
    
    memcpy(wifi_config.sta.ssid,s_wifi_ssid,32);
    memcpy(wifi_config.sta.password,s_wifi_pwd,64);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(__func__, "wifi_init_sta finished.");
}

void wifiThingsLoop() {

    esp_err_t err;
    wifi_scan_config_t config = {
        .ssid = (unsigned char *)s_wifi_ssid 
    };

    static bool isConnected = false;
    if (!isConnected) {
        esp_wifi_scan_start(&config,true);
        uint16_t num = 0;
        esp_wifi_scan_get_ap_num(&num);
        ESP_LOGI(__func__,"get scan wifi number %d",num);
        if (num > 0 && s_wifi_connect_fail == false ) { esp_wifi_connect(); }
    }
    
    s_wifi_connect_fail = false;
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            60000/portTICK_PERIOD_MS);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(__func__, "connected to ap SSID:%s password:%s",
                 s_wifi_ssid, s_wifi_pwd);
        isConnected = true;
        s_m_state.wifi_connected = true;
        mqttInit();
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(__func__, "Failed to connect to SSID:%s, password:%s",
                 s_wifi_ssid, s_wifi_pwd);
        s_wifi_connect_fail = true;
        isConnected = false;
        s_m_state.wifi_connected = false;
    } else if (bits == 0) {
        ESP_LOGI(__func__,"EVENT GROUP WAIT TIMEOUT");
    } else {
        ESP_LOGE(__func__, "UNEXPECTED EVENT");
    }
    xEventGroupClearBits(s_wifi_event_group,bits);
}

void mqttTask(void *arg) {
    
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    struct timeval tv_last = tv_now;
    char topic[128] = {};
    char payload[128] = {};
    sprintf(topic,"/esp/time/%s",s_mqtt_sn);
    while (true) {
        vdelay(100);
        gettimeofday(&tv_now,NULL);
        int64_t now = (int64_t)tv_now.tv_sec * 1000 + tv_now.tv_usec / 1000;
        int64_t last = (int64_t)tv_last.tv_sec * 1000 + tv_last.tv_usec / 1000;
        // ESP_LOGI(__func__,"now %lld last %lld",now,last);
        if (now - last < 1000) { continue; }
        if (!s_m_state.mqtt_connected) { continue; }
        tv_last = tv_now;
        int l = sprintf(payload,"%lld",now);
        payload[l] = 0;
        esp_mqtt_client_publish(s_mqtt_handler,topic,payload,l,1,0);
    }
    
}

void wifiTask(void *arg) {

    nvsGetBlob(KEY_WIFI_SSID_NAME,(void *)s_wifi_ssid,32);
    nvsGetBlob(KEY_WIFI_PWD_NAME,(void *)s_wifi_pwd,64);
    nvsGetBlob(KEY_MQTT_BROKER_URI_NAME,(void *)s_mqtt_broker,64);
    nvsGetBlob(KEY_MQTT_USERNAME_NAME,(void *)s_mqtt_user,32);
    nvsGetBlob(KEY_MQTT_PASSWORD_NAME,(void *)s_mqtt_pwd,64);
    nvsGetBlob(KEY_MQTT_PASSWORD_NAME,(void *)s_mqtt_pwd,16);
    nvsGetBlob(KEY_DEVICE_SN_NAME,s_mqtt_sn,16);

    uint8_t mac[8] = {};
    esp_read_mac(mac,ESP_MAC_WIFI_STA);
    sprintf(s_mqtt_sn,"%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    

    
    // uint8_t mac[8] = {};
    // esp_read_mac(mac,ESP_MAC_WIFI_STA);
    // esp_log_buffer_hex(__func__,mac,8);
    // uint8_t mac[8] = {};
    // esp_read_mac(mac,ESP_MAC_WIFI_STA);
    // esp_log_buffer_hex(__func__,mac,8);

    wifiInit();
    xTaskCreate(mqttTask,"mqtt task",4096,NULL,10,NULL);
    while (true) {
        wifiThingsLoop();
    }
}
