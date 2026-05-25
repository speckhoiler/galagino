#include "tutankhm.h"

void tutankhm::reset() {
  machineBase::reset();
  m6809_reset(&main_cpu);

  if(videoram) {
    free(videoram);
    videoram = nullptr;
  }   
}

void tutankhm::start() {
  // Allocate 32KB video RAM from PSRAM
  if (!videoram) {
    videoram = (uint8_t*)ps_malloc(32768);
    if (!videoram)
      videoram = (uint8_t*)malloc(32768);
        
    memset(videoram, 0, 32768);
  }

  memset(palette_rgb565, 0, sizeof(palette_rgb565) / 2);
  memset(snd_ram, 0, sizeof(snd_ram));
  irq_toggle = 0;
  irq_enable = 0;
  snd_irq_pending = 0;
  snd_irq_last = 0;
  snd_icnt = 0;
}

unsigned char tutankhm::m6809_read_opcode(m6809_state *s, uint16_t addr) {
  if (addr >= 0xA000)
    return tutankhm_rom[addr - 0xA000];

  return 0xFF;
}

unsigned char tutankhm::m6809_read(m6809_state *s, uint16_t addr) {
  // Video RAM 0x0000-0x7FFF (32KB bitmap)
  if (addr < 0x8000)
    return videoram[addr];

  // Palette RAM 0x8000-0x800F (mirror 0x00F0)
  if (addr >= 0x8000 && addr <= 0x80FF)
    return memory[TUT_PALETTE + (addr & 0x0F)];

  // Scroll register 0x8100 (mirror 0x0F)
  if ((addr & 0xFFF0) == 0x8100)
    return scroll_reg;

  // Watchdog 0x8120 (mirror 0x0F) - return 0xFF
  if ((addr & 0xFFF0) == 0x8120)
    return 0xFF;

  // DSW2 0x8160 (mirror 0x0F)
  if ((addr & 0xFFF0) == 0x8160)
    return TUTANKHM_DSW2 | (input->demoSoundsOff() ? TUTANKHM_DSW2_DEMO_SOUND_OFF : TUTANKHM_DSW2_DEMO_SOUND_ON);

  // IN0 0x8180 (mirror 0x0F) — Coin/Start/Service
  if ((addr & 0xFFF0) == 0x8180) {
    unsigned char keymask = input->buttons_get();
    unsigned char retval = 0xFF;
    if (keymask & BUTTON_COIN)   retval &= ~0x01;  // Coin 1
    if (keymask & BUTTON_START)  retval &= ~0x08;  // Start 1
    return retval;
  }

  // IN1 0x81A0 (mirror 0x0F) — P1 controls
  if ((addr & 0xFFF0) == 0x81A0) {
    unsigned char keymask = input->buttons_get();
    unsigned char retval = 0xFF;
    if (keymask & BUTTON_LEFT)   retval &= ~0x01;  // Move Left
    if (keymask & BUTTON_RIGHT)  retval &= ~0x02;  // Move Right
    if (keymask & BUTTON_UP)     retval &= ~0x04;  // Move Up
    if (keymask & BUTTON_DOWN)   retval &= ~0x08;  // Move Down
    if (keymask & BUTTON_FIRE)   retval &= ~0x10;  // Shoot Left
    if (keymask & BUTTON_FIRE)   retval &= ~0x20;  // Shoot Right
    if (keymask & BUTTON_EXTRA)  retval &= ~0x40;  // Flash Bomb
    return retval;
  }

  // IN2 0x81C0 (mirror 0x0F) — P2 controls (idle)
  if ((addr & 0xFFF0) == 0x81C0)
    return 0xFF;

  // DSW1 0x81E0 (mirror 0x0F)
  if ((addr & 0xFFF0) == 0x81E0)
    return TUTANKHM_DSW1;

  // Work RAM 0x8800-0x8FFF (2KB)
  if (addr >= 0x8800 && addr <= 0x8FFF)
    return memory[TUT_WORKRAM + (addr - 0x8800)];

  // Banked ROM 0x9000-0x9FFF (4KB, selected by write to 0x8300)
  if (addr >= 0x9000 && addr <= 0x9FFF)
    return tutankhm_bank_rom[bank_select * 0x1000 + (addr - 0x9000)];

  // Fixed ROM 0xA000-0xFFFF (24KB: m1+m2+3j+m4+m5+j6)
  if (addr >= 0xA000)
    return tutankhm_rom[addr - 0xA000];

  return 0xFF;
 }
 
void tutankhm::m6809_write(m6809_state *s, uint16_t addr, uint8_t val) {
  // Video RAM 0x0000-0x7FFF (32KB bitmap)
  if (addr < 0x8000) {
    if (!game_started) game_started = 1;
    videoram[addr] = val;
    return;
  }

  // Palette RAM 0x8000-0x800F (mirror 0x00F0)
  if (addr >= 0x8000 && addr <= 0x80FF) {
    unsigned char idx = addr & 0x0F;
    memory[TUT_PALETTE + idx] = val;
    palette_rgb565[idx] = palette_to_rgb565(val);
    return;
  }

  // Scroll register 0x8100 (mirror 0x0F)
  if ((addr & 0xFFF0) == 0x8100) {
    scroll_reg = val;
    return;
  }

  // LS259 latch 0x8200-0x8207 (mirror 0x00F8)
  // Q0=IRQ enable, Q1=payout(nop), Q2=coin2, Q3=coin1,
  // Q4=stars, Q5=audio mute, Q6=flipX, Q7=flipY
  if (addr >= 0x8200 && addr <= 0x82FF) {
    unsigned char bit = (addr & 0x07);
    unsigned char state = val & 1;
    switch (bit) {
      case 0: irq_enable = state; break;  // IRQ enable
      case 1: break;  // Payout (not used)
      case 2: break;  // Coin counter 2
      case 3: break;  // Coin counter 1
      case 4: break;  // Stars enable
      case 5: break;  // Audio mute
      case 6: break;  // Flip screen X (ignored)
      case 7: break;  // Flip screen Y (ignored)
    }
    return;
  }

  // Bank select 0x8300 (mirror 0x00FF)
  if ((addr & 0xFF00) == 0x8300) {
    bank_select = val & 0x0F;
    return;
  }

  // Sound trigger 0x8600 (mirror 0x00FF) — pulse IRQ to sound CPU
  if ((addr & 0xFF00) == 0x8600) {
    unsigned char bit_val = val & 1;
    if(bit_val && !snd_irq_last) snd_irq_pending = 1;
    snd_irq_last = bit_val;
    return;
  }

  // Sound data latch 0x8700 (mirror 0x00FF)
  if ((addr & 0xFF00) == 0x8700) {
    // Some sounds are multiple updated during one vblank. Not possible with gagaino. So, take other sounds...
    if (val == 71) // Game Over
      val = 3;
    else if (val == 74) // Appear //2
      val = 8;
    else if (val == 77) // Die //13
      val = 4;
    else if (val == 76) // Flash //5
      val = 2;

    soundlatch = val;
    return;
  }

  // Work RAM 0x8800-0x8FFF
  if (addr >= 0x8800 && addr <= 0x8FFF) {
    memory[TUT_WORKRAM + (addr - 0x8800)] = val;
    return;
  }
 }
 
unsigned short tutankhm::palette_to_rgb565(uint8_t val) {
  static const uint8_t pal3bit[8] = { 0x00, 0x24, 0x49, 0x6D, 0x92, 0xB6, 0xDB, 0xFF };
  static const uint8_t pal2bit[4] = { 0x00, 0x55, 0xAA, 0xFF };

  uint8_t r8 = pal3bit[val & 7];
  uint8_t g8 = pal3bit[(val >> 3) & 7];
  uint8_t b8 = pal2bit[(val >> 6) & 3];

  uint16_t r5 = r8 >> 3;
  uint16_t g6 = g8 >> 2;
  uint16_t b5 = b8 >> 3;
  uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;

  // Byte-swap for SPI display
  return ((rgb565 >> 8) & 0xFF) | ((rgb565 & 0xFF) << 8);
}

unsigned char tutankhm::opZ80(unsigned short Addr) {
  if (Addr < 0x2000)
    return tutankhm_snd_rom[Addr];

  return 0xFF;
}

unsigned char tutankhm::rdZ80(unsigned short Addr) {
  if (Addr < 0x2000)
    return tutankhm_snd_rom[Addr];
    
  // RAM 0x3000-0x3FFF (1KB mirrored)
  if (Addr >= 0x3000 && Addr <= 0x33FF)
    return snd_ram[Addr & 0x3FF];

  // AY#1 data read (0x4000)
  if((Addr & 0xF000) == 0x4000) {
    unsigned char reg = ay_addr[0] & 0x0F;
    if (reg == 14) {
      return soundlatch;
    }
    if (reg == 15) {
      static const unsigned char timer[10] = {
        0x00, 0x10, 0x20, 0x30, 0x40, 0x90, 0xa0, 0xb0, 0xa0, 0xd0
      };
      return timer[(snd_icnt / 24) % 10];
    }
    return ay_regs[0][reg];
  }

  // AY#2 data read (0x6000)
  if((Addr & 0xF000) == 0x6000) {
    unsigned char reg = ay_addr[1] & 0x0F;
    return ay_regs[1][reg];
  }
  return 0x00;
}

void tutankhm::wrZ80(unsigned short Addr, unsigned char Value) {
  // RAM 0x3000-0x3FFF
  if (Addr >= 0x3000 && Addr <= 0x33FF) {
    snd_ram[Addr & 0x3FF] = Value;
    return;
  }
    
  // AY#1 data write (0x4000). Some sound is multiple updated between 2 vblanks. Not possible with gagaino...
  if((Addr & 0xF000) == 0x4000) {
    unsigned char reg = ay_addr[0] & 0x0F;
    ay_regs[0][reg] = Value;
    if (reg < 14) soundregs[reg] = Value;
    return;
  }

  // AY#1 address write (0x5000)
  if((Addr & 0xF000) == 0x5000) {
    ay_addr[0] = Value & 0x0F;
    return;
  }

  // AY#2 data write (0x6000). Some sounds is multiple updated between 2 vblanks. Not possible with gagaino...
  if((Addr & 0xF000) == 0x6000) {
    unsigned char reg = ay_addr[1] & 0x0F;
    ay_regs[1][reg] = Value;
    if (reg < 14) soundregs[16 + reg] = Value;
    return;
  }

  // AY#2 address write (0x7000)
  if((Addr & 0xF000) == 0x7000) {
    ay_addr[1] = Value & 0x0F;
    return;
  }

  // Filter
  if (Addr >= 0x8000) {
    return;
  }
}

void tutankhm::run_frame(void) {
  // Main CPU: M6809E @ 1.5 MHz = ~25000 cycles/frame at 60Hz
  // Sound CPU: Z80 @ 1.789 MHz
  for (int i = 0; i < INST_PER_FRAME / 2; i++) {
    m6809_step(&main_cpu, 8);

    StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);
    snd_icnt += 2;

    // "latch" IRQ: only deliver when sound CPU has interrupts enabled (EI)
    // Same pattern as Frogger — prevents lost IRQs during DI periods
    if(snd_irq_pending && (cpu[0].IFF & IFF_1)) {
      IntZ80(&cpu[0], INT_RST38);
      snd_irq_pending = 0;
    }
  }

  // VBlank IRQ: fires every OTHER frame (toggle flip-flop, per MAME)
  irq_toggle ^= 1;
  if (irq_toggle && irq_enable) {
    m6809_irq(&main_cpu);
  }
}

void tutankhm::prepare_frame(void) {
  // No sprite extraction needed — Tutankham is purely bitmap-based
}

// ============================================================
// Bitmap rendering with ROT90
// ============================================================
// Tutankham native: 256x256, visible 256x224 (y:16-239), ROT90
// Galagino display: 224 wide x 288 tall (36 rows of 8px)
// Rows 2-33 map to game X 0-255
// Screen X 0-223 maps to game Y 16-239

void tutankhm::render_row(short row) {
  if (row < 2 || row > 33) return;
  if (!videoram) return;

  for (int dy = 0; dy < 8; dy++) {
    int bmp_x = 255 - ((row - 2) * 8 + dy);
    unsigned short *dst = frame_buffer + dy * 224;

    // Pre-compute constants that don't change across sx
    int half_x = bmp_x >> 1;        // bmp_x / 2
    int odd_x = bmp_x & 1;          // nibble select: 0=low, 1=high
    int y_base = bmp_x < 192 ? (16 + scroll_reg) : 16;  // scroll offset

    // Unrolled inner loop: videoram row is at y*128 + half_x
    // bmp_y = (sx + y_base) & 0xFF for each sx

	  const uint8_t *vram_col = videoram + half_x;  // column base

    if (odd_x) {
      // High nibble path
      for (int sx = 0; sx < 224; sx++) {
        int bmp_y = (sx + y_base) & 0xFF;
        dst[sx] = palette_rgb565[vram_col[bmp_y << 7] >> 4];
      }
    } 
    else {
      // Low nibble path
      for (int sx = 0; sx < 224; sx++) {
        int bmp_y = (sx + y_base) & 0xFF;
        dst[sx] = palette_rgb565[vram_col[bmp_y << 7] & 0x0F];
      }
    }
  }
}

const unsigned short *tutankhm::logo(void) {
  return tutankhm_logo;
}

#ifdef LED_PIN
void tutankhm::gameLeds(CRGB *leds) {
  static char sub_cnt = 0;
  if (sub_cnt++ == 32) {
    sub_cnt = 0;
    static char led = 0;
    char il = (led < NUM_LEDS) ? led : ((2 * NUM_LEDS - 2) - led);
    for (char c = 0; c < NUM_LEDS; c++) {
      if (c == il) leds[c] = LED_YELLOW;
      else         leds[c] = CRGB(100, 80, 0);
    }
    led = (led + 1) % (2 * NUM_LEDS - 2);
  }
}

void tutankhm::menuLeds(CRGB *leds) {
  memcpy(leds, menu_leds, NUM_LEDS * sizeof(CRGB));
}
#endif