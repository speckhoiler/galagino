#ifndef TUTANKHM_H
#define TUTANKHM_H

#include "../machineBase.h"
#include "tutankhm_rom.h"
#include "tutankhm_bank_rom.h"
#include "tutankhm_snd_rom.h"
#include "tutankhm_dipswitches.h"
#include "tutankhm_logo.h"

// Tutankham (Konami/Stern 1982)
//   Main CPU: MC6809E @ 1.5 MHz
//   Sound: timeplt_audio (Z80 @ 1.789 MHz + 2x AY-3-8910)
//   Video: 256x256 bitmap, 4bpp, ROT90
//
// Memory layout in shared memory[] buffer:
//   0x0000-0x07FF: Work RAM (2KB, maps to CPU 0x8800-0x8FFF)
//   0x0800-0x080F: Palette RAM (16 bytes, maps to CPU 0x8000-0x800F)
//   Total: 0x0810 = 2064 bytes (fits in RAMSIZE 9344)
//
// Video RAM (32KB) is allocated separately in PSRAM.

#define TUT_WORKRAM   0x0000
#define TUT_PALETTE   0x0800

class tutankhm : public machineBase
{
public:
    tutankhm() { }
    ~tutankhm() { }

    void start() override;
    void reset() override;

    signed char machineType() override { return MCH_TUTANKHM; }

    unsigned char rdZ80(unsigned short Addr) override;
    void wrZ80(unsigned short Addr, unsigned char Value) override;
    unsigned char opZ80(unsigned short Addr) override;
   
    unsigned char m6809_read(m6809_state *s, uint16_t addr) override;
    void m6809_write(m6809_state *s, uint16_t addr, uint8_t val) override;
    unsigned char m6809_read_opcode(m6809_state *s, uint16_t addr) override;

    void run_frame(void) override;
    void prepare_frame(void) override;
    void render_row(short row) override;
    const unsigned short *logo(void) override;

#ifdef LED_PIN
    void menuLeds(CRGB *leds) override;
    void gameLeds(CRGB *leds) override;
#endif

private:
    // Helper: convert palette byte to RGB565 (byte-swapped for SPI)
    unsigned short palette_to_rgb565(uint8_t val);    
    
    // M6809 main CPU
    m6809_state main_cpu;

    // Video RAM (32KB bitmap, allocated from PSRAM)
    uint8_t *videoram = nullptr;

    // Palette cache (16 entries, pre-computed RGB565 byte-swapped)
    unsigned short palette_rgb565[16];

    // Control registers
    unsigned char irq_enable;
    unsigned char irq_toggle;   // fires IRQ every other frame
    unsigned char scroll_reg;   // scroll register at 0x8100
    unsigned char soundlatch;

    // Sound CPU state (timeplt_audio clone)
    unsigned char snd_ram[1024];
    unsigned char snd_irq_pending;
    unsigned char ay_addr[2];
    unsigned char ay_regs[2][16];
    unsigned long snd_icnt;
    unsigned char snd_irq_last;
    
    // ROM bank select
    unsigned char bank_select;

#ifdef LED_PIN
    const CRGB menu_leds[7] = { LED_YELLOW, LED_WHITE, LED_YELLOW, LED_WHITE, LED_YELLOW, LED_WHITE, LED_YELLOW };
#endif
};

#endif
