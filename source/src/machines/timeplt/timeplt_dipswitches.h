#ifndef _timeplt_dipswitches_h_
#define _timeplt_dipswitches_h_

// Time Pilot DIP switches (MAME defaults are active-high)

// DSW1 (read at 0xC360) - Coinage
// bits 0-3: Coin A (0x0F = 1 coin 1 play)
// bits 4-7: Coin B (0xF0 = 1 coin 1 play)
// 0x00 = FREE PLAY!
#define TIMEPLT_DSW1  0xFF  // 1 coin 1 play both slots

// DSW2 (read at 0xC200)
// bits 0-1: Lives (0x03=3, 0x02=4, 0x01=5, 0x00=255)
// bit 2: Cabinet (0x00=Upright, 0x04=Cocktail)
// bit 3: Bonus (0x08=10K/50K, 0x00=20K/60K)
// bits 4-6: Difficulty (0x40=normal)
// bit 7: Demo sounds (0x00=ON, 0x80=OFF)

#define TIMEPILOT_DSW2_003_LIVES      0x03
#define TIMEPILOT_DSW2_004_LIVES      0x02
#define TIMEPILOT_DSW2_005_LIVES      0x01
#define TIMEPILOT_DSW2_255_LIVES      0x00
#define TIMEPILOT_DSW2_UPRIGHT        0x00
#define TIMEPILOT_DSW2_COCKTAIL       0x04
#define TIMEPILOT_DSW2_BONUS_10K_50K  0x08
#define TIMEPILOT_DSW2_BONUS_20K_60K  0x00

#define TIMEPILOT_DSW2_DIFFICULTY_1   0x70
#define TIMEPILOT_DSW2_DIFFICULTY_2   0x60
#define TIMEPILOT_DSW2_DIFFICULTY_3   0x50
#define TIMEPILOT_DSW2_DIFFICULTY_4   0x40
#define TIMEPILOT_DSW2_DIFFICULTY_5   0x30
#define TIMEPILOT_DSW2_DIFFICULTY_6   0x20 
#define TIMEPILOT_DSW2_DIFFICULTY_7   0x10
#define TIMEPILOT_DSW2_DIFFICULTY_8   0x00

#define TIMEPILOT_DSW2_DEMO_SOUND_ON  0x00
#define TIMEPILOT_DSW2_DEMO_SOUND_OFF 0x80

#define TIMEPLT_DSW2 (TIMEPILOT_DSW2_005_LIVES | TIMEPILOT_DSW2_UPRIGHT | TIMEPILOT_DSW2_BONUS_10K_50K | TIMEPILOT_DSW2_DIFFICULTY_4 | TIMEPILOT_DSW2_DEMO_SOUND_ON)

//#define TIMEPLT_DSW2  0x49  // 5 lives, upright, bonus 10K/50K, normal, demo sounds ON

// IN0 at 0xC300: coins, starts
// IN1 at 0xC320: P1 controls
// IN2 at 0xC340: P2 controls

#endif