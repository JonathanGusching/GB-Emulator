#include <stdint.h>
void RequestInterrupt(uint8_t id);
void ServiceInterrupt(int interrupt);
void DoInterrupt();