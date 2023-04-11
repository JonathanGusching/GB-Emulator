#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdlib.h>
#include "cpu.h"
#include "gpu.h"
#include "interrupts.h"
#include "timer.h"
#include "emulator.h"

#define TIMA 0xFF05
#define TMA 0xFF06
#define TMC 0xFF07
int timerCounter=1024;
int dividerCounter=0;
int dividerRegister=0;

uint8_t GetClockFreq(struct Emulator emu)
{
	return read(TMC,emu)&0x3;
}

void SetClockFreq(struct Emulator emu)
{
	uint8_t freq=GetClockFreq(emu);
	switch(freq)
	{
		case 0:emu.timerCounter=1024;break;
		case 1:emu.timerCounter=16;break;
		case 2:emu.timerCounter=64;break;
		case 3:emu.timerCounter=256;break;
	}
}

void DoDividerRegister(int cycles, struct Emulator emu)
{
	emu.dividerRegister+=cycles;
	if(emu.dividerCounter>=255)
	{
		emu.dividerCounter=0;
		emu.mem[0xFF04]++;
	}
}
bool IsClockedEnabled(struct Emulator emu)
{
	return((read(TMC,emu)>>2)&1);
}
void UpdateTimers(int cycles, struct Emulator emu)
{
	DoDividerRegister(cycles,emu);
	if(IsClockedEnabled(emu))
	{
		emu.timerCounter-=cycles;
		if(emu.timerCounter<=0)
		{
			SetClockFreq(emu);
			if(read(TIMA,emu)==255)
			{
				write(TIMA,read(TIMA,emu),emu);
				RequestInterrupt(2,emu);
			}
			else
			{
				write(TIMA,read(TIMA,emu)+1,emu);
			}
		}
	}
}