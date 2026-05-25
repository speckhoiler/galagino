#ifndef TIMEPLT_H
#define TIMEPLT_H

#include "timeplt_logo.h"
#include "timeplt_rom.h"
#include "timeplt_snd_rom.h"
#include "timeplt_dipswitches.h"
#include "timeplt_tilemap.h"
#include "timeplt_spritemap.h"
#include "../tileaddr.h"
#include "../machineBase.h"

// Time Pilot (Konami 1982) memory map:
//   Main CPU (Z80 @ 3.072 MHz):
//     0x0000-0x5FFF: ROM (24KB: tm1+tm2+tm3)
//     0xA000-0xA3FF: Color RAM (1KB)
//     0xA400-0xA7FF: Video RAM (1KB)
//     0xA800-0xAFFF: Work RAM (2KB)
//     0xB000-0xB0FF: Sprite RAM bank 0
//     0xB400-0xB4FF: Sprite RAM bank 1
//     0xC000 read: Scanline counter
//     0xC200 read: DSW2
//     0xC300 read: IN0 (coins, start)
//     0xC320 read: IN1 (P1 joystick + fire)
//     0xC340 read: IN2 (P2 controls)
//     0xC360 read: DSW1
//     0xC000 write: Sound latch
//     0xC200 write: Watchdog
//     0xC300 write: LS259 latch (NMI enable, flip, sound IRQ, video enable)

// Memory layout in our buffer:
//   0x0000-0x03FF: Color RAM (1KB) [from 0xA000]
//   0x0400-0x07FF: Video RAM (1KB) [from 0xA400]
//   0x0800-0x0FFF: Work RAM (2KB) [from 0xA800]
//   0x1000-0x10FF: Sprite RAM bank 0 [from 0xB000]
//   0x1100-0x11FF: Sprite RAM bank 1 [from 0xB400]
//   Total: 0x1200 = 4608 bytes (fits in RAMSIZE 9344)

#define MEM_COLORRAM  0x0000
#define MEM_VIDEORAM  0x0400
#define MEM_WORKRAM   0x0800
#define MEM_SPRITES0  0x1000
#define MEM_SPRITES1  0x1100

class timeplt : public machineBase
{
public:
	timeplt() { }
	~timeplt() { }

	signed char machineType() override { return MCH_TIMEPLT; }
	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;

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
	void blit_tile_cat(short row, char col, signed char cat_filter);
	void blit_sprite(short row, unsigned char s) override;
	void extract_sprites(const unsigned char *bank0, const unsigned char *bank1);

private:
	unsigned char nmi_enable;
	unsigned char video_enable = 0;
	unsigned char flip_screen;
	unsigned char soundlatch;
	unsigned char scanline_counter;
  	unsigned char multiplexUsed;
  	unsigned char multiplexUsedCopy;
  	unsigned char multiplexBank0[0x100];
  	unsigned char multiplexBank1[0x100];

	// Sound CPU state
	Z80 snd_cpu;
	unsigned char snd_ram[1024];
	unsigned char snd_irq_pending = 0;
	unsigned char snd_irq_last = 0;    // previous Q2 state for edge detection
	unsigned long snd_icnt = 0;

	// AY-3-8910 registers
	unsigned char ay_addr[2];
	unsigned char ay_regs[2][16];

#ifdef LED_PIN
	const CRGB menu_leds[7] = { LED_CYAN, LED_WHITE, LED_CYAN, LED_WHITE, LED_CYAN, LED_WHITE, LED_CYAN };
#endif
};

#endif
