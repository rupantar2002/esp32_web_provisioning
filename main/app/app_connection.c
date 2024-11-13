#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_log.h>
#include "app_connection.h"

#define MAX_INTERVAL (APP_CONNECTION_DEFAULT_INTERVAL * APP_CONNECTION_DEFAULT_RETRY)

typedef struct
{
    enum
    {
        STATE_NULL = 0,
        STATE_IDLE,
        STATE_START_RETRY,
        STATE_INCREMENT_INTERVAL,
        STATE_RETRY_WITH_MAX_INTERVAL,
        STATE_STOP,
        STATE_MAX,
    } state;
    uint8_t retryCount;
    uint16_t interval;
    uint8_t factor;
    uint8_t maxRetryCount;
    TimerHandle_t timer;
} app_connection_Ctx_t;

static const char *TAG = "APP_CONNECTION";

static app_connection_Ctx_t gCtx = {0};

static void HandleState()
{
    switch (gCtx.state)
    {
    case STATE_START_RETRY:
        ESP_LOGW(TAG, "STATE_START_RETRY");
        intf_wifi_Connect();
        gCtx.retryCount = 1;
        gCtx.state = STATE_INCREMENT_INTERVAL;
        break;

    case STATE_INCREMENT_INTERVAL:
        ESP_LOGW(TAG, "STATE_INCREMENT_INTERVAL");
        if (gCtx.retryCount < gCtx.maxRetryCount)
        {
            gCtx.interval *= gCtx.factor;
            if (gCtx.interval > MAX_INTERVAL)
            {
                gCtx.interval = MAX_INTERVAL;
            }
            intf_wifi_Connect();
            gCtx.retryCount++;
            (void)xTimerChangePeriod(gCtx.timer, pdMS_TO_TICKS(gCtx.interval * 1000), 0);
        }
        else
        {
            gCtx.state = STATE_RETRY_WITH_MAX_INTERVAL;
        }
        break;

    case STATE_RETRY_WITH_MAX_INTERVAL:
        ESP_LOGW(TAG, "STATE_RETRY_WITH_MAX_INTERVAL");
        // Repeatedly attempt reconnect with the max gCtx.interval
        intf_wifi_Connect();
        (void)xTimerChangePeriod(gCtx.timer, pdMS_TO_TICKS(gCtx.interval * 1000), 0);
        break;

    case STATE_STOP:
        ESP_LOGW(TAG, "STATE_STOP");
        xTimerStop(gCtx.timer, 0);
        gCtx.state = STATE_IDLE;
        gCtx.retryCount = 0;
        gCtx.interval = 1000;
        break;

    default:
        break;
    }
}

static void TimerCallback(TimerHandle_t timer)
{
    HandleState();
}

void app_connection_Init(void)
{
    if (gCtx.timer == NULL)
    {
        gCtx.state = STATE_IDLE;
        gCtx.retryCount = 0;
        gCtx.interval = APP_CONNECTION_DEFAULT_INTERVAL;
        gCtx.factor = APP_CONNECTION_DEFAULT_FACTOR;
        gCtx.maxRetryCount = APP_CONNECTION_DEFAULT_RETRY;

        gCtx.timer = xTimerCreate("connection_tmr",
                                  pdMS_TO_TICKS(gCtx.interval * 1000U),
                                  pdFALSE,
                                  NULL,
                                  &TimerCallback);
        configASSERT(gCtx.timer);
    }
}

void app_connection_Start(void)
{
    if (gCtx.state == STATE_IDLE && gCtx.timer)
    {
        gCtx.state = STATE_START_RETRY;
        gCtx.interval = APP_CONNECTION_DEFAULT_INTERVAL;
        gCtx.retryCount = 0;
        xTimerStart(gCtx.timer, 0);
    }
}

// Stop the reconnection process
void app_connection_Stop()
{
    if (gCtx.state != STATE_IDLE && gCtx.timer)
    {
        gCtx.state = STATE_STOP;
        HandleState();
    }
}