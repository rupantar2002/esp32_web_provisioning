#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <esp_mac.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_log.h>
#include <esp_err.h>
#include "intf_wifi.h"
#include "service.h"
#include "service_webserver.h"
#include "app_webserver.h"

// typedef enum
// {
//     EVENT_NULL = -1,
//     EVENT_START_SCAN = ((1UL << 0UL)),
//     EVENT_STOP_SCAN = ((1UL << 1UL)),
//     EVENT_PROVISION = ((1UL << 1UL)),
// } Event_t;

static const char *TAG = "MAIN";

static intf_wifi_ScanParams_t gScanParams = {
    .blocking = false,
    .channel = INTF_WIFI_DEFAULT_CHANNEL,
    .passive = false,
    .ssid = 0,
};

static app_webserver_ResponceData_t gResponceData = {0};

// static EventGroupHandle_t gEventGroup = NULL;

/**
 * \brief Initalize NVS flash,netif and default event loop.
 * \note  The operations are cutial so if not sccessfull
 *        restart will be performed.
 * \warning Restarts the device.
 */
static void SystemInit(void)
{
    esp_err_t errCode = ESP_OK;

    /* Initialize NVS */
    errCode = nvs_flash_init();

    if (errCode == ESP_ERR_NVS_NO_FREE_PAGES ||
        errCode == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        errCode = nvs_flash_init();
    }

    if (errCode != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS INITIALIZATION FAILED");
        esp_restart();
    }

    ESP_LOGI(TAG, "NVS Falsh Initialized");

    /* Init Netif */
    errCode = esp_netif_init();

    if (errCode != ESP_OK)
    {
        ESP_LOGE(TAG, "NETIF INITIALIZATION FAILED");
        esp_restart();
    }

    ESP_LOGI(TAG, "Netif Initialized");

    /* Init Event Loop */
    errCode = esp_event_loop_create_default();

    if (errCode != ESP_OK)
    {
        ESP_LOGE(TAG, "EVENT LOOP CREATION FAILED");
        esp_restart();
    }

    ESP_LOGI(TAG, "Default Event Loop Created");
}

/**
 * \brief Entry point for any application.
 *
 */
void app_main(void)
{

    // define a variable which holds the state of events
    // const EventBits_t bitsToWaitFor = (EVENT_START_SCAN | EVENT_STOP_SCAN | EVENT_PROVISION);
    // EventBits_t eventGroupValue;

    // gEventGroup = xEventGroupCreate();
    // configASSERT(gEventGroup);

    SystemInit();

    intf_wifi_IpInfo_t ipInfo = {
        .ip = INTF_WIFI_IPV4(10, 10, 10, 10),
        .getway = INTF_WIFI_IPV4(10, 10, 10, 10),
        .netmask = INTF_WIFI_IPV4(255, 255, 255, 0),
    };

    intf_wifi_Init();

    intf_wifi_SetMode(INTF_WIFI_MODE_APSTA);

    intf_wifi_SetIpInfo(&ipInfo);

    intf_wifi_Start();

    // while (true)
    // {
    //     eventGroupValue = xEventGroupWaitBits(gEventGroup,
    //                                           bitsToWaitFor,
    //                                           pdTRUE,
    //                                           pdTRUE,
    //                                           portMAX_DELAY);

    //     if ((eventGroupValue & EVENT_START_SCAN) != 0)
    //     {
    //         ESP_LOGI(TAG, "EVENT_START_SCAN");
    //     }
    //     if ((eventGroupValue & EVENT_STOP_SCAN) != 0)
    //     {
    //         ESP_LOGI(TAG, "EVENT_STOP_SCAN");
    //     }
    //     if ((eventGroupValue & EVENT_PROVISION) != 0)
    //     {
    //         ESP_LOGI(TAG, "EVENT_PROVISION");
    //     }
    // }
}

void intf_wifi_EventCallback(intf_wifi_Event_t event,
                             intf_wifi_EventData_t const *const pData)
{

    switch (event)
    {
    case INTF_WIFI_EVENT_AP_STARTED:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_AP_STARTED", __LINE__, __func__);
        service_webserver_Start();
        service_webserver_SetAuth("user1234567890123456789", "P@ssw0rd_12345678901234");
        break;
    case INTF_WIFI_EVENT_AP_STOPED:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_AP_STOPED", __LINE__, __func__);
        service_webserver_Stop();
        break;
    case INTF_WIFI_EVENT_APSTA_CONNECTED:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_APSTA_CONNECTED", __LINE__, __func__);
        break;
    case INTF_WIFI_EVENT_APSTA_DISCONNECTED:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_APSTA_DISCONNECTED", __LINE__, __func__);
        break;
    case INTF_WIFI_EVENT_APSTA_GOT_IP:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_APSTA_GOT_IP", __LINE__, __func__);
        // ESP_LOGI(TAG, "mac : " MACSTR, MAC2STR(pData->apStaGotIp.mac));
        // ESP_LOGI(TAG, "ip : " IPSTR, pData->apStaGotIp.ip[0],
        //          pData->apStaGotIp.ip[1],
        //          pData->apStaGotIp.ip[2],
        //          pData->apStaGotIp.ip[3]);
        break;
    case INTF_WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_AP_STARTED", __LINE__, __func__);
        break;
    case INTF_WIFI_EVENT_STA_STOP:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_AP_STARTED", __LINE__, __func__);
        break;
    case INTF_WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_STA_CONNECTED", __LINE__, __func__);
        ESP_LOGI(TAG, "ssid : \"%s\"", pData->staConnected.ssid);
        ESP_LOGI(TAG, "aid : \"%d\"", pData->staConnected.aid);

        break;
    case INTF_WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_STA_DISCONNECTED", __LINE__, __func__);
        ESP_LOGI(TAG, "ssid : \"%s\"", pData->staDisconnected.ssid);
        ESP_LOGI(TAG, "reason : \"%d\"", pData->staDisconnected.reason);

        break;
    case INTF_WIFI_EVENT_STA_GOT_IP:
        break;
    case INTF_WIFI_EVENT_STA_LOST_IP:
        break;

    case INTF_WIFI_EVENT_SCAN_DONE:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_SCAN_DONE", __LINE__, __func__);
        ESP_LOGI(TAG, "status : %d", pData->scanDone.status);
        ESP_LOGI(TAG, "Aps found : %d", pData->scanDone.count);

        // if (pData->scanDone.status)
        // {
        //     intf_wifi_CreateScanList(pData->scanDone.count);
        // }
        break;
    case INTF_WIFI_EVENT_SCAN_LIST:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_SCAN_LIST", __LINE__, __func__);
        if (pData->scanList.count > 0)
        {
            for (uint16_t i = 0; i < pData->scanList.count; i++) // TODO remove
            {
                ESP_LOGI(TAG, "\n");
                ESP_LOGI(TAG, "ssid : \"%s\"", pData->scanList.records[i].ssid);
                ESP_LOGI(TAG, "rssi : %d", pData->scanList.records[i].rssi);
                ESP_LOGI(TAG, "primary : %d", pData->scanList.records[i].primary);
                ESP_LOGI(TAG, "wps : %d", pData->scanList.records[i].wps);
                ESP_LOGI(TAG, "\n");
            }
            (void)memset(&gResponceData, 0, sizeof(gResponceData));
            gResponceData.scanlist.count = pData->scanList.count;
            gResponceData.scanlist.records = pData->scanList.records;
            (void)app_webserver_SendResponce(APP_WEBSERVER_REPONCE_SCANLIST, &gResponceData);
        }

        // intf_wifi_DestroyScanList();
        break;

    case INTF_WIFI_EVENT_MAX:
        break;
    default:
        break;
    }
}

service_Status_t service_webserver_EventCallback(service_webserver_Event_t event,
                                                 service_webserver_EventData_t const *const pData)
{
    service_Status_t status;
    switch (event)
    {
    case SERVICE_WEBSERVER_EVENT_USER:
        ESP_LOGI(TAG, " %d : %s : SERVICE_WEBSERVER_EVENT_USER", __LINE__, __func__);
        // ESP_LOGI(TAG, "parent : %d", pData->userBase.parent);
        // ESP_LOGI(TAG, "len : %d", pData->userBase.len);
        // ESP_LOGI(TAG, "data :'%s'", pData->userBase.data);

        if (pData->userBase.parent > 0)
        {
            app_webserver_UserData_t *usrData = SERVICE_CONTAINER_OF(&pData->userBase,
                                                                     app_webserver_UserData_t, super);
            ESP_LOGI(TAG, " request_type : %d ", usrData->req);

            switch (usrData->req)
            {
            case APP_WEBSERVER_REQUEST_PROVSN:
                ESP_LOGI(TAG, "provisioning Request :{ ssid: %s , pass: %s }",
                         usrData->reqData.provsn.ssid,
                         usrData->reqData.provsn.pass);

                /* Start Provisioning */
                intf_wifi_Cred_t cred;
                (void)strncpy(cred.ssid, usrData->reqData.provsn.ssid, sizeof(cred.ssid));
                (void)strncpy(cred.pass, usrData->reqData.provsn.pass, sizeof(cred.pass));
                (void)intf_wifi_SetCredentials(INTF_WIFI_MODE_STA, &cred);
                (void)intf_wifi_Connect();
                break;
            case APP_WEBSERVER_REQUEST_SCAN_START:
                (void)intf_wifi_StartScanning(&gScanParams);
                break;
            case APP_WEBSERVER_REQUEST_SCAN_STOP:
                (void)intf_wifi_StopScanning();
                break;
            default:
                break;
            }
        }

        // status = service_webserver_Send((const char *)pData->userBase.data, pData->userBase.len);

        break;
    default:
        status = SERVICE_STATUS_OK;
        break;
    }

    return status;
}