#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdlib.h>
#include "cpu.h"
#include "gpu.h"
#include "interrupts.h"
#include "emulator.h"
#endif
void DoDividerRegister(int cycles,struct Emulator emu);
uint8_t GetClockFreq(struct Emulator emu);
void SetClockFreq(struct Emulator emu);
bool IsClockedEnabled(struct Emulator emu);
void UpdateTimers(int cycles,struct Emulator emu);