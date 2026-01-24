#ifndef GALAGA_H
#define GALAGA_H

#include "galaga_rom1.h"
#include "galaga_rom2.h"
#include "galaga_rom3.h"
#include "galaga_dipswitches.h"
#include "galaga_logo.h"
#include "galaga_spritemap.h"
#include "galaga_tilemap.h"
#include "galaga_cmap_tiles.h"
#include "galaga_cmap_sprites.h"
#include "galaga_wavetable.h"
#include "galaga_sample_boom.h"
#include "galaga_starseed.h"
#include "../tileaddr.h"
#include "../machineBase.h"

class galaga : public machineBase
{
public:
	galaga() { }
	~galaga() { }

	signed char machineType() override { return MCH_GALAGA; } 
	
	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;
	
	void run_frame(void) override;
	void prepare_frame(void) override;
	void render_row(short row) override;

	const signed char *waveRom(unsigned char value) override;
	const unsigned short *logo(void) override;
	bool hasNamcoAudio() override { return true; }

	// the ship explosion sound is stored as a digi sample.
	// All other sounds are generated on the fly via the
	// original wave tables
	unsigned short snd_boom_cnt = 0;
	const signed char *snd_boom_ptr = NULL;
#ifdef LED_PIN
	void menuLeds(CRGB *leds) override;
	void gameLeds(CRGB *leds) override;
#endif

protected:	
	void blit_tile(short row, char col) override;
	void blit_sprite(short row, unsigned char s) override;
private:
	void render_stars_set(short row, const struct galaga_star *set);
	void trigger_sound_explosion(void);
	void check_galaga_sprite(struct sprite_S *spr);

    unsigned char led_state = 0;       // state set by game (usually video driver)
#ifdef LED_PIN
	const CRGB menu_leds[7] = { LED_RED, LED_BLUE, LED_WHITE, LED_WHITE, LED_WHITE, LED_BLUE, LED_RED };
#endif
	unsigned char stars_scroll_y = 0;
	unsigned char credit = 0;
	char credit_mode = 0;
	int namco_cnt = 0;
	int namco_busy = 0;
	unsigned char cs_ctrl = 0;
	int nmi_cnt = 0;
	int coincredMode = 0;   
	unsigned char starcontrol = 0;
	char sub_cpu_reset = 1;
};

#endif