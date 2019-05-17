/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_http_client.h>

#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>

#include <driver/gpio.h>

#include <cJSON.h>
#include "esp32_digital_led_lib.h"

// defines OWP_API_KEY
#include "secret.h"
#include "wifi.h"

#define WEATHER_API_URL "https://openweathermap.org/data/2.5/weather?id=5007804&appid=" OWP_API_KEY

#define TIMEZONE "EST+5EDT,M3.2.0/2,M11.1.0/2"

#define HIGH 1
#define LOW 0
#define OUTPUT GPIO_MODE_OUTPUT
#define INPUT GPIO_MODE_INPUT

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

#define min(a, b)  ((a) < (b) ? (a) : (b))
#define max(a, b)  ((a) > (b) ? (a) : (b))
#define floor(a)   ((int)(a))
#define ceil(a)    ((int)((int)(a) < (a) ? (a+1) : (a)))

static const char *TAG = "example";

#define WEATHER_JSON_BUFFLEN 1024
char weather_json[WEATHER_JSON_BUFFLEN];
int weather_json_index = 0;
bool weather_ready = false;
bool weather_parsed = false;
cJSON* weather;

strand_t STRANDS[] = { // Avoid using any of the strapping pins on the ESP32
  {.rmtChannel = 0, .gpioNum = 15, .ledType = LED_WS2812B_V3, .brightLimit = 32, .numPixels = 50,
   .pixels = NULL, ._stateVars = NULL},
};
int STRANDCNT = sizeof(STRANDS)/sizeof(STRANDS[0]);

uint32_t IRAM_ATTR millis()
{
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
}
void delay(uint32_t ms)
{
    if (ms == 0) return;
    vTaskDelay(ms / portTICK_PERIOD_MS);
}
void rainbow(strand_t * pStrand, unsigned long delay_ms, unsigned long timeout_ms)
{
  const uint8_t color_div = 4;
  const uint8_t anim_step = 1;
  const uint8_t anim_max = pStrand->brightLimit - anim_step;
  pixelColor_t color1 = pixelFromRGB(anim_max, 0, 0);
  pixelColor_t color2 = pixelFromRGB(anim_max, 0, 0);
  uint8_t stepVal1 = 0;
  uint8_t stepVal2 = 0;
  bool runForever = (timeout_ms == 0 ? true : false);
  unsigned long start_ms = millis();
  while (runForever || (millis() - start_ms < timeout_ms)) {
    color1 = color2;
    stepVal1 = stepVal2;
    for (uint16_t i = 0; i < pStrand->numPixels; i++) {
      pStrand->pixels[i] = pixelFromRGB(color1.r/color_div, color1.g/color_div, color1.b/color_div);
      if (i == 1) {
        color2 = color1;
        stepVal2 = stepVal1;
      }
      switch (stepVal1) {
        case 0:
        color1.g += anim_step;
        if (color1.g >= anim_max)
          stepVal1++;
        break;
        case 1:
        color1.r -= anim_step;
        if (color1.r == 0)
          stepVal1++;
        break;
        case 2:
        color1.b += anim_step;
        if (color1.b >= anim_max)
          stepVal1++;
        break;
        case 3:
        color1.g -= anim_step;
        if (color1.g == 0)
          stepVal1++;
        break;
        case 4:
        color1.r += anim_step;
        if (color1.r >= anim_max)
          stepVal1++;
        break;
        case 5:
        color1.b -= anim_step;
        if (color1.b == 0)
          stepVal1 = 0;
        break;
      }
    }
    digitalLeds_updatePixels(pStrand);
    delay(delay_ms);
  }
  digitalLeds_resetPixels(pStrand);
}

esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            //if (!esp_http_client_is_chunked_response(evt->client)) {
            if(weather_json_index + evt->data_len > WEATHER_JSON_BUFFLEN) {
                printf("too many data\n");
                return;
            }
            memcpy(weather_json+weather_json_index, evt->data, evt->data_len);
            weather_json_index += evt->data_len;
            weather_ready = false;

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH len=%d", weather_json_index);
            weather_json[weather_json_index] = '\0';
            //printf("%s\n", weather_json);
            weather_json_index = 0;
            if(weather) {
                cJSON_Delete(weather);
            }
            weather = cJSON_Parse(weather_json);
            weather_ready = true;
            weather_parsed = false;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}


static void http_get_task(void *pvParameters)
{
    esp_err_t err;
    esp_http_client_config_t config = {
        .url = WEATHER_API_URL,
        .event_handler = _http_event_handle,

    };


    while(1) {
        esp_http_client_handle_t client = esp_http_client_init(&config);
        err = esp_http_client_perform(client);

        esp_http_client_cleanup(client);
        vTaskDelay(1000000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Starting again!");
    }
}
void led_task(void *pvParameters)
{
  gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_15, LOW);
  if (digitalLeds_initStrands(STRANDS, STRANDCNT)) {
    ets_printf("Init FAILURE: halting\n");
    while (true) {};
  }
  while (true) {
    for (int i = 0; i < STRANDCNT; i++) {
      strand_t * pStrand = &STRANDS[i];
      rainbow(pStrand, 0, 2000);
      digitalLeds_resetPixels(pStrand);
    }
  }
}

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    setenv("TZ", TIMEZONE, 1);
    tzset();

    ESP_ERROR_CHECK(wifi_connect());

    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
    xTaskCreate(&led_task, "led_task", 4096, NULL, 10, NULL);

    char buf[64];
    while(1) {
        if(weather_ready && !weather_parsed) {
            weather_parsed = true;

            cJSON* sys = cJSON_GetObjectItemCaseSensitive(weather, "sys");
            cJSON* sunrise_j = cJSON_GetObjectItemCaseSensitive(sys, "sunrise");
            cJSON* sunset_j = cJSON_GetObjectItemCaseSensitive(sys, "sunset");
            cJSON* now_j = cJSON_GetObjectItemCaseSensitive(weather, "dt");
            if (!cJSON_IsNumber(now_j) || !cJSON_IsNumber(sunrise_j) || !cJSON_IsNumber(sunset_j)) {
                printf("weather, dt/sunrise/sunset is not number");
                continue;
            }
            time_t now_time = (int)now_j->valuedouble;
            time_t sunrise_time = (int)sunrise_j->valuedouble;
            time_t sunset_time = (int)sunset_j->valuedouble;
            const struct tm* now = localtime(&now_time);
            const struct tm* sunrise = localtime(&sunrise_time);
            const struct tm* sunset = localtime(&sunset_time);

            strftime(buf, sizeof(buf), "%F %H:%M:%S %z", now);
            printf("time is %ld %s\n", now_time, buf);
            strftime(buf, sizeof(buf), "%F %H:%M:%S %z", sunrise);
            printf("sunrise is %ld %s\n", sunrise_time, buf);
            strftime(buf, sizeof(buf), "%F %H:%M:%S %z", sunset);
            printf("sunset is %ld %s\n", sunset_time, buf);

            cJSON* current = cJSON_GetObjectItemCaseSensitive(weather, "main");
            cJSON* temp = cJSON_GetObjectItemCaseSensitive(current, "temp");
            cJSON* temp_min = cJSON_GetObjectItemCaseSensitive(current, "temp_min");
            cJSON* temp_max = cJSON_GetObjectItemCaseSensitive(current, "temp_max");
            cJSON* humidity = cJSON_GetObjectItemCaseSensitive(current, "humidity");

            // Assuming if temp works the rest will
            if (!cJSON_IsNumber(temp)) {
                printf("weather, temp is not number");
                continue;
            }
            printf("temp: %.1f [%.1f - %.1f], %.2f%% humidity\n", temp->valuedouble, temp_min->valuedouble, temp_max->valuedouble, humidity->valuedouble);
            //got weather, time is +808927389-540028987-808466992 807416096:544104778:544565332 GM
            //    temp: 19.1 [16.1 - 21.0], 20.00% humidity
        }
        vTaskDelay(1);
    }
}
