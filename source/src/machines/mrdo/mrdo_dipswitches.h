#ifndef _mrdo_dipswitches_h_
#define _mrdo_dipswitches_h_


#define MRDO_DIP_DIFFICULTY_EASY          0b00000011
#define MRDO_DIP_DIFFICULTY_MEDIUM        0b00000010
#define MRDO_DIP_DIFFICULTY_HARD          0b00000001
#define MRDO_DIP_DIFFICULTY_HARDEST       0b00000000

#define MRDO_DIP_RACK_TEST_OFF            0b00000100

#define MRDO_DIP_SPECIAL_EASY             0b00001000
#define MRDO_DIP_SPECIAL_HARD             0b00000000
#define MRDO_DIP_EXTRA_EASY               0b00010000
#define MRDO_DIP_EXTRA_HARD               0b00000000

#define MRDO_DIP_UPRIGHT                  0b00000000
#define MRDO_DIP_COCKTAIL                 0b00100000

#define MRDO_DIP_LIVES2                   0b00000000
#define MRDO_DIP_LIVES3                   0b11000000
#define MRDO_DIP_LIVES4                   0b10000000
#define MRDO_DIP_LIVES5                   0b01000000

#define MRDO_DIP1  (MRDO_DIP_DIFFICULTY_EASY | MRDO_DIP_RACK_TEST_OFF | MRDO_DIP_SPECIAL_EASY | MRDO_DIP_UPRIGHT | MRDO_DIP_LIVES3)


#define MRDO_DIP_COINA_4C1C               0b00000110
#define MRDO_DIP_COINA_3C1C               0b00001000
#define MRDO_DIP_COINA_2C1C               0b00001010
#define MRDO_DIP_COINA_3C2C               0b00000111
#define MRDO_DIP_COINA_1C1C               0b00001111
#define MRDO_DIP_COINA_2C3C               0b00001001
#define MRDO_DIP_COINA_1C2C               0b00001110
#define MRDO_DIP_COINA_1C3C               0b00001101
#define MRDO_DIP_COINA_1C4C               0b00001100
#define MRDO_DIP_COINA_1C5C               0b00001011
#define MRDO_DIP_COINA_FREE_PLAY          0b00000000

#define MRDO_DIP_COINB_4C1C               0b01100000
#define MRDO_DIP_COINB_3C1C               0b10000000
#define MRDO_DIP_COINB_2C1C               0b10100000
#define MRDO_DIP_COINB_3C2C               0b01110000
#define MRDO_DIP_COINB_1C1C               0b11110000
#define MRDO_DIP_COINB_2C3C               0b10010000
#define MRDO_DIP_COINB_1C2C               0b11100000
#define MRDO_DIP_COINB_1C3C               0b11010000
#define MRDO_DIP_COINB_1C4C               0b11000000
#define MRDO_DIP_COINB_1C5C               0b10110000
#define MRDO_DIP_COINB_FREE_PLAY          0b00000000

#define MRDO_DIP2  (MRDO_DIP_COINA_1C1C | MRDO_DIP_COINB_1C1C)

#endif