#ifndef DKONG3_H
#define DKONG3_H

#include "dkong3_tilemap.h"
#include "dkong3_spritemap.h"
#include "dkong3_cmap.h"
#include "dkong3_color_codes.h"
#include "dkong3_rom.h"
#include "dkong3_logo.h"
#include "dkong3_sound_roms.h"
#include "../../cpus/m6502/m6502.h"
#include "../tileaddr.h"
#include "../machineBase.h"

#define DK3_DSW1 0x00
#define DK3_DSW2 0x00

// Minimal NES APU state — no dynamic allocation, ~90 bytes per instance
struct dk3_apu_t {
    // Pulse channels (2)
    uint8_t  p_ctrl[2];     // $4000/$4004: DDlcVVVV
    uint16_t p_timer[2];    // 11-bit period
    bool     p_en[2];       // enabled ($4015 bits 0-1)
    int      p_cnt[2];      // synthesis countdown
    uint8_t  p_seq[2];      // duty sequencer 0-7
    uint8_t  p_env_vol[2];  // current envelope volume (0-15)
    uint8_t  p_env_cnt[2];  // envelope divider counter
    bool     p_env_rst[2];  // restart flag — set on $4003/$4007 write
    uint8_t  p_len[2];      // length counter (decrements at 120 Hz)
    uint8_t  p_sweep[2];    // $4001/$4005: EPPPNSSS
    uint8_t  p_sweep_cnt[2];// sweep divider counter
    bool     p_sweep_rst[2];// sweep reload flag — set on $4001/$4005 write

    // Triangle
    uint16_t t_timer;
    bool     t_en;
    int      t_cnt;
    uint8_t  t_seq;         // 0-31
    uint8_t  t_lin_ctrl;    // $4008: bit7=halt/loop, bits0-6=linear counter reload
    uint8_t  t_lin;         // linear counter current value (decrements at 240 Hz)
    bool     t_lin_reload;  // reload flag — set on $400B write
    uint8_t  t_len;         // length counter (decrements at 120 Hz)

    // Noise
    uint8_t  n_ctrl;        // $400C full byte: DDlcVVVV
    uint8_t  n_period_idx;  // 4-bit index from $400E
    bool     n_mode;        // bit 7 of $400E
    bool     n_en;          // $4015 bit 3
    int      n_cnt;
    uint16_t n_lfsr;        // 15-bit LFSR
    uint8_t  n_env_vol;     // current envelope volume
    uint8_t  n_env_cnt;     // envelope divider counter
    bool     n_env_rst;     // restart flag — set on $400F write
    uint8_t  n_len;         // length counter (decrements at 120 Hz)
};

class dkong3 : public machineBase
{
public:
    dkong3() { memset(snd_cpu, 0, sizeof(snd_cpu)); }
    ~dkong3() { }

    void reset() override;
    void start() override;

    signed char machineType() override { return MCH_DKONG3; }
    unsigned char rdZ80(unsigned short Addr) override;
    void wrZ80(unsigned short Addr, unsigned char Value) override;
    unsigned char opZ80(unsigned short Addr) override;

    void prepare_frame(void) override;
    void run_frame(void) override;
    void render_row(short row) override;
    const unsigned short *logo(void) override;

#ifdef LED_PIN
    void menuLeds(CRGB *leds) override;
    void gameLeds(CRGB *leds) override;
#endif

    // Public: accessed by audio.cpp
    static const int DK3_SAMPLES = 1024;
    uint16_t dk3_rptr = 0;
    uint16_t dk3_wptr = 0;
    short    dk3_samples[DK3_SAMPLES];

protected:
    void blit_tile(short row, char col) override;
    void blit_sprite(short row, unsigned char s) override;

private:
    // Video state
    unsigned char palette_bank = 0;
    unsigned char gfx_bank     = 0;
    unsigned char flip_screen  = 0;

    // Memory regions — pointers into machineBase::memory (16 KB)
    // Layout: [0x0000-0x0FFF] general_ram (4 KB)
    //         [0x1000-0x13FF] sprite_ram  (1 KB)
    //         [0x1400-0x17FF] video_ram   (1 KB)
    unsigned char *general_ram;
    unsigned char *sprite_ram;
    unsigned char *video_ram;

    // Sound latches (Z80 writes, RP2A03 reads)
    // latch[0]: $7C00 → CPU A $4016
    // latch[1]: $7C80 → CPU A $4017
    // latch[2]: $7D00 → CPU B $4016
    unsigned char sound_latch[3];

    uint8_t coin_latch = 0;  // counts down reads; bit HIGH only for first N reads after press
    bool    coin_prev  = false; // previous BUTTON_COIN state for rising-edge detection

    // RP2A03 internal RAM (2 KB each)
    unsigned char snd_ram[2][2048];

    // Sound CPUs and APUs
    m6502_t    snd_cpu[2];
    dk3_apu_t  apu[2];

    // 6502 memory callbacks
    static uint8_t snd0_read (m6502_t *cpu, uint16_t addr);
    static void    snd0_write(m6502_t *cpu, uint16_t addr, uint8_t val);
    static uint8_t snd1_read (m6502_t *cpu, uint16_t addr);
    static void    snd1_write(m6502_t *cpu, uint16_t addr, uint8_t val);

#ifdef LED_PIN
    const CRGB menu_leds[7] = { LED_GREEN, LED_BLUE, LED_YELLOW, LED_WHITE, LED_YELLOW, LED_BLUE, LED_GREEN };
#endif
};

#endif
