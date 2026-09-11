#ifndef RF_INPUT_COMPAT_H
#define RF_INPUT_COMPAT_H
#include <SDL2/SDL.h>
void RF_InputEvent(const SDL_Event *event);
void RF_InputReset(void);
void RF_InputRelativeMode(bool enabled);
void RF_InputTakeMotion(int *x, int *y);
void RF_InputMenuFrame(Uint32 physical_buttons);
int RF_InputTakeWheelStep(void);
#endif
