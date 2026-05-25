#ifndef GALAXIAN_H
#define GALAXIAN_H

#include "galaxian_logo.h"
#include "galaxian_rom.h"
#include "galaxian_dipswitches.h"
#include "galaxian_tilemap.h"
#include "galaxian_spritemap.h"
#include "galaxian_cmap.h"
#include "../tileaddr.h"
#include "../machineBase.h"

// Starfield: max number of visible stars (LFSR generates ~252)
#define GAL_MAX_STARS 256

class galaxian : public machineBase
{
public:
	galaxian() { }
	~galaxian() { }

	signed char machineType() override { return MCH_GALAXIAN; }
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

	void blit_tile_scroll(short row, signed char col, unsigned char scroll);

private:
	// Bullet rendering (8 bullets: 7 enemy shells + 1 player missile)
	short bullet_x[8], bullet_y[8];
	unsigned char bullet_active;  // bitmask of active bullets

	// Starfield
	struct star_entry {
		unsigned char x;       // 0-255 horizontal position (landscape)
		unsigned char y;       // 0-255 vertical position (landscape)
		unsigned short color;  // RGB565 byte-swapped
	};
	star_entry stars[GAL_MAX_STARS];
	int star_count = 0;
	int star_scroll_offset = 0;   // scrolls +1 each frame
	bool stars_enabled = false;
	bool stars_initialized = false;
	void stars_init();

#ifdef LED_PIN
	const CRGB menu_leds[7] = { LED_BLUE, LED_YELLOW, LED_BLUE, LED_YELLOW, LED_BLUE, LED_YELLOW, LED_BLUE };
#endif
};

#endif