/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "esp_chip_info.h"
#include "esp_log.h"
#include "shared.h"
#include "sensors.h"

static const char* TAG = "MAIN_MODULE";

void app_main(void)
{
    ESP_LOGI(TAG, "Booting VogonAir...");

    // Initialize the mutex
    data_mutex = xSemaphoreCreateMutex();
    if (data_mutex == NULL) {
        ESP_LOGE(TAG, "Unable to initialize shared memory mutex");
    }

    shared_data.humidity = 0;
    shared_data.temperature = 0;
    shared_data.pm25 = 0;
    shared_data.pm10 = 0;

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
