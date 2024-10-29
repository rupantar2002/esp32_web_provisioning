#ifndef __SERVICE_WEBSERVER_H__
#define __SERVICE_WEBSERVER_H__
#include "service.h"

#define SERVICE_WEBSERVER_USE_WEBSOCKET 1

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)

#define SERVICE_WEBSERVER_WS_MAX_BUFFER 256U

#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

typedef enum
{
    SERVICE_WEBSERVER_EVENT_NULL = 0,
    SERVICE_WEBSERVER_EVENT_USER,
    
    /* Websocket events */
#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)
    SERVICE_WEBSERVER_EVENT_SOCKET_CONN,
    SERVICE_WEBSERVER_EVENT_SOCKET_DATA,
#endif // SERVICE_WEBSERVER_USE_WEBSOCKET
    SERVICE_WEBSERVER_EVENT_MAX,
} service_webserver_Event_t;

typedef struct
{
    struct
    {
        uint32_t reserved;
    } user;

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)
    struct
    {
        bool connected;
    } socketConn;

    struct
    {
        char *data;
        uint16_t len;
    } socketData;

#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

} service_webserver_EventData_t;

service_Status_t
service_webserver_Start(void);

service_Status_t service_webserver_Stop(void);

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)

bool service_webserver_IsSocketConnected(void);

service_Status_t service_webserver_Send(const char *msg, uint16_t len);

#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

service_Status_t service_webserver_EventCallback(service_webserver_Event_t event,
                                                 service_webserver_EventData_t const *const pData);

#endif //__SERVICE_WEBSERVER_H__