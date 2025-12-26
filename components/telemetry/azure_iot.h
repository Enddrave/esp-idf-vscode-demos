#pragma once
#include "data_model.h"

void azure_iot_init(void);
void azure_iot_send(const sensor_data_t *data);
bool azure_iot_is_connected(void);
