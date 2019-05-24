#include <stdio.h>
#include <cJSON.h>

#include "parseWeather.h"


void parseWeather(cJSON* input, weather_t* weather)
{
    printf("starting parseWeather\n\n");
    if (input == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error no cjson object in parseWeather: %s\n", error_ptr);
        }
        return;
    }

    cJSON* currently = cJSON_GetObjectItemCaseSensitive(input, "currently");
    weather->current.timestamp = cJSON_GetObjectItemCaseSensitive(currently, "time")->valueint;
    localtime_r(&(weather->current.timestamp), &(weather->current.time));
    weather->current.temp = cJSON_GetObjectItemCaseSensitive(currently, "temperature")->valuedouble;

    cJSON* daily = cJSON_GetObjectItemCaseSensitive(input, "daily");
    cJSON* daily_data = cJSON_GetObjectItemCaseSensitive(daily, "data");
    weather->sunrise_ts = 0;
    cJSON* day;
    cJSON* testSunrise;
    cJSON* testSunset;
    int i = 0;
    do {
        day = cJSON_GetArrayItem(daily_data, i);
        testSunrise = cJSON_GetObjectItemCaseSensitive(day, "sunriseTime");
        testSunset = cJSON_GetObjectItemCaseSensitive(day, "sunsetTime");
        if(weather->current.timestamp < testSunrise->valueint) {
            weather->sunrise_ts = testSunrise->valueint;
            localtime_r(&(weather->sunrise_ts), &(weather->sunrise));
        }
        if(weather->current.timestamp < testSunset->valueint) {
            weather->sunset_ts = testSunset->valueint;
            localtime_r(&(weather->sunset_ts), &(weather->sunset));
        }
        //printf("weather->sunrise[%d] == 0: %d;  && i[%d] < cJSON_GetArraySize(daily_data)[%d]: %d\n",
        //        weather->sunrise,
        //        weather->sunrise == 0,
        //        i,
        //        cJSON_GetArraySize(daily_data),
        //        i < cJSON_GetArraySize(daily_data));
    } while(weather->sunrise_ts == 0 && ++i < cJSON_GetArraySize(daily_data));

    printf("now     [%ld]: %d-%02d-%02d %02d:%02d:%02d -400\n", weather->current.timestamp, weather->current.time.tm_year+1900, weather->current.time.tm_mon+1, weather->current.time.tm_mday, weather->current.time.tm_hour, weather->current.time.tm_min, weather->current.time.tm_sec);
    printf("sunrise [%ld]: %d-%02d-%02d %02d:%02d:%02d -400\n", weather->sunrise_ts, weather->sunrise.tm_year+1900, weather->sunrise.tm_mon+1, weather->sunrise.tm_mday, weather->sunrise.tm_hour, weather->sunrise.tm_min, weather->sunrise.tm_sec);
    printf("sunset  [%ld]: %d-%02d-%02d %02d:%02d:%02d -400\n", weather->sunset_ts, weather->sunset.tm_year+1900, weather->sunset.tm_mon+1, weather->sunset.tm_mday, weather->sunset.tm_hour, weather->sunset.tm_min, weather->sunset.tm_sec);

    cJSON* hourly = cJSON_GetObjectItemCaseSensitive(input, "hourly");
    cJSON* hourly_data = cJSON_GetObjectItemCaseSensitive(hourly, "data");
    cJSON* hour;
    for(i = 0; i < 25; ++i) {
        hour = cJSON_GetArrayItem(hourly_data, i);
        weather->hourly[i].timestamp = cJSON_GetObjectItemCaseSensitive(hour, "time")->valueint;
        //localtime_r(&(weather->hourly[i].timestamp), &(weather->hourly[i].time));
        weather->hourly[i].temp = cJSON_GetObjectItemCaseSensitive(hour, "temperature")->valuedouble;

        //printf("time[%d]: %d, temp: %f\n", i, weather->hourly[i].timestamp, weather->hourly[i].temp);
    }
}
