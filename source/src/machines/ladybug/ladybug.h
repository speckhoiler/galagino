#ifndef LADYBUG_H
#define LADYBUG_H

#include "ladybug_rom.h"
#include "ladybug_dipswitches.h"
#include "ladybug_logo.h"
#include "ladybug_tilemap.h"
#include "ladybug_spritemap.h"
#include "ladybug_cmap.h"
#include "../machineBase.h"
// NOTE: Lady Bug does NOT use shared tileaddr.h - has custom tilemap scan

// ============================================================================
// Lady Bug (Universal, 1981) - Machine Engine for galagino_arduino_BT
//
// Ported from oraQuadra Nano arcade emulator.
// Single Z80 @ 4MHz, 2x SN76489 sound, polled vblank, coin NMI.
//
// Hardware:
//   CPU: Z80A @ 4MHz (single CPU, no audio CPU)
//   ROM: 24KB (0x0000-0x5FFF)
//   RAM: 4KB work (0x6000-0x6FFF)
//   Video RAM: 1KB tiles (0xD000-0xD3FF) + 1KB color (0xD400-0xD7FF)
//   Sprite RAM: 1KB (0x7000-0x73FF)
//   Sound: 2x SN76489 at 0xB000 and 0xB001
//   Interrupts: NO vblank IRQ (polled via IN1 bits 6-7), Coin1 -> NMI
//   Display: ROT270, tilemap 36x28 with custom scan
//
// Memory layout in machineBase::memory[] buffer:
//   [0x0000-0x0FFF] <- Z80 0x6000-0x6FFF (work RAM, 4KB)
//   [0x1000-0x13FF] <- Z80 0x7000-0x73FF (sprite RAM, 1KB)
//   [0x1400-0x17FF] <- Z80 0xD000-0xD3FF (video RAM, 1KB)
//   [0x1800-0x1BFF] <- Z80 0xD400-0xD7FF (color RAM, 1KB)
//   [0x1C00-0x23FF] <- Z80 0xD800-0xDFFF (extra RAM, 2KB)
// ============================================================================

#define MEM_WORK_OFF   0x0000  // memory[] offset for work RAM
#define MEM_SPRITE_OFF 0x1000  // memory[] offset for sprite RAM
#define MEM_VIDEO_OFF  0x1400  // memory[] offset for video RAM
#define MEM_COLOR_OFF  0x1800  // memory[] offset for color RAM
#define MEM_EXTRA_OFF  0x1C00  // memory[] offset for extra RAM

class ladybug : public machineBase
{
public:
	ladybug() { }
	~ladybug() { }

	void reset() override;
	signed char machineType() override { return MCH_LADYBUG; }
	signed char videoFlipX() override { return 1; }
	signed char videoFlipY() override { return 1; }
	bool hasNamcoAudio() override { return false; }

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
	void blit_sprite(short row, unsigned char s) override;

private:
	void SN76489_Write_2chip(int chip, unsigned char data);
	int sn_last_register[2] = {0, 0};

	// Video state
	unsigned char flipScreen = 0;

	// Vblank state (toggled in run_frame for polling via IN1 bits 6-7)
	unsigned char vblankActive = 0;

	// Frame counter for game_started detection
	unsigned short startupFrameCount = 0;

	// Coin NMI tracking
	unsigned char coinPrev = 0;
#ifdef LED_PIN
	const CRGB menu_leds[7] = { LED_RED, LED_RED, LED_BLACK, LED_RED, LED_BLACK, LED_RED, LED_RED };
#endif
};

#endif