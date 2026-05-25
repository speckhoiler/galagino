#include "spaceinvaders.h"

void spaceinvaders::reset() {
  machineBase::reset();

  // Init shift register
  shift_data = 0;
  shift_amount = 0;
}

unsigned char spaceinvaders::opZ80(unsigned short Addr) {
  Addr &= 0x3FFF;  // Mirror: 0x4000+ wraps to 0x0000+
  if(Addr < 0x2000)
    return spaceinvaders_rom[Addr];
  
  return memory[Addr - RAM_OFFSET];
}

unsigned char spaceinvaders::rdZ80(unsigned short Addr) {
  Addr &= 0x3FFF;
  if(Addr < 0x2000)
    return spaceinvaders_rom[Addr];

  return memory[Addr - RAM_OFFSET];
}

void spaceinvaders::wrZ80(unsigned short Addr, unsigned char Value) {
  Addr &= 0x3FFF;
  if(Addr < 0x2000) return;  // ROM area - ignore writes

  memory[Addr - RAM_OFFSET] = Value;

  // Detect game start: writing to VRAM
  if(!game_started && Addr >= 0x2400 && Addr < 0x4000) {
    if(Value != 0) game_started = 1;
  }
}

unsigned char spaceinvaders::inZ80(unsigned short Port) {
  Port &= 0xFF;

  switch(Port) {
    case 0: {
      // Port 0: unused in most sets, return 0x0E for MAME compatibility
      return 0x0E;
    }

    case 1: {
      // Port 1: Player 1 inputs
      // Bit 0: Coin (active high)
      // Bit 1: P2 start
      // Bit 2: P1 start
      // Bit 3: Always 1
      // Bit 4: P1 fire
      // Bit 5: P1 left
      // Bit 6: P1 right
      // Bit 7: 0
      unsigned char keymask = input->buttons_get();
      unsigned char retval = 0x08;  // bit 3 always 1

      if(keymask & BUTTON_COIN) {
        retval |= 0x01;  // coin
        if(!last_coin) soundregs[2] = 1;  // trigger coin sound (rising edge)
        last_coin = 1;
      } 
      else {
        last_coin = 0;
      }
      if(keymask & BUTTON_EXTRA)  retval |= 0x02;  // P2 start
      if(keymask & BUTTON_START)  retval |= 0x04;  // P1 start
      if(keymask & BUTTON_FIRE)   retval |= 0x10;  // P1 fire
      if(keymask & BUTTON_LEFT)   retval |= 0x20;  // P1 left
      if(keymask & BUTTON_RIGHT)  retval |= 0x40;  // P1 right
      return retval;
    }

    case 2: {
      // Port 2: DIP switches + P2 controls
      // Bits 0-1: Ships (DIP)
      // Bit 2: Tilt
      // Bit 3: Extra ship bonus (DIP)
      // Bit 4: P2 fire
      // Bit 5: P2 left
      // Bit 6: P2 right
      // Bit 7: Coin info (DIP)
      unsigned char retval = SINV_DIP;
      unsigned char keymask = input->buttons_get();
      // P2 uses same buttons (1 player cabinet mode)
      if(keymask & BUTTON_FIRE)   retval |= 0x10;
      if(keymask & BUTTON_LEFT)   retval |= 0x20;
      if(keymask & BUTTON_RIGHT)  retval |= 0x40;
      return retval;
    }

    case 3: {
      // Port 3: Shift register result
      return (uint8_t)((shift_data >> (8 - shift_amount)) & 0xFF);
    }
  }
  return 0x00;
}

void spaceinvaders::outZ80(unsigned short Port, unsigned char Value) {
  Port &= 0xFF;

  switch(Port) {
    case 2:
      // Shift amount (bits 0-2)
      shift_amount = Value & 0x07;
      break;
    case 3:
      // Sound effects group 1
      // Bit 0: UFO repeat
      // Bit 1: Shot
      // Bit 2: Flash (player die)
      // Bit 3: Invader die
      // Bit 4: Extended play
      // Mapped to soundregs for basic audio support
      soundregs[0] = Value;
      break;
    case 4:
      // Shift data: push new byte, previous becomes low byte
      shift_data = (shift_data >> 8) | ((uint16_t)Value << 8);
      break;
    case 5:
      // Sound effects group 2
      // Bit 0: Fleet movement 1
      // Bit 1: Fleet movement 2
      // Bit 2: Fleet movement 3
      // Bit 3: Fleet movement 4
      // Bit 4: UFO hit
      soundregs[1] = Value;
      break;
    case 6:
      // Watchdog - ignore
      break;
  }
}

void spaceinvaders::run_frame(void) {
  // First half of frame: execute CPU, then fire RST 08H (mid-screen)
  // 8080 runs at 2MHz, Z80 emu uses IM0 mode, EI/DI controls IFF internally
  for(int i = 0; i < 832; i++) {
    StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); 
    
    // RST 08H mid-screen interrupt (Z80 core checks IFF1 internally)
    if (i == 415)
      IntZ80(&cpu[0], INT_RST08);
  }
  
  // RST 10H vblank interrupt
  IntZ80(&cpu[0], INT_RST10);
}

void spaceinvaders::prepare_frame(void) {
  // Space Invaders has no sprite hardware
  // All rendering is done directly from the VRAM bitmap in render_row()
  active_sprites = 0;
}

// Color overlay bands based on vertical position in player's view
// Approximates the colored cellophane strips on the original cabinet
unsigned short spaceinvaders::get_pixel_color(int y) {
  if(y < 32)  return COL_WHITE;   // Score area (top)
  if(y < 64)  return COL_RED;     // UFO row
  if(y < 184) return COL_WHITE;   // Invaders area
  if(y < 240) return COL_GREEN;   // Player + shields
  return COL_WHITE;                // Lives/credit (bottom)
}

void spaceinvaders::render_row(short row) {
  // Space Invaders: 224 wide x 256 tall (rotated from original 256x224)
  // Galagino renders 36 rows of 8 pixels = 288 tall
  // We use rows 2-33 (256 pixels), centering the game with 16px borders
  // Rows 0-1 and 34-35 are left black (cleared by memset in main loop)
  if(row < 2 || row > 33) return;

  // Player's vertical position for this tile row
  int base_y = (row - 2) * 8;  // 0-248

  // VRAM is at memory[VRAM_OFFSET], 7168 bytes
  // Original screen: 224 rows of 32 bytes each (256 pixels per row, 1bpp)
  //
  // Rotation mapping (original → player's rotated view):
  //   player_x (0..223) = original_row (y_orig)
  //   player_y (0..255) = 255 - original_col (x_orig)
  //
  // For pixel at player coords (px, py):
  //   y_orig = px
  //   x_orig = 255 - py
  //   VRAM byte = y_orig * 32 + x_orig / 8
  //   VRAM bit  = x_orig % 8 (LSB = leftmost pixel in original)

  unsigned char *vram = memory + VRAM_OFFSET;

  for(int sr = 0; sr < 8; sr++) {
    int py = base_y + sr;
    if(py >= 256) break;

    int x_orig = 255 - py;
    int byte_col = x_orig >> 3;
    int bit_pos  = x_orig & 7;

    unsigned short color = get_pixel_color(py);
    unsigned short *dst = frame_buffer + sr * 224;

    for(int px = 0; px < 224; px++) {
      // y_orig = px, so VRAM byte = px * 32 + byte_col
      unsigned char vram_byte = vram[px * 32 + byte_col];
      if(vram_byte & (1 << bit_pos)) {
        dst[px] = color;
      }
      // else: pixel stays 0 (black) from memset in renderRow()
    }
  }
}


const unsigned short *spaceinvaders::logo(void) {
  return spaceinvaders_logo;
}

#ifdef LED_PIN
void spaceinvaders::gameLeds(CRGB *leds) {
  // Alternating green/white pattern with animation
  static char sub_cnt = 0;
  if(sub_cnt++ == 32) {
    sub_cnt = 0;
    static char led = 0;
    char il = (led < NUM_LEDS) ? led : ((2 * NUM_LEDS - 2) - led);
    for(char c = 0; c < NUM_LEDS; c++) {
      if(c == il) leds[c] = LED_WHITE;
      else        leds[c] = LED_GREEN;
    }
    led = (led + 1) % (2 * NUM_LEDS - 2);
  }
}

void spaceinvaders::menuLeds(CRGB *leds) {
  memcpy(leds, menu_leds, NUM_LEDS * sizeof(CRGB));
}
#endif