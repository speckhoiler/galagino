#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "input.h"
#include "../config.h"
#include "../machines/machineBase.h"

class Menu {
public:
  Menu() { }
  ~Menu() { }

  void init(Input *input, machineBase **machines, signed char machinesCount, unsigned short *framebuffer);
  void attract_resetTimer();
  bool attract_gameTimeout();
  void render_row(short row);
  void handle();
  void show_menu();

  signed char machineIndexPreselection();
  signed char machineIndexSelected();
  bool startMachine();
  bool machineIndexIsMenu();
private:
  void menu_logo(short row, const unsigned short *logo, char active);
  unsigned short convert_RGB565_to_greyscale(unsigned short in);

  Input *input;
  signed char machinesCount;
  machineBase *currentMachine;
  machineBase **machines;
  unsigned short *frame_buffer;
  unsigned char last_mask;
  bool menuWasSelected;
  unsigned long master_attract_timeout; // menu timeout for master attract mode which randomly start games
  signed char machineIndexLast;
  
#ifndef SINGLE_MACHINE
  signed char machineIndex = MCH_MENU;  // start with menu
  signed char menu_sel = 2;
#else
  signed char machineIndex = 1;
  signed char menu_sel = 1;
#endif
};

#endif