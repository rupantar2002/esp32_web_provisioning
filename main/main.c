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

const char *GetAuthModeName(intf_wifi_AuthMode_t mode)
{
    switch (mode)
    {
    case INTF_WIFI_AUTH_OPEN:
        return "INTF_WIFI_AUTH_OPEN";
    case INTF_WIFI_AUTH_WEP:
        return "INTF_WIFI_AUTH_WEP";
    case INTF_WIFI_AUTH_WPA_PSK:
        return "INTF_WIFI_AUTH_WPA_PSK";
    case INTF_WIFI_AUTH_WPA2_PSK:
        return "INTF_WIFI_AUTH_WPA2_PSK";
    case INTF_WIFI_AUTH_WPA_WPA2_PSK:
        return "INTF_WIFI_AUTH_WPA_WPA2_PSK";
    case INTF_WIFI_AUTH_ENTERPRISE:
        return "INTF_WIFI_AUTH_ENTERPRISE";
    case INTF_WIFI_AUTH_WPA3_PSK:
        return "INTF_WIFI_AUTH_WPA3_PSK";
    case INTF_WIFI_AUTH_WPA2_WPA3_PSK:
        return "INTF_WIFI_AUTH_WPA2_WPA3_PSK";
    case INTF_WIFI_AUTH_WAPI_PSK:
        return "INTF_WIFI_AUTH_WAPI_PSK";
    case INTF_WIFI_AUTH_OWE:
        return "INTF_WIFI_AUTH_OWE";
    case INTF_WIFI_AUTH_WPA3_ENT_192:
        return "INTF_WIFI_AUTH_WPA3_ENT_192";
    case INTF_WIFI_AUTH_WPA3_EXT_PSK:
        return "INTF_WIFI_AUTH_WPA3_EXT_PSK";
    case INTF_WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE:
        return "INTF_WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE";
    default:
        return "Unknown Authentication Mode";
    }
}

static void Scan(bool block)
{

    if (intf_wifi_StartScanning(NULL, block) == INTF_WIFI_STATUS_OK)
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

    intf_wifi_SetMode(INTF_WIFI_MODE_APSTA);

    // intf_wifi_SetIpInfo(&ipInfo);

    intf_wifi_Start();

    intf_wifi_SetCredentials(INTF_WIFI_MODE_STA, &sta);

    intf_wifi_Connect();

    Scan(false);

    Scan(true);

    // if (intf_wifi_StartScanning(true) == INTF_WIFI_STATUS_OK)
    // {
    //     ESP_LOGI(TAG, "SCAN DONE");
    //     const intf_wifi_ApRecord_t *apList = NULL; // This will hold the list of APs
    //     uint16_t apCount = 0;                      // This will hold the number of APs

    //     // Call the function to get the scan list
    //     intf_wifi_Status_t status = intf_wifi_GetScanList(&apList, &apCount);

    //     // Check if the scan was successful
    //     if (status == INTF_WIFI_STATUS_OK)
    //     {
    //         ESP_LOGI(TAG, "Number of APs found: %u", apCount);

    //         // Loop through and print each AP's info
    //         for (uint16_t i = 0; i < apCount; i++)
    //         {
    //             ESP_LOGI(TAG, "AP %u :{ ssid : \"%s\", rssi: %d }", i, apList[i].ssid, apList[i].rssi);
    //         }
    //     }
    //     else
    //     {
    //         ESP_LOGE(TAG, "Failed to get the AP list.");
    //     }
    // }
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
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_APSTA_CONNECTED", __LINE__, __func__);
        break;
    case INTF_WIFI_EVENT_APSTA_DISCONNECTED:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_APSTA_DISCONNECTED", __LINE__, __func__);
        break;
    case INTF_WIFI_EVENT_APSTA_GOT_IP:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_APSTA_GOT_IP", __LINE__, __func__);
        ESP_LOGI(TAG, "mac : " MACSTR, MAC2STR(pData->apStaGotIp.mac));
        ESP_LOGI(TAG, "ip : " IPSTR, pData->apStaGotIp.ip[0],
                 pData->apStaGotIp.ip[1],
                 pData->apStaGotIp.ip[2],
                 pData->apStaGotIp.ip[3]);
        break;
    case INTF_WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_AP_STARTED", __LINE__, __func__);
        break;
    case INTF_WIFI_EVENT_STA_STOP:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_AP_STARTED", __LINE__, __func__);
        break;
    case INTF_WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_STA_CONNECTED", __LINE__, __func__);
        ESP_LOGI(TAG, "ssid : \"%s\"", pData->staConnected.ssid);
        ESP_LOGI(TAG, "authmode : \"%s\"", GetAuthModeName(pData->staConnected.authMode));
        ESP_LOGI(TAG, "aid : \"%d\"", pData->staConnected.aid);

        break;
    case INTF_WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_STA_DISCONNECTED", __LINE__, __func__);
        ESP_LOGI(TAG, "ssid : \"%s\"", pData->staDisconnected.ssid);
        ESP_LOGI(TAG, "reason : \"%d\"", pData->staDisconnected.reason);

        break;
    case INTF_WIFI_EVENT_STA_GOT_IP:
        break;
    case INTF_WIFI_EVENT_STA_LOST_IP:
        break;

    case INTF_WIFI_EVENT_SCAN_DONE:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_SCAN_DONE", __LINE__, __func__);
        ESP_LOGI(TAG, "status : %d", pData->scanDone.status);
        ESP_LOGI(TAG, "Aps found : %d", pData->scanDone.count);

        if (pData->scanDone.status)
        {
            intf_wifi_CreateScanList(pData->scanDone.count);
        }
        break;
    case INTF_WIFI_EVENT_SCAN_LIST:
        ESP_LOGI(TAG, " %d : %s : INTF_WIFI_EVENT_SCAN_LIST", __LINE__, __func__);
        if (pData->scanList.count > 0)
        {
            for (uint16_t i = 0; i < pData->scanList.count; i++) // TODO remove
            {
                ESP_LOGI(TAG, "\n");
                ESP_LOGI(TAG, "ssid : \"%s\"", pData->scanList.records[i].ssid);
                ESP_LOGI(TAG, "rssi : %d", pData->scanList.records[i].rssi);
                ESP_LOGI(TAG, "primary : %d", pData->scanList.records[i].primary);
                ESP_LOGI(TAG, "wps : %d", pData->scanList.records[i].wps);
                ESP_LOGI(TAG, "\n");
            }
        }
        intf_wifi_DestroyScanList();
        break;

    case INTF_WIFI_EVENT_MAX:
        break;
    default:
        break;
    }
}