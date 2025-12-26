
#include "producer_queue.h"
#include "azure_iot.h"
#include "esp_log.h"
#include "telemetry_task.h"

static const char *TAG = "TELEMETRY";

void telemetry_task(void *arg)
{
    sensor_data_t data;

    while (1) {
        if (producer_queue_receive(&data, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Sending telemetry");
            azure_iot_send(&data);
        }
    }
}