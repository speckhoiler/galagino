#include "dkong.h"

void dkong::reset() { 
  machineBase::reset();
  i8048_reset(&cpu_8048);
	memset(dkong_audio_transfer_buffer, 0, sizeof(dkong_audio_transfer_buffer)); 
}

void dkong::wrI8048_port(struct i8048_state_S *state, unsigned char port, unsigned char pos) {
  if(port == 0)
    return;
  else if(port == 1) {
    static int bptr = 0;
    
    dkong_audio_assembly_buffer[bptr++] = pos;
    
    // buffer now full?
    if(bptr == 64) {
      bptr = 0;

      // It must never happen that we get here with no free transfer buffers
      // available. This would mean that the buffers were full and the
      // 8048 emulation was still running. It should be stoppped as long as the
      // buffers are full.
      if(((dkong_audio_wptr + 1)&DKONG_AUDIO_QUEUE_MASK) == dkong_audio_rptr) {
	    // overflow
      } else {
        // copy data into transfer buffer
        memcpy(dkong_audio_transfer_buffer[dkong_audio_wptr], dkong_audio_assembly_buffer, 64);
        dkong_audio_wptr = (dkong_audio_wptr + 1)&DKONG_AUDIO_QUEUE_MASK;
      }
    }
  } else if(port == 2)
    state->p2_state = pos;
}

unsigned char dkong::rdI8048_port(struct i8048_state_S *state, unsigned char port) {
  if(port == 2) return state->p2_state;
  return 0;
}

unsigned char dkong::rdI8048_xdm(struct i8048_state_S *state, unsigned char addr) {
  // inverted Z80 MUS register
  if(state->p2_state & 0x40)
    return dkong_sfx_index ^ 0x0f;
  
  return dkong_rom_cpu2[2048 + addr + 256 * (state->p2_state & 7)];
}

unsigned char dkong::rdI8048_rom(struct i8048_state_S *state, unsigned short addr) {
  return dkong_rom_cpu2[addr];
}

unsigned char dkong::opZ80(unsigned short Addr) {
  if (current_cpu == 0)
    return dkong_rom_cpu1[Addr];
  else
    return dkong_rom_cpu1[Addr];
}

unsigned char dkong::rdZ80(unsigned short Addr) {
  if(Addr < 16384)
    return dkong_rom_cpu1[Addr];

  // 0x6000 - 0x77ff
  if(((Addr & 0xf000) == 0x6000) || ((Addr & 0xf800) == 0x7000)) 
    return memory[Addr - 0x6000];
  
  if((Addr & 0xfff0) == 0x7c00) {
    // get a mask of currently pressed keys
    unsigned char keymask = input->buttons_get();
    
    unsigned char retval = 0x00;
    if(keymask & BUTTON_RIGHT) retval |= 0x01;
    if(keymask & BUTTON_LEFT)  retval |= 0x02;
    if(keymask & BUTTON_UP)    retval |= 0x04;
    if(keymask & BUTTON_DOWN)  retval |= 0x08;
    if(keymask & BUTTON_FIRE)  retval |= 0x10;
    return retval;
  }
  
  if((Addr & 0xfff0) == 0x7c80) {  // IN1
    return 0x00;
  }
  
  if((Addr & 0xfff0) == 0x7d00) {
    // get a mask of currently pressed keys
    unsigned char keymask = input->buttons_get();
    
    unsigned char retval = 0x00;
    if(keymask & BUTTON_COIN)   retval |= 0x80;  
    if(keymask & BUTTON_START)  retval |= 0x04; 
    return retval;
  }
  
  if((Addr & 0xfff0) == 0x7d80)
    return DKONG_DIP;

  return 0xff;
}

void dkong::wrZ80(unsigned short Addr, unsigned char Value) {
  if(((Addr & 0xf000) == 0x6000) || ((Addr & 0xf800) == 0x7000)) {
    memory[Addr - 0x6000] = Value;
    return;
  }
  
  // ignore DMA register access
  if((Addr & 0xfe00) == 0x7800)
    return;
  
  if((Addr & 0xfe00) == 0x7c00) {  // 7cxx and 7dxx
    // music effect
    if(Addr == 0x7c00) dkong_sfx_index = Value;
    
    // 7d0x
    if((Addr & 0xfff0) == 0x7d00) {
      
      // trigger samples 0 (walk), 1 (jump) and 2 (stomp)
      if((Addr & 0x0f) <= 2  && Value)
	      trigger_sound(Addr & 0x0f);
      
      if((Addr & 0x0f) == 3) {
	      if(Value & 1) cpu_8048.p2_state &= ~0x20;
	      else          cpu_8048.p2_state |=  0x20;
      }
      
      if((Addr & 0x0f) == 4)
	      cpu_8048.T1 = !(Value & 1);
      
      if((Addr & 0x0f) == 5)
	      cpu_8048.T0 = !(Value & 1);          
    }
    
    if(Addr == 0x7d80)
      cpu_8048.notINT = !(Value & 1);
    
    if(Addr == 0x7d84)
      irq_enable[0] = Value & 1;
    
    if((Addr == 0x7d85) && (Value & 1)) {
      // trigger DRQ to start DMA
      // Dkong uses the DMA only to copy sprite data from 0x6900 to 0x7000
      memcpy(memory+0x1000, memory+0x900, 384);
    }
    
    if(Addr == 0x7d86) {
      colortable_select &= ~1;
      colortable_select |= (Value & 1);
    }
    
    if(Addr == 0x7d87) {
      colortable_select &= ~2;
      colortable_select |= ((Value<<1) & 2);
    }
    return;
  }
}

void dkong::run_frame(void) {
  current_cpu = 0;
  game_started = 1; // TODO: make this from some graphic thing
    
  // dkong      
  for(int i=0;i<INST_PER_FRAME;i++) {
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);

    // run audio cpu only when audio transfer buffers are not full. The
    // audio CPU seems to need more CPU time than the main Z80 itself.
    if(((dkong_audio_wptr+1)&DKONG_AUDIO_QUEUE_MASK) != dkong_audio_rptr) {
      i8048_step(&cpu_8048); i8048_step(&cpu_8048);
      i8048_step(&cpu_8048); i8048_step(&cpu_8048);
      i8048_step(&cpu_8048); i8048_step(&cpu_8048);
    }
  }
      
  if(irq_enable[0])
    IntZ80(cpu, INT_NMI);
}

void dkong::trigger_sound(char snd) {
  static const struct {
    const signed char *data;
    const unsigned short length; 
  } samples[] = {
    { (const signed char *)dkong_sample_walk0, sizeof(dkong_sample_walk0) },
    { (const signed char *)dkong_sample_walk1, sizeof(dkong_sample_walk1) },
    { (const signed char *)dkong_sample_walk2, sizeof(dkong_sample_walk2) },
    { (const signed char *)dkong_sample_jump,  sizeof(dkong_sample_jump)  },
    { (const signed char *)dkong_sample_stomp, sizeof(dkong_sample_stomp) }
  };

  // samples 0 = walk, 1 = jump, 2 = stomp

  if(!snd) {
    // walk0, walk1 and walk2 are variants
    char rnd = random() % 3;
    dkong_sample_cnt[0] = samples[rnd].length;
    dkong_sample_ptr[0] = samples[rnd].data;
  } else {
    dkong_sample_cnt[snd] = samples[snd+2].length;
    dkong_sample_ptr[snd] = samples[snd+2].data;
  }
}

void dkong::prepare_frame(void) {
  active_sprites = 0;
  for(int idx=0;idx<96 && active_sprites<92;idx++) {
    // sprites are stored at 0x7000
    unsigned char *sprite_base_ptr = memory + 0x1000 + 4*idx;
    struct sprite_S spr;     
    
    // adjust sprite position on screen for upright screen
    spr.x = sprite_base_ptr[0] - 23;
    spr.y = sprite_base_ptr[3] + 8;
    
    spr.code = sprite_base_ptr[1] & 0x7f;
    spr.color = sprite_base_ptr[2] & 0x0f;
    spr.flags =  ((sprite_base_ptr[2] & 0x80)?1:0) |
      ((sprite_base_ptr[1] & 0x80)?2:0);

    // save sprite in list of active sprites
    if((spr.y > -16) && (spr.y < 288) &&
       (spr.x > -16) && (spr.x < 224))
      sprite[active_sprites++] = spr;
  }
}

// draw a single 8x8 tile
void dkong::blit_tile(short row, char col) {
  unsigned short addr = tileaddr[row][col];

  if((row < 2) || (row >= 34)) return;    
  // skip blank dkong tiles (0x10) in rendering  
  if(memory[0x1400 + addr] == 0x10) return;   
  const unsigned short *tile = dkong_tilemap[memory[0x1400 + addr]];
  // donkey kong has some sort of global color table
  const unsigned short *colors = dkong_colormap[colortable_select][row-2 + 32*(col/4)];

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

// dkong has its own sprite drawing routine since unlike the other
// games, in dkong black is not always transparent. Black pixels
// are instead used for masking
void dkong::blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = dkong_sprites[sprite[s].flags & 3][sprite[s].code];
  const unsigned short *colors = dkong_colormap_sprite[colortable_select][sprite[s].color];
  
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
  if(y_offset < 0)
    spr -= y_offset;  

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

void dkong::render_row(short row) {
  // render 28 tile columns per row
  for(char col=0;col<28;col++)
    blit_tile(row, col);
  
  // render sprites
  for(unsigned char s=0;s<active_sprites;s++) {
    // check if sprite is visible on this row
    if((sprite[s].y < 8*(row+1)) && ((sprite[s].y+16) > 8*row))
      blit_sprite(row, s);
  }
}

const unsigned short *dkong::logo(void) {
  return dkong_logo;
}

#ifdef LED_PIN
void dkong::gameLeds(CRGB *leds) {
  // dkong: red "knight rider" ...
  static char sub_cnt = 0;
  if(sub_cnt++ == 4) {
    sub_cnt = 0;
    
    // and also do the marquee LEDs
    static char led = 0;
      
    char il = (led<NUM_LEDS)?led:((2*NUM_LEDS-2)-led);
    for(char c=0;c<NUM_LEDS;c++) {
      if(c == il) leds[c] = LED_RED;
      else        leds[c] = LED_BLACK;
    }
    led = (led + 1) % (2*NUM_LEDS-2);      
  }
}

void dkong::menuLeds(CRGB *leds) {
  memcpy(leds, menu_leds, NUM_LEDS*sizeof(CRGB));
}
#endif