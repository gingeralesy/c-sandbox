#include "common.h"
#include "ncurs.h"

int main(int argc, char *argv[])
{
  uint32_t id = ncurs_start(NULL, NULL, NULL, NULL);
  ncurs_wait(id);
  return EXIT_SUCCESS;
}
