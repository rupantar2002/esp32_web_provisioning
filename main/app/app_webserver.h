#ifndef __APP_WEBSERVER_H__
#define __APP_WEBSERVER_H__

#include "app.h"
#include "service_webserver.h"
#include "intf_wifi.h"

typedef enum
{
    APP_WEBSERVER_REQUEST_NULL = 0,
    APP_WEBSERVER_REQUEST_PROVSN,
    APP_WEBSERVER_REQUEST_SCAN_START,
    APP_WEBSERVER_REQUEST_SCAN_STOP,
    APP_WEBSERVER_REQUEST_MAX,
} app_webserver_Request_t;

typedef union
{
    struct
    {
        char ssid[INTF_WIFI_SSID_LEN + 2];
        char pass[INTF_WIFI_PASSWORD_LEN + 2];
    } provsn;

} app_webserver_RequestData_t;

typedef enum
{
    APP_WEBSERVER_REPONCE_NULL = 0,
    APP_WEBSERVER_REPONCE_CONNECTION,
    APP_WEBSERVER_REPONCE_SCANLIST,
    APP_WEBSERVER_REPONCE_MAX,
} app_webserver_Responce_t;

typedef union
{
    struct
    {
        uint16_t count;
        intf_wifi_ApRecord_t *records;
    } scanlist;

} app_webserver_ResponceData_t;

typedef struct
{
    service_webserver_UserBase_t super;
    app_webserver_Request_t req;
    app_webserver_RequestData_t reqData;
} app_webserver_UserData_t;

app_Status_t app_webserver_SendResponce(app_webserver_Responce_t resp,
                                  const app_webserver_ResponceData_t *pData);

#endif //__APP_WEBSERVER_H__