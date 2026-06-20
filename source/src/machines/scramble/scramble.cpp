#include "scramble.h"

void scramble::start() {
  stars_init();
  ignoreFireButton = 1;
}

unsigned char scramble::opZ80(unsigned short Addr) {
  if (current_cpu == 0 && Addr < 0x4000)
    return scramble_main_rom[Addr];
  if (current_cpu == 1 && Addr < 0x1800)
    return scramble_audio_rom[Addr];

  return 0x00; // 0x00 - NOP 0xff RST38
}

unsigned char scramble::rdZ80(unsigned short Addr) {
  if(current_cpu == 0) {
    if (Addr < CPU1_ROM_SIZE)
      return scramble_main_rom[Addr];

    if (Addr >= CPU1_RAM_ADDR && Addr < CPU1_RAM_ADDR + CPU1_RAM_SIZE)
      return memory[Addr - CPU1_RAM_ADDR];

    // 0x4800 with 0x4c00 mirror mask with 0xfbff
    if ((Addr & 0xfbff) >= CPU1_VRAM_ADDR && (Addr & 0xfbff) < CPU1_VRAM_ADDR + CPU1_VRAM_SIZE)
      return memory[CPU1_VRAM_OFFSET + (Addr & 0x03ff)];

    if (Addr >= CPU1_ATTR_ADDR && Addr < CPU1_ATTR_ADDR + CPU1_ATTR_SIZE)
      return memory[CPU1_ATTR_OFFSET + Addr - CPU1_ATTR_ADDR];

    if (Addr >= CPU1_SPRITE_ADDR && Addr < CPU1_SPRITE_ADDR + CPU1_SPRITE_SIZE)
      return memory[CPU1_SPRITE_OFFSET + Addr - CPU1_SPRITE_ADDR];

    if (Addr >= CPU1_BULLET_ADDR && Addr < CPU1_BULLET_ADDR + CPU1_BULLET_SIZE)
      return memory[CPU1_BULLET_OFFSET + Addr - CPU1_BULLET_ADDR];

    if (Addr >= CPU1_RAM2_ADDR && Addr < CPU1_RAM2_ADDR + CPU1_RAM2_SIZE)
      return memory[CPU1_RAM2_OFFSET + Addr - CPU1_RAM2_ADDR];

    unsigned char keymask;
    unsigned char retval;
    unsigned char bit;

    switch (Addr) {
      case 0x7000: // Watchdog
      case 0x7800:
        return 0xff;
      case 0x8100: // PPI0 - Port A - IN0
      case 0x8110:
        keymask = input->buttons_get();
        retval = SCRAMBLE_IN0_VALUE;

        if(keymask & BUTTON_COIN)  retval &= ~0x80;
        if(keymask & BUTTON_LEFT)  retval &= ~0x20;
        if(keymask & BUTTON_RIGHT) retval &= ~0x10;

        if(ignoreFireButton && !(keymask & BUTTON_FIRE) && (keymask & BUTTON_START))
          ignoreFireButton = 0;

        if(!ignoreFireButton && (keymask & BUTTON_FIRE))  retval &= ~0x08; // Laser
        if(!ignoreFireButton && (keymask & BUTTON_FIRE))  retval &= ~0x02; // Bomb
        return retval;
      case 0x8101: // PPI0 - Port B
      case 0x8111:
        retval = SCRAMBLE_IN1_VALUE;
        keymask = input->buttons_get();

        if(!ignoreFireButton && (keymask & BUTTON_START)) retval &= ~0x80;
        return retval;
      case 0x8102: // PPI0 - Port C
      case 0x8112:
        bit = (protectionResult >> 7) & 1;
        retval = bit ? SCRAMBLE_IN2_VALUE1 : SCRAMBLE_IN2_VALUE0;
        keymask = input->buttons_get();

        if(keymask & BUTTON_DOWN)  retval &= ~0x40;
        if(keymask & BUTTON_UP)    retval &= ~0x10;
        return retval;
      case 0x8200: // PPI1 - Port A
      case 0x8201: // PPI1 - Port B
        return 0xff;
      case 0x8202: // PPI1 - Port C
        return protectionResult;
      case 0x9103:
      case 0x9212:
        return 0xff;
    }
  }
  else {
    if (Addr < CPU2_ROM_SIZE)
      return scramble_audio_rom[Addr];

    if (Addr >= CPU2_RAM_ADDR && Addr < CPU2_RAM_ADDR + CPU2_RAM_SIZE)
      return memory[CPU2_RAM_OFFSET + Addr - CPU2_RAM_ADDR];
  }
  return 0xff;
}

void scramble::wrZ80(unsigned short Addr, unsigned char Value) {
  if (current_cpu == 0) {
    if (Addr >= CPU1_RAM_ADDR && Addr < CPU1_RAM_ADDR + CPU1_RAM_SIZE) {
      memory[Addr - CPU1_RAM_ADDR] = Value;
      return;
    }

    // 0x4800 with 0x4c00 mirror mask with 0xfbff
    if ((Addr & 0xfbff) >= CPU1_VRAM_ADDR && (Addr & 0xfbff) < CPU1_VRAM_ADDR + CPU1_VRAM_SIZE) {
      memory[CPU1_VRAM_OFFSET + (Addr & 0x03ff)] = Value;
      return;
    }

    if (Addr >= CPU1_ATTR_ADDR && Addr < CPU1_ATTR_ADDR + CPU1_ATTR_SIZE) {
      memory[CPU1_ATTR_OFFSET + Addr - CPU1_ATTR_ADDR] = Value;
      return;
    }

    if (Addr >= CPU1_SPRITE_ADDR && Addr < CPU1_SPRITE_ADDR + CPU1_SPRITE_SIZE) {
      memory[CPU1_SPRITE_OFFSET + Addr - CPU1_SPRITE_ADDR] = Value;
      return;
    }

    if (Addr >= CPU1_BULLET_ADDR && Addr < CPU1_BULLET_ADDR + CPU1_BULLET_SIZE) {
      memory[CPU1_BULLET_OFFSET + Addr - CPU1_BULLET_ADDR] = Value;
      return;
    }

    if (Addr >= CPU1_RAM2_ADDR && Addr < CPU1_RAM2_ADDR + CPU1_RAM2_SIZE) {
      memory[CPU1_RAM2_OFFSET + Addr - CPU1_RAM2_ADDR] = Value;
      return;
    }

    switch (Addr) {
      case 0x6801: // NMI
        irq_enable[0] = Value & 1;
        return;
      case 0x6802: // Coin counter
        return;
      case 0x6803: // Blue background
        background_enable = Value & 1;
        game_started = 1;
        return;
      case 0x6804: // Stars enable
        stars_enabled = Value & 1;
        return;
      case 0x6805: // Atlantis
      case 0x6806: // Flip Screen X
      case 0x6807: // Flip Screen Y
      case 0x7005: // NOP
      case 0x8100: // PPI0 - Port A
      case 0x8101: // PPI0 - Port B
      case 0x8102: // PPI0 - Port C
      case 0x8103: // PPI0 - Control
      case 0x8203: // Control Register
        return;
      case 0x8200: // PPI1 - Port A goes to AY
        sound_latch = Value;
        return;
      case 0x8201: // PPI1 - Port B
        if((Value & 0x08) && !snd_irq_last)
          snd_irq_state = Value;
        snd_irq_last = Value & 0x08;
        return;
      case 0x8202: // PPI1 - Port C
        protectionState = (protectionState << 4) | (Value & 0x0f);
        unsigned char num1 = (protectionState >> 8) & 0x0f;
        unsigned char num2 = (protectionState >> 4) & 0x0f;

        switch (protectionState & 0x0f) {
          case 0x6: // scrambles
            protectionResult ^= 0x80;
            break;
          case 0x9: // scramble
            protectionResult = min(num1 + 1, 0xf) << 4;
            break;
          case 0xb: // theend
            protectionResult = max(num2 - num1, 0) << 4;
            break;
          case 0xa: // theend
            protectionResult = 0x00;
            break;
          case 0xf: // scrambles
            protectionResult = max(num1 - num2, 0) << 4;
            break;
        }
        return;
    }
  }
  else {
    if (Addr >= CPU2_RAM_ADDR && Addr < CPU2_RAM_ADDR + CPU2_RAM_SIZE) {
      memory[CPU2_RAM_OFFSET + Addr - CPU2_RAM_ADDR] = Value;
      return;
    }
    // this is memory mapped sound filters, not RAM
    if (Addr >= CPU2_FILTER_ADDR && Addr < CPU2_FILTER_ADDR + CPU2_FILTER_SIZE) {
      return;
    }
  }
}

unsigned char scramble::inZ80(unsigned short Port) {
  static const unsigned char _timer[10] = {
    0x00, 0x10, 0x20, 0x30, 0x40, 0x90, 0xa0, 0xb0, 0xa0, 0xd0
  };

  // AY2_PORTB - timer
  switch (Port & 0xff) {
    case 0x20: //AY1_DATA
      if (ay_port < 14)
        return soundregs[0x00 + ay_port];
      break;
    case 0x80: //AY2_DATA
      if (ay_port < 14)
        return soundregs[0x10 + ay_port];
      // AY2_PORTA - sound_latch
      if (ay_port == 14)
        return sound_latch;
      // Port B = timer: LS90 bi-quinary counter, divide-by-5120
      if (ay_port == 15) {
        // MAME: scramble_timer[(total_cycles / 512) % 10]
        return _timer[(snd_icnt / 40) % 10];
      }
      break;
  }
  return 0x00;
}

void scramble::outZ80(unsigned short Port, unsigned char Value) {
  switch (Port & 0xff) {
    case 0x10: //AY1_ADDR
      ay_port = Value & 0x0f;
      return;
    case 0x20: //AY1_DATA
      if (ay_port < 14)
        soundregs[0x00 + ay_port] = Value;
      return;
    case 0x40: //AY2_ADDR
      ay_port = Value & 0x0f;
      return;
    case 0x80: //AY2_DATA
      if (ay_port < 14)
        soundregs[0x10 + ay_port] = Value;
      return;
  }
}

void scramble::run_frame(void) {
  // Main Z80 @ 3.072 MHz; Sound Z80 @ 1.789 MHz + 2x AY-3-8910
  for(int i= 0; i < INST_PER_FRAME; i++) {
    current_cpu=0; StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);
    current_cpu=1; StepZ80(&cpu[1]); snd_icnt++; StepZ80(&cpu[1]); snd_icnt++;

    // "latch" IRQ: only deliver when sound CPU has interrupts enabled (EI)
    // Same pattern as Frogger — prevents lost IRQs during DI periods
    if((snd_irq_state & 0x08) && (cpu[1].IFF & IFF_1)) {
      IntZ80(&cpu[1], INT_RST38);
      snd_irq_state = 0; //&= ~0x08;
    }
  }

  if(irq_enable[0]) {
    current_cpu = 0;
    IntZ80(&cpu[0], INT_NMI);
  }
}


void scramble::prepare_frame(void) {
  active_sprites = 0;

  // Sprite data at sprite_ram (HW 0x5040), 4 bytes per sprite
  // base[0]=Y  base[1]=code|flipx|flipy  base[2]=color  base[3]=X
  for(int idx = 7; idx >= 0 && active_sprites < 128; idx--) {
    unsigned char *base = memory + CPU1_SPRITE_OFFSET + idx * 4;

    struct sprite_S spr;
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

  // Bullet data at bullet_ram (HW 0x5060), 4 bytes per bullet
  // MAME format: base[1]=Y complement, base[3]=X position
  // Bullet visible at MAME scanline = 255-base[1], MAME x = 255-base[3]
  // 90° rotation: MAME scanline → galagino X, MAME x → galagino Y
  // Indices 0-6 = enemy shells (white), index 7 = player missile (yellow)
  bullet_active = 0;
  for(int idx = 0; idx < 8; idx++) {
    unsigned char *bbase = memory + CPU1_BULLET_OFFSET + 4 * idx;
    // galagino X = must match tile scroll direction (ship uses scroll registers)
    // The scroll shifts tiles RIGHT with increasing value.
    // bbase[1] encodes the bullet's scanline position in the same direction as scroll.
    bullet_x[idx] = bbase[1] - 16;
    // galagino Y = MAME X position: (255 - bbase[3]) + 15 - 4 (bullet width adjust)
    bullet_y[idx] = 264 - bbase[3];
    if(bbase[1] && bbase[3] && bullet_x[idx] >= 0 && bullet_x[idx] < 224 &&
       bullet_y[idx] > -4 && bullet_y[idx] < 288)
      bullet_active |= (1 << idx);
  }
}

void scramble::blit_tile(short row, char col) {
  if((row < 2) || (row >= 34))
    return;

  unsigned short addr = tileaddr[row][col];
  const unsigned short *tile = scramble_tilemap[memory[CPU1_VRAM_OFFSET + addr]];
  int c = memory[CPU1_ATTR_OFFSET + 2 * (addr & 31) + 1] & 7;
  const unsigned short *colors = scramble_colormap[c];

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
void scramble::blit_tile_scroll(short row, signed char col, unsigned char scroll) {
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

  const unsigned short *tile = scramble_tilemap[memory[CPU1_VRAM_OFFSET + addr]];
  int c = memory[CPU1_ATTR_OFFSET + 2 * (addr & 31) + 1] & 7;
  const unsigned short *colors = scramble_colormap[c];
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

void scramble::blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = scramble_spritemap[sprite[s].flags & 3][sprite[s].code];
  const unsigned short *colors = scramble_colormap[sprite[s].color];

  unsigned long mask = 0xffffffff;
  if(sprite[s].x < 0)        mask <<= -2 * sprite[s].x;
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

void scramble::render_row(short row) {
  if(row <= 1 || row >= 34) return;

  // blue background
  if (background_enable)
    memset(frame_buffer, 8, 2 * 224 * 8);

  if(stars_enabled) {
    if (row == 2) {
      stars_frame_counter++;
      
      if ((stars_frame_counter % 60) == 0) {
        stars_index = 2 + ((stars_frame_counter / 60) % 4);
      }
    }

    int row_top = 8 * row;
    int row_bot = row_top + 8;
    for(int i = 0; i < star_count; i++) {
      if (stars[i].y >= row_top && stars[i].y < row_bot) {
        if (stars[i].x >= 0 &&  stars[i].x < 224 && (i % stars_index) == 0) {
          int fb_idx = (stars[i].y - row_top) * 224 + stars[i].x;
          frame_buffer[fb_idx] = stars[i].color;
        }
      }
    }
  }

  // Read scroll register for this portrait row (per-column scroll in MAME terms)
  // ObjRAM even bytes at 0x5000+2*col → attribute_ram[2*(row-2)]
  unsigned char scroll = memory[CPU1_ATTR_OFFSET + 2 * (row - 2)];

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

  // Draw bullets as single yellow pixels (player missile is a single dot,
  // and the bullet-slot index can vary, so don't special-case index 7)
  if(bullet_active) {
    short row_top = 8 * row;
    short row_bot = row_top + 8;
    for(int b = 0; b < 8; b++) {
      if(!(bullet_active & (1 << b))) continue;
      short bx = bullet_x[b];
      short by = bullet_y[b];
      if(bx < 0 || bx >= 224) continue;
      if(by < row_top || by >= row_bot) continue;
      frame_buffer[(by - row_top) * 224 + bx] = 0xE0FF;  // yellow (byte-swapped)
    }
  }
}

void scramble::stars_init(void) {
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
  for(int i = 0; i < 131071 && star_count < SCRAMBLE_MAX_STARS; i++) {
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
}

// Convert 8-bit R,G,B to RGB565 byte-swapped for ESP32 SPI
inline unsigned short scramble::rgb_to_swapped565(unsigned char r, unsigned char g, unsigned char b) {
  unsigned short c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  return (c >> 8) | (c << 8);  // byte-swap
}

const unsigned short *scramble::logo(void) {
  return scramble_logo;
}