#ifndef MSPACMAN_H
#define MSPACMAN_H

// Ms. Pac-Man CPU address map con decode switching (come MAME):
//   decode=true  (Ms.Pac-Man mode):
//     0x0000-0x3FFF: mspacman_pacrom (Pac-Man ROMs con 40 patch)
//     0x8000-0x9FFF: mspacman_auxrom (extension ROM Ms.Pac-Man)
//     0xA000-0xBFFF: mirror di mspacman_pacrom[0x2000-0x3FFF]
//   decode=false (Pac-Man mode):
//     0x0000-0x3FFF: pacman_rom (ROM originale non patchata)
//     0x8000-0xBFFF: mirror di pacman_rom[addr & 0x3FFF]
//
//   ENABLE  trap (decode=true):  lettura/scrittura 0x3FF8-0x3FFF
//   DISABLE trap (decode=false): lettura/scrittura
//     0x0038-0x003F, 0x03B0-0x03B7, 0x1600-0x1607,
//     0x2120-0x2127, 0x3FF0-0x3FF7, 0x8000-0x8007, 0x97F0-0x97F7
#include "../pacman/pacman.h"
#include "mspacman_pacrom.h"
#include "mspacman_auxrom.h"
#include "mspacman_tilemap.h"
#include "mspacman_spritemap.h"
#include "mspacman_logo.h"

class mspacman : public pacman
{
public:
  mspacman() { }
  ~mspacman() { }

  signed char machineType() override { return MCH_MSPACMAN; }
  void reset() override;
  unsigned char rdZ80(unsigned short Addr) override;
  void wrZ80(unsigned short Addr, unsigned char Value) override;
  unsigned char opZ80(unsigned short Addr) override;
  const unsigned short *logo(void) override;

protected:
  const unsigned short *tileRom(unsigned short addr) override;
  const unsigned long  *spriteRom(unsigned char flags, unsigned char code) override;

private:
  bool decode; // true = Ms. Pac-Man mode, false = original Pac-Man mode
};

#endif
