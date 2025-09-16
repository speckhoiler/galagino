#ifndef MRTNT_H
#define MRTNT_H

#include "mrtnt_rom.h"
#include "mrtnt_dipswitches.h"
#include "mrtnt_logo.h"
#include "mrtnt_tilemap.h"
#include "mrtnt_spritemap.h"
#include "mrtnt_cmap.h"
#include "mrtnt_wavetable.h"
#include "..\tileaddr.h"
#include "..\pacman\pacman.h"

class mrtnt : public pacman
{
public:
	mrtnt() { }
	~mrtnt() { }

	signed char machineType() override { return MCH_MRTNT; } 
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
};

#endif