#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "data_model.h"

void producer_queue_init(void);
BaseType_t producer_queue_send(const sensor_data_t *data);
BaseType_t producer_queue_receive(sensor_data_t *data, TickType_t wait);
