#include <stdint.h>
#include <stdbool.h>
#include "interrupts.h"
extern bool ime;
extern bool ime_scheduled;
extern uint8_t op;

extern uint8_t b;
extern uint8_t c;

extern uint16_t pc;
extern uint8_t mem[0x200000];

extern int timerCounter;
extern int dividerCounter;
extern int dividerRegister;

uint8_t read(uint16_t addr);
void DoDMATransfer(uint8_t data);
void READ_GAME(char* game_name);
void write(uint16_t addr, uint8_t val);
extern void inc_mem(uint16_t addr);
void waitUntilInterrupt();
bool fetch(uint8_t op);
void PUSH(uint16_t addr);
void Initialize();