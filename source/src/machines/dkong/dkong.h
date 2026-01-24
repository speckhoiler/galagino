#ifndef DKONG_H
#define DKONG_H

#define DKONG_AUDIO_QUEUE_LEN   16
#define DKONG_AUDIO_QUEUE_MASK (DKONG_AUDIO_QUEUE_LEN-1)

#include "dkong_rom1.h"
#include "dkong_rom2.h"
#include "dkong_dipswitches.h"
#include "dkong_logo.h"
#include "dkong_tilemap.h"
#include "dkong_spritemap.h"
#include "dkong_cmap.h"
#include "dkong_sample_walk0.h"
#include "dkong_sample_walk1.h"
#include "dkong_sample_walk2.h"
#include "dkong_sample_jump.h"
#include "dkong_sample_stomp.h"
#include "../tileaddr.h"
#include "../machineBase.h"

class dkong : public machineBase
{
public:
	dkong() { }
	~dkong() { }

 	void reset() override;
	signed char machineType() override { return MCH_DKONG; } 
	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;

	void wrI8048_port(struct i8048_state_S *state, unsigned char port, unsigned char pos) override;
	unsigned char rdI8048_port(struct i8048_state_S *state, unsigned char port) override;
	unsigned char rdI8048_xdm(struct i8048_state_S *state, unsigned char addr) override;
	unsigned char rdI8048_rom(struct i8048_state_S *state, unsigned short addr) override;

	void run_frame(void) override;
	void prepare_frame(void) override;
	void render_row(short row) override;
	const unsigned short *logo(void) override;
#ifdef LED_PIN
	void menuLeds(CRGB *leds) override;
	void gameLeds(CRGB *leds) override;
#endif

	unsigned char dkong_obuf_toggle = 0;
	unsigned char dkong_audio_transfer_buffer[DKONG_AUDIO_QUEUE_LEN][64];
	unsigned char dkong_audio_rptr = 0, dkong_audio_wptr = 0;
	unsigned short dkong_sample_cnt[3] = { 0,0,0 };
	const signed char *dkong_sample_ptr[3];

protected:
 	void blit_tile(short row, char col) override;
	void blit_sprite(short row, unsigned char s) override;

private:
	void trigger_sound(char snd);

#ifdef LED_PIN
	const CRGB menu_leds[7] = { LED_BLACK, LED_YELLOW, LED_RED, LED_RED, LED_RED, LED_YELLOW, LED_BLACK };
#endif
	// the audio cpu is a mb8884 which in turn is 8048/49 compatible
	i8048_state_S cpu_8048;
	
	// special variables for dkong
	unsigned char colortable_select = 0;

	// sound effects register between 8048 and z80
	unsigned char dkong_sfx_index = 0x00;

	unsigned char dkong_audio_assembly_buffer[64];
};

#endif