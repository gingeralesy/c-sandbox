#include "wifi.h"

void wifi_close(WifiStatus *status)
{
  iw_sockets_close(status->socket);
}

bool wifi_init(WifiStatus *status, const char *interface)
{
  static const char *DEFAULT_INTERFACE = "wlp2s0";
  const char *iface = (interface != NULL ? interface : DEFAULT_INTERFACE);
  int sock = 0;
  int iface_len = min(strlen(iface), 15);

  sock = iw_sockets_open();
  if (iw_get_range_info(sock, iface, &(status->range)) != 0)
  {
    iw_sockets_close(sock);
    return false;
  }
  status->socket = sock;
  memcpy(status->interface, iface, iface_len);
  status->interface[iface_len] = '\0';
  return true;
}

bool wifi_scan(WifiStatus *status)
{
  if (iw_scan(status->socket, status->interface,
              status->range.we_version_compiled, &(status->scan)) != 0)
  {
    wifi_close(status);
    return false;
  }
  return true;
}

int wifi_main(int argc, char *argv[])
{
  WifiStatus status = {0};
  wireless_scan *result = NULL;

  if (!wifi_init(&status, NULL))
  {
    fprintf(stderr, "Initialisation failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  if (!wifi_scan(&status))
  {
    fprintf(stderr, "Scan failed: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  result = status.scan.result;
  while (NULL != result)
  {
    fprintf(stdout, "%s\n", result->b.essid);
    result = result->next;
  }

  wifi_close(&status);
  return EXIT_SUCCESS;
}
