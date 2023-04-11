#include "cpu.h"
#include "gpu.h"
#include <stdbool.h>

enum ID {VBlank, LCD, Timer, Nothing, Joypad};
void RequestInterrupt(uint8_t id)
{
	uint8_t req = read(0xFF0F);
	req=req|(1<<id);
	write(0xFF0F,req);
}

void ServiceInterrupt(int interrupt)
{
	if(interrupt>7)
	{
		printf("Interrupt error: %d > 7\n",interrupt);
		return;
	}
	ime=false;
	uint8_t req=read(0xFF0F);
	req=req & (~(1<<interrupt)); //reset interrupt bit
	write(0xFF0F,req);
	PUSH(pc);
	switch(interrupt)
	{
		case 0: pc=0x40; break;
		case 1: pc=0x48; break;
		case 2: pc=0x50; break;
		case 4: pc=0x60; break;
		default: printf("Error: ServiceInterrupt: %d out of range", interrupt);break;
	}

}

void DoInterrupt()
{
	if(ime)//master activé
	{
		uint8_t req=read(0xFF0F);
		uint8_t enab=read(0xFFFF);

		if(req>0)
		{
			for(int i=0; i<5; i++)
			{
				if((req>>i&1)==1 && (enab>>i&1)==1)
				{
					ServiceInterrupt(i);
				}
			}
		}

	}
}
