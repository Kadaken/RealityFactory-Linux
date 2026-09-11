#ifndef RF_BOOT_GAME_H
#define RF_BOOT_GAME_H
// Observers for the inherited GameLoop; inert outside hidden stage 5.
bool RF_BootGameActive();
bool RF_BootGameBeginTick();
bool RF_BootGameCapture();
bool RF_BootGameEndFrame(bool captured, int result);
bool RF_BootGameEndTick();
float RF_BootGameStepMilliseconds();
#endif
