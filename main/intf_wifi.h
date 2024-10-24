#ifndef __INTF_WIFI_H__
#define __INTF_WIFI_H__
#include <stdint.h>
#include <stdbool.h>

#define INTF_WIFI_SSID_LEN 32 /* Max SSID length*/

#define INTF_WIFI_PASSWORD_LEN 64 /* Max Password length */

#define INTF_WIFI_DEFAULT_SSID "ESP32" /* Default Ap ssid */

#define INTF_WIFI_DEFAULT_CHANNEL 0 /* Default channel */

#define INTF_WIFI_MAX_CONNECTIONS 1 /* Maximum number of connections */

#define INTF_WIFI_MAX_CONNECTION_RETRY 1 /* Maximum number of connection retry */

#define INTF_WIFI_SCAN_ENABLE 1 /* Enable scanning */

#define INTF_WIFI_SCAN_DISABLE 0 /* Disable scanning */

#define INTF_WIFI_SCAN_STATE INTF_WIFI_SCAN_ENABLE /* Select scanning state */

#if (INTF_WIFI_SCAN_STATE == INTF_WIFI_SCAN_ENABLE)

#define INTF_WIFI_SCAN_LIST_ALLOC_STATIC 0 /* Compiletime alocated scanlist */

#define INTF_WIFI_SCAN_LIST_ALLOC_DYNAMIC 1 /* Runtime alocated scanlist */

#define INTF_WIFI_SCAN_LIST_ALLOC_TYPE INTF_WIFI_SCAN_LIST_ALLOC_DYNAMIC /* Runtime alocated scanlist */

#if (INTF_WIFI_SCAN_LIST_ALLOC_TYPE == INTF_WIFI_SCAN_LIST_ALLOC_STATIC) /* Select scanlist type */

#define INTF_WIFI_SCAN_LIST_MAX_LENGTH 10 /* Select scanlist max length */

#endif

#endif

enum intf_wifi_Status
{
    INTF_WIFI_STATUS_OK = 0,
    INTF_WIFI_STATUS_ERROR,
    INTF_WIFI_STATUS_MAX,
};

enum intf_wifi_Mode
{
    INTF_WIFI_MODE_NULL = 0,
    INTF_WIFI_MODE_STA,
    INTF_WIFI_MODE_AP,
    INTF_WIFI_MODE_APSTA,
    INTF_WIFI_MODE_MAX,
};

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

typedef struct // TODO update accordingly
{
    uint8_t bssid[6]; /**< MAC address of AP */
    uint8_t ssid[33]; /**< SSID of AP */
    uint8_t primary;  /**< channel of AP */
    int reserved0;
    // wifi_second_chan_t second;          /**< secondary channel of AP */
    int8_t rssi; /**< signal strength of AP. Note that in some rare cases where signal strength is very strong, rssi values can be slightly positive */
    int authMode;
    // wifi_auth_mode_t authmode;          /**< authmode of AP */
    int reservpairwiseCiphered3;
    // wifi_cipher_type_t pairwise_cipher; /**< pairwise cipher of AP */
    int groupChipher;
    // wifi_cipher_type_t group_cipher; /**< group cipher of AP */
    int reserved1;
    // wifi_ant_t ant;                  /**< antenna used to receive beacon from AP */
    uint32_t reserved2 : 5;
    // uint32_t phy_11b : 1;            /**< bit: 0 flag to identify if 11b mode is enabled or not */
    // uint32_t phy_11g : 1;            /**< bit: 1 flag to identify if 11g mode is enabled or not */
    // uint32_t phy_11n : 1;            /**< bit: 2 flag to identify if 11n mode is enabled or not */
    // uint32_t phy_lr : 1;             /**< bit: 3 flag to identify if low rate is enabled or not */
    // uint32_t phy_11ax : 1;           /**< bit: 4 flag to identify if 11ax mode is enabled or not */
    uint32_t wps : 1; /**< bit: 5 flag to identify if WPS is supported or not */
    uint32_t reserved3 : 26;
    // uint32_t ftm_responder : 1;      /**< bit: 6 flag to identify if FTM is supported in responder mode */
    // uint32_t ftm_initiator : 1;      /**< bit: 7 flag to identify if FTM is supported in initiator mode */
    // uint32_t reserved : 24;          /**< bit: 8..31 reserved */
    uint8_t reserved4[12U];
    // wifi_country_t country;  /**< country information of AP */
    uint8_t reserved5[2U];
    // wifi_he_ap_info_t he_ap; /**< HE AP info */

} intf_wifi_ApRecord_t;

typedef enum
{
    INTF_WIFI_EVENT_NLL = 0,
    INTF_WIFI_EVENT_AP_STARTED,
    INTF_WIFI_EVENT_AP_STOPED,
    INTF_WIFI_EVENT_APSTA_CONNECTED,
    INTF_WIFI_EVENT_APSTA_DISCONNECTED,
    INTF_WIFI_EVENT_APSTA_GOT_IP,
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

typedef struct
{
    /* Access Point Events */
    struct
    {
        uint32_t reserved;
    } apStarted;

    struct
    {
        uint32_t reserved;
    } apStoped;

    struct
    {
        uint32_t reserved;
    } apStaConnected;

    struct
    {
        uint32_t reserved;
    } apStaDisconnected;

    /* Station Events */
    struct
    {
        uint32_t reserved;
    } staStarted;

    struct
    {
        uint32_t reserved;
    } staStoped;

    struct
    {
        uint32_t reserved;
    } staConnected;

    struct
    {
        uint32_t reserved;
    } staDisconnected;

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

#if (INTF_WIFI_SCAN_STATE == INTF_WIFI_SCAN_ENABLE)

intf_wifi_Status_t intf_wifi_StartScanning(bool block);

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