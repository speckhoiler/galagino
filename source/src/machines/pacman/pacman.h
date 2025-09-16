#ifndef PACMAN_H
#define PACMAN_H

#include "pacman_dipswitches.h"
#include "pacman_logo.h"
#include "pacman_rom.h"
#include "pacman_tilemap.h"
#include "pacman_spritemap.h"
#include "pacman_cmap.h"
#include "pacman_wavetable.h"
#include "..\tileaddr.h"
#include "..\machineBase.h"

class pacman : public machineBase
{
public:
	pacman() { }
	~pacman() { }

	signed char machineType() override { return MCH_PACMAN; } 
	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	void outZ80(unsigned short Port, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;

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
	virtual const unsigned short *tileRom(unsigned short addr);
	virtual const unsigned short *colorRom(unsigned short addr);
	virtual const unsigned long *spriteRom(unsigned char flags, unsigned char code);

private:
#ifdef LED_PIN
	const CRGB menu_leds[7] = { LED_BLUE, LED_BLACK, LED_YELLOW, LED_YELLOW, LED_YELLOW, LED_BLACK, LED_BLUE };
#endif
};

#endif