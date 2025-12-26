#pragma once
#include <stdint.h>

typedef enum {
    SENSOR_DHT22 = 1,
} sensor_type_t;

typedef struct {
    uint8_t sensor_id;
    sensor_type_t type;
    float temperature;
    float humidity;
    uint32_t timestamp;
} sensor_data_t;
