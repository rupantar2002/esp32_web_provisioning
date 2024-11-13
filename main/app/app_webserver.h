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
    APP_WEBSERVER_REPONCE_PROVSN,
    APP_WEBSERVER_REPONCE_WIFI_CONN,
    APP_WEBSERVER_REPONCE_SCAN,
    APP_WEBSERVER_REPONCE_MAX,
} app_webserver_Responce_t;

typedef union
{
    app_Status_t status; /* needs to check status of the event before donig anything*/

    struct
    {
        uint16_t count;
        intf_wifi_ApRecord_t *records;
    } scan;

    struct
    {
        bool accepted;
    } provsn;

    struct
    {
        bool connected;
    } wifiConn;

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