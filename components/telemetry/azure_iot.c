#include <stdio.h>
#include <time.h>

#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "azure_iot.h"
#include "azure_ca.h"
#include "data_model.h"

static const char *TAG = "AZURE";
static esp_mqtt_client_handle_t client;
static bool mqtt_connected = false;

/* These come from menuconfig / sdkconfig (NOT GitHub) */
#define AZURE_HOSTNAME   CONFIG_AZURE_IOT_HOSTNAME
#define AZURE_DEVICE_ID  CONFIG_AZURE_IOT_DEVICE_ID
#define AZURE_SAS_TOKEN  CONFIG_AZURE_IOT_SAS_TOKEN

/* ---------- Time sync (required for SAS) ---------- */
static void sync_time(void)
{
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    time_t now = 0;
    while (now < 1700000000) {
        ESP_LOGI(TAG, "Waiting for time sync...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);
    }
}

/* ---------- MQTT event handler ---------- */
static void mqtt_event_handler(void *arg,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            mqtt_connected = true;
            ESP_LOGI(TAG, "MQTT connected");
            break;

        case MQTT_EVENT_DISCONNECTED:
            mqtt_connected = false;
            ESP_LOGW(TAG, "MQTT disconnected");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;
    }
}

bool azure_iot_is_connected(void)
{
    return mqtt_connected;
}

/* ---------- Azure IoT Init ---------- */
void azure_iot_init(void)
{
    sync_time();

    char mqtt_uri[128];
    char username[128];

    snprintf(mqtt_uri, sizeof(mqtt_uri),
             "mqtts://%s:8883", AZURE_HOSTNAME);

    snprintf(username, sizeof(username),
             "%s/%s/?api-version=2021-04-12",
             AZURE_HOSTNAME, AZURE_DEVICE_ID);

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = mqtt_uri,
        .broker.verification.certificate = azure_root_ca,
        .credentials.client_id = AZURE_DEVICE_ID,
        .credentials.username = username,
        .credentials.authentication.password = AZURE_SAS_TOKEN,
        .session.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
    };

    client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client,
                                   ESP_EVENT_ANY_ID,
                                   mqtt_event_handler,
                                   NULL);
    esp_mqtt_client_start(client);
}

/* ---------- Send Telemetry (CONSUMER) ---------- */
void azure_iot_send(const sensor_data_t *data)
{
    if (!mqtt_connected) {
        ESP_LOGW(TAG, "MQTT not connected, dropping telemetry");
        return;
    }

    

char payload[512];

float anomaly_score =
    (data->temperature > 40.0f || data->humidity > 80.0f) ? 0.9f : 0.1f;

snprintf(payload, sizeof(payload),
    "{"
    "\"id\":\"esp32-%lu\","
    "\"deviceId\":\"ENV-NODE-01\","
    "\"location\":\"Factory Lab - Gurugram\","
    "\"firmwareVersion\":\"v1.0.3\","
    "\"temperature\":%.2f,"
    "\"humidity\":%.2f,"
    "\"status\":\"online\","
    "\"anomalyScore\":%.2f,"
    "\"ts\":%lu,"
    "\"ledState\":\"off\","
    "\"otaStatus\":\"idle\","
    "\"lastCommand\":null"
    "}",
    (unsigned long)data->timestamp,   // id
    data->temperature,
    data->humidity,
    anomaly_score,
    (unsigned long)data->timestamp    // ts
);

    esp_mqtt_client_publish(
        client,
        "devices/" AZURE_DEVICE_ID "/messages/events/",
        payload,
        0,
        1,
        0
    );

    ESP_LOGI(TAG, "Telemetry sent");
}
