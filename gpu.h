#ifndef GPU_H
#define GPU_H
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdlib.h>
#include "cpu.h"
#include "interrupts.h"
#endif
const uint8_t GetColour(uint8_t colourNum, uint16_t addr,struct Emulator emu);
void RenderTiles(struct Emulator emu);
void RenderSprites(struct Emulator emu);
void DrawScanLine(struct Emulator emu);
void SetLCDStatus(struct Emulator emu);
bool IsLCDEnabled(struct Emulator emu);
void UpdateGraphics(uint8_t cycles,struct Emulator emu);