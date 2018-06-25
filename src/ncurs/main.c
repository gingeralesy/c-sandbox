#include "common.h"
#include "ncurs.h"

typedef struct game_state_t
{
  uint32_t process;
  chtype current_key;
  WINDOW *win;
} GameState;

void handle_key(chtype key, Pointer data)
{
  GameState *state = (GameState *)data;
  if (state != NULL)
    state->current_key = key;
}

void update(struct timespec *dt, Pointer data)
{
  GameState *state = (GameState *)data;
  if (state != NULL && 0U < state->process)
  {
    if (state->win == NULL)
      state->win = ncurs_window(state->process);
    if (state->current_key != '\0')
    {
      if (state->current_key == 'q')
      {
        mvwaddstr(state->win, 0, 0, "Quitting...");
        ncurs_quit(state->process);
      }
      else if (state->win != NULL)
      {
        mvwaddch(state->win, 0, 0, state->current_key);
      }
    }
  }
}

int main(int argc, char *argv[])
{
  GameState state = {0};

  state.process = ncurs_start(&update, &state, &handle_key, &state);
  if (0U < state.process)
    ncurs_wait(state.process);
  else
    ncurs_quit(state.process);

  return EXIT_SUCCESS;
}
