#ifndef DIGDUG_H
#define DIGDUG_H

#include "digdug_rom1.h"
#include "digdug_rom2.h"
#include "digdug_rom3.h"
#include "digdug_dipswitches.h"
#include "digdug_logo.h"
#include "digdug_spritemap.h"
#include "digdug_tilemap.h"
#include "digdug_pftiles.h"
#include "digdug_cmap_tiles.h"
#include "digdug_cmap_sprites.h"
#include "digdug_cmap.h"
#include "digdug_playfield.h"
#include "digdug_wavetable.h"
#include "..\tileaddr.h"
#include "..\machineBase.h"

#define NAMCO_NMI_DELAY  30  // 10 results in errors

class digdug : public machineBase
{
public:
	digdug() { }
	~digdug() { }

	void reset() override;
	signed char machineType() override { return MCH_DIGDUG; } 

	//Z80 calls as inline for more speed
	inline unsigned char rdZ80(unsigned short Addr) override;
	inline void wrZ80(unsigned short Addr, unsigned char Value) override;
	inline unsigned char opZ80(unsigned short Addr) override;

	void run_frame(void) override;
	void prepare_frame(void) override;
	void render_row(short row) override;
	
	const signed char *waveRom(unsigned char value) override;
	const unsigned short *logo(void) override;
	bool hasNamcoAudio() override { return true; }
#ifdef LED_PIN
	void menuLeds(CRGB *leds) override;
	void gameLeds(CRGB *leds) override;
#endif
protected:
	void blit_tile(short row, char col) override;
	void blit_sprite(short row, unsigned char s) override;

private:
	//namco as inline for more speed
	inline unsigned char namco_read(unsigned short Addr);
	inline void namco_write(unsigned short Addr, unsigned char Value);
#ifdef LED_PIN
	const CRGB menu_leds[7] = { LED_WHITE, LED_BLUE, LED_RED, LED_RED, LED_RED, LED_BLUE, LED_WHITE };
#endif
	unsigned char namco_command = 0;
	unsigned char namco_mode = 0;
	unsigned char namco_nmi_counter = 0;
	unsigned char namco_credit = 0x00;
	unsigned char digdug_video_latch;
	char sub_cpu_reset = 1;
};

#endif