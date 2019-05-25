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
#include "inaudible_led.h"
#include "parseWeather.h"
#include "yy_color_converter.h"

// defines OWP_API_KEY
#include "secret.h"
#include "wifi.h"

#define WEATHER_API_URL "https://api.darksky.net/forecast/"DARKSKY_API_KEY"/"LAT","LNG"?exclude=minutely,alerts,flags&lang=en&units=si"

#define TIMEZONE "EST+5EDT,M3.2.0/2,M11.1.0/2"


//kinda red to blue
//Black body radiation
//Taken from https://github.com/abzman/photo-transmission-suit/blob/master/redbull/redbull.ino
int armsR[] = {255,255,255,255,255,255,255,255,255,255,255,255,192,160,128,128,224,192,160,128,128,128,255,224,192,160,128,96, 80, 64, 64, 64 };
int armsG[] = {0,  0,  0,  0,  16, 32, 48, 64, 80, 96, 112,128,128,128,128,128,128,128,128,128,128,128,255,224,192,160,128,96, 80, 64, 64, 64 };
int armsB[] = {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  64, 128,255,255,255,255,255,255,255,255,255,255};
pixel_t colourMap(double temp)
{
    if(temp < 12) {
        temp = 12;
    }
    if(temp > 21) {
        temp = 21;
    }
    int i = 32-(temp-12)/(21-12)*32;

    pixel_t fuck;
    fuck.red = armsR[(int)i]/2;
    fuck.green = armsG[(int)i]/2;
    fuck.blue = armsB[(int)i]/2;

    //return (pixel_t){{, armsG[(int)temp]/2, armsB[(int)temp]/2}};
    return fuck;
}
static const char *TAG = "example";

#define WEATHER_JSON_BUFFLEN 25*1024
char weather_json[WEATHER_JSON_BUFFLEN];
int weather_json_index = 0;
bool weather_ready = false;
bool weather_parsed = false;
cJSON* weather_cjson;

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
            if(weather_cjson) {
                cJSON_Delete(weather_cjson);
            }
            weather_cjson = cJSON_Parse(weather_json);
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

void setLEDBasedOnTemp(pixel_t* p, float temp)
{
  if(temp < 15) {
    p->blue = 55;
  } else if(temp < 20) {
    p->green = 55;
  } else {
    p->red = 55;
  }
}

pixel_t lchToRGB(double l, double c, double h)
{
    CGFloat L, a, b;
    CGFloat X, Y, Z;
    CGFloat R, G, B;
    CIELCHab2CIELab(l, c, h, &L, &a, &b);
    CIELab2CIEXYZ(yy_illuminant_D50,
                   L, a, b,
                   &X, &Y, &Z);
    CIEXYZ2RGB(yy_illuminant_D50, yy_gamma_1_0,
                yy_rgbspace_sRGB, yy_adaption_NONE,
                X, Y, Z,
                &R, &G, &B);
    printf("LCH: %f, %f, %f\n", l, c, h);
    printf("RGB: %f, %f, %f\n", R, G, B);
    pixel_t ret = {(uint8_t)R, (uint8_t)G, (uint8_t)B};
    return ret;

}

int lineIntersection(
double Ax, double Ay,
double Bx, double By,
double Cx, double Cy,
double Dx, double Dy,
double *X, double *Y) {

  double  distAB, theCos, theSin, newX, ABpos ;

  //  Fail if either line is undefined.
  if ((Ax==Bx && Ay==By) || (Cx==Dx && Cy==Dy)) return 1;

  //  (1) Translate the system so that point A is on the origin.
  Bx-=Ax; By-=Ay;
  Cx-=Ax; Cy-=Ay;
  Dx-=Ax; Dy-=Ay;

  //  Discover the length of segment A-B.
  distAB=sqrt(Bx*Bx+By*By);

  //  (2) Rotate the system so that point B is on the positive X axis.
  theCos=Bx/distAB;
  theSin=By/distAB;
  newX=Cx*theCos+Cy*theSin;
  Cy  =Cy*theCos-Cx*theSin; Cx=newX;
  newX=Dx*theCos+Dy*theSin;
  Dy  =Dy*theCos-Dx*theSin; Dx=newX;

  //  Fail if the lines are parallel.
  if (Cy==Dy) return 1;

  //  (3) Discover the position of the intersection point along line A-B.
  ABpos=Dx+(Cx-Dx)*Dy/(Dy-Cy);

  //  (4) Apply the discovered position to line A-B in the original coordinate system.
  *X=Ax+ABpos*theCos;
  *Y=Ay+ABpos*theSin;

  //  Success.
  return 0;
}
double lineIntercept(double h, double a1, double r1, double a2, double r2)
{
    double x1 = r1* cos(a1);
    double y1 = r1* sin(a1);
    double x2 = r2* cos(a2);
    double y2 = r2* sin(a2);

    double x3 = 500* cos(h);
    double y3 = 500* sin(h);

    double X, Y;

    lineIntersection(x1, x2, y1, y2, 0, 0, x3, y3, &X, &Y);

    return sqrt(pow(X, 2) + pow(Y, 2));

}
double maxChromaForHue(double h)
{
    //triangle
    //red  : 36.31, 159.87
    double redA = 36.314239;
    double redD = 159.870807;
    //green: 138.56 204.76
    double greenA = 138.564686;
    double greenD = 204.756793;
    //blue : 304.441 162.86
    double blueA = 304.441534;
    double blueD = 162.864741;
    if(h < redA && h+360 > blueA) {
        return lineIntercept(h, redA, redD, blueA, blueD);
    }
    if(h > redA && h < greenA) {
        return lineIntercept(h, redA, redD, greenA, greenD);
    }
    if(h > greenA && h < blueA) {
        return lineIntercept(h, blueA, blueD, greenA, greenD);
    }
    else return 0;
}
void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    setenv("TZ", TIMEZONE, 1);
    tzset();

    led_init(0, 15);

    pixel_t pixels[288];
    for(int i=0; i<288; ++i) {

      //pixels[i] = lchToRGB(70, 5, i*2.5);
      pixels[i] = lchToRGB(200, maxChromaForHue(i*1.25), i*1.25);
      printf("%d, %d, %d\n", pixels[i].red, pixels[i].green, pixels[i].blue);
    };
    led_write(0, pixels, 288);

    //ESP_ERROR_CHECK(wifi_connect());

    //xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
    //static HSVpixel rainbow16[] = {{0.0f,1.0f,0.2f},{15.0f,1.0f,0.2f},{30.0f,1.0f,0.2f},{37.5f,1.0f,0.2f},{45.0f,1.0f,0.2f},
    //  {60.0f,1.0f,0.2f},{75.0f,1.0f,0.2f}, {90.0f,1.0f,0.2f},{120.0f,1.0f,0.2f},
    //  {150.0f,1.0f,0.0f},{180.0f,1.0f,0.0f},{210.0f,1.0f,0.0f},{240.0f,1.0f,0.0f},
    //  {270.0f,1.0f,0.0f},{300.0f,1.0f,0.0f},{330.0f,1.0f,0.0f},};
    //pixel_t pixels[16];
    //for(int i=0; i<16; ++i) {
    //    HSVtoRGB(&rainbow16[i], &pixels[i]);
    //}
    //led_write(0, pixels, 16);


    weather_t weather;
    while(1) {
        vTaskDelay(1);
        continue;
        if(weather_ready && !weather_parsed) {
            weather_parsed = true;


            parseWeather(weather_cjson, &weather);

            //clear pixels
            for(int i=0; i<24; ++i) {
                pixels[i] = colourMap(weather.hourly[i].temp);
                printf("%f: %d %d %d\n", weather.hourly[i].temp, pixels[i].red, pixels[i].green, pixels[i].blue);
            }
            for(int i=0; i<32; ++i) {
                pixels[i+28].red = (int)armsR[i]/2;
                pixels[i+28].green = (int)armsG[i]/2;
                pixels[i+28].blue = (int)armsB[i]/2;
            }
            ////pixel 0 = 0:00, pixel 1 = 0:30, pixel 58 == 00:00 next day
            ////pixel 0 is 0, pixel 58 is 86400
            ////pixel 1 is 1800
            //int nowSeconds = now.tm_hour*60*60 + now.tm_min*60 + now.tm_sec;
            //int nowPixel = nowSeconds/60/30+1;

            //int sunriseSeconds = sunrise.tm_hour*60*60 + sunrise.tm_min*60 + sunrise.tm_sec;
            //int sunrisePixel = sunriseSeconds/60/30+1;

            //int sunsetSeconds = sunset.tm_hour*60*60 + sunset.tm_min*60 + sunset.tm_sec;
            //int sunsetPixel = sunsetSeconds/60/30+1;

            //printf("sunrise: %d sunset: %d now: %d\n", sunriseSeconds, sunsetSeconds, nowSeconds);
            //printf("sunrise: %d sunset: %d now: %d\n", sunrisePixel, sunsetPixel, nowPixel);
            //pixels[sunrisePixel].red = 55;
            //pixels[sunsetPixel].blue = 55;
            //pixels[nowPixel].green = 55;

            //setLEDBasedOnTemp(&pixels[49], temp_min->valuedouble);
            //setLEDBasedOnTemp(&pixels[50], temp->valuedouble);
            //setLEDBasedOnTemp(&pixels[51], temp_max->valuedouble);

            led_write(0, pixels, 55);
        }
    }
}
