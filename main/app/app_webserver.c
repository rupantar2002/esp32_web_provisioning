#include <string.h>
#include <cJSON.h>
#include "app_webserver.h"

static const char *TAG = "APP_WEBSERVER";

static app_webserver_UserData_t gUserData = {0};

const service_webserver_UserBase_t *service_webserver_ParseUserData(const service_webserver_UserBase_t *src)
{
    SERVICE_LOGI("Parser called { len: %d , data: '%s'  }", src->len, src->data);

    (void)memset(&gUserData, '\0', sizeof(gUserData));

    gUserData.req = APP_WEBSERVER_REPONCE_NULL;

    cJSON *type = NULL;
    cJSON *reqJson = cJSON_ParseWithLength((const char *)src->data, (src->len + 5U)); /* extra 5 bytes recomanded by cjson*/

    if (reqJson == NULL)
    {
        const char *errPtr = cJSON_GetErrorPtr();
        if (errPtr != NULL)
        {
            SERVICE_LOGE("error : %s", errPtr);
        }
        goto end;
    }

    type = cJSON_GetObjectItemCaseSensitive(reqJson, "type");
    if (cJSON_IsString(type) && (type->valuestring != NULL))
    {

        if (strcmp("prvsn", type->valuestring) == 0)
        {
            cJSON *ssid = cJSON_GetObjectItemCaseSensitive(reqJson, "ssid");
            cJSON *pass = cJSON_GetObjectItemCaseSensitive(reqJson, "pass");

            if (cJSON_IsString(ssid) &&
                cJSON_IsString(pass) &&
                ssid->valuestring != NULL)
            {
                gUserData.req = APP_WEBSERVER_REQUEST_PROVSN;
                (void)strncpy(gUserData.reqData.provsn.ssid, ssid->valuestring, sizeof(gUserData.reqData.provsn.ssid));
                (void)strncpy(gUserData.reqData.provsn.pass, pass->valuestring, sizeof(gUserData.reqData.provsn.pass));
                SERVICE_LOGD("Provisioning request : {ssid: %s,pass: %s}",
                             ssid->valuestring,
                             pass->valuestring);
            }
            else
            {
                SERVICE_LOGE(" %d : INVALID PROVISIONING REQUEST", __LINE__);
            }
        }
        else if (strcmp("scan", type->valuestring) == 0)
        {
            gUserData.req = APP_WEBSERVER_REQUEST_SCAN_START;
        }
        else
        {
            SERVICE_LOGE(" %d : INVALID JSON REQUEST", __LINE__);
        }
    }

end:
    cJSON_Delete(reqJson);

    if (gUserData.req == APP_WEBSERVER_REQUEST_NULL)
    {
        return NULL;
    }
    else
    {
        /* Fill  */
        gUserData.super.parent = src->parent + 1;
        gUserData.super.data = src->data;
        gUserData.super.len = src->len;
    }
    return &gUserData.super;
}

app_Status_t app_webserver_SendResponce(app_webserver_Responce_t resp,
                                        const app_webserver_ResponceData_t *pData)
{
    char *string = NULL;
    cJSON *responce = NULL;

    /* Create responce object */
    responce = cJSON_CreateObject();
    if (responce == NULL)
    {
        goto end;
    }

    /* Add Status to object */
    if (cJSON_AddBoolToObject(responce, "status", (pData->status == APP_STATUS_OK)) == NULL)
    {
        goto end;
    }

    switch (resp)
    {
    case APP_WEBSERVER_REPONCE_SCAN:
    {

        cJSON *networks = NULL;
        cJSON *network = NULL;
        uint16_t index = 0;

        if (cJSON_AddStringToObject(responce, "type", "scan") == NULL)
        {
            goto end;
        }

        networks = cJSON_AddArrayToObject(responce, "networks");
        if (networks == NULL)
        {
            goto end;
        }

        for (index = 0; index < pData->scan.count; ++index)
        {
            network = cJSON_CreateObject();

            if (cJSON_AddStringToObject(network, "ssid", (const char *)pData->scan.records[index].ssid) == NULL)
            {
                goto end;
            }

            if (cJSON_AddNumberToObject(network, "rssi", pData->scan.records[index].rssi) == NULL)
            {
                goto end;
            }

            if (cJSON_AddBoolToObject(network, "open", (pData->scan.records[index].authMode == INTF_WIFI_AUTH_OPEN)) == NULL)
            {
                goto end;
            }

            cJSON_AddItemToArray(networks, network);
        }
        break;
    }
    case APP_WEBSERVER_REPONCE_PROVSN:
    {
        if (cJSON_AddStringToObject(responce, "type", "provsn") == NULL)
        {
            goto end;
        }

        if (cJSON_AddBoolToObject(responce, "accepted", pData->provsn.accepted) == NULL)
        {
            goto end;
        }
        break;
    }
    case APP_WEBSERVER_REPONCE_WIFI_CONN:
    {

        if (cJSON_AddStringToObject(responce, "type", "wifi_conn") == NULL)
        {
            goto end;
        }

        if (cJSON_AddBoolToObject(responce, "connected", pData->wifiConn.connected) == NULL)
        {
            goto end;
        }

        break;
    }

    default:
        break;
    }

    string = cJSON_PrintUnformatted(responce); // TODO Use JSON_PrintPreallocated();

    if (!string)
    {
        SERVICE_LOGE(" %d : Failed create responce json (type : %d)", __LINE__, resp);
    }
    else
    {
        SERVICE_LOGD("resp : '%s'", string);
        (void)service_webserver_SendAsync(string, strlen(string));
    }

end:
    cJSON_Delete(responce);
    return APP_STATUS_OK; // TODO add valid status
}