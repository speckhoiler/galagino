#ifndef crush_H
#define crush_H

#include "crush_dipswitches.h"
#include "crush_logo.h"
#include "crush_rom.h"
#include "crush_tilemap.h"
#include "crush_spritemap.h"
#include "crush_cmap.h"
#include "crush_wavetable.h"
#include "../tileaddr.h"
#include "../pacman/pacman.h"

class crush : public pacman
{
public:
	crush() { }
	~crush() { }

	signed char machineType() override { return MCH_CRUSH; } 

#ifdef LED_PIN
	void menuLeds(CRGB *leds) override { memcpy(leds, menu_leds, NUM_LEDS * sizeof(CRGB)); }
	void gameLeds(CRGB *leds) override {
		static char sub_cnt = 0;
		if(sub_cnt++ == 16) {
			sub_cnt = 0;
			static char pos = 0;
			static char color_offset = 0;
			const CRGB paint_colors[6] = { LED_RED, LED_YELLOW, LED_GREEN, LED_CYAN, LED_BLUE, LED_MAGENTA };
			for(char c = 0; c < NUM_LEDS; c++) {
				if(c == pos)  leds[c] = LED_WHITE;
				else if(c < pos) leds[c] = paint_colors[(c + color_offset) % 6];
				else             leds[c] = LED_BLACK;
			}
			pos++;
			if(pos == NUM_LEDS) {
				pos = 0;
				color_offset = (color_offset + 1) % 6;
			}
		}
	}
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
	void maketrax_protection_w(uint8_t data);
	uint8_t maketrax_special_port2_r(unsigned short offset);
	uint8_t maketrax_special_port3_r(unsigned short offset);
	uint8_t m_maketrax_counter;
	uint8_t m_maketrax_offset;
	uint8_t m_maketrax_disable_protection;
	unsigned long timerSoundChanged;

#ifdef LED_PIN
	const CRGB menu_leds[7] = { LED_RED, LED_YELLOW, LED_GREEN, LED_CYAN, LED_BLUE, LED_MAGENTA, LED_WHITE };
#endif
};

#endif