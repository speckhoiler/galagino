#ifndef LIZWIZ_H
#define LIZWIZ_H

#include "lizwiz_rom.h"
#include "lizwiz_dipswitches.h"
#include "lizwiz_logo.h"
#include "lizwiz_tilemap.h"
#include "lizwiz_spritemap.h"
#include "lizwiz_cmap.h"
#include "lizwiz_wavetable.h"
#include "../tileaddr.h"
#include "../pacman/pacman.h"

class lizwiz : public pacman
{
public:
	lizwiz() { }
	~lizwiz() { }

	signed char machineType() override { return MCH_LIZWIZ; } 

#ifdef LED_PIN
	void menuLeds(CRGB *leds) override;
	void gameLeds(CRGB *leds) override;
#endif 
	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	void outZ80(unsigned short Port, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;

	void run_frame(void) override;
	const signed char *waveRom(unsigned char value) override;
	const unsigned short *logo(void) override;	

protected:
	const unsigned short *tileRom(unsigned short addr) override;
	const unsigned short *colorRom(unsigned short addr) override;
	const unsigned long *spriteRom(unsigned char flags, unsigned char code) override;

private:
#ifdef LED_PIN
	const CRGB menu_leds[7] = { LED_GREEN, LED_MAGENTA, LED_GREEN, LED_WHITE, LED_GREEN, LED_MAGENTA, LED_GREEN };
#endif
};

#endif