#ifndef _ladybug_dipswitches_h_
#define _ladybug_dipswitches_h_

// Lady Bug DIP switches - values match MAME defaults (read directly, no inversion)
//
// DSW0 layout:
//   Bits 0-1: Difficulty  (0x03=Easy, 0x02=Medium, 0x01=Hard, 0x00=Hardest)
//   Bit 2:    High Score Names (0x04=Yes, 0x00=No)
//   Bit 3:    Rack Test  (0x08=Off, 0x00=On)
//   Bit 4:    Freeze     (0x10=Off, 0x00=On)
//   Bit 5:    Cabinet    (0x00=Upright, 0x20=Cocktail)
//   Bit 6:    Unused     (always 1)
//   Bits 6-7: Lives      (0xC0=3, 0x80=4, 0x40=5, 0x00=Infinite)
//
// Default: Easy, HSN=Yes, RackTest=Off, Freeze=Off, Upright, 5 lives
// = 0x03 | 0x04 | 0x08 | 0x10 | 0x00 | 0x40 = 0x5F
#define LADYBUG_DSW0  0x5F

// DSW1 layout:
//   Bits 0-3: Coin B  (0x0F=1C/1Credit, others=various rates)
//   Bits 4-7: Coin A  (0xF0=1C/1Credit, others=various rates)
//
// Default: 1 coin = 1 credit for both slots = 0xFF
#define LADYBUG_DSW1  0xFF

#endif
