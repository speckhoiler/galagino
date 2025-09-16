#include "Arduino.h"
#include "emulation.h"
#include "input.h"
#include "..\machines\machineBase.h"

// including of "..\machines\machineBase.h" in "emulation.h" not possible
TaskHandle_t emulationTaskHandle;
extern machineBase *currentMachine;
extern Input input;

void emulation_start() {
  currentMachine->reset();
  xTaskCreatePinnedToCore(emulation_task, "emulation task", 4096, NULL, 2, &emulationTaskHandle, 1);
}

void emulation_stop() {
  if (emulationTaskHandle == NULL)
    return;
  
  input.disable(); // disable input read from nunchuck
  
  vTaskDelete(emulationTaskHandle);
  emulationTaskHandle = NULL;
  currentMachine->reset();  // clear sound output

  input.enable(); // enable input read from nunchuck
}

void emulation_notifyGive() {
  if (emulationTaskHandle == NULL)
    return;

  xTaskNotifyGive(emulationTaskHandle);
}

void emulation_task(void *p) {  
  while(1) {
    emulation_frame();
  }
}

void emulation_frame() {
  // It may happen that the emulation runs too slow. It will then miss the
  // vblank notification and in turn will miss a frame and significantly
  // slow down. This risk is only given with Galaga as the emulation of
  // all three CPUs takes nearly 13ms. The 60hz vblank rate is in turn 
  // 16.6 ms.
  currentMachine->run_frame();

  // Wait for signal from video task to emulate a 60Hz frame rate. Don't do
  // this unless the game has actually started to speed up the boot process
  // a little bit.
  if(currentMachine->game_started)   
    ulTaskNotifyTake(1, 0xffffffffUL);
  else
    vTaskDelay(1); // give a millisecond delay to make the watchdog happy
}

unsigned char OpZ80_INL(unsigned short Addr) {
  return currentMachine->opZ80(Addr);
}

void OutZ80(unsigned short Port, unsigned char Value) {
  currentMachine->outZ80(Port, Value);
}
  
unsigned char InZ80(unsigned short Port) {
  return currentMachine->inZ80(Port);
}

void WrZ80(unsigned short Addr, unsigned char Value) {
  currentMachine->wrZ80(Addr, Value);
}

unsigned char RdZ80(unsigned short Addr) {
  return currentMachine->rdZ80(Addr);
}

void PatchZ80(Z80 *R) {
}

void i8048_port_write(i8048_state_S *state, unsigned char port, unsigned char pos) {
  currentMachine->wrI8048_port(state, port, pos);
}

unsigned char i8048_port_read(i8048_state_S *state, unsigned char port) {
  return currentMachine->rdI8048_port(state, port);
}

unsigned char i8048_rom_read(i8048_state_S *state, unsigned short addr) {
  return currentMachine->rdI8048_rom(state, addr);
}

unsigned char i8048_xdm_read(i8048_state_S *state, unsigned char addr) {
  return currentMachine->rdI8048_xdm(state, addr);
}

void i8048_xdm_write(i8048_state_S *state, unsigned char addr, unsigned char data) {
}