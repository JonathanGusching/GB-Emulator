#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdlib.h>
#include "interrupts.h"
#define TIMA 0xFF05
#define TMA 0xFF06
#define TMC 0xFF07
//Pour usleep:
//#include <unistd.h>
//clock speed=4,194,304Hz
// period : 2.38418579*10^(-7)s =2.38418579 us

/*2^16=65536
*MEMORY:65536
*ROM0 :0000 => 						3FFF 	FIXE PAR LA CARTOUCHE
*ROM1 : 4000 => 					7FFF 	FIXE PAR LA CARTOUCHE
*VRAM: 8000 => 						9FFF
*EXT RAM: A000 => 					BFFF
*WRAM0: C000 => 					CFFF
*WRAM1: D000 => 					DFFF
*ECHO RAM: E000 => 					FDFF 	(PAS UTILISEE)
*OAM (Sprites Attr. Table.):FE00 => FE9F
*FEA0=>								FEFF 	(PAS UTILISABLE)
*I/O: FF00 => 						FF7F
*HRAM: FF80 => 						FFFE
*FFFF = INTERRUPTS ENABLE REGISTER (IE)
*/
//VARIABLE GLOBALE:
/*
uint8_t mem[0x200000]={0}; //16 bits
//A = accumulateur, F= flag, 8 bits chacun
uint8_t a=0; //111

//INACCESSIBLE PAR LE PROGRAMMEUR = flags
uint8_t f;

//idem
uint8_t b=0;//000
uint8_t c=0;//001

//2x8 bits pour stocker quelques données, idem à BC
uint8_t d=0;//101
uint8_t e=0;//011

//Enregistrement d'adresses et utilisation libre
uint8_t h=0;//100
uint8_t l=0;//101

//Stack pointer = adresse basse de la pile
uint16_t sp=0xFFFE;
//PROGRAM COUNTER
uint16_t pc = 0x0100;

uint8_t op=0;
bool MBC1=false;
bool MBC2=false;

//Interrupt Master Enabled :
bool ime=0;
bool ime_scheduled=false;
*/

//Variables temporaires
uint16_t temp_ptr=0x0000;
uint16_t nn;
uint16_t carry;
uint8_t carry8;
uint8_t n;
uint8_t bit7;
uint8_t bit0;
int8_t signed_n;
uint8_t c3;
uint8_t c7;
uint8_t borrow, borrow4;

int16_t signed_nn;

bool interrupt=false;
#define GAME "Tetris.gb"
#define CLOCK_PERIOD 2.38418579
#define LONGUEUR 160
#define HAUTEUR 144
#define RATIO 3
//usleep(...)
/* lit 8 bits */
extern int timerCounter;

void CP(uint8_t n_in,struct Emulator emu)
{
	borrow4=((a<<4)>=(n_in<<4))<<5;
	//borrow=(a-n_in>=0)?0b00010000:0b00000000;
	f=(a<n_in)?0b01010000:0b01000000;
	f=f|(a==n_in)<<7|borrow4;
}
void DEC(uint8_t *reg)
{
	printf("REG %d\n",*reg);
	if(*reg<<4!=0)
		f|=0b00100000;
	else
		f&=0b11011111;
	(*reg)--;
	if((*reg)==0)
	{
		f|=0b10000000;
		//printf("%0x\n",f);
		//exit(0);
	}
	else
	{
		f&=0b01111111;
	}
}
void Initialize(struct emulator emu)
{
	/*
	for(int i=0; i<160;i++)
		{
			for(int j=0;j<144;j++)
			{
				for(int k=0;k<3;k++)
				{
					ScreenData[i][j][k]=255;
				}
			}
		}*/
	emu.ime=true;
	emu.timerCounter=1024;
	emu.pc=0x0100;
	emu.a=0x01;
	emu.f=0xb0;
	emu.b=0x00;
	emu.c=0x13;
	emu.h=0x01;
	emu.l=0x4D;
	emu.d=0x00;
	emu.e=0xD8;
	emu.sp=0xFFFE;
	emu.mem[0xFF05] = 0x00 ;
	emu.mem[0xFF06] = 0x00 ;
	emu.mem[0xFF07] = 0x00 ;
	emu.mem[0xFF10] = 0x80 ;
	emu.mem[0xFF11] = 0xBF ;
	emu.mem[0xFF12] = 0xF3 ;
	emu.mem[0xFF14] = 0xBF ;
	emu.mem[0xFF16] = 0x3F ;
	emu.mem[0xFF17] = 0x00 ;
	emu.mem[0xFF19] = 0xBF ;
	emu.mem[0xFF1A] = 0x7F ;
	emu.mem[0xFF1B] = 0xFF ;
	emu.mem[0xFF1C] = 0x9F ;
	emu.mem[0xFF1E] = 0xBF ;
	emu.mem[0xFF20] = 0xFF ;
	emu.mem[0xFF21] = 0x00 ;
	emu.mem[0xFF22] = 0x00 ;
	emu.mem[0xFF23] = 0xBF ;
	emu.mem[0xFF24] = 0x77 ;
	emu.mem[0xFF25] = 0xF3 ;
	emu.mem[0xFF26] = 0xF1 ;
	emu.mem[0xFF40] = 0x91 ;
	emu.mem[0xFF42] = 0x00 ;
	emu.mem[0xFF43] = 0x00 ;
	emu.mem[0xFF45] = 0x00 ;
	emu.mem[0xFF47] = 0xFC ;
	emu.mem[0xFF48] = 0xFF ;
	emu.mem[0xFF49] = 0xFF ;
	emu.mem[0xFF4A] = 0x00 ;
	emu.mem[0xFF4B] = 0x00 ;
	emu.mem[0xFFFF] = 0x00 ; 
}
void READ_GAME(char* game_name)
{
	FILE* f=fopen(game_name,"rb");
	//uint16_t buff=0x0000;
    if ( f == NULL ) {
        printf( "Cannot open file %s\n", game_name );
        exit( 0 );
    }
	fread(mem,1,0x200000,f); //CHECK
	fclose(f);
}


uint8_t padState=0;
void KeyPressed(int key)
{
	bool prevUnset=false;
	if(!(padState>>key&1))
	{
		prevUnset=true;
	}
	padState=padState&(~(1<<key));
	bool button=true;
	if(key>3)
		button=true;
	uint8_t keyReq=mem[0xFF00];
	bool reqInt=false;
	if(button&&!(keyReq>>5&1))
		reqInt=true;
	else if(!button &&(!(keyReq>>5&1)))
		reqInt=true;
	if(reqInt&&!prevUnset)
		RequestInterrupt(4);
}

void KeyReleased(int key)
{
	padState=padState|(1<<key);
}

uint8_t GetJoypadState()
{
   uint8_t res = mem[0xFF00] ;
   // flip all the bits
   res ^= 0xFF ;

   // are we interested in the standard buttons?
   if (!(res>>4&1))
   {
     uint8_t topJoypad = padState >> 4 ;
     topJoypad |= 0xF0 ; // turn the top 4 bits on
     res &= topJoypad ; // show what buttons are pressed
   }
   else if (!(res>>4&1))//directional buttons
   {
     uint8_t bottomJoypad = padState & 0xF ;
     bottomJoypad |= 0xF0 ;
     res &= bottomJoypad ;
   }
   return res ;
}

uint8_t read(uint16_t addr)
{
	if(addr==0xFF00)
	{
		printf("Joypad");
		GetJoypadState();
		return mem[0xFF00];
	}
	/*else if(addr>=0x4000 && addr<=0x7FFF)
	{
		uint8_t newAddr=addr-0x4000;
		return mem[newAddr];
	}*/
	else
		return mem[addr];
	//BOUCHON
	//TO DO
	//return 0x10;
}


void DoDMATransfer(uint8_t data)
{
	printf("DMA transfer\n");
	uint16_t addr = data<<8;
	for(int i=0;i<0xA0;i++)
	{
		//write(0xFE00+i,read(addr+i));

		mem[0xFE00+i]=mem[addr+i];
	}
}

void write(uint16_t addr, uint8_t val)
{
	//printf("Tentative d'écriture de %0x à l'adresse %0x\n",val,addr);
	if(addr <0x8000) //READ ONLY
	{
		printf("Error: tried to write at address %0x which is read only\n",addr);
		//mem[addr]=val;
		exit(1);
	}
	else if((addr >=0xE000)&& (addr < 0xFE00)) //mémoire ECHO
	{
		mem[addr]=val;
		mem[addr-0x2000]=val;
	}
	else if((addr>=0xFEA0) && (addr <0xFEFF))
	{
		printf("Error: tried to write restricted area %0x\n",addr);
		//mem[addr]=val;
		exit(1);
	}
	else if(addr==0xFF44) //scanline address
	{
		mem[addr]=0;
	}
	else if(addr==0xFF46) //DMA transfer
	{
		DoDMATransfer(val);
	}
	else if(addr==0xFF04)
	{
		mem[0xFF04]=0;
	}
	else if(addr==TMC)
	{
		uint8_t currFreq=GetClockFreq();
		mem[TMC]=val;
		uint8_t newFreq=GetClockFreq();
		if(currFreq!=newFreq)
			SetClockFreq();
	}
	else //pas de contrôle
	{
		mem[addr]=val;
	}
}

void inc_mem(uint16_t addr)
{
	mem[addr]++;
}
void dec_mem(uint16_t addr)
{
	mem[addr]--;
}
void waitUntilInterrupt()
{
	//SDL_Event event;
	//TODO écrire la fonction pour tester les interruptions
	//while(!interrupt)
	//{
		//attendre
	//}
}


//TODO : revoir ça
void PUSH(uint16_t addr)
{
	printf("PUSH");
	sp--;
	uint8_t temp=addr>>8;
	write(sp--,addr);
	write(sp,temp);
}
uint16_t POP()
{
	printf("POP\n");
	uint8_t n_temp=read(sp++);
	uint16_t nn_temp=n_temp|read(sp++)<<8;
	return nn_temp;

}
void JUMP(uint16_t addr)
{
	//PUSH(pc);
	pc=addr;
}

int CallCB()
{
	//TODO
	int cycles=1;
	printf("Extended Operations\n");
	return cycles;
}

int fetch(uint8_t op)
{
	if(op!=0)
		printf("OPCODE: %0x\n",op);
	int cycles=1;
		switch(op)
		{
			/*MISC SECTION*/
			case 0x00:
				//NOP
				break;

			case 0x76:
				//HALT
				//TODO: Attendre jusqu'à interrupt
				printf("Wait\n");
				waitUntilInterrupt();
				break;
				//return false;
			case 0x10:
				///STOP
				return -1;
				break;

			/*LD nn,n SECTION */
			case 0x06:
				//LD B,n
				b=read(pc++);
				cycles=2;
				break;
			case 0x0E:
				//LD C,n
				c=read(pc++);
				cycles=2;
				break;
			case 0x16:
				//LD D,n
				d=read(pc++);
				cycles=2;
				break;
			case 0x1E:
				//LD E,n
				e=read(pc++);
				cycles=2;
				break;
			case 0x26:
				//LD H,n
				h=read(pc++);
				cycles=2;
				break;
			case 0x2E:
				//LD L,n
				l=read(pc++);
				cycles=2;
				break;

			/* LD r1,r2 section */
			case 0x7F:
				//LD A,A
				//nothing
				break;
			case 0x78:
				//LD A,B
				a=b;
				break;
			case 0x79:
				//LD A,C
				a=c;
				break;
			case 0x7A:
				//LD A,D
				a=d;
				break;
			case 0x7B:
				//LD A,E
				a=e;
				break;
			case 0x7C:
				//LD A,H
				a=h;
				break;
			case 0x7D:
				//LD A,L
				a=l;
				break;
			case 0x7E:
				//LD A,(HL)
				a=read(h<<8|l);
				cycles=2;
				break;

			case 0x0A:
				//LD A,(BC)
				a=read(b<<8|c);
				cycles=2;
				break;
			case 0x1A:
				//LD A,(DE)
				a=read(d<<8|e);
				cycles=2;
				break;
			//TODO: à revoir (pc++?)
			case 0xFA:
				//LD A,(nn)
				temp_ptr=pc++;
				nn=read(pc++)<<8|read(temp_ptr);
				a=read(nn);
				cycles=4;
				break;
			case 0x3E:
				//LD A, n
				a=read(pc++);
				cycles=2;
				break;

			case 0x47:
				//LD B,A
				b=a;
				break;
			case 0x4F:
				//LD C,A
				c=a;
				break;
			case 0x57:
				//LD D,A
				d=a;
				break;
			case 0x5F:
				//LD E,A
				e=a;
				break;
			case 0x67:
				//LD H,A
				h=a;
				break;
			case 0x6F:
				//LD L,A
				l=a;
				break;

			case 0x02:
				//LD (BC),A
				write(b<<8|c,a);
				cycles=2;
				break;
			case 0x12:
				//LD (DE),A
				write(d<<8|e,a);
				cycles=2;
				break;
			case 0x77:
				//LD (HL),A
				write(h<<8|l,a);
				cycles=2;
				break;
			case 0xEA:
				//LD (nn),A
				temp_ptr=pc++;
				printf("LD PC: %d\n",pc);
				nn=read(pc++)<<8|read(temp_ptr);
				printf("nn: %d\n",nn);
				printf("LD PC: %d\n",pc);
				write(nn,a);
				cycles=4;
				break;

			case 0xF2:
				//LD A,(C)
				a=read((0xFF<<8)|c);
				cycles=2;
				break;
			case 0xE2:
				//LD (C),A
				write((0xFF<<8)|c,a);
				cycles=2;
				break;
			case 0xF0:
				//LD A,(n)
				n=read(pc++);
				a=read(0xFF00+n);
				printf("%0x\n",a);
				cycles=3;
				break;
			case 0xE0:
				//LD (n),A
				n=read(pc++);
				write(0xFF00+n,a);
				cycles=3;
				break;

			case 0x3A:
				//LD A,(HL-)
				nn=h<<8|l;
				a=read(nn);
				nn--;
				h=nn>>8;
				l=nn;
				cycles=2;
				break;
			case 0x32:
				//LD (HL-),A
				nn=h<<8|l;
				write(nn,a);
				nn--;
				h=nn>>8;
				l=nn;
				cycles=2;
				break;

			case 0x2A:
				//LD A,(HL+)
				nn=h<<8|l;
				a=read(nn);
				nn++;
				h=nn>>8;
				l=nn;
				cycles=2;
				break;
			case 0x22:
				//LD (HL+),A
				nn=h<<8|l;
				write(nn,a);
				nn++;
				h=nn>>8;
				l=nn;
				cycles=2;
				break;

			case 0x01:
				//LD BC,nn
				temp_ptr=pc++;
				nn=read(pc++)<<8|read(temp_ptr);
				c=nn;
				b=nn>>8;
				cycles=3;
				break;
			case 0x11:
				//LD DE,nn
				temp_ptr=pc++;
				nn=read(pc++)<<8|read(temp_ptr);
				e=nn;
				d=nn>>8;
				cycles=3;
				break;
			case 0x21:
				//LD HL,nn
				temp_ptr=pc++;
				nn=(read(pc++)<<8)|read(temp_ptr);
				l=nn;
				h=nn>>8;
				cycles=3;
				break;
			case 0x31:
				//LD SP,nn
				temp_ptr=pc++;
				sp=read(pc++)<<8|read(temp_ptr);
				cycles=3;
				break;
			case 0xF9:
				//LD SP,HL
				sp=h<<8|l;
				cycles=2;
				break;

			//TODO: à revoir
			case 0xF8:
				//LDHL SP,n
				signed_n=read(pc++);
				carry=sp&signed_n;
				printf("hehe");
				uint16_t c11=0b0000100000000000&carry;
				uint16_t c15=0b1000000000000000&carry;
				nn=sp+signed_n;
				l=nn;
				h=nn>>8;
				f=(0b0000|c11>>8|c15>>14)<<4; //actualisation des flags
				cycles=3;
				break;

			case 0x40:
				//LD B,B
				printf("nothing\n");
				break;
			case 0x41:
				//LD B,C
				b=c;
				break;
			case 0x42:
				//LD B,D
				b=d;
				break;
			case 0x43:
				//LD B,E
				b=e;
				break;
			case 0x44:
				//LD B,H
				b=h;
				break;
			case 0x45:
				//LD B,L
				b=l;
				break;
			case 0x46:
				//LD B,(HL)
				b=read(h<<8|l);
				cycles=2;
				break;
			case 0x48:
				//LD C,B
				c=b;
				break;
			case 0x49:
				//LD C,C
				break;
			case 0x4A:
				//LD C,D
				c=d;
				break;
			case 0x4B:
				//LD C,E
				c=e;
				break;
			case 0x4C:
				//LD C,H
				c=h;
				break;
			case 0x4D:
				//LD C,L
				c=l;
				break;
			case 0x4E:
				//LD C,(HL)
				c=read(h<<8|l);
				cycles=2;
				break;

			case 0x50:
				//LD D,B
				d=b;
				break;
			case 0x51:
				//LD D,C
				d=c;
				break;
			case 0x52:
				//LD D,D
				break;
			case 0x53:
				//LD D,E
				d=e;
				break;
			case 0x54:
				//LD D,H
				d=h;
				break;
			case 0x55:
				//LD D,L
				d=l;
				break;
			case 0x56:
				//LD D,(HL)
				d=read(h<<8|l);
				cycles=2;
				break;		

			case 0x58:
				//LD E,B
				e=b;
				break;
			case 0x59:
				//LD E,C
				e=c;
				break;
			case 0x5A:
				//LD E,D
				e=d;
				break;
			case 0x5B:
				//LD E,E
				break;
			case 0x5C:
				//LD E,H
				e=h;
				break;
			case 0x5D:
				//LD D,L
				e=l;
				break;
			case 0x5E:
				//LD D,(HL)
				e=read(h<<8|l);
				cycles=2;
				break;


			case 0x60:
				//LD H,B
				h=b;
				break;
			case 0x61:
				//LD H,C
				h=c;
				break;
			case 0x62:
				//LD H,D
				h=d;
				break;
			case 0x63:
				//LD H,E
				h=e;
				break;
			case 0x64:
				//LD H,H
				break;
			case 0x65:
				//LD H,L
				h=l;
				break;
			case 0x66:
				//LD H,(HL)
				h=read(h<<8|l);
				cycles=2;
				break;

			case 0x68:
				//LD L,B
				l=b;
				break;
			case 0x69:
				//LD L,C
				l=c;
				break;
			case 0x6A:
				//LD L,D
				l=d;
				break;
			case 0x6B:
				//LD L,E
				l=e;
				break;
			case 0x6C:
				//LD L,H
				l=h;
				break;
			case 0x6D:
				//LD L,L
				break;
			case 0x6E:
				//LD L,(HL)
				l=read(h<<8|l);
				cycles=2;
				break;

			case 0x70:
				//LD (HL),B
				write(h<<8|l,b);
				cycles=2;
				break;
			case 0x71:
				//LD (HL),C
				write(h<<8|l,c);
				cycles=2;
				break;
			case 0x72:
				//LD (HL),D
				write(h<<8|l,d);
				cycles=2;
				break;
			case 0x73:
				//LD (HL),E
				write(h<<8|l,e);
				cycles=2;
				break;
			case 0x74:
				//LD (HL),H
				write(h<<8|l,h);
				cycles=2;
				break;
			case 0x75:
				//LD (HL),L
				write(h<<8|l,l);
				cycles=2;
				break;

			case 0x36:
				//LD (HL),n
				write(h<<8|l,read(pc++));
				cycles=3;
				break;
			case 0xF5:
				//PUSH AF
				sp--;
				write(sp--,a);
				write(sp,f);
				cycles=4;
				break;
			case 0xC5:
				//PUSH BC
				sp--;
				write(sp--,b);
				write(sp,c);
				cycles=4;
				break;
			case 0xD5:
				//PUSH DE
				sp--;
				write(sp--,d);
				write(sp,e);
				cycles=4;
				break;
			case 0xE5:
				//PUSH HL
				sp--;
				write(sp--,h);
				write(sp,l);
				cycles=4;
				break;

			case 0xF1:
				//POP AF
				sp++;
				f=read(sp++);
				a=read(sp);
				cycles=3;
				break;
			case 0xC1:
				//POP BC
				sp++;
				c=read(sp++);
				b=read(sp);
				cycles=3;
				break;
			case 0xD1:
				//POP DE
				sp++;
				e=read(sp++);
				d=read(sp);
				cycles=3;
				break;
			case 0xE1:
				//POP HL
				sp++;
				l=read(sp++);
				h=read(sp);
				cycles=3;
				break;
			//ALU:
			case 0x87:
				//ADD A,A
				carry8=a&a;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=a;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x80:
				//ADD A,B
				carry8=a&b;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=b;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x81:
				//ADD A,C
				carry8=a&c;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=c;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x82:
				//ADD A,D
				carry8=a&d;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=d;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x83:
				//ADD A,E
				carry8=a&e;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=c;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x84:
				//ADD A,H
				carry8=a&h;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=h;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x85:
				//ADD A,L
				carry8=a&l;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=l;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x86:
				//ADD A,(HL)
				nn=h<<8|l;
				n=read(nn);
				carry8=a&n;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=n;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				cycles=2;
				break;
			case 0xC6:
				//ADD A,#
				n=read(pc++);
				carry8=a&n;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=n;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				cycles=2;
				break;




			case 0x8F:
				//ADC A,A
				n=a+(0b00010000&f);
				carry8=a&n;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=n;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x88:
				//ADC A,B
				n=b+(0b00010000&f);
				carry8=a&n;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=n;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x89:
				//ADC A,C
				n=c+(0b00010000&f);
				carry8=a&n;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=n;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x8A:
				//ADC A,D
				n=d+(0b00010000&f);
				carry8=a&n;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=n;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x8B:
				//ADC A,E
				n=e+(0b00010000&f);
				carry8=a&n;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=n;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x8C:
				//ADC A,H
				n=h+(0b00010000&f);
				carry8=a&n;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=n;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x8D:
				//ADC A,L
				n=l+(0b00010000&f);
				carry8=a&n;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=n;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				break;
			case 0x8E:
				//ADC A,(HL)
				nn=h<<8|l;
				n=read(nn)+(0b00010000&f);
				carry8=a&n;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=n;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				cycles=2;
				break;
			case 0xCE:
				//ADC A,#
				n=read(pc++)+(0b00010000&f);
				carry8=a&n;

				c3=0b00001000&carry8;
				c7=0b10000000&carry8;
				
				a+=n;
				
				f=(0b00000000|c3<<2|c7>>2); //actualisation des flags
				if(a==0) //Z bit
					f=f|0b10000000;
				cycles=2;
				break;

			//SUB n (from A)
			case 0x97:
				//SUB A
				a=0;
				f=0b11000000;
				break;
			case 0x90:
				//SUB B
				borrow4=((a<<4)>=(b<<4));
				a=a-b;
				borrow=(a>=0)?0b00010000:0b00000000;
				f=0b01000000|(a==0)<<7|borrow|borrow4<<5;
				break;
			case 0x91:
				//SUB C
				borrow4=((a<<4)>=(c<<4));
				a=a-c;
				borrow=(a>=0)?0b00010000:0b00000000;
				f=0b01000000|(a==0)<<7|borrow|borrow4<<5;
				break;
			case 0x92:
				//SUB D
				borrow4=((a<<4)>=(d<<4));
				a=a-d;
				borrow=(a>=0)?0b00010000:0b00000000;
				f=0b01000000|(a==0)<<7|borrow|borrow4<<5;
				break;
			case 0x93:
				//SUB E
				borrow4=((a<<4)>=(e<<4));
				a=a-e;
				borrow=(a>=0)?0b00010000:0b00000000;
				f=0b01000000|(a==0)<<7|borrow|borrow4<<5;
				break;
			case 0x94:
				//SUB H
				//borrow4=((a&0b00001000)|(~(n&0b00001000)));
				borrow4=((a<<4)>=(h<<4));
				a=a-h;
				borrow=(a>=0)?0b00010000:0b00000000;
				f=0b01000000|(a==0)<<7|borrow|borrow4<<5;
				break;
			case 0x95:
				//SUB L
				borrow4=((a<<4)>=(l<<4));
				a=a-l;
				borrow=(a>=0)?0b00010000:0b00000000;
				f=0b01000000|(a==0)<<7|borrow|borrow4<<5;
				break;
			case 0x96:
				//SUB (HL)
				n=read(h<<8|l);
				borrow4=((a<<4)>=(n<<4));
				a=a-n;
				borrow=(a>=0)?0b00010000:0b00000000;
				f=0b01000000|(a==0)<<7|borrow|borrow4<<5;
				cycles=2;
				break;
			case 0xD6:
				//SUB #
				n=read(pc++);
				borrow4=((a<<4)>=(n<<4));
				a=a-n;
				borrow=(a>=0)?0b00010000:0b00000000;
				f=0b01000000|(a==0)<<7|borrow|borrow4<<5;
				cycles=2;
				break;

			//TODO : RAJOUTER SBC

			//TODO : and bit à bit ou and logique?
			//AND n : A=A and n
			case 0xA7:
				//AND A
				f=(a!=0)?0b00100000:0b10100000;
				break;
			case 0xA0:
				//AND B
				a=a&b;
				f=(a!=0)?0b00100000:0b10100000;
				break;
			case 0xA1:
				//AND C
				a=a&c;
				f=(a!=0)?0b00100000:0b10100000;
				break;
			case 0xA2:
				//AND D
				a=a&d;
				f=(a!=0)?0b00100000:0b10100000;
				break;
			case 0xA3:
				//AND E
				a=a&e;
				f=(a!=0)?0b00100000:0b10100000;
				break;
			case 0xA4:
				//AND H
				a=a&h;
				f=(a!=0)?0b00100000:0b10100000;
				break;
			case 0xA5:
				//AND L
				a=a&l;
				f=(a!=0)?0b00100000:0b10100000;
				break;
			case 0xA6:
				//AND (HL)
				n=read(h<<8|l);
				a=a&n;
				f=(a!=0)?0b00100000:0b10100000;
				cycles=2;
				break;
			case 0xE6:
				//AND #
				n=read(pc++);
				a=a&n;
				f=(a!=0)?0b00100000:0b10100000;
				cycles=2;
				break;

			//OR n
			case 0xB7:
				//OR A
				f=(a==0)?0b10000000:0;
				break;
			case 0xB0:
				//OR B
				a=a|b;
				f=(a==0)?0b10000000:0;
				break;
			case 0xB1:
				//OR C
				a=a|c;
				f=(a==0)?0b10000000:0;
				break;
			case 0xB2:
				//OR D
				a=a|d;
				f=(a==0)?0b10000000:0;
				break;
			case 0xB3:
				//OR E
				a=a|e;
				f=(a==0)?0b10000000:0;
				break;
			case 0xB4:
				//OR H
				a=a|h;
				f=(a==0)?0b10000000:0;
				break;
			case 0xB5:
				//OR L
				a=a|l;
				f=(a==0)?0b10000000:0;
				break;
			case 0xB6:
				//OR (HL)
				n=read(h<<8|l);
				a=a|n;
				f=(a==0)?0b10000000:0;
				cycles=2;
				break;
			case 0xF6:
				//OR #
				n=read(pc++);
				a=a|n;
				f=(a==0)?0b10000000:0;
				cycles=2;
				break;

			//XOR n
			case 0xAF:
				//XOR A
				a=0;
				f=0b10000000;
				break;
			case 0xA8:
				//XOR B
				a=a^b;
				f=(a==0)?0b10000000:0;
				break;
			case 0xA9:
				//XOR C
				a=a^c;
				f=(a==0)?0b10000000:0;
				break;
			case 0xAA:
				//XOR D
				a=a^d;
				f=(a==0)?0b10000000:0;
				break;
			case 0xAB:
				//XOR E
				a=a^e;
				f=(a==0)?0b10000000:0;
				break;
			case 0xAC:
				//XOR H
				a=a^h;
				f=(a==0)?0b10000000:0;
				break;
			case 0xAD:
				//OR L
				a=a^l;
				f=(a==0)?0b10000000:0;
				break;
			case 0xAE:
				//XOR (HL)
				n=read(h<<8|l);
				a=a^n;
				f=(a==0)?0b10000000:0;
				cycles=2;
				break;
			case 0xEE:
				//XOR #
				n=read(pc++);
				a=a|n;
				f=(a==0)?0b10000000:0;
				cycles=2;
				break;
			//CP n
			//TODO: revoir les flags
			case 0xBF:
				//CP A
				f=0b1110000;
				break;

			case 0xB8:
				//CP B
				printf("CP B\n");
				CP(b);
				break;
			case 0xB9:
				//CP C
				printf("CP C\n");
				CP(c);
				break;
			case 0xBA:
				//CP D
				printf("CP D\n");
				CP(d);
				break;
			case 0xBB:
				//CP E
				printf("CP E\n");
				CP(e);
				break;
			case 0xBC:
				//CP H
				printf("CP H\n");
				CP(h);
				break;
			case 0xBD:
				//CP L
				printf("CP L\n");
				CP(l);
				break;
			case 0xBE:
				//CP (HL)
				printf("CP (HL)\n");
				n=read(h<<8|l);
				CP(n);
				cycles=2;
				break;
			//TODO: revoir les CP
			case 0xFE:
				//CP #
				printf("CP #\n");
				n=read(pc++);
				printf("n vaut :%0x\n",n);
				CP(n);
				cycles=2;
				break;

			//URGENT: TODO / TO DO: rajouter les flags
			//INC n
			case 0x3C:
				if((a&(a+1))&0b0001000)
					f|=0b00100000;
				a++;
				f=f&0b10111111; //reset N
				if(a==0)
				{
					f|=0b10000000;
				}
				break;
			case 0x04:
				if((b&(b+1))&0b0001000)
					f|=0b00100000;
				b++;
				f=f&0b10111111; //reset N
				if(b==0)
				{
					f|=0b10000000;
				}
				break;
			case 0x0C:
				if((c&(c+1))&0b0001000)
					f|=0b00100000;
				c++;
				f=f&0b10111111; //reset N
				if(c==0)
				{
					f|=0b10000000;
				}
				break;
			case 0x14:
				if((d&(d+1))&0b0001000)
					f|=0b00100000;
				d++;
				f=f&0b10111111; //reset N
				if(d==0)
				{
					f|=0b10000000;
				}
				break;
			case 0x1C:
				if((e&(e+1))&0b0001000)
					f|=0b00100000;
				e++;
				f=f&0b10111111; //reset N
				if(e==0)
				{
					f|=0b10000000;
				}
				break;
			case 0x24:
				if((h&(h+1))&0b0001000)
					f|=0b00100000;
				h++;
				f=f&0b10111111; //reset N
				if(h==0)
				{
					f|=0b10000000;
				}
				break;
			case 0x2C:
				if((l&(l+1))&0b0001000)
					f|=0b00100000;
				l++;
				f=f&0b10111111; //reset N
				if(l==0)
				{
					f|=0b10000000;
				}
				break;
			case 0x34:
				n=read(h>>8|l);
				if((n&(n+1))&0b0001000)
					f|=0b00100000;
				inc_mem(h>>8|l);
				n=n+1;
				f=f&0b10111111; //reset N
				if(n==0)
				{
					f|=0b10000000;
				}
				
				cycles=3;
				break;
			//URGENT TODO
			//DEC n
			case 0x3D:
				DEC(&a);
				break;
			case 0x05:
				DEC(&b);
				break;
			case 0x0D:
				DEC(&c);
				break;
			case 0x15:
				DEC(&d);
				break;
			case 0x1D:
				DEC(&e);
				break;
			case 0x25:
				DEC(&h);
				break;
			case 0x2D:
				DEC(&l);
				break;
			case 0x35:
				n=read(h<<8|l);
				DEC(&n);
				h=n>>8;
				l=n;
				cycles=3;
				break;
			
			//16-bit
			//ADD HL, n
			//TODO: flags
			case 0x09:
				nn=h<<8|l;
				nn=nn|(b<<8)|c;
				h=nn>>8;
				l=nn;
				cycles=2;
				break;
			case 0x19:
				nn=h<<8|l;
				nn=nn|(d<<8)|e;
				h=nn>>8;
				l=nn;
				cycles=2;
				break;
			case 0x29:
				nn=h<<8|l;
				nn=nn|(h<<8)|l;
				h=nn>>8;
				l=nn;
				cycles=2;
				break;
			case 0x39:
				nn=h<<8|l;
				nn=nn|sp;
				h=nn>>8;
				l=nn;
				cycles=2;
				break;

			//TODO: SP ou (SP) ?
			//ADD SP,n
			case 0xE8:
				n=read(sp++);
				sp+=n;
				f=0b00000000;
				cycles=4;
				break;

			//INC nn
			case 0x03:
				//INC BC
				n=read(b<<8|c);
				n++;
				b=n>>8;
				c=n;
				cycles=2;
				break;
			case 0x13:
				//INC DE
				n=read(d<<8|e);
				n++;
				d=n>>8;
				e=n;
				cycles=2;
				break;
			case 0x23:
				//INC HL
				n=read(h<<8|l);
				n++;
				h=n>>8;
				l=n;
				cycles=2;
				break;
			case 0x33:
				sp++;
				cycles=2;
				break;

			//DEC nn
			case 0x0B:
				//DEC BC
				nn=b<<8|c;
				nn--;
				b=nn>>8;
				c=nn;
				cycles=2;
				break;
			case 0x1B:
				//DEC DE
				nn=d<<8|e;
				nn--;
				d=nn>>8;
				e=nn;
				cycles=2;
				break;
			case 0x2B:
				//DEC HL
				nn=h<<8|l;
				nn--;
				h=nn>>8;
				l=nn;
				cycles=2;
				break;
			case 0x3B:
				//DEC SP
				sp--;
				cycles=2;
				break;	
			case 0xCB:
				cycles=CallCB();
				break;	

			//TODO : DAA

			//case 0x27:
			//CPL A
			case 0x2F:
				a=~(a);
				f|=0b01100000;
				break;
			case 0x3F:
			//CCF
				f&=0b10011111;
				if(f&16)
					f&=0b111011111;
				else
					f|=0b000100000;
				break;
			case 0x37:
			//SCF
				f&=0b10011111;
				f|=0b00010000;
				break;
			//TODO : pas sûr de la différence entre RLCA et RLA
			case 0x07:
				//RLCA : rotation A vers la gauche
				bit7=a&128;
				a=a<<1|bit7;
				f=(a==0)?0b10000000:0;
				f|=(bit7>>3);
				break;
			case 0x17:
				//RLA : rotation à travers la retenue
				bit7=a&128;
				a=a<<1|((f&16)<<3);//on récupère l'ancienne retenue
				f=(a==0)?0b10000000:0;
				f|=(bit7>>3);
				break;
			//Rotations droite
			case 0x0F:
				//RRCA : rotation A vers la droite
				bit0=a&1;
				a=a>>1|bit0;
				f=(a==0)?0b10000000:0;
				f|=(bit0<<4);
				break;
			case 0x1F:
				//RRA : rotation à travers la retenue
				bit0=a&1;
				a=a>>1|((f&16)>>4);//on récupère l'ancienne retenue
				f=(a==0)?0b10000000:0;
				f|=(bit0<<4);
				break;
			//Quelques trucs pour les interrupts:
			//TODO / TO DO: DI et EI après un cycle de plus, normalement...
			case 0xFB:
				//EI
				ime=true;
				ime_scheduled=true; //lance la procédure de lancement d'un interrupt
				break;
			case 0xF3:
				//DI : désactiver interrupt
				ime=false;
				ime_scheduled=false;
				break;
			case 0xC3:
				//JP nn
				n=read(pc++);
				nn=n|((read(pc)<<8));
				JUMP(nn);
				cycles=4;
				break;
			//Jumps conditionnels:
			//JP cc,nn
			case 0xC2:
				//JP NZ,nn
				if((~(f>>7))&1)
				{
					n=read(pc++);
					nn=n|(read(pc)<<8);
					pc=nn;
					cycles=4;
				}
				else
					cycles=3;
				break;
			case 0xCA:
				//JP Z,nn
				if(f>>7)
				{
					n=read(pc++);
					nn=n|(read(pc)<<8);
					pc=nn;
					cycles=4;
				}
				else
					cycles=3;
				break;
			case 0xD2:
				//JP NC,nn
				if((~(f>>4))&1)
				{
					n=read(pc++);
					nn=n|(read(pc)<<8);
					pc=nn;
					cycles=4;
				}
				else
					cycles=3;
				break;
			case 0xDA:
				//JP C,nn
				if(f>>4)
				{
					n=read(pc++);
					nn=n|(read(pc)<<8);
					pc=nn;
					cycles=4;
				}
				else
					cycles=3;
				break;
			//TODO: c'est bon?
			case 0xE9:
				//JP (HL)
				nn=h<<8|l;
				pc=nn;
				break;
			case 0x18:
				//JR n : ajouter n à l'adresse actuelle et JP
				signed_n=read(pc++);
				pc+=signed_n;
				cycles=2;
				break;

			//JR cc,nn
			//TODO: à finir:
			case 0x20:
				if((~(f>>7))&1)
				{
					signed_n=read(pc++);
					pc+=signed_n;
					cycles=3;
				}
				else
					cycles=2;
				break;
			case 0x28:
				if((f>>7))
				{
					signed_n=read(pc++);
					pc+=signed_n;
					cycles=3;
				}
				else
					cycles=2;
				break;
			case 0x30:
				if(((~(f>>4))&1)==1)
				{
					signed_n=read(pc++);
					pc+=signed_n;
					cycles=3;
				}
				else
					cycles=2;
				break;
			case 0x38:
				if(((f>>4)&1)==1)
				{
					printf("DSOQIDJOIQSDJIOSQDJ\n");
					signed_n=read(pc++);
					pc+=signed_n;
					cycles=3;
				}
				else
					cycles=2;
				break;
			//TODO: OK?
			//CALL nn
			case 0xCD:
				PUSH(pc++);
				n=read(pc++);
				nn=n|read(pc++)<<8;
				pc=nn;
				cycles=6;
				break;
			//CALL cc,nn
			case 0xC4:
				if((~(f>>7))&1)
				{
					PUSH(pc++);
					n=read(pc++);
					nn=n|read(pc++)<<8;
					pc=nn;

				}
				cycles=3;
				break;
			case 0xCC:
				if((f>>7))
				{
					PUSH(pc++);
					n=read(pc++);
					nn=n|read(pc++)<<8;
					pc=nn;
				}
				cycles=3;
				break;
			case 0xD4:
				if((~(f>>4))&1)
				{
					PUSH(pc++);
					n=read(pc++);
					nn=n|read(pc++)<<8;
					pc=nn;
				}
				cycles=3;
				break;
			case 0xDC:
				if((f>>4)&1)
				{
					PUSH(pc++);
					n=read(pc++);
					nn=n|read(pc++)<<8;
					pc=nn;
				}
				cycles=3;
				break;
			
			//Restarts:
			//RST n
			case 0xC7:
				//RST 00H
				PUSH(pc);
				pc=0;
				cycles=8;
				break;
			case 0xCF:
				//RST 08H
				PUSH(pc);
				pc=0x08;
				cycles=8;
				break;
			case 0xD7:
				//RST 10H
				PUSH(pc);
				pc=0x10;
				cycles=8;
				break;
			case 0xDF:
				//RST 08H
				PUSH(pc);
				pc=0x18;
				cycles=8;
				break;
			case 0xE7:
				//RST 20H
				PUSH(pc);
				pc=0x20;
				cycles=8;
				break;
			case 0xEF:
				//RST 28H
				PUSH(pc);
				pc=0x28;
				cycles=8;
				break;
			case 0xF7:
				//RST 30H
				PUSH(pc);
				pc=0x30;
				cycles=8;
				break;
			case 0xFF:
				//RST 38H
				PUSH(pc);
				pc=0x38;
				cycles=8;
				break;	

			//RET
			case 0xC9:
				pc=POP();
				cycles=2;
				break;

			//RET cc
			case 0xC0:
				//RET NZ
				if((~(f>>7))&1)
				{
					pc=POP();
				}
				cycles=2;
				break;
			case 0xC8:
				//RET Z
				if((f>>7))
				{
					pc=POP();
				}
				cycles=2;
				break;
			case 0xD0:
				//RET NC
				if((~(f>>4))&1)
				{
					pc=POP();
				}
				cycles=2;
				break;
			case 0xD8:
				//RET NC
				if((f>>4)&1)
				{
					pc=POP();
				}
				cycles=2;
				break;

			//RETI
			case 0xD9:
				pc=POP();
				cycles=2;
				ime=true;
				break;

			//compléter:
			case 0x08:
				//LD nn,SP
				n=read(pc++);
				nn=n|(read(pc++)<<8);
				sp=nn;
				cycles=5;
				break;
			default:
				printf("Unknown opcode %0x\n",op);
				cycles=0;
				break;
		}
	return cycles*4;
}