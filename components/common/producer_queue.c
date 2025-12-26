#include "producer_queue.h"

#define SENSOR_QUEUE_DEPTH 10

static QueueHandle_t sensor_queue;

void producer_queue_init(void)
{
    sensor_queue = xQueueCreate(SENSOR_QUEUE_DEPTH, sizeof(sensor_data_t));
}

BaseType_t producer_queue_send(const sensor_data_t *data)
{
    return xQueueSend(sensor_queue, data, 0);
}

BaseType_t producer_queue_receive(sensor_data_t *data, TickType_t wait)
{
    return xQueueReceive(sensor_queue, data, wait);
}
