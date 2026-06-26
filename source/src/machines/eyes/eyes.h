#ifndef EYES_H
#define EYES_H

#include "eyes_dipswitches.h"
#include "eyes_logo.h"
#include "eyes_rom.h"
#include "eyes_tilemap.h"
#include "eyes_spritemap.h"
#include "eyes_cmap.h"
#include "eyes_wavetable.h"
#include "../tileaddr.h"
#include "../pacman/pacman.h"

class eyes : public pacman
{
public:
	eyes() { }
	~eyes() { }

	signed char machineType() override { return MCH_EYES; } 

#ifdef LED_PIN
	void menuLeds(CRGB *leds) override { memcpy(leds, menu_leds, NUM_LEDS * sizeof(CRGB)); }
	void gameLeds(CRGB *leds) override {
		static char sub_cnt = 0;
		if(sub_cnt++ == 12) {
			sub_cnt = 0;
			static char bullet_pos = 1;
			leds[0] = LED_YELLOW;
			leds[NUM_LEDS - 1] = LED_YELLOW;
			for(char c = 1; c < NUM_LEDS - 1; c++) {
				if(c == bullet_pos || c == NUM_LEDS - 1 - bullet_pos) leds[c] = LED_BLUE;
				else                                                  leds[c] = LED_BLACK;
			}
			bullet_pos++;
			if(bullet_pos == NUM_LEDS / 2 + 1) bullet_pos = 1;
		}
	}
#endif 
	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	void outZ80(unsigned short Port, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;

	void run_frame(void) override;
	const signed char * waveRom(unsigned char value) override;
	const unsigned short *logo(void) override;	
	
protected:
	const unsigned short *tileRom(unsigned short addr) override;
	const unsigned short *colorRom(unsigned short addr) override;
	const unsigned long *spriteRom(unsigned char flags, unsigned char code) override;

private:
#ifdef LED_PIN
	const CRGB menu_leds[7] = { LED_YELLOW, LED_RED, LED_BLUE, LED_BLACK, LED_BLUE, LED_RED, LED_YELLOW };
#endif
};

#endif