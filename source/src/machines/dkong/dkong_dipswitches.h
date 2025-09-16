#ifndef _dkong_dipswitches_h_
#define _dkong_dipswitches_h_

#define DKONG_DIP_COCKTAIL    0b00000000
#define DKONG_DIP_UPRIGHT     0b10000000

#define DKONG_DIP_1C1P        0b00000000
#define DKONG_DIP_2C1P        0b00010000
#define DKONG_DIP_1C2P        0b00100000
#define DKONG_DIP_3C1P        0b00110000
#define DKONG_DIP_1C3P        0b01000000
#define DKONG_DIP_4C3P        0b01010000
#define DKONG_DIP_1C4P        0b01100000
#define DKONG_DIP_4C1P        0b01110000

#define DKONG_DIP_B7K         0b00000000
#define DKONG_DIP_B10K        0b00000100
#define DKONG_DIP_B15K        0b00001000
#define DKONG_DIP_B20K        0b00001100

#define DKONG_DIP_LIVE3       0b00000000
#define DKONG_DIP_LIVE4       0b00000001
#define DKONG_DIP_LIVE5       0b00000010
#define DKONG_DIP_LIVE6       0b00000011

#define DKONG_DIP  (DKONG_DIP_UPRIGHT | DKONG_DIP_1C1P | DKONG_DIP_B7K | DKONG_DIP_LIVE3 )

#endif