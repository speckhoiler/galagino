#ifndef SCRAMBLE_H
#define SCRAMBLE_H

#include "scramble_logo.h"
#include "scramble_main_rom.h"
#include "scramble_audio_rom.h"
#include "scramble_spritemap.h"
#include "scramble_tilemap.h"
#include "scramble_cmap.h"
#include "scramble_dipswitches.h"
#include "../tileaddr.h"
#include "../machineBase.h"

// ============================================================
// Scramble (Konami 1981) memory map:
// Main CPU (Z80 @ 3.072 MHz):
//   0x0000-0x3fff: ROM (16KiB)      (0x4000)
//   0x4000-0x47ff: RAM (2KiB)       (0x0800)
//   0x4800-0x4bff: Video RAM (1KiB) (0x0400)
//   0x4c00-0x4fff: Video RAM mirror (0x0400)
//   0x5000-0x503f: Attributes RAM   (0x0040)
//   0x5040-0x505f: Sprite RAM       (0x0020)
//   0x5060-0x507f: Bullet RAM       (0x0020)
//   0x5080-0x50ff: RAM              (0x0080)
//   0x6801-0x6801: NMI           galaxold_nmi_enable_w
//   0x6802-0x6802: Coin Counter  galaxold_coin_counter_w
//   0x6804-0x6804: Stars Enable  galaxold_stars_enable_w
//   0x6806-0x6806: Flip Screen X galaxold_flip_screen_x_w
//   0x6807-0x6807: Flip Screen Y galaxold_flip_screen_y_w
//   0x7000-0x7000: Watchdog      reset_r
//   0x7800-0x7800: Watchdog      reset_r
//   0x8100-0x8103: PPI0          read/write
//   0x8110-0x8113: PPI0 mirror   read
//   0x8200-0x8203: PPI1          read/write
//
// Audio CPU (Z80 @ 1.78975 MHz):
//   0x0000-0x2fff: ROM (12KiB) - scramble only uses 3 x 2Kib = 6KiB
//   0x8000-0x8fff: Sound RAM (4Kib)
//   0x9000-0x9fff: Audio filters - not used by scramble
//   
// IO: mask 0x00ff
//   0x0010-0x0010: AY8910 1 address_w
//   0x0020-0x0020: AY8910 1 data_r/data_w
//   0x0040-0x0040: AY8910 2 address_w
//   0x0080-0x0080: AY8910 2 data_r/data_w
//
// ============================================================

class scramble : public machineBase
{
public:
  scramble() {}
  ~scramble() {}

  signed char machineType() override { return MCH_SCRAMBLE; }
  void start() override;

  unsigned char opZ80(unsigned short Addr) override;
  unsigned char rdZ80(unsigned short Addr) override;
  unsigned char inZ80(unsigned short Port) override;
  void wrZ80(unsigned short Addr, unsigned char Value) override;
  void outZ80(unsigned short Port, unsigned char Value) override;

  void run_frame(void) override;
  void prepare_frame(void) override;
  void render_row(short row) override;
  const unsigned short *logo(void) override;

#ifdef LED_PIN
  void menuLeds(CRGB *leds) override;
  void gameLeds(CRGB *leds) override;
#endif

protected:
  void blit_tile(short row, char col) override;
  void blit_sprite(short row, unsigned char s) override;
  void blit_tile_scroll(short row, signed char col, unsigned char scroll);

  unsigned char ignoreFireButton;
  unsigned char sound_latch;
  unsigned char ay_port;
  unsigned char snd_irq_state;
  unsigned char snd_irq_last = 0;   
  unsigned long snd_icnt = 0;

  unsigned short protectionState = 0;
  unsigned char protectionResult = 0;

  uint8_t gfx_bank[4] = {0x00,0x00,0x00,0x00};
  uint8_t gfx_scroll;

  // Bullet rendering (8 bullets: 7 enemy shells + 1 player missile)
  short bullet_x[8], bullet_y[8];
  unsigned char bullet_active;  // bitmask of active bullets

  // Starfield
  void stars_init(void);
  static constexpr unsigned short SCRAMBLE_MAX_STARS = 256;
  struct star_entry {
    unsigned char x;       // 0-255 horizontal position (landscape)
    unsigned char y;       // 0-255 vertical position (landscape)
    unsigned short color;  // RGB565 byte-swapped
  };
  star_entry stars[SCRAMBLE_MAX_STARS];

  int star_count = 0;
  int stars_frame_counter = 0;
  int stars_index = 2;
  unsigned char stars_enabled = 0;
  
private:
  unsigned short rgb_to_swapped565(unsigned char r, unsigned char g, unsigned char b);
 
  static constexpr unsigned short CPU1_ROM_SIZE    = 0x4000;
  static constexpr unsigned short CPU2_ROM_SIZE    = 0x2000;

  static constexpr unsigned short CPU1_RAM_ADDR    = 0x4000;
  static constexpr unsigned short CPU1_VRAM_ADDR   = 0x4800;
  static constexpr unsigned short CPU1_ATTR_ADDR   = 0x5000;
  static constexpr unsigned short CPU1_SPRITE_ADDR = 0x5040;
  static constexpr unsigned short CPU1_BULLET_ADDR = 0x5060;
  static constexpr unsigned short CPU1_RAM2_ADDR   = 0x5080;

  static constexpr unsigned short CPU1_RAM_SIZE    = 0x0800;
  static constexpr unsigned short CPU1_VRAM_SIZE   = 0x0400;
  static constexpr unsigned short CPU1_ATTR_SIZE   = 0x0040;
  static constexpr unsigned short CPU1_SPRITE_SIZE = 0x0020;
  static constexpr unsigned short CPU1_BULLET_SIZE = 0x0020;
  static constexpr unsigned short CPU1_RAM2_SIZE   = 0x0080;
  static constexpr unsigned short CPU1_RAM_OFFSET    = 0x0000;
  static constexpr unsigned short CPU1_VRAM_OFFSET   = CPU1_RAM_OFFSET    + CPU1_RAM_SIZE;
  static constexpr unsigned short CPU1_ATTR_OFFSET   = CPU1_VRAM_OFFSET   + CPU1_VRAM_SIZE;
  static constexpr unsigned short CPU1_SPRITE_OFFSET = CPU1_ATTR_OFFSET   + CPU1_ATTR_SIZE;
  static constexpr unsigned short CPU1_BULLET_OFFSET = CPU1_SPRITE_OFFSET + CPU1_SPRITE_SIZE;
  static constexpr unsigned short CPU1_RAM2_OFFSET   = CPU1_BULLET_OFFSET + CPU1_BULLET_SIZE;
  static constexpr unsigned short CPU1_MEM_FREE      = CPU1_RAM2_OFFSET   + CPU1_RAM2_SIZE;

  static constexpr unsigned short CPU2_RAM_ADDR      = 0x8000;
  static constexpr unsigned short CPU2_RAM_SIZE      = 0x1000;
  static constexpr unsigned short CPU2_RAM_OFFSET    = CPU1_MEM_FREE;
  static constexpr unsigned short CPU2_MEM_FREE      = CPU2_RAM_OFFSET    + CPU2_RAM_SIZE;
  static constexpr unsigned short CPU2_FILTER_ADDR   = 0x9000; // hardware sound filter, not RAM
  static constexpr unsigned short CPU2_FILTER_SIZE   = 0x1000;
  static_assert(CPU2_MEM_FREE <= RAMSIZE, "RAMSIZE is too low");

#ifdef LED_PIN
  const CRGB menu_leds[7] = { LED_YELLOW, LED_BLUE, LED_GREEN, LED_WHITE, LED_GREEN, LED_BLUE, LED_YELLOW };
#endif
};
#endif