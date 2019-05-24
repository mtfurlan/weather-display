#include <stdio.h>
#include <stdlib.h>
#include "parseWeather.h"
#include "colour.h"

int main(void)
{
    FILE *f = fopen("darksky.json", "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    char* json = malloc(fsize + 1);
    fread(json, 1, fsize, f);
    fclose(f);

    json[fsize] = 0;
    cJSON* weather_cjson = cJSON_Parse(json);
    printf("Read JSON\n");

    weather_t weather;

    parseWeather(weather_cjson, &weather);

    for(int i = 0; i < 25; ++i) {
        char sun = ' ';
        if(weather.hourly[i].timestamp < weather.sunrise_ts && weather.sunrise_ts < weather.hourly[i].timestamp + 60*60) {
            sun = '^';
        } else if(weather.hourly[i].timestamp < weather.sunset_ts && weather.sunset_ts < weather.hourly[i].timestamp + 60*60) {
            sun = 'v';
        }
        printf("[%2.1f %c]", weather.hourly[i].temp, sun);
        //localtime_r(&(weather.hourly[i].timestamp), &(weather.hourly[i].time));
        //printf("%04d-%02d-%02d %02d:%02d:%02d -400 %f %c\n", weather.hourly[i].time.tm_year+1900, weather.hourly[i].time.tm_mon+1, weather.hourly[i].time.tm_mday, weather.hourly[i].time.tm_hour, weather.hourly[i].time.tm_min, weather.hourly[i].time.tm_sec, weather.hourly[i].temp, sun);
    }
    printf("\n");

    colour_t red = {{53.2328817858425, 104.575518439936, 40.0001579064637}};
    pixel_t pixel =  lch2rgb(red);
    printf("%d, %d, %d\n", pixel.red, pixel.green, pixel.blue);

    return 0;
}
