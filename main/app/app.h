#ifndef __APP_H__
#define __APP_H__

#include <stdint.h>

enum app_Status
{
    APP_STATUS_OK = 0,
    APP_STATUS_ERROR,
    APP_STATUS_MAX,
};

typedef uint8_t app_Status_t;

#endif //__APP_H__