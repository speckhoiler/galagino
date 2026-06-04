#ifndef MOONCRESTA_H
#define MOONCRESTA_H

#include "mooncresta_logo.h"
#include "mooncresta_rom.h"
#include "mooncresta_tilemap.h"
#include "mooncresta_spritemap.h"
#include "mooncresta_cmap.h"
#include "mooncresta_dipswitches.h"
#include "../tileaddr.h"
#include "../machineBase.h"

// ============================================================
// Moon Cresta (Nichibutsu 1980) memory map:
//   Main CPU (Z80 @ 3.072 MHz):                       Galaxian ADDR
//     0x0000-0x3fff: ROM (16KB: ...)                  (was 0x0000) 0000 0000 0000 0000 - 0011 1111 1111 1111
//     0x8000-0x83ff: Work   RAM (1KB)  mirror(0x0400) (was 0x4000) 1000 0000 0000 0000 - 1000 0011 1111 1111
//     0x9000-0x93ff: Video  RAM (1KB)  mirror(0x0400) (was 0x5000) 1001 0000 0000 0000 - 1001 0011 1111 1111
//     0x9800-0x98ff: Sprite RAM (256B) mirror(0x0700) (was 0x5800) 1001 1000 0000 0000 - 1001 1000 1111 1111
//     0xa000-0xa000: IN0                              (was 0x6000) 1010 0000 0000 0000 - 1010 0000 0000 0000
//     0xa800-0xa800: IN1                              (was 0x6800) 
//     0xb000-0xb000: IN2                              (was 0x7000)
//     0xb000-0xb000: irq_enable_w                     (was 0x7001)
//     0xb004-0xb004: galaxian_stars_enable_w          (was 0x7004)
//     0xb006-0xb006: galaxian_flip_screen_x_w         (was 0x7006)
//     0xb007-0xb007: galaxian_flip_screen_y_w         (was 0x7007)
//     0xb800-0xb800: watchdog_timer_device::reset_r
//
//     0xa000-0xa002: galaxian_gfxbank_w mirror(0x07f8)     1010 0000 0000 0000 - 1010 0000 0000 0010
//     0xa003-0xa003: coin_count_0_w     mirror(0x07f8)     1010 0000 0000 0011 - 1010 0000 0000 0011
//     0xa004-0xa007: lfo_freq_w         mirror(0x07f8)     1010 0000 0000 0100 - 1010 0000 0000 0111
//     0xa800-0xa807: sound_w            mirror(0x07f8)     1010 1000 0000 0000 - 1010 1000 0000 0111
//     0xb000-0xb000: GalIrqFire         mirror(0x07f8)     1011 0000 0000 0000 - 1011 0000 0000 0000
//     0xb004-0xb004: GalStarsEnable
//     0xb006-0xb006: GalFlipScreenX
//     0xb007-0xb007: GalFlipScreenY
//     0xb800-0xb800: pitch_w            mirror(0x07ff)     1011 1000 0000 0000 - 1011 1000 0000 0000
//
//     0xa000-0xa000: bit 0 - gfx_bank_bit0
//     0xa001-0xa001: bit 0 - gfx_bank_bit1
//     0xa002-0xa002: bit 0 - gfx_bank_enable
//
//     mirror(0x0400) -> mask 0xfbff              0xf800 -> 1111 1000 0000 0000
//     mirror(0x0700) -> mask 0xf8ff
//     mirror(0x07f8) -> mask 0xf807
//     mirror(0x07ff) -> mask 0xf800
// ============================================================
//

// Memory layout in our buffer:
// 0x0000-0x03ff - Work   RAM from 0x8000 (addr - MC_BASE_WORKRAM  + MC_OFF_WORKRAM)
// 0x0400-0x07ff - Video  RAM from 0x9000 (addr - MC_BASE_VIDEORAM + MC_OFF_VIDEORAM)
// 0x0800-0x0bff - Sprite RAM from 0x9800 (addr - MC_OFF_SPRITERAM + MC_OFF_SPRITERAM)

#define MC_BASE_WORKRAM   0x8000 // was 0x4000
#define MC_BASE_VIDEORAM  0x9000 // was 0x5000
#define MC_BASE_SPRITERAM 0x9800 // was 0x5800

#define MC_OFF_WORKRAM   0x0000
#define MC_OFF_VIDEORAM  0x0800
#define MC_OFF_SPRITERAM 0x0c00

#define MC_IN0 0xa000
#define MC_IN1 0xa800
#define MC_IN2 0xb000

// Starfield: max number of visible stars (LFSR generates ~252)
#define GAL_MAX_STARS 256

class mooncresta : public machineBase
{
public:
  mooncresta() {}
  ~mooncresta() {}

  signed char machineType() override { return MCH_MOONCRESTA; }
  void start() override;
  unsigned char opZ80(unsigned short Addr) override;
  unsigned char rdZ80(unsigned short Addr) override;
  void wrZ80(unsigned short Addr, unsigned char Value) override;
  void run_frame(void) override;
  void prepare_frame(void) override;
  void render_row(short row) override;
  const unsigned short *logo(void) override;

protected:
  void blit_tile(short row, char col) override;
  void blit_sprite(short row, unsigned char s) override;
  void blit_tile_scroll(short row, signed char col, unsigned char scroll);
  
private:
  // Bullet rendering (8 bullets: 7 enemy shells + 1 player missile)
  short bullet_x[8], bullet_y[8];
  unsigned char bullet_active;  // bitmask of active bullets

  // Starfield
  void stars_init(void);
  unsigned short rgb_to_swapped565(unsigned char r, unsigned char g, unsigned char b);

  struct star_entry {
    unsigned char x;       // 0-255 horizontal position (landscape)
    unsigned char y;       // 0-255 vertical position (landscape)
    unsigned short color;  // RGB565 byte-swapped
  };
  star_entry stars[GAL_MAX_STARS];

  int star_count = 0;
  int star_scroll_offset = 0;   // scrolls +1 each frame
  unsigned char stars_enabled = 0;
  unsigned char stars_toggle = 0;
 
  uint8_t gfx_bank[4] = {0x00,0x00,0x00,0x00};
  uint8_t gfx_scroll;
};
#endif