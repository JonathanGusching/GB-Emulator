#ifndef EMULATOR_H

#define EMULATOR_H
#include <stdint.h>
#include <stdbool.h>

#endif

struct Emulator{

	uint8_t mem[0x200000]; //16 bits
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
	uint16_t sp;
	//PROGRAM COUNTER
	uint16_t pc;
	bool MBC1;
	bool MBC2;

	//Interrupt Master Enabled :
	bool ime;
	bool ime_scheduled;

	int timerCounter;
	int dividerCounter;
	int dividerRegister;
	uint8_t ScreenData[160][144][3];
	int scanlineCounter;

};