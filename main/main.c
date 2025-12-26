#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi.h"
#include "azure_iot.h"
#include "producer_queue.h"
#include "dht22.h"
#include "telemetry_task.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_LOGI(TAG, "System init");

    producer_queue_init();

    wifi_start();
    azure_iot_init();

    xTaskCreate(dht22_task, "dht22_task", 4096, NULL, 5, NULL);
    xTaskCreate(telemetry_task, "telemetry_task", 4096, NULL, 6, NULL);
}
