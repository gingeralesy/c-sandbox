#include "sbsdl.h"

int sbsdl_main(int argc, char *argv[])
{
  SDL_Event e = {0};
  bool running = true;
  SDL_Window *win = NULL;

  if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
  {
    SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  if ((win =
       SDL_CreateWindow("Sandbox", SDL_WINDOWPOS_UNDEFINED,
                        SDL_WINDOWPOS_UNDEFINED, 640, 480,
                        SDL_WINDOW_SHOWN)) == NULL)
  {
    SDL_Log("Unable to initialize SDL window: %s", SDL_GetError());
    SDL_Quit();
    return EXIT_FAILURE;
  }

  while (running)
  {
    while (SDL_PollEvent(&e) != 0)
    {
      if (e.type == SDL_QUIT)
      {
        running = false;
      }
      else if (e.type == SDL_KEYDOWN)
      {
        switch (e.key.keysym.sym)
        {
        case SDLK_q:
          running = false;
          break;
        default:
          SDL_Log("Key: %s", SDL_GetKeyName(e.key.keysym.sym));
          break;
        }
      }
    }
  }

  SDL_DestroyWindow(win);
  SDL_Quit();

  return EXIT_SUCCESS;
}
