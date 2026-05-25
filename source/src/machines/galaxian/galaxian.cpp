#include "galaxian.h"

unsigned char galaxian::opZ80(unsigned short Addr) {
  // Galaxian hardware ignores A15 — 0x8000-0xFFFF mirrors 0x0000-0x7FFF
  Addr &= 0x7FFF;

  if(Addr < 0x4000)
    return galaxian_rom[Addr];

  return 0xff;
}

unsigned char galaxian::rdZ80(unsigned short Addr) {
  // Galaxian hardware ignores A15 — 0x8000-0xFFFF mirrors 0x0000-0x7FFF
  Addr &= 0x7FFF;

  if(Addr < 0x4000)
    return galaxian_rom[Addr];

  if((Addr & 0xf800) == 0x4000)
    return memory[Addr - 0x4000];

  if((Addr & 0xfc00) == 0x5000)
    return memory[Addr - 0x5000 + 0x0800];

  if((Addr & 0xf800) == 0x5800)
    return memory[Addr - 0x5800 + 0x0C00];

  if((Addr & 0xf800) == 0x6000) {
    unsigned char keymask = input->buttons_get();
    unsigned char retval = GALAXIAN_DIP_IN0;
    if(keymask & BUTTON_COIN)   retval |= 0x01;
    if(keymask & BUTTON_LEFT)   retval |= 0x04;
    if(keymask & BUTTON_RIGHT)  retval |= 0x08;
    if(keymask & BUTTON_FIRE)   retval |= 0x10;
    return retval;
  }

  if((Addr & 0xf800) == 0x6800) {
    unsigned char keymask = input->buttons_get();
    unsigned char retval = GALAXIAN_DIP_IN1;
    if(keymask & BUTTON_START)  retval |= 0x01;
    return retval;
  }

  if((Addr & 0xf800) == 0x7000) {
    return GALAXIAN_DIP_IN2;
  }

  // Watchdog
  if((Addr & 0xf800) == 0x7800) {
    return 0xFF;
  }
  return 0xFF;
}

void galaxian::wrZ80(unsigned short Addr, unsigned char Value) {
  // Galaxian hardware ignores A15 — 0x8000-0xFFFF mirrors 0x0000-0x7FFF
  Addr &= 0x7FFF;

  if((Addr & 0xf800) == 0x4000) {
    memory[Addr - 0x4000] = Value;
    return;
  }

  if((Addr & 0xfc00) == 0x5000) {
    if(!game_started && Addr == 0x5000 + 800 && Value != 0x10)
      game_started = 1;
    memory[Addr - 0x5000 + 0x0800] = Value;
    return;
  }

  if((Addr & 0xf800) == 0x5800) {
    memory[Addr - 0x5800 + 0x0C00] = Value;
    return;
  }

  if((Addr & 0xfff8) == 0x6000) {
    // 0x6000-0x6003: start lamps, coin lockout/counter (ignore)
    // 0x6004-0x6007: LFO DAC bits (4-bit value for background march sound)
    if(Addr >= 0x6004) {
      soundregs[1 + (Addr & 0x03)] = Value & 1;  // soundregs[1-4] = LFO DAC bits
    }
    return;
  }

  if((Addr & 0xfff8) == 0x6800) {
    // 0x6800-0x6807: sound latches (bit 0 only)
    // FS1,FS2,FS3,HIT,n/c,FIRE,VOL1,VOL2
    soundregs[8 + (Addr & 0x07)] = Value & 1;
    return;
  }

  if((Addr & 0xfff8) == 0x7000) {
    unsigned char offset = Addr & 0x07;
    if(offset == 1) {
      irq_enable[0] = Value & 1;  // NMI enable at 0x7001
    }
    if(offset == 4) {
      stars_enabled = (Value & 1);  // stars enable at 0x7004
    }
    return;
  }

  if((Addr & 0xf800) == 0x7800) {
    // 0x7800: pitch register for VCO (tone frequency, always active)
    soundregs[0] = Value;
    return;
  }
}

void galaxian::run_frame(void) {
  for(int i = 0; i < INST_PER_FRAME; i++) {
    StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);
  }

  if(irq_enable[0]) {
    IntZ80(&cpu[0], INT_NMI);
  }
}

void galaxian::prepare_frame(void) {
  // Initialize starfield on first frame
  if(!stars_initialized) stars_init();

  // Scroll stars: advance 1 pixel per frame (slow upward scroll)
  if(stars_enabled) {
    star_scroll_offset = (star_scroll_offset + 1) % 288;
  }

  active_sprites = 0;

  // Sprite data at spriteram + 0x40 (HW 0x5840), 4 bytes per sprite
  // base[0]=Y  base[1]=code|flipx|flipy  base[2]=color  base[3]=X
  for(int idx = 7; idx >= 0 && active_sprites < 92; idx--) {
    struct sprite_S spr;

    unsigned char *base = memory + 0x0C40 + idx * 4;

    spr.code = base[1] & 0x3f;
    spr.flags = (base[1] >> 6) & 3;
    spr.color = base[2] & 7;

    // 90° rotation: raw base[0] maps directly to portrait X (same as Anteater/Frogger)
    // MAME landscape sy = 240 - base[0], portrait x = base[0] - 16
    spr.x = base[0] - 16;
    spr.y = base[3] + 16;

    if(base[3] && (spr.y > -16) && (spr.y < 288) && (spr.x > -16) && (spr.x < 224)) {
      sprite[active_sprites++] = spr;
    }
  }

  // Bullet data at spriteram + 0x60 (HW 0x5860), 4 bytes per bullet
  // MAME format: base[1]=Y complement, base[3]=X position
  // Bullet visible at MAME scanline = 255-base[1], MAME x = 255-base[3]
  // 90° rotation: MAME scanline → galagino X, MAME x → galagino Y
  // Indices 0-6 = enemy shells (white), index 7 = player missile (yellow)
  bullet_active = 0;
  for(int idx = 0; idx < 8; idx++) {
    unsigned char *bbase = memory + 0x0C60 + idx * 4;
    // galagino X = must match tile scroll direction (ship uses scroll registers)
    // The scroll shifts tiles RIGHT with increasing value.
    // bbase[1] encodes the bullet's scanline position in the same direction as scroll.
    bullet_x[idx] = bbase[1] - 16;
    // galagino Y = MAME X position: (255 - bbase[3]) + 15 - 4 (bullet width adjust)
    bullet_y[idx] = 266 - bbase[3];
    if(bbase[1] && bbase[3] && bullet_x[idx] >= 0 && bullet_x[idx] < 224 &&
       bullet_y[idx] > -4 && bullet_y[idx] < 288)
      bullet_active |= (1 << idx);
  }
}

void galaxian::blit_tile(short row, char col) {
  if((row < 2) || (row >= 34))
    return;

  unsigned short addr = tileaddr[row][col];
  const unsigned short *tile = galaxian_tilemap[memory[0x0800 + addr]];
  int c = memory[0x0C00 + 2 * (addr & 31) + 1] & 7;
  const unsigned short *colors = galaxian_colormap[c];
  unsigned short *ptr = frame_buffer + 8 * col;

  for(char r = 0; r < 8; r++, ptr += (224 - 8)) {
    unsigned short pix = *tile++;
    for(char c = 0; c < 8; c++, pix >>= 2) {
      long index = ((pix & 2) >> 1) | ((pix & 1) << 1);
      if(pix & 3) *ptr = colors[index];
      ptr++;
    }
  }
}

// Render a tile with horizontal scroll offset (for player ship movement)
void galaxian::blit_tile_scroll(short row, signed char col, unsigned char scroll) {
  if((row < 2) || (row >= 34))
    return;

  unsigned short addr;
  unsigned short mask = 0xffff;
  int sub = scroll & 0x07;

  if(col >= 0) {
    addr = tileaddr[row][col];
    // shift tile selection by scroll/8 tiles (each tile = 32 bytes in VRAM)
    addr = (addr + ((scroll & ~7) << 2)) & 1023;
    // clip rightmost tile if partial
    if((sub != 0) && (col == 27))
      mask = 0xffff >> (2 * sub);
  } 
  else {
    // col == -1: partial tile at left edge
    addr = tileaddr[row][0];
    addr = (addr + 32 + ((scroll & ~7) << 2)) & 1023;
    mask = 0xffff << (2 * (8 - sub));
  }

  const unsigned short *tile = galaxian_tilemap[memory[0x0800 + addr]];
  int c = memory[0x0C00 + 2 * (addr & 31) + 1] & 7;
  const unsigned short *colors = galaxian_colormap[c];
  unsigned short *ptr = frame_buffer + 8 * col + sub;

  for(char r = 0; r < 8; r++, ptr += (224 - 8)) {
    unsigned short pix = *tile++ & mask;
    for(char c = 0; c < 8; c++, pix >>= 2) {
      long index = ((pix & 2) >> 1) | ((pix & 1) << 1);
      if(pix & 3) *ptr = colors[index];
      ptr++;
    }
  }
}

void galaxian::blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = galaxian_spritemap[sprite[s].flags & 3][sprite[s].code];
  const unsigned short *colors = galaxian_colormap[sprite[s].color];

  unsigned long mask = 0xffffffff;
  if(sprite[s].x < 0)       mask <<= -2 * sprite[s].x;
  if(sprite[s].x > 224 - 16) mask >>= 2 * (sprite[s].x - (224 - 16));

  short y_offset = sprite[s].y - 8 * row;

  unsigned char lines2draw = 8;
  if(y_offset < -8) lines2draw = 16 + y_offset;

  unsigned short startline = 0;
  if(y_offset > 0) {
    startline = y_offset;
    lines2draw = 8 - y_offset;
  }

  if(y_offset < 0) spr -= y_offset;

  unsigned short *ptr = frame_buffer + sprite[s].x + 224 * startline;

  for(char r = 0; r < lines2draw; r++, ptr += (224 - 16)) {
    unsigned long pix = *spr++ & mask;
    for(char c = 0; c < 16; c++, pix >>= 2) {
      long index = ((pix & 2) >> 1) | ((pix & 1) << 1);
      if(pix & 3) *ptr = colors[index];
      ptr++;
    }
  }
}

// Convert 8-bit R,G,B to RGB565 byte-swapped for ESP32 SPI
static inline unsigned short rgb_to_swapped565(unsigned char r, unsigned char g, unsigned char b) {
  unsigned short c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  return (c >> 8) | (c << 8);  // byte-swap
}

void galaxian::stars_init() {
  // MAME algorithm: 17-bit LFSR, period 2^17-1 = 131071
  // Stars visible when upper 8 bits == 0xFF and bit 0 == 0
  // Color from bits 3-8 (6-bit, 64 colors)
  static const unsigned char starmap[4] = { 0, 150, 200, 255 };

  unsigned short star_color_lut[64];
  for(int i = 0; i < 64; i++) {
    unsigned char r = starmap[((i >> 4) & 2) | ((i >> 5) & 1)];
    unsigned char g = starmap[((i >> 2) & 2) | ((i >> 3) & 1)];
    unsigned char b = starmap[((i >> 0) & 2) | ((i >> 1) & 1)];
    star_color_lut[i] = rgb_to_swapped565(r, g, b);
  }

  // Run the LFSR and collect visible stars
  star_count = 0;
  uint32_t shiftreg = 0;
  for(int i = 0; i < 131071 && star_count < GAL_MAX_STARS; i++) {
    if((shiftreg & 0x1fe01) == 0x1fe00) {
      int color_idx = (~shiftreg >> 3) & 0x3f;
      int x = (i % 512) / 2;
      int y = i / 512;

      if((y ^ (x >> 3)) & 1) {
        int gx = 255 - y;
        int gy = x + 16;

        if(gx >= 0 && gx < 224 && gy >= 16 && gy < 288) {
          stars[star_count].x = gx;
          stars[star_count].y = gy;
          stars[star_count].color = star_color_lut[color_idx];
          star_count++;
        }
      }
    }
    shiftreg = (shiftreg >> 1) | ((((shiftreg >> 12) ^ ~shiftreg) & 1) << 16);
  }
  stars_initialized = true;
}

void galaxian::render_row(short row) {
  if(row <= 1 || row >= 34) return;

  // Draw stars BEFORE tiles (stars are background, tiles overwrite)
  if(stars_enabled && stars_initialized) {
    int row_top = 8 * row;
    int row_bot = row_top + 8;
    for(int i = 0; i < star_count; i++) {
      int sy = ((int)stars[i].y + star_scroll_offset) % 288;
      if(sy < 16) sy += 272;
      if(sy >= row_top && sy < row_bot) {
        int sx = stars[i].x;
        if(sx >= 0 && sx < 224) {
          int fb_idx = (sy - row_top) * 224 + sx;
          if(frame_buffer[fb_idx] == 0x0000) {
            frame_buffer[fb_idx] = stars[i].color;
          }
        }
      }
    }
  }

  // Read scroll register for this portrait row (per-column scroll in MAME terms)
  // ObjRAM even bytes at 0x5800+2*col → memory[0x0C00+2*(row-2)]
  unsigned char scroll = memory[0x0C00 + 2 * (row - 2)];

  if(scroll == 0) {
    for(char col = 0; col < 28; col++)
      blit_tile(row, col);
  } 
  else {
    // partial tile at left edge when sub-tile scroll active
    if(scroll & 7)
      blit_tile_scroll(row, -1, scroll);
    for(char col = 0; col < 28; col++)
      blit_tile_scroll(row, col, scroll);
  }

  for(unsigned char s = 0; s < active_sprites; s++) {
    if((sprite[s].y < 8 * (row + 1)) && ((sprite[s].y + 16) > 8 * row))
      blit_sprite(row, s);
  }

  // Draw bullets: 4-pixel vertical lines
  // Colors: shells (0-6) = white 0xFFFF, player missile (7) = yellow 0xE0FF (byte-swapped)
  if(bullet_active) {
    short row_top = 8 * row;
    short row_bot = row_top + 8;
    for(int b = 0; b < 8; b++) {
      if(!(bullet_active & (1 << b))) continue;
      // Bullet is 4 pixels tall in portrait mode (was 4 pixels wide in landscape)
      short bx = bullet_x[b];
      short by = bullet_y[b];
      if(bx < 0 || bx >= 224) continue;
      if(by + 4 <= row_top || by >= row_bot) continue;

      unsigned short color = (b == 7) ? 0xE0FF : 0xFFFF;  // yellow for player, white for enemies
      for(int py = 0; py < 4; py++) {
        short sy = by + py;
        if(sy >= row_top && sy < row_bot) {
          frame_buffer[(sy - row_top) * 224 + bx] = color;
        }
      }
    }
  }
}

const unsigned short *galaxian::logo(void) {
  return galaxian_logo;
}

#ifdef LED_PIN
void galaxian::gameLeds(CRGB *leds) {
  static char sub_cnt = 0;
  if(sub_cnt++ == 32) {
    sub_cnt = 0;
    static char led = 0;
    char il = (led < NUM_LEDS) ? led : ((2 * NUM_LEDS - 2) - led);
    for(char c = 0; c < NUM_LEDS; c++) {
      if(c == il) leds[c] = LED_YELLOW;
      else        leds[c] = LED_BLUE;
    }
    led = (led + 1) % (2 * NUM_LEDS - 2);
  }
}

void galaxian::menuLeds(CRGB *leds) {
  memcpy(leds, menu_leds, NUM_LEDS * sizeof(CRGB));
}
#endif