#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdlib.h>
#include "cpu.h"
#include "interrupts.h"
//Tile data table : soit 8000 - 8FFF OU BIEN 8800 - 97FF
//Dans le premier cas : pattern = non signé entre 0 et 255 (0 situé en 8000)
//Dans le second : -128 - 127 et 0 à l'adresse 9000
//Registre LCDC donne l'adresse pour le background
//utiliser : https://gbdev.gg8.se/wiki/articles/Video_Display

//0xFF44 = scanline actuelle
// 144 -> 153 vertical blank

//int scanlineCounter=0;
#define MODE2BOUNDS 376
#define MODE3BOUNDS 204

#define WHITE 0
#define LIGHT_GRAY 1
#define DARK_GRAY 2
#define BLACK 3
/*
SDL_Color WHITE={.r = 202, .g = 220, .b = 159}; //blanc
SDL_Color BLACK={.r = 15, .g = 56, .b = 15}; //noir
SDL_Color DARK_GREEN={.r = 48, .g = 98, .b = 48}; //vert foncé
SDL_Color LIGHT_GREEN={.r = 155, .g = 188, .b = 15}; //vert clair
*/
//uint8_t emu.ScreenData[160][144][3]={{{255}}};

const uint8_t GetColour(uint8_t colourNum, uint16_t addr, struct Emulator emu)
{
	uint8_t res=WHITE;
	uint8_t palette=read(addr,emu);
	int hi=0;
	int lo=0;

	switch(colourNum)
	{     
		case 0: hi = 1 ; lo = 0 ;break ;
    	case 1: hi = 3 ; lo = 2 ;break ;
    	case 2: hi = 5 ; lo = 4 ;break ;
    	case 3: hi = 7 ; lo = 6 ;break ;
	}

	int colour=0;
	colour=((palette>>hi)&1)<<1;
	colour|=((palette>>lo)&1);
	switch(colour)
	{
		case 0: res = WHITE ;break ;
    	case 1: res = LIGHT_GRAY ;break ;
    	case 2: res = DARK_GRAY ;break ;
    	case 3: res = BLACK ;break ;
	}
	return res;
}

void RenderTiles(struct Emulator emu)
{
	uint16_t tileData=0;
	uint16_t bgMem=0;
	
	bool unsig=true;
	bool usingWindow=false;

	uint8_t lcdControl=read(0xFF40,emu);

	//localisation:
	uint8_t scrollY=read(0xFF42,emu);
	uint8_t scrollX=read(0xFF43,emu);
	uint8_t windowY=read(0xFF4A,emu);
	uint8_t windowX=read(0xFF4B,emu)-7;

	//fenêtre activée:
	if(lcdControl>>5&1 && windowY<= read(0xFF44,emu))
	{
		usingWindow=true;
	}
	if(lcdControl>>4&1)
	{
		tileData=0x8000;
	}
	else
	{
		tileData=0x8800;
		unsig=false; //rappel : on va naviguer dans les tiles avec -128 => +127
	}

	if(!usingWindow)
	{
		if(lcdControl>>3&1)
			bgMem=0x9C00;
		else
			bgMem=0x9800;
	}
	else
	{
		if(lcdControl>>6&1)
			bgMem=0x9C00;
		else
			bgMem=0x9800;		
	}

	uint8_t yPos=0; //pour calculer quelle ligne se fait afficher
	if(!usingWindow)
		yPos=scrollY+read(0xFF44,emu);
	else
		yPos=read(0xFF44,emu)-windowY;

	//Quelle position dans la tile actuelle
	uint16_t tileRow=((uint16_t)(yPos/8)*32); //TODO: OK?
	uint16_t xPos, tileCol, tileAddr, tileLoc;
	int16_t tileNum;
	for(int pixel=0;pixel<160;pixel++)
	{
		xPos=pixel+scrollX;
		if(usingWindow && pixel>=windowX)
		{
			xPos=pixel-windowX;
		}
		
		tileCol=(xPos/8);
		tileAddr=bgMem+tileRow+tileCol;
		
		if(unsig)
		{
			tileNum=(uint8_t)read(tileAddr,emu);//TODO: doit être positif
			//printf("tileNum est positif? %d\n",tileNum);
		}
		else
			tileNum=(int16_t)read(tileAddr,emu); //TODO: revoir si c'est vraiment signé
		
		tileLoc=tileData;
		
		if(unsig)
			tileLoc+=(tileNum*16);
		else
			tileLoc+=(tileNum+128)*16;

		uint8_t line=yPos%8;
		line*=2; //2 octets de mémoire par ligne verticale
		uint8_t data1=read(tileLoc+line,emu);
		uint8_t data2=read(tileLoc+line+1,emu);
		
		int colourBit=xPos%8;
		colourBit-=7;
		colourBit*=-1;

		//id de la couleur
		int colourNum=(data2>>colourBit)&1;
		colourNum<<=1;
		colourNum |= (data1>>colourBit)&1;

		uint8_t col=GetColour(colourNum,0xFF47,emu);
		int r,g,b;
		switch(col)
		{
			case WHITE: r=255; g=255; b=255; break;
			case LIGHT_GRAY:r=0xCC;g=0xCC;b=0xCC;
			case DARK_GRAY:r=0x77;g=0x77;b=0x77;
		}
		int finally=read(0xFF44,emu);
		emu.ScreenData[pixel][finally][0]=r;
		emu.ScreenData[pixel][finally][1]=g;
		emu.ScreenData[pixel][finally][2]=b;
	}
}
void RenderSprites(struct Emulator emu)
{
	uint8_t lcdControl=read(0xFF40);
	bool use8x16=false;
	if((lcdControl>>2)&1)
		use8x16=true;

	uint8_t index, yPos, xPos, tileLocation, attributes;
	bool yFlip, xFlip;
	int scanline;
	int ySize,line, colourBit, colourNum;
	uint16_t dataAddr, colourAddr;
	uint8_t data1,data2,col;
	for(int sprite=0;sprite<40;sprite++)
	{
		index=sprite*4;
		yPos=read(0xFE00+index,emu)-16;
		xPos=read(0xFE00+index+1,emu)-8;
		tileLocation=read(0xFE00+index+2,emu);
		attributes=read(0xFE00+index+3,emu);
		scanline=read(0xFF44,emu);

		ySize=(use8x16)?16:8;
		yFlip=(attributes>>6&1);
		xFlip=(attributes>>5&1);

		if((scanline)>=yPos &&(scanline<(yPos+ySize)))
		{
			line=scanline-yPos;
			if(yFlip)
			{
				line-=ySize;
				line*=-1;
			}
			line*=2;
			dataAddr=(0x8000+(tileLocation*16))+line;
			data1=read(dataAddr,emu);
			data2=read(dataAddr+1,emu);

			for(int tilePixel=7;tilePixel>=0;tilePixel--)
			{
				colourBit=tilePixel;
				if(xFlip)
				{
					colourBit-=7;
					colourBit*=-1;
				}
				colourNum=((data2>>colourBit)&1);
				colourNum<<=1;
				colourNum|=((data1>>colourBit)&1);
				
				colourAddr=((attributes>>4)&1)?0xFF49:0xFF48;

				col=GetColour(colourNum,colourAddr,emu);
				int r=0;
				int g=0;
				int b=0;
				switch(col)
				{
					case WHITE:r=255;g=255;b=255;break;
					case LIGHT_GRAY:r=0xCC;g=0xCC;b=0xCC;break;
					case DARK_GRAY:r=0x77;g=0x77;b=0x77;break;
				}
				int xPix=0-tilePixel;
				xPix+=7;
				int pixel=xPos+xPix;
				emu.ScreenData[pixel][scanline][0]=r;
				emu.ScreenData[pixel][scanline][1]=g;
				emu.ScreenData[pixel][scanline][2]=b;
			}
		}
	}
}

void DrawScanLine(struct Emulator emu)
{
	uint8_t control=read(0xFF40,emu);
	if(control&1)
	{
		RenderTiles(emu);
	}
	if(control>>1&1)
	{
		RenderSprites(emu);
	}
}

bool IsLCDEnabled(struct Emulator emu)
{
	uint8_t test=read(0xFF40,emu);
	return (test>>7&1);
}

void SetLCDStatus(struct Emulator emu)
{
	uint8_t status=read(0xFF41,emu);
	if(!IsLCDEnabled(emu))
	{
		emu.scanlineCounter=456;
		emu.mem[0xFF44]=0;
		status &=252;
		status=status|1;
		write(0xFF41,status,emu);
		return;
	}

	uint8_t currentLine=read(0xFF44,emu);
	uint8_t currentMode=status & 0x3; // xxxxxxxx & 00000011

	uint8_t mode=0;
	bool reqInt=false; //request interrupt

	if(currentLine>=144)
	{
		//mode 1
		mode=1;
		status=status|1;
		status=status&0b11111101;
		reqInt=status>>4 & 1;
	}
	else
	{	
		//MODE2
		if(emu.scanlineCounter>=MODE2BOUNDS)
		{
			mode =2;
			status=status|2;
			status=status&0b11111110;
			reqInt=status>>5 & 1;
		}
		//MODE3
		else if(emu.scanlineCounter>=MODE3BOUNDS)
		{
			mode=3;
			status=status|0b00000011;
		}
		//MODE0
		else
		{
			mode=0;
			status=status&0b11111100;
			reqInt=status>>3 & 1;
		}
	}
	//Nouveau mode
	if(reqInt&&(mode!=currentMode))
		RequestInterrupt(1,emu);
	//coincidence
	if(read(0xFF44,emu)==read(0xFF45,emu))
	{
		status=status|0b00000100;
		if((status>>6&1)==1)
			RequestInterrupt(1,emu);
	}
	else
	{
		status=status&0b11111011;
	}
	write(0xFF41,status,emu);
}


void UpdateGraphics(uint8_t cycles, struct Emulator emu)
{
	SetLCDStatus();

	if(IsLCDEnabled())
		emu.scanlineCounter-=cycles;
	else
		return;

	if(emu.scanlineCounter<=0)
	{
		inc_mem(0xFF44); //prochaine ligne
		uint8_t currentLine=read(0xFF44,emu);//actualisation de la ligne
		emu.scanlineCounter=456;

		//VBLANK
		if(currentLine==144)
			RequestInterrupt(0,emu);
		else if(currentLine>153)
			write(0xFF44,0,emu);
		else if(currentLine<144) //dessine la ligne
			DrawScanLine(emu);
	}


}