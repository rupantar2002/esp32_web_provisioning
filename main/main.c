/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/*  WiFi softAP & station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_log.h>
#include <esp_err.h>
#include "intf_wifi.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    intf_wifi_Cred_t sta = {
        .ssid = "DESKTOP-762IJ28 3231",
        .pass = "12345678",
    };

    intf_wifi_IpInfo_t ipInfo = {
        .ip = INTF_WIFI_IPV4(10, 10, 10, 10),
        .getway = INTF_WIFI_IPV4(10, 10, 10, 10),
        .netmask = INTF_WIFI_IPV4(255, 255, 255, 0),
    };

    intf_wifi_Init();

    intf_wifi_CreateScanList(10U);

    intf_wifi_SetMode(INTF_WIFI_MODE_APSTA);

    // intf_wifi_SetIpInfo(&ipInfo);

    intf_wifi_Start();

    // intf_wifi_SetCredentials(INTF_WIFI_MODE_STA, &sta);

    // intf_wifi_Connect();

    if (intf_wifi_StartScanning(true) == INTF_WIFI_STATUS_OK)
    {
        ESP_LOGI(TAG, "SCAN DONE");
        const intf_wifi_ApRecord_t *apList = NULL; // This will hold the list of APs
        uint16_t apCount = 0;                      // This will hold the number of APs

        // Call the function to get the scan list
        intf_wifi_Status_t status = intf_wifi_GetScanList(&apList, &apCount);

        // Check if the scan was successful
        if (status == INTF_WIFI_STATUS_OK)
        {
            ESP_LOGI(TAG, "Number of APs found: %u", apCount);

            // Loop through and print each AP's info
            for (uint16_t i = 0; i < apCount; i++)
            {
                ESP_LOGI(TAG, "AP %u :{ ssid : \"%s\", rssi: %d }", i, apList[i].ssid, apList[i].rssi);
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to get the AP list.");
        }
    }

    intf_wifi_DestroyScanList();

    if (intf_wifi_StartScanning(true) == INTF_WIFI_STATUS_OK)
    {
        ESP_LOGI(TAG, "SCAN DONE");
        const intf_wifi_ApRecord_t *apList = NULL; // This will hold the list of APs
        uint16_t apCount = 0;                      // This will hold the number of APs

        // Call the function to get the scan list
        intf_wifi_Status_t status = intf_wifi_GetScanList(&apList, &apCount);

        // Check if the scan was successful
        if (status == INTF_WIFI_STATUS_OK)
        {
            ESP_LOGI(TAG, "Number of APs found: %u", apCount);

            // Loop through and print each AP's info
            for (uint16_t i = 0; i < apCount; i++)
            {
                ESP_LOGI(TAG, "AP %u :{ ssid : \"%s\", rssi: %d }", i, apList[i].ssid, apList[i].rssi);
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to get the AP list.");
        }
    }
}

void intf_wifi_EventCallback(intf_wifi_Event_t event,
                             intf_wifi_EventData_t const *const pData)
{

    switch (event)
    {
    case INTF_WIFI_EVENT_AP_STARTED:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_AP_STARTED", __LINE__, __func__);
        break;
    case INTF_WIFI_EVENT_AP_STOPED:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_AP_STOPED", __LINE__, __func__);
        break;
    case INTF_WIFI_EVENT_APSTA_CONNECTED:
        break;
    case INTF_WIFI_EVENT_APSTA_DISCONNECTED:
        break;
    case INTF_WIFI_EVENT_APSTA_GOT_IP:
        break;
    case INTF_WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_AP_STARTED", __LINE__, __func__);
        break;
    case INTF_WIFI_EVENT_STA_STOP:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_AP_STARTED", __LINE__, __func__);
        break;
    case INTF_WIFI_EVENT_STA_CONNECTED:
        break;
    case INTF_WIFI_EVENT_STA_DISCONNECTED:
        break;
    case INTF_WIFI_EVENT_STA_GOT_IP:
        break;
    case INTF_WIFI_EVENT_STA_LOST_IP:
        break;
    case INTF_WIFI_EVENT_SCAN_COMPLETE:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_SCAN_COMPLETE", __LINE__, __func__);
        ESP_LOGI(TAG, "count : %d", pData->scanComplete.count);
        for (int i = 0; i < pData->scanComplete.count; i++) // TODO remove
        {
            ESP_LOGI(TAG, "\n");
            ESP_LOGI(TAG, "ssid : \"%s\"", pData->scanComplete.records[i].ssid);
            ESP_LOGI(TAG, "rssi : %d", pData->scanComplete.records[i].rssi);
            ESP_LOGI(TAG, "primary : %d", pData->scanComplete.records[i].primary);
            ESP_LOGI(TAG, "wps : %d", pData->scanComplete.records[i].wps);
        }
        break;
    case INTF_WIFI_EVENT_MAX:
        break;
    default:
        break;
    }
}