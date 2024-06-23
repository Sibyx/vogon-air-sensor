/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <inttypes.h>
#include <esp_netif.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "esp_chip_info.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "shared.h"
#include "sensors.h"
#include "sync.h"

static const char* TAG = "MAIN_MODULE";

void app_main(void)
{
    ESP_LOGI(TAG, "Booting VogonAir...");

    // NVS flash for some reason (required by WiFi and MQTT)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Init TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Initialize the mutex
    data_mutex = xSemaphoreCreateMutex();
    if (data_mutex == NULL) {
        ESP_LOGE(TAG, "Unable to initialize shared memory mutex");
    }

    shared_data.humidity = 0;
    shared_data.temperature = 0;
    shared_data.pm25 = 0;
    shared_data.pm10 = 0;

    // Warm up - 10s
    vTaskDelay(pdMS_TO_TICKS(10 * 1000));

    xTaskCreatePinnedToCore(
            sync_task,
            "sync",
            configMINIMAL_STACK_SIZE * 8,
            NULL,
            10,
            NULL,
            PRO_CPU_NUM
    );

    xTaskCreatePinnedToCore(
            dht22_task,
            "dht22",
            configMINIMAL_STACK_SIZE * 8,
            NULL,
            10,
            NULL,
            APP_CPU_NUM
    );

    xTaskCreatePinnedToCore(
            sds011_task,
            "sds011",
            configMINIMAL_STACK_SIZE * 8,
            NULL,
            10,
            NULL,
            APP_CPU_NUM
    );

    ESP_LOGI(TAG, "All tasks are pinned!");
}
