#include "dht22.h"
#include "producer_queue.h"
#include "data_model.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include <time.h>

static const char *TAG = "DHT22";

/* ================= DHT22 GPIO ================= */
#define DHT22_PIN GPIO_NUM_4
#define TIMEOUT_US 100
#define BIT_1_THRESHOLD_US 40

static portMUX_TYPE dht_mux = portMUX_INITIALIZER_UNLOCKED;

/* ================= LOW LEVEL DRIVER ================= */
static esp_err_t wait_level(int level, uint32_t timeout_us)
{
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(DHT22_PIN) != level) {
        if ((esp_timer_get_time() - start) > timeout_us) {
            return ESP_ERR_TIMEOUT;
        }
    }
    return ESP_OK;
}

/* 🔴 THIS FUNCTION WAS MISSING */
esp_err_t dht22_read(dht22_reading_t *out)
{
    uint8_t data[5] = {0};

    gpio_set_direction(DHT22_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT22_PIN, 0);
    esp_rom_delay_us(18000);

    gpio_set_level(DHT22_PIN, 1);
    esp_rom_delay_us(40);
    gpio_set_direction(DHT22_PIN, GPIO_MODE_INPUT);

    if (wait_level(0, TIMEOUT_US) != ESP_OK) return ESP_ERR_TIMEOUT;
    if (wait_level(1, TIMEOUT_US) != ESP_OK) return ESP_ERR_TIMEOUT;
    if (wait_level(0, TIMEOUT_US) != ESP_OK) return ESP_ERR_TIMEOUT;

    taskENTER_CRITICAL(&dht_mux);

    for (int i = 0; i < 40; i++) {
        if (wait_level(1, TIMEOUT_US) != ESP_OK) {
            taskEXIT_CRITICAL(&dht_mux);
            return ESP_ERR_TIMEOUT;
        }

        esp_rom_delay_us(30);
        data[i / 8] <<= 1;

        if (gpio_get_level(DHT22_PIN))
            data[i / 8] |= 1;

        if (wait_level(0, TIMEOUT_US) != ESP_OK) {
            taskEXIT_CRITICAL(&dht_mux);
            return ESP_ERR_TIMEOUT;
        }
    }

    taskEXIT_CRITICAL(&dht_mux);

    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4])
        return ESP_ERR_INVALID_CRC;

    out->humidity = ((data[0] << 8) | data[1]) / 10.0f;
    out->temperature = ((data[2] << 8) | data[3]) / 10.0f;

    return ESP_OK;
}

/* ================= PRODUCER TASK ================= */
void dht22_task(void *arg)
{
    dht22_reading_t r;
    sensor_data_t pkt;

    vTaskDelay(pdMS_TO_TICKS(3000));

    while (1) {
        if (dht22_read(&r) == ESP_OK) {
            pkt.type = SENSOR_DHT22;
            pkt.temperature = r.temperature;
            pkt.humidity = r.humidity;
            pkt.timestamp = time(NULL);

            producer_queue_send(&pkt);

            ESP_LOGI(TAG, "Queued T=%.1f H=%.1f",
                     r.temperature, r.humidity);
        } else {
            ESP_LOGW(TAG, "DHT22 read failed");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
