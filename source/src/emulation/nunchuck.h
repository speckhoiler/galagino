#ifndef _NUNCHUCK_H_
#define _NUNCHUCK_H_

#include "../config.h"

#ifdef NUNCHUCK_INPUT
#include <Wire.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------
#include <NintendoExtensionCtrl.h>
// This library is for interfacing with the Nunchuck

// Can be installed from the library manager
// https://github.com/dmadison/NintendoExtensionCtrl

// a total of 7 button is needed for most games
#define BUTTON_LEFT  0x01
#define BUTTON_RIGHT 0x02
#define BUTTON_UP    0x04
#define BUTTON_DOWN  0x08
#define BUTTON_FIRE  0x10
#define BUTTON_START 0x20
#define BUTTON_COIN  0x40
#define BUTTON_EXTRA 0x80

class Nunchuck {
public:
  void setup();
  void enable();
  void disable();
  unsigned char getInput();
 
private:
  Nunchuk nchuk;
  bool enabled;
  unsigned long timeout;
  unsigned char lastValue;
};

#endif

#endif //_NUNCHUCK_H_