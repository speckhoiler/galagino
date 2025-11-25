#ifndef _crush_dipswitches_h_
#define _crush_dipswitches_h_

#define CRUSH_DIP_FREE               0b00000000
#define CRUSH_DIP_1C1P               0b00000001
#define CRUSH_DIP_1C2P               0b00000010
#define CRUSH_DIP_2C1P               0b00000011

#define CRUSH_DIP_LIVE3              0b00000000
#define CRUSH_DIP_LIVE4              0b00000100
#define CRUSH_DIP_LIVE5              0b00001000
#define CRUSH_DIP_LIVE6              0b00001100

#define CRUSH_DIP_HARD               0b00000000
#define CRUSH_DIP_EASY               0b00010000

#define CRUSH_DIP_TELPORT_HOLES_OFF  0b00100000
#define CRUSH_DIP_TELPORT_HOLES_ON   0b00000000

#define CRUSH_DIP (CRUSH_DIP_1C1P | CRUSH_DIP_LIVE3 | CRUSH_DIP_EASY | CRUSH_DIP_TELPORT_HOLES_ON)

#endif