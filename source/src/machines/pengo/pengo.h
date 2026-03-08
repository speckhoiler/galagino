#ifndef PENGO_H
#define PENGO_H

#include "pengo_rom.h"
#include "pengo_dipswitches.h"
#include "pengo_logo.h"
#include "pengo_tiles.h"
#include "pengo_spritemap.h"
#include "pengo_colormap.h"
#include "pengo_wavetable.h"
#include "../tileaddr.h"
#include "../pacman/pacman.h"

#define VIDEORAM_BASE   (0x8000 - 0x8000)
#define COLORRAM_BASE   (0x8400 - 0x8000)
#define SPRITERAM_BASE  (0x8FF0 - 0x8000)
#define SPRITEPOS_BASE  (0x9022 - 0x8000) // Coordinate X,Y di 6 sprites

class pengo : public pacman
{
public:
	pengo() { }
	~pengo() { }

	signed char machineType() override { return MCH_PENGO; } 
	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;

	void run_frame(void) override;
	void prepare_frame(void) override;
	void render_row(short row) override;
	const signed char *waveRom(unsigned char value) override;
	const unsigned short *logo(void) override;	

private:
	unsigned char sprite_coords[16]; // NUOVO: Array separato per le coordinate
	int gfx_bank = 0; // PENGO: Banco per tile e sprite (0 o 1)
	int palette_bank = 0;
	int colortable_bank = 0; 
	char flipscreen = 0;// Indirizzi base nella RAM emulata (memory[])
	char coinFrameCounter = 0;
	char coinBackup = 0x00;

protected:
	void blit_tile(short row, char col) override;
	void blit_sprite(short row, unsigned char s) override;

};

#endif