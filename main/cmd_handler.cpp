#include "cmd_handler.hpp"
#include <string>
#include "string.h"

using namespace std;

#define MSG_MAX_LEN 256

void cmdHello() {

}

void doSetCmd(CmdRet *ret,string cmd,long flag,string value) {
    bool isOk = false;
    char msg_buf[MSG_MAX_LEN] = {};
    long msg_len = 0;
    if (cmd == KEY_WIFI_SSID_NAME ) {
        isOk = nvsSetBlob(KEY_WIFI_SSID_NAME,(void *)value.c_str(),(size_t)value.length()) == ESP_OK;
        msg_len = sprintf(msg_buf,"%ld:%s",flag,isOk ? "ok" : "fail");
    } else if (cmd == KEY_WIFI_PWD_NAME) {
        isOk = nvsSetBlob(KEY_WIFI_PWD_NAME,(void *)value.c_str(),(size_t)value.length()) == ESP_OK;
        msg_len = sprintf(msg_buf,"%ld:%s",flag,isOk ? "ok" : "fail");
    } else if (cmd == KEY_MQTT_BROKER_URI_NAME) {
        isOk = nvsSetBlob(KEY_MQTT_BROKER_URI_NAME,(void *)value.c_str(),value.length()) == ESP_OK;
        msg_len = sprintf(msg_buf,"%ld:%s",flag,isOk ? "ok" : "fail");
    } else if (cmd == KEY_MQTT_USERNAME_NAME) {
        isOk = nvsSetBlob(KEY_MQTT_USERNAME_NAME,(void *)value.c_str(),value.length()) == ESP_OK;
        msg_len = sprintf(msg_buf,"%ld:%s",flag,isOk ? "ok" : "fail");
    } else if (cmd == KEY_MQTT_PASSWORD_NAME) {
        isOk = nvsSetBlob(KEY_MQTT_PASSWORD_NAME,(void *)value.c_str(),value.length()) == ESP_OK;
        msg_len = sprintf(msg_buf,"%ld:%s",flag,isOk ? "ok" : "fail");
    } else if (cmd == KEY_DEVICE_SN_NAME) {
        isOk = false;
        msg_len = sprintf(msg_buf,"%ld:%s",flag,isOk ? "ok" : "fail");
    }
    ret->state = isOk;
    if (msg_len == 0) { return; }
    ret->value = (uint8_t *)malloc(msg_len + 2);
    ret->value_len = msg_len + 2;
    memcpy(ret->value,msg_buf,msg_len);
    ret->value[msg_len] = '\n';
    ret->value[msg_len + 1] = '\0';
}

void doReadCmd(CmdRet *ret,string cmd,long flag) {
    char read_buf[MSG_MAX_LEN] = {};
    int offset = sprintf(read_buf,"%ld:",flag);
    int readlen = 0;
    if (cmd == KEY_WIFI_SSID_NAME ) {
        readlen = nvsGetBlob(KEY_WIFI_SSID_NAME,read_buf + offset,MSG_MAX_LEN - offset);
    } else if (cmd == KEY_WIFI_PWD_NAME) {
        readlen = nvsGetBlob(KEY_WIFI_PWD_NAME,read_buf + offset,MSG_MAX_LEN - offset);
    } else if (cmd == KEY_MQTT_BROKER_URI_NAME) {
        readlen = nvsGetBlob(KEY_MQTT_BROKER_URI_NAME,read_buf + offset,MSG_MAX_LEN - offset);
    } else if (cmd == KEY_MQTT_USERNAME_NAME) {
        readlen = nvsGetBlob(KEY_MQTT_USERNAME_NAME,read_buf + offset,MSG_MAX_LEN - offset);
    } else if (cmd == KEY_MQTT_PASSWORD_NAME) {
        readlen = nvsGetBlob(KEY_MQTT_PASSWORD_NAME,read_buf + offset,MSG_MAX_LEN - offset);
    } else if (cmd == KEY_DEVICE_SN_NAME) {
        uint8_t mac[8] = {};
        esp_read_mac(mac,ESP_MAC_WIFI_STA);
        readlen = sprintf(read_buf+offset,"%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
        // readlen = nvsGetBlob(KEY_DEVICE_SN_NAME,read_buf + offset,MSG_MAX_LEN - offset);
    }
    ret->state = readlen > 0;
    ret->value = (uint8_t *)malloc(readlen + offset + 2);
    ret->value[readlen + offset] = '\n';
    ret->value[readlen + offset + 1] = '\0';
    memcpy(ret->value,read_buf,offset + readlen);
    ret->value_len = readlen + offset + 2;
}

void doMqttSetCmd(CmdRet *ret,string cmd,long flag,string value) {
    char buf[256] = {};
    int len = 0;
    if (cmd == "time") {
        long long time = strtoll(value.c_str(),NULL,10);
        if (time != 0) {
            struct timeval tv = {};
            tv.tv_sec = time / 1000;
            tv.tv_usec = (time %1000) * 1000;
            settimeofday(&tv,NULL);
            ret->state = 1;
            len = sprintf(buf,"%ld:ok\n",flag);
        } else {
            len = sprintf(buf,"%ld,fail\n",flag);
        }
        ret->value = (uint8_t *)malloc(len + 2);
        memcpy(ret->value,buf,len);
        ret->value_len = len;
        return;
    }
}

void doMqttReadCmd(CmdRet *ret,string cmd,long flag) {

}

bool parseCmd(char *buf,size_t len,std::string &cmd,long *flag,std::string &value,bool *isSet) {
    if (len <= 0 || buf == NULL) { return false; }
    if (buf[len - 1] != '\n') { return false; }

    char *numptr = strchr(buf,':');
    if (numptr == NULL || numptr == buf) { return false; }
    char *valueptr = strchr(numptr,'=');

    *flag = strtol(numptr + 1,NULL,10);
    if (*flag == 0) { return false; }
    cmd = std::string(buf,numptr);

    if (valueptr != NULL) {
        *isSet = true;
        value = std::string(valueptr + 1,buf+len - 1);
    }
    return true;
}

CmdRet runCmd(uint8_t *buf,size_t len) {

    CmdRet ret = {};
    std::string cmd;
    long f;
    bool isSet = false;
    std::string value;
    if (!parseCmd((char *)buf,len,cmd,&f,value,&isSet)) { goto end; }

    if (s_m_state.blue) {
        if (isSet) {
            doSetCmd(&ret,cmd,f,value);
        } else {
            doReadCmd(&ret,cmd,f);
        }
    } else {
        if (isSet) {
            doMqttSetCmd(&ret,cmd,f,value);
        } else {

        }
    }
    
    end:
    return ret;
}

void clearCmdRet(CmdRet *ret) {
    if (ret->value != NULL) {
        free(ret->value);
    }
    ret->value = NULL;
}



