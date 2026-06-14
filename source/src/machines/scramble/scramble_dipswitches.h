#ifndef SCRAMBLE_DIPSWITCHES_H
#define SCRAMBLE_DIPSWITCHES_H

// Fire1 - Laser
// Fire2 - Bomb
// (c)   - Player2 Cocktail

// bit | 0     | 1      | 2       | 3      | 4     | 5    | 6     | 7     |
//     | Up(c) | Bomb2  | Service | Laser1 | Right | Left | Coin2 | Coin1 |
#define SCRAMBLE_IN0_IDLE 0b11111111

// bit | 0  | 1 | 2        | 3        | 4        | 5       | 6      | 7      |
//     | Lives  | Bomb2(c) | Laser1(c)| Right(c) | Left(c) | Start2 | Start1 |
#define SCRAMBLE_IN1_IDLE     0b11111100
#define SCRAMBLE_IN1_3_LIVES  0b00000000
#define SCRAMBLE_IN1_4_LIVES  0b00000001
#define SCRAMBLE_IN1_5_LIVES  0b00000010
#define SCRAMBLE_IN1_FREE     0b00000011

// bit | 0       | 1 | 2 | 3       | 4  | 5          | 6    | 7          |
//     | Down(c) | Coins | Cabinet | Up | Protection | Down | Protection |
#define SCRAMBLE_IN2_IDLE0    0b01010001 
#define SCRAMBLE_IN2_IDLE1    0b11110001 

#define SCRAMBLE_IN2_1C_1C    0b00000000
#define SCRAMBLE_IN2_1C_2C    0b00000010
#define SCRAMBLE_IN2_1C_3C    0b00000100
#define SCRAMBLE_IN2_1C_4C    0b00000110
#define SCRAMBLE_IN2_UPRIGHT  0b00000000
#define SCRAMBLE_IN2_COCKTAIL 0b00001000

#define SCRAMBLE_IN0_VALUE  (SCRAMBLE_IN0_IDLE)
#define SCRAMBLE_IN1_VALUE  (SCRAMBLE_IN1_IDLE | SCRAMBLE_IN1_5_LIVES)
#define SCRAMBLE_IN2_VALUE0 (SCRAMBLE_IN2_IDLE0 | SCRAMBLE_IN2_1C_1C | SCRAMBLE_IN2_UPRIGHT)
#define SCRAMBLE_IN2_VALUE1 (SCRAMBLE_IN2_IDLE1 | SCRAMBLE_IN2_1C_1C | SCRAMBLE_IN2_UPRIGHT)

#endif
