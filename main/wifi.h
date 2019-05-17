
#ifndef _WIFI_H_
#define _WIFI_H_

#include "secret.h"

/* set up connection, Wi-Fi or Ethernet */
static void start();

/* tear down connection, release resources */
static void stop();

esp_err_t wifi_connect();

#endif // _WIFI_H_
