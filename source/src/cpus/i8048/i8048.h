#ifndef _I8048_H_
#define _I8048_H_

#ifdef __cplusplus
extern "C" {
#endif

#define false 0
#define true (!false)

#ifndef boolean
  #define boolean char
#endif

// Bits in PSW
#define CY_BIT  7
#define AC_BIT  6
#define F0_BIT  5
#define BS_BIT  4

#define REGISTER_BANK_0_BASE 0
#define REGISTER_BANK_1_BASE 24

struct i8048_state_S {
  // Interrupt pins and flipflops
  boolean TF; // Timer Flag
  boolean notINT;
  boolean timerInterruptRequested;
  boolean T0;
  boolean T1;
  
  unsigned char T;
  unsigned char A;
  unsigned short PC;
  unsigned char PSW;
  boolean DBF;
  boolean F1;

  boolean externalInterruptsEnabled;
  boolean tcntInterruptsEnabled;
  boolean counterRunning; // Whether counter is bound to T1 (STRT CNT)
  boolean timerRunning; // Whether counter is bound to clock (STRT T)
  long cyclesUntilCount; // prescaler: Number of cycles until we need to increment the count (if counter is bound to clock)
  boolean inInterrupt; // True if handling an interrupt. Reset by RETR

  unsigned char ram[128];

  int p2_state;
};

void i8048_reset(struct i8048_state_S *state);
void i8048_step(struct i8048_state_S *state);

unsigned char i8048_rom_read(struct i8048_state_S *state, unsigned short addr);

// ----- functions to be provided externally -----
void i8048_port_write(struct i8048_state_S *, unsigned char, unsigned char);
unsigned char i8048_port_read(struct i8048_state_S *, unsigned char);

unsigned char i8048_xdm_read(struct i8048_state_S *, unsigned char);
void i8048_xdm_write(struct i8048_state_S *, unsigned char, unsigned char);

#ifdef __cplusplus
}
#endif

#endif // _I8048_H_
