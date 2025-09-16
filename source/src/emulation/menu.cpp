#include "menu.h"

void Menu::init(Input *input, machineBase **machines,  signed char machinesCount, unsigned short *framebuffer) {
  this->master_attract_timeout = millis();
  this->input = input;
  this->machines = machines;
  this->machinesCount = machinesCount;
  this->frame_buffer = framebuffer;
}

void Menu::show_menu() {
  machineIndex = MCH_MENU;
  menuWasSelected = true;

  // when going back to menu, reactivate attract mode
  master_attract_timeout = millis();

  // prevent start after a reset
  last_mask = BUTTON_START;
  printf("show menu\n");
}

signed char Menu::machineIndexSelected() { 
  return machineIndex - 1; 
}

signed char Menu::machineIndexPreselection() {
  return menu_sel - 1;
}

bool Menu::startMachine() { 
  if(machineIndex != machineIndexLast | menuWasSelected) {
    machineIndexLast = machineIndex;
    menuWasSelected = false;
    return true;
  }
  return false; 
}
  
bool Menu::machineIndexIsMenu() { 
  return machineIndex == MCH_MENU; 
}

bool Menu::attract_gameTimeout() {
#ifdef MASTER_ATTRACT_GAME_TIMEOUT
  if(master_attract_timeout && (millis() - master_attract_timeout > MASTER_ATTRACT_GAME_TIMEOUT)) {
    master_attract_timeout = millis();

    // select next attract machine
    menu_sel++;
    if (menu_sel > machinesCount)
      menu_sel = 1;

    printf("MASTER ATTRACT game timeout, return to menu\n");
    return true;
  }
#endif
  return false;
}

void Menu::attract_resetTimer() {
  if (master_attract_timeout  != 0) {
    master_attract_timeout = 0;
    printf("MASTER ATTRACT timer reset!!!\n");
  }
}

void Menu::handle() {
  // get a mask of currently pressed keys
  unsigned char keymask = input->buttons_get();     
	    
  if((keymask & BUTTON_UP) && !(last_mask & BUTTON_UP))
    menu_sel--;

  if((keymask & BUTTON_DOWN) && !(last_mask & BUTTON_DOWN))
    menu_sel++;

  if(((keymask & BUTTON_FIRE) && !(last_mask & BUTTON_FIRE)) || ((keymask & BUTTON_START) && !(last_mask & BUTTON_START))) {
    machineIndex = menu_sel;
    printf("select machine %d\n", machineIndex);
  }

  if(machinesCount <= 3) {
    if(menu_sel < 1) 
      menu_sel = 1;
    if(menu_sel > machinesCount) 
      menu_sel = machinesCount;
  } 
  else {
    if(menu_sel < 1)
      menu_sel = machinesCount;
    if(menu_sel > machinesCount)
      menu_sel = 1;
  }
  last_mask = keymask;

#ifdef MASTER_ATTRACT_MENU_TIMEOUT
  // check for master attract timeout
  if(master_attract_timeout && (millis() - master_attract_timeout > MASTER_ATTRACT_MENU_TIMEOUT)) {
    master_attract_timeout = millis();  // new timeout for running game

    machineIndex = menu_sel;
    printf("MASTER ATTRACT to machine %d!!!\n", machineIndex);
  }
#endif
}

void Menu::render_row(short row) {
  if(machinesCount <= 3) {    
    // non-scrolling menu for 2 or 3 machines
    for(char i = 0; i < machinesCount; i++) {
      char offset = i * 12;
      if(machinesCount == 2) offset += 6;	
      if(row >= offset && row < offset + 12)  
	menu_logo(8 * (row - offset), machines[i]->logo(), menu_sel == i + 1);
    }
  } 
  else {
    // scrolling menu for more than 3 machines    
    // valid offset values range from 0 to MACHINE * 96 - 1
    static int offset = 0;

    // check which logo would show up in this row. Actually
    // two may show up in the same character row when scrolling
    int logo_idx = ((row + offset / 8) / 12) % machinesCount;
    if(logo_idx < 0) logo_idx += machinesCount;
    
    int logo_y = (row * 8 + offset) % 96;  // logo line in this row
      
    // check if logo at logo_y shows up in current row
    menu_logo(logo_y, machines[logo_idx]->logo(), (menu_sel-1) == logo_idx);
      
    // check if a second logo may show up here
    if(logo_y > (96 - 8)) {
      logo_idx = (logo_idx + 1) % machinesCount;
      logo_y -= 96;
      menu_logo(logo_y, machines[logo_idx]->logo(), (menu_sel-1) == logo_idx);
    }
      
    if(row == 35) {
      // finally offset is bound to game, something like 96 * game:    
      int new_offset = 96 * ((unsigned)(menu_sel - 2) % machinesCount);
      if(menu_sel == 1) 
        new_offset = (machinesCount - 1) * 96;
	
	    // check if we need to scroll
	    if(new_offset != offset) {
	      int diff = (new_offset - offset) % (machinesCount * 96);
	      if(diff < 0) diff += machinesCount * 96;

	      if(diff < machinesCount * 96 / 2) 
          offset = (offset + 8) % (machinesCount * 96);
	      else 
          offset = (offset - 8) % (machinesCount * 96);
	        
        if(offset < 0) 
          offset += machinesCount * 96;
      }
    }
  }
}

// render one of three the menu logos. Only the active one is colorful
// render logo into current buffer starting with line "row" of the logo
void Menu::menu_logo(short row, const unsigned short *logo, char active) {
  unsigned short marker = logo[0];
  const unsigned short *data = logo + 1;

  // current pixel to be drawn
  unsigned short ipix = 0;
    
  // less than 8 rows in image left?
  unsigned short pix2draw = ((row <= 96 - 8) ? (224 * 8) : ((96 - row) * 224));
  
  if(row >= 0) {
    // skip ahead to row
    unsigned short col = 0;
    unsigned short pix = 0;
    while(pix < 224 * row) {
      if(data[0] != marker) {
        pix++;
        data++;
      } else {
        pix += data[1] + 1;
        col = data[2];
        data += 3;
      }
    }
    
    // draw pixels remaining from previous run
    if(!active) col = convert_RGB565_to_greyscale(col);
    while(ipix < ((pix - 224 * row < pix2draw) ? (pix - 224 * row) : pix2draw))
      frame_buffer[ipix++] = col;
  } else
    // if row is negative, then skip target pixel
    ipix -= row * 224;
    
  while(ipix < pix2draw) {
    if(data[0] != marker)
      frame_buffer[ipix++] = active ? *data++ : convert_RGB565_to_greyscale(*data++);
    else {
      unsigned short color = data[2];
      if(!active) color = convert_RGB565_to_greyscale(color);
      for(unsigned short j = 0; j < data[1] + 1 && ipix < pix2draw; j++)
        frame_buffer[ipix++] = color;

      data += 3;
    }
  }  
}

unsigned short Menu::convert_RGB565_to_greyscale(unsigned short in) {
  unsigned short r = (in >> 3) & 31;
  unsigned short g = ((in << 3) & 0x38) | ((in >> 13) & 0x07);
  unsigned short b = (in >> 8) & 31;
  unsigned short avg = (2 * r + g + 2 * b) / 4;
  
  return (((avg << 13) & 0xe000) |   // g2-g0
          ((avg <<  7) & 0x1f00) |   // b5-b0
          ((avg <<  2) & 0x00f8) |   // r5-r0
          ((avg >>  3) & 0x0007));   // g5-g3
}