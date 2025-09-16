#ifndef _eyes_dipswitches_h_
#define _eyes_dipswitches_h_

#define EYES_DIP_FREE       0b00000000
#define EYES_DIP_2C1P       0b00000001
#define EYES_DIP_1C2P       0b00000010
#define EYES_DIP_1C1P       0b00000011

#define EYES_DIP_LIVE2      0b00010000
#define EYES_DIP_LIVE3      0b00001000
#define EYES_DIP_LIVE4      0b00000100
#define EYES_DIP_LIVE5      0b00000000

#define EYES_DIP_B125K      0b00000000
#define EYES_DIP_B100K      0b00100000
#define EYES_DIP_B75K       0b01000000
#define EYES_DIP_B50K       0b01100000

#define EYES_DIP_UPRIGHT    0b10000000
#define EYES_DIP_NORMAL     0b00000000

#define EYES_DIP (EYES_DIP_UPRIGHT | EYES_DIP_B50K | EYES_DIP_LIVE3 | EYES_DIP_1C1P)

#endif