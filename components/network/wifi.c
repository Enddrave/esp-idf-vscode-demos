#include <string.h>
#include "wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "driver/gpio.h"

/* Credentials via menuconfig */
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASSWORD

/* Optional onboard LED */
#define WIFI_LED_GPIO GPIO_NUM_2

static const char *TAG = "WIFI";
static EventGroupHandle_t wifi_event_group;

/* Expose WiFi event group */
EventGroupHandle_t wifi_event_group_get(void)
{
    return wifi_event_group;
}

/* WiFi event handler */
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START) {

        ESP_LOGI(TAG, "STA start, connecting...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {

        ESP_LOGW(TAG, "Disconnected, retrying...");
        gpio_set_level(WIFI_LED_GPIO, 0);
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP) {

        ESP_LOGI(TAG, "Got IP, WiFi connected");
        gpio_set_level(WIFI_LED_GPIO, 1);
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* Start WiFi (NON-BLOCKING) */
void wifi_start(void)
{
    wifi_event_group = xEventGroupCreate();

    /* LED GPIO setup */
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << WIFI_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(WIFI_LED_GPIO, 0);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi started (non-blocking)");
}
