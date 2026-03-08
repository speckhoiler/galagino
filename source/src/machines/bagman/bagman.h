#ifndef BAGMAN_H
#define BAGMAN_H

#include "bagman_rom.h"
#include "bagman_dipswitches.h"
#include "bagman_logo.h"
#include "bagman_tilemap.h"
#include "bagman_spritemap.h"
#include "bagman_cmap.h"

#include "../tileaddr.h"
#include "../machineBase.h"

class bagman : public machineBase
{
public:
	bagman() { }
	~bagman() { }

	signed char machineType() override { return MCH_BAGMAN; } 
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
	virtual const unsigned short *tileRom(unsigned short addr);
	virtual const unsigned short *colorRom(unsigned short addr);
	virtual const unsigned long *spriteRom(unsigned char flags, unsigned char code);
	
private:
	void pitch_w(uint8_t data);
	unsigned char gfxbank;
#ifdef LED_PIN
	const CRGB menu_leds[7] = { LED_RED, LED_GREEN, LED_YELLOW, LED_YELLOW, LED_YELLOW, LED_GREEN, LED_RED };
#endif

};

#endif