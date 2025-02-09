#include <string.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_mac.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_log.h>
#include <esp_err.h>
#include "intf_wifi.h"

#define INTF_WIFI_LOGE(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)

#define INTF_WIFI_LOGD(fmt, ...) ESP_LOGD(TAG, fmt, ##__VA_ARGS__)

#define INTF_WIFI_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)

typedef struct
{
    struct
    {
        uint8_t init : 1;
        uint8_t started : 1;
        uint8_t staConn : 1;
        uint8_t scanning : 1;
    } flags;
    wifi_mode_t mode;
    esp_netif_t *apNetif;
    esp_netif_t *staNetif;
    intf_wifi_Event_t evt;
    intf_wifi_EventData_t evtData;
#if (INTF_WIFI_SCAN_STATE == INTF_WIFI_SCAN_ENABLE)
#if (INTF_WIFI_SCAN_LIST_ALLOC_TYPE == INTF_WIFI_SCAN_LIST_ALLOC_STATIC)
    wifi_ap_record_t apRecordList[INTF_WIFI_SCAN_LIST_MAX_LENGTH];
    uint16_t apRecordCount;
#else
    wifi_ap_record_t *apRecordList;
    uint16_t apRecordCount;
    uint16_t apCount;
#endif // INTF_WIFI_SCAN_LIST_ALLOC_TYPE
#endif // INTF_WIFI_SCAN_STATE

} intf_wifi_Ctx_t;

static const char *TAG = "INTF_WIFI";

static intf_wifi_Ctx_t gIntfWifi = {0};

/**
 * \brief Handles events comming from wifi api.
 *
 * \param arg Argument passed by user.
 * \param event_base Event base.
 * \param event_id Event id.
 * \param event_data Event data.
 */
static void WifiEventHandler(void *arg,
                             esp_event_base_t event_base,
                             int32_t event_id,
                             void *event_data);

/**
 * \brief Convert intf_wifi_Mode_t type to wifi_mode_t.
 *
 * \param mode Mode (i.e INTF_WIFI_MODE_STA)
 * \return wifi_mode_t
 */
static wifi_mode_t ConvertToMode(intf_wifi_Mode_t mode)
{
    wifi_mode_t temp;
    switch (mode)
    {
    case INTF_WIFI_MODE_AP:
        temp = WIFI_MODE_AP;
        break;
    case INTF_WIFI_MODE_STA:
        temp = WIFI_MODE_STA;
        break;
    case INTF_WIFI_MODE_APSTA:
        temp = WIFI_MODE_APSTA;
        break;
    default:
        temp = WIFI_MODE_NULL;
        break;
    }
    return temp;
}

/**
 * \brief Gives mode value name.
 *
 * \param mode Mode.
 * \return const char* Mode name.
 */
static const char *GetModeStr(intf_wifi_Mode_t mode)
{
    char *modeStr = NULL;

    switch (mode)
    {
    case INTF_WIFI_MODE_AP:
        modeStr = (char *)"Access Point";
        break;
    case INTF_WIFI_MODE_STA:
        modeStr = (char *)"Station";
        break;
    case INTF_WIFI_MODE_APSTA:
        modeStr = (char *)"Access Point + Station";
        break;
    default:
        break;
    }
    return (const char *)modeStr;
}

/**
 * \brief Set the wifi mode.
 *
 * \return true For success.
 * \return false For faliour.
 */
static bool SetWifiMode(void)
{
    if (esp_wifi_set_mode(gIntfWifi.mode) != ESP_OK)
    {
        INTF_WIFI_LOGE(" %d : FAILED TO SET MODE", __LINE__);
        return false;
    }
    else
    {
        return true;
    }
}

/**
 * \brief Copy esp_ip4_addr_t to byte array;
 *
 * \param ip Ip address.
 * \param bytes Byte array.
 * \warning bytes must be >=4 bytes in length.
 */
static void IpToBytes(esp_ip4_addr_t ip, uint8_t *bytes)
{
    bytes[3] = (ip.addr >> 24) & 0xFF; /* Extract the first byte */
    bytes[2] = (ip.addr >> 16) & 0xFF; /* Extract the second byte */
    bytes[1] = (ip.addr >> 8) & 0xFF;  /* Extract the third byte */
    bytes[0] = ip.addr & 0xFF;         /* Extract the fourth byte */
}

/**
 * \brief Copy ap records from internal wifi driver to localy allocated list.
 *
 * \param count Number of ap records.
 * \return true For success.
 * \return false For faliour.
 */
static bool CopyListToBuff(uint16_t count)
{
    uint8_t success = false;
#if (INTF_WIFI_SCAN_STATE == INTF_WIFI_SCAN_ENABLE)
#if (INTF_WIFI_SCAN_LIST_ALLOC_TYPE == INTF_WIFI_SCAN_LIST_ALLOC_STATIC)

    /* clear buffer */
    (void)memset(gIntfWifi.apRecordList, '\0', sizeof(gIntfWifi.apRecordList));

    if (count > INTF_WIFI_SCAN_LIST_MAX_LENGTH)
    {
        count = INTF_WIFI_SCAN_LIST_MAX_LENGTH;
    }

    if (esp_wifi_scan_get_ap_records(&count,
                                     gIntfWifi.apRecordList) != ESP_OK)
    {
        INTF_WIFI_LOGE(" %d : FAILED TO READ RECORDS", __LINE__);
    }
    else
    {
        success = true; /* set flag*/
    }

#else

    if (gIntfWifi.apRecordList)
    {

        /* clear buffer */
        (void)memset(gIntfWifi.apRecordList, '\0', gIntfWifi.apCount);

        if (count > gIntfWifi.apCount)
        {
            count = gIntfWifi.apCount;
        }

        if (esp_wifi_scan_get_ap_records(&count,
                                         gIntfWifi.apRecordList) != ESP_OK)
        {
            INTF_WIFI_LOGE(" %d : FAILED TO READ RECORDS", __LINE__);
        }
        else
        {
            success = true; /* set flag*/
        }
    }
    else
    {
        INTF_WIFI_LOGE(" %d : NO LIST FOUND", __LINE__);
    }

#endif // INTF_WIFI_SCAN_LIST_ALLOC_TYPE
#endif // INTF_WIFI_SCAN_STATE
    gIntfWifi.apRecordCount = count;
    return success;
}

/**
 * \brief Configure wifi interface.
 *
 * \param intf Wifi interface.
 * \param pCred Wifi credentials.
 * \return true For success.
 * \return false For faliour.
 */
static bool ConfigureInterface(wifi_interface_t intf, intf_wifi_Cred_t *const pCred)
{

    wifi_config_t wifiCfg;
    (void)memset(&wifiCfg, '\0', sizeof(wifiCfg));

    if (esp_wifi_get_config(intf, &wifiCfg) == ESP_OK) /* retrive wifi interface config */
    {
        switch (intf)
        {
        case WIFI_IF_AP:
            wifiCfg.ap.channel = INTF_WIFI_DEFAULT_CHANNEL;
            wifiCfg.ap.max_connection = INTF_WIFI_MAX_CONNECTIONS;
            wifiCfg.ap.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
            wifiCfg.ap.pmf_cfg.required = false;
            wifiCfg.ap.ssid_len = 0;
            (void)strncpy((char *)wifiCfg.ap.ssid, pCred->ssid, sizeof(wifiCfg.ap.ssid));
            if (strlen(pCred->pass) == 0)
            {
                wifiCfg.ap.authmode = WIFI_AUTH_OPEN;
            }
            else
            {
                (void)strncpy((char *)wifiCfg.ap.password, pCred->pass, sizeof(wifiCfg.ap.password));
            }

            break;
        case WIFI_IF_STA:
            (void)strncpy((char *)wifiCfg.sta.ssid, pCred->ssid, sizeof(wifiCfg.sta.ssid));
            (void)strncpy((char *)wifiCfg.sta.password, pCred->pass, sizeof(wifiCfg.sta.password));
            wifiCfg.sta.threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
            wifiCfg.sta.pmf_cfg.capable = true;
            wifiCfg.sta.pmf_cfg.required = false;
#if (INTF_WIFI_FAST_SCAN == 1)
            wifiCfg.sta.scan_method = WIFI_FAST_SCAN;
            wifiCfg.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
            wifiCfg.sta.threshold.rssi = -127;
            wifiCfg.sta.threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
#else
            wifiCfg.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
#endif
            break;
        default:
            INTF_WIFI_LOGE(" %d : INVALID INTERFACE [%d]", __LINE__, intf);
            return false;
            break;
        }

        /* configure wifi interface */
        if (esp_wifi_set_config(intf, &wifiCfg) == ESP_OK)
        {
            return true;
        }
        else
        {
            INTF_WIFI_LOGE(" %d : FAILED TO CONFIGURE INTERFACE [%d]", __LINE__, intf);
        }
    }
    else
    {
        INTF_WIFI_LOGE(" %d : FAILED TO GET CONFIG [%d]", __LINE__, intf);
    }

    return false;
}

intf_wifi_Status_t intf_wifi_Init(void)
{
    if (gIntfWifi.flags.init)
    {
        INTF_WIFI_LOGE(" %d : ALREADY INITIALIZED", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    /* register for wifi events */
    if (esp_event_handler_instance_register(WIFI_EVENT,
                                            ESP_EVENT_ANY_ID,
                                            &WifiEventHandler,
                                            NULL,
                                            NULL) != ESP_OK)
    {
        INTF_WIFI_LOGE(" %d : FAILED TO REGISTER WIFI EVENT", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    /* register for ip events */
    if (esp_event_handler_instance_register(IP_EVENT,
                                            ESP_EVENT_ANY_ID,
                                            &WifiEventHandler,
                                            NULL,
                                            NULL) != ESP_OK)
    {
        INTF_WIFI_LOGE(" %d : FAILED TO REGISTER IP EVENT", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    /* Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    intf_wifi_Cred_t tempCred;
    esp_err_t errCode = ESP_OK;

    if (esp_wifi_init(&cfg) != ESP_OK)
    {
        INTF_WIFI_LOGE(" %d : FAILED TO INIT WIFI", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    gIntfWifi.apNetif = esp_netif_create_default_wifi_ap();
    if (gIntfWifi.apNetif == NULL)
    {
        INTF_WIFI_LOGE(" %d : FAILED TO CREATE NETIF AP", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    gIntfWifi.staNetif = esp_netif_create_default_wifi_sta();
    if (gIntfWifi.staNetif == NULL)
    {
        INTF_WIFI_LOGE(" %d : FAILED TO CREATE NETIF STA", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    // set some default values
    gIntfWifi.mode = WIFI_MODE_APSTA;

    if (!SetWifiMode())
    {
        return INTF_WIFI_STATUS_ERROR;
    }

    (void)strncpy(tempCred.ssid, INTF_WIFI_DEFAULT_SSID, sizeof(tempCred.ssid));
    (void)memset(tempCred.pass, '\0', sizeof(tempCred.pass));

    if (!ConfigureInterface(WIFI_IF_AP, &tempCred))
    {
        return INTF_WIFI_STATUS_ERROR;
    }

    (void)memset(tempCred.ssid, '\0', sizeof(tempCred.ssid));
    if (!ConfigureInterface(WIFI_IF_STA, &tempCred))
    {
        return INTF_WIFI_STATUS_ERROR;
    }

    /* Set default interface */
    switch (gIntfWifi.mode)
    {
    case WIFI_MODE_AP:
        errCode = esp_netif_set_default_netif(gIntfWifi.apNetif);
        break;
    case WIFI_MODE_STA:
    case WIFI_MODE_APSTA:
    default:
        errCode = esp_netif_set_default_netif(gIntfWifi.staNetif);
        break;
    }

    if (errCode != ESP_OK)
    {
        INTF_WIFI_LOGE(" %d : FAILED TO SET DEFAULT INTERFACE", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }
    else
    {
        gIntfWifi.flags.init = true; /* Set init flag */
        INTF_WIFI_LOGI("WIFI INITIALIZED");
        return INTF_WIFI_STATUS_OK;
    }
}

intf_wifi_Status_t intf_wifi_SetIpInfo(intf_wifi_IpInfo_t *pIpInfo)
{
    if (pIpInfo == NULL)
    {
        INTF_WIFI_LOGE(" %d : INVALID IPINFO", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    if (gIntfWifi.mode == WIFI_MODE_AP || gIntfWifi.mode == WIFI_MODE_APSTA)
    {
        esp_netif_ip_info_t ipInfo;

        if (esp_netif_dhcps_stop(gIntfWifi.apNetif) != ESP_OK)
        {
            INTF_WIFI_LOGE(" %d : FAILED TO STOP DHCPS", __LINE__);
            return INTF_WIFI_STATUS_ERROR;
        }

        ipInfo.ip.addr = ESP_IP4TOADDR(pIpInfo->ip[0], pIpInfo->ip[1],
                                       pIpInfo->ip[2], pIpInfo->ip[3]);
        ipInfo.netmask.addr = ESP_IP4TOADDR(pIpInfo->netmask[0], pIpInfo->netmask[1],
                                            pIpInfo->netmask[2], pIpInfo->netmask[3]);
        ipInfo.gw.addr = ESP_IP4TOADDR(pIpInfo->getway[0], pIpInfo->getway[1],
                                       pIpInfo->getway[2], pIpInfo->getway[3]);

        if (esp_netif_set_ip_info(gIntfWifi.apNetif, &ipInfo) != ESP_OK)
        {
            INTF_WIFI_LOGE(" %d : FAILED TO SET IP INFO", __LINE__);
            return INTF_WIFI_STATUS_ERROR;
        }

        if (esp_netif_dhcps_start(gIntfWifi.apNetif) != ESP_OK)
        {
            INTF_WIFI_LOGE(" %d : FAILED TO START DHCPS", __LINE__);
            return INTF_WIFI_STATUS_ERROR;
        }
        return INTF_WIFI_STATUS_OK;
    }
    else
    {
        INTF_WIFI_LOGE(" %d : MODE SHOULD BE INTF_WIFI_MODE_AP or INTF_WIFI_MODE_APSTA", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }
}

intf_wifi_Status_t intf_wifi_SetMode(intf_wifi_Mode_t mode)
{

    if (mode >= INTF_WIFI_MODE_MAX && mode <= INTF_WIFI_MODE_NULL)
    {
        INTF_WIFI_LOGE(" %d : INVALID MODE", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    gIntfWifi.mode = ConvertToMode(mode);
    if (!SetWifiMode())
    {
        return INTF_WIFI_STATUS_ERROR;
    }
    else
    {
        INTF_WIFI_LOGI("WIFI MODE SET TO => \"%s\"", GetModeStr(mode));
        return INTF_WIFI_STATUS_OK;
    }
}

intf_wifi_Status_t intf_wifi_SetCredentials(intf_wifi_Mode_t mode,
                                            intf_wifi_Cred_t *const pCred)
{
    if (pCred == NULL)
    {
        INTF_WIFI_LOGE(" %d : INVALID CREDENTIALS", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    if (mode >= INTF_WIFI_MODE_MAX && mode <= INTF_WIFI_MODE_NULL)
    {
        INTF_WIFI_LOGE(" %d : INVALID MODE", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    wifi_interface_t intf = WIFI_IF_MAX;

    switch (mode)
    {
    case INTF_WIFI_MODE_AP:
        intf = WIFI_IF_AP;
        break;
    case INTF_WIFI_MODE_STA:
        intf = WIFI_IF_STA;
        break;
    default:
        break;
    }

    if (ConfigureInterface(intf, pCred))
    {
        INTF_WIFI_LOGI("\"%s\" CREDENTIALS SET", GetModeStr(mode));
        return INTF_WIFI_STATUS_OK;
    }
    else
    {
        INTF_WIFI_LOGE(" %d : FAILED TO CONFIGURE INTERFACE", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }
}

intf_wifi_Status_t intf_wifi_Start(void)
{

    if (esp_wifi_start() != ESP_OK)
    {
        INTF_WIFI_LOGE(" %d : FAILED TO START WIFI", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }
    else
    {
        return INTF_WIFI_STATUS_OK;
    }
}

intf_wifi_Status_t intf_wifi_Stop(void)
{
    if (gIntfWifi.flags.staConn)
    {
        (void)esp_wifi_disconnect();
    }

    if (esp_wifi_stop() != ESP_OK)
    {
        INTF_WIFI_LOGE(" %d : FAILED TO STOP WIFI", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    return INTF_WIFI_STATUS_OK;
}

bool intf_wifi_IsStaConnected(void)
{
    return gIntfWifi.flags.staConn;
}

intf_wifi_Status_t intf_wifi_Connect(void)
{
    if (!gIntfWifi.flags.staConn)
    {
        if (esp_wifi_connect() != ESP_OK)
        {
            INTF_WIFI_LOGE(" %d : FAILED TO CONNECT TO AP", __LINE__);
        }
        else
        {
            return INTF_WIFI_STATUS_OK;
        }
    }
    else
    {
        INTF_WIFI_LOGE(" %d : CONNECTION PRESENT", __LINE__);
    }
    return INTF_WIFI_STATUS_ERROR;
}

intf_wifi_Status_t intf_wifi_Disconnect(void)
{
    if (!gIntfWifi.flags.staConn)
    {
        if (esp_wifi_disconnect() != ESP_OK)
        {
            INTF_WIFI_LOGE(" %d : FAILED TO DISCONNECT FROM REMOTE AP", __LINE__);
            return INTF_WIFI_STATUS_ERROR;
        }
        return INTF_WIFI_STATUS_OK;
    }
    else
    {
        INTF_WIFI_LOGE(" %d : NOT CONNECTED", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }
}

void intf_wifi_DeInit(void)
{
    if (gIntfWifi.flags.init)
    {
#if (INTF_WIFI_SCAN_LIST_ALLOC_TYPE == INTF_WIFI_SCAN_LIST_ALLOC_DYNAMIC)
        if (gIntfWifi.apRecordList)
        {
            free(gIntfWifi.apRecordList); /* free record list*/
        }
#endif // INTF_WIFI_SCAN_LIST_ALLOC_TYPE

        (void)esp_wifi_disconnect();
        (void)esp_wifi_stop();
        (void)esp_wifi_deinit();
        gIntfWifi.flags.init = false;
    }
}

#if (INTF_WIFI_SCAN_STATE == INTF_WIFI_SCAN_ENABLE)

intf_wifi_Status_t intf_wifi_StartScanning(intf_wifi_ScanParams_t *pParams)
{
    if (pParams)
    {

        wifi_scan_config_t scanCfg = {
            .ssid = (uint8_t *)pParams->ssid,
            .bssid = 0,
            .channel = pParams->channel,
            .show_hidden = INTF_WIFI_SHOW_HIDDEN,
            .scan_type = (pParams->passive ? WIFI_SCAN_TYPE_PASSIVE : WIFI_SCAN_TYPE_ACTIVE),
        };

        if (pParams->passive)
        {
            scanCfg.scan_time.passive = INTF_WIFI_PASSIVE_SCAN_TIME;
        }

        // scanCfg.ssid = (uint8_t *)pParams->ssid;
        // scanCfg.channel = pParams->channel;
        // scanCfg.show_hidden = INTF_WIFI_SHOW_HIDDEN;
        // scanCfg.scan_type = (pParams->passive ? WIFI_SCAN_TYPE_PASSIVE : WIFI_SCAN_TYPE_ACTIVE);
        // scanCfg.scan_time.active.min = INTF_WIFI_MIN_ACTIVE_SCAN_TIME;
        // scanCfg.scan_time.active.max = INTF_WIFI_MAX_ACTIVE_SCAN_TIME;
        // scanCfg.scan_time.passive = INTF_WIFI_PASSIVE_SCAN_TIME;
        // scanCfg.home_chan_dwell_time = 100U, /* 100 ms is generally enough; lower values may reduce scan time but increase missed results.*/

        if (esp_wifi_scan_start(&scanCfg, pParams->blocking) == ESP_OK) // TODO scanning params
        {
            return INTF_WIFI_STATUS_OK;
        }
        else
        {
            INTF_WIFI_LOGE(" %d : FAILED TO START SCANNING", __LINE__);
        }
    }
    return INTF_WIFI_STATUS_ERROR;
}

intf_wifi_Status_t intf_wifi_StopScanning(void)
{
    if (esp_wifi_scan_stop() != ESP_OK)
    {
        INTF_WIFI_LOGE(" %d : FAILED TO STOP SCANNING", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }
    else
    {
        return INTF_WIFI_STATUS_OK;
    }
}

intf_wifi_Status_t intf_wifi_GetScanList(const intf_wifi_ApRecord_t **records,
                                         uint16_t *count)
{
    if (records == NULL || count == NULL)
    {
        return INTF_WIFI_STATUS_ERROR;
    }

#if (INTF_WIFI_SCAN_LIST_ALLOC_TYPE == INTF_WIFI_SCAN_LIST_ALLOC_DYNAMIC)

    if (gIntfWifi.apRecordList == NULL)
    {
        INTF_WIFI_LOGE(" %d : NO LIST FOUND", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }
#endif // INTF_WIFI_SCAN_LIST_ALLOC_TYPE

    if (gIntfWifi.apRecordCount)
    {
        *records = (intf_wifi_ApRecord_t *)gIntfWifi.apRecordList;
        *count = gIntfWifi.apRecordCount;
        return INTF_WIFI_STATUS_OK;
    }
    else
    {
        INTF_WIFI_LOGE(" %d : LIST EMPTY", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }
}

#if (INTF_WIFI_SCAN_LIST_ALLOC_TYPE == INTF_WIFI_SCAN_LIST_ALLOC_DYNAMIC)

intf_wifi_Status_t intf_wifi_CreateScanList(uint16_t count)
{
    if (count > 0)
    {
        if (gIntfWifi.apRecordList == NULL)
        {
            gIntfWifi.apRecordList = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * count);
            if (gIntfWifi.apRecordList != NULL)
            {
                gIntfWifi.apCount = count;
                INTF_WIFI_LOGI("LIST CREATED");
                return INTF_WIFI_STATUS_OK;
            }
            else
            {
                gIntfWifi.apRecordList = NULL;
                gIntfWifi.apCount = 0;
            }
        }
        else
        {
            INTF_WIFI_LOGE(" %d : LIST EXISTS", __LINE__);
        }
    }

    return INTF_WIFI_STATUS_ERROR;
}

void intf_wifi_DestroyScanList(void)
{
    if (gIntfWifi.apRecordList)
    {
        free(gIntfWifi.apRecordList);
        gIntfWifi.apRecordList = NULL;
        gIntfWifi.apCount = 0;
    }
}

#endif // INTF_WIFI_SCAN_LIST_ALLOC_TYPE
#endif // INTF_WIFI_SCAN_STATE

__attribute__((__weak__)) void intf_wifi_EventCallback(intf_wifi_Event_t event,
                                                       intf_wifi_EventData_t const *const pData)
{
    (void)event;
    (void)pData;
}

static void WifiEventHandler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    /* clears event data */
    (void)memset(&gIntfWifi.evtData, 0, sizeof(gIntfWifi.evtData));

    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        // AP
        case WIFI_EVENT_AP_START:
        {
            INTF_WIFI_LOGD("WIFI_EVENT => WIFI_EVENT_AP_START");
            intf_wifi_EventCallback(INTF_WIFI_EVENT_AP_STARTED, &gIntfWifi.evtData);
            break;
        }
        case WIFI_EVENT_AP_STOP:
        {
            INTF_WIFI_LOGD("WIFI_EVENT => WIFI_EVENT_AP_STOP");
            intf_wifi_EventCallback(INTF_WIFI_EVENT_AP_STOPED, &gIntfWifi.evtData);
            break;
        }
        case WIFI_EVENT_AP_STACONNECTED:
        {
            INTF_WIFI_LOGD("WIFI_EVENT => WIFI_EVENT_AP_STACONNECTED");
            wifi_event_ap_staconnected_t *evt = (wifi_event_ap_staconnected_t *)event_data;

            (void)memcpy(gIntfWifi.evtData.apStaConnected.mac, evt->mac, sizeof(evt->mac));
            gIntfWifi.evtData.apStaConnected.aid = evt->aid;

            intf_wifi_EventCallback(INTF_WIFI_EVENT_APSTA_CONNECTED, &gIntfWifi.evtData);
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            INTF_WIFI_LOGD("WIFI_EVENT => WIFI_EVENT_AP_STADISCONNECTED");
            wifi_event_ap_stadisconnected_t *evt = (wifi_event_ap_stadisconnected_t *)event_data;

            (void)memcpy(gIntfWifi.evtData.apStaDisconnected.mac, evt->mac, sizeof(evt->mac));
            gIntfWifi.evtData.apStaDisconnected.aid = evt->aid;
            gIntfWifi.evtData.apStaDisconnected.reason = evt->reason;

            intf_wifi_EventCallback(INTF_WIFI_EVENT_APSTA_DISCONNECTED, &gIntfWifi.evtData);
            break;
        }
            // STA
        case WIFI_EVENT_STA_START:
        {
            INTF_WIFI_LOGD("WIFI_EVENT => WIFI_EVENT_STA_START");
            intf_wifi_EventCallback(INTF_WIFI_EVENT_STA_START, &gIntfWifi.evtData);
            break;
        }
        case WIFI_EVENT_STA_STOP:
        {
            INTF_WIFI_LOGD("WIFI_EVENT => WIFI_EVENT_STA_STOP");
            intf_wifi_EventCallback(INTF_WIFI_EVENT_STA_STOP, &gIntfWifi.evtData);
            break;
        }
        case WIFI_EVENT_STA_CONNECTED:
        {
            INTF_WIFI_LOGD("WIFI_EVENT => WIFI_EVENT_STA_CONNECTED");
            wifi_event_sta_connected_t *evt = (wifi_event_sta_connected_t *)event_data;

            gIntfWifi.flags.staConn = true; /* set connection flag */

            (void)memcpy(gIntfWifi.evtData.staConnected.ssid, evt->ssid, evt->ssid_len);
            (void)memcpy(gIntfWifi.evtData.staConnected.bssid, evt->bssid, sizeof(evt->bssid));
            gIntfWifi.evtData.staConnected.ssidLen = evt->ssid_len;
            gIntfWifi.evtData.staConnected.authMode = (intf_wifi_AuthMode_t)evt->authmode;
            gIntfWifi.evtData.staConnected.aid = evt->aid;

            intf_wifi_EventCallback(INTF_WIFI_EVENT_STA_CONNECTED, &gIntfWifi.evtData);
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            INTF_WIFI_LOGD("WIFI_EVENT => WIFI_EVENT_STA_DISCONNECTED");
            wifi_event_sta_disconnected_t *evt = (wifi_event_sta_disconnected_t *)event_data;

            gIntfWifi.flags.staConn = false; /* reset connection flag */

            (void)memcpy(gIntfWifi.evtData.staDisconnected.ssid, evt->ssid, evt->ssid_len);
            (void)memcpy(gIntfWifi.evtData.staDisconnected.bssid, evt->bssid, sizeof(evt->bssid));
            gIntfWifi.evtData.staDisconnected.ssidLen = evt->ssid_len;
            gIntfWifi.evtData.staDisconnected.reason = evt->reason;

            intf_wifi_EventCallback(INTF_WIFI_EVENT_STA_DISCONNECTED, &gIntfWifi.evtData);
            break;
        }
        // Scan
        case WIFI_EVENT_SCAN_DONE:
        {
            INTF_WIFI_LOGD("WIFI_EVENT => WIFI_EVENT_SCAN_DONE");
            wifi_event_sta_scan_done_t *evt = (wifi_event_sta_scan_done_t *)event_data;

            INTF_WIFI_LOGD("status : %d ", (int)evt->status);
            INTF_WIFI_LOGD("number : %d ", evt->number);
            INTF_WIFI_LOGD("scan_id : %d ", evt->scan_id);

            /* scan event */
            gIntfWifi.evtData.scanDone.status = !evt->status; /* as 0 value means success */
            gIntfWifi.evtData.scanDone.count = (evt->status == 0) ? evt->number : 0U;
            intf_wifi_EventCallback(INTF_WIFI_EVENT_SCAN_DONE, &gIntfWifi.evtData);

            uint8_t success = CopyListToBuff(evt->number);
            if (success && (evt->status == 0)) /* only call if successful */
            {
                // for (int i = 0; i < evt->number; i++)
                // {
                //     INTF_WIFI_LOGD("\n");
                //     INTF_WIFI_LOGD("ssid : \"%s\"", gIntfWifi.apRecordList[i].ssid);
                //     INTF_WIFI_LOGD("rssi : %d", gIntfWifi.apRecordList[i].rssi);
                //     INTF_WIFI_LOGD("primary : %d", gIntfWifi.apRecordList[i].primary);
                //     INTF_WIFI_LOGD("wps : %d", gIntfWifi.apRecordList[i].wps);
                //     INTF_WIFI_LOGD("\n");
                // }

                /* scanlist event */
                gIntfWifi.evtData.scanList.records = (intf_wifi_ApRecord_t *)gIntfWifi.apRecordList;
                gIntfWifi.evtData.scanList.count = gIntfWifi.apRecordCount;
                intf_wifi_EventCallback(INTF_WIFI_EVENT_SCAN_LIST, &gIntfWifi.evtData);
            }
            break;
        }
        default:
            // ignored events
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        // AP
        case IP_EVENT_AP_STAIPASSIGNED:
        {
            INTF_WIFI_LOGD("IP_EVENT => IP_EVENT_AP_STAIPASSIGNED");
            ip_event_ap_staipassigned_t *evt = (ip_event_ap_staipassigned_t *)event_data;

            (void)memcpy(gIntfWifi.evtData.apStaGotIp.mac, evt->mac, sizeof(evt->mac));
            IpToBytes(evt->ip, gIntfWifi.evtData.apStaGotIp.ip);

            intf_wifi_EventCallback(INTF_WIFI_EVENT_APSTA_GOT_IP, &gIntfWifi.evtData);
            break;
        }
        // STA
        case IP_EVENT_STA_GOT_IP:
        {
            INTF_WIFI_LOGD("IP_EVENT => IP_EVENT_STA_GOT_IP");

            ip_event_got_ip_t *evt = (ip_event_got_ip_t *)event_data;
            IpToBytes(evt->ip_info.ip, gIntfWifi.evtData.staGotIp.ipInfo.ip);
            IpToBytes(evt->ip_info.gw, gIntfWifi.evtData.staGotIp.ipInfo.getway);
            IpToBytes(evt->ip_info.netmask, gIntfWifi.evtData.staGotIp.ipInfo.netmask);
            gIntfWifi.evtData.staGotIp.changed = evt->ip_changed;

            intf_wifi_EventCallback(INTF_WIFI_EVENT_STA_GOT_IP, &gIntfWifi.evtData);
            break;
        }
        case IP_EVENT_STA_LOST_IP:
        {
            INTF_WIFI_LOGD("IP_EVENT => IP_EVENT_STA_LOST_IP");
            intf_wifi_EventCallback(INTF_WIFI_EVENT_STA_LOST_IP, &gIntfWifi.evtData);
            break;
        }
        default:
            // ignored events
            break;
        }
    }
    else
    {
        // not used
    }
}