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

#include "driver/rmt.h"
#include "LEDs.h"
#include "analysis.h"
#include "../components/inaudible_led/include/types.h"

// defines OWP_API_KEY
#include "secret.h"
#include "wifi.h"

#define WEATHER_API_URL "https://openweathermap.org/data/2.5/weather?id=5007804&appid=" OWP_API_KEY

#define TIMEZONE "EST+5EDT,M3.2.0/2,M11.1.0/2"

static const char *TAG = "example";

#define WEATHER_JSON_BUFFLEN 1024
char weather_json[WEATHER_JSON_BUFFLEN];
int weather_json_index = 0;
bool weather_ready = false;
bool weather_parsed = false;
cJSON* weather;

uint32_t IRAM_ATTR millis()
{
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
}
void delay(uint32_t ms)
{
    if (ms == 0) return;
    vTaskDelay(ms / portTICK_PERIOD_MS);
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

void setLEDBasedOnTemp(pixel* p, float temp)
{
  if(temp < 15) {
    p->blue = 255;
  } else if(temp < 20) {
    p->green = 255;
  } else {
    p->red = 255;
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
    rmt_init(0, 15);
    //static HSVpixel rainbow16[] = {{0.0f,1.0f,0.2f},{15.0f,1.0f,0.2f},{30.0f,1.0f,0.2f},{37.5f,1.0f,0.2f},{45.0f,1.0f,0.2f},
    //  {60.0f,1.0f,0.2f},{75.0f,1.0f,0.2f}, {90.0f,1.0f,0.2f},{120.0f,1.0f,0.2f},
    //  {150.0f,1.0f,0.0f},{180.0f,1.0f,0.0f},{210.0f,1.0f,0.0f},{240.0f,1.0f,0.0f},
    //  {270.0f,1.0f,0.0f},{300.0f,1.0f,0.0f},{330.0f,1.0f,0.0f},};
    //pixel pixels[16];
    //for(int i=0; i<16; ++i) {
    //    HSVtoRGB(&rainbow16[i], &pixels[i]);
    //}
    //rmt_write_sample(0, pixels, 16*3, false);

    pixel pixels[55];
    for(int i=0; i<55; ++i) {
      pixels[i].red = 0;
      pixels[i].green = 0;
      pixels[i].blue = 0;
    };
    rmt_write_sample(0, pixels, 55*3, false);


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
            //TODO: double probably not big enough for 32 bit number
            time_t now_time = (int)now_j->valuedouble;
            time_t sunrise_time = (int)sunrise_j->valuedouble;
            time_t sunset_time = (int)sunset_j->valuedouble;
            const struct tm sunset;
            const struct tm now;
            const struct tm sunrise;
            localtime_r(&now_time, &now);
            localtime_r(&sunrise_time, &sunrise);
            localtime_r(&sunset_time, &sunset);
            printf("now tm struct %d-%d-%d %d:%d:%d -400\n", now.tm_year+1900, now.tm_mon+1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
            printf("sunrise tm struct %d-%d-%d %d:%d:%d -400\n", sunrise.tm_year+1900, sunrise.tm_mon+1, sunrise.tm_mday, sunrise.tm_hour, sunrise.tm_min, sunrise.tm_sec);
            printf("sunset tm struct %d-%d-%d %d:%d:%d -400\n", sunset.tm_year+1900, sunset.tm_mon+1, sunset.tm_mday, sunset.tm_hour, sunset.tm_min, sunset.tm_sec);

            printf("now fuckery %d %d %d %d \n", now.tm_hour*60*60, now.tm_min*60, now.tm_sec, now.tm_hour*60*60 + now.tm_min*60 + now.tm_sec);
            printf("sunrise fuckery %d %d %d %d \n", sunrise.tm_hour*60*60, sunrise.tm_min*60, sunrise.tm_sec, sunrise.tm_hour*60*60 + sunrise.tm_min*60 + sunrise.tm_sec);
            printf("sunset fuckery %d %d %d %d \n", sunset.tm_hour*60*60, sunset.tm_min*60, sunset.tm_sec, sunset.tm_hour*60*60 + sunset.tm_min*60 + sunset.tm_sec);

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

            //clear pixels
            for(int i=0; i<55; ++i) {
              pixels[i].red = 0;
              pixels[i].green = 0;
              pixels[i].blue = 0;
            };
            //pixel 0 = 0:00, pixel 1 = 0:30, pixel 58 == 00:00 next day
            //pixel 0 is 0, pixel 58 is 86400
            //pixel 1 is 1800
            int nowSeconds = now.tm_hour*60*60 + now.tm_min*60 + now.tm_sec;
            int nowPixel = nowSeconds/60/30+1;

            int sunriseSeconds = sunrise.tm_hour*60*60 + sunrise.tm_min*60 + sunrise.tm_sec;
            int sunrisePixel = sunriseSeconds/60/30+1;

            int sunsetSeconds = sunset.tm_hour*60*60 + sunset.tm_min*60 + sunset.tm_sec;
            int sunsetPixel = sunsetSeconds/60/30+1;

            printf("sunrise: %d sunset: %d now: %d\n", sunriseSeconds, sunsetSeconds, nowSeconds);
            printf("sunrise: %d sunset: %d now: %d\n", sunrisePixel, sunsetPixel, nowPixel);
            pixels[sunrisePixel].red = 255;
            pixels[sunsetPixel].blue = 255;
            pixels[nowPixel].green = 255;

            setLEDBasedOnTemp(&pixels[49], temp_min->valuedouble);
            setLEDBasedOnTemp(&pixels[50], temp->valuedouble);
            setLEDBasedOnTemp(&pixels[51], temp_max->valuedouble);

            rmt_write_sample(0, pixels, 55*3, false);
        }
        vTaskDelay(1);
    }
}
