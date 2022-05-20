#ifndef CMD_HANDLER_HPP
#define CMD_HANDLER_HPP

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "nvs_params/nvs_params.h"

typedef struct {
    uint8_t state;
    uint8_t *value;
    size_t value_len;
}CmdRet;

extern esp_err_t nvsSetBlob(const char *key,void *buffer,size_t len);
extern size_t nvsGetBlob(const char *key,void *buffer,size_t len);

void cmdHello();

CmdRet runCmd(uint8_t *buf,size_t len);

void clearCmdRet(CmdRet *ret);


#ifdef __cplusplus
};
#endif

#endif 