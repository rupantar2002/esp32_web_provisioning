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

void app_webserver_CreateResponce(app_webserver_Responce_t resp)
{
    SERVICE_LOGI("Responce created");
}