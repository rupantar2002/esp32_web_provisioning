#ifndef __SERVICE_H__
#define __SERVICE_H__

#include <stdint.h>
#include <esp_log.h>

enum service_Status
{
    SERVICE_STATUS_OK = 0,
    SERVICE_STATUS_ERROR,
    SERVICE_STATUS_MAX,
};

typedef uint8_t service_Status_t;

#define SERVICE_LOGE(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)

#define SERVICE_LOGD(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)

#define SERVICE_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)

#endif //__SERVICE_H__