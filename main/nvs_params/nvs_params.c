
#include "nvs_params.h"

static nvs_handle _nhandle = NULL;
#define NVS_NAME "storage"

nvs_handle nvsGetStorageHandle() {
    if (_nhandle == NULL) {
        nvs_open(NVS_NAME,NVS_READWRITE,&_nhandle);
    }
    return _nhandle;
}

esp_err_t nvsSetBlob(const char *key,void *buffer,size_t len) {
    nvs_handle handle = nvsGetStorageHandle();
    return nvs_set_blob(handle,key,buffer,len);
}

size_t nvsGetBlob(const char *key,void *buffer,size_t len) {
    nvs_handle handle = nvsGetStorageHandle();
    size_t l = len;
    nvs_get_blob(handle,key,buffer,&l);
    return l;
}
