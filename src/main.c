#include "common.h"
#include "ncurs.h"

void handle_key(chtype key, Pointer data)
{
  uint32_t *id = (uint32_t *)data;
  if (key == 'q')
    ncurs_quit(*id);
}

int main(int argc, char *argv[])
{
  Pointer data = calloc(1, sizeof(uint32_t));
  uint32_t id = ncurs_start(NULL, NULL, handle_key, data);
  if (0U < id)
  {
    memcpy(data, &id, sizeof(uint32_t));
    ncurs_wait(id);
  }
  else
  {
    ncurs_quit(id);
  }

  free(data);
  return EXIT_SUCCESS;
}
