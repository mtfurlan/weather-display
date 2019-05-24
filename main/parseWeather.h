#ifndef PARSE_WEATHER_H_
#define PARSE_WEATHER_H_

#include <stdint.h>
#include <time.h>
#include <cJSON.h>

typedef struct hour_data {
    time_t timestamp;
    struct tm time;
    double temp;
} hour_data_t;

typedef struct current_data {
    time_t timestamp;
    struct tm time;
    double temp;
} current_data_t;

typedef struct weather {
    time_t sunrise_ts;
    struct tm sunrise;
    time_t sunset_ts;
    struct tm sunset;
    current_data_t current;
    hour_data_t hourly[25];
} weather_t;

void parseWeather(cJSON* input, weather_t* weather);

#endif // PARSE_WEATHER_H_
