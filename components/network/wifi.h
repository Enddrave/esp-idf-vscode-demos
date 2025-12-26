#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/* WiFi-specific bit (internal) */
#define WIFI_CONNECTED_BIT BIT0

void wifi_start(void);
EventGroupHandle_t wifi_event_group_get(void);
