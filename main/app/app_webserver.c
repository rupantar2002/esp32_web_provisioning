#include <string.h>
#include "app_webserver.h"

static const char *TAG = "APP_WEBSERVER";

static app_webserver_UserData_t gUserData = {0};

const service_webserver_UserBase_t *service_webserver_ParseUserData(const service_webserver_UserBase_t *src)
{
    SERVICE_LOGI("Parser called");
    (void)memset(&gUserData, '\0', sizeof(gUserData));

    /* Fill  */
    gUserData.super.parent = src->parent + 1;
    gUserData.super.data = src->data;
    gUserData.super.len = src->len;
    gUserData.req = APP_WEBSERVER_REQUEST_SCANLIST;

    return &gUserData.super;
}

void app_webserver_CreateResponce(app_webserver_Responce_t resp)
{
    SERVICE_LOGI("Responce created");
}