#include "galaga.h"

unsigned char galaga::opZ80(unsigned short Addr) {
  if (current_cpu == 0)
    return galaga_rom_cpu1[Addr];
  else if (current_cpu == 1)
    return galaga_rom_cpu2[Addr];
  else
    return galaga_rom_cpu3[Addr];
}

unsigned char galaga::rdZ80(unsigned short Addr) {
  if(Addr < 16384) {
    if (current_cpu == 0)
      return galaga_rom_cpu1[Addr];
    else if (current_cpu == 1)
      return galaga_rom_cpu2[Addr];
    else
      return galaga_rom_cpu3[Addr];
  }

  /* video/sprite ram */
  if((Addr & 0xe000) == 0x8000)
    return memory[Addr-0x8000];

  // latch
  if((Addr & 0xfff8) == 0x6800) {
    unsigned char dip_a = (GALAGA_DIPA & (0x80 >> (Addr&7))) ? 0:1;
    unsigned char dip_b = (GALAGA_DIPB & (0x80 >> (Addr&7))) ? 0:2;    
    return dip_a + dip_b;
  }
  
  // 0x7100 namco_06xx_device, ctrl_r, ctrl_w
  if((Addr&0xfe00) == 0x7000) {
    if(Addr & 0x100) {
      return namco_busy ? 0x00 : 0x10;   // cmd ack (game_ctrl.s L1024)
    } else  {
      unsigned char retval = 0x00;
      
      if(cs_ctrl & 1) {   // bit[0] -> 51xx selected
	      if(!credit_mode) {
	        // galaga doesn't seem to use the button mappings in byte 1 and 2 in 
	        // non-credit mode. So we don't implement that
	        unsigned char map71[] = { 0b11111111, 0xff, 0xff };
	        if(namco_cnt > 2) return 0xff;
	  
	        retval = map71[namco_cnt];
	      } else {
	        static unsigned char prev_mask = 0;
	        static unsigned char fire_timer = 0;
          
	        // byte 0 is credit in BCD, byte 1 and 2: ...FLURD 
	        unsigned char mapb1[] = { (unsigned char)(16*(credit/10) + credit % 10), 0b11111111, 0b11111111 };
	  
	        // get a mask of currently pressed keys
	        unsigned char keymask = input->buttons_get();
	  
	        // report directions directly
	        if(keymask & BUTTON_LEFT)  mapb1[1] &= ~0x08;
	        if(keymask & BUTTON_UP)    mapb1[1] &= ~0x04;
	        if(keymask & BUTTON_RIGHT) mapb1[1] &= ~0x02;
	        if(keymask & BUTTON_DOWN)  mapb1[1] &= ~0x01;
	  
	        // report fire only when it was pressed
	        if((keymask & BUTTON_FIRE) && !(prev_mask & BUTTON_FIRE)) {
	          mapb1[1] &= ~0x10;
	          fire_timer = 1;         // 0 is too short for score enter, 5 is too long
	          // should probably be done via a global counter
	        } else if(fire_timer) {
	          mapb1[1] &= ~0x10;
	          fire_timer--;
	        }
	  
	        // 51xx leaves credit mode when user presses start? Nope ...
	        if((keymask & BUTTON_START) && !(prev_mask & BUTTON_START) && credit)
	          credit -= 1;
	  
	        if((keymask & BUTTON_COIN) && !(prev_mask & BUTTON_COIN) && (credit < 99))
	          credit += 1;
	  
	        if(namco_cnt > 2) return 0xff;
          
	        retval = mapb1[namco_cnt];
	        prev_mask = keymask;
	      }
	      namco_cnt++;
      }
      return retval;
    }
  }

  return 0xff;
}

void galaga::wrZ80(unsigned short Addr, unsigned char Value) {
  if(Addr < 16384) return;   // ignore rom writes

  if((Addr & 0xe000) == 0x8000)
    memory[Addr-0x8000] = Value;
  
  // namco 06xx
  if((Addr & 0xf800) == 0x7000) {
    
    if(Addr & 0x100) {  // 7100
      // see task_man.s L450
      namco_cnt = 0;
      cs_ctrl = Value;
      namco_busy = 5000;   // this delay is important for proper startup 
      
      if(Value == 0xa8) trigger_sound_explosion();
      
    } else {            // 7000
      if(cs_ctrl & 1) {
	      if(coincredMode) {
	        coincredMode--;
	        return;
	      }
	      switch(Value) {
	        case 1:
	          // main cpu sets coin and cred mode for both players (four bytes)
	          coincredMode = 4;
	          break;
	        case 2:
	          credit_mode = 1;
	            break;        
	        case 3: // disable joystick remapping
	        case 4: // enable joystick remapping
	          break;          
	        case 5:
	          credit_mode = 0;
	          break;
	      }
	      namco_cnt++;
      }
    }    
  }
  if((Addr & 0xffe0) == 0x6800) {
    int offset = Addr - 0x6800;
    Value &= 0x0f;
    
    if(soundregs[offset] != Value)
      soundregs[offset] = Value;
    
    return;
  }
  
  if((Addr & 0xfffc) == 0x6820) {
    if((Addr & 3) < 3)
      irq_enable[Addr & 3] = Value;
    else {
      sub_cpu_reset = !Value;
      credit_mode = 0;   // this also resets the 51xx
      
      if(sub_cpu_reset) {
	      current_cpu = 1; ResetZ80(&cpu[1]);
	      current_cpu = 2; ResetZ80(&cpu[2]);
      }	
    }
    return;
  }
  
  if(Value == 0 && Addr == 0x8210) { // gg1-4.s L1725
    game_started = 1;
    printf("game_started\n");
  }
  
  if((Addr & 0xfff0) == 0xa000) {
    if(Value & 1)  starcontrol |=  (1<<(Addr & 7));   // set bit
    else           starcontrol &= ~(1<<(Addr & 7));   // clear bit
    return;
  }
  
#if 0
  // inject a fixed score to be able to check hiscore routines easily
  if(game_started && (Addr == 0x8000 + 1020) && (memory[1017] == 0) && (memory[1018] == 0x24)) {
    // start with score = 23450
      memory[1020] = 2; memory[1019] = 3; memory[1018] = 4; memory[1017] = 5; 
  }     
#endif
}

void galaga::run_frame(void) {
  for(int i=0; i < INST_PER_FRAME; i++) {
    current_cpu = 0;
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
    if(!sub_cpu_reset) {
      current_cpu = 1;
      StepZ80(cpu+1); StepZ80(cpu+1); StepZ80(cpu+1); StepZ80(cpu+1);       
      current_cpu = 2;
      StepZ80(cpu+2); StepZ80(cpu+2); StepZ80(cpu+2); StepZ80(cpu+2);       
    }

    if(namco_busy) namco_busy--;

    // nmi counter for cpu0
    if((cs_ctrl & 0xe0) != 0) {
      // ctr_ctrl[7:5] * 64 * 330ns (hl)->(de)
      if(nmi_cnt < (cs_ctrl >> 5) * 64) {
        nmi_cnt++;
      } else {
        current_cpu = 0;
        IntZ80(&cpu[0], INT_NMI);
        nmi_cnt = 0;          
      }
    } 

    // run cpu2 nmi at ~line 64 and line 192
    if(!sub_cpu_reset && !irq_enable[2] &&
       ((i == INST_PER_FRAME / 4) || (i == 3 * INST_PER_FRAME / 4))) {
      current_cpu = 2;
      IntZ80(&cpu[2], INT_NMI);
    }
  }
    
  if(irq_enable[0]) {
    current_cpu = 0;
    IntZ80(&cpu[0], INT_RST38);
  }

  if(!sub_cpu_reset && irq_enable[1]) {
    current_cpu = 1;
    IntZ80(&cpu[1], INT_RST38);
  }
}

void galaga::prepare_frame(void) {
  // Do all the preparations to render a screen.

#ifdef LED_PIN
  led_state = 0;
#endif
  
  /* preprocess sprites */
  active_sprites = 0;
  for(int idx = 0; idx < 64 && active_sprites < 92; idx++) {
    unsigned char *sprite_base_ptr = memory + 2*(63 - idx);
    // check if sprite is visible
    if ((sprite_base_ptr[0x1b80 + 1] & 2) == 0) {
      struct sprite_S spr;     
      
      spr.code = sprite_base_ptr[0x0b80];
      spr.color = sprite_base_ptr[0x0b80 + 1];
      spr.flags = sprite_base_ptr[0x1b80];
      spr.x = sprite_base_ptr[0x1380] - 16;
      spr.y = sprite_base_ptr[0x1380 + 1] +
	      0x100*(sprite_base_ptr[0x1b80 + 1] & 1) - 40;

      if((spr.code < 128) &&
    	  (spr.y > -16) && (spr.y < 288) &&
	      (spr.x > -16) && (spr.x < 224)) {      

#ifdef LED_PIN
        check_galaga_sprite(&spr);
#endif

    	  // save sprite in list of active sprites
	      sprite[active_sprites] = spr;
      	// for horizontally doubled sprites, this one becomes the code + 2 part
	      if(spr.flags & 0x08) sprite[active_sprites].code += 2;	
	      active_sprites++;
      }

      // handle horizontally doubled sprites
      if((spr.flags & 0x08) &&
    	   (spr.y > -16) && (spr.y < 288) &&
    	   ((spr.x+16) >= -16) && ((spr.x+16) < 224)) {
	      // place a copy right to the current one
	      sprite[active_sprites] = spr;
	      sprite[active_sprites].x += 16;
	      active_sprites++;
      }

      // handle vertically doubled sprites
      // (these don't seem to happen in galaga)
      if((spr.flags & 0x04) &&
	       ((spr.y+16) > -16) && ((spr.y+16) < 288) && 
	      (spr.x > -16) && (spr.x < 224)) {      
	      // place a copy below the current one
	      sprite[active_sprites] = spr;
	      sprite[active_sprites].code += 3;
	      sprite[active_sprites].y += 16;
	      active_sprites++;
      }
	
      // handle in both directions doubled sprites
      if(((spr.flags & 0x0c) == 0x0c) &&
	       ((spr.y+16) > -16) && ((spr.y+16) < 288) &&
	       ((spr.x+16) > -16) && ((spr.x+16) < 224)) {
	      // place a copy right and below the current one
	      sprite[active_sprites] = spr;
	      sprite[active_sprites].code += 1;
	      sprite[active_sprites].x += 16;
	      sprite[active_sprites].y += 16;
	      active_sprites++;
      }
    }
  }

   /* the screen is only updated every second frame, scroll speed is thus doubled */
  static const signed char speeds[8] = { -1, -2, -3, 0, 3, 2, 1, 0 };
  stars_scroll_y += 2 * speeds[starcontrol & 7];
}

void galaga::check_galaga_sprite(struct sprite_S *spr) {
  if(game_started) {
    // this is a visible sprite. Extract information for LEDs if needed
    // printf("SPR %d = %d @ %d %d\n", idx, spr.code, spr.x, spr.y);

    // sprite code 6 at y == 257 is the players ship,
    // below are remaining ships (which we ignore)
    if(spr->code >= 0 && spr->code <= 7 && spr->y <= 257) {
      // printf("SHIP %d/%d @ %d %d\n", spr->code, spr->color, spr->x, spr->y);

      if(spr->y == 257 && spr->code == 6)
      	led_state = 1;    // normal fighter
	    
      // color 9 is regular ship color, color 7 is red captured one
      if(spr->color == 7)
      	led_state = 2;    // captured fighter
    }

    // check for exploding enemies
    if(spr->code >= 0x41 && spr->code <= 0x4b) {
      // printf("EXPLODE %d @ %d/%d\n", spr->code-0x41, spr->x, spr->y);
      led_state = 3 + spr->code-0x41;
    }
    
    // check for score marks
    //	  if(spr->code >= 0x34 && spr->code <= 0x3a)
    //	    printf("SCORE %d @ %d/%d\n", spr->code-0x34, spr->x, spr->y);
  }
}

void galaga::trigger_sound_explosion(void) {
  if(game_started) {
    snd_boom_cnt = 2 * sizeof(galaga_sample_boom);
    snd_boom_ptr = (const signed char*)galaga_sample_boom;
  }
}

// draw a single 8x8 tile
void galaga::blit_tile(short row, char col) {
  unsigned short addr = tileaddr[row][col];

  // skip blank galaga tiles (0x24) in rendering  
  if(memory[addr] == 0x24) return;
  
  const unsigned short *tile = galaga_tilemap[memory[addr]];
  const unsigned short *colors = galaga_colormap_tiles[memory[0x400 + addr] & 63];  

  unsigned short *ptr = frame_buffer + 8*col;

  // 8 pixel rows per tile
  for(char r = 0; r < 8; r++, ptr += (224 - 8)) {
    unsigned short pix = *tile++;
    // 8 pixel columns per tile
    for(char c=0;c<8;c++,pix>>=2) {
      if(pix & 3) *ptr = colors[pix&3];
      ptr++;
    }
  }
}

// render a single 16x16 sprite. This is called multiple times for
// double sized sprites. This renders onto a single 224 x 8 tile row
// thus will be called multiple times even for single sized sprites
void galaga::blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = galaga_sprites[sprite[s].flags & 3][sprite[s].code];
  const unsigned short *colors = galaga_colormap_sprites[sprite[s].color & 63];
  if(colors[0] != 0) return;   // not a valid colormap entry

  // create mask for sprites that clip left or right
  unsigned long mask = 0xffffffff;
  if(sprite[s].x < 0)      mask <<= -2*sprite[s].x;
  if(sprite[s].x > 224-16) mask >>= (2*(sprite[s].x-(224-16)));		

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
      if(col) *ptr = col;
      ptr++;
    }
  }
}

void galaga::render_stars_set(short row, const struct galaga_star *set) {    
  for(char star_cntr = 0;star_cntr < 63 ;star_cntr++) {
    const struct galaga_star *s = set+star_cntr;

    unsigned short x = (244 - s->x) & 0xff;
    unsigned short y = ((s->y + stars_scroll_y) & 0xff) + 16 - row * 8;

    if(y < 8 && x < 224)
      frame_buffer[224*y + x] = s->col;
  }     
}

void galaga::render_row(short row) {
  if(starcontrol & 0x20) {
    /* two sets of stars controlled by these bits */
    render_stars_set(row, galaga_star_set[(starcontrol & 0x08)?1:0]);
    render_stars_set(row, galaga_star_set[(starcontrol & 0x10)?3:2]);
  }
  
  // render sprites
  for(unsigned char s=0;s<active_sprites;s++) {
    // check if sprite is visible on this row
    if((sprite[s].y < 8*(row+1)) && ((sprite[s].y+16) > 8*row))
      blit_sprite(row, s);
  }
  
  // render 28 tile columns per row
  for(char col=0;col<28;col++)
    blit_tile(row, col);
} 

const signed char * galaga::waveRom(unsigned char value) {
  return galaga_wavetable[value]; 
}

const unsigned short *galaga::logo(void) {
  return galaga_logo;
}

#ifdef LED_PIN
void galaga::gameLeds(CRGB *leds) {
  static const CRGB galaga_leds[][NUM_LEDS] = {
      {  LED_BLACK,  LED_BLACK,  LED_BLACK,  LED_BLACK,  LED_BLACK,  LED_BLACK,  LED_BLACK },  // 0
      {    LED_RED,   LED_BLUE,  LED_WHITE,  LED_WHITE,  LED_WHITE,   LED_BLUE,    LED_RED },  // 1
      {  LED_WHITE,   LED_BLUE,    LED_RED,    LED_RED,    LED_RED,   LED_BLUE,  LED_WHITE },  // 2
      {  LED_BLACK,  LED_BLACK,  LED_BLACK, LED_YELLOW,  LED_BLACK,  LED_BLACK,  LED_BLACK },  // 3   
      {  LED_BLACK,  LED_BLACK, LED_YELLOW,   LED_BLUE, LED_YELLOW,  LED_BLACK,  LED_BLACK },  // 4
      {  LED_BLACK, LED_YELLOW,   LED_BLUE,   LED_CYAN,   LED_BLUE, LED_YELLOW,  LED_BLACK },  // 5
      { LED_YELLOW,   LED_BLUE,   LED_CYAN,    LED_RED,   LED_CYAN,   LED_BLUE, LED_YELLOW },  // 6
      {   LED_BLUE,   LED_CYAN,    LED_RED,  LED_WHITE,    LED_RED,   LED_CYAN,   LED_BLUE },  // 7
      {   LED_CYAN,    LED_RED,  LED_WHITE, LED_YELLOW,  LED_WHITE,    LED_RED,   LED_CYAN },  // 8
      {    LED_RED,  LED_WHITE, LED_YELLOW,  LED_WHITE, LED_YELLOW,  LED_WHITE,    LED_RED },  // 9
      {  LED_WHITE, LED_YELLOW,  LED_WHITE,  LED_BLACK,  LED_WHITE, LED_YELLOW,  LED_WHITE },  // 10
      { LED_YELLOW,  LED_WHITE,  LED_BLACK,  LED_BLACK,  LED_BLACK, LED_WHITE,  LED_YELLOW },  // 11
      {  LED_WHITE,  LED_BLACK,  LED_BLACK,  LED_BLACK, LED_BLACK,  LED_BLACK,   LED_WHITE }   // 12
    };
  
  memcpy(leds, galaga_leds + led_state, NUM_LEDS*sizeof(CRGB));    
}

void galaga::menuLeds(CRGB *leds) {
  memcpy(leds, menu_leds, NUM_LEDS*sizeof(CRGB));
}
#endif