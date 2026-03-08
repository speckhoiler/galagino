#include "bagman.h"

unsigned char bagman::opZ80(unsigned short Addr) {
  return bagman_rom_cpu[Addr];
}

unsigned char bagman::rdZ80(unsigned short Addr) {
  if(Addr < 24576) return bagman_rom_cpu[Addr];
    
  if((Addr >= 0x6000) && (Addr <= 0x67ff)) return memory[Addr - 0x6000];

  // videoram
  if((Addr >= 0x9000) && (Addr <= 0x93ff)) return memory[Addr - 0x9000 + 0x800];

 	// spriteram
  if((Addr >= 0x9800) && (Addr <= 0x9bff)) return memory[Addr - 0x9800 + 0xc00];   // map to 0x0c00

  // IN0
  if(Addr == 0xa000) {
    unsigned char keymask = input->buttons_get();
    unsigned char retval = 0x00;
    
    if(keymask & BUTTON_COIN)  retval |= 0x02;
    if(keymask & BUTTON_START) retval |= 0x04;
    if(keymask & BUTTON_LEFT)  retval |= 0x08;
    if(keymask & BUTTON_RIGHT) retval |= 0x10;
    if(keymask & BUTTON_UP)    retval |= 0x20;
    if(keymask & BUTTON_DOWN)  retval |= 0x40;
    if(keymask & BUTTON_FIRE)  retval |= 0x80;
      return retval;
  }

  // IN1
  if(Addr == 0xa800) return BAGMAN_DIP1;

  // IN2
  if(Addr == 0xb000) return BAGMAN_DIP2;

  // watchdog
  if(Addr == 0xb800) return 0x00;

  return 0xff;
}

void bagman::wrZ80(unsigned short Addr, unsigned char Value) {     
  // ram
  if((Addr >= 0x6000) && (Addr <= 0x67ff)) {
    memory[Addr - 0x6000] = Value;
    return;
  }

  // videoram
  if((Addr >= 0x9000) && (Addr <= 0x93ff)) {
    memory[Addr - 0x9000 + 0x800] = Value;  // map to 0x0800
    return;
  }
    
 	// spriteram
  if((Addr >= 0x9800) && (Addr <= 0x9bff)) {
    memory[Addr - 0x9800 + 0xc00] = Value;   // map to 0x0c00
    return;
  }

  // coin_count_0_w
  if(Addr == 0xa003) return;

  // lfo_freq_w (not used)
  if((Addr >= 0xa004) && (Addr <= 0xa007)) return;

	// sound_w (not used)
  if((Addr >= 0xa800) && (Addr <= 0xa807)) return;

  // irq_enable_w
  if(Addr == 0xb001) {
    irq_enable[0] = Value & 1;
    return;
  }
    
  // gfxbank_w
  if (Addr == 0xb002) {
    gfxbank = Value;
    return;
  }

  // flip_screen_x_w
  if (Addr == 0xb006) return;

  // flip_screen_y_w
  if (Addr == 0xb007) return;
  
	// pitch_w
  if (Addr == 0xb800) {
    pitch_w(Value);
    return;
  }
}

void bagman::pitch_w(uint8_t data) {
  unsigned char duration = (data & 0xf0) >> 4;
  unsigned char sound = data & 0x0f;
  unsigned char pause = 0xf;

  // silence...
  if (data == 0xff) {
    duration = 0;
  }

  soundregs[0] = 0; // duration lowbyte
  soundregs[1] = duration; // duration highbyte
  soundregs[2] = sound;
  soundregs[3] = pause;
}

void bagman::run_frame(void) {
  for(int i=0;i<INST_PER_FRAME;i++) {
    StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);
  }
  
  if(irq_enable[0]) {
    // wait till last interrupt acknowledged
    while (cpu[0].IFF == IFF_IM1) {
      StepZ80(&cpu[0]);
    }

    IntZ80(&cpu[0], INT_RST38);
  }
  
  if (!game_started)
    game_started = 1;
}

void bagman::prepare_frame(void) {
  active_sprites = 0;
  // bagman supports a total of 8 sprites of 8x8 size
  for(int idx=7;idx>=0 && active_sprites < 92;idx--) {
    // sprites are stored at 0x0c40
    unsigned char *sprite_base_ptr = memory + 0xc40 + 4*idx;
    struct sprite_S spr;     

    if(sprite_base_ptr[3]) {
      spr.x = sprite_base_ptr[0] - 17;
      spr.y = sprite_base_ptr[3] + 15;
      spr.color = sprite_base_ptr[2] & 7;
      spr.code = sprite_base_ptr[1] & 0x3f;
      spr.code |= ((gfxbank << 7) | 0x40);
      spr.code &= 0x7F; //0...127
      spr.flags = (sprite_base_ptr[1] & 0x80) ? 1 : 0; //flipX = 1
      if((spr.y > -16) && (spr.y < 288) && (spr.x > -16) && (spr.x < 224))
      	sprite[active_sprites++] = spr;
    }    
  }
}

// draw a single 8x8 tile
void bagman::blit_tile(short row, char col) {
  unsigned short addr = tileaddr[row][col];

  if((row < 2) || (row >= 34))
    return;

  unsigned short chr = memory[0x0800 + addr];
  
  chr |= (gfxbank << 9);
  const unsigned short *tile = tileRom(chr);

  // bagman has a very reduced color handling
  int c = memory[0xc00 + 2 * (addr & 31) + 1] & 7;
  const unsigned short *colors = colorRom(c);

  unsigned short *ptr = frame_buffer + 8*col;

  // 8 pixel rows per tile
  for(char r=0;r<8;r++,ptr+=(224-8)) {
    unsigned short pix = *tile++;
    // 8 pixel columns per tile
    for(char c=0;c<8;c++,pix>>=2) {
      if(pix & 3) *ptr = colors[pix&3];
      ptr++;
    }
  }
}

void bagman::blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = spriteRom(sprite[s].flags, sprite[s].code);
  const unsigned short *colors = colorRom(sprite[s].color);
  
  // create mask for sprites that clip left or right
  unsigned long mask = 0xffffffff;
  if(sprite[s].x < 0)      mask <<= -2*sprite[s].x;
  if(sprite[s].x > 224-16) mask >>= 2*(sprite[s].x-224-16);		

  short y_offset = sprite[s].y - 8*row;

  // check if there are less than 8 lines to be drawn in this row
  unsigned char lines2draw = 8;
  if(y_offset < -8) lines2draw = 16+y_offset;

  // check which sprite line to begin with
  unsigned short startline = 0;
  if(y_offset > 0) {
    startline = y_offset;
    lines2draw = 8 - y_offset;
  }

  // if we are not starting to draw with the first line, then
  // skip into the sprite image
  if(y_offset < 0) spr -= y_offset;  

  // calculate pixel lines to paint  
  unsigned short *ptr = frame_buffer + sprite[s].x + 224*startline;
  
  // 16 pixel rows per sprite
  for(char r=0;r<lines2draw;r++,ptr+=(224-16)) {
    unsigned long pix = *spr++ & mask;
    // 16 pixel columns per tile
    for(char c=0;c<16;c++,pix>>=2) {
      unsigned short col = colors[pix&3];
      if(pix & 3) *ptr = col;
      ptr++;
    }
  }
}

void bagman::render_row(short row) {
  // don't render lines 0, 1, 34 and 35
  if(row <= 1 || row >= 34) return;

  for(char col=0;col<28;col++)
    blit_tile(row, col);
  
  // render sprites
  for(unsigned char s=0;s<active_sprites;s++) {
    // check if sprite is visible on this row
    if((sprite[s].y < 8*(row+1)) && ((sprite[s].y+16) > 8*row))
      blit_sprite(row, s);
  }
}

const unsigned short *bagman::tileRom(unsigned short addr) {
  return bagman_tilemap[addr];
}

const unsigned short *bagman::colorRom(unsigned short addr) {
  return bagman_colormap[addr];
}

const unsigned long *bagman::spriteRom(unsigned char flags, unsigned char code) {
  return bagman_sprites[flags][code];
}

const unsigned short *bagman::logo(void) {
  return bagman_logo;
}

#ifdef LED_PIN
void bagman::gameLeds(CRGB *leds) {
  // bagman: slow yellow on green "knight rider" ...
  static char sub_cnt = 0;
  if(sub_cnt++ == 32) {
    sub_cnt = 0;
      
    // and also do the marquee LEDs
    static char led = 0;
      
    char il = (led<NUM_LEDS)?led:((2*NUM_LEDS-2)-led);
    for(char c=0;c<NUM_LEDS;c++) {
      if(c == il) leds[c] = LED_YELLOW;
      else        leds[c] = LED_GREEN;
    }
    led = (led + 1) % (2*NUM_LEDS-2);      
  }    
}

void bagman::menuLeds(CRGB *leds) {
  memcpy(leds, menu_leds, NUM_LEDS*sizeof(CRGB));
}
#endif