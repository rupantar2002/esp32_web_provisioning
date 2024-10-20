#ifndef __INTF_WIFI_H__
#define __INTF_WIFI_H__
#include <stdint.h>
#include <stdbool.h>

#define INTF_WIFI_SSID_LEN 32

#define INTF_WIFI_PASSWORD_LEN 64

#define INTF_WIFI_DEFAULT_SSID "ESP32"

#define INTF_WIFI_DEFAULT_CHANNEL 0

#define INTF_WIFI_MAX_CONNECTIONS 1

#define INTF_WIFI_MAX_CONNECTION_RETRY 1

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
    INTF_WIFI_EVENT_SCAN_COMPLETE,
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
        uint32_t reserved;
    } scanComplete;

} intf_wifi_EventData_t;

#define INTF_WIFI_IPV4(a, b, c, d) {(a), (b), (c), (d)}

intf_wifi_Status_t intf_wifi_Init(void);

intf_wifi_Status_t intf_wifi_SetIpInfo(intf_wifi_IpInfo_t *pIpInfo);

intf_wifi_Status_t intf_wifi_SetMode(intf_wifi_Mode_t mode);

intf_wifi_Status_t intf_wifi_SetCredentials(intf_wifi_Mode_t mode, intf_wifi_Cred_t *const pCred);

intf_wifi_Status_t intf_wifi_Start(void);

intf_wifi_Status_t intf_wifi_Stop(void);

bool intf_wifi_IsStaConnected(void);

intf_wifi_Status_t intf_wifi_Connect(void);

intf_wifi_Status_t intf_wifi_Disconnect(void);

intf_wifi_Status_t intf_wifi_StartScanning(void);

void intf_wifi_EventCallback(intf_wifi_Event_t event, intf_wifi_EventData_t const *const pData);

#endif //__INTF_WIFI_H__