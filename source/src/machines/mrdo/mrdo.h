#ifndef MRDO_H
#define MRDO_H

#include "mrdo_rom1.h"
#include "mrdo_dipswitches.h"
#include "mrdo_logo.h"
#include "mrdo_bg_tiles.h"
#include "mrdo_fg_tiles.h"
#include "mrdo_sprites.h"
#include "mrdo_sprite_colormap.h"
#include "mrdo_palette.h"
#include "../tileaddr.h"
#include "../machineBase.h"

#define SPRITE_FLIP_X 0x01
#define SPRITE_FLIP_Y 0x02

class mrdo : public machineBase
{
public:
	mrdo() { }
	~mrdo() { }

	void reset() override;
	signed char machineType() override { return MCH_MRDO; }
    signed char videoFlipY() override { return 1; }
	signed char useVideoHalfRate() override { return 1; } 

	unsigned char opZ80(unsigned short Addr) override; 
	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;

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
	unsigned char protection_r();
	void protection_w(unsigned char data);
	void render_background_strip(short screen_strip_row);
	void SN76489_Write_2chip(int chip, unsigned char data);

	unsigned char m_pal_u001 = 0xFF;
	unsigned char flipscreen_w = 0; // 0 = normale, 1 = flip attivo
	unsigned char scrollx_w = 0;
	unsigned char scrolly_w = 0;

	unsigned char ignoreFireButton;
    int sn_last_register[2];

#ifdef LED_PIN
	const CRGB menu_leds[7] = { LED_RED, LED_GREEN, LED_YELLOW, LED_YELLOW, LED_YELLOW, LED_GREEN, LED_RED };
#endif

};

#endif