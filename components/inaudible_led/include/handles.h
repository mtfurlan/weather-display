#ifndef HANDLES_H
#define HANDLES_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

extern TaskHandle_t LEDTaskHandle;
extern TaskHandle_t AnalysisTaskHandle;
extern TaskHandle_t FFTTaskHandle;
extern xTaskHandle s_bt_app_task_handle;

extern QueueHandle_t rainbowDataBufferQueue;
extern QueueHandle_t VUMeterDataBufferQueue;
extern QueueHandle_t diffDataBufferQueue;
extern QueueHandle_t phaseDataBufferQueue;
extern QueueHandle_t LEDEmptyBufferQueue;
extern QueueHandle_t audioDataQueue;
extern QueueHandle_t freqDataQueue;

extern SemaphoreHandle_t audioDataSemaphore;

#endif
