#ifndef __SERVICE_WEBSERVER_H__
#define __SERVICE_WEBSERVER_H__

#include <stdbool.h>
#include "service.h"

#define SERVICE_WEBSERVER_USE_WEBSOCKET 1

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)

#define SERVICE_WEBSERVER_RX_BUFFER 256U
#define SERVICE_WEBSERVER_TX_BUFFER 512U

#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

#define SERVICE_WEBSERVER_USE_BASIC_AUTH 1

#if (SERVICE_WEBSERVER_USE_BASIC_AUTH == 1)

#define SERVICE_WEBSERVER_MAX_USERNAME 24U
#define SERVICE_WEBSERVER_MAX_PASSWORD 24U

#define SESERVICE_WEBSERVER_DEFAULT_USERNAME "admin"
#define SESERVICE_WEBSERVER_DEFAULT_PASSWORD "admin"

#endif // SERVICE_WEBSERVER_USE_BASIC_AUTH

typedef enum
{
    SERVICE_WEBSERVER_EVENT_NULL = 0,
/* Websocket events */
#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)
    SERVICE_WEBSERVER_EVENT_SOCKET_CONN,
#endif                            // SERVICE_WEBSERVER_USE_WEBSOCKET
    SERVICE_WEBSERVER_EVENT_USER, /* event for user */
} service_webserver_Event_t;

typedef struct
{
    uint16_t parent; /* TODO applcation should set this value to 1 which indicate it is implemented */
    uint8_t *data;
    uint16_t len;
} service_webserver_UserBase_t;

typedef union
{

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)
    struct
    {
        bool connected;
    } socketConn;
#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

    service_webserver_UserBase_t userBase; /* Generic user event.*/

} service_webserver_EventData_t;

service_Status_t service_webserver_Start(void);

service_Status_t service_webserver_Stop(void);

#if (SERVICE_WEBSERVER_USE_BASIC_AUTH == 1)

service_Status_t service_webserver_SetAuth(const char *username,
                                           const char *password);

#endif // SERVICE_WEBSERVER_USE_BASIC_AUTH

#if (SERVICE_WEBSERVER_USE_WEBSOCKET == 1)

bool service_webserver_IsSocketConnected(void);

service_Status_t service_webserver_Send(const char *msg, uint16_t len);

service_Status_t service_webserver_SendAsync(const char *msg, uint16_t len);

#endif // SERVICE_WEBSERVER_USE_WEBSOCKET

void service_webserver_EventCallback(service_webserver_Event_t event,
                                                 service_webserver_EventData_t const *const pData);

const service_webserver_UserBase_t *service_webserver_ParseUserData(service_webserver_UserBase_t const *const src);

#endif //__SERVICE_WEBSERVER_H__