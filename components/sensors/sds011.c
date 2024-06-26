#include <sys/cdefs.h>
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sensors.h"
#include "shared.h"

#define TXD_PIN (GPIO_NUM_17) // Use GPIO17 for TX
#define RXD_PIN (GPIO_NUM_16) // Use GPIO16 for RX
#define UART_BUFFER (1024)

static const char* TAG = "MODULE_SDS011";

const uint8_t ACTIVE_MODE = 0x00;
const uint8_t QUERY_MODE = 0x01;

const uint8_t WORK_STATE = 0x01;
const uint8_t SLEEP_STATE = 0x00;

static esp_err_t sds011_send_command(const uint8_t payload[13], char *response) {
    int len;
    uint8_t command[] = {
            0xAA,0xB4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00,0x00, 0x00,0x00, 0x00,0xFF, 0xFF, 0x00, 0xAB
    };

    memcpy(&command[2], payload, 13);

    // Checksum
    uint16_t checksum = 0;
    for (int i = 2; i <= 16; i++ ) {
        checksum += command[i];
    }
    command[17] = (uint8_t)(checksum & 0xFF);

    uart_write_bytes(UART_NUM_2, (const char *)command, 19);
    ESP_ERROR_CHECK(uart_wait_tx_done(UART_NUM_2, pdMS_TO_TICKS(250)));
    len = uart_read_bytes(UART_NUM_2, response, 10, pdMS_TO_TICKS(250));

    if (len != 10) {
        ESP_LOGE(TAG, "Error reading response: %d", len);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t sds011_read_reporting_mode(char *buffer, uint8_t *mode) {
    const uint8_t payload[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00,0x00, 0x00,0x00, 0x00};

    if (sds011_send_command(payload, buffer) != ESP_OK) {
        return ESP_FAIL;
    }


    *mode = *((uint8_t *)(buffer + 4));

    return ESP_OK;
}

static esp_err_t sds011_write_reporting_mode(char *buffer, uint8_t mode) {
    const uint8_t payload[] = {0x02, 0x01, mode, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00,0x00, 0x00,0x00, 0x00};

    if (sds011_send_command(payload, buffer) != ESP_OK) {
        return ESP_FAIL;
    }

    uint8_t result = *((uint8_t *)(buffer + 4));

    if (result != mode) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t sds011_query_data(char *buffer, uint16_t *pm25, uint16_t *pm10) {
    const uint8_t payload[] = {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00,0x00, 0x00,0x00, 0x00};

    if (sds011_send_command(payload, buffer) != ESP_OK) {
        return ESP_FAIL;
    }

    *pm25 = *((uint16_t *)(buffer + 2));
    *pm10 = *((uint16_t *)(buffer + 4));

    return ESP_OK;
}

static esp_err_t sds011_write_state(char *buffer, uint8_t state) {
    const uint8_t payload[] = {0x06, 0x01, state, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00,0x00, 0x00,0x00, 0x00};

    if (sds011_send_command(payload, buffer) != ESP_OK) {
        return ESP_FAIL;
    }

    uint8_t result = *((uint8_t *)(buffer + 4));

    if (result != state) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * Protocol description: https://sensebox.kaufen/assets/datenblatt/SDS011_Control_Protocol.pdf
 * @param pvParameters
 */
void sds011_task(void *pvParameters) {
    const uart_config_t uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, UART_BUFFER * 2, 0, 0, NULL, 0));

    uint8_t reporting_mode;

    char data[UART_BUFFER];
    memset(&data, 0, UART_BUFFER);

    sds011_write_state(data, WORK_STATE);
    vTaskDelay(pdMS_TO_TICKS(CONFIG_SENSORS_SDS011_WARM_UP * 1000));

    if (sds011_read_reporting_mode(data, &reporting_mode) != ESP_OK) {
        ESP_LOGE(TAG, "Unable to read SDS011 reporting mode! Commiting suicide...");
        return;
    }

    if (reporting_mode == ACTIVE_MODE) {
        ESP_LOGW(TAG, "SDS011 is currently in ACTIVE reporting mode. Switching to QUERY reporting mode.");

        if (sds011_write_reporting_mode(data, QUERY_MODE) != ESP_OK) {
            ESP_LOGE(TAG, "Unable to set SDS011 QUERY reporting mode. Commiting suicide...");
            return;
        }
    }

    while (1) {
        ESP_LOGI(TAG, "Waking up SDS011");
        sds011_write_state(data, WORK_STATE);

        vTaskDelay(pdMS_TO_TICKS(CONFIG_SENSORS_SDS011_WARM_UP * 1000));

        uint16_t pm25_total = 0;
        uint16_t pm10_total = 0;

        for (int i = 0; i < CONFIG_SENSORS_SDS011_MEASUREMENT_BULK_SIZE; i++) {
            uint16_t pm25 = 0;
            uint16_t pm10 = 0;
            ESP_LOGI(TAG, "Performing measurement %d of %d", i + 1, CONFIG_SENSORS_SDS011_MEASUREMENT_BULK_SIZE);
            sds011_query_data(data, &pm25, &pm10);

            pm25_total += pm25;
            pm10_total += pm10;

            vTaskDelay(pdMS_TO_TICKS(CONFIG_SENSORS_SDS011_MEASUREMENT_BULK_SLEEP * 1000));
        }

        uint16_t pm25 = pm25_total / CONFIG_SENSORS_SDS011_MEASUREMENT_BULK_SIZE;
        uint16_t pm10 = pm10_total / CONFIG_SENSORS_SDS011_MEASUREMENT_BULK_SIZE;

        ESP_LOGI(TAG, "Measurements: PM2.5=%d, PM10=%d", pm25, pm10);

        ESP_LOGI(TAG, "Setting SDS011 to sleep");
        sds011_write_state(data, SLEEP_STATE);

        // Store measurements to shared memory
        if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) {
            shared_data.pm25 = pm25;
            shared_data.pm10 = pm10;

            xSemaphoreGive(data_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_SENSORS_SDS011_SLEEP_PERIOD * 60 * 1000));
    }
}