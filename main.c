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

#define LONGUEUR 160
#define HAUTEUR 144
#define RATIO 1
#define FRAMES_PER_SECOND 60

int cycles=0;
int ExecuteOpCode(struct emulator emu)
{
	printf("PC: %0x\n",emu.pc);
	op=read((emu.pc)++);
	printf("OP: %0x\n",op);
	return fetch(op);
}
void RenderScreen(SDL_Renderer *pRenderer, struct emulator emu)
{
	printf("Start rendering\n");
	//SDL_SetRenderDrawColor(pRenderer,150,150,150,255);
	for(int i=0; i<160;i++)
	{
		for(int j=0; j<144;j++)
		{
			SDL_SetRenderDrawColor(pRenderer,emu.ScreenData[i][j][0],emu.ScreenData[i][j][1],emu.ScreenData[i][j][2],255);
			SDL_RenderDrawPoint(pRenderer,i,j);
			//if(SDL_RenderDrawPoint(pRenderer,i,j)!=0)
				//printf("Couldn't render %d %d\n",i,i);
		}
		
	}	
	SDL_RenderPresent(pRenderer);
}

int Update(SDL_Renderer *pRenderer, struct emulator emu)
{
	FILE *f=fopen("log.txt","a");
	int cycles;
	fputs("Start Update\n",f);
	const int MAXCYCLES=69905;
	int cyclesThisUpdate=0;
	while(cyclesThisUpdate<MAXCYCLES)
	{
		char* buff=malloc(sizeof(char)*80);
		if(op!=0)
			printf("PC: %0x\n",emu.pc);
		sprintf(buff,"Executing PC:%0x\n",emu.pc);
		fputs(buff,f);
		cycles=ExecuteOpCode(emu);
		if(cycles==-1)
		{
			printf("Error Opcode=-1 end program");
			return -1;
		}
		sprintf(buff,"OP: %0x et b vaut %d, c=%0x et PC= %0x, CYCLES:%d\n",emu.op,emu.b,emu.c,emu.pc,cycles);
		fputs(buff,f);
		cyclesThisUpdate+=cycles;
		UpdateTimers(cycles);
		UpdateGraphics(cycles);
		DoInterrupt();
	}
	RenderScreen(pRenderer);
	fclose(f);
	return 0;
}

int main(int argc, char *argv[])
{

	if(argc!=2)
	{
		printf("1 argument required : GAME_NAME");
		return EXIT_FAILURE;
	}

	struct Emulator emu;

	char* GAME=argv[1];
	//Problème, hors de la boucle => utiliser extern
	//uint8_t mem[65536]; //16 bits

	//Ouverture du jeu:
	READ_GAME(GAME);
	printf("Game read.\n");

	//FENÊTRE:
	//char* name="emulator";
	if(0!=SDL_Init(SDL_INIT_VIDEO))
	{
		return EXIT_FAILURE;
	}
	printf("Video initialized.\n");

	SDL_Window* pWindow=NULL;
	//SDL_Event event;
	SDL_Renderer *pRenderer=NULL;
	SDL_CreateWindowAndRenderer(LONGUEUR*RATIO,HAUTEUR*RATIO,SDL_WINDOW_SHOWN,&pWindow,&pRenderer);
	printf("Window and renderer created.\n");
	Initialize(emu);
	printf("Initialized.\n");
	while(1)
	{
		printf("Update\n");
		if(Update(pRenderer,emu)==-1)
		{
			goto out;
		}
		SDL_Delay(100);
	}

	//FIN DE LA BOUCLE
	out:
	printf("Ending the game.\n");
	SDL_DestroyRenderer(pRenderer);
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
	
	return EXIT_SUCCESS;
}