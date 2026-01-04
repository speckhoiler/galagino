#include "anteater.h"

void anteater::reset() { 
  machineBase::reset();
  skipFirstInterrupt = 1;
  ignoreFireButton = 1;
  game_started = 1;
}

unsigned char anteater::opZ80(unsigned short Addr) {
  if (current_cpu == 0)
    return anteater_rom_cpu1[Addr];
  else
    return anteater_rom_cpu2[Addr];
}

unsigned char anteater::rdZ80(unsigned short Addr) {   
  if(current_cpu == 0) {
    // ROM
    if(Addr <= 0x3fff)
      return anteater_rom_cpu1[Addr];

    // 0x8000 - 0x87ff - main RAM
    if((Addr & 0xf800) == 0x8000) 
      return memory[Addr - 0x8000];
    
    // 0x8800 - 0xbff - tile ram
    if((Addr >= 0x8800) & (Addr <= 0x8bff))
      return memory[Addr - 0x8800 + 0x800];
    
    // 0x9800 - 0x9803 - PPI8255 0 
    if((Addr >= 0x9800) && (Addr <= 0x9803)) {
      unsigned char keymask = input->buttons_get();
      unsigned char retval = 0xff;
    
      if(Addr == 0x9800) {
	      if(keymask & BUTTON_COIN)  retval &= ~0x40;
	      if(keymask & BUTTON_LEFT)  retval &= ~0x20;
	      if(keymask & BUTTON_RIGHT) retval &= ~0x10;
	      if(keymask & BUTTON_DOWN)  retval &= ~0x08;
	      if(keymask & BUTTON_UP)    retval &= ~0x04;
	      
        // if game started with fire button, do not jump to service mode...
        if (ignoreFireButton & !(keymask & BUTTON_FIRE))
          ignoreFireButton = 0;

        if(!ignoreFireButton && (keymask & BUTTON_FIRE)) {
          retval &= ~0x01;
        }
      }
      
      if(Addr == 0x9801) {
	      return ANTEATER_DIP1;
      }
      
      if(Addr == 0x9802) {
	      retval = ANTEATER_DIP2;
        if(keymask & BUTTON_START) 
          retval += 0x00;
        else
          retval += 0x01;
        
        return retval;
      }

      if(Addr == 0x9803) {
        return 0xff;
      }
      return retval;
    }
    
    // PPI8255 1 not used...
    if((Addr >= 0xa000) & (Addr <= 0xa003))
      return 0x00;
 
    // watchdog
    if(Addr == 0xb000)
      return 0x00;
  } else {
    // anteater audio cpu 0x1000
    if(Addr < 4096) 
      return anteater_rom_cpu2[Addr];

    // konami_sound_map: 0x8000, 0x83ff
    if((Addr & 0xf000) == 0x8000)
      return memory[Addr - 0x8000 + 0x0d00];
  } 
  return 0xff;
}

void anteater::wrZ80(unsigned short Addr, unsigned char Value) {
  if(current_cpu == 0) {    
    // 0x8000 - 0x87ff - main RAM
    if((Addr & 0xf800) == 0x8000) {
      memory[Addr - 0x8000] = Value;
      return;
    }
    
    // 0x8800 - 0xbff - tile ram
    if((Addr >= 0x8800) & (Addr <= 0x8bff)) {
      memory[Addr - 0x8800 + 0x0800] = Value;  // map to 0x0800
      return;
    }
    
    // sprite ram
    if((Addr >= 0x9000) & (Addr <= 0x90ff)) {
      memory[Addr - 0x9000 + 0x0c00] = Value;   // map to 0x0c00
      return;
    }

    // PPI8255 0 not used: 0x9803: 0x9b
    if((Addr >= 0x9800) & (Addr <= 0x9803)) {
      return;
    }

    // PPI8255 1
    if((Addr >= 0xa000) & (Addr <= 0xa003)) {
      // PA goes to AY port A and can be read by SND CPU through the AY
      if(Addr == 0xa000) {
        //Todo: Sounds with filter acivated 0x1, 0x2, 0x3, 0x4, 0x5, 0x8, 0x9, 0xa not working??? - select other ones...
        if (Value == 0x3)
          snd_command = 0x6;
        else if (Value == 0x5)
          snd_command = 0x7;
        else if (Value == 0xa)
          snd_command = 0xb;
        else
          snd_command = Value;
      }
	      
      // rising edge on bit 3 sets audio irq
      if(Addr == 0xa001) {
	      snd_irq_state = Value & 8;
      }
	    return;
    }

    if(Addr == 0xa801) {  
      irq_enable[0] = Value & 1;

      if (irq_enable[0] && skipFirstInterrupt) {
        irq_enable[0] = 0;
        skipFirstInterrupt = 0;
      }
      return;
    }

    if(Addr == 0xa803) {
      showCustomBackground = Value & 1;
      return;
    } 
  } else {
    // anteater audio cpu
    if((Addr & 0xf000) == 0x8000) {
      memory[Addr - 0x8000 + 0x0d00] = Value;
      return;
    }

    //konami_sound_filter
    if((Addr & 0xf000) == 0x9000) {
      snd_filter = Value;
      konami_sound_filter(Addr, Value);
    }
  }
}

void anteater::konami_sound_filter(unsigned short offset, uint8_t data)
{
    //printf("filter: %04x\n", data);
    return;

		// the offset is used as data, 6 channels * 2 bits each
		// AV0 .. AV5  ==> AY8910 #2 - 3C
		// AV6 .. AV11 ==> AY8910 #1 - 3D
		for (int which = 0; which < 2; which++)
		{
				for (int flt = 0; flt < 6; flt++)
				{
					const int fltnum = (flt + 6 * which);
					const uint8_t bit = (offset >> (flt + 6 * (1 - which))) & 1;
     			// low bit goes to 0.22uF capacitor = 220000pF
					// high bit goes to 0.047uF capacitor = 47000pF
					//m_filter_ctl[fltnum]->write(bit);
				  printf("filter: %04x %04x\n", fltnum, bit);
        }
		}
}

// called by audio cpu only...
void anteater::outZ80(unsigned short Port, unsigned char Value) {
  // AY8910 2 address
  if((Port & 0xff) == 0x10) {
    snd_ay_port = Value & 15;
    return;
  } 

  // AY8910 2 value
  if((Port & 0xff) == 0x20) {
    soundregs[16 + snd_ay_port] = Value; 
    return;
  }

  // AY8910 1 address
  if((Port & 0xff) == 0x40) {
    snd_ay_port = Value & 15;
    return;
  }

  // AY8910 1 value
  if((Port & 0xff) == 0x80) {
    soundregs[snd_ay_port] = Value;      
    return;
  }
}

// called by audio cpu only...
unsigned char anteater::inZ80(unsigned short Port) {     
  // AY8910 2
  if((Port & 0xff) == 0x20) {
    if(snd_ay_port < 14)
      return soundregs[16 + snd_ay_port];
  }

  // AY8910 1
  if((Port & 0xff) == 0x80) {
    if(snd_ay_port < 14)
      return soundregs[snd_ay_port];

    // command register Port A
    if(snd_ay_port == 14)
      return snd_command;    
  }
  return 0x0;
}

void anteater::run_frame(void) {
  for(int i=0;i<INST_PER_FRAME;i++) {
    current_cpu=0; StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);    
    current_cpu=1; StepZ80(&cpu[1]); StepZ80(&cpu[1]);
    
    if(snd_irq_state) {
      snd_irq_state = 0;
      IntZ80(&cpu[1], INT_RST38);
    } 
  }

  if(irq_enable[0]) {
    current_cpu = 0;
    IntZ80(&cpu[0], INT_NMI);
  }
}

void anteater::prepare_frame(void) {
  active_sprites = 0;
  for(int idx=7;idx>=0 && active_sprites < 92;idx--) {
    // sprites are stored at 0x0c40
    unsigned char *sprite_base_ptr = memory + 0xc40 + 4*idx;
    struct sprite_S spr;     

    if(sprite_base_ptr[3]) {
      spr.x = sprite_base_ptr[0] - 16;
      spr.y = sprite_base_ptr[3] + 17;
      spr.color = sprite_base_ptr[2] & 7; 
      spr.code = sprite_base_ptr[1] & 0x3f;
      spr.flags =   ((sprite_base_ptr[1] & 0x80)?2:0);

      if((spr.y > -16) && (spr.y < 288) && (spr.x > -16) && (spr.x < 224))
      	sprite[active_sprites++] = spr;
    }    
  }
}

// draw a single 8x8 tile
void anteater::blit_tile(short row, char col) {
  if((row < 2) || (row >= 34))
    return;

  unsigned short addr = tileaddr[row][col];
  const unsigned short *tile = tileRom(memory[0x0800 + addr]);

  // anteater has a very reduced color handling
  int c = memory[0xc00 + 2 * (addr & 31) + 1] & 0x07;
  const unsigned short *colors = colorRom(c);
  
  unsigned short *ptr = frame_buffer + 8*col;

  // 8 pixel rows per tile
  for(char r=0;r<8;r++,ptr+=(224-8)) {
    unsigned short pix = *tile++;
    // 8 pixel columns per tile
    for(char c=0;c<8;c++,pix>>=2) {
      // bit 0 and bit 1 swapped....
      long index = ((pix & 2) >> 1) | ((pix & 1) << 1);

      if(pix & 3) *ptr = colors[index];
      ptr++;
    }
  }
}

// anteater can scroll single lines
void anteater::blit_tile_scroll(short row, signed char col, short scroll) {
  if((row < 2) || (row >= 34))
    return;

  unsigned short addr;
  unsigned short mask = 0xffff;
  int sub = scroll & 0x07;
  if(col >= 0) {
    addr = tileaddr[row][col];

    // one tile (8 pixels) further is an address offset of 32
    addr = (addr + ((scroll & ~7) << 2)) & 1023;

    if((sub != 0) && (col == 27))
      mask = 0xffff >> (2*sub);    
  } else {
    // negative column is a special case for the leftmost
    // tile when it's only partly visible
    addr = tileaddr[row][0];
    addr = (addr + 32 + ((scroll & ~7) << 2)) & 1023;

    mask = 0xffff << (2*(8-sub));
  }
    
  const unsigned char chr = memory[0x0800 + addr];
  const unsigned short *tile = tileRom(chr);

  // anteater has a very reduced color handling
  int c = memory[0xc00 + 2 * (addr & 31) + 1] & 7;
  const unsigned short *colors = colorRom(c);
  unsigned short *ptr = frame_buffer + 8*col + sub;

  // 8 pixel rows per tile
  for(char r=0;r<8;r++,ptr+=(224-8)) {
    unsigned short pix = *tile++ & mask;
    // 8 pixel columns per tile
    for(char c=0;c<8;c++,pix>>=2) {  
      // bit 0 and bit 1 swapped....
      long index = ((pix & 2) >> 1) | ((pix & 1) << 1);
    
      if(pix & 3) *ptr = colors[index];
      ptr++;      
    }
  }
}

void anteater::blit_sprite(short row, unsigned char s) {
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
      // bit 0 and bit 1 swapped....
      long index = ((pix & 2) >> 1) | ((pix & 1) << 1);

      unsigned short col = colors[index];
      if(pix & 3) *ptr = col;
      ptr++;
    }
  }
}

void anteater::render_row(short row) {
  // don't render lines 0, 1, 34 and 35
  if(row <= 1 || row >= 34) return;

  if (showCustomBackground && row >=2 && row <= 33)
    memset(frame_buffer, 8, 2 * 224 * 8);

  // get scroll info for this row
  unsigned char scroll = memory[0xc00 + 2 * (row - 2)];
  //scroll = ((scroll << 4) & 0xf0) | ((scroll >> 4) & 0x0f);
  
  // render 28 tile columns per row. Handle anteater specific
  // scroll capabilities
  if(scroll == 0) // no scroll in this line?
    for(char col=0;col<28;col++)
      blit_tile(row, col);
  else {
    // if scroll offset is multiple of 8, then
    // 28 tiles are sufficient, otherwise the first
    // fragment needs to be drawn
    if(scroll & 7) 
      blit_tile_scroll(row, -1, scroll);
    
    for(char col=0;col<28;col++)
      blit_tile_scroll(row, col, scroll);
  }
  // render sprites
  for(unsigned char s=0;s<active_sprites;s++) {
    // check if sprite is visible on this row
    if((sprite[s].y < 8*(row+1)) && ((sprite[s].y+16) > 8*row))
      blit_sprite(row, s);
  }
}

const unsigned short *anteater::tileRom(unsigned short addr) {
  return anteater_tilemap[addr];
}

const unsigned short *anteater::colorRom(unsigned short addr) {
  return anteater_colormap[addr];
}

const unsigned long *anteater::spriteRom(unsigned char flags, unsigned char code) {
  return anteater_sprites[flags][code];
}

const unsigned short *anteater::logo(void) {
  return anteater_logo;
}

#ifdef LED_PIN
void anteater::gameLeds(CRGB *leds) {
  // anteater: slow yellow on green "knight rider" ...
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

void anteater::menuLeds(CRGB *leds) {
  memcpy(leds, menu_leds, NUM_LEDS*sizeof(CRGB));
}
#endif