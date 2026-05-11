#include <game/game.h>

GAME_State* GAME_Init(EM* em, SDL_Window* window) {
  auto state = new (GAME_State);
  state->em = em;
  state->window = window;

  make(em, state->systems, 3);
  return state;
}
