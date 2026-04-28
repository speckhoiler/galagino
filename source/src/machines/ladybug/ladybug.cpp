#include "ladybug.h"

void ladybug::reset() {
  machineBase::reset();
  startupFrameCount = 0;
}

unsigned char ladybug::opZ80(unsigned short Addr) {
  if (Addr < 0x6000) return ladybug_rom_cpu1[Addr];
  return rdZ80(Addr);
}

unsigned char ladybug::rdZ80(unsigned short Addr) {
  // ROM: 0x0000-0x5FFF (24KB)
  if (Addr < 0x6000)
    return ladybug_rom_cpu1[Addr];

  // Work RAM: 0x6000-0x6FFF (4KB)
  if ((Addr & 0xF000) == 0x6000)
    return memory[Addr - 0x6000 + MEM_WORK_OFF];

  // Sprite RAM: 0x7000-0x73FF (1KB)
  if (Addr >= 0x7000 && Addr <= 0x73FF)
    return memory[Addr - 0x7000 + MEM_SPRITE_OFF];

  // I/O read: 0x9000-0x9003 (input ports)
  if ((Addr & 0xF000) == 0x9000) {
    unsigned char keymask = input->buttons_get();
    unsigned char coinNow = (keymask & BUTTON_COIN) ? 1 : 0;

    // Coin NMI on rising edge
    if (coinNow && !coinPrev)
      IntZ80(&cpu[0], INT_NMI);
    coinPrev = coinNow;

    switch (Addr & 0x0003) {
      case 0: {
        // IN0: P1 joystick + start (active low)
        // Bits: 0=LEFT 1=DOWN 2=RIGHT 3=UP 4=BUTTON1 5=START1 6=START2 7=TILT
        unsigned char retval = 0xFF;
        if (keymask & BUTTON_LEFT)  retval &= ~0x01;
        if (keymask & BUTTON_DOWN)  retval &= ~0x02;
        if (keymask & BUTTON_RIGHT) retval &= ~0x04;
        if (keymask & BUTTON_UP)    retval &= ~0x08;
        if (keymask & BUTTON_START) retval &= ~0x20;
        return retval;
      }
      case 1: {
        // IN1: P2 joystick + vblank status
        // Bits 0-4: P2 joystick (mirrors P1 in upright, active low)
        // Bit 5: unused (1)
        // Bit 6: VBLANK active low  (0=in vblank, 1=active display)
        // Bit 7: VBLANK active high (1=in vblank, 0=active display)
        unsigned char retval = 0x3F;  // bits 0-5 all high (nothing pressed)
        if (keymask & BUTTON_LEFT)  retval &= ~0x01;
        if (keymask & BUTTON_DOWN)  retval &= ~0x02;
        if (keymask & BUTTON_RIGHT) retval &= ~0x04;
        if (keymask & BUTTON_UP)    retval &= ~0x08;
        retval |= vblankActive ? 0x80 :  0x40;   // bit 7 = 1 during vblank
        return retval;
      }
      case 2:
        return LADYBUG_DSW0;
      case 3:
        return LADYBUG_DSW1;
    }
  }

  // Video RAM: 0xD000-0xD3FF
  if (Addr >= 0xD000 && Addr <= 0xD3FF)
    return memory[Addr - 0xD000 + MEM_VIDEO_OFF];

  // Color RAM: 0xD400-0xD7FF
  if (Addr >= 0xD400 && Addr <= 0xD7FF)
    return memory[Addr - 0xD400 + MEM_COLOR_OFF];

  // Extra RAM: 0xD800-0xDFFF (2KB)
  if (Addr >= 0xD800 && Addr <= 0xDFFF)
    return memory[Addr - 0xD800 + MEM_EXTRA_OFF];

  return 0xFF;
}

void ladybug::wrZ80(unsigned short Addr, unsigned char Value) {
  // Work RAM: 0x6000-0x6FFF
  if ((Addr & 0xF000) == 0x6000) {
    memory[Addr - 0x6000 + MEM_WORK_OFF] = Value;
    return;
  }

  // Sprite RAM: 0x7000-0x73FF
  if (Addr >= 0x7000 && Addr <= 0x73FF) {
    memory[Addr - 0x7000 + MEM_SPRITE_OFF] = Value;
    return;
  }

  // Grid color control: 0x8000 (write-only, not emulated visually)
  if ((Addr & 0xF000) == 0x8000)
    return;

  // Flip screen: 0xA000
  if ((Addr & 0xF000) == 0xA000) {
    flipScreen = Value & 1;
    return;
  }

  // SN76489 sound chips at 0xB000 and 0xB001
  // Write directly to sn_period/sn_volume (Mr. Do! pattern)
  if ((Addr & 0xF000) == 0xB000) {
    int chip = Addr & 1;  // 0xB000 = chip 0, 0xB001 = chip 1
    SN76489_Write_2chip(chip, Value);
    return;
  }

  // Video RAM: 0xD000-0xD3FF
  if (Addr >= 0xD000 && Addr <= 0xD3FF) {
    memory[Addr - 0xD000 + MEM_VIDEO_OFF] = Value;
    if (!game_started && Value != 0 && startupFrameCount > 300)
      game_started = 1;
    return;
  }

  // Color RAM: 0xD400-0xD7FF
  if (Addr >= 0xD400 && Addr <= 0xD7FF) {
    memory[Addr - 0xD400 + MEM_COLOR_OFF] = Value;
    return;
  }

  // Extra RAM: 0xD800-0xDFFF (2KB)
  if (Addr >= 0xD800 && Addr <= 0xDFFF) {
    memory[Addr - 0xD800 + MEM_EXTRA_OFF] = Value;
    return;
  }
}

void ladybug::SN76489_Write_2chip(int chip, unsigned char data) {
  if (data & 0x80) {               // Latch command
    int reg = (data >> 5) & 0x03;  // Channel 0-3
    int type = (data >> 4) & 0x01; // 0=Frequency, 1=Volume
    sn_last_register[chip] = (reg * 2) + type;

    if (reg < 4) {
      if (type == 0) { // Frequency (4 lower bits)
        sn_period[chip][reg] = (sn_period[chip][reg] & 0x3F0) | (data & 0x0F);
      }
      else { // Volume
        unsigned char vol = data & 0x0F;
        sn_volume[chip][reg] = vol;
        if (vol < sn_min_volume[chip][reg])
          sn_min_volume[chip][reg] = vol;
        // Brief sounds: hold active volume for several render cycles
        if (vol < 15)
          sn_hold[chip][reg] = 6;  // ~16ms at 24kHz/64 samples per buffer
      }
    }
  }
  else { // Data write
    int reg = sn_last_register[chip] / 2;
    int type = sn_last_register[chip] % 2;

    if (reg < 4 && type == 0) { // If latch was for frequency
      if (reg == 3) {   // Noise channel
        sn_period[chip][3] = data & 0x07; // 3 bits for noise control
      }
      else { // Tone channels
        sn_period[chip][reg] = (sn_period[chip][reg] & 0x00F) | ((data & 0x3F) << 4);
      }
    }
  }
}

void ladybug::run_frame(void) {
  // Lady Bug polls vblank via IN1 bits 6-7 (no IRQ), coin triggers NMI.
  // Split frame into active (75%) and vblank (25%) phases.
  vblankActive = 0;
  for (int i = 0; i < INST_PER_FRAME; i++) {
    StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);
    if (i == 936)
      vblankActive = 1;
  }

  startupFrameCount++;
}

// ============================================================================
// Prepare frame: extract sprites
//
// MAME Lady Bug sprite RAM layout (from ladybug_video.cpp draw_sprites):
//   64-byte blocks (0x40), skip first 2 and last 2 blocks
//   4-byte entries within each block (up to 16 entries per block):
//     Byte 0: bit7=enable, bit6=big(16x16), bit5=xflip, bit4=yflip, bits3-0=y_fine
//     Byte 1: sprite code low 8 bits
//     Byte 2: bit4=code_high, bits3-0=color
//     Byte 3: X position (native)
//     Y position: (block_offset >> 2) | y_fine
//
// ROT270 coordinate transform (matching galagino convention):
//   display_x = ypos - 24   (native Y maps to display X)
//   display_y = 256 - xpos  (native X maps to inverted display Y)
// ============================================================================
void ladybug::prepare_frame(void) {
  active_sprites = 0;

  for (int offs = 0x400 - 0x80; offs >= 0x80 && active_sprites < 92; offs -= 0x40) {
    // Find end of active entries in this block
    int i = 0;
    while (i < 0x40 && memory[MEM_SPRITE_OFF + offs + i])
      i += 4;

    // Process entries in reverse order within block
    while (i > 0) {
      i -= 4;
      unsigned char *spr_ptr = memory + MEM_SPRITE_OFF + offs + i;

      if (!(spr_ptr[0] & 0x80)) continue; // Not enabled

      bool big = (spr_ptr[0] & 0x40) != 0;
      if (!big) continue; // Only 16x16 sprites supported

      unsigned char nativeFlipX = (spr_ptr[0] >> 5) & 1;
      unsigned char nativeFlipY = (spr_ptr[0] >> 4) & 1;
      unsigned char yFine = spr_ptr[0] & 0x0F;

      unsigned short code = spr_ptr[1] | (((unsigned short)(spr_ptr[2] & 0x10)) << 4);
      unsigned char color = spr_ptr[2] & 0x0F;
      unsigned char xpos = spr_ptr[3];          // native X
      int ypos = (offs >> 2) | yFine;           // native Y

      struct sprite_S spr;

      // ROT270 coordinate transform
      spr.x = ypos - 24;
      spr.y = 16 + (int)xpos;

      spr.code = (code >> 2) & 0x7F;
      spr.color = color & 0x07;

      // Variant index: bit0=nativeFlipX (flip_h), bit1=nativeFlipY (flip_v)
      spr.flags = nativeFlipX | (nativeFlipY << 1);

      // Clip check (224x288 display frame)
      if ((spr.y > -16) && (spr.y < 288) && (spr.x > -16) && (spr.x < 224))
        sprite[active_sprites++] = spr;
    }
  }
}

// ============================================================================
// Render a single 8x8 tile
//
// Lady Bug tilemap: 32x32 (TILEMAP_SCAN_ROWS: addr = native_row*32 + native_col)
// MAME screen: ROT270, visible area native [8..247] x [32..223]
//   -> 30 native cols (1-30) x 24 native rows (4-27)
//   -> after ROT270: 24 display cols x 30 display rows = 192x240 pixels
//
// Centered in 224x288 display frame (16px X pad, 24px Y pad):
//   Valid content: cols 2-25, rows 3-32
//   ROT270 mapping (matching 1942/galagino convention):
//     native_row = display_col + 2   (display left->low native row)
//     native_col = 33 - display_row  (display top->high native col)
//     addr = native_row * 32 + native_col
// ============================================================================
void ladybug::blit_tile(short row, char col) {
  // Centered 192x240 content within 224x288 frame
  if (col < 2 || col > 25 || row < 3 || row > 32) return;

  unsigned short addr = (unsigned short)(col + 2) * 32 + (row - 2);

  unsigned char colorByte = memory[MEM_COLOR_OFF + addr];
  unsigned short tileCode = memory[MEM_VIDEO_OFF + addr];

  // Bit 3 of color RAM extends tile code by 256 (MAME: code + 32*(color & 0x08))
  if (colorByte & 0x08) tileCode += 256;

  unsigned char colorAttr = colorByte & 0x07;

  const unsigned short *tile = ladybug_tilemap[tileCode & 0x1FF];
  const unsigned short *colors = ladybug_colormap[colorAttr];

  // Linear scan - tiles are pre-rotated for display orientation
  unsigned short *ptr = frame_buffer + 8 * col;
  for (char r = 0; r < 8; r++, ptr += (224 - 8)) {
    unsigned short pix = tile[r];
    for (char c = 0; c < 8; c++, pix >>= 2) {
      if (pix & 3) *ptr = colors[pix & 3];
      ptr++;
    }
  }
}

void ladybug::blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = ladybug_sprites[sprite[s].flags & 3][sprite[s].code];
  const unsigned short *colors = ladybug_sprite_colormap[sprite[s].color & 7];

  // Horizontal clip mask
  unsigned long mask = 0xFFFFFFFF;
  if (sprite[s].x < 0)       mask <<= -2 * sprite[s].x;
  if (sprite[s].x > 224 - 16) mask >>= (2 * (sprite[s].x - (224 - 16)));

  short y_offset = sprite[s].y - 8 * row;

  unsigned char lines2draw = 8;
  if (y_offset < -8) lines2draw = 16 + y_offset;

  unsigned short startline = 0;
  if (y_offset > 0) {
    startline = y_offset;
    lines2draw = 8 - y_offset;
  }

  if (y_offset < 0)
    spr -= y_offset;

  unsigned short *ptr = frame_buffer + sprite[s].x + 224 * startline;
  for (char r = 0; r < lines2draw; r++, ptr += (224 - 16)) {
    unsigned long pix = *spr++ & mask;
    for (char c = 0; c < 16; c++, pix >>= 2) {
      if (pix & 3) *ptr = colors[pix & 3];
      ptr++;
    }
  }
}

void ladybug::render_row(short row) {
  // Tiles first (background)
  for (char col = 0; col < 28; col++)
    blit_tile(row, col);

  // Sprites on top
  for (unsigned char s = 0; s < active_sprites; s++) {
    if ((sprite[s].y < 8 * (row + 1)) && ((sprite[s].y + 16) > 8 * row))
      blit_sprite(row, s);
  }
}

const unsigned short *ladybug::logo(void) {
  return ladybug_logo;
}

#ifdef LED_PIN
void ladybug::gameLeds(CRGB *leds) {
  // Lady Bug: alternating red and black (ladybug spots pattern)
  static char sub_cnt = 0;
  if (sub_cnt++ == 32) {
    sub_cnt = 0;

    static char led = 0;
    char il = (led < NUM_LEDS) ? led : ((2 * NUM_LEDS - 2) - led);
    for (char c = 0; c < NUM_LEDS; c++) {
      if (c == il)
        leds[c] = LED_WHITE;
      else
        leds[c] = LED_RED;
    }
    led = (led + 1) % (2 * NUM_LEDS - 2);
  }
}

void ladybug::menuLeds(CRGB *leds) {
  memcpy(leds, menu_leds, NUM_LEDS * sizeof(CRGB));
}
#endif