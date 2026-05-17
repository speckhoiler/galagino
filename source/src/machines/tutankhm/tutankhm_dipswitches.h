#ifndef _tutankhm_dipswitches_h_
#define _tutankhm_dipswitches_h_

// Tutankham DIP switches (from MAME tutankhm.cpp)

// DSW1 (read at 0x81E0) - Coinage
// KONAMI_COINAGE_LOC defaults: 0xFF = 1 coin 1 credit both slots
#define TUTANKHM_DSW1  0xFF

// DSW2 (read at 0x8160) - Game settings
// bit 0-1: Lives        (0x03 = 3 lives)
// bit 2:   Cabinet      (0x00 = Upright)
// bit 3:   Bonus life   (0x08 = 30000)
// bit 4-5: Difficulty   (0x20 = Normal)
// bit 6:   Flash Bomb   (0x40 = 1 per Life)
// bit 7:   Demo Sounds  (0x00 = On)
// Default: 0x02|0x00|0x08|0x20|0x40|0x00 = 0x6A (5 lives)
#define TUTANKHM_DSW2  0x6A

#define TUTANKHM_DSW2_DEMO_SOUND_ON  0x00
#define TUTANKHM_DSW2_DEMO_SOUND_OFF 0x80

#endif
