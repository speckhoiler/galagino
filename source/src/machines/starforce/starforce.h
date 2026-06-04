#ifndef STARFORCE_H
#define STARFORCE_H

#include "starforce_logo.h"
#include "starforce_bg1_tiles.h"
#include "starforce_bg2_tiles.h"
#include "starforce_bg3_tiles.h"
#include "starforce_fg_tiles.h"
#include "starforce_sprites.h"
#include "starforce_main_cpu_rom.h"
#include "starforce_sub_cpu_rom.h"
#include "starforce_dipswitches.h"
#include "../tileaddr.h"
#include "../machineBase.h"

// Offsets
#define STARFORCE_GENERAL_RAM   	0x0000	//1000
#define STARFORCE_FG_VIDEO_RAM  	0x1000	//400
#define STARFORCE_FG_COLOR_RAM  	0x1400	//400
#define STARFORCE_SPRITE_RAM  		0x1800	//80
#define STARFORCE_PALETTE_RAM  		0x1880	//200
#define STARFORCE_HW_CONTROL_RAM  	0x1A80	//40
#define STARFORCE_BG3_VIDEO_RAM  	0x1AC0	//800
#define STARFORCE_BG2_VIDEO_RAM  	0x22C0	//800
#define STARFORCE_BG1_VIDEO_RAM  	0x2AC0	//800
#define STARFORCE_RADAR_RAM  		0x32C0	//400
#define STARFORCE_SOUND_RAM  		0x36C0	//400

// with BG3 the game is running too slow (up to 220ms for 10 frames)
//#define ENABLE_BG3

class starforce : public machineBase
{
public:
	starforce() { }
	~starforce() { }

	signed char machineType() override { return MCH_STARFORCE; }
	signed char useVideoHalfRate() override { return 1; } 

	unsigned char opZ80(unsigned short Addr) override; 
	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
    unsigned char inZ80(unsigned short Port) override; 
	void outZ80(unsigned short Port, unsigned char Value) override;

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
	void blit_sprite(short row, unsigned char s_idx, unsigned char target_priority);

private:
    unsigned short calculate_color_starforce(unsigned char raw_palette_byte);
    void blit_background_line(short start_screen_row, int layer_num);
	void SN76489_Write_3chip(int chip, unsigned char data);
	int sn_last_register[3];

	// Latch per la comunicazione tra CPU principale e CPU audio
	unsigned char sound_latch = 0;
	unsigned char sound_latch_pending = 0; 
	unsigned char sound_irq_toggle = 0;
	
	// Aggiungi queste variabili per i DSW e gli input, se non le hai già
	unsigned short starforce_palette[512];

	unsigned char coinBackup = 0;
	unsigned char coinFrameCounter = 0;

	#ifdef LED_PIN
		const CRGB menu_leds[7] = { LED_BLUE, LED_CYAN, LED_BLUE, LED_WHITE, LED_BLUE, LED_CYAN, LED_BLUE };
	#endif

	};

	#endif