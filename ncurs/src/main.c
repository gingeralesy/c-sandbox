#include "common.h"
#include "ncurs.h"

void handle_key(chtype key, Pointer data)
{
  uint32_t *id = (uint32_t *)data;
  if (id != NULL && 0U < *id)
  {
    WINDOW *win = ncurs_window(*id);
    if (win != NULL)
      mvwaddch(win, 0, 0, key);
    if (key == 'q')
      ncurs_quit(*id);
  }
}

int main(int argc, char *argv[])
{
  uint32_t *data = (uint32_t *)calloc(1, sizeof(uint32_t));
  uint32_t id = ncurs_start(NULL, NULL, &handle_key, data);
  if (0U < id)
  {
    (*data) = id;
    ncurs_wait(id);
  }
  else
  {
    ncurs_quit(id);
  }

  free(data);
  return EXIT_SUCCESS;
}
