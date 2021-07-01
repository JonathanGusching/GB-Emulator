#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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

#define CLOCK_PERIOD 2.38418579
//usleep(...)
/* lit 8 bits */
uint8_t read(uint16_t addr)
{
	//BOUCHON
	//TO DO
	return 0x10;
}

int main()
{
	//Problème, hors de la boucle => utiliser extern
	uint8_t mem[65536]; //16 bits

	printf("%d",mem[0]);
	//A = accumulateur, F= flag, 8 bits chacun
	uint8_t a; //111

	//INACCESSIBLE PAR LE PROGRAMMEUR = flags
	uint8_t f;

	//idem
	uint8_t b;//000
	uint8_t c;//001

	//2x8 bits pour stocker quelques données, idem à BC
	uint8_t d;//101
	uint8_t e;//011

	//Enregistrement d'adresses et utilisation libre
	uint8_t h;//100
	uint8_t l;//101

	//Stack pointer = adresse basse de la pile
	uint16_t sp=0xFFFE;
	//PROGRAM COUNTER
	uint16_t pc = 0x0100;

	uint8_t op;

	while(1)
	{
		//lecture instruction à l'adresse pc
		op=read(pc);
		//Boucle principale:
		switch(op)
		{
			/*MISC SECTION*/
			case 0x00:
				//NOP
				break;

			case 0x76:
				//HALT
				goto out;
			case 0x10:
				///STOP
				goto out;

			/*LD nn,n SECTION */
			case 0x06:
				//LD B,n
				b=read(pc++);
				break;
			case 0x0E:
				//LD C,n
				c=read(pc++);
				break;
			case 0x16:
				//LD D,n
				d=read(pc++);
				break;
			case 0x1E:
				//LD E,n
				e=read(pc++);
				break;
			case 0x26:
				//LD H,n
				h=read(pc++);
				break;
			case 0x2E:
				//LD L,n
				l=read(pc++);
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
				a=mem[l<<8|h];
				break;
			case 0x40:
				//LD B,B
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
				b=mem[l<<8|h];
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
				c=mem[l<<8|h];
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
				d=mem[l<<8|h];
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
				e=mem[l<<8|h];
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
				h=mem[l<<8|h];
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
				l=mem[l<<8|h];
				break;

			//TODO:
			case 0x70:
				//LD (HL),B
				mem[l<<8|h]=b;
				break;
			case 0x71:
				//LD (HL),C
				mem[l<<8|h]=c;
				break;
			case 0x72:
				//LD (HL),D
				mem[l<<8|h]=d;
				break;
			case 0x73:
				//LD (HL),E
				mem[l<<8|h]=e;
				break;
			case 0x74:
				//LD (HL),H
				mem[l<<8|h]=h;
				break;
			case 0x75:
				//LD (HL),L
				mem[l<<8|h]=l;
				break;

			case 0x36:
				//LD (HL),n
				mem[l<<8|h]=read(pc++);
				break;

			default:
				break;
		}
		pc++;
	}


	//FIN DE LA BOUCLE
	out:
	return 0;
}