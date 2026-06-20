#ifndef SUPERCOBRA_H
#define SUPERCOBRA_H

#include "supercobra_logo.h"
#include "supercobra_main_rom.h"
#include "supercobra_audio_rom.h"
#include "supercobra_spritemap.h"
#include "supercobra_tilemap.h"
#include "supercobra_cmap.h"
#include "supercobra_dipswitches.h"
#include "../tileaddr.h"
#include "../machineBase.h"
#include "../scramble/scramble.h"

// ============================================================
// Super Cobra (Konami 1981) memory map:
// Main CPU (Z80 @ 3.072 MHz):
//   0x0000-0x2fff: ROM (24KiB)      (0x6000)
//   0x8000-0x87ff: RAM (2KiB)       (0x0800)  0x4000 -> 0x8000
//   0x8800-0x8bff: Video RAM (1KiB) (0x0400)  0x4800 -> 0x8800
//   0x8c00-0x8fff: Video RAM mirror (0x0400)  0x4c00 -> 0x8c00
//   0x9000-0x903f: Attributes RAM   (0x0040)  0x5000 -> 0x9000 
//   0x9040-0x905f: Sprite RAM       (0x0020)
//   0x9060-0x907f: Bullet RAM       (0x0020)
//   0x9080-0x90ff: RAM              (0x0080)
//   0xa801-0xa801: NMI           galaxold_nmi_enable_w       6801 -> a801
//   0xa802-0xa802: Coin Counter  galaxold_coin_counter_w     6802 -> a802
//   0xa803-0xa803: Background    GalBackgroundEnable = d & 1;
//   0xa804-0xa804: Stars Enable  galaxold_stars_enable_w     6804 -> a804 
//   0xa806-0xa806: Flip Screen X galaxold_flip_screen_x_w    
//   0xa807-0xa807: Flip Screen Y galaxold_flip_screen_y_w
//   0xb000-0xb000: Watchdog      reset_r                     7000 -> b000
//   0x9800-0x9803: PPI0          read/write                0x8100 -> 0x9800
//   0xa000-0xa003: PPI1          read/write                0x8200 -> 0xa000
//
// Audio CPU (Z80 @ 1.78975 MHz):
//   0x0000-0x2fff: ROM (12KiB) - super cobra only uses 3 x 2Kib = 6KiB
//   0x8000-0x8fff: Sound RAM (4Kib)
//   0x9000-0x9fff: Audio filters - not used by super cobra
//   
// IO: mask 0x00ff
//   0x0010-0x0010: AY8910 1 address_w
//   0x0020-0x0020: AY8910 1 data_r/data_w
//   0x0040-0x0040: AY8910 2 address_w
//   0x0080-0x0080: AY8910 2 data_r/data_w
//
// ============================================================

class supercobra : public scramble
{
public:
  supercobra() {}
  ~supercobra() {}

  signed char machineType() override { return MCH_SUPERCOBRA; }
 
  unsigned char opZ80(unsigned short Addr) override;
  unsigned char rdZ80(unsigned short Addr) override;
  void wrZ80(unsigned short Addr, unsigned char Value) override;
  void outZ80(unsigned short Port, unsigned char Value) override;

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

private:
  static constexpr unsigned short CPU1_ROM_SIZE = 0x6000;
  static constexpr unsigned short CPU2_ROM_SIZE = 0x2000;

  static constexpr unsigned short CPU1_RAM_ADDR    = 0x8000; //0x4000 + 0x4000;
  static constexpr unsigned short CPU1_VRAM1_ADDR  = 0x8800; //0x4000 + 0x4800;
  static constexpr unsigned short CPU1_VRAM2_ADDR  = 0x8c00; //0x4000 + 0x4c00;
  static constexpr unsigned short CPU1_ATTR_ADDR   = 0x9000; //0x4000 + 0x5000;
  static constexpr unsigned short CPU1_SPRITE_ADDR = 0x9040; //0x4000 + 0x5040;
  static constexpr unsigned short CPU1_BULLET_ADDR = 0x9060; //0x4000 + 0x5060;
  static constexpr unsigned short CPU1_RAM2_ADDR   = 0x9080; //0x4000 + 0x5080;
  static constexpr unsigned short CPU1_OBJRAM_ADDR = 0x9000; //CPU1_ATTR_ADDR;

  static constexpr unsigned short CPU1_RAM_SIZE    = 0x0800;
  static constexpr unsigned short CPU1_VRAM1_SIZE  = 0x0400;
  static constexpr unsigned short CPU1_VRAM2_SIZE  = 0x0400;
  static constexpr unsigned short CPU1_ATTR_SIZE   = 0x0040;
  static constexpr unsigned short CPU1_SPRITE_SIZE = 0x0020;
  static constexpr unsigned short CPU1_BULLET_SIZE = 0x0020;
  static constexpr unsigned short CPU1_RAM2_SIZE   = 0x0080;
  static constexpr unsigned short CPU1_OBJRAM_SIZE = CPU1_ATTR_SIZE + CPU1_SPRITE_SIZE + CPU1_BULLET_SIZE + CPU1_RAM2_SIZE;
  static_assert(CPU1_OBJRAM_SIZE == 0x0100, "CPU1_OBJRAM_SIZE is wrong");

  static constexpr unsigned short CPU1_RAM_OFFSET    = 0x0000;
  static constexpr unsigned short CPU1_VRAM1_OFFSET  = CPU1_RAM_OFFSET    + CPU1_RAM_SIZE;
  static constexpr unsigned short CPU1_VRAM2_OFFSET  = CPU1_VRAM1_OFFSET  + CPU1_VRAM1_SIZE;
  static constexpr unsigned short CPU1_ATTR_OFFSET   = CPU1_VRAM2_OFFSET  + CPU1_VRAM2_SIZE;
  static constexpr unsigned short CPU1_SPRITE_OFFSET = CPU1_ATTR_OFFSET   + CPU1_ATTR_SIZE;
  static constexpr unsigned short CPU1_BULLET_OFFSET = CPU1_SPRITE_OFFSET + CPU1_SPRITE_SIZE;
  static constexpr unsigned short CPU1_RAM2_OFFSET   = CPU1_BULLET_OFFSET + CPU1_BULLET_SIZE;
  static constexpr unsigned short CPU1_MEM_FREE      = CPU1_RAM2_OFFSET   + CPU1_RAM2_SIZE;
  static constexpr unsigned short CPU1_OBJRAM_OFFSET = CPU1_ATTR_OFFSET;

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
