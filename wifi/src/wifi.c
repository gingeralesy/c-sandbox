#include "wifi.h"

#include <iwlib.h>

int wifi_main(int argc, char *argv[])
{
  static const char *INTERFACE = "wlp2s0";
  wireless_scan_head head;
  wireless_scan *result;
  iwrange range;
  int sock;

  /* Open socket to kernel */
  sock = iw_sockets_open();

  /* Get some metadata to use for scanning */
  if (iw_get_range_info(sock, INTERFACE, &range) < 0)
  {
    printf("Error during iw_get_range_info. Aborting.\n");
    iw_sockets_close(sock);
    return EXIT_FAILURE;
  }

  /* Perform the scan */
  if (iw_scan(sock, INTERFACE, range.we_version_compiled, &head) < 0)
  {
    printf("Error during iw_scan. Aborting.\n");
    iw_sockets_close(sock);
    return EXIT_FAILURE;
  }

  /* Traverse the results */
  result = head.result;
  while (NULL != result)
  {
    printf("%s\n", result->b.essid);
    result = result->next;
  }
  iw_sockets_close(sock);

  return EXIT_SUCCESS;
}
