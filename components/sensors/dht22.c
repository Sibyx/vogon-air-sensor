#include "dht.h"
#include "shared.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>

static const char* TAG = "MODULE_DHT22";

_Noreturn void dht22_task(void *pvParameters) {
    float temperature = 0, humidity = 0;

    while (1) {
        if (dht_read_float_data(DHT_TYPE_AM2301, CONFIG_SENSORS_DHT22_PIN, &humidity, &temperature) != ESP_OK)
        {
            ESP_LOGW(TAG, "Temperature/humidity reading failed");
            continue;
        }

        ESP_LOGI(TAG, "Measurements: temperature=%.2fC, humidity=%.2f%%", temperature, humidity);

        // Store measurements to shared memory
        if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) {
            shared_data.temperature = temperature;
            shared_data.humidity = humidity;

            xSemaphoreGive(data_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_SENSORS_DHT22_SLEEP_PERIOD * 60 * 1000));
    }
}