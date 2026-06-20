#ifndef SUPERCOBRA_DIPSWITCHES_H
#define SUPERCOBRA_DIPSWITCHES_H

// Fire1 - Laser
// Fire2 - Bomb
// (c)   - Player2 Cocktail

// bit | 0     | 1      | 2       | 3      | 4     | 5    | 6     | 7     |
//     | Up(c) | Bomb2  | Service | Laser1 | Right | Left | Coin2 | Coin1 |
#define SUPERCOBRA_IN0_IDLE 0b11111111

// bit | 0        | 1      | 2        | 3        | 4        | 5       | 6      | 7      |
//     | Continue | Lives  | Bomb2(c) | Laser1(c)| Right(c) | Left(c) | Start2 | Start1 |
#define SUPERCOBRA_IN1_IDLE       0b11111100
#define SUPERCOBRA_IN1_CONTINUE_0 0b00000000
#define SUPERCOBRA_IN1_CONTINUE_4 0b00000001
#define SUPERCOBRA_IN1_3_LIVES    0b00000000
#define SUPERCOBRA_IN1_4_LIVES    0b00000010

// bit | 0       | 1 | 2 | 3       | 4  | 5          | 6    | 7          |
//     | Down(c) | Coins | Cabinet | Up | Protection | Down | Protection |
#define SUPERCOBRA_IN2_IDLE0 0b01010001 
#define SUPERCOBRA_IN2_IDLE1 0b11110001 

#define SUPERCOBRA_IN2_1C_1C    0b00000010
#define SUPERCOBRA_IN2_1C_2C    0b00000100
#define SUPERCOBRA_IN2_4C_3C    0b00000110
#define SUPERCOBRA_IN2_FREE_99C 0b00000000
#define SUPERCOBRA_IN2_UPRIGHT  0b00000000
#define SUPERCOBRA_IN2_COCKTAIL 0b00001000

#define SUPERCOBRA_IN0_VALUE  (SUPERCOBRA_IN0_IDLE)
#define SUPERCOBRA_IN1_VALUE  (SUPERCOBRA_IN1_IDLE | SUPERCOBRA_IN1_CONTINUE_4 | SUPERCOBRA_IN1_4_LIVES)
#define SUPERCOBRA_IN2_VALUE0 (SUPERCOBRA_IN2_IDLE0 | SUPERCOBRA_IN2_1C_1C | SUPERCOBRA_IN2_UPRIGHT)
#define SUPERCOBRA_IN2_VALUE1 (SUPERCOBRA_IN2_IDLE1 | SUPERCOBRA_IN2_1C_1C | SUPERCOBRA_IN2_UPRIGHT)

#endif
