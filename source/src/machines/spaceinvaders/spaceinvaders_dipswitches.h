#ifndef _spaceinvaders_dipswitches_h_
#define _spaceinvaders_dipswitches_h_

// Space Invaders DIP switches (active on Port 2 read)
// Bits 0-1: Number of ships
#define SINV_DIP_SHIPS_3      0b00000000
#define SINV_DIP_SHIPS_4      0b00000001
#define SINV_DIP_SHIPS_5      0b00000010
#define SINV_DIP_SHIPS_6      0b00000011

// Bit 3: Extra ship at
#define SINV_DIP_EXTRA_1500   0b00000000
#define SINV_DIP_EXTRA_1000   0b00001000

// Bit 7: Coin info display
#define SINV_DIP_COININFO_ON  0b00000000
#define SINV_DIP_COININFO_OFF 0b10000000

#define SINV_DIP  (SINV_DIP_SHIPS_5 | SINV_DIP_EXTRA_1500 | SINV_DIP_COININFO_OFF)

#endif
