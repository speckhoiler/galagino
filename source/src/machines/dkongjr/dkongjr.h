#ifndef DKONGJR_H
#define DKONGJR_H

#include "dkongjr_rom1.h"
#include "dkongjr_rom2.h"
#include "dkongjr_logo.h"
#include "dkongjr_tilemap.h"
#include "dkongjr_spritemap.h"
#include "dkongjr_cmap.h"
#include "../dkong/dkong.h"
#include "../tileaddr.h"
#include "../machineBase.h"

class dkongjr : public dkong
{
public:
	dkongjr() { }
	~dkongjr() { }

	signed char machineType() override { return MCH_DKONGJR; } 
	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;

	unsigned char rdI8048_xdm(struct i8048_state_S *state, unsigned char addr) override;
	unsigned char rdI8048_rom(struct i8048_state_S *state, unsigned short addr) override;

	void prepare_frame(void) override;
	const unsigned short *logo(void) override;

protected:
 	void blit_tile(short row, char col) override;
	void blit_sprite(short row, unsigned char s) override;

private:
	unsigned char palette_bank = 0; // bank-switching   byte (8 bit) 256 valori massimi  Banco 0: Contiene i tile dall'indice 0 al 255. Banco 1: Contiene i tile dall'indice 256 al 511.
	unsigned char gfx_bank = 0; // bank-switching   byte (8 bit) 256 valori massimi  Banco 0: Contiene i tile dall'indice 0 al 255. Banco 1: Contiene i tile dall'indice 256 al 511.
	unsigned char flip_screen = 0;//penso venga usata per tabletop io non penso di usarla
};

#endif