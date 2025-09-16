#ifndef _galaga_dipswitches_h_
#define _galaga_dipswitches_h_

#define GALAGA_DIPA_1C1CR     0b00000000
#define GALAGA_DIPA_1C2CR     0b00100000
#define GALAGA_DIPA_1C3CR     0b01000000
#define GALAGA_DIPA_2C1CR     0b10000000
#define GALAGA_DIPA_2C3CR     0b01100000
#define GALAGA_DIPA_3C1CR     0b10100000
#define GALAGA_DIPA_4C1CR     0b11000000
#define GALAGA_DIPA_FREE      0b11100000

// settings for lives != 5
#define GALAGA_DIPA_B20K60K   0b00000100  // bonus at 20k and 60k
#define GALAGA_DIPA_B20K60KE  0b00011000  // bonus at 20k, 60k and every 60k
#define GALAGA_DIPA_B20K70KE  0b00010100  // bonus at 20k, 70k and every 70k
#define GALAGA_DIPA_B20K80KE  0b00010000  // bonus at 20k, 80k and every 80k
#define GALAGA_DIPA_B30K80K   0b00000000  // bonus at 30k and 80k
#define GALAGA_DIPA_B30K100KE 0b00001100  // bonus at 30k, 100K and every 100k
#define GALAGA_DIPA_B30K120KE 0b00001000  // bonus at 30k, 120K and every 120k
#define GALAGA_DIPA_BNONE     0b00011100  // no bonus at all

// settings for lives == 5
#define GALAGA_DIPA_B30K100K  0b00001100  // bonus at 30k and 100k
#define GALAGA_DIPA_B30K120K  0b00001000  // bonus at 30k and 120k
#define GALAGA_DIPA_B30K150K  0b00000100  // bonus at 30k and 150k
#define GALAGA_DIPA_B30K      0b00000000  // bonus at 30k
#define GALAGA_DIPA_B30K100Ke 0b00011000  // bonus at 30k, 100k and every 100k
#define GALAGA_DIPA_B30K120Ke 0b00010100  // bonus at 30k, 120k and every 120k
#define GALAGA_DIPA_B30K150Ke 0b00010000  // bonus at 30k, 150k and every 150k

#define GALAGA_DIPA_LIVE2     0b00000011
#define GALAGA_DIPA_LIVE3     0b00000010
#define GALAGA_DIPA_LIVE4     0b00000001
#define GALAGA_DIPA_LIVE5     0b00000000

#define GALAGA_DIPB_RANK_A    0b00000000
#define GALAGA_DIPB_RANK_B    0b11000000
#define GALAGA_DIPB_RANK_C    0b01000000
#define GALAGA_DIPB_RANK_D    0b10000000

#define GALAGA_DIPB_RACKTEST  0b00100000

#define GALAGA_DIPB_DEMO_SND  0b00010000

#define GALAGA_DIPB_FREEZE    0b00001000

#define GALAGA_DIPB_TABLE     0b00000001
#define GALAGA_DIPB_UPRIGHT   0b00000000

#define GALAGA_DIPA (GALAGA_DIPA_1C1CR | GALAGA_DIPA_B20K70KE | GALAGA_DIPA_LIVE3)

#define GALAGA_DIPB (GALAGA_DIPB_RANK_A | GALAGA_DIPB_UPRIGHT)

#endif