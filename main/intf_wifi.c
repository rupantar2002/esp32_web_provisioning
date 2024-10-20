#include <string.h>
#include <esp_mac.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_log.h>
#include <esp_err.h>
#include "intf_wifi.h"

#define INTF_WIFI_PRINT_ERR(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)

#define INTF_WIFI_PRINT(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)

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
    // intf_wifi_Cred_t apCred;  // TODO may not need to store localy
    // intf_wifi_Cred_t staCred; // TODO may not need to store localy
    esp_netif_t *apNetif;
    esp_netif_t *staNetif;

} intf_wifi_Ctx_t;

static const char *TAG = "INTF_WIFI";

static intf_wifi_Ctx_t gIntfWifi = {0};

static void WifiEventHandler(void *arg,
                             esp_event_base_t event_base,
                             int32_t event_id,
                             void *event_data);

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

static bool SetWifiMode(void)
{
    if (esp_wifi_set_mode(gIntfWifi.mode) != ESP_OK)
    {
        INTF_WIFI_PRINT_ERR(" %d : FAILED TO SET MODE", __LINE__);
        return false;
    }
    else
    {
        return true;
    }
}

/* Initialize soft AP */
static bool ConfigureInterface(wifi_interface_t intf, intf_wifi_Cred_t *const pCred)
{

    wifi_config_t wifiCfg = {0};

    if (esp_wifi_get_config(intf, &wifiCfg) != ESP_OK)
    {
        INTF_WIFI_PRINT_ERR(" %d : FAILED TO GET INTERFACE [%d]", __LINE__, intf);
        return false;
    }

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
        wifiCfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifiCfg.sta.pmf_cfg.capable = true;
        wifiCfg.sta.pmf_cfg.required = false;
        wifiCfg.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        break;

    default:
        INTF_WIFI_PRINT_ERR(" %d : INVALID INTERFACE [%d]", __LINE__, intf);
        return false;
        break;
    }

    if (esp_wifi_set_config(intf, &wifiCfg) != ESP_OK)
    {
        INTF_WIFI_PRINT_ERR(" %d : FAILED TO CONFIGURE INTERFACE [%d]", __LINE__, intf);
        return false;
    }
    else
    {
        return true;
    }
}

intf_wifi_Status_t intf_wifi_Init(void)
{
    /* Register Event handler */
    if (esp_event_handler_instance_register(WIFI_EVENT,
                                            ESP_EVENT_ANY_ID,
                                            &WifiEventHandler,
                                            NULL,
                                            NULL) != ESP_OK)
    {
        INTF_WIFI_PRINT_ERR(" %d : FAILED TO REGISTER WIFI EVENT", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    if (esp_event_handler_instance_register(IP_EVENT,
                                            ESP_EVENT_ANY_ID,
                                            &WifiEventHandler,
                                            NULL,
                                            NULL) != ESP_OK)
    {
        INTF_WIFI_PRINT_ERR(" %d : FAILED TO REGISTER IP EVENT", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    /*Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    intf_wifi_Cred_t tempCred;
    esp_err_t errCode = ESP_OK;

    if (esp_wifi_init(&cfg) != ESP_OK)
    {
        INTF_WIFI_PRINT_ERR(" %d : FAILED TO INIT WIFI", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    gIntfWifi.apNetif = esp_netif_create_default_wifi_ap();
    if (gIntfWifi.apNetif == NULL)
    {
        INTF_WIFI_PRINT_ERR(" %d : FAILED TO CREATE NETIF AP", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    gIntfWifi.staNetif = esp_netif_create_default_wifi_sta();
    if (gIntfWifi.staNetif == NULL)
    {
        INTF_WIFI_PRINT_ERR(" %d : FAILED TO CREATE NETIF STA", __LINE__);
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
        INTF_WIFI_PRINT_ERR(" %d : FAILED TO SET DEFAULT INTERFACE", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }
    else
    {
        INTF_WIFI_PRINT("WIFI INITIALIZED");
        return INTF_WIFI_STATUS_OK;
    }
}

intf_wifi_Status_t intf_wifi_SetIpInfo(intf_wifi_IpInfo_t *pIpInfo)
{
    if (pIpInfo == NULL)
    {
        INTF_WIFI_PRINT_ERR(" %d : INVALID IPINFO", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    if (gIntfWifi.mode == WIFI_MODE_AP || gIntfWifi.mode == WIFI_MODE_APSTA)
    {
        esp_netif_ip_info_t ipInfo;

        if (esp_netif_dhcps_stop(gIntfWifi.apNetif) != ESP_OK)
        {
            INTF_WIFI_PRINT_ERR(" %d : FAILED TO STOP DHCPS", __LINE__);
            return INTF_WIFI_STATUS_ERROR;
        }

        ipInfo.ip.addr = ESP_IP4TOADDR(pIpInfo->ip[0], pIpInfo->ip[1], pIpInfo->ip[2], pIpInfo->ip[3]);
        ipInfo.netmask.addr = ESP_IP4TOADDR(pIpInfo->netmask[0], pIpInfo->netmask[1], pIpInfo->netmask[2], pIpInfo->netmask[3]);
        ipInfo.gw.addr = ESP_IP4TOADDR(pIpInfo->getway[0], pIpInfo->getway[1], pIpInfo->getway[2], pIpInfo->getway[3]);

        if (esp_netif_set_ip_info(gIntfWifi.apNetif, &ipInfo) != ESP_OK)
        {
            INTF_WIFI_PRINT_ERR(" %d : FAILED TO SET IP INFO", __LINE__);
            return INTF_WIFI_STATUS_ERROR;
        }

        if (esp_netif_dhcps_start(gIntfWifi.apNetif) != ESP_OK)
        {
            INTF_WIFI_PRINT_ERR(" %d : FAILED TO START DHCPS", __LINE__);
            return INTF_WIFI_STATUS_ERROR;
        }
        return INTF_WIFI_STATUS_OK;
    }
    else
    {
        INTF_WIFI_PRINT_ERR(" %d : MODE SHOULD BE INTF_WIFI_MODE_AP or INTF_WIFI_MODE_APSTA", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }
}

intf_wifi_Status_t intf_wifi_SetMode(intf_wifi_Mode_t mode)
{

    if (mode >= INTF_WIFI_MODE_MAX && mode <= INTF_WIFI_MODE_NULL)
    {
        INTF_WIFI_PRINT_ERR(" %d : INVALID MODE", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    gIntfWifi.mode = ConvertToMode(mode);
    if (!SetWifiMode())
    {
        return INTF_WIFI_STATUS_ERROR;
    }
    else
    {
        INTF_WIFI_PRINT("WIFI MODE SET TO => \"%s\"", GetModeStr(mode));
        return INTF_WIFI_STATUS_OK;
    }
}

intf_wifi_Status_t intf_wifi_SetCredentials(intf_wifi_Mode_t mode,
                                            intf_wifi_Cred_t *const pCred)
{
    if (pCred == NULL)
    {
        INTF_WIFI_PRINT_ERR(" %d : INVALID CREDENTIALS", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    if (mode >= INTF_WIFI_MODE_MAX && mode <= INTF_WIFI_MODE_NULL)
    {
        INTF_WIFI_PRINT_ERR(" %d : INVALID MODE", __LINE__);
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
        INTF_WIFI_PRINT("\"%s\" CREDENTIALS SET", GetModeStr(mode));
        return INTF_WIFI_STATUS_OK;
    }
    else
    {
        INTF_WIFI_PRINT_ERR(" %d : FAILED TO CONFIGURE INTERFACE", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }
}

intf_wifi_Status_t intf_wifi_Start(void)
{

    if (esp_wifi_start() != ESP_OK)
    {
        INTF_WIFI_PRINT_ERR(" %d : FAILED TO START WIFI", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    return INTF_WIFI_STATUS_OK;
}

intf_wifi_Status_t intf_wifi_Stop(void)
{
    if (gIntfWifi.flags.staConn)
    {
        (void)esp_wifi_disconnect();
    }

    if (esp_wifi_stop() != ESP_OK)
    {
        INTF_WIFI_PRINT_ERR(" %d : FAILED TO STOP WIFI", __LINE__);
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
    if (gIntfWifi.flags.staConn)
    {
        if (esp_wifi_connect() != ESP_OK)
        {
            INTF_WIFI_PRINT_ERR(" %d : FAILED TO CONNECT TO REMOTE AP", __LINE__);
            return INTF_WIFI_STATUS_ERROR;
        }
        return INTF_WIFI_STATUS_OK;
    }
    else
    {
        INTF_WIFI_PRINT_ERR(" %d : DISCONNECT FIRST", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }

    return INTF_WIFI_STATUS_OK;
}

intf_wifi_Status_t intf_wifi_Disconnect(void)
{
    if (!gIntfWifi.flags.staConn)
    {
        if (esp_wifi_disconnect() != ESP_OK)
        {
            INTF_WIFI_PRINT_ERR(" %d : FAILED TO DISCONNECT FROM REMOTE AP", __LINE__);
            return INTF_WIFI_STATUS_ERROR;
        }
        return INTF_WIFI_STATUS_OK;
    }
    else
    {
        INTF_WIFI_PRINT_ERR(" %d : NOT CONNECTED", __LINE__);
        return INTF_WIFI_STATUS_ERROR;
    }
}

intf_wifi_Status_t intf_wifi_StartScanning(void)
{
        esp_wifi_scan_start(NULL, true);

}

__attribute__((__weak__)) void intf_wifi_EventCallback(intf_wifi_Event_t event,
                                                       intf_wifi_EventData_t const *const pData)
{
    (void)event;
    (void)pData;
}

static void WifiEventHandler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        // AP
        case WIFI_EVENT_AP_START:
            INTF_WIFI_PRINT("WIFI_EVENT => WIFI_EVENT_AP_START");
            break;
        case WIFI_EVENT_AP_STOP:
            INTF_WIFI_PRINT("WIFI_EVENT => WIFI_EVENT_AP_STOP");

            break;
        case WIFI_EVENT_AP_STACONNECTED:
            INTF_WIFI_PRINT("WIFI_EVENT => WIFI_EVENT_AP_STACONNECTED");

            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            INTF_WIFI_PRINT("WIFI_EVENT => WIFI_EVENT_AP_STADISCONNECTED");

            break;
            // STA
        case WIFI_EVENT_STA_START:
            INTF_WIFI_PRINT("WIFI_EVENT => WIFI_EVENT_STA_START");

            break;
        case WIFI_EVENT_STA_STOP:
            INTF_WIFI_PRINT("WIFI_EVENT => WIFI_EVENT_STA_STOP");

            break;
        case WIFI_EVENT_STA_CONNECTED:
            INTF_WIFI_PRINT("WIFI_EVENT => WIFI_EVENT_STA_CONNECTED");

            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            INTF_WIFI_PRINT("WIFI_EVENT => WIFI_EVENT_STA_DISCONNECTED");

            break;
        // Scan
        case WIFI_EVENT_SCAN_DONE:
            INTF_WIFI_PRINT("WIFI_EVENT => WIFI_EVENT_SCAN_DONE");

            break;
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
            INTF_WIFI_PRINT("IP_EVENT => IP_EVENT_AP_STAIPASSIGNED");

            break;
        // STA
        case IP_EVENT_STA_GOT_IP:
            INTF_WIFI_PRINT("IP_EVENT => IP_EVENT_STA_GOT_IP");

            break;
        case IP_EVENT_STA_LOST_IP:
            INTF_WIFI_PRINT("IP_EVENT => IP_EVENT_STA_LOST_IP");

            break;
        default:
            // ignored events
            break;
        }
    }
    else
    {
        // not used
    }

    // if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    // {
    //     wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
    //     ESP_LOGI(TAG, "Station " MACSTR " joined, AID=%d",
    //              MAC2STR(event->mac), event->aid);
    // }
    // else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    // {
    //     wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    //     ESP_LOGI(TAG, "Station " MACSTR " left, AID=%d",
    //              MAC2STR(event->mac), event->aid);
    // }
    // else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    // {
    //     esp_wifi_connect();
    //     ESP_LOGI(TAG, "Station started");
    // }
    // else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    // {
    //     ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    //     ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
    //     // s_retry_num = 0;
    //     // xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    // }
}