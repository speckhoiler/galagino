#ifndef BOMBJACK_H
#define BOMBJACK_H

#include "bombjack_rom1.h"
#include "bombjack_rom2.h"
#include "bombjack_dipswitches.h"
#include "bombjack_logo.h"
#include "bombjack_bg_maps.h"
#include "bombjack_bg_tiles.h"
#include "bombjack_fg_tiles.h"
#include "bombjack_sprites.h"
#include "../tileaddr.h"
#include "../machineBase.h"

#define PORT_P1_IN      0xB000  // P1 Controls (JOYSTICK + BUTTON)
#define PORT_P2_IN      0xB001  // P2 Controls (per modalità COCKTAIL)
#define PORT_SYSTEM_IN  0xB002  // System Inputs (COIN + START)
#define PORT_WATCHDOG   0xB003  // Watchdog Reset (lettura resetta il timer)
#define PORT_DSW1_IN    0xB004  // DIP Switch 1
#define PORT_DSW2_IN    0xB005  // DIP Switch 2

class bombjack : public machineBase
{
public:
	bombjack() { }
	~bombjack() { }

	void reset() override;
	signed char machineType() override { return MCH_BOMBJACK; } 
	signed char useVideoHalfRate() override { return 1; } 

	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	void outZ80(unsigned short Port, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;
	unsigned char inZ80(unsigned short Port) override;

	void run_frame(void) override;
	void prepare_frame(void) override;
	void render_row(short row) override;
    const unsigned short *logo(void) override;

#ifdef LED_PIN	
	void menuLeds(CRGB *leds) override;
	void gameLeds(CRGB *leds) override;
#endif

protected:
	void blit_tile_bg(short logical_row);
	void blit_tile_fg(short row, char col);
	void blit_sprite(short row, unsigned char s_idx);

private:
	uint16_t bombjack_palette[128];
	uint8_t palette[256];

	unsigned char sound_latch; // Scritta dalla Main CPU, letta dalla Audio CPU
	unsigned char sound_latch_backup = 0;
	unsigned char ay_address[3];

	unsigned char m_bg_image;  // Contiene il codice dello sfondo e il bit di visibilità
	bool m_nmi_on;              // Flag per abilitare l'interrupt NMI
	bool m_mmi_skip_audio_cpu;
	bool m_flip;                // Flag per lo screen flip

#ifdef LED_PIN
	const CRGB menu_leds[7] = { LED_RED, LED_GREEN, LED_YELLOW, LED_YELLOW, LED_YELLOW, LED_GREEN, LED_RED };
#endif

};

#endif