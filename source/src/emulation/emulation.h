#ifndef _EMULATION_H_
#define _EMULATION_H_

#include "cpus/z80/Z80.h"
#include "cpus/i8048/i8048.h"

void emulation_start(void);
void emulation_stop(void);
void emulation_notifyGive(void);
void emulation_task(void *p);
void emulation_frame(void);

#ifdef __cplusplus
extern "C" void OutZ80(unsigned short Port, unsigned char Value);
extern "C" unsigned char InZ80(unsigned short Port);
extern "C" void WrZ80(unsigned short Addr, unsigned char Value);
extern "C" unsigned char RdZ80(unsigned short Addr);
extern "C" void StepZ80(Z80 *R);
extern "C" unsigned char OpZ80_INL(unsigned short Addr);
extern "C" void PatchZ80(Z80 *R);
#endif

#ifdef __cplusplus
extern "C" void i8048_reset(i8048_state_S *state);
extern "C" void i8048_step(i8048_state_S *state);

extern "C" void i8048_port_write(i8048_state_S *state, unsigned char port, unsigned char pos);
extern "C" unsigned char i8048_port_read(i8048_state_S *state, unsigned char port);
extern "C" unsigned char i8048_xdm_read(i8048_state_S *state, unsigned char addr);
extern "C" void i8048_xdm_write(i8048_state_S *state, unsigned char addr, unsigned char data);
extern "C" unsigned char i8048_rom_read(i8048_state_S *state, unsigned short addr);
#endif

#endif