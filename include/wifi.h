#ifndef _WIFI_H
#define _WIFI_H

#include "common.h"

#include <iwlib.h>

typedef struct wifi_status_t
{
  char interface[16];
  int socket;
  iwrange range;
  wireless_scan_head scan;
} WifiStatus;

void wifi_close(WifiStatus *status);
bool wifi_init (WifiStatus *status, const char *interface);
bool wifi_scan (WifiStatus *status);

int wifi_main(int argc, char *argv[]);

#endif // _WIFI_H
