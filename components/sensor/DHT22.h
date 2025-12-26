#pragma once
#include "esp_err.h"

typedef struct {
    float temperature;
    float humidity;
} dht22_reading_t;

/* MUST be declared */
esp_err_t dht22_read(dht22_reading_t *out);

/* Task entry */
void dht22_task(void *arg);
