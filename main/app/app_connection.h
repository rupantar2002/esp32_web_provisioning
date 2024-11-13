#ifndef __APP_CONNECTION_H__
#define __APP_CONNECTION_H__

#include <stdint.h>
#include <stdbool.h>
#include "intf_wifi.h"

#define APP_CONNECTION_DEFAULT_INTERVAL 10U

#define APP_CONNECTION_DEFAULT_FACTOR 2U

#define APP_CONNECTION_DEFAULT_RETRY 4U

void app_connection_Init(void);

void app_connection_Start(void);

void app_connection_Stop(void);

#endif