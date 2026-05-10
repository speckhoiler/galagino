#include "timeplt.h"

unsigned char timeplt::opZ80(unsigned short Addr) {
  if(current_cpu == 0) {
    if(Addr < 0x6000)
      return timeplt_rom[Addr];
  } 
  else {
    // Sound CPU ROM 0x0000-0x0FFF (4KB data)
    if(Addr < 0x1000)
      return timeplt_snd_rom[Addr];
    // 0x1000-0x2FFF mapped as ROM but no data (returns 0xFF)
  }
  return 0xff;
}

unsigned char timeplt::rdZ80(unsigned short Addr) {
  if(current_cpu == 1) {
    // ---- Sound CPU memory map (MAME: timeplt_sound_map) ----
    // ROM 0x0000-0x2FFF (only 0x0000-0x0FFF has data)
    if(Addr < 0x1000)
      return timeplt_snd_rom[Addr];
  
    if(Addr < 0x3000)
      return 0xFF;  // unmapped ROM space

    // RAM 0x3000-0x33FF mirrored across 0x3000-0x3FFF
    if(Addr >= 0x3000 && Addr <= 0x3FFF)
      return snd_ram[Addr & 0x3FF];

    // AY#1 data read (0x4000, mirrored 0x4000-0x4FFF)
    if((Addr & 0xF000) == 0x4000) {
      unsigned char reg = ay_addr[0] & 0x0F;
      if(reg == 14) return soundlatch;         // Port A = soundlatch
      if(reg == 15) {
        // Port B = timer: LS90 bi-quinary counter, divide-by-5120
        // MAME: timeplt_timer[(total_cycles / 512) % 10]
        static const unsigned char timeplt_timer[10] = {
          0x00, 0x10, 0x20, 0x30, 0x40, 0x90, 0xa0, 0xb0, 0xa0, 0xd0
        };
        return timeplt_timer[(snd_icnt / 45) % 10]; // regolare questo numero per dettare la giusta velocità dell'audio
      }
      return ay_regs[0][reg];
    }

    // AY#2 data read (0x6000, mirrored 0x6000-0x6FFF)
    if((Addr & 0xF000) == 0x6000) {
      unsigned char reg = ay_addr[1] & 0x0F;
      return ay_regs[1][reg];
    }

    return 0xFF;
  }

  // ---- Main CPU memory map ----
  // ROM 0x0000-0x5FFF
  if(Addr < 0x6000)
    return timeplt_rom[Addr];

  // Color RAM 0xA000-0xA3FF
  if(Addr >= 0xA000 && Addr <= 0xA3FF)
    return memory[MEM_COLORRAM + (Addr & 0x03FF)];

  // Video RAM 0xA400-0xA7FF
  if(Addr >= 0xA400 && Addr <= 0xA7FF)
    return memory[MEM_VIDEORAM + (Addr & 0x03FF)];

  // Work RAM 0xA800-0xAFFF
  if(Addr >= 0xA800 && Addr <= 0xAFFF)
    return memory[MEM_WORKRAM + (Addr & 0x07FF)];

  // Sprite RAM bank 0: 0xB000-0xB3FF (mirrors 256 bytes)
  if((Addr & 0xFC00) == 0xB000)
    return memory[MEM_SPRITES0 + (Addr & 0xFF)];

  // Sprite RAM bank 1: 0xB400-0xB7FF (mirrors 256 bytes)
  if((Addr & 0xFC00) == 0xB400)
    return memory[MEM_SPRITES1 + (Addr & 0xFF)];

  // I/O reads
  if(Addr == 0xC000) {
    multiplexUsed = 1;
    return scanline_counter;
  }

  if(Addr == 0xC200)
    return TIMEPLT_DSW2 | (input->demoSoundsOff() ? TIMEPILOT_DSW2_DEMO_SOUND_OFF : TIMEPILOT_DSW2_DEMO_SOUND_ON);

  if(Addr == 0xC300) {
    // IN0: coins, start (active-LOW: 0xFF = idle, clear bit = pressed)
    unsigned char keymask = input->buttons_get();
    unsigned char retval = 0xFF;
    if(keymask & BUTTON_COIN)   retval &= ~0x01;  // coin 1
    if(keymask & BUTTON_START)  retval &= ~0x08;  // start 1
    return retval;
  }

  if(Addr == 0xC320) {
    // IN1: P1 joystick + fire (active-LOW)
    unsigned char keymask = input->buttons_get();
    unsigned char retval = 0xFF;
    if(keymask & BUTTON_LEFT)   retval &= ~0x01;
    if(keymask & BUTTON_RIGHT)  retval &= ~0x02;
    if(keymask & BUTTON_UP)     retval &= ~0x04;
    if(keymask & BUTTON_DOWN)   retval &= ~0x08;
    if(keymask & BUTTON_FIRE)   retval &= ~0x10;
    return retval;
  }

  if(Addr == 0xC340) {
    // IN2: P2 (active-LOW, idle)
    return 0xFF;
  }

  if(Addr == 0xC360)
    return TIMEPLT_DSW1;

  return 0x00;
}

void timeplt::wrZ80(unsigned short Addr, unsigned char Value) {
  if(current_cpu == 1) {
    // ---- Sound CPU writes (MAME: timeplt_sound_map) ----
    // RAM 0x3000-0x3FFF (1KB mirrored)
    if(Addr >= 0x3000 && Addr <= 0x3FFF) {
      snd_ram[Addr & 0x3FF] = Value;
      return;
    }

    // AY#1 data write (0x4000-0x4FFF mirrored)
    if((Addr & 0xF000) == 0x4000) {
      unsigned char reg = ay_addr[0] & 0x0F;
      ay_regs[0][reg] = Value;
      if(reg < 14) soundregs[reg] = Value;  // copy to audio engine
      return;
    }

    // AY#1 address write (0x5000-0x5FFF mirrored)
    if((Addr & 0xF000) == 0x5000) {
      ay_addr[0] = Value & 0x0F;
      return;
    }

    // AY#2 data write (0x6000-0x6FFF mirrored)
    if((Addr & 0xF000) == 0x6000) {
      unsigned char reg = ay_addr[1] & 0x0F;
      ay_regs[1][reg] = Value;
      if(reg < 14) soundregs[16 + reg] = Value;  // copy to audio engine
      return;
    }

    // AY#2 address write (0x7000-0x7FFF mirrored)
    if((Addr & 0xF000) == 0x7000) {
      ay_addr[1] = Value & 0x0F;
      return;
    }

    // Filter control writes 0x8000-0xFFFF (ignore, no RC filters in galagino)
    return;
  }

  // ---- Main CPU writes ----
  // Color RAM 0xA000-0xA3FF
  if(Addr >= 0xA000 && Addr <= 0xA3FF) {
    memory[MEM_COLORRAM + (Addr & 0x03FF)] = Value;
    return;
  }

  // Video RAM 0xA400-0xA7FF
  if(Addr >= 0xA400 && Addr <= 0xA7FF) {
    if(!game_started)
      game_started = 1;
    memory[MEM_VIDEORAM + (Addr & 0x03FF)] = Value;
    return;
  }

  // Work RAM 0xA800-0xAFFF
  if(Addr >= 0xA800 && Addr <= 0xAFFF) {
    memory[MEM_WORKRAM + (Addr & 0x07FF)] = Value;
    return;
  }

  // Sprite RAM bank 0: 0xB000-0xB3FF (mirrors 256 bytes)
  if((Addr & 0xFC00) == 0xB000) {
    memory[MEM_SPRITES0 + (Addr & 0xFF)] = Value;
    return;
  }

  // Sprite RAM bank 1: 0xB400-0xB7FF (mirrors 256 bytes)
  if((Addr & 0xFC00) == 0xB400) {
    memory[MEM_SPRITES1 + (Addr & 0xFF)] = Value;
    return;
  }

  // Sound latch 0xC000
  if(Addr == 0xC000) {
    soundlatch = Value;
    return;
  }

  // Watchdog 0xC200
  if(Addr == 0xC200)
    return;

  // LS259 latch at 0xC300-0xC30F (address bit 1-3 select output, data bit 0 = value)
  if(Addr >= 0xC300 && Addr <= 0xC30F) {
    unsigned char bit_sel = (Addr >> 1) & 0x07;
    unsigned char bit_val = Value & 1;
    switch(bit_sel) {
      case 0: nmi_enable = bit_val; break;           // Q0 = NMI enable
      case 1: break;                                 // Q1 = flip screen (ignored, always upright)
      case 2:                                        // Q2 = sound CPU IRQ trigger
        // MAME: edge-triggered (0→1 only), not level-triggered
        if(bit_val && !snd_irq_last) snd_irq_pending = 1;
        snd_irq_last = bit_val;
        break;
      case 3: break;                                 // Q3 = mute audio (ignore)
      case 4: video_enable = bit_val; break;         // Q4 = video enable
      case 5: break;                                 // Q5 = coin counter 1
      case 6: break;                                 // Q6 = coin counter 2
      case 7: break;                                 // Q7 = unused
    }
    return;
  }
}

// Time Pilot: Main Z80 @ 3.072 MHz + Sound Z80 @ 1.789 MHz + 2x AY-3-8910
void timeplt::run_frame(void) {
  multiplexUsed = 0;
  multiplexUsedCopy = 0;
  scanline_counter = 0;
 
  for(int i = 0; i < 1280; i++) {
    current_cpu = 0;
    StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);

    current_cpu = 1;
    StepZ80(&cpu[1]); snd_icnt++;
    StepZ80(&cpu[1]); snd_icnt++;

    if ((i % 5) == 0) scanline_counter++;
 
    // "latch" IRQ: only deliver when sound CPU has interrupts enabled (EI)
    // Same pattern as Frogger — prevents lost IRQs during DI periods
    if(snd_irq_pending && (cpu[1].IFF & IFF_1)) {
      IntZ80(&cpu[1], INT_RST38);
      snd_irq_pending = 0;
    }
    
    if (multiplexUsed && !multiplexUsedCopy) {
      multiplexUsedCopy = 1;
      memcpy(multiplexBank0, &memory[MEM_SPRITES0], 0x100);
      memcpy(multiplexBank1, &memory[MEM_SPRITES1], 0x100);
    }
  }

  if (!multiplexUsed) {
    memset(multiplexBank0, 0, sizeof(multiplexBank0));
    memset(multiplexBank1, 0, sizeof(multiplexBank1));
  }

  // Main CPU: NMI at VBlank
  if(nmi_enable) {
    current_cpu = 0;
    IntZ80(&cpu[0], INT_NMI);
  }
}

// Helper: extract sprites from a sprite RAM buffer pair into the sprite list
// Sprites use LANDSCAPE orientation (no pre-rotation in romconv).
// Transposed rendering in blit_sprite handles the ROT90 display rotation.
// Coordinates derived from: MAME portrait → frame buffer with 180° MADCTL display flip.
void timeplt::extract_sprites(const unsigned char *bank0, const unsigned char *bank1) {
  for(int offs = 0x3E; offs >= 0x10 && active_sprites < 128; offs -= 2) {
    struct sprite_S spr;

    unsigned char sx_raw = bank0[offs];           // landscape X position
    unsigned char sy_raw = bank1[offs + 1];       // landscape Y position
    unsigned char code   = bank0[offs + 1];       // sprite code
    unsigned char attr   = bank1[offs];            // color + flip

    unsigned char color = attr & 0x3F;          // 6-bit color
    unsigned char flipx = (~attr >> 6) & 1;     // bit6 inverted = flipX
    unsigned char flipy = (attr >> 7) & 1;      // bit7 = flipY

    // MAME: sy = 241 - spriteram2[offs+1]
    int sy = 241 - sy_raw;

    // Skip offscreen sprites
    if(sy <= -16 || sy >= 256) continue;

    // Transposed coordinate mapping for landscape sprites + 180° MADCTL display flip:
    // ROM row (landscape dy) → screen X (reversed via 15-r in blit)
    // ROM col (landscape dx) → screen Y (forward)
    spr.x = (int)sy_raw - 17;
    spr.y = (int)sx_raw + 16;

    spr.code = code;
    spr.color = color;

    // Landscape sprite orientations: [0]=normal, [1]=flipY, [2]=flipX, [3]=flipXY
    // landscape flipX → orientation bit 1 (col reversal → screen Y flip)
    // landscape flipY → orientation bit 0 (row reversal → screen X flip)
    // No XOR 3 needed — transposed rendering + display 180° handles the rotation
    spr.flags = (flipy ? 1 : 0) | (flipx ? 2 : 0);

    if((spr.y > -16) && (spr.y < 304) && (spr.x > -16) && (spr.x < 224)) {
      sprite[active_sprites++] = spr;
    }
  }
}

void timeplt::prepare_frame(void) {
  active_sprites = 0;

  if(!video_enable) return;

 // saved sprites
  extract_sprites(multiplexBank0, multiplexBank1);
  // current sprites
  extract_sprites(&memory[MEM_SPRITES0], &memory[MEM_SPRITES1]);
}

void timeplt::blit_tile(short row, char col) {
  blit_tile_cat(row, col, -1);  // -1 = draw all (no category filter)
}

void timeplt::blit_tile_cat(short row, char col, signed char cat_filter) {
  unsigned short addr = tileaddr[row][col];

  if((row < 2) || (row >= 34))
    return;

  unsigned char vram_val = memory[MEM_VIDEORAM + addr];
  unsigned char cram_val = memory[MEM_COLORRAM + addr];

  // MAME: tileinfo.category = (attr & 0x10) >> 4
  unsigned char category = (cram_val & 0x10) >> 4;

  // If filtering by category, skip non-matching tiles
  if(cat_filter >= 0 && category != (unsigned char)cat_filter)
    return;

  // Tile code: videoram + bit5 of colorram extends code by 256
  // MAME: m_videoram[tile_index] + ((attr & 0x20) << 3)
  unsigned short tile_code = vram_val + ((cram_val & 0x20) << 3);

  // Color palette: 5 bits from colorram
  unsigned char color = cram_val & 0x1F;

  // Flip flags: ROT90 swaps axes — bit6 (landscape flipX) → portrait flip_y
  //                                 bit7 (landscape flipY) → portrait flip_x
  unsigned char flip_x = (cram_val >> 6) & 1;
  unsigned char flip_y = (cram_val >> 7) & 1;

  const unsigned short *tile = timeplt_tilemap[tile_code];
  const unsigned short *colors = timeplt_char_colormap[color];

  unsigned short *ptr = frame_buffer + 8 * col;

  for(char r = 0; r < 8; r++, ptr += (224 - 8)) {
    int src_r = flip_y ? (7 - r) : r;
    unsigned short pix = tile[src_r];
    if(flip_x) {
      for(char c = 0; c < 8; c++) {
        unsigned short p = (pix >> ((7 - c) * 2)) & 3;
        ptr[c] = colors[p];
      }
      ptr += 8;
    } else {
      for(char c = 0; c < 8; c++, pix >>= 2) {
        *ptr = colors[pix & 3];
        ptr++;
      }
    }
  }
}

// Transposed sprite rendering for landscape-oriented sprite data.
// ROM row (landscape dy) → screen X offset, ROM col (landscape dx) → screen Y offset (reversed).
// This preserves multi-sprite tiling: landscape column edges naturally become
// adjacent portrait rows, so clouds and other multi-sprite objects tile correctly.
void timeplt::blit_sprite(short row, unsigned char s) {
  const unsigned long *spr_data = timeplt_spritemap[sprite[s].flags & 3][sprite[s].code];
  const unsigned short *colors = timeplt_sprite_colormap[sprite[s].color];

  int spr_x = sprite[s].x;   // screen X start (ROM rows map here)
  int spr_y = sprite[s].y;   // screen Y start (ROM cols map here, reversed)
  int row_start = 8 * row;

  // Sprite occupies screen Y: [spr_y, spr_y + 15]
  // Current tile row: screen Y [row_start, row_start + 7]

  int y_begin = (spr_y > row_start) ? spr_y : row_start;
  int y_end   = ((spr_y + 15) < (row_start + 7)) ? (spr_y + 15) : (row_start + 7);
  if(y_begin > y_end) return;

  //  int y_begin = (spr_y > row_start) ? spr_y : row_start;
  //  int y_end   = (spr_y + 15); 
  //  if (y_end > row_start + 7) y_end = row_start + 7;
  //  if(y_begin > y_end) return;

  // ROM row r → screen_x = spr_x + (15 - r)  [reversed row for correct orientation]
  // ROM col c = screen_y - spr_y              [forward column for correct tiling]
  for(int r = 0; r < 16; r++) {
    int screen_x = spr_x + 15 - r;

    if(screen_x < 0 || screen_x >= 224) continue;
    unsigned long row_data = spr_data[r];

    for(int screen_y = y_begin; screen_y <= y_end; screen_y++) {
      int c = screen_y - spr_y;   // forward column mapping
      unsigned char px = (row_data >> (c * 2)) & 3;
      if(px) {
        frame_buffer[(screen_y - row_start) * 224 + screen_x] = colors[px];
      }
    }
  }
}

void timeplt::render_row(short row) {
  if(row <= 1 || row >= 34) return;

  if(!video_enable) return;

  // MAME render order: tiles(category 0) → sprites → tiles(category 1)
  // Category 1 tiles are drawn ON TOP of sprites (e.g. "© KONAMI 1982" text)

  // Pass 1: tiles with category 0 (under sprites)
  for(char col = 0; col < 28; col++)
    blit_tile_cat(row, col, 0);

  // Pass 2: sprites
  for(unsigned char s = 0; s < active_sprites; s++) {
    if((sprite[s].y < 8 * (row + 1)) && ((sprite[s].y + 16) > 8 * row))
      blit_sprite(row, s);
  }

  // Pass 3: tiles with category 1 (over sprites)
  for(char col = 0; col < 28; col++)
    blit_tile_cat(row, col, 1);
}

const unsigned short *timeplt::logo(void) {
  return timeplt_logo;
}

#ifdef LED_PIN
void timeplt::gameLeds(CRGB *leds) {
  static char sub_cnt = 0;
  if(sub_cnt++ == 32) {
    sub_cnt = 0;
    static char led = 0;
    char il = (led < NUM_LEDS) ? led : ((2 * NUM_LEDS - 2) - led);
    for(char c = 0; c < NUM_LEDS; c++) {
      if(c == il) leds[c] = LED_CYAN;
      else        leds[c] = LED_WHITE;
    }
    led = (led + 1) % (2 * NUM_LEDS - 2);
  }
}

void timeplt::menuLeds(CRGB *leds) {
  memcpy(leds, menu_leds, NUM_LEDS * sizeof(CRGB));
}
#endif