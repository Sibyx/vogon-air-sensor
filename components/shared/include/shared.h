#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    float temperature;
    float humidity;
    uint16_t pm10;
    uint16_t pm25;
} shared_data_t;

extern shared_data_t shared_data;
extern SemaphoreHandle_t data_mutex;
