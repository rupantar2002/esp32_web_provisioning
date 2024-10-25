#ifndef __INTF_WIFI_H__
#define __INTF_WIFI_H__
#include <stdint.h>
#include <stdbool.h>

#define INTF_WIFI_SSID_LEN 32 /*!< Maximum ssid length */

#define INTF_WIFI_PASSWORD_LEN 64 /*!< Maximum password length */

#define INTF_WIFI_DEFAULT_SSID "ESP32" /*!< Default ssid */

#define INTF_WIFI_DEFAULT_CHANNEL 0 /*!< Default channel */

#define INTF_WIFI_MAX_CONNECTIONS 1 /*!< Maximum station connected to ap */

#define INTF_WIFI_MAX_CONNECTION_RETRY 1 /*!< Maximum connection retry */

#define INTF_WIFI_SCAN_ENABLE 1 /*!< Enable scaning */

#define INTF_WIFI_SCAN_DISABLE 0 /*!< Disable scanning */

#define INTF_WIFI_SCAN_STATE INTF_WIFI_SCAN_ENABLE /*!< Turns on or off scanning */

#if (INTF_WIFI_SCAN_STATE == INTF_WIFI_SCAN_ENABLE)

#define INTF_WIFI_SCAN_LIST_ALLOC_STATIC 0 /*!< Static scanlist */

#define INTF_WIFI_SCAN_LIST_ALLOC_DYNAMIC 1 /*!< Dynamic scanlist */

#define INTF_WIFI_SCAN_LIST_ALLOC_TYPE INTF_WIFI_SCAN_LIST_ALLOC_STATIC /*!< Set scanlist allocation mechanism */

#if (INTF_WIFI_SCAN_LIST_ALLOC_TYPE == INTF_WIFI_SCAN_LIST_ALLOC_STATIC)

#define INTF_WIFI_SCAN_LIST_MAX_LENGTH 10 /*!< Static scanlist length */

#endif // INTF_WIFI_SCAN_LIST_ALLOC_TYPE

#endif // INTF_WIFI_SCAN_STATE

/**
 * \brief Status code.
 *
 */
enum intf_wifi_Status
{
    INTF_WIFI_STATUS_OK = 0, /*!< Value indicate succes */
    INTF_WIFI_STATUS_ERROR,  /*!< Value indicate error */
    INTF_WIFI_STATUS_MAX,    /*!< Value indicate invalid status */
};

/**
 * \brief Wifi mode.
 *
 */
enum intf_wifi_Mode
{
    INTF_WIFI_MODE_NULL = 0, /*!< Value indicate no mode */
    INTF_WIFI_MODE_STA,      /*!< Value indicate Station mode */
    INTF_WIFI_MODE_AP,       /*!< Value indicate Ap mode */
    INTF_WIFI_MODE_APSTA,    /*!< Value indicate Ap + Station mode */
    INTF_WIFI_MODE_MAX,      /*!< Value indicate invalid mode */
};

/**
 * \brief WIfi authentication type.
 *
 */
typedef enum
{
    INTF_WIFI_AUTH_OPEN = 0,                                    /*!< Value indicat open authentication */
    INTF_WIFI_AUTH_WEP,                                         /*!< Value indicate WEP authentication */
    INTF_WIFI_AUTH_WPA_PSK,                                     /*!< Value indicate WPA password authentication */
    INTF_WIFI_AUTH_WPA2_PSK,                                    /*!< Value indicate WPA2 password authentication */
    INTF_WIFI_AUTH_WPA_WPA2_PSK,                                /*!< Value indicate WPA and WPA2 password authentication */
    INTF_WIFI_AUTH_ENTERPRISE,                                  /*!< Value indicate WPA2 enterprise authentication */
    INTF_WIFI_AUTH_WPA2_ENTERPRISE = INTF_WIFI_AUTH_ENTERPRISE, /*!< Value indicate WPA3 authentication */
    INTF_WIFI_AUTH_WPA3_PSK,                                    /*!< Value indicate  */
    INTF_WIFI_AUTH_WPA2_WPA3_PSK,                               /*!< Value indicate succes */
    INTF_WIFI_AUTH_WAPI_PSK,                                    /*!< Value indicate succes */
    INTF_WIFI_AUTH_OWE,                                         /*!< Value indicate succes */
    INTF_WIFI_AUTH_WPA3_ENT_192,                                /*!< Value indicate succes */
    INTF_WIFI_AUTH_WPA3_EXT_PSK,                                /*!< Value indicate succes */
    INTF_WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE,                     /*!< Value indicate succes */
    INTF_WIFI_AUTH_MAX                                          /*!< Value indicate succes */
} intf_wifi_AuthMode_t;

typedef uint8_t intf_wifi_Status_t;

typedef uint8_t intf_wifi_Mode_t;

typedef struct
{
    uint8_t ip[4];
    uint8_t netmask[4];
    uint8_t getway[4];
} intf_wifi_IpInfo_t;

typedef struct
{
    char ssid[INTF_WIFI_SSID_LEN + 2];
    char pass[INTF_WIFI_PASSWORD_LEN + 2];
} intf_wifi_Cred_t;

typedef struct
{
    uint8_t *ssid;   /**< SSID of AP */
    uint8_t *bssid;  /**< MAC address of AP */
    uint8_t channel; /**< channel, scan the specific channel */
    bool showHidden; /**< enable to scan AP whose SSID is hidden */
    bool passive;    /**< scan type, active or passive */
    struct
    {
        uint32_t min;
        uint32_t max;
    } activeTime;
    uint32_t passiveTime;
    uint8_t reserved;
} intf_wifi_ScanParams_t;

/**
 * \brief AP record.
 * \warning Byte aligned to wifi_ap_record_t.
 *
 */
typedef struct
{
    uint8_t bssid[6];
    uint8_t ssid[33];
    uint8_t primary;
    int reserved0;
    int8_t rssi;
    intf_wifi_AuthMode_t authMode;
    int pairwiseCipher;
    int groupChipher;
    int reserved1;
    uint32_t reserved2 : 5;
    uint32_t wps : 1;
    uint32_t reserved3 : 26;
    uint8_t reserved4[12U];
    uint8_t reserved5[2U];

} intf_wifi_ApRecord_t;

typedef enum
{
    INTF_WIFI_EVENT_NLL = 0,
    // Ap
    INTF_WIFI_EVENT_AP_STARTED,
    INTF_WIFI_EVENT_AP_STOPED,
    INTF_WIFI_EVENT_APSTA_CONNECTED,
    INTF_WIFI_EVENT_APSTA_DISCONNECTED,
    INTF_WIFI_EVENT_APSTA_GOT_IP,
    // Sta
    INTF_WIFI_EVENT_STA_START,
    INTF_WIFI_EVENT_STA_STOP,
    INTF_WIFI_EVENT_STA_CONNECTED,
    INTF_WIFI_EVENT_STA_DISCONNECTED,
    INTF_WIFI_EVENT_STA_GOT_IP,
    INTF_WIFI_EVENT_STA_LOST_IP,
    // scan
    INTF_WIFI_EVENT_SCAN_DONE,
    INTF_WIFI_EVENT_SCAN_LIST,
    INTF_WIFI_EVENT_MAX,
} intf_wifi_Event_t;

typedef union
{
    /* Ap Events */

    struct
    {
        uint8_t mac[6]; /**< MAC address of the station connected to Soft-AP */
        uint8_t aid;    /**< the aid that soft-AP gives to the station connected to  */
    } apStaConnected;

    struct
    {
        uint8_t mac[6]; /**< MAC address of the station disconnects to soft-AP */
        uint8_t aid;    /**< the aid that soft-AP gave to the station disconnects to  */
        uint8_t reason; /**< reason of disconnection */
    } apStaDisconnected;

    struct
    {
        uint8_t ip[4];  /*!< IP address which was assigned to the station */
        uint8_t mac[6]; /*!< MAC address of the connected client */
    } apStaGotIp;

    /* Station Events */
    struct
    {
        uint8_t ssid[INTF_WIFI_SSID_LEN + 2]; /**< SSID of connected AP */
        uint8_t ssidLen;                      /**< SSID length of connected AP */
        uint8_t bssid[6];                     /**< BSSID of connected AP*/
        intf_wifi_AuthMode_t authMode;        /**< authentication mode used by AP*/
        uint16_t aid;                         /**< authentication id assigned by the connected AP */
    } staConnected;

    struct
    {
        uint8_t ssid[INTF_WIFI_SSID_LEN + 2]; /**< SSID of disconnected AP */
        uint8_t ssidLen;                      /**< SSID length of disconnected AP */
        uint8_t bssid[6];                     /**< BSSID of disconnected AP */
        uint8_t reason;                       /**< reason of disconnection */
    } staDisconnected;

    struct
    {
        intf_wifi_IpInfo_t ipInfo; /*!< IP address, netmask, gatway IP address */
        bool changed;              /*!< Whether the assigned IP has changed or not */
    } staGotIp;

    /* Scanning Events*/
    struct
    {
        bool status;
        uint16_t count;
    } scanDone;

    struct
    {
        uint16_t count;
        intf_wifi_ApRecord_t *records;
    } scanList;

} intf_wifi_EventData_t;

#define INTF_WIFI_IPV4(a, b, c, d) {(a), (b), (c), (d)}

intf_wifi_Status_t intf_wifi_Init(void);

intf_wifi_Status_t intf_wifi_SetIpInfo(intf_wifi_IpInfo_t *pIpInfo);

intf_wifi_Status_t intf_wifi_SetMode(intf_wifi_Mode_t mode);

intf_wifi_Status_t intf_wifi_SetCredentials(intf_wifi_Mode_t mode,
                                            intf_wifi_Cred_t *const pCred);

intf_wifi_Status_t intf_wifi_Start(void);

intf_wifi_Status_t intf_wifi_Stop(void);

bool intf_wifi_IsStaConnected(void);

intf_wifi_Status_t intf_wifi_Connect(void);

intf_wifi_Status_t intf_wifi_Disconnect(void);

void intf_wifi_DeInit(void);

#if (INTF_WIFI_SCAN_STATE == INTF_WIFI_SCAN_ENABLE)

intf_wifi_Status_t intf_wifi_StartScanning(intf_wifi_ScanParams_t *pParams,
                                           bool block);

intf_wifi_Status_t intf_wifi_StopScanning(void);

intf_wifi_Status_t intf_wifi_GetScanList(const intf_wifi_ApRecord_t **records,
                                         uint16_t *count);

#if (INTF_WIFI_SCAN_LIST_ALLOC_TYPE == INTF_WIFI_SCAN_LIST_ALLOC_DYNAMIC)

intf_wifi_Status_t intf_wifi_CreateScanList(uint16_t count);

void intf_wifi_DestroyScanList(void);

#endif // INTF_WIFI_SCAN_LIST_ALLOC_TYPE

#endif // INTF_WIFI_SCAN_STATE

void intf_wifi_EventCallback(intf_wifi_Event_t event,
                             intf_wifi_EventData_t const *const pData);

#endif //__INTF_WIFI_H__